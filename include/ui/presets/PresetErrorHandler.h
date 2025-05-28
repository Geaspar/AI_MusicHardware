#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <map>

namespace AIMusicHardware {

/**
 * @brief Error severity levels for preset operations
 */
enum class PresetErrorSeverity {
    Info,       // Informational messages
    Warning,    // Non-critical issues
    Error,      // Recoverable errors
    Critical    // System-threatening errors
};

/**
 * @brief Error codes for specific preset operation failures
 */
enum class PresetErrorCode {
    // File system errors
    FileNotFound = 1000,
    FileAccessDenied = 1001,
    FileCorrupted = 1002,
    DiskSpaceFull = 1003,
    InvalidPath = 1004,
    
    // JSON parsing errors
    JsonParseError = 2000,
    JsonMissingField = 2001,
    JsonInvalidType = 2002,
    JsonStructureInvalid = 2003,
    
    // Database errors
    DatabaseCorrupted = 3000,
    DatabaseLocked = 3001,
    DatabaseOutOfMemory = 3002,
    IndexCorrupted = 3003,
    
    // ML/AI errors
    MLModelNotLoaded = 4000,
    MLAnalysisFailed = 4001,
    MLInvalidFeatures = 4002,
    MLMemoryError = 4003,
    
    // Network/IoT errors
    NetworkTimeout = 5000,
    NetworkConnectionFailed = 5001,
    AuthenticationFailed = 5002,
    ServerError = 5003,
    
    // Memory/Resource errors
    OutOfMemory = 6000,
    ResourceLeakDetected = 6001,
    ThreadPoolExhausted = 6002,
    HandleLimitExceeded = 6003,
    
    // Logic errors
    InvalidOperation = 7000,
    StateCorruption = 7001,
    ConcurrencyViolation = 7002,
    ValidationFailed = 7003,
    
    // Unknown/Generic
    Unknown = 9999
};

/**
 * @brief Detailed error information with context
 */
struct PresetError {
    PresetErrorCode code;
    PresetErrorSeverity severity;
    std::string message;
    std::string context;           // What was being done when error occurred
    std::string filePath;          // Relevant file path if applicable
    std::string function;          // Function where error occurred
    int line = 0;                  // Line number where error occurred
    std::chrono::system_clock::time_point timestamp;
    
    // Recovery suggestions
    std::vector<std::string> recoverySuggestions;
    bool isRecoverable = false;
    
    // Additional metadata
    std::map<std::string, std::string> metadata;
    
    PresetError(PresetErrorCode code, 
                PresetErrorSeverity severity,
                const std::string& message,
                const std::string& context = "",
                const std::string& function = "",
                int line = 0)
        : code(code), severity(severity), message(message), context(context),
          function(function), line(line), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Recovery action for automatic error recovery
 */
struct RecoveryAction {
    std::string description;
    std::function<bool()> action;
    int priority = 0;              // Higher priority actions tried first
    int maxRetries = 3;
    std::chrono::milliseconds delay{0}; // Delay before retry
};

/**
 * @brief Error recovery result
 */
struct RecoveryResult {
    bool successful = false;
    std::string actionTaken;
    std::string resultMessage;
    int retriesUsed = 0;
    std::chrono::milliseconds timeSpent{0};
};

/**
 * @brief Comprehensive error handling and recovery system for preset management
 */
class PresetErrorHandler {
public:
    using ErrorCallback = std::function<void(const PresetError&)>;
    using RecoveryCallback = std::function<void(const PresetError&, const RecoveryResult&)>;
    
    /**
     * @brief Constructor
     */
    PresetErrorHandler();
    
    /**
     * @brief Destructor
     */
    ~PresetErrorHandler();
    
    // Error reporting and logging
    
    /**
     * @brief Report an error with automatic recovery attempt
     * @param error Error details
     * @return Recovery result
     */
    RecoveryResult reportError(const PresetError& error);
    
    /**
     * @brief Report error with simple parameters
     */
    RecoveryResult reportError(PresetErrorCode code, 
                              PresetErrorSeverity severity,
                              const std::string& message,
                              const std::string& context = "",
                              const std::string& function = "",
                              int line = 0);
    
    /**
     * @brief Report critical error that requires immediate attention
     */
    void reportCriticalError(const PresetError& error);
    
    // Recovery system management
    
    /**
     * @brief Register recovery action for specific error code
     * @param errorCode Error code to handle
     * @param action Recovery action
     */
    void registerRecoveryAction(PresetErrorCode errorCode, const RecoveryAction& action);
    
    /**
     * @brief Remove recovery action for error code
     * @param errorCode Error code
     */
    void removeRecoveryAction(PresetErrorCode errorCode);
    
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
    std::vector<PresetError> getRecentErrors(int maxCount = 100, 
                                           PresetErrorSeverity minSeverity = PresetErrorSeverity::Info) const;
    
    /**
     * @brief Get error statistics
     */
    struct ErrorStatistics {
        int totalErrors = 0;
        int criticalErrors = 0;
        int recoveredErrors = 0;
        int unrecoveredErrors = 0;
        float recoverySuccessRate = 0.0f;
        std::map<PresetErrorCode, int> errorCounts;
        std::chrono::system_clock::time_point lastError;
        std::chrono::system_clock::time_point lastCriticalError;
    };
    ErrorStatistics getStatistics() const;
    
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
     * @brief Set recovery timeout
     * @param timeout Maximum time to spend on recovery
     */
    void setRecoveryTimeout(std::chrono::milliseconds timeout);
    
    // Utility methods
    
    /**
     * @brief Convert error code to human-readable string
     * @param code Error code
     * @return String description
     */
    static std::string errorCodeToString(PresetErrorCode code);
    
    /**
     * @brief Convert severity to string
     * @param severity Error severity
     * @return String description
     */
    static std::string severityToString(PresetErrorSeverity severity);
    
    /**
     * @brief Create error with file context
     */
    static PresetError createFileError(PresetErrorCode code,
                                     const std::string& filePath,
                                     const std::string& operation,
                                     const std::string& systemError = "");
    
    /**
     * @brief Create error with JSON context
     */
    static PresetError createJsonError(const std::string& filePath,
                                     const std::string& jsonError,
                                     const std::string& field = "");
    
    /**
     * @brief Create error with ML context
     */
    static PresetError createMLError(PresetErrorCode code,
                                   const std::string& operation,
                                   const std::string& details);

private:
    mutable std::mutex errorMutex_;
    std::vector<PresetError> errorHistory_;
    std::map<PresetErrorCode, std::vector<RecoveryAction>> recoveryActions_;
    
    // Configuration
    int maxErrorHistory_ = 1000;
    bool autoRecoveryEnabled_ = true;
    std::chrono::milliseconds recoveryTimeout_{5000};
    
    // Callbacks
    ErrorCallback errorCallback_;
    RecoveryCallback recoveryCallback_;
    
    // Statistics
    mutable ErrorStatistics stats_;
    
    // Internal methods
    RecoveryResult attemptRecovery(const PresetError& error);
    void updateStatistics(const PresetError& error, const RecoveryResult& recovery);
    void addToHistory(const PresetError& error);
    void trimHistory();
    
    // Default recovery actions
    void initializeDefaultRecoveryActions();
    
    // File system recovery actions
    RecoveryAction createFileNotFoundRecovery();
    RecoveryAction createFileAccessRecovery();
    RecoveryAction createDiskSpaceRecovery();
    
    // Database recovery actions
    RecoveryAction createDatabaseRecovery();
    RecoveryAction createIndexRecovery();
    
    // Memory recovery actions
    RecoveryAction createMemoryRecovery();
    RecoveryAction createResourceLeakRecovery();
};

/**
 * @brief RAII wrapper for error context tracking
 */
class ErrorContext {
public:
    ErrorContext(PresetErrorHandler& handler, const std::string& operation)
        : handler_(handler), operation_(operation) {}
    
    ~ErrorContext() = default;
    
    void reportError(PresetErrorCode code, 
                    PresetErrorSeverity severity,
                    const std::string& message,
                    const std::string& function = "",
                    int line = 0) {
        handler_.reportError(code, severity, message, operation_, function, line);
    }
    
private:
    PresetErrorHandler& handler_;
    std::string operation_;
};

/**
 * @brief Macros for convenient error reporting with automatic context
 */
#define PRESET_ERROR_CONTEXT(handler, operation) \
    ErrorContext errorCtx(handler, operation)

#define REPORT_PRESET_ERROR(ctx, code, severity, message) \
    ctx.reportError(code, severity, message, __FUNCTION__, __LINE__)

#define REPORT_PRESET_CRITICAL(handler, code, message) \
    handler.reportCriticalError(PresetError(code, PresetErrorSeverity::Critical, message, "", __FUNCTION__, __LINE__))

} // namespace AIMusicHardware