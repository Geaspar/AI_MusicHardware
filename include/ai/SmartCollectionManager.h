#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <set>
#include "PresetMLAnalyzer.h"
#include "PresetRecommendationEngine.h"
#include "ui/presets/PresetInfo.h"

namespace AIMusicHardware {

/**
 * @brief Rule for dynamic collection updates
 */
struct CollectionRule {
    enum class Type {
        AudioCharacteristic,    // Based on audio features (brightness > 0.8)
        Category,              // Based on preset category
        Author,                // Based on preset author
        Tag,                   // Based on preset tags
        UserBehavior,          // Based on user interactions
        Temporal,              // Based on time-based patterns
        Similarity,            // Based on similarity to reference presets
        Custom                 // Custom rule with callback
    };
    
    Type type;
    std::string parameter;          // Feature name, category, author, etc.
    std::string operation;          // "greater_than", "equals", "contains", etc.
    float value;                    // Threshold value for numeric comparisons
    std::string stringValue;        // String value for text comparisons
    float weight = 1.0f;           // Rule weight for scoring
    
    // Custom rule callback for complex logic
    std::function<bool(const PresetInfo&, const AudioFeatureVector&)> customEvaluator;
    
    /**
     * @brief Evaluate if a preset matches this rule
     * @param preset Preset to evaluate
     * @param features Audio features of the preset
     * @return Match score (0.0-1.0)
     */
    float evaluate(const PresetInfo& preset, const AudioFeatureVector& features) const;
};

/**
 * @brief Smart collection that automatically updates based on rules
 */
struct SmartCollection {
    std::string id;
    std::string name;
    std::string description;
    std::vector<CollectionRule> rules;
    
    // Update behavior
    bool autoUpdate = true;                // Automatically update when new presets are added
    int maxSize = 100;                     // Maximum number of presets in collection
    float minScore = 0.3f;                 // Minimum score threshold for inclusion
    
    // Current state
    std::vector<std::string> presetPaths;  // Current presets in collection
    std::unordered_map<std::string, float> presetScores; // Scores for each preset
    std::chrono::system_clock::time_point lastUpdated;
    
    // Display settings
    enum class SortBy {
        Score,          // Sort by rule match score
        Name,           // Sort alphabetically by name
        DateAdded,      // Sort by when preset was added to collection
        DateCreated,    // Sort by preset creation date
        UserRating,     // Sort by user rating
        Popularity      // Sort by usage/popularity
    } sortBy = SortBy::Score;
    
    bool ascending = false; // Sort direction
    
    // Metadata
    std::string iconName;                  // Icon for UI display
    std::string color;                     // Color theme for UI
    std::vector<std::string> tags;         // Tags for collection organization
    bool isSystem = false;                 // System collection (can't be deleted)
    bool isVisible = true;                 // Visible in UI
    
    /**
     * @brief Calculate match score for a preset
     * @param preset Preset to evaluate
     * @param features Audio features of the preset
     * @return Total match score (0.0-1.0)
     */
    float calculateScore(const PresetInfo& preset, const AudioFeatureVector& features) const;
    
    /**
     * @brief Check if preset should be included in collection
     * @param preset Preset to evaluate
     * @param features Audio features of the preset
     * @return true if preset meets inclusion criteria
     */
    bool shouldInclude(const PresetInfo& preset, const AudioFeatureVector& features) const;
};

/**
 * @brief Collection template for creating new smart collections
 */
struct CollectionTemplate {
    std::string name;
    std::string description;
    std::vector<CollectionRule> defaultRules;
    SmartCollection::SortBy defaultSortBy;
    std::string iconName;
    std::string color;
    std::vector<std::string> suggestedTags;
    
    /**
     * @brief Create a smart collection from this template
     * @param collectionName Name for the new collection
     * @return New smart collection instance
     */
    SmartCollection createCollection(const std::string& collectionName) const;
};

/**
 * @brief Playlist with smart suggestions and workflow integration
 */
struct SmartPlaylist {
    std::string id;
    std::string name;
    std::string description;
    
    // Playlist contents
    std::vector<std::string> presetPaths;      // Manually added presets
    std::vector<std::string> suggestedPaths;   // Smart suggestions
    
    // Smart features
    bool enableSmartSuggestions = true;        // Automatically suggest similar presets
    float diversityLevel = 0.5f;               // How diverse suggestions should be
    int maxSuggestions = 10;                   // Maximum number of smart suggestions
    
    // Workflow integration
    std::string workflowType;                  // "creative", "mixing", "performance"
    std::vector<std::string> contextTags;     // Context for smart suggestions
    
    // Playback features
    bool shuffleMode = false;
    bool loopMode = false;
    int currentIndex = 0;
    
    // Metadata
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point lastModified;
    std::string creator;
    bool isShared = false;
    
    /**
     * @brief Get next preset in playlist order
     * @return Path to next preset, empty if at end
     */
    std::string getNext();
    
    /**
     * @brief Get previous preset in playlist order
     * @return Path to previous preset, empty if at beginning
     */
    std::string getPrevious();
    
    /**
     * @brief Update smart suggestions based on current playlist content
     * @param recommendationEngine Engine for generating suggestions
     */
    void updateSmartSuggestions(PresetRecommendationEngine& recommendationEngine);
};

/**
 * @brief Manager for smart collections and intelligent playlists
 * Provides automated collection management with machine learning integration
 */
class SmartCollectionManager {
public:
    explicit SmartCollectionManager(std::shared_ptr<PresetMLAnalyzer> analyzer,
                                   std::shared_ptr<PresetRecommendationEngine> recommendationEngine);
    ~SmartCollectionManager();
    
    // Smart collection management
    
    /**
     * @brief Create a new smart collection
     * @param name Collection name
     * @param description Collection description
     * @param rules Vector of collection rules
     * @return ID of created collection
     */
    std::string createSmartCollection(const std::string& name,
                                    const std::string& description,
                                    const std::vector<CollectionRule>& rules);
    
    /**
     * @brief Create collection from template
     * @param templateName Name of template to use
     * @param collectionName Name for new collection
     * @return ID of created collection, empty if template not found
     */
    std::string createFromTemplate(const std::string& templateName,
                                  const std::string& collectionName);
    
    /**
     * @brief Update a smart collection's rules
     * @param collectionId ID of collection to update
     * @param rules New rules for the collection
     * @return Success/failure status
     */
    bool updateCollectionRules(const std::string& collectionId,
                              const std::vector<CollectionRule>& rules);
    
    /**
     * @brief Delete a smart collection
     * @param collectionId ID of collection to delete
     * @return Success/failure status
     */
    bool deleteSmartCollection(const std::string& collectionId);
    
    /**
     * @brief Get all smart collections
     * @return Vector of all collections
     */
    std::vector<SmartCollection> getAllCollections() const;
    
    /**
     * @brief Get collection by ID
     * @param collectionId ID of collection to retrieve
     * @return Pointer to collection, nullptr if not found
     */
    const SmartCollection* getCollection(const std::string& collectionId) const;
    
    // Collection updates and maintenance
    
    /**
     * @brief Update all collections with new/changed presets
     * @param presets Vector of all available presets
     * @param progressCallback Progress callback function (current, total)
     */
    void updateAllCollections(const std::vector<PresetInfo>& presets,
                             std::function<void(int, int)> progressCallback = nullptr);
    
    /**
     * @brief Update specific collection
     * @param collectionId ID of collection to update
     * @param presets Vector of all available presets
     * @return Success/failure status
     */
    bool updateCollection(const std::string& collectionId,
                         const std::vector<PresetInfo>& presets);
    
    /**
     * @brief Add new preset to relevant collections
     * @param preset New preset to evaluate
     * @return Vector of collection IDs that included the preset
     */
    std::vector<std::string> addPresetToCollections(const PresetInfo& preset);
    
    /**
     * @brief Remove preset from all collections
     * @param presetPath Path of preset to remove
     */
    void removePresetFromCollections(const std::string& presetPath);
    
    // Smart playlist management
    
    /**
     * @brief Create a new smart playlist
     * @param name Playlist name
     * @param description Playlist description
     * @param workflowType Workflow context for smart suggestions
     * @return ID of created playlist
     */
    std::string createSmartPlaylist(const std::string& name,
                                   const std::string& description,
                                   const std::string& workflowType = "");
    
    /**
     * @brief Add preset to playlist
     * @param playlistId ID of playlist
     * @param presetPath Path of preset to add
     * @param position Position to insert (-1 for end)
     * @return Success/failure status
     */
    bool addToPlaylist(const std::string& playlistId,
                      const std::string& presetPath,
                      int position = -1);
    
    /**
     * @brief Remove preset from playlist
     * @param playlistId ID of playlist
     * @param presetPath Path of preset to remove
     * @return Success/failure status
     */
    bool removeFromPlaylist(const std::string& playlistId,
                           const std::string& presetPath);
    
    /**
     * @brief Update smart suggestions for playlist
     * @param playlistId ID of playlist to update
     */
    void updatePlaylistSuggestions(const std::string& playlistId);
    
    /**
     * @brief Get all smart playlists
     * @return Vector of all playlists
     */
    std::vector<SmartPlaylist> getAllPlaylists() const;
    
    /**
     * @brief Get playlist by ID
     * @param playlistId ID of playlist to retrieve
     * @return Pointer to playlist, nullptr if not found
     */
    const SmartPlaylist* getPlaylist(const std::string& playlistId) const;
    
    // Template management
    
    /**
     * @brief Register a collection template
     * @param templateName Name of template
     * @param template Template definition
     */
    void registerTemplate(const std::string& templateName,
                          const CollectionTemplate& templateDef);
    
    /**
     * @brief Get all available templates
     * @return Map of template names to templates
     */
    std::unordered_map<std::string, CollectionTemplate> getTemplates() const;
    
    /**
     * @brief Create default system templates
     */
    void createDefaultTemplates();
    
    // Search and discovery
    
    /**
     * @brief Find collections containing a specific preset
     * @param presetPath Path of preset to search for
     * @return Vector of collection IDs containing the preset
     */
    std::vector<std::string> findCollectionsWithPreset(const std::string& presetPath) const;
    
    /**
     * @brief Search collections by name or description
     * @param query Search query
     * @return Vector of matching collection IDs
     */
    std::vector<std::string> searchCollections(const std::string& query) const;
    
    /**
     * @brief Get similar collections based on content
     * @param collectionId Reference collection ID
     * @param maxResults Maximum number of similar collections
     * @return Vector of similar collection IDs with similarity scores
     */
    std::vector<std::pair<std::string, float>> getSimilarCollections(
        const std::string& collectionId,
        int maxResults = 5) const;
    
    // Analytics and insights
    
    struct CollectionStats {
        int totalCollections = 0;
        int totalPlaylists = 0;
        int activeCollections = 0;      // Collections with recent updates
        float averageCollectionSize = 0.0f;
        float averageUpdateFrequency = 0.0f;
        std::unordered_map<std::string, int> templateUsage;
        std::unordered_map<std::string, int> ruleTypeUsage;
        std::chrono::system_clock::time_point lastUpdated;
    };
    
    /**
     * @brief Get collection system statistics
     * @return Current statistics
     */
    CollectionStats getStatistics() const;
    
    /**
     * @brief Get insights about collection effectiveness
     * @param collectionId Collection to analyze
     * @return Human-readable insights about collection performance
     */
    std::vector<std::string> getCollectionInsights(const std::string& collectionId) const;
    
    // Data persistence
    
    /**
     * @brief Export collections and playlists to JSON
     * @return JSON representation of all data
     */
    nlohmann::json exportData() const;
    
    /**
     * @brief Import collections and playlists from JSON
     * @param data JSON data to import
     * @return Success/failure status
     */
    bool importData(const nlohmann::json& data);
    
    /**
     * @brief Save collections to file
     * @param filename File path to save to
     * @return Success/failure status
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Load collections from file
     * @param filename File path to load from
     * @return Success/failure status
     */
    bool loadFromFile(const std::string& filename);

private:
    // Core components
    std::shared_ptr<PresetMLAnalyzer> analyzer_;
    std::shared_ptr<PresetRecommendationEngine> recommendationEngine_;
    
    // Data storage
    std::unordered_map<std::string, SmartCollection> collections_;
    std::unordered_map<std::string, SmartPlaylist> playlists_;
    std::unordered_map<std::string, CollectionTemplate> templates_;
    
    // ID generation
    int nextCollectionId_ = 1;
    int nextPlaylistId_ = 1;
    
    // Statistics and monitoring
    mutable CollectionStats stats_;
    
    // Helper methods
    
    /**
     * @brief Generate unique collection ID
     * @return New unique ID
     */
    std::string generateCollectionId();
    
    /**
     * @brief Generate unique playlist ID
     * @return New unique ID
     */
    std::string generatePlaylistId();
    
    /**
     * @brief Update collection statistics
     */
    void updateStatistics() const;
    
    /**
     * @brief Sort collection presets according to sort settings
     * @param collection Collection to sort
     * @param presets Map of preset paths to preset info
     */
    void sortCollectionPresets(SmartCollection& collection,
                              const std::unordered_map<std::string, PresetInfo>& presets);
    
    /**
     * @brief Evaluate all rules for a preset
     * @param preset Preset to evaluate
     * @param features Audio features of preset
     * @param rules Rules to evaluate
     * @return Combined rule score
     */
    float evaluateRules(const PresetInfo& preset,
                       const AudioFeatureVector& features,
                       const std::vector<CollectionRule>& rules) const;
    
    /**
     * @brief Calculate collection similarity based on content overlap
     * @param collection1 First collection
     * @param collection2 Second collection
     * @return Similarity score (0.0-1.0)
     */
    float calculateCollectionSimilarity(const SmartCollection& collection1,
                                       const SmartCollection& collection2) const;
    
    /**
     * @brief Generate insights for collection performance
     * @param collection Collection to analyze
     * @return Vector of insight strings
     */
    std::vector<std::string> generateInsights(const SmartCollection& collection) const;
};

} // namespace AIMusicHardware