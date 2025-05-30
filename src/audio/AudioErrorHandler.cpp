#include "../../include/audio/AudioErrorHandler.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <sstream>

namespace AIMusicHardware {

AudioErrorHandler::AudioErrorHandler() {
    initializeDefaultRecoveryActions();
}

AudioErrorHandler::~AudioErrorHandler() {
    // Process any remaining real-time errors
    processRealTimeErrors();
}

AudioRecoveryResult AudioErrorHandler::reportError(const AudioError& error, bool isRealTime) {
    // For real-time contexts, use lock-free reporting if possible
    if (isRealTime && error.severity != AudioErrorSeverity::Critical) {
        reportRealTimeError(error.code, error.message);
        return AudioRecoveryResult{false, "Queued for processing", "Real-time error queued", 0, 
                                  std::chrono::microseconds{0}, true};
    }
    
    AUDIO_ERROR_CONTEXT(*this, "reportError");
    
    // Add to history
    addToHistory(error);
    
    // Update statistics
    AudioRecoveryResult result;
    updateStatistics(error, result);
    
    // Notify error callback
    if (errorCallback_) {
        try {
            errorCallback_(error);
        } catch (...) {
            // Don't let callback errors crash the system
        }
    }
    
    // Attempt recovery if enabled and not in real-time context
    if (autoRecoveryEnabled_ && !isRealTime) {
        result = attemptRecovery(error, isRealTime);
    }
    
    // Handle critical errors
    if (error.severity == AudioErrorSeverity::Critical) {
        reportCriticalError(error);
    }
    
    return result;
}

AudioRecoveryResult AudioErrorHandler::reportError(AudioErrorCode code, 
                                                  AudioErrorSeverity severity,
                                                  const std::string& message,
                                                  const std::string& context,
                                                  const std::string& function,
                                                  int line,
                                                  bool isRealTime) {
    AudioError error(code, severity, message, context, function, line);
    
    // Add current stream context
    error.sampleRate = sampleRate_.load();
    error.bufferSize = bufferSize_.load();
    error.channelCount = channelCount_.load();
    error.streamTime = streamTime_.load();
    error.cpuLoad = currentCPULoad_.load();
    error.memoryUsage = currentMemoryUsage_.load();
    error.latency = std::chrono::microseconds{currentLatency_.load()};
    error.jitter = std::chrono::microseconds{currentJitter_.load()};
    
    return reportError(error, isRealTime);
}

void AudioErrorHandler::reportCriticalError(const AudioError& error) {
    // Critical errors always get immediate attention
    std::lock_guard<std::mutex> lock(errorMutex_);
    
    stats_.criticalErrors++;
    stats_.lastCriticalError = std::chrono::system_clock::now();
    
    // Log critical error
    std::cerr << "CRITICAL AUDIO ERROR [" << errorCodeToString(error.code) << "]: " 
              << error.message << std::endl;
    
    if (criticalErrorCallback_) {
        try {
            criticalErrorCallback_(error);
        } catch (...) {
            std::cerr << "Critical error callback threw exception!" << std::endl;
        }
    }
    
    // For certain critical errors, take immediate safety actions
    switch (error.code) {
        case AudioErrorCode::CallbackException:
        case AudioErrorCode::CallbackMemoryViolation:
        case AudioErrorCode::AudioSafetyViolation:
            // Emergency mute to prevent damage
            // This would be implemented by the audio engine
            std::cerr << "Emergency audio safety measures activated!" << std::endl;
            break;
        default:
            break;
    }
}

void AudioErrorHandler::reportRealTimeError(AudioErrorCode code, const std::string& message) {
    // Lock-free error reporting for real-time contexts
    size_t writeIndex = rtErrorWriteIndex_.load();
    size_t nextIndex = (writeIndex + 1) % RT_ERROR_QUEUE_SIZE;
    
    // Check if queue is full
    if (nextIndex == rtErrorReadIndex_.load()) {
        // Queue full, drop oldest error (or we could drop this one)
        rtErrorReadIndex_.store((rtErrorReadIndex_.load() + 1) % RT_ERROR_QUEUE_SIZE);
    }
    
    // Store error
    rtErrorQueue_[writeIndex] = {code, message, std::chrono::system_clock::now()};
    rtErrorWriteIndex_.store(nextIndex);
}

void AudioErrorHandler::processRealTimeErrors() {
    // Process queued real-time errors in non-RT context
    size_t readIndex = rtErrorReadIndex_.load();
    
    while (readIndex != rtErrorWriteIndex_.load()) {
        const auto& rtError = rtErrorQueue_[readIndex];
        
        // Create full error and process normally
        AudioError error(rtError.code, AudioErrorSeverity::Warning, rtError.message, "Real-time callback");
        error.timestamp = rtError.timestamp;
        
        addToHistory(error);
        
        readIndex = (readIndex + 1) % RT_ERROR_QUEUE_SIZE;
        rtErrorReadIndex_.store(readIndex);
    }
}

void AudioErrorHandler::updatePerformanceMetrics(float cpuLoad, float memoryUsage, 
                                                 std::chrono::microseconds latency,
                                                 std::chrono::microseconds jitter) {
    currentCPULoad_.store(cpuLoad);
    currentMemoryUsage_.store(memoryUsage);
    currentLatency_.store(latency.count());
    currentJitter_.store(jitter.count());
    
    // Check thresholds and report errors if exceeded
    if (cpuLoad > maxCPULoad_) {
        reportRealTimeError(AudioErrorCode::CPUOverload, 
                           "CPU load exceeded threshold: " + std::to_string(cpuLoad) + "%");
    }
    
    if (latency > maxLatency_) {
        reportRealTimeError(AudioErrorCode::LatencyBudgetExceeded,
                           "Latency exceeded threshold: " + std::to_string(latency.count()) + "μs");
    }
    
    if (jitter > maxJitter_) {
        reportRealTimeError(AudioErrorCode::JitterTooHigh,
                           "Jitter exceeded threshold: " + std::to_string(jitter.count()) + "μs");
    }
}

void AudioErrorHandler::setStreamContext(int sampleRate, int bufferSize, int channelCount) {
    sampleRate_.store(sampleRate);
    bufferSize_.store(bufferSize);
    channelCount_.store(channelCount);
}

void AudioErrorHandler::updateStreamTime(double streamTime) {
    streamTime_.store(streamTime);
}

void AudioErrorHandler::registerRecoveryAction(AudioErrorCode errorCode, const AudioRecoveryAction& action) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recoveryActions_[errorCode].push_back(action);
    
    // Sort by priority (higher priority first)
    std::sort(recoveryActions_[errorCode].begin(), recoveryActions_[errorCode].end(),
             [](const AudioRecoveryAction& a, const AudioRecoveryAction& b) {
                 return a.priority > b.priority;
             });
}

void AudioErrorHandler::removeRecoveryAction(AudioErrorCode errorCode) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recoveryActions_.erase(errorCode);
}

void AudioErrorHandler::clearRecoveryActions() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recoveryActions_.clear();
    initializeDefaultRecoveryActions();
}

std::vector<AudioError> AudioErrorHandler::getRecentErrors(int maxCount, 
                                                          AudioErrorSeverity minSeverity) const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    std::vector<AudioError> result;
    
    for (auto it = errorHistory_.rbegin(); it != errorHistory_.rend() && result.size() < maxCount; ++it) {
        if (it->severity >= minSeverity) {
            result.push_back(*it);
        }
    }
    
    return result;
}

AudioErrorHandler::AudioErrorStatistics AudioErrorHandler::getStatistics() const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    
    // Update derived statistics
    stats_.recoverySuccessRate = stats_.totalErrors > 0 ? 
        static_cast<float>(stats_.recoveredErrors) / stats_.totalErrors * 100.0f : 0.0f;
    
    return stats_;
}

void AudioErrorHandler::clearHistory() {
    std::lock_guard<std::mutex> lock(errorMutex_);
    errorHistory_.clear();
    stats_ = AudioErrorStatistics{};
}

void AudioErrorHandler::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    errorCallback_ = callback;
}

void AudioErrorHandler::setRecoveryCallback(RecoveryCallback callback) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    recoveryCallback_ = callback;
}

void AudioErrorHandler::setCriticalErrorCallback(CriticalErrorCallback callback) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    criticalErrorCallback_ = callback;
}

void AudioErrorHandler::setMaxErrorHistory(int maxErrors) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    maxErrorHistory_ = maxErrors;
    trimHistory();
}

void AudioErrorHandler::setAutoRecoveryEnabled(bool enabled) {
    autoRecoveryEnabled_ = enabled;
}

void AudioErrorHandler::setRealTimeRecoveryTimeout(std::chrono::microseconds timeout) {
    realTimeRecoveryTimeout_ = timeout;
}

void AudioErrorHandler::setRecoveryTimeout(std::chrono::milliseconds timeout) {
    recoveryTimeout_ = timeout;
}

void AudioErrorHandler::setPerformanceThresholds(float maxCPULoad, 
                                                 std::chrono::microseconds maxLatency,
                                                 std::chrono::microseconds maxJitter) {
    maxCPULoad_ = maxCPULoad;
    maxLatency_ = maxLatency;
    maxJitter_ = maxJitter;
}

std::string AudioErrorHandler::errorCodeToString(AudioErrorCode code) {
    switch (code) {
        case AudioErrorCode::DeviceNotFound: return "Device Not Found";
        case AudioErrorCode::DeviceDisconnected: return "Device Disconnected";
        case AudioErrorCode::DeviceConfigurationFailed: return "Device Configuration Failed";
        case AudioErrorCode::StreamOpenFailed: return "Stream Open Failed";
        case AudioErrorCode::StreamUnderrun: return "Stream Underrun";
        case AudioErrorCode::StreamOverrun: return "Stream Overrun";
        case AudioErrorCode::CallbackTimeout: return "Callback Timeout";
        case AudioErrorCode::CallbackException: return "Callback Exception";
        case AudioErrorCode::CPUOverload: return "CPU Overload";
        case AudioErrorCode::OutOfMemory: return "Out of Memory";
        case AudioErrorCode::AudioClipping: return "Audio Clipping";
        case AudioErrorCode::EmergencyMute: return "Emergency Mute";
        default: return "Unknown Audio Error";
    }
}

std::string AudioErrorHandler::severityToString(AudioErrorSeverity severity) {
    switch (severity) {
        case AudioErrorSeverity::Info: return "Info";
        case AudioErrorSeverity::Warning: return "Warning";
        case AudioErrorSeverity::Error: return "Error";
        case AudioErrorSeverity::Critical: return "Critical";
        default: return "Unknown";
    }
}

AudioError AudioErrorHandler::createStreamError(AudioErrorCode code,
                                               const std::string& message,
                                               const std::string& operation) const {
    AudioError error(code, AudioErrorSeverity::Error, message, operation);
    error.sampleRate = sampleRate_.load();
    error.bufferSize = bufferSize_.load();
    error.channelCount = channelCount_.load();
    error.streamTime = streamTime_.load();
    return error;
}

AudioError AudioErrorHandler::createPerformanceError(AudioErrorCode code,
                                                     const std::string& message,
                                                     float currentCPU,
                                                     std::chrono::microseconds currentLatency) const {
    AudioError error(code, AudioErrorSeverity::Warning, message, "Performance Monitoring");
    error.cpuLoad = currentCPU;
    error.latency = currentLatency;
    error.sampleRate = sampleRate_.load();
    error.bufferSize = bufferSize_.load();
    return error;
}

AudioRecoveryResult AudioErrorHandler::attemptRecovery(const AudioError& error, bool isRealTime) {
    auto start = std::chrono::steady_clock::now();
    AudioRecoveryResult result;
    result.wasRealTime = isRealTime;
    
    auto it = recoveryActions_.find(error.code);
    if (it == recoveryActions_.end()) {
        result.resultMessage = "No recovery actions available";
        return result;
    }
    
    // Try recovery actions in priority order
    for (const auto& action : it->second) {
        // Skip actions not suitable for real-time context
        if (isRealTime && !action.allowInRealTime) {
            continue;
        }
        
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto timeout = isRealTime ? realTimeRecoveryTimeout_ : 
                      std::chrono::duration_cast<std::chrono::microseconds>(recoveryTimeout_);
        
        if (elapsed > timeout) {
            result.resultMessage = "Recovery timeout exceeded";
            break;
        }
        
        // Attempt recovery
        for (int retry = 0; retry <= action.maxRetries; ++retry) {
            result.retriesUsed = retry + 1;
            
            try {
                if (action.action()) {
                    result.successful = true;
                    result.actionTaken = action.description;
                    result.resultMessage = "Recovery successful";
                    
                    auto end = std::chrono::steady_clock::now();
                    result.timeSpent = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    
                    if (recoveryCallback_) {
                        recoveryCallback_(error, result);
                    }
                    
                    return result;
                }
            } catch (...) {
                // Recovery action failed, try next retry or next action
            }
            
            // Delay before retry (but not in real-time context)
            if (!isRealTime && retry < action.maxRetries && action.maxDelay.count() > 0) {
                std::this_thread::sleep_for(action.maxDelay);
            }
        }
    }
    
    result.resultMessage = "All recovery attempts failed";
    auto end = std::chrono::steady_clock::now();
    result.timeSpent = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return result;
}

void AudioErrorHandler::updateStatistics(const AudioError& error, const AudioRecoveryResult& recovery) {
    stats_.totalErrors++;
    stats_.lastError = error.timestamp;
    
    if (recovery.wasRealTime) {
        stats_.realTimeErrors++;
    }
    
    if (recovery.successful) {
        stats_.recoveredErrors++;
    } else {
        stats_.unrecoveredErrors++;
    }
    
    stats_.errorCounts[error.code]++;
    
    // Update audio-specific statistics
    switch (error.code) {
        case AudioErrorCode::StreamUnderrun:
            stats_.underrunCount++;
            break;
        case AudioErrorCode::StreamOverrun:
            stats_.overrunCount++;
            break;
        case AudioErrorCode::CallbackTimeout:
            stats_.callbackTimeouts++;
            break;
        default:
            break;
    }
    
    // Update performance averages
    if (error.cpuLoad > 0) {
        stats_.averageCPULoad = (stats_.averageCPULoad * (stats_.totalErrors - 1) + error.cpuLoad) / stats_.totalErrors;
    }
    
    if (error.latency.count() > 0) {
        auto totalLatency = stats_.averageLatency.count() * (stats_.totalErrors - 1) + error.latency.count();
        stats_.averageLatency = std::chrono::microseconds{totalLatency / stats_.totalErrors};
    }
    
    if (error.jitter > stats_.maxJitter) {
        stats_.maxJitter = error.jitter;
    }
}

void AudioErrorHandler::addToHistory(const AudioError& error) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    errorHistory_.push_back(error);
    trimHistory();
}

void AudioErrorHandler::trimHistory() {
    if (errorHistory_.size() > maxErrorHistory_) {
        errorHistory_.erase(errorHistory_.begin(), 
                           errorHistory_.begin() + (errorHistory_.size() - maxErrorHistory_));
    }
}

void AudioErrorHandler::initializeDefaultRecoveryActions() {
    // Device recovery actions
    registerRecoveryAction(AudioErrorCode::DeviceDisconnected, createDeviceRecovery());
    registerRecoveryAction(AudioErrorCode::StreamOpenFailed, createStreamRecovery());
    registerRecoveryAction(AudioErrorCode::StreamUnderrun, createBufferRecovery());
    
    // Performance recovery actions
    registerRecoveryAction(AudioErrorCode::CPUOverload, createCPULoadRecovery());
    registerRecoveryAction(AudioErrorCode::LatencyBudgetExceeded, createLatencyRecovery());
    registerRecoveryAction(AudioErrorCode::OutOfMemory, createMemoryRecovery());
    
    // Safety recovery actions
    registerRecoveryAction(AudioErrorCode::AudioClipping, createVolumeClamp());
    registerRecoveryAction(AudioErrorCode::AudioSafetyViolation, createEmergencyMute());
}

AudioRecoveryAction AudioErrorHandler::createDeviceRecovery() {
    AudioRecoveryAction action;
    action.description = "Attempt device reconnection";
    action.priority = 100;
    action.maxRetries = 3;
    action.allowInRealTime = false;
    action.requiresAudioStop = true;
    action.action = []() {
        // Device recovery logic would go here
        std::cout << "Attempting device recovery..." << std::endl;
        return false; // Placeholder
    };
    return action;
}

AudioRecoveryAction AudioErrorHandler::createStreamRecovery() {
    AudioRecoveryAction action;
    action.description = "Restart audio stream";
    action.priority = 90;
    action.maxRetries = 2;
    action.allowInRealTime = false;
    action.requiresAudioStop = true;
    action.action = []() {
        // Stream recovery logic would go here
        std::cout << "Attempting stream recovery..." << std::endl;
        return false; // Placeholder
    };
    return action;
}

AudioRecoveryAction AudioErrorHandler::createBufferRecovery() {
    AudioRecoveryAction action;
    action.description = "Adjust buffer size";
    action.priority = 80;
    action.maxRetries = 1;
    action.allowInRealTime = false;
    action.requiresAudioStop = true;
    action.action = []() {
        // Buffer size adjustment logic would go here
        std::cout << "Attempting buffer size adjustment..." << std::endl;
        return false; // Placeholder
    };
    return action;
}

AudioRecoveryAction AudioErrorHandler::createCPULoadRecovery() {
    AudioRecoveryAction action;
    action.description = "Reduce CPU load";
    action.priority = 70;
    action.maxRetries = 1;
    action.allowInRealTime = true;
    action.action = []() {
        // CPU load reduction logic would go here
        std::cout << "Attempting CPU load reduction..." << std::endl;
        return false; // Placeholder
    };
    return action;
}

AudioRecoveryAction AudioErrorHandler::createLatencyRecovery() {
    AudioRecoveryAction action;
    action.description = "Optimize latency";
    action.priority = 60;
    action.maxRetries = 1;
    action.allowInRealTime = false;
    action.action = []() {
        // Latency optimization logic would go here
        std::cout << "Attempting latency optimization..." << std::endl;
        return false; // Placeholder
    };
    return action;
}

AudioRecoveryAction AudioErrorHandler::createMemoryRecovery() {
    AudioRecoveryAction action;
    action.description = "Free memory";
    action.priority = 50;
    action.maxRetries = 1;
    action.allowInRealTime = false;
    action.action = []() {
        // Memory cleanup logic would go here
        std::cout << "Attempting memory cleanup..." << std::endl;
        return false; // Placeholder
    };
    return action;
}

AudioRecoveryAction AudioErrorHandler::createEmergencyMute() {
    AudioRecoveryAction action;
    action.description = "Emergency mute";
    action.priority = 999; // Highest priority
    action.maxRetries = 0;
    action.allowInRealTime = true;
    action.action = []() {
        // Emergency mute logic would go here
        std::cout << "Emergency mute activated!" << std::endl;
        return true; // This should always succeed
    };
    return action;
}

AudioRecoveryAction AudioErrorHandler::createVolumeClamp() {
    AudioRecoveryAction action;
    action.description = "Clamp volume levels";
    action.priority = 95;
    action.maxRetries = 0;
    action.allowInRealTime = true;
    action.action = []() {
        // Volume clamping logic would go here
        std::cout << "Volume clamping activated!" << std::endl;
        return true;
    };
    return action;
}

AudioRecoveryAction AudioErrorHandler::createGainReduction() {
    AudioRecoveryAction action;
    action.description = "Reduce gain levels";
    action.priority = 85;
    action.maxRetries = 0;
    action.allowInRealTime = true;
    action.action = []() {
        // Gain reduction logic would go here
        std::cout << "Gain reduction activated!" << std::endl;
        return true;
    };
    return action;
}

} // namespace AIMusicHardware