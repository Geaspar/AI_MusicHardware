#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <queue>
#include "PresetMLAnalyzer.h"
#include "ui/presets/PresetInfo.h"

namespace AIMusicHardware {

/**
 * @brief User interaction data for collaborative filtering
 */
struct UserInteraction {
    std::string presetPath;
    enum class Type {
        View,           // User viewed/browsed the preset
        Select,         // User selected the preset
        Load,           // User loaded the preset
        Favorite,       // User favorited the preset
        Rate,           // User rated the preset
        Share,          // User shared the preset
        Skip,           // User skipped/dismissed the preset
        Search          // User found preset through search
    } type;
    
    float value = 1.0f;                    // Interaction strength (0.0-1.0)
    std::chrono::system_clock::time_point timestamp;
    std::string context;                   // "morning", "studio_session", "genre:techno", etc.
    int sessionId = 0;                     // Session identifier for workflow analysis
    
    UserInteraction(const std::string& path, Type t, float v = 1.0f)
        : presetPath(path), type(t), value(v), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief User preference profile for personalized recommendations
 */
struct UserProfile {
    // Audio preference weights (learned from interactions)
    std::unordered_map<std::string, float> featurePreferences;
    
    // Category preferences
    std::unordered_map<std::string, float> categoryPreferences;
    
    // Author/creator preferences
    std::unordered_map<std::string, float> authorPreferences;
    
    // Temporal usage patterns
    std::unordered_map<std::string, float> timeContextPreferences; // "morning", "evening", etc.
    
    // Session workflow patterns
    std::vector<std::vector<std::string>> commonWorkflows; // Sequences of preset categories used together
    
    // Diversity preference (0.0 = likes similar presets, 1.0 = likes variety)
    float diversityPreference = 0.5f;
    
    // Exploration vs. exploitation balance
    float explorationFactor = 0.3f; // 0.0 = stick to known preferences, 1.0 = try new things
    
    // Last update timestamp
    std::chrono::system_clock::time_point lastUpdated;
    
    /**
     * @brief Calculate preference score for a preset based on profile
     * @param features Audio features of the preset
     * @param category Preset category
     * @param author Preset author
     * @return Preference score (0.0-1.0)
     */
    float calculatePreferenceScore(const AudioFeatureVector& features, 
                                 const std::string& category,
                                 const std::string& author) const;
};

/**
 * @brief Recommendation with explanation and confidence
 */
struct PresetRecommendation {
    std::string presetPath;
    float relevanceScore;           // How relevant this is to the request (0.0-1.0)
    float confidenceScore;          // How confident we are in this recommendation (0.0-1.0)
    float noveltyScore;             // How novel/different this is (0.0-1.0)
    
    // Explanation components
    struct Explanation {
        std::string primary;        // Main reason ("Similar to your favorites")
        std::vector<std::string> factors; // Contributing factors
        std::string algorithm;      // Which algorithm produced this
    } explanation;
    
    // Context metadata
    std::string recommendationType; // "similar", "discovery", "workflow", "trending"
    std::vector<std::string> sourcePresets; // Presets this recommendation is based on
    float userProfileMatch = 0.0f;  // How well this matches user profile
    
    bool operator<(const PresetRecommendation& other) const {
        return relevanceScore > other.relevanceScore; // Higher relevance first
    }
};

/**
 * @brief Context for generating recommendations
 */
struct RecommendationContext {
    std::string currentPreset;              // Currently selected/loaded preset
    std::vector<std::string> recentPresets; // Recently used presets (last 10)
    std::string sessionType;                // "creative", "mixing", "performance", etc.
    std::string timeOfDay;                  // "morning", "afternoon", "evening", "night"
    std::string genre;                      // User-specified or detected genre context
    std::vector<std::string> tags;          // Additional context tags
    int maxRecommendations = 10;            // Maximum number of recommendations to return
    float diversityWeight = 0.3f;           // Balance between relevance and diversity
    bool includeExplanations = true;        // Include human-readable explanations
};

/**
 * @brief Advanced recommendation engine with machine learning and collaborative filtering
 * Implements multiple recommendation strategies inspired by modern recommendation systems
 */
class PresetRecommendationEngine {
public:
    explicit PresetRecommendationEngine(std::shared_ptr<PresetMLAnalyzer> analyzer);
    ~PresetRecommendationEngine();
    
    // Core recommendation methods
    
    /**
     * @brief Get personalized recommendations for the user
     * @param context Recommendation context and constraints
     * @return Vector of ranked recommendations with explanations
     */
    std::vector<PresetRecommendation> getRecommendations(const RecommendationContext& context);
    
    /**
     * @brief Find presets similar to a reference preset
     * @param referencePreset Path to reference preset
     * @param maxResults Maximum number of recommendations
     * @param diversityWeight Weight for diversity vs. similarity (0.0-1.0)
     * @return Vector of similar preset recommendations
     */
    std::vector<PresetRecommendation> getSimilarPresets(
        const std::string& referencePreset,
        int maxResults = 10,
        float diversityWeight = 0.2f
    );
    
    /**
     * @brief Get discovery recommendations (novel presets user might like)
     * @param explorationLevel How adventurous to be (0.0 = safe, 1.0 = very experimental)
     * @param maxResults Maximum number of recommendations
     * @return Vector of discovery recommendations
     */
    std::vector<PresetRecommendation> getDiscoveryRecommendations(
        float explorationLevel = 0.5f,
        int maxResults = 10
    );
    
    /**
     * @brief Get workflow-based recommendations (next preset in typical workflows)
     * @param currentWorkflow Recent presets used in current session
     * @param maxResults Maximum number of recommendations
     * @return Vector of workflow-based recommendations
     */
    std::vector<PresetRecommendation> getWorkflowRecommendations(
        const std::vector<std::string>& currentWorkflow,
        int maxResults = 10
    );
    
    /**
     * @brief Get trending/popular recommendations
     * @param timeWindow Time window for popularity calculation (hours)
     * @param maxResults Maximum number of recommendations
     * @return Vector of trending preset recommendations
     */
    std::vector<PresetRecommendation> getTrendingRecommendations(
        int timeWindow = 168, // 1 week
        int maxResults = 10
    );
    
    // User interaction tracking and learning
    
    /**
     * @brief Record user interaction with a preset
     * @param interaction User interaction data
     */
    void recordInteraction(const UserInteraction& interaction);
    
    /**
     * @brief Record multiple interactions efficiently
     * @param interactions Vector of interaction data
     */
    void recordInteractions(const std::vector<UserInteraction>& interactions);
    
    /**
     * @brief Provide feedback on recommendation quality
     * @param recommendedPreset Path to recommended preset
     * @param wasUseful Whether the recommendation was useful (true/false)
     * @param rating User rating (1-5, optional)
     */
    void provideFeedback(const std::string& recommendedPreset, bool wasUseful, int rating = 0);
    
    /**
     * @brief Update user profile based on recent interactions
     * This should be called periodically to keep recommendations fresh
     */
    void updateUserProfile();
    
    // Configuration and tuning
    
    /**
     * @brief Set algorithm weights for different recommendation types
     * @param weights Map of algorithm names to weights
     */
    void setAlgorithmWeights(const std::unordered_map<std::string, float>& weights);
    
    /**
     * @brief Configure recommendation parameters
     * @param params Parameter map for fine-tuning behavior
     */
    void setRecommendationParameters(const std::unordered_map<std::string, float>& params);
    
    /**
     * @brief Set minimum interaction count for reliable recommendations
     * @param minInteractions Minimum number of interactions before providing recommendations
     */
    void setMinimumInteractions(int minInteractions);
    
    // Batch processing and precomputation
    
    /**
     * @brief Precompute similarity matrices for faster recommendations
     * @param presets List of all available presets
     * @param progressCallback Progress callback function (current, total)
     */
    void precomputeSimilarities(const std::vector<PresetInfo>& presets,
                               std::function<void(int, int)> progressCallback = nullptr);
    
    /**
     * @brief Update precomputed data with new presets
     * @param newPresets List of newly added presets
     */
    void updatePrecomputedData(const std::vector<PresetInfo>& newPresets);
    
    /**
     * @brief Clear all precomputed data (forces recomputation)
     */
    void clearPrecomputedData();
    
    // Analytics and insights
    
    struct RecommendationStats {
        int totalRecommendations = 0;
        int successfulRecommendations = 0;
        float averageRelevanceScore = 0.0f;
        float averageUserRating = 0.0f;
        std::unordered_map<std::string, int> algorithmUsage;
        std::unordered_map<std::string, float> algorithmSuccessRate;
        std::chrono::system_clock::time_point lastUpdated;
    };
    
    /**
     * @brief Get recommendation system statistics
     * @return Current statistics and performance metrics
     */
    RecommendationStats getStatistics() const;
    
    /**
     * @brief Get user profile summary
     * @return Current user preferences and learned patterns
     */
    UserProfile getUserProfile() const;
    
    /**
     * @brief Export user data for backup/analysis
     * @return JSON representation of user interactions and profile
     */
    nlohmann::json exportUserData() const;
    
    /**
     * @brief Import user data from backup
     * @param userData JSON representation of user data
     * @return Success/failure status
     */
    bool importUserData(const nlohmann::json& userData);

private:
    // Core components
    std::shared_ptr<PresetMLAnalyzer> analyzer_;
    
    // User data and learning
    UserProfile userProfile_;
    std::vector<UserInteraction> interactionHistory_;
    std::unordered_map<std::string, float> presetPopularity_;
    
    // Algorithm configurations
    std::unordered_map<std::string, float> algorithmWeights_;
    std::unordered_map<std::string, float> recommendationParams_;
    int minimumInteractions_ = 10;
    
    // Precomputed data for performance
    std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> similarityCache_;
    std::unordered_map<std::string, AudioFeatureVector> featureCache_;
    mutable std::mutex precomputeMutex_;
    
    // Statistics and monitoring
    mutable RecommendationStats stats_;
    std::unordered_map<std::string, std::vector<float>> feedbackHistory_;
    
    // Algorithm implementations
    
    /**
     * @brief Content-based filtering using audio features
     * @param context Recommendation context
     * @return Vector of content-based recommendations
     */
    std::vector<PresetRecommendation> contentBasedRecommendations(const RecommendationContext& context);
    
    /**
     * @brief Collaborative filtering based on user similarities
     * @param context Recommendation context
     * @return Vector of collaborative filtering recommendations
     */
    std::vector<PresetRecommendation> collaborativeFilteringRecommendations(const RecommendationContext& context);
    
    /**
     * @brief Hybrid approach combining multiple algorithms
     * @param contentRecs Content-based recommendations
     * @param collaborativeRecs Collaborative filtering recommendations
     * @param context Recommendation context
     * @return Vector of hybrid recommendations
     */
    std::vector<PresetRecommendation> hybridRecommendations(
        const std::vector<PresetRecommendation>& contentRecs,
        const std::vector<PresetRecommendation>& collaborativeRecs,
        const RecommendationContext& context
    );
    
    // User profile learning
    
    /**
     * @brief Learn feature preferences from user interactions
     */
    void learnFeaturePreferences();
    
    /**
     * @brief Learn category preferences from user interactions
     */
    void learnCategoryPreferences();
    
    /**
     * @brief Learn temporal usage patterns
     */
    void learnTemporalPatterns();
    
    /**
     * @brief Learn workflow patterns from interaction sequences
     */
    void learnWorkflowPatterns();
    
    // Recommendation optimization
    
    /**
     * @brief Apply diversity to recommendation list
     * @param recommendations List of recommendations to diversify
     * @param diversityWeight Weight for diversity vs. relevance
     * @return Diversified recommendation list
     */
    std::vector<PresetRecommendation> applyDiversification(
        std::vector<PresetRecommendation> recommendations,
        float diversityWeight
    );
    
    /**
     * @brief Calculate novelty score for a preset
     * @param presetPath Path to preset
     * @param userInteractions User's interaction history
     * @return Novelty score (0.0-1.0)
     */
    float calculateNoveltyScore(const std::string& presetPath, 
                               const std::vector<UserInteraction>& userInteractions);
    
    /**
     * @brief Generate explanation for a recommendation
     * @param recommendation Recommendation to explain
     * @param algorithm Algorithm that generated the recommendation
     * @param context Recommendation context
     * @return Human-readable explanation
     */
    PresetRecommendation::Explanation generateExplanation(
        const PresetRecommendation& recommendation,
        const std::string& algorithm,
        const RecommendationContext& context
    );
    
    // Utility methods
    
    /**
     * @brief Get time context string from timestamp
     * @param timestamp Time to categorize
     * @return Context string ("morning", "afternoon", etc.)
     */
    std::string getTimeContext(const std::chrono::system_clock::time_point& timestamp);
    
    /**
     * @brief Calculate interaction weight based on type and recency
     * @param interaction User interaction data
     * @return Weighted interaction value
     */
    float calculateInteractionWeight(const UserInteraction& interaction);
    
    /**
     * @brief Filter interactions by time window
     * @param interactions All interactions
     * @param hours Time window in hours
     * @return Filtered interactions within time window
     */
    std::vector<UserInteraction> filterInteractionsByTime(
        const std::vector<UserInteraction>& interactions,
        int hours
    );
    
    /**
     * @brief Initialize default algorithm weights and parameters
     */
    void initializeDefaults();
    
    /**
     * @brief Update recommendation statistics
     * @param recommendations Generated recommendations
     * @param algorithm Algorithm used
     */
    void updateStatistics(const std::vector<PresetRecommendation>& recommendations,
                         const std::string& algorithm);
};

} // namespace AIMusicHardware