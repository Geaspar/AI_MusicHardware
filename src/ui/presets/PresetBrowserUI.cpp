#include "ui/presets/PresetBrowserUI.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <filesystem>

namespace AIMusicHardware {

PresetBrowserUI::PresetBrowserUI(std::shared_ptr<PresetDatabase> database)
    : database_(database)
    , lastFrameTime_(std::chrono::high_resolution_clock::now()) {
    if (!database_) {
        throw std::invalid_argument("PresetDatabase cannot be null");
    }
}

PresetBrowserUI::~PresetBrowserUI() = default;

void PresetBrowserUI::initialize() {
    // Wait for database to finish initial scan
    if (database_->isUpdating()) {
        database_->waitForUpdate(10000); // 10 second timeout
    }
    
    // Build initial folder tree and preset list
    rebuildFolderTree();
    rebuildPresetList();
    
    // Initialize virtual list
    updateVirtualList();
}

void PresetBrowserUI::update(float deltaTime) {
    // Update animations
    updateAnimations(deltaTime);
    
    // Update scrolling
    updateScrolling(deltaTime);
    
    // Update performance stats
    auto currentTime = std::chrono::high_resolution_clock::now();
    float frameTime = std::chrono::duration<float, std::milli>(currentTime - lastFrameTime_).count();
    lastFrameTime_ = currentTime;
    
    frameTimeAccumulator_ += frameTime;
    frameCount_++;
    
    if (frameCount_ >= 60) {
        renderStats_.lastFrameTime = frameTime;
        renderStats_.averageFrameTime = frameTimeAccumulator_ / frameCount_;
        frameTimeAccumulator_ = 0.0f;
        frameCount_ = 0;
    }
    
    // Check for database updates
    if (database_->isUpdating()) {
        rebuildPresetList();
    }
}

void PresetBrowserUI::render() {
    renderStats_.totalItems = static_cast<int>(currentPresets_.size());
    renderStats_.visibleItems = visibleItemCount_;
    renderStats_.renderedItems = 0;
    
    // Render folder tree if enabled
    if (layout_.showFolderTree) {
        renderFolderTree();
    }
    
    // Render main preset list
    renderPresetList();
    
    // Render preview panel if enabled
    if (layout_.showPreviewPanel && selectedIndex_ >= 0 && selectedIndex_ < currentPresets_.size()) {
        renderPreviewPanel();
    }
    
    // Render loading indicator if database is updating
    if (database_->isUpdating()) {
        renderLoadingIndicator();
    }
}

void PresetBrowserUI::resize(int width, int height) {
    layout_.totalWidth = width;
    layout_.totalHeight = height;
    
    // Recalculate panel widths
    int availableWidth = width;
    if (layout_.showFolderTree) {
        layout_.folderTreeWidth = std::min(250, width / 4);
        availableWidth -= layout_.folderTreeWidth;
    }
    
    if (layout_.showPreviewPanel) {
        layout_.previewPanelWidth = std::min(300, width / 3);
        availableWidth -= layout_.previewPanelWidth;
    }
    
    layout_.listPanelWidth = availableWidth;
    
    // Update visible item count based on height
    visibleItemCount_ = (height - 50) / itemHeight_; // 50px for header
    
    // Update virtual list
    updateVirtualList();
}

void PresetBrowserUI::selectPreset(const std::string& filePath) {
    auto it = std::find_if(currentPresets_.begin(), currentPresets_.end(),
        [&filePath](const PresetInfo& preset) { return preset.filePath == filePath; });
    
    if (it != currentPresets_.end()) {
        int newIndex = static_cast<int>(std::distance(currentPresets_.begin(), it));
        if (newIndex != selectedIndex_) {
            selectedIndex_ = newIndex;
            ensureItemVisible(selectedIndex_);
            
            if (presetSelectedCallback_) {
                presetSelectedCallback_(*it);
            }
        }
    }
}

void PresetBrowserUI::selectNext() {
    if (!currentPresets_.empty()) {
        int newIndex = (selectedIndex_ + 1) % static_cast<int>(currentPresets_.size());
        selectPreset(currentPresets_[newIndex].filePath);
    }
}

void PresetBrowserUI::selectPrevious() {
    if (!currentPresets_.empty()) {
        int newIndex = selectedIndex_ > 0 ? selectedIndex_ - 1 : static_cast<int>(currentPresets_.size()) - 1;
        selectPreset(currentPresets_[newIndex].filePath);
    }
}

void PresetBrowserUI::selectRandom() {
    if (!currentPresets_.empty()) {
        int randomIndex = rand() % static_cast<int>(currentPresets_.size());
        selectPreset(currentPresets_[randomIndex].filePath);
    }
}

void PresetBrowserUI::setSearchTerm(const std::string& term) {
    if (currentFilter_.searchTerm != term) {
        currentFilter_.searchTerm = term;
        rebuildPresetList();
        
        if (filterChangedCallback_) {
            filterChangedCallback_(currentFilter_);
        }
    }
}

void PresetBrowserUI::setFilter(const PresetBrowserFilter& filter) {
    if (!(currentFilter_ == filter)) {
        currentFilter_ = filter;
        rebuildPresetList();
        
        if (filterChangedCallback_) {
            filterChangedCallback_(currentFilter_);
        }
    }
}

void PresetBrowserUI::clearFilters() {
    PresetBrowserFilter emptyFilter;
    setFilter(emptyFilter);
}

void PresetBrowserUI::setSortOption(PresetSortOption option) {
    if (currentSort_ != option) {
        currentSort_ = option;
        sortPresets();
        updateVirtualList();
    }
}

void PresetBrowserUI::toggleFavorite(const std::string& filePath) {
    // Find preset in current list
    auto it = std::find_if(currentPresets_.begin(), currentPresets_.end(),
        [&filePath](PresetInfo& preset) { return preset.filePath == filePath; });
    
    if (it != currentPresets_.end()) {
        it->isFavorite = !it->isFavorite;
        
        // Update in database (this would typically save to disk)
        // database_->updatePreset(*it);
        
        // If we're filtering by favorites and this was unfavorited, rebuild list
        if (currentFilter_.favoritesOnly && !it->isFavorite) {
            rebuildPresetList();
        }
    }
}

void PresetBrowserUI::setRating(const std::string& filePath, int rating) {
    rating = std::clamp(rating, 0, 5);
    
    auto it = std::find_if(currentPresets_.begin(), currentPresets_.end(),
        [&filePath](PresetInfo& preset) { return preset.filePath == filePath; });
    
    if (it != currentPresets_.end()) {
        it->userRating = rating;
        
        // Update in database
        // database_->updatePreset(*it);
    }
}

void PresetBrowserUI::setViewMode(bool showFolderTree, bool showPreviewPanel) {
    layout_.showFolderTree = showFolderTree;
    layout_.showPreviewPanel = showPreviewPanel;
    
    // Trigger resize to recalculate layout
    resize(layout_.totalWidth, layout_.totalHeight);
}

void PresetBrowserUI::setItemHeight(int height) {
    itemHeight_ = std::clamp(height, 16, 64);
    visibleItemCount_ = (layout_.totalHeight - 50) / itemHeight_;
    updateVirtualList();
}

void PresetBrowserUI::setVisibleItemCount(int count) {
    visibleItemCount_ = std::max(1, count);
    updateVirtualList();
}

void PresetBrowserUI::setPresetSelectedCallback(PresetSelectedCallback callback) {
    presetSelectedCallback_ = callback;
}

void PresetBrowserUI::setPresetDoubleClickCallback(PresetDoubleClickCallback callback) {
    presetDoubleClickCallback_ = callback;
}

void PresetBrowserUI::setFilterChangedCallback(FilterChangedCallback callback) {
    filterChangedCallback_ = callback;
}

const PresetInfo* PresetBrowserUI::getSelectedPreset() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < currentPresets_.size()) {
        return &currentPresets_[selectedIndex_];
    }
    return nullptr;
}

bool PresetBrowserUI::isLoading() const {
    return database_->isUpdating();
}

void PresetBrowserUI::rebuildPresetList() {
    // Get all presets from database
    auto allPresets = database_->getAllPresets();
    
    // Apply current filter
    currentPresets_.clear();
    for (const auto& preset : allPresets) {
        if (matchesFilter(preset)) {
            currentPresets_.push_back(preset);
        }
    }
    
    // Sort presets
    sortPresets();
    
    // Update virtual list
    updateVirtualList();
    
    // Maintain selection if possible
    if (selectedIndex_ >= static_cast<int>(currentPresets_.size())) {
        selectedIndex_ = std::max(0, static_cast<int>(currentPresets_.size()) - 1);
    }
}

void PresetBrowserUI::rebuildFolderTree() {
    rootFolder_ = std::make_shared<FolderTreeNode>();
    rootFolder_->name = "Root";
    rootFolder_->fullPath = "";
    rootFolder_->isExpanded = true;
    
    // Get all preset directories from database
    // For now, use a mock list of directories since getDirectories() is not implemented
    std::vector<std::string> directories = {
        "presets/factory",
        "presets/user"
    };
    
    for (const auto& dir : directories) {
        std::filesystem::path path(dir);
        auto current = rootFolder_;
        
        for (const auto& component : path) {
            std::string componentStr = component.string();
            
            // Find or create child folder
            auto child = std::find_if(current->children.begin(), current->children.end(),
                [&componentStr](const std::shared_ptr<FolderTreeNode>& node) {
                    return node->name == componentStr;
                });
            
            if (child == current->children.end()) {
                auto newNode = std::make_shared<FolderTreeNode>();
                newNode->name = componentStr;
                newNode->fullPath = (current->fullPath.empty() ? "" : current->fullPath + "/") + componentStr;
                newNode->parent = current;
                newNode->depth = current->depth + 1;
                
                current->children.push_back(newNode);
                current = newNode;
            } else {
                current = *child;
            }
        }
        
        // Add presets to leaf folder
        // For now, use empty preset list since getPresetsInDirectory() is not implemented
        current->presets = std::vector<PresetInfo>();
        current->presetCount = static_cast<int>(current->presets.size());
    }
    
    flattenFolderTree();
}

void PresetBrowserUI::updateVirtualList() {
    virtualItems_.clear();
    virtualItems_.reserve(currentPresets_.size());
    
    for (size_t i = 0; i < currentPresets_.size(); ++i) {
        VirtualListItem item;
        item.index = static_cast<int>(i);
        item.preset = currentPresets_[i];
        item.isSelected = (item.index == selectedIndex_);
        virtualItems_.push_back(item);
    }
    
    updateVisibleRange();
}

void PresetBrowserUI::updateScrolling(float deltaTime) {
    // Smooth scrolling animation
    if (std::abs(targetScrollOffset_ - scrollOffset_) > 0.1f) {
        float scrollSpeed = 1.0f / animation_.scrollAnimationDuration;
        scrollOffset_ += (targetScrollOffset_ - scrollOffset_) * scrollSpeed * deltaTime;
        
        updateVisibleRange();
    }
}

void PresetBrowserUI::updateAnimations(float deltaTime) {
    // Update virtual item animations
    for (auto& item : virtualItems_) {
        if (item.isSelected) {
            item.animationProgress = std::min(1.0f, 
                item.animationProgress + animation_.selectionFadeSpeed * deltaTime);
        } else {
            item.animationProgress = std::max(0.0f, 
                item.animationProgress - animation_.selectionFadeSpeed * deltaTime);
        }
    }
    
    // Update folder expand/collapse animations
    for (auto& folder : flattenedFolders_) {
        if (folder->isExpanded && folder->animationProgress < 1.0f) {
            folder->animationProgress = std::min(1.0f, 
                folder->animationProgress + (1.0f / animation_.folderExpandDuration) * deltaTime);
        } else if (!folder->isExpanded && folder->animationProgress > 0.0f) {
            folder->animationProgress = std::max(0.0f, 
                folder->animationProgress - (1.0f / animation_.folderExpandDuration) * deltaTime);
        }
    }
}

void PresetBrowserUI::flattenFolderTree() {
    flattenedFolders_.clear();
    
    std::function<void(const std::shared_ptr<FolderTreeNode>&)> flatten = 
        [&](const std::shared_ptr<FolderTreeNode>& node) {
            if (node->depth > 0) { // Skip root
                flattenedFolders_.push_back(node);
            }
            
            if (node->isExpanded) {
                for (const auto& child : node->children) {
                    flatten(child);
                }
            }
        };
    
    flatten(rootFolder_);
    updateFolderVisibility();
}

std::shared_ptr<FolderTreeNode> PresetBrowserUI::findFolder(const std::string& path) {
    for (const auto& folder : flattenedFolders_) {
        if (folder->fullPath == path) {
            return folder;
        }
    }
    return nullptr;
}

void PresetBrowserUI::updateFolderVisibility() {
    for (auto& folder : flattenedFolders_) {
        folder->isVisible = true;
        auto parent = folder->parent.lock();
        while (parent && parent->depth > 0) {
            if (!parent->isExpanded) {
                folder->isVisible = false;
                break;
            }
            parent = parent->parent.lock();
        }
    }
}

void PresetBrowserUI::ensureItemVisible(int index) {
    if (index < 0 || index >= currentPresets_.size()) return;
    
    float itemTop = index * itemHeight_;
    float itemBottom = itemTop + itemHeight_;
    float viewTop = scrollOffset_;
    float viewBottom = scrollOffset_ + (visibleItemCount_ * itemHeight_);
    
    if (itemTop < viewTop) {
        targetScrollOffset_ = itemTop;
    } else if (itemBottom > viewBottom) {
        targetScrollOffset_ = itemBottom - (visibleItemCount_ * itemHeight_);
    }
    
    targetScrollOffset_ = std::max(0.0f, targetScrollOffset_);
    float maxScroll = std::max(0.0f, static_cast<float>(currentPresets_.size() - visibleItemCount_) * itemHeight_);
    targetScrollOffset_ = std::min(targetScrollOffset_, maxScroll);
}

void PresetBrowserUI::updateVisibleRange() {
    firstVisibleIndex_ = static_cast<int>(scrollOffset_ / itemHeight_);
    firstVisibleIndex_ = std::max(0, firstVisibleIndex_);
    
    for (auto& item : virtualItems_) {
        int itemIndex = item.index;
        item.isVisible = (itemIndex >= firstVisibleIndex_ && 
                         itemIndex < firstVisibleIndex_ + visibleItemCount_);
    }
}

bool PresetBrowserUI::matchesFilter(const PresetInfo& preset) const {
    // Search term filter
    if (!currentFilter_.searchTerm.empty()) {
        std::string searchLower = currentFilter_.searchTerm;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
        
        std::string nameLower = preset.name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        
        if (nameLower.find(searchLower) == std::string::npos) {
            return false;
        }
    }
    
    // Category filter
    if (!currentFilter_.selectedCategory.empty() && 
        preset.category != currentFilter_.selectedCategory) {
        return false;
    }
    
    // Author filter
    if (!currentFilter_.selectedAuthor.empty() && 
        preset.author != currentFilter_.selectedAuthor) {
        return false;
    }
    
    // Favorites filter
    if (currentFilter_.favoritesOnly && !preset.isFavorite) {
        return false;
    }
    
    // Rating filter
    if (preset.userRating < currentFilter_.minRating) {
        return false;
    }
    
    // Audio characteristics filters
    const auto& af = currentFilter_.audioFilters;
    const auto& ac = preset.audioCharacteristics;
    
    if (ac.bassContent < af.minBassContent || ac.bassContent > af.maxBassContent) {
        return false;
    }
    
    if (ac.brightness < af.minBrightness || ac.brightness > af.maxBrightness) {
        return false;
    }
    
    if (ac.complexity < af.minComplexity || ac.complexity > af.maxComplexity) {
        return false;
    }
    
    if (af.hasArpeggiator && !ac.hasArpeggiator) {
        return false;
    }
    
    if (af.hasSequencer && !ac.hasSequencer) {
        return false;
    }
    
    return true;
}

void PresetBrowserUI::sortPresets() {
    switch (currentSort_) {
        case PresetSortOption::NameAscending:
            std::sort(currentPresets_.begin(), currentPresets_.end(),
                [](const PresetInfo& a, const PresetInfo& b) { return a.name < b.name; });
            break;
            
        case PresetSortOption::NameDescending:
            std::sort(currentPresets_.begin(), currentPresets_.end(),
                [](const PresetInfo& a, const PresetInfo& b) { return a.name > b.name; });
            break;
            
        case PresetSortOption::AuthorAscending:
            std::sort(currentPresets_.begin(), currentPresets_.end(),
                [](const PresetInfo& a, const PresetInfo& b) { return a.author < b.author; });
            break;
            
        case PresetSortOption::CategoryAscending:
            std::sort(currentPresets_.begin(), currentPresets_.end(),
                [](const PresetInfo& a, const PresetInfo& b) { return a.category < b.category; });
            break;
            
        case PresetSortOption::DateCreatedDescending:
            std::sort(currentPresets_.begin(), currentPresets_.end(),
                [](const PresetInfo& a, const PresetInfo& b) { return a.created > b.created; });
            break;
            
        case PresetSortOption::RatingDescending:
            std::sort(currentPresets_.begin(), currentPresets_.end(),
                [](const PresetInfo& a, const PresetInfo& b) { return a.userRating > b.userRating; });
            break;
            
        default:
            break;
    }
}

// Rendering methods (simplified implementations)
void PresetBrowserUI::renderFolderTree() {
    // This would contain actual rendering code for the folder tree
    // For now, just update render stats
    renderStats_.renderedItems += static_cast<int>(flattenedFolders_.size());
}

void PresetBrowserUI::renderPresetList() {
    // This would contain actual rendering code for the preset list
    // For now, just update render stats
    int rendered = 0;
    for (const auto& item : virtualItems_) {
        if (item.isVisible) {
            rendered++;
        }
    }
    renderStats_.renderedItems += rendered;
}

void PresetBrowserUI::renderPreviewPanel() {
    // This would contain actual rendering code for the preview panel
    renderStats_.renderedItems += 1;
}

void PresetBrowserUI::renderLoadingIndicator() {
    // This would contain actual rendering code for the loading indicator
}

std::string PresetBrowserUI::formatPresetDate(const std::chrono::time_point<std::chrono::system_clock>& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d");
    return ss.str();
}

std::string PresetBrowserUI::formatFileSize(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 3) {
        size /= 1024.0;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << size << " " << units[unit];
    return ss.str();
}

// PresetPreviewPanel implementation
PresetPreviewPanel::PresetPreviewPanel() = default;
PresetPreviewPanel::~PresetPreviewPanel() = default;

void PresetPreviewPanel::setPreset(const PresetInfo& preset) {
    currentPreset_ = preset;
    hasPreset_ = true;
}

void PresetPreviewPanel::render(int x, int y, int width, int height) {
    if (!hasPreset_) return;
    
    int yPos = y + 10;
    
    if (showDescription_) {
        renderBasicInfo(yPos, width);
    }
    
    if (showAudioCharacteristics_) {
        renderAudioCharacteristics(yPos, width);
    }
    
    if (showTechnicalInfo_) {
        renderTechnicalInfo(yPos, width);
    }
}

void PresetPreviewPanel::clear() {
    hasPreset_ = false;
}

void PresetPreviewPanel::setShowAudioCharacteristics(bool show) {
    showAudioCharacteristics_ = show;
}

void PresetPreviewPanel::setShowTechnicalInfo(bool show) {
    showTechnicalInfo_ = show;
}

void PresetPreviewPanel::setShowDescription(bool show) {
    showDescription_ = show;
}

void PresetPreviewPanel::renderBasicInfo(int& yPos, int width) {
    // This would render preset name, author, category, etc.
    yPos += 60;
}

void PresetPreviewPanel::renderAudioCharacteristics(int& yPos, int width) {
    // This would render bass/mid/treble, warmth, brightness bars
    yPos += 80;
}

void PresetPreviewPanel::renderTechnicalInfo(int& yPos, int width) {
    // This would render file size, creation date, etc.
    yPos += 40;
}

void PresetPreviewPanel::renderProgressBar(int x, int y, int width, float value, const std::string& label) {
    // This would render an actual progress bar
}

void PresetBrowserUI::expandFolder(const std::string& path) {
    auto folder = findFolder(path);
    if (folder) {
        folder->isExpanded = true;
        flattenFolderTree();
    }
}

void PresetBrowserUI::collapseFolder(const std::string& path) {
    auto folder = findFolder(path);
    if (folder) {
        folder->isExpanded = false;
        flattenFolderTree();
    }
}

void PresetBrowserUI::selectFolder(const std::string& path) {
    // Clear current folder selection
    for (auto& folder : flattenedFolders_) {
        folder->isSelected = false;
    }

    auto folder = findFolder(path);
    if (folder) {
        folder->isSelected = true;
    }
}

void PresetBrowserUI::refreshFolderTree() {
    rebuildFolderTree();
}

} // namespace AIMusicHardware