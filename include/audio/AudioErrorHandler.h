#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <map>
#include <atomic>

namespace AIMusicHardware {

/**
 * @brief Error severity levels for audio operations
 */
enum class AudioErrorSeverity {
    Info,       // Informational messages (buffer adjustments, device changes)
    Warning,    // Non-critical issues (underruns, minor glitches)
    Error,      // Recoverable errors (device disconnects, driver issues)
    Critical    // System-threatening errors (total audio failure, corruption)
};

/**
 * @brief Error codes for specific audio operation failures
 */
enum class AudioErrorCode {
    // Hardware/Device errors
    DeviceNotFound = 1000,
    DeviceDisconnected = 1001,
    DeviceConfigurationFailed = 1002,
    DeviceDriverError = 1003,
    UnsupportedSampleRate = 1004,
    UnsupportedBufferSize = 1005,
    
    // Stream errors
    StreamOpenFailed = 2000,
    StreamStartFailed = 2001,
    StreamStopFailed = 2002,
    StreamUnderrun = 2003,
    StreamOverrun = 2004,
    StreamDropout = 2005,
    StreamLatencyTooHigh = 2006,
    
    // Real-time processing errors
    CallbackTimeout = 3000,
    CallbackException = 3001,
    CallbackNullPointer = 3002,
    CallbackCPUOverload = 3003,
    CallbackMemoryViolation = 3004,
    CallbackDeadlock = 3005,
    
    // Audio processing errors
    SampleRateConversionFailed = 4000,
    ChannelMixingFailed = 4001,
    BufferOverflow = 4002,
    BufferUnderflow = 4003,
    AudioClipping = 4004,
    DCOffsetDetected = 4005,
    
    // System resource errors
    OutOfMemory = 5000,
    CPUOverload = 5001,
    ThreadPriorityFailed = 5002,
    SystemLatencyTooHigh = 5003,
    PageFaultInCallback = 5004,
    
    // Thread safety errors
    ConcurrentAccess = 6000,
    RaceCondition = 6001,
    DeadlockDetected = 6002,
    LockTimeout = 6003,
    AtomicOperationFailed = 6004,
    
    // Performance errors
    PerformanceGoalMissed = 7000,
    LatencyBudgetExceeded = 7001,
    ThroughputTooLow = 7002,
    JitterTooHigh = 7003,
    
    // Safety errors
    AudioSafetyViolation = 8000,
    VolumeClampingActivated = 8001,
    EmergencyMute = 8002,
    GainStageOverload = 8003,
    
    // Unknown/Generic
    Unknown = 9999
};

/**
 * @brief Detailed audio error information with real-time context
 */
struct AudioError {
    AudioErrorCode code;
    AudioErrorSeverity severity;
    std::string message;
    std::string context;           // What was being done when error occurred
    std::string function;          // Function where error occurred
    int line = 0;                  // Line number where error occurred
    std::chrono::system_clock::time_point timestamp;
    
    // Audio-specific context
    int sampleRate = 0;
    int bufferSize = 0;
    int channelCount = 0;
    double streamTime = 0.0;
    float cpuLoad = 0.0f;
    float memoryUsage = 0.0f;
    
    // Performance metrics at time of error
    std::chrono::microseconds latency{0};
    std::chrono::microseconds jitter{0};
    int consecutiveUnderruns = 0;
    
    // Recovery suggestions
    std::vector<std::string> recoverySuggestions;
    bool isRecoverable = false;
    bool requiresRestart = false;
    
    // Additional metadata
    std::map<std::string, std::string> metadata;
    
    AudioError(AudioErrorCode code, 
               AudioErrorSeverity severity,
               const std::string& message,
               const std::string& context = "",
               const std::string& function = "",
               int line = 0)
        : code(code), severity(severity), message(message), context(context),
          function(function), line(line), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Audio-specific recovery action with real-time considerations
 */
struct AudioRecoveryAction {
    std::string description;
    std::function<bool()> action;
    int priority = 0;              // Higher priority actions tried first
    int maxRetries = 3;
    std::chrono::microseconds maxDelay{1000}; // Max delay in RT context
    bool allowInRealTime = false;  // Whether safe to execute in audio callback
    bool requiresAudioStop = false; // Whether audio must be stopped first
};

/**
 * @brief Audio recovery result with timing information
 */
struct AudioRecoveryResult {
    bool successful = false;
    std::string actionTaken;
    std::string resultMessage;
    int retriesUsed = 0;
    std::chrono::microseconds timeSpent{0};
    bool wasRealTime = false;      // Whether recovery happened in RT context
};

/**
 * @brief Enterprise-grade error handling and recovery system for audio processing
 */
class AudioErrorHandler {
public:
    using ErrorCallback = std::function<void(const AudioError&)>;
    using RecoveryCallback = std::function<void(const AudioError&, const AudioRecoveryResult&)>;
    using CriticalErrorCallback = std::function<void(const AudioError&)>;
    
    /**
     * @brief Constructor
     */
    AudioErrorHandler();
    
    /**
     * @brief Destructor
     */
    ~AudioErrorHandler();
    
    // Error reporting and logging
    
    /**
     * @brief Report an error with automatic recovery attempt
     * @param error Error details
     * @param isRealTime Whether called from real-time audio context
     * @return Recovery result
     */
    AudioRecoveryResult reportError(const AudioError& error, bool isRealTime = false);
    
    /**
     * @brief Report error with simple parameters
     */
    AudioRecoveryResult reportError(AudioErrorCode code, 
                                   AudioErrorSeverity severity,
                                   const std::string& message,
                                   const std::string& context = "",
                                   const std::string& function = "",
                                   int line = 0,
                                   bool isRealTime = false);
    
    /**
     * @brief Report critical error that requires immediate attention
     */
    void reportCriticalError(const AudioError& error);
    
    /**
     * @brief Fast error reporting for real-time contexts (lock-free)
     */
    void reportRealTimeError(AudioErrorCode code, const std::string& message);
    
    // Performance monitoring integration
    
    /**
     * @brief Update performance metrics
     */
    void updatePerformanceMetrics(float cpuLoad, float memoryUsage, 
                                 std::chrono::microseconds latency,
                                 std::chrono::microseconds jitter);
    
    /**
     * @brief Set audio stream context for error reporting
     */
    void setStreamContext(int sampleRate, int bufferSize, int channelCount);
    
    /**
     * @brief Update stream time for error correlation
     */
    void updateStreamTime(double streamTime);
    
    // Recovery system management
    
    /**
     * @brief Register recovery action for specific error code
     * @param errorCode Error code to handle
     * @param action Recovery action
     */
    void registerRecoveryAction(AudioErrorCode errorCode, const AudioRecoveryAction& action);
    
    /**
     * @brief Remove recovery action for error code
     * @param errorCode Error code
     */
    void removeRecoveryAction(AudioErrorCode errorCode);
    
    /**
     * @brief Clear all recovery actions
     */
    void clearRecoveryActions();
    
    // Error history and analysis
    
    /**
     * @brief Get recent errors
     * @param maxCount Maximum number of errors to return
     * @param minSeverity Minimum severity level to include
     * @return Vector of recent errors
     */
    std::vector<AudioError> getRecentErrors(int maxCount = 100, 
                                          AudioErrorSeverity minSeverity = AudioErrorSeverity::Info) const;
    
    /**
     * @brief Get comprehensive error statistics
     */
    struct AudioErrorStatistics {
        int totalErrors = 0;
        int criticalErrors = 0;
        int recoveredErrors = 0;
        int unrecoveredErrors = 0;
        int realTimeErrors = 0;
        float recoverySuccessRate = 0.0f;
        std::map<AudioErrorCode, int> errorCounts;
        std::chrono::system_clock::time_point lastError;
        std::chrono::system_clock::time_point lastCriticalError;
        
        // Audio-specific statistics
        int underrunCount = 0;
        int overrunCount = 0;
        int callbackTimeouts = 0;
        float averageCPULoad = 0.0f;
        std::chrono::microseconds averageLatency{0};
        std::chrono::microseconds maxJitter{0};
    };
    AudioErrorStatistics getStatistics() const;
    
    /**
     * @brief Clear error history
     */
    void clearHistory();
    
    // Callback management
    
    /**
     * @brief Set callback for error notifications
     * @param callback Function to call when errors occur
     */
    void setErrorCallback(ErrorCallback callback);
    
    /**
     * @brief Set callback for recovery notifications
     * @param callback Function to call when recovery actions complete
     */
    void setRecoveryCallback(RecoveryCallback callback);
    
    /**
     * @brief Set callback for critical error notifications
     * @param callback Function to call for critical errors
     */
    void setCriticalErrorCallback(CriticalErrorCallback callback);
    
    // Configuration
    
    /**
     * @brief Set maximum number of errors to keep in history
     * @param maxErrors Maximum error count
     */
    void setMaxErrorHistory(int maxErrors);
    
    /**
     * @brief Enable/disable automatic recovery
     * @param enabled Whether to attempt automatic recovery
     */
    void setAutoRecoveryEnabled(bool enabled);
    
    /**
     * @brief Set recovery timeout for real-time context
     * @param timeout Maximum time to spend on recovery in RT context
     */
    void setRealTimeRecoveryTimeout(std::chrono::microseconds timeout);
    
    /**
     * @brief Set recovery timeout for non-real-time context
     * @param timeout Maximum time to spend on recovery
     */
    void setRecoveryTimeout(std::chrono::milliseconds timeout);
    
    /**
     * @brief Set performance thresholds for automatic error detection
     */
    void setPerformanceThresholds(float maxCPULoad, 
                                 std::chrono::microseconds maxLatency,
                                 std::chrono::microseconds maxJitter);
    
    // Utility methods
    
    /**
     * @brief Convert error code to human-readable string
     * @param code Error code
     * @return String description
     */
    static std::string errorCodeToString(AudioErrorCode code);
    
    /**
     * @brief Convert severity to string
     * @param severity Error severity
     * @return String description
     */
    static std::string severityToString(AudioErrorSeverity severity);
    
    /**
     * @brief Create error with stream context
     */
    AudioError createStreamError(AudioErrorCode code,
                                const std::string& message,
                                const std::string& operation) const;
    
    /**
     * @brief Create error with performance context
     */
    AudioError createPerformanceError(AudioErrorCode code,
                                     const std::string& message,
                                     float currentCPU,
                                     std::chrono::microseconds currentLatency) const;

private:
    mutable std::mutex errorMutex_;
    std::vector<AudioError> errorHistory_;
    std::map<AudioErrorCode, std::vector<AudioRecoveryAction>> recoveryActions_;
    
    // Configuration
    int maxErrorHistory_ = 1000;
    bool autoRecoveryEnabled_ = true;
    std::chrono::milliseconds recoveryTimeout_{1000};
    std::chrono::microseconds realTimeRecoveryTimeout_{100};
    
    // Performance thresholds
    float maxCPULoad_ = 80.0f;
    std::chrono::microseconds maxLatency_{10000};
    std::chrono::microseconds maxJitter_{1000};
    
    // Stream context
    std::atomic<int> sampleRate_{44100};
    std::atomic<int> bufferSize_{512};
    std::atomic<int> channelCount_{2};
    std::atomic<double> streamTime_{0.0};
    
    // Current performance metrics
    std::atomic<float> currentCPULoad_{0.0f};
    std::atomic<float> currentMemoryUsage_{0.0f};
    std::atomic<std::chrono::microseconds::rep> currentLatency_{0};
    std::atomic<std::chrono::microseconds::rep> currentJitter_{0};
    
    // Callbacks
    ErrorCallback errorCallback_;
    RecoveryCallback recoveryCallback_;
    CriticalErrorCallback criticalErrorCallback_;
    
    // Statistics
    mutable AudioErrorStatistics stats_;
    
    // Lock-free queue for real-time error reporting
    struct RTError {
        AudioErrorCode code;
        std::string message;
        std::chrono::system_clock::time_point timestamp;
    };
    static constexpr size_t RT_ERROR_QUEUE_SIZE = 256;
    std::array<RTError, RT_ERROR_QUEUE_SIZE> rtErrorQueue_;
    std::atomic<size_t> rtErrorWriteIndex_{0};
    std::atomic<size_t> rtErrorReadIndex_{0};
    
    // Internal methods
    AudioRecoveryResult attemptRecovery(const AudioError& error, bool isRealTime);
    void updateStatistics(const AudioError& error, const AudioRecoveryResult& recovery);
    void addToHistory(const AudioError& error);
    void trimHistory();
    void processRealTimeErrors(); // Process queued RT errors
    
    // Default recovery actions
    void initializeDefaultRecoveryActions();
    
    // Device recovery actions
    AudioRecoveryAction createDeviceRecovery();
    AudioRecoveryAction createStreamRecovery();
    AudioRecoveryAction createBufferRecovery();
    
    // Performance recovery actions
    AudioRecoveryAction createCPULoadRecovery();
    AudioRecoveryAction createLatencyRecovery();
    AudioRecoveryAction createMemoryRecovery();
    
    // Safety recovery actions
    AudioRecoveryAction createEmergencyMute();
    AudioRecoveryAction createVolumeClamp();
    AudioRecoveryAction createGainReduction();
};

/**
 * @brief RAII wrapper for audio error context tracking
 */
class AudioErrorContext {
public:
    AudioErrorContext(AudioErrorHandler& handler, const std::string& operation)
        : handler_(handler), operation_(operation) {}
    
    ~AudioErrorContext() = default;
    
    void reportError(AudioErrorCode code, 
                    AudioErrorSeverity severity,
                    const std::string& message,
                    bool isRealTime = false,
                    const std::string& function = "",
                    int line = 0) {
        handler_.reportError(code, severity, message, operation_, function, line, isRealTime);
    }
    
private:
    AudioErrorHandler& handler_;
    std::string operation_;
};

/**
 * @brief Macros for convenient audio error reporting with automatic context
 */
#define AUDIO_ERROR_CONTEXT(handler, operation) \
    AudioErrorContext errorCtx(handler, operation)

#define REPORT_AUDIO_ERROR(ctx, code, severity, message) \
    ctx.reportError(code, severity, message, false, __FUNCTION__, __LINE__)

#define REPORT_AUDIO_RT_ERROR(ctx, code, severity, message) \
    ctx.reportError(code, severity, message, true, __FUNCTION__, __LINE__)

#define REPORT_AUDIO_CRITICAL(handler, code, message) \
    handler.reportCriticalError(AudioError(code, AudioErrorSeverity::Critical, message, "", __FUNCTION__, __LINE__))

} // namespace AIMusicHardware