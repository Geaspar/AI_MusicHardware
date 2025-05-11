#include "../../include/audio/AudioEngine.h"

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

// RtAudio callback function
int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nFrames,
                 double streamTime, RtAudioStreamStatus status, void* userData) {
    
    // Cast user data to AudioEngine instance
    AudioEngine* engine = static_cast<AudioEngine*>(userData);
    if (!engine) {
        return 0;  // Safety check
    }
    
    // Check for stream underflow/overflow
    if (status) {
        if (status & RTAUDIO_INPUT_OVERFLOW) {
            std::cerr << "Input overflow detected!" << std::endl;
        }
        if (status & RTAUDIO_OUTPUT_UNDERFLOW) {
            std::cerr << "Output underflow detected!" << std::endl;
        }
    }
    
    // Get correct number of channels (default to stereo if we can't determine)
    int numChannels = 2; // Default assumption
    if (engine) {
        numChannels = engine->getNumChannels();  // Get actual channel count from engine
    }
    
    // Zero output buffer first with correct channel count
    std::memset(outputBuffer, 0, nFrames * numChannels * sizeof(float));
    
    // Access the callback through a thread-safe getter
    AudioEngine::AudioCallback callback = engine->getCallback();
    if (callback) {
        // Execute the callback
        callback(static_cast<float*>(outputBuffer), nFrames);
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
      // Create implementation with parent pointer already set
      pimpl_(new Impl(sampleRate, bufferSize, this)) {
    // No need to set parent pointer here as it's done in constructor
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

// Forward declaration of Sequencer to avoid circular includes
#include "../../include/sequencer/Sequencer.h"

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

} // namespace AIMusicHardware