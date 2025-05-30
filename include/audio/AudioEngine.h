#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include "AudioErrorHandler.h"

namespace AIMusicHardware {

class AudioEngine {
public:
    AudioEngine(int sampleRate = 44100, int bufferSize = 512);
    ~AudioEngine();
    
    bool initialize();
    void shutdown();
    
    // Audio callback interface
    using AudioCallback = std::function<void(float* outputBuffer, int numFrames)>;
    void setAudioCallback(AudioCallback callback);
    
    // Accessor for callback (used by audio callback function) - thread-safe
    AudioCallback getCallback() const;
    
    // Channel information
    int getNumChannels() const { return numChannels_; }
    
    // Accessor methods
    int getSampleRate() const { return sampleRate_; }
    int getBufferSize() const { return bufferSize_; }

    // Get current audio stream time in seconds (since stream started)
    double getStreamTime() const;

    // Synchronize a sequencer with the audio engine's timing
    void synchronizeSequencer(std::shared_ptr<class Sequencer> sequencer);
    
    // Enterprise-grade error handling and monitoring
    
    /**
     * @brief Get the audio error handler for monitoring and recovery
     * @return Reference to the error handler
     */
    AudioErrorHandler& getErrorHandler() { return errorHandler_; }
    
    /**
     * @brief Get performance statistics
     */
    struct PerformanceMetrics {
        float cpuLoad = 0.0f;
        float memoryUsage = 0.0f;
        std::chrono::microseconds latency{0};
        std::chrono::microseconds jitter{0};
        int underrunCount = 0;
        int overrunCount = 0;
        double uptime = 0.0;
        bool isHealthy = true;
    };
    PerformanceMetrics getPerformanceMetrics() const;
    
    /**
     * @brief Enable/disable performance monitoring
     * @param enabled Whether to enable monitoring
     */
    void setPerformanceMonitoringEnabled(bool enabled);
    
    /**
     * @brief Set performance thresholds for automatic error detection
     * @param maxCPULoad Maximum CPU load percentage
     * @param maxLatency Maximum acceptable latency
     * @param maxJitter Maximum acceptable jitter
     */
    void setPerformanceThresholds(float maxCPULoad, 
                                 std::chrono::microseconds maxLatency,
                                 std::chrono::microseconds maxJitter);
    
    /**
     * @brief Enable/disable audio safety mechanisms
     * @param enabled Whether to enable safety features
     */
    void setAudioSafetyEnabled(bool enabled);
    
    /**
     * @brief Get current audio engine health status
     * @return true if engine is operating within normal parameters
     */
    bool isHealthy() const;
    
    /**
     * @brief Get error statistics from the error handler
     */
    AudioErrorHandler::AudioErrorStatistics getErrorStatistics() const;
    
private:
    int sampleRate_;
    int bufferSize_;
    int numChannels_ = 2;  // Default to stereo
    std::atomic<bool> isInitialized_{false};
    
    // Thread-safe callback handling
    mutable std::mutex callbackMutex_;
    AudioCallback callback_;
    
    // Enterprise-grade error handling and monitoring
    AudioErrorHandler errorHandler_;
    
    // Performance monitoring
    mutable std::mutex performanceMutex_;
    std::atomic<bool> performanceMonitoringEnabled_{true};
    std::atomic<bool> audioSafetyEnabled_{true};
    std::chrono::steady_clock::time_point startTime_;
    
    // Performance metrics (atomic for thread safety)
    std::atomic<float> currentCPULoad_{0.0f};
    std::atomic<float> currentMemoryUsage_{0.0f};
    std::atomic<std::chrono::microseconds::rep> currentLatency_{0};
    std::atomic<std::chrono::microseconds::rep> currentJitter_{0};
    std::atomic<int> underrunCount_{0};
    std::atomic<int> overrunCount_{0};
    std::atomic<bool> isHealthy_{true};
    
    // Performance measurement
    std::chrono::steady_clock::time_point lastCallbackTime_;
    std::chrono::microseconds lastCallbackDuration_{0};
    float cpuLoadSmoothingFactor_ = 0.95f; // For exponential smoothing
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Internal methods for performance monitoring
    void updatePerformanceMetrics();
    void measureCallbackPerformance(const std::chrono::steady_clock::time_point& start,
                                   const std::chrono::steady_clock::time_point& end);
    void checkAudioSafety(float* outputBuffer, int numFrames);
    
    // Friend function for callback access to private methods
    friend int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nFrames,
                            double streamTime, unsigned int status, void* userData);
};

} // namespace AIMusicHardware