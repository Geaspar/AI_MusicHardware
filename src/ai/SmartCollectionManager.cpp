#include "ai/SmartCollectionManager.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <random>
#include <cmath>

namespace AIMusicHardware {

// CollectionRule implementation

float CollectionRule::evaluate(const PresetInfo& preset, const AudioFeatureVector& features) const {
    switch (type) {
        case Type::AudioCharacteristic: {
            float featureValue = 0.0f;
            
            // Map parameter name to audio feature
            if (parameter == "brightness") featureValue = features.brightness;
            else if (parameter == "warmth") featureValue = features.warmth;
            else if (parameter == "bassContent") featureValue = features.energyBands[0] + features.energyBands[1];
            else if (parameter == "complexity") featureValue = features.oscillatorComplexity;
            else if (parameter == "filterResonance") featureValue = features.filterResonance;
            else if (parameter == "lfoDepth") featureValue = features.lfoDepth;
            else return 0.0f; // Unknown parameter
            
            // Apply operation
            if (operation == "greater_than") {
                return featureValue > value ? 1.0f : 0.0f;
            } else if (operation == "less_than") {
                return featureValue < value ? 1.0f : 0.0f;
            } else if (operation == "equals") {
                return std::abs(featureValue - value) < 0.1f ? 1.0f : 0.0f;
            } else if (operation == "range") {
                // Use stringValue as max value for range
                float maxValue = std::stof(stringValue);
                return (featureValue >= value && featureValue <= maxValue) ? 1.0f : 0.0f;
            }
            break;
        }
        
        case Type::Category: {
            if (operation == "equals") {
                return preset.category == stringValue ? 1.0f : 0.0f;
            } else if (operation == "contains") {
                return preset.category.find(stringValue) != std::string::npos ? 1.0f : 0.0f;
            }
            break;
        }
        
        case Type::Author: {
            if (operation == "equals") {
                return preset.author == stringValue ? 1.0f : 0.0f;
            } else if (operation == "contains") {
                return preset.author.find(stringValue) != std::string::npos ? 1.0f : 0.0f;
            }
            break;
        }
        
        case Type::Tag: {
            for (const auto& tag : preset.tags) {
                if (operation == "equals" && tag == stringValue) return 1.0f;
                if (operation == "contains" && tag.find(stringValue) != std::string::npos) return 1.0f;
            }
            return 0.0f;
        }
        
        case Type::UserBehavior: {
            if (parameter == "favorite") {
                return preset.isFavorite ? 1.0f : 0.0f;
            } else if (parameter == "rating") {
                if (operation == "greater_than") {
                    return preset.userRating > static_cast<int>(value) ? 1.0f : 0.0f;
                } else if (operation == "equals") {
                    return preset.userRating == static_cast<int>(value) ? 1.0f : 0.0f;
                }
            } else if (parameter == "playCount") {
                if (operation == "greater_than") {
                    return preset.playCount > static_cast<int>(value) ? 1.0f : 0.0f;
                }
            }
            break;
        }
        
        case Type::Temporal: {
            auto now = std::chrono::system_clock::now();
            auto ageHours = std::chrono::duration_cast<std::chrono::hours>(now - preset.created).count();
            
            if (parameter == "age_hours") {
                if (operation == "less_than") {
                    return ageHours < value ? 1.0f : 0.0f;
                } else if (operation == "greater_than") {
                    return ageHours > value ? 1.0f : 0.0f;
                }
            }
            break;
        }
        
        case Type::Custom: {
            if (customEvaluator) {
                return customEvaluator(preset, features) ? 1.0f : 0.0f;
            }
            break;
        }
        
        default:
            break;
    }
    
    return 0.0f;
}

// SmartCollection implementation

float SmartCollection::calculateScore(const PresetInfo& preset, const AudioFeatureVector& features) const {
    float totalScore = 0.0f;
    float totalWeight = 0.0f;
    
    for (const auto& rule : rules) {
        float ruleScore = rule.evaluate(preset, features);
        totalScore += ruleScore * rule.weight;
        totalWeight += rule.weight;
    }
    
    return totalWeight > 0.0f ? totalScore / totalWeight : 0.0f;
}

bool SmartCollection::shouldInclude(const PresetInfo& preset, const AudioFeatureVector& features) const {
    float score = calculateScore(preset, features);
    return score >= minScore;
}

// CollectionTemplate implementation

SmartCollection CollectionTemplate::createCollection(const std::string& collectionName) const {
    SmartCollection collection;
    collection.name = collectionName;
    collection.description = description;
    collection.rules = defaultRules;
    collection.sortBy = defaultSortBy;
    collection.iconName = iconName;
    collection.color = color;
    collection.tags = suggestedTags;
    collection.lastUpdated = std::chrono::system_clock::now();
    
    return collection;
}

// SmartPlaylist implementation

std::string SmartPlaylist::getNext() {
    if (presetPaths.empty()) return "";
    
    if (shuffleMode) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, static_cast<int>(presetPaths.size()) - 1);
        currentIndex = dis(gen);
    } else {
        currentIndex = (currentIndex + 1) % static_cast<int>(presetPaths.size());
        if (!loopMode && currentIndex == 0 && presetPaths.size() > 1) {
            currentIndex = static_cast<int>(presetPaths.size()); // Past end
            return "";
        }
    }
    
    return currentIndex < static_cast<int>(presetPaths.size()) ? presetPaths[currentIndex] : "";
}

std::string SmartPlaylist::getPrevious() {
    if (presetPaths.empty()) return "";
    
    if (shuffleMode) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, static_cast<int>(presetPaths.size()) - 1);
        currentIndex = dis(gen);
    } else {
        currentIndex = currentIndex > 0 ? currentIndex - 1 : 
                      (loopMode ? static_cast<int>(presetPaths.size()) - 1 : -1);
        if (currentIndex < 0) return "";
    }
    
    return currentIndex >= 0 && currentIndex < static_cast<int>(presetPaths.size()) ? 
           presetPaths[currentIndex] : "";
}

void SmartPlaylist::updateSmartSuggestions(PresetRecommendationEngine& recommendationEngine) {
    if (!enableSmartSuggestions || presetPaths.empty()) {
        suggestedPaths.clear();
        return;
    }
    
    // Create recommendation context from playlist
    RecommendationContext context;
    context.recentPresets = presetPaths;
    context.sessionType = workflowType;
    context.maxRecommendations = maxSuggestions;
    context.diversityWeight = diversityLevel;
    
    // Add context tags
    for (const auto& tag : contextTags) {
        context.tags.push_back(tag);
    }
    
    // Get recommendations
    auto recommendations = recommendationEngine.getRecommendations(context);
    
    // Extract preset paths
    suggestedPaths.clear();
    for (const auto& rec : recommendations) {
        // Don't suggest presets already in the playlist
        if (std::find(presetPaths.begin(), presetPaths.end(), rec.presetPath) == presetPaths.end()) {
            suggestedPaths.push_back(rec.presetPath);
        }
    }
    
    lastModified = std::chrono::system_clock::now();
}

// SmartCollectionManager implementation

SmartCollectionManager::SmartCollectionManager(std::shared_ptr<PresetMLAnalyzer> analyzer,
                                               std::shared_ptr<PresetRecommendationEngine> recommendationEngine)
    : analyzer_(analyzer)
    , recommendationEngine_(recommendationEngine) {
    
    if (!analyzer_) {
        throw std::invalid_argument("PresetMLAnalyzer cannot be null");
    }
    if (!recommendationEngine_) {
        throw std::invalid_argument("PresetRecommendationEngine cannot be null");
    }
    
    createDefaultTemplates();
}

SmartCollectionManager::~SmartCollectionManager() = default;

std::string SmartCollectionManager::createSmartCollection(const std::string& name,
                                                         const std::string& description,
                                                         const std::vector<CollectionRule>& rules) {
    SmartCollection collection;
    collection.id = generateCollectionId();
    collection.name = name;
    collection.description = description;
    collection.rules = rules;
    collection.lastUpdated = std::chrono::system_clock::now();
    
    collections_[collection.id] = collection;
    updateStatistics();
    
    return collection.id;
}

std::string SmartCollectionManager::createFromTemplate(const std::string& templateName,
                                                       const std::string& collectionName) {
    auto it = templates_.find(templateName);
    if (it == templates_.end()) {
        return ""; // Template not found
    }
    
    SmartCollection collection = it->second.createCollection(collectionName);
    collection.id = generateCollectionId();
    
    collections_[collection.id] = collection;
    updateStatistics();
    
    return collection.id;
}

bool SmartCollectionManager::updateCollectionRules(const std::string& collectionId,
                                                   const std::vector<CollectionRule>& rules) {
    auto it = collections_.find(collectionId);
    if (it == collections_.end()) {
        return false;
    }
    
    it->second.rules = rules;
    it->second.lastUpdated = std::chrono::system_clock::now();
    
    return true;
}

bool SmartCollectionManager::deleteSmartCollection(const std::string& collectionId) {
    auto it = collections_.find(collectionId);
    if (it == collections_.end() || it->second.isSystem) {
        return false; // Not found or system collection
    }
    
    collections_.erase(it);
    updateStatistics();
    
    return true;
}

std::vector<SmartCollection> SmartCollectionManager::getAllCollections() const {
    std::vector<SmartCollection> result;
    result.reserve(collections_.size());
    
    for (const auto& [id, collection] : collections_) {
        if (collection.isVisible) {
            result.push_back(collection);
        }
    }
    
    // Sort by name
    std::sort(result.begin(), result.end(),
        [](const SmartCollection& a, const SmartCollection& b) {
            return a.name < b.name;
        });
    
    return result;
}

const SmartCollection* SmartCollectionManager::getCollection(const std::string& collectionId) const {
    auto it = collections_.find(collectionId);
    return it != collections_.end() ? &it->second : nullptr;
}

void SmartCollectionManager::updateAllCollections(const std::vector<PresetInfo>& presets,
                                                  std::function<void(int, int)> progressCallback) {
    int current = 0;
    int total = static_cast<int>(collections_.size());
    
    for (auto& [id, collection] : collections_) {
        if (collection.autoUpdate) {
            updateCollection(id, presets);
        }
        
        if (progressCallback) {
            progressCallback(++current, total);
        }
    }
    
    updateStatistics();
}

bool SmartCollectionManager::updateCollection(const std::string& collectionId,
                                             const std::vector<PresetInfo>& presets) {
    auto it = collections_.find(collectionId);
    if (it == collections_.end()) {
        return false;
    }
    
    SmartCollection& collection = it->second;
    
    // Clear current contents
    collection.presetPaths.clear();
    collection.presetScores.clear();
    
    // Evaluate each preset against collection rules
    std::vector<std::pair<std::string, float>> scoredPresets;
    
    for (const auto& preset : presets) {
        AudioFeatureVector features = analyzer_->extractFeatures(preset);
        
        if (collection.shouldInclude(preset, features)) {
            float score = collection.calculateScore(preset, features);
            scoredPresets.emplace_back(preset.filePath, score);
            collection.presetScores[preset.filePath] = score;
        }
    }
    
    // Sort by score (descending)
    std::sort(scoredPresets.begin(), scoredPresets.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Limit to max size
    if (static_cast<int>(scoredPresets.size()) > collection.maxSize) {
        scoredPresets.resize(collection.maxSize);
    }
    
    // Extract preset paths
    collection.presetPaths.reserve(scoredPresets.size());
    for (const auto& [presetPath, score] : scoredPresets) {
        collection.presetPaths.push_back(presetPath);
    }
    
    collection.lastUpdated = std::chrono::system_clock::now();
    
    return true;
}

std::vector<std::string> SmartCollectionManager::addPresetToCollections(const PresetInfo& preset) {
    std::vector<std::string> updatedCollections;
    AudioFeatureVector features = analyzer_->extractFeatures(preset);
    
    for (auto& [id, collection] : collections_) {
        if (collection.autoUpdate && collection.shouldInclude(preset, features)) {
            float score = collection.calculateScore(preset, features);
            
            // Check if collection is at max size
            if (static_cast<int>(collection.presetPaths.size()) >= collection.maxSize) {
                // Find lowest scoring preset and replace if new preset scores higher
                auto minIt = std::min_element(collection.presetScores.begin(), collection.presetScores.end(),
                    [](const auto& a, const auto& b) { return a.second < b.second; });
                
                if (minIt != collection.presetScores.end() && score > minIt->second) {
                    // Remove lowest scoring preset
                    auto removeIt = std::find(collection.presetPaths.begin(), collection.presetPaths.end(), minIt->first);
                    if (removeIt != collection.presetPaths.end()) {
                        collection.presetPaths.erase(removeIt);
                    }
                    collection.presetScores.erase(minIt);
                }
            }
            
            // Add new preset if there's room or we made room
            if (static_cast<int>(collection.presetPaths.size()) < collection.maxSize) {
                collection.presetPaths.push_back(preset.filePath);
                collection.presetScores[preset.filePath] = score;
                collection.lastUpdated = std::chrono::system_clock::now();
                updatedCollections.push_back(id);
            }
        }
    }
    
    return updatedCollections;
}

void SmartCollectionManager::removePresetFromCollections(const std::string& presetPath) {
    for (auto& [id, collection] : collections_) {
        auto it = std::find(collection.presetPaths.begin(), collection.presetPaths.end(), presetPath);
        if (it != collection.presetPaths.end()) {
            collection.presetPaths.erase(it);
            collection.presetScores.erase(presetPath);
            collection.lastUpdated = std::chrono::system_clock::now();
        }
    }
}

// Smart playlist methods

std::string SmartCollectionManager::createSmartPlaylist(const std::string& name,
                                                        const std::string& description,
                                                        const std::string& workflowType) {
    SmartPlaylist playlist;
    playlist.id = generatePlaylistId();
    playlist.name = name;
    playlist.description = description;
    playlist.workflowType = workflowType;
    playlist.created = std::chrono::system_clock::now();
    playlist.lastModified = playlist.created;
    
    playlists_[playlist.id] = playlist;
    updateStatistics();
    
    return playlist.id;
}

bool SmartCollectionManager::addToPlaylist(const std::string& playlistId,
                                          const std::string& presetPath,
                                          int position) {
    auto it = playlists_.find(playlistId);
    if (it == playlists_.end()) {
        return false;
    }
    
    SmartPlaylist& playlist = it->second;
    
    if (position < 0 || position >= static_cast<int>(playlist.presetPaths.size())) {
        playlist.presetPaths.push_back(presetPath);
    } else {
        playlist.presetPaths.insert(playlist.presetPaths.begin() + position, presetPath);
    }
    
    playlist.lastModified = std::chrono::system_clock::now();
    
    // Update smart suggestions
    if (playlist.enableSmartSuggestions) {
        playlist.updateSmartSuggestions(*recommendationEngine_);
    }
    
    return true;
}

bool SmartCollectionManager::removeFromPlaylist(const std::string& playlistId,
                                               const std::string& presetPath) {
    auto it = playlists_.find(playlistId);
    if (it == playlists_.end()) {
        return false;
    }
    
    SmartPlaylist& playlist = it->second;
    auto removeIt = std::find(playlist.presetPaths.begin(), playlist.presetPaths.end(), presetPath);
    
    if (removeIt != playlist.presetPaths.end()) {
        playlist.presetPaths.erase(removeIt);
        playlist.lastModified = std::chrono::system_clock::now();
        
        // Update smart suggestions
        if (playlist.enableSmartSuggestions) {
            playlist.updateSmartSuggestions(*recommendationEngine_);
        }
        
        return true;
    }
    
    return false;
}

void SmartCollectionManager::updatePlaylistSuggestions(const std::string& playlistId) {
    auto it = playlists_.find(playlistId);
    if (it != playlists_.end()) {
        it->second.updateSmartSuggestions(*recommendationEngine_);
    }
}

std::vector<SmartPlaylist> SmartCollectionManager::getAllPlaylists() const {
    std::vector<SmartPlaylist> result;
    result.reserve(playlists_.size());
    
    for (const auto& [id, playlist] : playlists_) {
        result.push_back(playlist);
    }
    
    // Sort by name
    std::sort(result.begin(), result.end(),
        [](const SmartPlaylist& a, const SmartPlaylist& b) {
            return a.name < b.name;
        });
    
    return result;
}

const SmartPlaylist* SmartCollectionManager::getPlaylist(const std::string& playlistId) const {
    auto it = playlists_.find(playlistId);
    return it != playlists_.end() ? &it->second : nullptr;
}

// Template management

void SmartCollectionManager::registerTemplate(const std::string& templateName,
                                              const CollectionTemplate& templateDef) {
    templates_[templateName] = templateDef;
}

std::unordered_map<std::string, CollectionTemplate> SmartCollectionManager::getTemplates() const {
    return templates_;
}

void SmartCollectionManager::createDefaultTemplates() {
    // Bright Presets template
    {
        CollectionTemplate brightTemplate;
        brightTemplate.name = "Bright Presets";
        brightTemplate.description = "Presets with high brightness and clarity";
        brightTemplate.iconName = "brightness_high";
        brightTemplate.color = "#FFD700";
        brightTemplate.defaultSortBy = SmartCollection::SortBy::Score;
        
        CollectionRule brightnessRule;
        brightnessRule.type = CollectionRule::Type::AudioCharacteristic;
        brightnessRule.parameter = "brightness";
        brightnessRule.operation = "greater_than";
        brightnessRule.value = 0.7f;
        brightnessRule.weight = 1.0f;
        
        brightTemplate.defaultRules = {brightnessRule};
        templates_["Bright Presets"] = brightTemplate;
    }
    
    // Bass Presets template
    {
        CollectionTemplate bassTemplate;
        bassTemplate.name = "Bass Presets";
        bassTemplate.description = "Presets with strong low-frequency content";
        bassTemplate.iconName = "volume_up";
        bassTemplate.color = "#8B0000";
        bassTemplate.defaultSortBy = SmartCollection::SortBy::Score;
        
        CollectionRule bassRule;
        bassRule.type = CollectionRule::Type::AudioCharacteristic;
        bassRule.parameter = "bassContent";
        bassRule.operation = "greater_than";
        bassRule.value = 0.6f;
        bassRule.weight = 1.0f;
        
        bassTemplate.defaultRules = {bassRule};
        templates_["Bass Presets"] = bassTemplate;
    }
    
    // Favorites template
    {
        CollectionTemplate favoritesTemplate;
        favoritesTemplate.name = "Favorites";
        favoritesTemplate.description = "Your favorite presets";
        favoritesTemplate.iconName = "favorite";
        favoritesTemplate.color = "#FF1744";
        favoritesTemplate.defaultSortBy = SmartCollection::SortBy::DateAdded;
        
        CollectionRule favoriteRule;
        favoriteRule.type = CollectionRule::Type::UserBehavior;
        favoriteRule.parameter = "favorite";
        favoriteRule.operation = "equals";
        favoriteRule.value = 1.0f;
        favoriteRule.weight = 1.0f;
        
        favoritesTemplate.defaultRules = {favoriteRule};
        templates_["Favorites"] = favoritesTemplate;
    }
    
    // Recent Presets template
    {
        CollectionTemplate recentTemplate;
        recentTemplate.name = "Recent Presets";
        recentTemplate.description = "Recently added presets";
        recentTemplate.iconName = "schedule";
        recentTemplate.color = "#00BCD4";
        recentTemplate.defaultSortBy = SmartCollection::SortBy::DateCreated;
        
        CollectionRule recentRule;
        recentRule.type = CollectionRule::Type::Temporal;
        recentRule.parameter = "age_hours";
        recentRule.operation = "less_than";
        recentRule.value = 168.0f; // 1 week
        recentRule.weight = 1.0f;
        
        recentTemplate.defaultRules = {recentRule};
        templates_["Recent Presets"] = recentTemplate;
    }
    
    // Complex Presets template
    {
        CollectionTemplate complexTemplate;
        complexTemplate.name = "Complex Presets";
        complexTemplate.description = "Presets with complex synthesis and modulation";
        complexTemplate.iconName = "settings";
        complexTemplate.color = "#9C27B0";
        complexTemplate.defaultSortBy = SmartCollection::SortBy::Score;
        
        CollectionRule complexityRule;
        complexityRule.type = CollectionRule::Type::AudioCharacteristic;
        complexityRule.parameter = "complexity";
        complexityRule.operation = "greater_than";
        complexityRule.value = 0.6f;
        complexityRule.weight = 0.7f;
        
        CollectionRule lfoRule;
        lfoRule.type = CollectionRule::Type::AudioCharacteristic;
        lfoRule.parameter = "lfoDepth";
        lfoRule.operation = "greater_than";
        lfoRule.value = 0.5f;
        lfoRule.weight = 0.3f;
        
        complexTemplate.defaultRules = {complexityRule, lfoRule};
        templates_["Complex Presets"] = complexTemplate;
    }
}

// Search and discovery methods

std::vector<std::string> SmartCollectionManager::findCollectionsWithPreset(const std::string& presetPath) const {
    std::vector<std::string> result;
    
    for (const auto& [id, collection] : collections_) {
        auto it = std::find(collection.presetPaths.begin(), collection.presetPaths.end(), presetPath);
        if (it != collection.presetPaths.end()) {
            result.push_back(id);
        }
    }
    
    return result;
}

std::vector<std::string> SmartCollectionManager::searchCollections(const std::string& query) const {
    std::vector<std::string> result;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& [id, collection] : collections_) {
        std::string lowerName = collection.name;
        std::string lowerDesc = collection.description;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);
        
        if (lowerName.find(lowerQuery) != std::string::npos ||
            lowerDesc.find(lowerQuery) != std::string::npos) {
            result.push_back(id);
        }
    }
    
    return result;
}

std::vector<std::pair<std::string, float>> SmartCollectionManager::getSimilarCollections(
    const std::string& collectionId,
    int maxResults) const {
    
    auto it = collections_.find(collectionId);
    if (it == collections_.end()) {
        return {};
    }
    
    const SmartCollection& reference = it->second;
    std::vector<std::pair<std::string, float>> similarities;
    
    for (const auto& [otherId, otherCollection] : collections_) {
        if (otherId == collectionId) continue;
        
        float similarity = calculateCollectionSimilarity(reference, otherCollection);
        if (similarity > 0.1f) { // Minimum similarity threshold
            similarities.emplace_back(otherId, similarity);
        }
    }
    
    // Sort by similarity (descending)
    std::sort(similarities.begin(), similarities.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Limit results
    if (static_cast<int>(similarities.size()) > maxResults) {
        similarities.resize(maxResults);
    }
    
    return similarities;
}

// Analytics and insights

SmartCollectionManager::CollectionStats SmartCollectionManager::getStatistics() const {
    updateStatistics();
    return stats_;
}

std::vector<std::string> SmartCollectionManager::getCollectionInsights(const std::string& collectionId) const {
    auto it = collections_.find(collectionId);
    if (it == collections_.end()) {
        return {};
    }
    
    return generateInsights(it->second);
}

// Data persistence

nlohmann::json SmartCollectionManager::exportData() const {
    nlohmann::json data;
    
    // Export collections
    for (const auto& [id, collection] : collections_) {
        nlohmann::json collectionJson;
        collectionJson["id"] = collection.id;
        collectionJson["name"] = collection.name;
        collectionJson["description"] = collection.description;
        collectionJson["autoUpdate"] = collection.autoUpdate;
        collectionJson["maxSize"] = collection.maxSize;
        collectionJson["minScore"] = collection.minScore;
        collectionJson["sortBy"] = static_cast<int>(collection.sortBy);
        collectionJson["ascending"] = collection.ascending;
        collectionJson["iconName"] = collection.iconName;
        collectionJson["color"] = collection.color;
        collectionJson["tags"] = collection.tags;
        collectionJson["isSystem"] = collection.isSystem;
        collectionJson["isVisible"] = collection.isVisible;
        
        // Export rules
        for (const auto& rule : collection.rules) {
            nlohmann::json ruleJson;
            ruleJson["type"] = static_cast<int>(rule.type);
            ruleJson["parameter"] = rule.parameter;
            ruleJson["operation"] = rule.operation;
            ruleJson["value"] = rule.value;
            ruleJson["stringValue"] = rule.stringValue;
            ruleJson["weight"] = rule.weight;
            collectionJson["rules"].push_back(ruleJson);
        }
        
        data["collections"].push_back(collectionJson);
    }
    
    // Export playlists
    for (const auto& [id, playlist] : playlists_) {
        nlohmann::json playlistJson;
        playlistJson["id"] = playlist.id;
        playlistJson["name"] = playlist.name;
        playlistJson["description"] = playlist.description;
        playlistJson["presetPaths"] = playlist.presetPaths;
        playlistJson["enableSmartSuggestions"] = playlist.enableSmartSuggestions;
        playlistJson["diversityLevel"] = playlist.diversityLevel;
        playlistJson["maxSuggestions"] = playlist.maxSuggestions;
        playlistJson["workflowType"] = playlist.workflowType;
        playlistJson["contextTags"] = playlist.contextTags;
        playlistJson["shuffleMode"] = playlist.shuffleMode;
        playlistJson["loopMode"] = playlist.loopMode;
        playlistJson["currentIndex"] = playlist.currentIndex;
        playlistJson["creator"] = playlist.creator;
        playlistJson["isShared"] = playlist.isShared;
        
        data["playlists"].push_back(playlistJson);
    }
    
    return data;
}

bool SmartCollectionManager::importData(const nlohmann::json& data) {
    try {
        // Import collections
        if (data.contains("collections")) {
            for (const auto& collectionJson : data["collections"]) {
                SmartCollection collection;
                collection.id = collectionJson["id"];
                collection.name = collectionJson["name"];
                collection.description = collectionJson["description"];
                collection.autoUpdate = collectionJson.value("autoUpdate", true);
                collection.maxSize = collectionJson.value("maxSize", 100);
                collection.minScore = collectionJson.value("minScore", 0.3f);
                collection.sortBy = static_cast<SmartCollection::SortBy>(collectionJson.value("sortBy", 0));
                collection.ascending = collectionJson.value("ascending", false);
                collection.iconName = collectionJson.value("iconName", "");
                collection.color = collectionJson.value("color", "");
                collection.tags = collectionJson.value("tags", std::vector<std::string>());
                collection.isSystem = collectionJson.value("isSystem", false);
                collection.isVisible = collectionJson.value("isVisible", true);
                
                // Import rules
                if (collectionJson.contains("rules")) {
                    for (const auto& ruleJson : collectionJson["rules"]) {
                        CollectionRule rule;
                        rule.type = static_cast<CollectionRule::Type>(ruleJson["type"]);
                        rule.parameter = ruleJson["parameter"];
                        rule.operation = ruleJson["operation"];
                        rule.value = ruleJson["value"];
                        rule.stringValue = ruleJson["stringValue"];
                        rule.weight = ruleJson["weight"];
                        collection.rules.push_back(rule);
                    }
                }
                
                collections_[collection.id] = collection;
            }
        }
        
        // Import playlists
        if (data.contains("playlists")) {
            for (const auto& playlistJson : data["playlists"]) {
                SmartPlaylist playlist;
                playlist.id = playlistJson["id"];
                playlist.name = playlistJson["name"];
                playlist.description = playlistJson["description"];
                playlist.presetPaths = playlistJson.value("presetPaths", std::vector<std::string>());
                playlist.enableSmartSuggestions = playlistJson.value("enableSmartSuggestions", true);
                playlist.diversityLevel = playlistJson.value("diversityLevel", 0.5f);
                playlist.maxSuggestions = playlistJson.value("maxSuggestions", 10);
                playlist.workflowType = playlistJson.value("workflowType", "");
                playlist.contextTags = playlistJson.value("contextTags", std::vector<std::string>());
                playlist.shuffleMode = playlistJson.value("shuffleMode", false);
                playlist.loopMode = playlistJson.value("loopMode", false);
                playlist.currentIndex = playlistJson.value("currentIndex", 0);
                playlist.creator = playlistJson.value("creator", "");
                playlist.isShared = playlistJson.value("isShared", false);
                
                playlists_[playlist.id] = playlist;
            }
        }
        
        updateStatistics();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SmartCollectionManager::saveToFile(const std::string& filename) const {
    try {
        nlohmann::json data = exportData();
        std::ofstream file(filename);
        file << data.dump(2);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool SmartCollectionManager::loadFromFile(const std::string& filename) {
    try {
        std::ifstream file(filename);
        nlohmann::json data;
        file >> data;
        return importData(data);
    } catch (const std::exception&) {
        return false;
    }
}

// Private helper methods

std::string SmartCollectionManager::generateCollectionId() {
    return "collection_" + std::to_string(nextCollectionId_++);
}

std::string SmartCollectionManager::generatePlaylistId() {
    return "playlist_" + std::to_string(nextPlaylistId_++);
}

void SmartCollectionManager::updateStatistics() const {
    stats_.totalCollections = static_cast<int>(collections_.size());
    stats_.totalPlaylists = static_cast<int>(playlists_.size());
    
    // Count active collections (updated in last 30 days)
    auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * 30);
    stats_.activeCollections = 0;
    float totalSize = 0.0f;
    
    for (const auto& [id, collection] : collections_) {
        if (collection.lastUpdated >= cutoff) {
            stats_.activeCollections++;
        }
        totalSize += collection.presetPaths.size();
    }
    
    stats_.averageCollectionSize = stats_.totalCollections > 0 ? 
        totalSize / stats_.totalCollections : 0.0f;
    
    stats_.lastUpdated = std::chrono::system_clock::now();
}

float SmartCollectionManager::calculateCollectionSimilarity(const SmartCollection& collection1,
                                                           const SmartCollection& collection2) const {
    // Calculate similarity based on content overlap
    std::set<std::string> set1(collection1.presetPaths.begin(), collection1.presetPaths.end());
    std::set<std::string> set2(collection2.presetPaths.begin(), collection2.presetPaths.end());
    
    std::set<std::string> intersection;
    std::set_intersection(set1.begin(), set1.end(),
                         set2.begin(), set2.end(),
                         std::inserter(intersection, intersection.begin()));
    
    std::set<std::string> unionSet;
    std::set_union(set1.begin(), set1.end(),
                   set2.begin(), set2.end(),
                   std::inserter(unionSet, unionSet.begin()));
    
    // Jaccard similarity coefficient
    return unionSet.empty() ? 0.0f : 
           static_cast<float>(intersection.size()) / static_cast<float>(unionSet.size());
}

std::vector<std::string> SmartCollectionManager::generateInsights(const SmartCollection& collection) const {
    std::vector<std::string> insights;
    
    // Collection size insights
    if (collection.presetPaths.size() == 0) {
        insights.push_back("Collection is empty - consider adjusting rules or thresholds");
    } else if (static_cast<int>(collection.presetPaths.size()) >= collection.maxSize) {
        insights.push_back("Collection is at maximum capacity - some presets may be excluded");
    } else if (collection.presetPaths.size() < 5) {
        insights.push_back("Collection has few presets - consider relaxing rules for more matches");
    }
    
    // Rule effectiveness insights
    if (collection.rules.empty()) {
        insights.push_back("Collection has no rules - all presets will be included");
    } else if (collection.rules.size() > 5) {
        insights.push_back("Collection has many rules - consider simplifying for better performance");
    }
    
    // Update frequency insights
    auto now = std::chrono::system_clock::now();
    auto daysSinceUpdate = std::chrono::duration_cast<std::chrono::hours>(now - collection.lastUpdated).count() / 24;
    
    if (daysSinceUpdate > 7) {
        insights.push_back("Collection hasn't been updated recently - consider enabling auto-update");
    }
    
    // Score distribution insights
    if (!collection.presetScores.empty()) {
        float totalScore = 0.0f;
        float minScore = 1.0f;
        float maxScore = 0.0f;
        
        for (const auto& [preset, score] : collection.presetScores) {
            totalScore += score;
            minScore = std::min(minScore, score);
            maxScore = std::max(maxScore, score);
        }
        
        float avgScore = totalScore / collection.presetScores.size();
        
        if (avgScore < 0.4f) {
            insights.push_back("Average match score is low - rules may be too strict");
        } else if (maxScore - minScore < 0.2f) {
            insights.push_back("Score range is narrow - rules may need more differentiation");
        }
    }
    
    return insights;
}

} // namespace AIMusicHardware