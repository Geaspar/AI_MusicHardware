#pragma once

#include "PresetInfo.h"
#include <vector>
#include <map>
#include <set>
#include <string>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace AIMusicHardware {

/**
 * @brief High-performance preset database with indexing and caching
 * 
 * Inspired by Vital's PresetInfoCache, this class provides fast lookup
 * operations for large preset collections with background indexing.
 */
class PresetDatabase {
public:
    /**
     * @brief Callback for database update notifications
     */
    using UpdateCallback = std::function<void(const std::vector<PresetInfo>&)>;
    
    /**
     * @brief Constructor
     */
    PresetDatabase();
    
    /**
     * @brief Destructor
     */
    ~PresetDatabase();
    
    /**
     * @brief Initialize the database with preset directories
     * @param directories List of directories to scan for presets
     * @return true if initialization successful
     */
    bool initialize(const std::vector<std::string>& directories);
    
    /**
     * @brief Add a directory to watch for presets
     * @param directory Directory path to add
     * @param recursive Whether to scan subdirectories
     * @return true if directory added successfully
     */
    bool addDirectory(const std::string& directory, bool recursive = true);
    
    /**
     * @brief Remove a directory from the database
     * @param directory Directory path to remove
     */
    void removeDirectory(const std::string& directory);
    
    /**
     * @brief Get all presets in the database
     * @return Vector of all cached preset info
     */
    std::vector<PresetInfo> getAllPresets() const;
    
    /**
     * @brief Search presets by name (fast indexed search)
     * @param query Search query string
     * @return Vector of matching presets
     */
    std::vector<PresetInfo> searchByName(const std::string& query) const;
    
    /**
     * @brief Get presets filtered by category
     * @param category Category to filter by
     * @return Vector of matching presets
     */
    std::vector<PresetInfo> getByCategory(const std::string& category) const;
    
    /**
     * @brief Get presets by author
     * @param author Author name to filter by
     * @return Vector of matching presets
     */
    std::vector<PresetInfo> getByAuthor(const std::string& author) const;
    
    /**
     * @brief Get favorite presets
     * @return Vector of favorite presets
     */
    std::vector<PresetInfo> getFavorites() const;
    
    /**
     * @brief Advanced filtering with multiple criteria
     * @param criteria Filter criteria
     * @return Vector of matching presets
     */
    std::vector<PresetInfo> filter(const PresetFilterCriteria& criteria) const;
    
    /**
     * @brief Sort presets by specified criteria
     * @param presets Presets to sort (modified in place)
     * @param criteria Sort criteria
     * @param direction Sort direction
     */
    void sort(std::vector<PresetInfo>& presets, 
              PresetSortCriteria criteria, 
              SortDirection direction = SortDirection::Ascending) const;
    
    /**
     * @brief Get a specific preset by file path
     * @param filePath Path to the preset file
     * @return Preset info if found, nullptr otherwise
     */
    std::shared_ptr<PresetInfo> getPreset(const std::string& filePath) const;
    
    /**
     * @brief Update preset metadata
     * @param filePath Path to the preset file
     * @param updatedInfo Updated preset information
     * @return true if update successful
     */
    bool updatePreset(const std::string& filePath, const PresetInfo& updatedInfo);
    
    /**
     * @brief Add or update a preset in the database
     * @param presetInfo Preset information to add/update
     * @return true if operation successful
     */
    bool addPreset(const PresetInfo& presetInfo);
    
    /**
     * @brief Remove a preset from the database
     * @param filePath Path to the preset file
     * @return true if removal successful
     */
    bool removePreset(const std::string& filePath);
    
    /**
     * @brief Force rebuild of all indices
     */
    void rebuildIndices();
    
    /**
     * @brief Get all unique categories in the database
     * @return Set of category strings
     */
    std::set<std::string> getAllCategories() const;
    
    /**
     * @brief Get all unique authors in the database
     * @return Set of author strings
     */
    std::set<std::string> getAllAuthors() const;
    
    /**
     * @brief Get all unique tags in the database
     * @return Set of tag strings
     */
    std::set<std::string> getAllTags() const;
    
    /**
     * @brief Get database statistics
     */
    struct Statistics {
        size_t totalPresets;
        size_t totalCategories;
        size_t totalAuthors;
        size_t totalFavorites;
        size_t cacheHitRate;  // Percentage 0-100
        std::chrono::milliseconds lastUpdateTime;
    };
    Statistics getStatistics() const;
    
    /**
     * @brief Set callback for database updates
     * @param callback Function to call when database is updated
     */
    void setUpdateCallback(UpdateCallback callback);
    
    /**
     * @brief Check if database is currently updating
     * @return true if background update in progress
     */
    bool isUpdating() const;
    
    /**
     * @brief Wait for current update to complete
     * @param timeoutMs Maximum time to wait in milliseconds
     * @return true if update completed, false if timeout
     */
    bool waitForUpdate(int timeoutMs = 5000) const;
    
private:
    // Core data storage
    mutable std::mutex dataMutex_;
    std::map<std::string, PresetInfo> presets_;  // filePath -> PresetInfo
    std::vector<std::string> watchedDirectories_;
    
    // Performance indices (rebuilt when data changes)
    mutable std::mutex indexMutex_;
    std::multimap<std::string, std::string> nameIndex_;     // lowercase name -> filePath
    std::multimap<std::string, std::string> categoryIndex_; // category -> filePath
    std::multimap<std::string, std::string> authorIndex_;   // author -> filePath
    std::multimap<std::string, std::string> tagIndex_;     // tag -> filePath
    std::set<std::string> favoriteIndex_;                  // filePath of favorites
    
    // Background scanning
    std::atomic<bool> isScanning_{false};
    std::atomic<bool> shouldStopScanning_{false};
    std::unique_ptr<std::thread> scanThread_;
    mutable std::condition_variable updateCondition_;
    mutable std::mutex updateMutex_;
    
    // Cache management
    mutable std::map<std::string, PresetInfo> metadataCache_;
    mutable std::mutex cacheMutex_;
    mutable std::atomic<size_t> cacheHits_{0};
    mutable std::atomic<size_t> cacheMisses_{0};
    
    // Callbacks
    UpdateCallback updateCallback_;
    
    // Internal methods
    void scanDirectoriesBackground();
    void scanDirectory(const std::string& directory, bool recursive);
    void processPresetFile(const std::string& filePath);
    void rebuildIndicesInternal();
    void addToIndices(const PresetInfo& preset);
    void removeFromIndices(const std::string& filePath);
    
    // String utilities for searching
    std::string toLowercase(const std::string& str) const;
    bool matchesSearch(const std::string& text, const std::string& query) const;
    bool matchesFilter(const PresetInfo& preset, const PresetFilterCriteria& criteria) const;
    
    // Cache management
    PresetInfo loadPresetMetadata(const std::string& filePath) const;
    void updateCache(const PresetInfo& preset) const;
    
    // Validation
    bool isValidPresetFile(const std::string& filePath) const;
    
    // Statistics tracking
    mutable Statistics stats_;
    void updateStatistics() const;
};

} // namespace AIMusicHardware