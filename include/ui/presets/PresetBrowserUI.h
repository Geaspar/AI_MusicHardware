#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include "PresetDatabase.h"
#include "PresetInfo.h"

namespace AIMusicHardware {

/**
 * @brief Search and filter criteria for the preset browser
 */
struct PresetBrowserFilter {
    std::string searchTerm;
    std::string selectedCategory;
    std::string selectedAuthor;
    std::vector<std::string> selectedTags;
    bool favoritesOnly = false;
    int minRating = 0;
    
    // Audio characteristics filters
    struct AudioFilters {
        float minBassContent = 0.0f;
        float maxBassContent = 1.0f;
        float minBrightness = 0.0f;
        float maxBrightness = 1.0f;
        float minComplexity = 0.0f;
        float maxComplexity = 1.0f;
        bool hasArpeggiator = false;
        bool hasSequencer = false;
    } audioFilters;
    
    bool operator==(const PresetBrowserFilter& other) const {
        return searchTerm == other.searchTerm &&
               selectedCategory == other.selectedCategory &&
               selectedAuthor == other.selectedAuthor &&
               selectedTags == other.selectedTags &&
               favoritesOnly == other.favoritesOnly &&
               minRating == other.minRating;
    }
};

/**
 * @brief Sorting options for preset list
 */
enum class PresetSortOption {
    NameAscending,
    NameDescending,
    AuthorAscending,
    AuthorDescending,
    CategoryAscending,
    CategoryDescending,
    DateCreatedDescending,
    DateModifiedDescending,
    RatingDescending,
    PlayCountDescending
};

/**
 * @brief Virtual list item for performance-optimized rendering
 */
struct VirtualListItem {
    int index;
    PresetInfo preset;
    bool isVisible = false;
    bool needsUpdate = true;
    
    // Visual state
    bool isSelected = false;
    bool isHovered = false;
    float animationProgress = 0.0f;
};

/**
 * @brief Folder tree node for hierarchical preset organization
 */
struct FolderTreeNode {
    std::string name;
    std::string fullPath;
    int presetCount = 0;
    bool isExpanded = false;
    bool isSelected = false;
    
    std::weak_ptr<FolderTreeNode> parent;
    std::vector<std::shared_ptr<FolderTreeNode>> children;
    std::vector<PresetInfo> presets;
    
    // Visual state
    int depth = 0;
    bool isVisible = true;
    float animationProgress = 1.0f;
};

/**
 * @brief Main preset browser UI component based on Vital's architecture
 */
class PresetBrowserUI {
public:
    using PresetSelectedCallback = std::function<void(const PresetInfo&)>;
    using PresetDoubleClickCallback = std::function<void(const PresetInfo&)>;
    using FilterChangedCallback = std::function<void(const PresetBrowserFilter&)>;
    
    explicit PresetBrowserUI(std::shared_ptr<PresetDatabase> database);
    ~PresetBrowserUI();
    
    // Core functionality
    void initialize();
    void update(float deltaTime);
    void render();
    void resize(int width, int height);
    
    // Preset selection and navigation
    void selectPreset(const std::string& filePath);
    void selectNext();
    void selectPrevious();
    void selectRandom();
    
    // Search and filtering
    void setSearchTerm(const std::string& term);
    void setFilter(const PresetBrowserFilter& filter);
    void clearFilters();
    
    // Sorting
    void setSortOption(PresetSortOption option);
    PresetSortOption getSortOption() const { return currentSort_; }
    
    // Folder tree operations
    void expandFolder(const std::string& path);
    void collapseFolder(const std::string& path);
    void selectFolder(const std::string& path);
    void refreshFolderTree();
    
    // Favorites and ratings
    void toggleFavorite(const std::string& filePath);
    void setRating(const std::string& filePath, int rating);
    
    // View options
    void setViewMode(bool showFolderTree, bool showPreviewPanel);
    void setItemHeight(int height);
    void setVisibleItemCount(int count);
    
    // Callbacks
    void setPresetSelectedCallback(PresetSelectedCallback callback);
    void setPresetDoubleClickCallback(PresetDoubleClickCallback callback);
    void setFilterChangedCallback(FilterChangedCallback callback);
    
    // State queries
    const PresetInfo* getSelectedPreset() const;
    const std::vector<PresetInfo>& getCurrentPresets() const { return currentPresets_; }
    const PresetBrowserFilter& getCurrentFilter() const { return currentFilter_; }
    bool isLoading() const;
    
    // Performance monitoring
    struct RenderStats {
        int totalItems;
        int visibleItems;
        int renderedItems;
        float lastFrameTime;
        float averageFrameTime;
        int cacheHitRate;
    };
    RenderStats getRenderStats() const { return renderStats_; }

private:
    // Core components
    std::shared_ptr<PresetDatabase> database_;
    
    // Data management
    std::vector<PresetInfo> currentPresets_;
    std::vector<VirtualListItem> virtualItems_;
    std::shared_ptr<FolderTreeNode> rootFolder_;
    std::vector<std::shared_ptr<FolderTreeNode>> flattenedFolders_;
    
    // Current state
    PresetBrowserFilter currentFilter_;
    PresetSortOption currentSort_ = PresetSortOption::NameAscending;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    
    // Virtual scrolling state
    int firstVisibleIndex_ = 0;
    int visibleItemCount_ = 20;
    int itemHeight_ = 24;
    float scrollOffset_ = 0.0f;
    float targetScrollOffset_ = 0.0f;
    
    // UI layout
    struct UILayout {
        int totalWidth = 800;
        int totalHeight = 600;
        int folderTreeWidth = 200;
        int previewPanelWidth = 250;
        int listPanelWidth = 350;
        bool showFolderTree = true;
        bool showPreviewPanel = true;
    } layout_;
    
    // Animation and visual state
    struct AnimationState {
        float folderExpandDuration = 0.2f;
        float scrollAnimationDuration = 0.15f;
        float hoverFadeSpeed = 8.0f;
        float selectionFadeSpeed = 12.0f;
    } animation_;
    
    // Performance optimization
    mutable RenderStats renderStats_;
    std::chrono::high_resolution_clock::time_point lastFrameTime_;
    float frameTimeAccumulator_ = 0.0f;
    int frameCount_ = 0;
    
    // Callbacks
    PresetSelectedCallback presetSelectedCallback_;
    PresetDoubleClickCallback presetDoubleClickCallback_;
    FilterChangedCallback filterChangedCallback_;
    
    // Internal methods
    void rebuildPresetList();
    void rebuildFolderTree();
    void updateVirtualList();
    void updateScrolling(float deltaTime);
    void updateAnimations(float deltaTime);
    
    // Folder tree operations
    void flattenFolderTree();
    std::shared_ptr<FolderTreeNode> findFolder(const std::string& path);
    void updateFolderVisibility();
    int calculateFolderTreeHeight() const;
    
    // Virtual list operations
    void ensureItemVisible(int index);
    void updateVisibleRange();
    void recycleVirtualItems();
    
    // Sorting and filtering
    void sortPresets();
    void applyCurrentFilter();
    bool matchesFilter(const PresetInfo& preset) const;
    
    // Rendering helpers
    void renderFolderTree();
    void renderPresetList();
    void renderPreviewPanel();
    void renderScrollbar();
    void renderLoadingIndicator();
    
    void renderFolderNode(const std::shared_ptr<FolderTreeNode>& node, int& yPos);
    void renderPresetItem(const VirtualListItem& item, int yPos);
    void renderPresetPreview(const PresetInfo& preset);
    
    // Input handling
    void handleClick(int x, int y);
    void handleDoubleClick(int x, int y);
    void handleScroll(float delta);
    void handleKeyPress(char key);
    
    // Utility methods
    std::string formatPresetDate(const std::chrono::time_point<std::chrono::system_clock>& time) const;
    std::string formatFileSize(size_t bytes) const;
    int calculateScrollbarHeight() const;
    int calculateScrollbarPosition() const;
};

/**
 * @brief Preset preview panel for displaying preset details
 */
class PresetPreviewPanel {
public:
    PresetPreviewPanel();
    ~PresetPreviewPanel();
    
    void setPreset(const PresetInfo& preset);
    void render(int x, int y, int width, int height);
    void clear();
    
    // Configuration
    void setShowAudioCharacteristics(bool show);
    void setShowTechnicalInfo(bool show);
    void setShowDescription(bool show);
    
private:
    PresetInfo currentPreset_;
    bool hasPreset_ = false;
    
    // Display options
    bool showAudioCharacteristics_ = true;
    bool showTechnicalInfo_ = true;
    bool showDescription_ = true;
    
    // Rendering helpers
    void renderBasicInfo(int& yPos, int width);
    void renderAudioCharacteristics(int& yPos, int width);
    void renderTechnicalInfo(int& yPos, int width);
    void renderDescription(int& yPos, int width);
    void renderProgressBar(int x, int y, int width, float value, const std::string& label);
};

} // namespace AIMusicHardware