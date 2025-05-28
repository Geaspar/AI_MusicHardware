#include "ui/presets/PresetErrorHandler.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <algorithm>
#include <sstream>

namespace AIMusicHardware {

PresetErrorHandler::PresetErrorHandler() {
    initializeDefaultRecoveryActions();
}

PresetErrorHandler::~PresetErrorHandler() = default;

RecoveryResult PresetErrorHandler::reportError(const PresetError& error) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    
    // Add to history
    addToHistory(error);
    
    // Notify callback
    if (errorCallback_) {
        errorCallback_(error);
    }
    
    RecoveryResult result;
    
    // Attempt recovery if enabled and error is recoverable
    if (autoRecoveryEnabled_ && error.isRecoverable) {
        result = attemptRecovery(error);
    }
    
    // Update statistics
    updateStatistics(error, result);
    
    // Handle critical errors
    if (error.severity == PresetErrorSeverity::Critical) {
        reportCriticalError(error);
    }
    
    return result;
}

RecoveryResult PresetErrorHandler::reportError(PresetErrorCode code, 
                                             PresetErrorSeverity severity,
                                             const std::string& message,
                                             const std::string& context,
                                             const std::string& function,
                                             int line) {
    PresetError error(code, severity, message, context, function, line);
    
    // Determine if error is recoverable based on code
    switch (code) {
        case PresetErrorCode::FileNotFound:
        case PresetErrorCode::FileAccessDenied:
        case PresetErrorCode::DiskSpaceFull:
        case PresetErrorCode::DatabaseLocked:
        case PresetErrorCode::NetworkTimeout:
        case PresetErrorCode::MLModelNotLoaded:
            error.isRecoverable = true;
            break;
        default:
            error.isRecoverable = false;
            break;
    }
    
    return reportError(error);
}

void PresetErrorHandler::reportCriticalError(const PresetError& error) {
    // Critical errors bypass normal flow and are logged immediately
    std::lock_guard<std::mutex> lock(errorMutex_);
    
    stats_.lastCriticalError = error.timestamp;
    stats_.criticalErrors++;
    
    // Could trigger emergency shutdown procedures here
    // For now, just ensure it's logged with highest priority
}

void PresetErrorHandler::registerRecoveryAction(PresetErrorCode errorCode, const RecoveryAction& action) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recoveryActions_[errorCode].push_back(action);
    
    // Sort by priority (highest first)
    std::sort(recoveryActions_[errorCode].begin(), recoveryActions_[errorCode].end(),
              [](const RecoveryAction& a, const RecoveryAction& b) {
                  return a.priority > b.priority;
              });
}

void PresetErrorHandler::removeRecoveryAction(PresetErrorCode errorCode) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recoveryActions_.erase(errorCode);
}

void PresetErrorHandler::clearRecoveryActions() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recoveryActions_.clear();
}

std::vector<PresetError> PresetErrorHandler::getRecentErrors(int maxCount, 
                                                           PresetErrorSeverity minSeverity) const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    
    std::vector<PresetError> filtered;
    for (const auto& error : errorHistory_) {
        if (static_cast<int>(error.severity) >= static_cast<int>(minSeverity)) {
            filtered.push_back(error);
        }
    }
    
    // Return most recent errors first
    std::reverse(filtered.begin(), filtered.end());
    
    if (filtered.size() > static_cast<size_t>(maxCount)) {
        filtered.resize(maxCount);
    }
    
    return filtered;
}

PresetErrorHandler::ErrorStatistics PresetErrorHandler::getStatistics() const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    return stats_;
}

void PresetErrorHandler::clearHistory() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    errorHistory_.clear();
    stats_ = ErrorStatistics{};
}

void PresetErrorHandler::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    errorCallback_ = callback;
}

void PresetErrorHandler::setRecoveryCallback(RecoveryCallback callback) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recoveryCallback_ = callback;
}

void PresetErrorHandler::setMaxErrorHistory(int maxErrors) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    maxErrorHistory_ = maxErrors;
    trimHistory();
}

void PresetErrorHandler::setAutoRecoveryEnabled(bool enabled) {
    autoRecoveryEnabled_ = enabled;
}

void PresetErrorHandler::setRecoveryTimeout(std::chrono::milliseconds timeout) {
    recoveryTimeout_ = timeout;
}

std::string PresetErrorHandler::errorCodeToString(PresetErrorCode code) {
    switch (code) {
        case PresetErrorCode::FileNotFound: return "File not found";
        case PresetErrorCode::FileAccessDenied: return "File access denied";
        case PresetErrorCode::FileCorrupted: return "File corrupted";
        case PresetErrorCode::DiskSpaceFull: return "Disk space full";
        case PresetErrorCode::InvalidPath: return "Invalid path";
        case PresetErrorCode::JsonParseError: return "JSON parse error";
        case PresetErrorCode::JsonMissingField: return "JSON missing field";
        case PresetErrorCode::JsonInvalidType: return "JSON invalid type";
        case PresetErrorCode::JsonStructureInvalid: return "JSON structure invalid";
        case PresetErrorCode::DatabaseCorrupted: return "Database corrupted";
        case PresetErrorCode::DatabaseLocked: return "Database locked";
        case PresetErrorCode::DatabaseOutOfMemory: return "Database out of memory";
        case PresetErrorCode::IndexCorrupted: return "Index corrupted";
        case PresetErrorCode::MLModelNotLoaded: return "ML model not loaded";
        case PresetErrorCode::MLAnalysisFailed: return "ML analysis failed";
        case PresetErrorCode::MLInvalidFeatures: return "ML invalid features";
        case PresetErrorCode::MLMemoryError: return "ML memory error";
        case PresetErrorCode::NetworkTimeout: return "Network timeout";
        case PresetErrorCode::NetworkConnectionFailed: return "Network connection failed";
        case PresetErrorCode::AuthenticationFailed: return "Authentication failed";
        case PresetErrorCode::ServerError: return "Server error";
        case PresetErrorCode::OutOfMemory: return "Out of memory";
        case PresetErrorCode::ResourceLeakDetected: return "Resource leak detected";
        case PresetErrorCode::ThreadPoolExhausted: return "Thread pool exhausted";
        case PresetErrorCode::HandleLimitExceeded: return "Handle limit exceeded";
        case PresetErrorCode::InvalidOperation: return "Invalid operation";
        case PresetErrorCode::StateCorruption: return "State corruption";
        case PresetErrorCode::ConcurrencyViolation: return "Concurrency violation";
        case PresetErrorCode::ValidationFailed: return "Validation failed";
        case PresetErrorCode::Unknown: return "Unknown error";
        default: return "Unrecognized error code";
    }
}

std::string PresetErrorHandler::severityToString(PresetErrorSeverity severity) {
    switch (severity) {
        case PresetErrorSeverity::Info: return "Info";
        case PresetErrorSeverity::Warning: return "Warning";
        case PresetErrorSeverity::Error: return "Error";
        case PresetErrorSeverity::Critical: return "Critical";
        default: return "Unknown";
    }
}

PresetError PresetErrorHandler::createFileError(PresetErrorCode code,
                                               const std::string& filePath,
                                               const std::string& operation,
                                               const std::string& systemError) {
    std::ostringstream message;
    message << "File operation failed: " << operation;
    if (!systemError.empty()) {
        message << " (" << systemError << ")";
    }
    
    PresetError error(code, PresetErrorSeverity::Error, message.str(), operation);
    error.filePath = filePath;
    
    // Add recovery suggestions based on error type
    switch (code) {
        case PresetErrorCode::FileNotFound:
            error.recoverySuggestions.push_back("Check if file path is correct");
            error.recoverySuggestions.push_back("Verify file hasn't been moved or deleted");
            error.recoverySuggestions.push_back("Try refreshing the preset database");
            error.isRecoverable = true;
            break;
        case PresetErrorCode::FileAccessDenied:
            error.recoverySuggestions.push_back("Check file permissions");
            error.recoverySuggestions.push_back("Ensure file is not locked by another application");
            error.recoverySuggestions.push_back("Run application with appropriate privileges");
            error.isRecoverable = true;
            break;
        case PresetErrorCode::DiskSpaceFull:
            error.recoverySuggestions.push_back("Free up disk space");
            error.recoverySuggestions.push_back("Move presets to different location");
            error.recoverySuggestions.push_back("Clean up temporary files");
            error.isRecoverable = true;
            break;
    }
    
    return error;
}

PresetError PresetErrorHandler::createJsonError(const std::string& filePath,
                                               const std::string& jsonError,
                                               const std::string& field) {
    std::ostringstream message;
    message << "JSON parsing failed";
    if (!field.empty()) {
        message << " for field '" << field << "'";
    }
    message << ": " << jsonError;
    
    PresetError error(PresetErrorCode::JsonParseError, PresetErrorSeverity::Error, 
                     message.str(), "JSON parsing");
    error.filePath = filePath;
    error.recoverySuggestions.push_back("Check JSON syntax in preset file");
    error.recoverySuggestions.push_back("Restore from backup if available");
    error.recoverySuggestions.push_back("Regenerate preset from default template");
    
    return error;
}

PresetError PresetErrorHandler::createMLError(PresetErrorCode code,
                                             const std::string& operation,
                                             const std::string& details) {
    std::ostringstream message;
    message << "ML operation failed: " << operation;
    if (!details.empty()) {
        message << " (" << details << ")";
    }
    
    PresetError error(code, PresetErrorSeverity::Warning, message.str(), "ML Analysis");
    
    switch (code) {
        case PresetErrorCode::MLModelNotLoaded:
            error.recoverySuggestions.push_back("Reload ML model");
            error.recoverySuggestions.push_back("Check model file integrity");
            error.recoverySuggestions.push_back("Use fallback analysis method");
            error.isRecoverable = true;
            break;
        case PresetErrorCode::MLAnalysisFailed:
            error.recoverySuggestions.push_back("Retry analysis with different parameters");
            error.recoverySuggestions.push_back("Use cached analysis if available");
            error.recoverySuggestions.push_back("Skip ML analysis and use basic categorization");
            error.isRecoverable = true;
            break;
    }
    
    return error;
}

RecoveryResult PresetErrorHandler::attemptRecovery(const PresetError& error) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    RecoveryResult result;
    result.successful = false;
    
    auto it = recoveryActions_.find(error.code);
    if (it == recoveryActions_.end()) {
        result.resultMessage = "No recovery actions available for error code";
        return result;
    }
    
    // Try each recovery action in priority order
    for (const auto& action : it->second) {
        int retries = 0;
        bool actionSucceeded = false;
        
        while (retries <= action.maxRetries && !actionSucceeded) {
            try {
                if (action.delay.count() > 0) {
                    std::this_thread::sleep_for(action.delay);
                }
                
                actionSucceeded = action.action();
                
                if (actionSucceeded) {
                    result.successful = true;
                    result.actionTaken = action.description;
                    result.retriesUsed = retries;
                    result.resultMessage = "Recovery successful";
                    
                    if (recoveryCallback_) {
                        recoveryCallback_(error, result);
                    }
                    
                    break;
                }
            } catch (const std::exception& e) {
                result.resultMessage = std::string("Recovery action failed: ") + e.what();
            }
            
            retries++;
        }
        
        if (actionSucceeded) {
            break;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.timeSpent = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (!result.successful) {
        result.resultMessage = "All recovery actions failed";
    }
    
    return result;
}

void PresetErrorHandler::updateStatistics(const PresetError& error, const RecoveryResult& recovery) {
    stats_.totalErrors++;
    stats_.errorCounts[error.code]++;
    stats_.lastError = error.timestamp;
    
    if (error.severity == PresetErrorSeverity::Critical) {
        stats_.criticalErrors++;
    }
    
    if (recovery.successful) {
        stats_.recoveredErrors++;
    } else if (error.isRecoverable) {
        stats_.unrecoveredErrors++;
    }
    
    // Calculate recovery success rate
    int totalRecoveryAttempts = stats_.recoveredErrors + stats_.unrecoveredErrors;
    if (totalRecoveryAttempts > 0) {
        stats_.recoverySuccessRate = static_cast<float>(stats_.recoveredErrors) / totalRecoveryAttempts * 100.0f;
    }
}

void PresetErrorHandler::addToHistory(const PresetError& error) {
    errorHistory_.push_back(error);
    trimHistory();
}

void PresetErrorHandler::trimHistory() {
    while (errorHistory_.size() > static_cast<size_t>(maxErrorHistory_)) {
        errorHistory_.erase(errorHistory_.begin());
    }
}

void PresetErrorHandler::initializeDefaultRecoveryActions() {
    // File system recovery actions
    registerRecoveryAction(PresetErrorCode::FileNotFound, createFileNotFoundRecovery());
    registerRecoveryAction(PresetErrorCode::FileAccessDenied, createFileAccessRecovery());
    registerRecoveryAction(PresetErrorCode::DiskSpaceFull, createDiskSpaceRecovery());
    
    // Database recovery actions
    registerRecoveryAction(PresetErrorCode::DatabaseCorrupted, createDatabaseRecovery());
    registerRecoveryAction(PresetErrorCode::IndexCorrupted, createIndexRecovery());
    
    // Memory recovery actions
    registerRecoveryAction(PresetErrorCode::OutOfMemory, createMemoryRecovery());
    registerRecoveryAction(PresetErrorCode::ResourceLeakDetected, createResourceLeakRecovery());
}

RecoveryAction PresetErrorHandler::createFileNotFoundRecovery() {
    RecoveryAction action;
    action.description = "Search for file in alternative locations";
    action.priority = 100;
    action.maxRetries = 1;
    action.action = []() -> bool {
        // Could implement logic to search for moved files
        // For now, just return false as this requires specific file context
        return false;
    };
    return action;
}

RecoveryAction PresetErrorHandler::createFileAccessRecovery() {
    RecoveryAction action;
    action.description = "Retry file access with different permissions";
    action.priority = 90;
    action.maxRetries = 3;
    action.delay = std::chrono::milliseconds(100);
    action.action = []() -> bool {
        // Could implement retry logic with backoff
        // For now, just simulate a retry
        return false;
    };
    return action;
}

RecoveryAction PresetErrorHandler::createDiskSpaceRecovery() {
    RecoveryAction action;
    action.description = "Clean up temporary files to free disk space";
    action.priority = 80;
    action.maxRetries = 1;
    action.action = []() -> bool {
        try {
            // Clean up temporary preset files
            std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "AIMusicHardware";
            if (std::filesystem::exists(tempDir)) {
                std::filesystem::remove_all(tempDir);
                return true;
            }
        } catch (...) {
            return false;
        }
        return false;
    };
    return action;
}

RecoveryAction PresetErrorHandler::createDatabaseRecovery() {
    RecoveryAction action;
    action.description = "Rebuild database from preset files";
    action.priority = 70;
    action.maxRetries = 1;
    action.action = []() -> bool {
        // This would trigger database rebuild
        // Implementation would depend on PresetDatabase class
        return false;
    };
    return action;
}

RecoveryAction PresetErrorHandler::createIndexRecovery() {
    RecoveryAction action;
    action.description = "Rebuild search indices";
    action.priority = 60;
    action.maxRetries = 2;
    action.action = []() -> bool {
        // This would trigger index rebuild
        return false;
    };
    return action;
}

RecoveryAction PresetErrorHandler::createMemoryRecovery() {
    RecoveryAction action;
    action.description = "Clear caches and force garbage collection";
    action.priority = 50;
    action.maxRetries = 1;
    action.action = []() -> bool {
        // Could implement cache clearing and memory optimization
        return false;
    };
    return action;
}

RecoveryAction PresetErrorHandler::createResourceLeakRecovery() {
    RecoveryAction action;
    action.description = "Close unused resources and reset resource counters";
    action.priority = 40;
    action.maxRetries = 1;
    action.action = []() -> bool {
        // Could implement resource cleanup
        return false;
    };
    return action;
}

} // namespace AIMusicHardware