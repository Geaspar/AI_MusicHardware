#include "../../include/audio/AudioEngine.h"
#include "../../include/sequencer/Sequencer.h" // Include Sequencer.h early to avoid forward declaration issues
#include "../../include/audio/AudioErrorHandler.h"

// RtAudio header can be in different locations depending on installation method
// Try standard includes first, then fallback to rtaudio subdirectory
#if defined(HAVE_RTAUDIO)
  #include <RtAudio.h>
#else
  // Try alternate include paths for RtAudio
  #if __has_include(<RtAudio.h>)
    #include <RtAudio.h>
    #define HAVE_RTAUDIO 1
  #elif __has_include(<rtaudio/RtAudio.h>)
    #include <rtaudio/RtAudio.h>
    #define HAVE_RTAUDIO 1
  #elif __has_include("RtAudio.h")
    #include "RtAudio.h"
    #define HAVE_RTAUDIO 1
  #else
    // Dummy implementation for when RtAudio is not available
    #warning "RtAudio not found. Using dummy implementation."
    class RtAudio {
    public:
      struct StreamParameters { int deviceId; int nChannels; int firstChannel; };
      struct StreamOptions { unsigned int flags; int numberOfBuffers; int priority; };
      struct DeviceInfo { bool probed; std::string name; int outputChannels; int inputChannels; int duplexChannels; bool isDefaultOutput; bool isDefaultInput; };
      
      RtAudio() {}
      ~RtAudio() {}
      unsigned int getDeviceCount() { return 0; }
      unsigned int getDefaultOutputDevice() { return 0; }
      DeviceInfo getDeviceInfo(int id) { return DeviceInfo(); }
      void openStream(StreamParameters* output, void* input, int format, int sampleRate, unsigned int* bufferFrames, void* callback, void* userData, StreamOptions* options = nullptr) {}
      void startStream() {}
      void stopStream() {}
      void closeStream() {}
      void abortStream() {} // Added missing function
      bool isStreamOpen() { return false; }
      bool isStreamRunning() { return false; }
      double getStreamTime() { return 0.0; } // Added missing function
      long getStreamLatency() { return 0; } // Added missing function
      int getStreamSampleRate() { return 0; } // Added missing function
      void setStreamTime(double time) {} // Added missing function
    };
    #define RTAUDIO_FLOAT32 0
    #define RTAUDIO_SCHEDULE_REALTIME 0
  #endif
#endif

// Include standard status flags - these are now defined as global constants in newer RtAudio
#ifndef RTAUDIO_INPUT_OVERFLOW
  #define RTAUDIO_INPUT_OVERFLOW 0x1
#endif
#ifndef RTAUDIO_OUTPUT_UNDERFLOW
  #define RTAUDIO_OUTPUT_UNDERFLOW 0x2
#endif

#include <iostream>
#include <cstring>
#include <stdexcept>

namespace AIMusicHardware {

// Forward declare for access to AudioEngine::callback_
class AudioEngineAccess;

// Define RtAudioStreamStatus type for callback
using RtAudioStreamStatus = unsigned int;

// Enhanced RtAudio callback function with enterprise-grade error handling
int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nFrames,
                 double streamTime, RtAudioStreamStatus status, void* userData) {
    
    auto callbackStart = std::chrono::steady_clock::now();
    
    // Cast user data to AudioEngine instance
    AudioEngine* engine = static_cast<AudioEngine*>(userData);
    if (!engine) {
        // Critical error - null engine pointer
        return 1;  // Return error code
    }
    
    // Update stream time in error handler
    engine->getErrorHandler().updateStreamTime(streamTime);
    
    // Check for stream underflow/overflow with enterprise error handling
    if (status) {
        if (status & RTAUDIO_INPUT_OVERFLOW) {
            engine->overrunCount_.fetch_add(1);
            engine->getErrorHandler().reportRealTimeError(
                AudioErrorCode::StreamOverrun, 
                "Input buffer overflow detected"
            );
        }
        if (status & RTAUDIO_OUTPUT_UNDERFLOW) {
            engine->underrunCount_.fetch_add(1);
            engine->getErrorHandler().reportRealTimeError(
                AudioErrorCode::StreamUnderrun, 
                "Output buffer underrun detected"
            );
        }
    }
    
    // Get correct number of channels with error checking
    int numChannels = engine->getNumChannels();
    if (numChannels <= 0 || numChannels > 32) {
        engine->getErrorHandler().reportRealTimeError(
            AudioErrorCode::DeviceConfigurationFailed,
            "Invalid channel count: " + std::to_string(numChannels)
        );
        numChannels = 2; // Safe fallback
    }
    
    // Zero output buffer first with correct channel count
    std::memset(outputBuffer, 0, nFrames * numChannels * sizeof(float));
    
    try {
        // Access the callback through a thread-safe getter
        AudioEngine::AudioCallback callback = engine->getCallback();
        if (callback) {
            // Execute the callback with error handling
            callback(static_cast<float*>(outputBuffer), nFrames);
        }
        
        // Check audio safety if enabled
        if (engine->audioSafetyEnabled_.load()) {
            engine->checkAudioSafety(static_cast<float*>(outputBuffer), nFrames * numChannels);
        }
        
    } catch (const std::exception& e) {
        // Critical error in callback
        engine->getErrorHandler().reportRealTimeError(
            AudioErrorCode::CallbackException,
            "Exception in audio callback: " + std::string(e.what())
        );
        
        // Zero the buffer to prevent noise
        std::memset(outputBuffer, 0, nFrames * numChannels * sizeof(float));
        
        return 1; // Signal error
    } catch (...) {
        // Unknown exception
        engine->getErrorHandler().reportRealTimeError(
            AudioErrorCode::CallbackException,
            "Unknown exception in audio callback"
        );
        
        std::memset(outputBuffer, 0, nFrames * numChannels * sizeof(float));
        return 1;
    }
    
    // Measure callback performance if monitoring is enabled
    if (engine->performanceMonitoringEnabled_.load()) {
        auto callbackEnd = std::chrono::steady_clock::now();
        engine->measureCallbackPerformance(callbackStart, callbackEnd);
    }
    
    return 0;
}

// Pimpl implementation
class AudioEngine::Impl {
public:
    Impl(int sampleRate, int bufferSize, AudioEngine* parent) 
        : sampleRate(sampleRate), bufferSize(bufferSize), 
          audio(nullptr), parent(parent), numChannels(2) {
    }
    
    ~Impl() {
        shutdown(); // Ensure clean shutdown
    }
    
    bool initialize() {
        try {
            // Create RtAudio instance using unique_ptr
            audio = std::make_unique<RtAudio>();
            
            // Check available audio devices
            unsigned int devices = audio->getDeviceCount();
            if (devices < 1) {
                std::cerr << "No audio devices found!" << std::endl;
                audio.reset(); // Clean up allocated audio instance
                return false;
            }
            
            // Get default output device
            unsigned int outputDevice = audio->getDefaultOutputDevice();
            RtAudio::DeviceInfo info = audio->getDeviceInfo(outputDevice);
            
            // Store number of output channels
            numChannels = 2; // Default to stereo
            if (info.outputChannels > 0) {
                numChannels = info.outputChannels;
                // Limit to stereo for simplicity if more channels available
                if (numChannels > 2) numChannels = 2;
            }
            
            // Pass number of channels back to parent
            if (parent) {
                parent->numChannels_ = numChannels;
            }
            
            // Configure stream parameters
            RtAudio::StreamParameters oParams;
            oParams.deviceId = outputDevice;
            oParams.nChannels = numChannels;
            oParams.firstChannel = 0;
            
            // Configure stream options with platform-appropriate priority
            RtAudio::StreamOptions options;
            options.flags = RTAUDIO_SCHEDULE_REALTIME;
            options.numberOfBuffers = 2; // Double buffering
            
            // Set reasonable priority level based on platform
            #ifdef _WIN32
                options.priority = 1; // Windows: THREAD_PRIORITY_HIGHEST
            #else
                options.priority = 70; // Unix-like: use 70 as moderate RT priority
            #endif
            
            // Open audio stream - need to convert bufferSize to unsigned int*
            unsigned int bufferFrames = bufferSize;
            audio->openStream(&oParams, nullptr, RTAUDIO_FLOAT32, 
                             sampleRate, &bufferFrames, &audioCallback, 
                             this->parent, &options);
            
            // Check if buffer size was changed by RtAudio
            if (bufferSize != static_cast<int>(bufferFrames)) {
                std::cout << "Buffer size adjusted from " << bufferSize 
                          << " to " << bufferFrames << " frames" << std::endl;
                bufferSize = static_cast<int>(bufferFrames);
                
                // Update parent's buffer size
                if (parent) {
                    parent->bufferSize_ = bufferSize;
                }
            }
            
            // Start the stream
            audio->startStream();
            
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "RtAudio error: " << e.what() << std::endl;
            audio.reset(); // Clean up on error
            return false;
        }
    }
    
    void shutdown() {
        if (audio) {
            try {
                // Stop stream if running
                if (audio->isStreamRunning()) {
                    audio->stopStream();
                }
                
                // Close stream if open
                if (audio->isStreamOpen()) {
                    audio->closeStream();
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error during audio shutdown: " << e.what() << std::endl;
            }
        }
    }
    
    int sampleRate;
    int bufferSize;
    int numChannels;
    std::unique_ptr<RtAudio> audio;
    AudioEngine* parent;
};

// AudioEngine implementation
AudioEngine::AudioEngine(int sampleRate, int bufferSize)
    : sampleRate_(sampleRate), 
      bufferSize_(bufferSize),
      startTime_(std::chrono::steady_clock::now()),
      lastCallbackTime_(std::chrono::steady_clock::now()),
      // Create implementation with parent pointer already set
      pimpl_(new Impl(sampleRate, bufferSize, this)) {
    
    // Initialize error handler with stream context
    errorHandler_.setStreamContext(sampleRate, bufferSize, numChannels_);
    
    // Set up performance thresholds (can be customized later)
    errorHandler_.setPerformanceThresholds(
        80.0f,                                    // 80% CPU load
        std::chrono::microseconds{10000},         // 10ms latency
        std::chrono::microseconds{1000}           // 1ms jitter
    );
    
    // Set up error callbacks for logging
    errorHandler_.setErrorCallback([](const AudioError& error) {
        std::cout << "[AUDIO ERROR] " << AudioErrorHandler::severityToString(error.severity) 
                  << ": " << error.message << std::endl;
    });
    
    errorHandler_.setCriticalErrorCallback([](const AudioError& error) {
        std::cerr << "[CRITICAL AUDIO ERROR] " << error.message 
                  << " (Code: " << static_cast<int>(error.code) << ")" << std::endl;
    });
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize() {
    // Use atomic for thread-safety
    if (isInitialized_.load(std::memory_order_acquire)) {
        return true;
    }
    
    bool success = pimpl_->initialize();
    isInitialized_.store(success, std::memory_order_release);
    return success;
}

void AudioEngine::shutdown() {
    // Use atomic for thread-safety
    if (isInitialized_.load(std::memory_order_acquire)) {
        pimpl_->shutdown();
        isInitialized_.store(false, std::memory_order_release);
    }
}

void AudioEngine::setAudioCallback(AudioCallback callback) {
    // Thread-safe callback update
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callback_ = callback;
}

// Thread-safe accessor for the callback
AudioEngine::AudioCallback AudioEngine::getCallback() const {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    return callback_; // Return a copy of the callback
}

// Get current audio stream time in seconds (since stream started)
double AudioEngine::getStreamTime() const {
    if (!isInitialized_.load(std::memory_order_acquire) || !pimpl_ || !pimpl_->audio) {
        return 0.0;
    }

    try {
        return pimpl_->audio->getStreamTime();
    } catch (const std::exception& e) {
        std::cerr << "Error getting stream time: " << e.what() << std::endl;
        return 0.0;
    }
}

// Synchronize a sequencer with the audio engine's timing
void AudioEngine::synchronizeSequencer(std::shared_ptr<Sequencer> sequencer) {
    if (!sequencer || !isInitialized_.load(std::memory_order_acquire)) {
        return;
    }

    // Get current audio stream time
    double streamTime = getStreamTime();

    // Synchronize the sequencer with the audio engine's timing
    sequencer->synchronizeWithAudioEngine(streamTime, static_cast<double>(sampleRate_));
}

// Enterprise-grade performance monitoring and error handling methods

AudioEngine::PerformanceMetrics AudioEngine::getPerformanceMetrics() const {
    PerformanceMetrics metrics;
    
    metrics.cpuLoad = currentCPULoad_.load();
    metrics.memoryUsage = currentMemoryUsage_.load();
    metrics.latency = std::chrono::microseconds{currentLatency_.load()};
    metrics.jitter = std::chrono::microseconds{currentJitter_.load()};
    metrics.underrunCount = underrunCount_.load();
    metrics.overrunCount = overrunCount_.load();
    metrics.isHealthy = isHealthy_.load();
    
    // Calculate uptime
    auto now = std::chrono::steady_clock::now();
    metrics.uptime = std::chrono::duration<double>(now - startTime_).count();
    
    return metrics;
}

void AudioEngine::setPerformanceMonitoringEnabled(bool enabled) {
    performanceMonitoringEnabled_.store(enabled);
    
    if (enabled) {
        AUDIO_ERROR_CONTEXT(errorHandler_, "setPerformanceMonitoringEnabled");
        errorCtx.reportError(AudioErrorCode::Unknown, AudioErrorSeverity::Info, 
                           "Performance monitoring enabled");
    }
}

void AudioEngine::setPerformanceThresholds(float maxCPULoad, 
                                          std::chrono::microseconds maxLatency,
                                          std::chrono::microseconds maxJitter) {
    errorHandler_.setPerformanceThresholds(maxCPULoad, maxLatency, maxJitter);
}

void AudioEngine::setAudioSafetyEnabled(bool enabled) {
    audioSafetyEnabled_.store(enabled);
    
    AUDIO_ERROR_CONTEXT(errorHandler_, "setAudioSafetyEnabled");
    errorCtx.reportError(AudioErrorCode::Unknown, AudioErrorSeverity::Info, 
                       enabled ? "Audio safety enabled" : "Audio safety disabled");
}

bool AudioEngine::isHealthy() const {
    return isHealthy_.load();
}

AudioErrorHandler::AudioErrorStatistics AudioEngine::getErrorStatistics() const {
    return errorHandler_.getStatistics();
}

void AudioEngine::updatePerformanceMetrics() {
    if (!performanceMonitoringEnabled_.load()) {
        return;
    }
    
    // Calculate CPU load based on callback duration vs. available time
    auto bufferDurationUs = static_cast<float>(bufferSize_) / sampleRate_ * 1000000.0f;
    float instantCPULoad = (lastCallbackDuration_.count() / bufferDurationUs) * 100.0f;
    
    // Apply exponential smoothing
    float currentCPU = currentCPULoad_.load();
    float smoothedCPU = cpuLoadSmoothingFactor_ * currentCPU + (1.0f - cpuLoadSmoothingFactor_) * instantCPULoad;
    currentCPULoad_.store(smoothedCPU);
    
    // Update memory usage (placeholder - would use actual memory measurement)
    // For now, estimate based on buffer usage and voice count
    float memoryUsage = (bufferSize_ * numChannels_ * sizeof(float)) / (1024.0f * 1024.0f); // MB
    currentMemoryUsage_.store(memoryUsage);
    
    // Calculate latency (theoretical based on buffer size)
    auto latencyUs = static_cast<long>((bufferSize_ / static_cast<double>(sampleRate_)) * 1000000.0);
    currentLatency_.store(latencyUs);
    
    // Update error handler with current metrics
    errorHandler_.updatePerformanceMetrics(
        smoothedCPU, memoryUsage,
        std::chrono::microseconds{latencyUs},
        std::chrono::microseconds{currentJitter_.load()}
    );
    
    // Update health status
    bool healthy = (smoothedCPU < 90.0f) && 
                   (latencyUs < 20000) && 
                   (underrunCount_.load() < 10) &&
                   (overrunCount_.load() < 5);
    isHealthy_.store(healthy);
}

void AudioEngine::measureCallbackPerformance(const std::chrono::steady_clock::time_point& start,
                                            const std::chrono::steady_clock::time_point& end) {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    lastCallbackDuration_ = duration;
    
    // Calculate jitter (variation in callback timing)
    auto timeSinceLastCallback = std::chrono::duration_cast<std::chrono::microseconds>(start - lastCallbackTime_);
    auto expectedCallbackInterval = std::chrono::microseconds{static_cast<long>((bufferSize_ / static_cast<double>(sampleRate_)) * 1000000.0)};
    auto jitter = std::abs(timeSinceLastCallback.count() - expectedCallbackInterval.count());
    currentJitter_.store(jitter);
    
    lastCallbackTime_ = start;
    
    // Update performance metrics
    updatePerformanceMetrics();
    
    // Check for performance issues
    if (duration.count() > expectedCallbackInterval.count()) {
        errorHandler_.reportRealTimeError(
            AudioErrorCode::CallbackTimeout,
            "Callback duration exceeded buffer time: " + std::to_string(duration.count()) + "μs"
        );
    }
}

void AudioEngine::checkAudioSafety(float* outputBuffer, int totalSamples) {
    if (!outputBuffer || totalSamples <= 0) {
        return;
    }
    
    float maxSample = 0.0f;
    float rmsSum = 0.0f;
    bool clippingDetected = false;
    bool dcOffsetDetected = false;
    
    // Analyze audio samples
    for (int i = 0; i < totalSamples; ++i) {
        float sample = outputBuffer[i];
        float absSample = std::abs(sample);
        
        // Check for clipping
        if (absSample >= 0.99f) {
            clippingDetected = true;
        }
        
        maxSample = std::max(maxSample, absSample);
        rmsSum += sample * sample;
    }
    
    float rms = std::sqrt(rmsSum / totalSamples);
    
    // Check for dangerous levels
    if (clippingDetected) {
        errorHandler_.reportRealTimeError(
            AudioErrorCode::AudioClipping,
            "Audio clipping detected - max sample: " + std::to_string(maxSample)
        );
        
        // Apply safety limiter
        for (int i = 0; i < totalSamples; ++i) {
            if (outputBuffer[i] > 0.95f) outputBuffer[i] = 0.95f;
            if (outputBuffer[i] < -0.95f) outputBuffer[i] = -0.95f;
        }
    }
    
    // Check for dangerous RMS levels
    if (rms > 0.7f) {
        errorHandler_.reportRealTimeError(
            AudioErrorCode::VolumeClampingActivated,
            "High RMS level detected: " + std::to_string(rms)
        );
    }
    
    // Check for DC offset
    if (maxSample > 0.1f && rms < 0.01f) {
        errorHandler_.reportRealTimeError(
            AudioErrorCode::DCOffsetDetected,
            "Potential DC offset detected"
        );
    }
}

} // namespace AIMusicHardware