#pragma once

#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include <chrono>

namespace AIMusicHardware {

// Forward declarations
class Parameter;
class UIComponent;
class ParameterBridge;

/**
 * @brief MIDI CC (Continuous Controller) Learning System
 * 
 * Professional MIDI controller mapping system inspired by industry standards.
 * Automatically learns and maps MIDI CC messages to synthesizer parameters.
 */
class MidiCCLearning {
public:
    /**
     * @brief MIDI CC mapping information
     */
    struct CCMapping {
        int channel = -1;           // MIDI channel (-1 = any channel)
        int ccNumber = -1;          // CC number (0-127)
        std::string parameterId;    // Parameter ID to control
        float minValue = 0.0f;      // Minimum parameter value
        float maxValue = 1.0f;      // Maximum parameter value
        bool inverted = false;      // Invert CC direction
        float smoothing = 0.0f;     // Smoothing factor (0.0 = none, 0.95 = heavy)
        
        // Curve types for non-linear mapping
        enum class CurveType {
            Linear,
            Exponential,
            Logarithmic,
            SShape       // S-curve for musical response
        } curveType = CurveType::Linear;
        
        // Learning metadata
        std::chrono::system_clock::time_point learnTime;
        std::string deviceName;
        bool isActive = true;
    };
    
    /**
     * @brief Learning state
     */
    enum class LearningState {
        Idle,           // Not learning
        WaitingForCC,   // Waiting for CC input
        WaitingForParam, // Waiting for parameter selection
        Learning        // Actively learning mapping
    };
    
    /**
     * @brief Learning mode options
     */
    enum class LearningMode {
        Manual,         // User initiates each mapping
        Auto,           // Automatic detection of CC usage
        Batch          // Learn multiple mappings in sequence
    };

    /**
     * @brief Constructor
     */
    MidiCCLearning();
    
    /**
     * @brief Destructor
     */
    ~MidiCCLearning();
    
    // Learning interface
    
    /**
     * @brief Start learning mode for a specific parameter
     * @param parameterId Parameter to learn mapping for
     * @param timeout Learning timeout in milliseconds
     * @return true if learning started successfully
     */
    bool startLearning(const std::string& parameterId, 
                      std::chrono::milliseconds timeout = std::chrono::milliseconds{5000});
    
    /**
     * @brief Start auto-learning mode
     * @param duration How long to monitor for CC activity
     * @return true if auto-learning started
     */
    bool startAutoLearning(std::chrono::milliseconds duration = std::chrono::milliseconds{30000});
    
    /**
     * @brief Stop current learning session
     */
    void stopLearning();
    
    /**
     * @brief Get current learning state
     */
    LearningState getLearningState() const { return learningState_.load(); }
    
    /**
     * @brief Set learning mode
     */
    void setLearningMode(LearningMode mode) { learningMode_ = mode; }
    
    /**
     * @brief Get learning mode
     */
    LearningMode getLearningMode() const { return learningMode_; }
    
    // MIDI input processing
    
    /**
     * @brief Process incoming MIDI CC message
     * @param channel MIDI channel (0-15)
     * @param ccNumber CC number (0-127)
     * @param value CC value (0-127)
     * @param deviceName Name of input device (optional)
     */
    void processMidiCC(int channel, int ccNumber, int value, const std::string& deviceName = "");
    
    // Mapping management
    
    /**
     * @brief Create manual mapping
     * @param mapping Mapping configuration
     * @return true if mapping was created successfully
     */
    bool createMapping(const CCMapping& mapping);
    
    /**
     * @brief Remove mapping by CC
     * @param channel MIDI channel
     * @param ccNumber CC number
     * @return true if mapping was removed
     */
    bool removeMapping(int channel, int ccNumber);
    
    /**
     * @brief Remove mapping by parameter
     * @param parameterId Parameter ID
     * @return true if mapping was removed
     */
    bool removeMapping(const std::string& parameterId);
    
    /**
     * @brief Get mapping for CC
     * @param channel MIDI channel
     * @param ccNumber CC number
     * @return Mapping pointer or nullptr if not found
     */
    const CCMapping* getMapping(int channel, int ccNumber) const;
    
    /**
     * @brief Get mapping for parameter
     * @param parameterId Parameter ID
     * @return Mapping pointer or nullptr if not found
     */
    const CCMapping* getMapping(const std::string& parameterId) const;
    
    /**
     * @brief Get all mappings
     * @return Vector of all current mappings
     */
    std::vector<CCMapping> getAllMappings() const;
    
    /**
     * @brief Clear all mappings
     */
    void clearAllMappings();
    
    // Configuration
    
    /**
     * @brief Enable/disable learning system
     * @param enabled Whether learning is enabled
     */
    void setEnabled(bool enabled) { enabled_.store(enabled); }
    
    /**
     * @brief Check if learning is enabled
     */
    bool isEnabled() const { return enabled_.load(); }
    
    /**
     * @brief Set default learning timeout
     * @param timeout Default timeout for learning sessions
     */
    void setDefaultTimeout(std::chrono::milliseconds timeout) { defaultTimeout_ = timeout; }
    
    /**
     * @brief Set learning sensitivity (how much CC movement required)
     * @param sensitivity Minimum CC change to register (0-127)
     */
    void setLearningSensitivity(int sensitivity) { learningSensitivity_ = sensitivity; }
    
    /**
     * @brief Enable/disable automatic curve detection
     * @param enabled Whether to auto-detect appropriate curves
     */
    void setAutoCurveDetection(bool enabled) { autoCurveDetection_ = enabled; }
    
    // Callbacks
    
    /**
     * @brief Callback for successful mapping creation
     */
    using MappingCallback = std::function<void(const CCMapping&)>;
    
    /**
     * @brief Callback for parameter value changes
     */
    using ParameterChangeCallback = std::function<void(const std::string& parameterId, float value)>;
    
    /**
     * @brief Callback for learning state changes
     */
    using LearningStateCallback = std::function<void(LearningState state, const std::string& message)>;
    
    /**
     * @brief Set callback for successful mappings
     */
    void setMappingCreatedCallback(MappingCallback callback) { mappingCreatedCallback_ = callback; }
    
    /**
     * @brief Set callback for parameter changes
     */
    void setParameterChangeCallback(ParameterChangeCallback callback) { parameterChangeCallback_ = callback; }
    
    /**
     * @brief Set callback for learning state changes
     */
    void setLearningStateCallback(LearningStateCallback callback) { learningStateCallback_ = callback; }
    
    // Persistence
    
    /**
     * @brief Save mappings to file
     * @param filePath Path to save mappings
     * @return true if saved successfully
     */
    bool saveMappings(const std::string& filePath) const;
    
    /**
     * @brief Load mappings from file
     * @param filePath Path to load mappings from
     * @return true if loaded successfully
     */
    bool loadMappings(const std::string& filePath);
    
    /**
     * @brief Get default mappings file path
     */
    static std::string getDefaultMappingsPath();
    
    // Statistics and monitoring
    
    /**
     * @brief Learning statistics
     */
    struct LearningStats {
        int totalMappings = 0;
        int activeMappings = 0;
        int messagesProcessed = 0;
        int learningSessionsCompleted = 0;
        std::chrono::system_clock::time_point lastActivity;
        std::map<int, int> ccUsageCount; // CC number -> usage count
    };
    
    /**
     * @brief Get learning statistics
     */
    LearningStats getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

private:
    // Internal state
    std::atomic<LearningState> learningState_{LearningState::Idle};
    std::atomic<bool> enabled_{true};
    LearningMode learningMode_ = LearningMode::Manual;
    
    // Learning session data
    std::string currentParameterId_;
    std::chrono::system_clock::time_point learningStartTime_;
    std::chrono::milliseconds learningTimeout_;
    std::chrono::milliseconds defaultTimeout_{5000};
    int learningSensitivity_ = 5; // Minimum CC change to register
    bool autoCurveDetection_ = true;
    
    // CC activity tracking for auto-learning
    struct CCActivity {
        int lastValue = -1;
        int changeCount = 0;
        std::chrono::system_clock::time_point lastActivity;
        std::string deviceName;
    };
    std::map<std::pair<int, int>, CCActivity> ccActivity_; // (channel, cc) -> activity
    
    // Mappings storage
    mutable std::mutex mappingsMutex_;
    std::map<std::pair<int, int>, CCMapping> mappings_; // (channel, cc) -> mapping
    std::map<std::string, std::pair<int, int>> parameterToCC_; // parameter -> (channel, cc)
    
    // Callbacks
    MappingCallback mappingCreatedCallback_;
    ParameterChangeCallback parameterChangeCallback_;
    LearningStateCallback learningStateCallback_;
    
    // Statistics
    mutable std::mutex statsMutex_;
    LearningStats stats_;
    
    // Internal methods
    void updateLearningState(LearningState newState, const std::string& message = "");
    void processLearningCC(int channel, int ccNumber, int value, const std::string& deviceName);
    void processNormalCC(int channel, int ccNumber, int value);
    float convertCCValue(int ccValue, const CCMapping& mapping) const;
    float applyCurve(float normalizedValue, CCMapping::CurveType curveType) const;
    CCMapping::CurveType detectOptimalCurve(const std::string& parameterId) const;
    void notifyParameterChange(const std::string& parameterId, float value);
    void updateStatistics(int channel, int ccNumber);
    bool isLearningTimedOut() const;
    void completeMapping(int channel, int ccNumber, const std::string& deviceName);
    
    // Auto-learning methods
    void updateCCActivity(int channel, int ccNumber, int value, const std::string& deviceName);
    void processAutoLearning();
    std::vector<std::pair<int, int>> getActiveCCs() const;
    
    // Persistence helpers
    std::string mappingToJson(const CCMapping& mapping) const;
    CCMapping mappingFromJson(const std::string& json) const;
};

/**
 * @brief Global MIDI CC Learning manager
 * 
 * Singleton instance for application-wide CC learning
 */
class MidiCCLearningManager {
public:
    static MidiCCLearningManager& getInstance();
    
    MidiCCLearning& getLearning() { return learning_; }
    
    /**
     * @brief Initialize with MIDI input callback
     */
    void initialize();
    
    /**
     * @brief Shutdown and save mappings
     */
    void shutdown();

private:
    MidiCCLearningManager() = default;
    ~MidiCCLearningManager() = default;
    
    MidiCCLearning learning_;
    bool initialized_ = false;
};

} // namespace AIMusicHardware