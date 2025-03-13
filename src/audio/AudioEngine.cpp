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
      bool isStreamOpen() { return false; }
      bool isStreamRunning() { return false; }
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
    
    // Check for stream underflow/overflow
    if (status) {
        if (status & RTAUDIO_INPUT_OVERFLOW) {
            std::cerr << "Input overflow detected!" << std::endl;
        }
        if (status & RTAUDIO_OUTPUT_UNDERFLOW) {
            std::cerr << "Output underflow detected!" << std::endl;
        }
    }
    
    // Zero output buffer first
    std::memset(outputBuffer, 0, nFrames * 2 * sizeof(float)); // Stereo = 2 channels
    
    // Access the callback through our helper class
    if (engine) {
        // Get the callback
        AudioEngine::AudioCallback callback = engine->getCallback();
        if (callback) {
            callback(static_cast<float*>(outputBuffer), nFrames);
        }
    }
    
    return 0;
}

// Pimpl implementation
class AudioEngine::Impl {
public:
    Impl(int sampleRate, int bufferSize) 
        : sampleRate(sampleRate), bufferSize(bufferSize), audio(nullptr) {
    }
    
    ~Impl() {
        if (audio) {
            if (audio->isStreamRunning()) {
                audio->stopStream();
            }
            if (audio->isStreamOpen()) {
                audio->closeStream();
            }
            delete audio;
        }
    }
    
    bool initialize() {
        try {
            // Create RtAudio instance
            audio = new RtAudio();
            
            // Check available audio devices
            unsigned int devices = audio->getDeviceCount();
            if (devices < 1) {
                std::cerr << "No audio devices found!" << std::endl;
                return false;
            }
            
            // Get default output device
            unsigned int outputDevice = audio->getDefaultOutputDevice();
            RtAudio::DeviceInfo info = audio->getDeviceInfo(outputDevice);
            
            // Configure stream parameters
            RtAudio::StreamParameters oParams;
            oParams.deviceId = outputDevice;
            oParams.nChannels = 2; // Stereo output
            oParams.firstChannel = 0;
            
            // Configure stream options
            RtAudio::StreamOptions options;
            options.flags = RTAUDIO_SCHEDULE_REALTIME;
            options.numberOfBuffers = 2; // Double buffering
            options.priority = 85; // High priority
            
            // Open audio stream - need to convert bufferSize to unsigned int*
            unsigned int bufferFrames = bufferSize;
            audio->openStream(&oParams, nullptr, RTAUDIO_FLOAT32, 
                              sampleRate, &bufferFrames, &audioCallback, 
                              this->parent, &options);
            
            // Update buffer size in case it was changed by RtAudio
            bufferSize = static_cast<int>(bufferFrames);
            
            // Start the stream
            audio->startStream();
            
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "RtAudio error: " << e.what() << std::endl;
            return false;
        }
    }
    
    void shutdown() {
        if (audio) {
            if (audio->isStreamRunning()) {
                try {
                    audio->stopStream();
                }
                catch (const std::exception& e) {
                    std::cerr << "Error stopping stream: " << e.what() << std::endl;
                }
            }
            
            if (audio->isStreamOpen()) {
                audio->closeStream();
            }
        }
    }
    
    int sampleRate;
    int bufferSize;
    RtAudio* audio;
    AudioEngine* parent;
};

// AudioEngine implementation
AudioEngine::AudioEngine(int sampleRate, int bufferSize)
    : sampleRate_(sampleRate), 
      bufferSize_(bufferSize), 
      isInitialized_(false),
      pimpl_(new Impl(sampleRate, bufferSize)) {
    // Store the parent pointer in the implementation
    pimpl_->parent = this;
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize() {
    if (isInitialized_) {
        return true;
    }
    
    bool success = pimpl_->initialize();
    isInitialized_ = success;
    return success;
}

void AudioEngine::shutdown() {
    if (isInitialized_) {
        pimpl_->shutdown();
        isInitialized_ = false;
    }
}

void AudioEngine::setAudioCallback(AudioCallback callback) {
    callback_ = callback;
}

// Helper method to access the callback from our C callback function
AudioEngine::AudioCallback AudioEngine::getCallback() const {
    return callback_;
}

} // namespace AIMusicHardware