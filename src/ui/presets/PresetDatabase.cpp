#include "../../../include/ui/presets/PresetDatabase.h"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>

namespace AIMusicHardware {

PresetDatabase::PresetDatabase() {
    stats_.totalPresets = 0;
    stats_.totalCategories = 0;
    stats_.totalAuthors = 0;
    stats_.totalFavorites = 0;
    stats_.cacheHitRate = 0;
}

PresetDatabase::~PresetDatabase() {
    shouldStopScanning_ = true;
    if (scanThread_ && scanThread_->joinable()) {
        updateCondition_.notify_all();
        scanThread_->join();
    }
}

bool PresetDatabase::initialize(const std::vector<std::string>& directories) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Clear existing data
    presets_.clear();
    watchedDirectories_.clear();
    
    // Add directories
    for (const auto& dir : directories) {
        if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
            watchedDirectories_.push_back(dir);
        } else {
            std::cerr << "Warning: Directory does not exist: " << dir << std::endl;
        }
    }
    
    if (watchedDirectories_.empty()) {
        return false;
    }
    
    // Start background scanning
    shouldStopScanning_ = false;
    scanThread_ = std::make_unique<std::thread>(&PresetDatabase::scanDirectoriesBackground, this);
    
    return true;
}

bool PresetDatabase::addDirectory(const std::string& directory, bool recursive) {
    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        
        // Check if already being watched
        auto it = std::find(watchedDirectories_.begin(), watchedDirectories_.end(), directory);
        if (it != watchedDirectories_.end()) {
            return true; // Already watching
        }
        
        watchedDirectories_.push_back(directory);
    }
    
    // Scan the new directory
    scanDirectory(directory, recursive);
    rebuildIndicesInternal();
    
    // Notify listeners
    if (updateCallback_) {
        updateCallback_(getAllPresets());
    }
    
    return true;
}

void PresetDatabase::removeDirectory(const std::string& directory) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    // Remove from watched directories
    auto it = std::find(watchedDirectories_.begin(), watchedDirectories_.end(), directory);
    if (it != watchedDirectories_.end()) {
        watchedDirectories_.erase(it);
    }
    
    // Remove all presets from this directory
    auto presetIt = presets_.begin();
    while (presetIt != presets_.end()) {
        if (presetIt->second.filePath.find(directory) == 0) {
            removeFromIndices(presetIt->first);
            presetIt = presets_.erase(presetIt);
        } else {
            ++presetIt;
        }
    }
    
    updateStatistics();
}

std::vector<PresetInfo> PresetDatabase::getAllPresets() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<PresetInfo> result;
    result.reserve(presets_.size());
    
    for (const auto& pair : presets_) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<PresetInfo> PresetDatabase::searchByName(const std::string& query) const {
    if (query.empty()) {
        return getAllPresets();
    }
    
    std::lock_guard<std::mutex> indexLock(indexMutex_);
    std::lock_guard<std::mutex> dataLock(dataMutex_);
    
    std::vector<PresetInfo> result;
    std::string lowerQuery = toLowercase(query);
    
    // Use index for fast lookup
    auto range = nameIndex_.equal_range(lowerQuery);
    for (auto it = range.first; it != range.second; ++it) {
        auto presetIt = presets_.find(it->second);
        if (presetIt != presets_.end()) {
            result.push_back(presetIt->second);
        }
    }
    
    // Also search for partial matches
    for (auto it = nameIndex_.begin(); it != nameIndex_.end(); ++it) {
        if (it->first.find(lowerQuery) != std::string::npos) {
            auto presetIt = presets_.find(it->second);
            if (presetIt != presets_.end()) {
                // Avoid duplicates
                if (std::find_if(result.begin(), result.end(), 
                    [&](const PresetInfo& p) { return p.filePath == presetIt->second.filePath; }) == result.end()) {
                    result.push_back(presetIt->second);
                }
            }
        }
    }
    
    return result;
}

std::vector<PresetInfo> PresetDatabase::getByCategory(const std::string& category) const {
    std::lock_guard<std::mutex> indexLock(indexMutex_);
    std::lock_guard<std::mutex> dataLock(dataMutex_);
    
    std::vector<PresetInfo> result;
    
    auto range = categoryIndex_.equal_range(category);
    for (auto it = range.first; it != range.second; ++it) {
        auto presetIt = presets_.find(it->second);
        if (presetIt != presets_.end()) {
            result.push_back(presetIt->second);
        }
    }
    
    return result;
}

std::vector<PresetInfo> PresetDatabase::getByAuthor(const std::string& author) const {
    std::lock_guard<std::mutex> indexLock(indexMutex_);
    std::lock_guard<std::mutex> dataLock(dataMutex_);
    
    std::vector<PresetInfo> result;
    
    auto range = authorIndex_.equal_range(author);
    for (auto it = range.first; it != range.second; ++it) {
        auto presetIt = presets_.find(it->second);
        if (presetIt != presets_.end()) {
            result.push_back(presetIt->second);
        }
    }
    
    return result;
}

std::vector<PresetInfo> PresetDatabase::getFavorites() const {
    std::lock_guard<std::mutex> indexLock(indexMutex_);
    std::lock_guard<std::mutex> dataLock(dataMutex_);
    
    std::vector<PresetInfo> result;
    
    for (const auto& filePath : favoriteIndex_) {
        auto presetIt = presets_.find(filePath);
        if (presetIt != presets_.end()) {
            result.push_back(presetIt->second);
        }
    }
    
    return result;
}

std::vector<PresetInfo> PresetDatabase::filter(const PresetFilterCriteria& criteria) const {
    if (!criteria.hasActiveFilters()) {
        return getAllPresets();
    }
    
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    std::vector<PresetInfo> result;
    
    for (const auto& pair : presets_) {
        if (matchesFilter(pair.second, criteria)) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

void PresetDatabase::sort(std::vector<PresetInfo>& presets, 
                         PresetSortCriteria criteria, 
                         SortDirection direction) const {
    
    auto comparator = [criteria, direction](const PresetInfo& a, const PresetInfo& b) -> bool {
        bool result = false;
        
        switch (criteria) {
            case PresetSortCriteria::Name:
                result = a.name < b.name;
                break;
            case PresetSortCriteria::Author:
                result = a.author < b.author;
                break;
            case PresetSortCriteria::Category:
                result = a.category < b.category;
                break;
            case PresetSortCriteria::DateCreated:
                result = a.created < b.created;
                break;
            case PresetSortCriteria::DateModified:
                result = a.modified < b.modified;
                break;
            case PresetSortCriteria::Favorites:
                result = a.isFavorite && !b.isFavorite;
                break;
            case PresetSortCriteria::Rating:
                result = a.userRating < b.userRating;
                break;
            case PresetSortCriteria::PlayCount:
                result = a.playCount < b.playCount;
                break;
            case PresetSortCriteria::FileSize:
                result = a.fileSize < b.fileSize;
                break;
        }
        
        return direction == SortDirection::Ascending ? result : !result;
    };
    
    std::sort(presets.begin(), presets.end(), comparator);
}

std::shared_ptr<PresetInfo> PresetDatabase::getPreset(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    auto it = presets_.find(filePath);
    if (it != presets_.end()) {
        cacheHits_++;
        return std::make_shared<PresetInfo>(it->second);
    }
    
    cacheMisses_++;
    
    // Try to load from file if not in cache
    if (isValidPresetFile(filePath)) {
        PresetInfo info = PresetInfo::fromFile(filePath);
        return std::make_shared<PresetInfo>(info);
    }
    
    return nullptr;
}

bool PresetDatabase::updatePreset(const std::string& filePath, const PresetInfo& updatedInfo) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    auto it = presets_.find(filePath);
    if (it != presets_.end()) {
        // Remove from indices
        removeFromIndices(filePath);
        
        // Update preset
        it->second = updatedInfo;
        
        // Add back to indices
        addToIndices(updatedInfo);
        
        updateStatistics();
        return true;
    }
    
    return false;
}

bool PresetDatabase::addPreset(const PresetInfo& presetInfo) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    presets_[presetInfo.filePath] = presetInfo;
    addToIndices(presetInfo);
    updateStatistics();
    
    return true;
}

bool PresetDatabase::removePreset(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    
    auto it = presets_.find(filePath);
    if (it != presets_.end()) {
        removeFromIndices(filePath);
        presets_.erase(it);
        updateStatistics();
        return true;
    }
    
    return false;
}

void PresetDatabase::rebuildIndices() {
    rebuildIndicesInternal();
}

void PresetDatabase::rebuildIndicesInternal() {
    std::lock_guard<std::mutex> indexLock(indexMutex_);
    std::lock_guard<std::mutex> dataLock(dataMutex_);
    
    // Clear existing indices
    nameIndex_.clear();
    categoryIndex_.clear();
    authorIndex_.clear();
    tagIndex_.clear();
    favoriteIndex_.clear();
    
    // Rebuild indices
    for (const auto& pair : presets_) {
        addToIndices(pair.second);
    }
    
    updateStatistics();
}

std::set<std::string> PresetDatabase::getAllCategories() const {
    std::lock_guard<std::mutex> lock(indexMutex_);
    
    std::set<std::string> categories;
    for (const auto& pair : categoryIndex_) {
        categories.insert(pair.first);
    }
    
    return categories;
}

std::set<std::string> PresetDatabase::getAllAuthors() const {
    std::lock_guard<std::mutex> lock(indexMutex_);
    
    std::set<std::string> authors;
    for (const auto& pair : authorIndex_) {
        authors.insert(pair.first);
    }
    
    return authors;
}

std::set<std::string> PresetDatabase::getAllTags() const {
    std::lock_guard<std::mutex> lock(indexMutex_);
    
    std::set<std::string> tags;
    for (const auto& pair : tagIndex_) {
        tags.insert(pair.first);
    }
    
    return tags;
}

PresetDatabase::Statistics PresetDatabase::getStatistics() const {
    std::lock_guard<std::mutex> lock(dataMutex_);
    return stats_;
}

void PresetDatabase::setUpdateCallback(UpdateCallback callback) {
    updateCallback_ = callback;
}

bool PresetDatabase::isUpdating() const {
    return isScanning_.load();
}

bool PresetDatabase::waitForUpdate(int timeoutMs) const {
    std::unique_lock<std::mutex> lock(updateMutex_);
    return updateCondition_.wait_for(lock, std::chrono::milliseconds(timeoutMs), 
                                   [this] { return !isScanning_.load(); });
}

// Private methods implementation

void PresetDatabase::scanDirectoriesBackground() {
    isScanning_ = true;
    
    try {
        for (const auto& directory : watchedDirectories_) {
            if (shouldStopScanning_) break;
            scanDirectory(directory, true);
        }
        
        rebuildIndicesInternal();
        
        if (updateCallback_) {
            updateCallback_(getAllPresets());
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during background scanning: " << e.what() << std::endl;
    }
    
    isScanning_ = false;
    updateCondition_.notify_all();
}

void PresetDatabase::scanDirectory(const std::string& directory, bool recursive) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (shouldStopScanning_) break;
            
            if (entry.is_regular_file() && isValidPresetFile(entry.path().string())) {
                processPresetFile(entry.path().string());
            } else if (recursive && entry.is_directory()) {
                scanDirectory(entry.path().string(), true);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error scanning directory " << directory << ": " << e.what() << std::endl;
    }
}

void PresetDatabase::processPresetFile(const std::string& filePath) {
    try {
        PresetInfo info = loadPresetMetadata(filePath);
        
        std::lock_guard<std::mutex> lock(dataMutex_);
        presets_[filePath] = info;
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing preset file " << filePath << ": " << e.what() << std::endl;
    }
}

void PresetDatabase::addToIndices(const PresetInfo& preset) {
    // Name index (lowercase for case-insensitive search)
    nameIndex_.emplace(toLowercase(preset.name), preset.filePath);
    
    // Category index
    if (!preset.category.empty()) {
        categoryIndex_.emplace(preset.category, preset.filePath);
    }
    
    // Author index
    if (!preset.author.empty()) {
        authorIndex_.emplace(preset.author, preset.filePath);
    }
    
    // Tag index
    for (const auto& tag : preset.tags) {
        tagIndex_.emplace(tag, preset.filePath);
    }
    
    // Favorites index
    if (preset.isFavorite) {
        favoriteIndex_.insert(preset.filePath);
    }
}

void PresetDatabase::removeFromIndices(const std::string& filePath) {
    // This is inefficient but necessary for correctness
    // In a production system, you might want to maintain reverse indices
    
    auto removeFromMultimap = [&filePath](auto& multimap) {
        auto it = multimap.begin();
        while (it != multimap.end()) {
            if (it->second == filePath) {
                it = multimap.erase(it);
            } else {
                ++it;
            }
        }
    };
    
    removeFromMultimap(nameIndex_);
    removeFromMultimap(categoryIndex_);
    removeFromMultimap(authorIndex_);
    removeFromMultimap(tagIndex_);
    
    favoriteIndex_.erase(filePath);
}

std::string PresetDatabase::toLowercase(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool PresetDatabase::matchesSearch(const std::string& text, const std::string& query) const {
    return toLowercase(text).find(toLowercase(query)) != std::string::npos;
}

bool PresetDatabase::matchesFilter(const PresetInfo& preset, const PresetFilterCriteria& criteria) const {
    // Search text filter
    if (!criteria.searchText.empty()) {
        bool found = matchesSearch(preset.name, criteria.searchText) ||
                    matchesSearch(preset.author, criteria.searchText) ||
                    matchesSearch(preset.description, criteria.searchText);
        
        // Also search tags
        for (const auto& tag : preset.tags) {
            if (matchesSearch(tag, criteria.searchText)) {
                found = true;
                break;
            }
        }
        
        if (!found) return false;
    }
    
    // Category filter
    if (!criteria.categories.empty()) {
        bool found = std::find(criteria.categories.begin(), criteria.categories.end(), 
                              preset.category) != criteria.categories.end();
        if (!found) return false;
    }
    
    // Author filter
    if (!criteria.authors.empty()) {
        bool found = std::find(criteria.authors.begin(), criteria.authors.end(), 
                              preset.author) != criteria.authors.end();
        if (!found) return false;
    }
    
    // Tags filter
    if (!criteria.tags.empty()) {
        bool found = false;
        for (const auto& criteriaTag : criteria.tags) {
            if (std::find(preset.tags.begin(), preset.tags.end(), criteriaTag) != preset.tags.end()) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    // Favorites filter
    if (criteria.favoritesOnly && !preset.isFavorite) {
        return false;
    }
    
    // Rating filter
    if (preset.userRating < criteria.minRating) {
        return false;
    }
    
    // Date range filter
    if (criteria.hasDateRange) {
        if (preset.created < criteria.dateFrom || preset.created > criteria.dateTo) {
            return false;
        }
    }
    
    // Audio characteristics filter
    if (criteria.hasAudioFilter) {
        if (preset.audioCharacteristics.bassContent < criteria.minBassContent ||
            preset.audioCharacteristics.bassContent > criteria.maxBassContent ||
            preset.audioCharacteristics.brightness < criteria.minBrightness ||
            preset.audioCharacteristics.brightness > criteria.maxBrightness) {
            return false;
        }
    }
    
    return true;
}

PresetInfo PresetDatabase::loadPresetMetadata(const std::string& filePath) const {
    // Check cache first
    {
        std::lock_guard<std::mutex> cacheLock(cacheMutex_);
        auto it = metadataCache_.find(filePath);
        if (it != metadataCache_.end()) {
            cacheHits_++;
            return it->second;
        }
    }
    
    cacheMisses_++;
    
    // Load from file
    PresetInfo info = PresetInfo::fromFile(filePath);
    
    // Update cache
    updateCache(info);
    
    return info;
}

void PresetDatabase::updateCache(const PresetInfo& preset) const {
    std::lock_guard<std::mutex> cacheLock(cacheMutex_);
    metadataCache_[preset.filePath] = preset;
    
    // Simple cache size management (keep last 1000 entries)
    if (metadataCache_.size() > 1000) {
        auto it = metadataCache_.begin();
        metadataCache_.erase(it);
    }
}

bool PresetDatabase::isValidPresetFile(const std::string& filePath) const {
    std::filesystem::path path(filePath);
    std::string extension = path.extension().string();
    
    // Support common preset file extensions
    return extension == ".json" || 
           extension == ".preset" || 
           extension == ".vital" ||
           extension == ".vitalbank";
}

void PresetDatabase::updateStatistics() const {
    stats_.totalPresets = presets_.size();
    stats_.totalCategories = getAllCategories().size();
    stats_.totalAuthors = getAllAuthors().size();
    stats_.totalFavorites = favoriteIndex_.size();
    
    if (cacheHits_ + cacheMisses_ > 0) {
        stats_.cacheHitRate = (cacheHits_ * 100) / (cacheHits_ + cacheMisses_);
    }
    
    stats_.lastUpdateTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch());
}

} // namespace AIMusicHardware