#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <array>
#include "ui/presets/PresetInfo.h"

namespace AIMusicHardware {

/**
 * @brief Audio feature vector for machine learning analysis
 */
struct AudioFeatureVector {
    // Spectral features
    std::array<float, 12> chromaVector;        // Chroma features (12 semitones)
    std::array<float, 13> mfccVector;          // Mel-frequency cepstral coefficients
    std::array<float, 8> spectralMoments;     // Spectral centroid, spread, skewness, kurtosis, etc.
    
    // Temporal features
    float tempo = 0.0f;
    float rhythmComplexity = 0.0f;
    float attackTime = 0.0f;
    float releaseTime = 0.0f;
    
    // Harmonic features
    float harmonicity = 0.0f;
    float fundamentalFrequency = 0.0f;
    float inharmonicity = 0.0f;
    
    // Timbral features
    float brightness = 0.0f;
    float warmth = 0.0f;
    float roughness = 0.0f;
    float sharpness = 0.0f;
    
    // Energy distribution
    std::array<float, 10> energyBands;        // Energy in frequency bands
    float totalEnergy = 0.0f;
    float dynamicRange = 0.0f;
    
    // Modulation features
    float lfoDepth = 0.0f;
    float filterMovement = 0.0f;
    float amplitudeModulation = 0.0f;
    float frequencyModulation = 0.0f;
    
    // Synthesis-specific features
    float oscillatorComplexity = 0.0f;
    float filterResonance = 0.0f;
    float effectsComplexity = 0.0f;
    float voiceCount = 0.0f;
    
    /**
     * @brief Calculate distance between two feature vectors
     * @param other Another feature vector
     * @param weights Weights for different feature categories
     * @return Distance value (0.0 = identical, higher = more different)
     */
    float calculateDistance(const AudioFeatureVector& other, 
                          const std::unordered_map<std::string, float>& weights = {}) const;
    
    /**
     * @brief Normalize feature vector for machine learning
     */
    void normalize();
    
    /**
     * @brief Convert to dense vector for ML algorithms
     * @return Flattened feature vector
     */
    std::vector<float> toDenseVector() const;
    
    /**
     * @brief Load from dense vector
     * @param denseVector Flattened feature vector
     */
    void fromDenseVector(const std::vector<float>& denseVector);
};

/**
 * @brief Preset similarity result with confidence score
 */
struct PresetSimilarity {
    std::string presetPath;
    float similarityScore;      // 0.0 = completely different, 1.0 = identical
    float confidenceScore;      // 0.0 = low confidence, 1.0 = high confidence
    std::string similarityReason; // Human-readable explanation
    AudioFeatureVector features;
    
    bool operator<(const PresetSimilarity& other) const {
        return similarityScore > other.similarityScore; // Sort by highest similarity first
    }
};

/**
 * @brief Machine learning preset categorization result
 */
struct PresetCategorization {
    std::string suggestedCategory;
    float confidence;
    std::vector<std::pair<std::string, float>> alternativeCategories;
    std::vector<std::string> suggestedTags;
    std::string reasoning;
};

/**
 * @brief Advanced machine learning analyzer for preset audio characteristics
 * Based on modern audio analysis techniques and synthesizer parameter correlation
 */
class PresetMLAnalyzer {
public:
    PresetMLAnalyzer();
    ~PresetMLAnalyzer();
    
    // Core analysis functions
    
    /**
     * @brief Extract comprehensive audio features from preset parameters
     * @param preset Preset to analyze
     * @return Audio feature vector
     */
    AudioFeatureVector extractFeatures(const PresetInfo& preset);
    
    /**
     * @brief Find presets similar to a reference preset
     * @param reference Reference preset for comparison
     * @param candidates Pool of presets to search through
     * @param maxResults Maximum number of similar presets to return
     * @param minSimilarity Minimum similarity threshold (0.0-1.0)
     * @return Vector of similar presets sorted by similarity
     */
    std::vector<PresetSimilarity> findSimilarPresets(
        const PresetInfo& reference,
        const std::vector<PresetInfo>& candidates,
        int maxResults = 10,
        float minSimilarity = 0.3f
    );
    
    /**
     * @brief Suggest category for preset based on audio analysis
     * @param preset Preset to categorize
     * @return Categorization result with confidence
     */
    PresetCategorization suggestCategory(const PresetInfo& preset);
    
    /**
     * @brief Generate automatic tags based on audio characteristics
     * @param preset Preset to tag
     * @return Vector of suggested tags with relevance scores
     */
    std::vector<std::pair<std::string, float>> suggestTags(const PresetInfo& preset);
    
    /**
     * @brief Detect if preset is likely a duplicate or very similar variant
     * @param preset Preset to check
     * @param existingPresets Pool of existing presets
     * @param threshold Similarity threshold for duplicate detection
     * @return Vector of potential duplicates
     */
    std::vector<PresetSimilarity> detectDuplicates(
        const PresetInfo& preset,
        const std::vector<PresetInfo>& existingPresets,
        float threshold = 0.85f
    );
    
    // Machine learning training and model management
    
    /**
     * @brief Train categorization model on user-labeled data
     * @param labeledPresets Presets with user-confirmed categories
     * @return Training success and accuracy metrics
     */
    struct TrainingResult {
        bool success;
        float accuracy;
        float precision;
        float recall;
        int trainingSetSize;
        std::string modelVersion;
    };
    TrainingResult trainCategorizationModel(
        const std::vector<std::pair<PresetInfo, std::string>>& labeledPresets
    );
    
    /**
     * @brief Update similarity weights based on user feedback
     * @param userFeedback Pairs of (preset1, preset2, user_similarity_rating)
     */
    void updateSimilarityWeights(
        const std::vector<std::tuple<PresetInfo, PresetInfo, float>>& userFeedback
    );
    
    // Configuration and optimization
    
    /**
     * @brief Set feature extraction parameters
     * @param params Parameter map for fine-tuning analysis
     */
    void setAnalysisParameters(const std::unordered_map<std::string, float>& params);
    
    /**
     * @brief Enable/disable specific feature categories
     * @param spectral Include spectral analysis features
     * @param temporal Include temporal analysis features
     * @param harmonic Include harmonic analysis features
     * @param synthesis Include synthesizer-specific features
     */
    void setFeatureCategories(bool spectral, bool temporal, bool harmonic, bool synthesis);
    
    /**
     * @brief Set similarity calculation weights
     * @param weights Weight map for different feature categories
     */
    void setSimilarityWeights(const std::unordered_map<std::string, float>& weights);
    
    // Batch processing and caching
    
    /**
     * @brief Process multiple presets efficiently
     * @param presets Vector of presets to process
     * @param callback Progress callback function
     * @return Map of preset paths to feature vectors
     */
    std::unordered_map<std::string, AudioFeatureVector> batchExtractFeatures(
        const std::vector<PresetInfo>& presets,
        std::function<void(int, int)> progressCallback = nullptr
    );
    
    /**
     * @brief Cache feature vectors for fast lookup
     * @param presetPath Preset file path
     * @param features Feature vector to cache
     */
    void cacheFeatures(const std::string& presetPath, const AudioFeatureVector& features);
    
    /**
     * @brief Get cached features if available
     * @param presetPath Preset file path
     * @return Feature vector if cached, nullptr otherwise
     */
    const AudioFeatureVector* getCachedFeatures(const std::string& presetPath) const;
    
    /**
     * @brief Clear feature cache
     */
    void clearCache();
    
    // Statistics and performance monitoring
    
    struct AnalysisStats {
        int totalAnalyzed = 0;
        int cacheHits = 0;
        int cacheMisses = 0;
        float averageAnalysisTime = 0.0f;
        float averageSimilarityTime = 0.0f;
        std::string modelVersion;
        float modelAccuracy = 0.0f;
    };
    
    /**
     * @brief Get analysis statistics
     * @return Current statistics
     */
    AnalysisStats getStatistics() const;
    
    /**
     * @brief Reset statistics counters
     */
    void resetStatistics();

private:
    // Feature extraction implementation
    AudioFeatureVector extractSpectralFeatures(const PresetInfo& preset);
    AudioFeatureVector extractTemporalFeatures(const PresetInfo& preset);
    AudioFeatureVector extractHarmonicFeatures(const PresetInfo& preset);
    AudioFeatureVector extractSynthesisFeatures(const PresetInfo& preset);
    
    // Parameter analysis helpers
    float analyzeOscillatorComplexity(const nlohmann::json& parameters);
    float analyzeFilterCharacteristics(const nlohmann::json& parameters);
    float analyzeEnvelopeComplexity(const nlohmann::json& parameters);
    float analyzeModulationDepth(const nlohmann::json& parameters);
    float analyzeEffectsProcessing(const nlohmann::json& parameters);
    
    // Similarity calculation methods
    float calculateSpectralSimilarity(const AudioFeatureVector& a, const AudioFeatureVector& b);
    float calculateTemporalSimilarity(const AudioFeatureVector& a, const AudioFeatureVector& b);
    float calculateHarmonicSimilarity(const AudioFeatureVector& a, const AudioFeatureVector& b);
    float calculateTimbralSimilarity(const AudioFeatureVector& a, const AudioFeatureVector& b);
    
    // Machine learning model components
    std::vector<std::string> categoryLabels_;
    std::vector<std::vector<float>> categoryModel_;  // Simple linear classifier weights
    std::unordered_map<std::string, float> similarityWeights_;
    std::unordered_map<std::string, float> analysisParameters_;
    
    // Feature selection flags
    bool useSpectralFeatures_ = true;
    bool useTemporalFeatures_ = true;
    bool useHarmonicFeatures_ = true;
    bool useSynthesisFeatures_ = true;
    
    // Caching and performance
    mutable std::unordered_map<std::string, AudioFeatureVector> featureCache_;
    mutable std::mutex cacheMutex_;
    mutable AnalysisStats stats_;
    
    // Default category mappings based on audio characteristics
    void initializeDefaultCategories();
    void initializeDefaultWeights();
    void initializeDefaultParameters();
    
    // Utility functions
    std::vector<float> normalizeVector(const std::vector<float>& input);
    float euclideanDistance(const std::vector<float>& a, const std::vector<float>& b);
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
    
    // Tag suggestion based on feature analysis
    std::vector<std::pair<std::string, float>> analyzeGenreTags(const AudioFeatureVector& features);
    std::vector<std::pair<std::string, float>> analyzeMoodTags(const AudioFeatureVector& features);
    std::vector<std::pair<std::string, float>> analyzeInstrumentTags(const AudioFeatureVector& features);
    std::vector<std::pair<std::string, float>> analyzeTechnicalTags(const AudioFeatureVector& features);
};

} // namespace AIMusicHardware