#include "../../include/audio/AudioEngine.h"
#include <RtAudio.h>
#include <iostream>
#include <cstring>
#include <stdexcept>

namespace AIMusicHardware {

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
    std::memset(outputBuffer, 0, nFrames * sizeof(float));
    
    // If we have a callback registered, call it
    if (engine && engine->callback_) {
        engine->callback_(static_cast<float*>(outputBuffer), nFrames);
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
            
            // Open audio stream
            audio->openStream(&oParams, nullptr, RTAUDIO_FLOAT32, 
                              sampleRate, &bufferSize, &audioCallback, 
                              this->parent, &options);
            
            // Start the stream
            audio->startStream();
            
            return true;
        }
        catch (RtAudioError& e) {
            std::cerr << "RtAudio error: " << e.getMessage() << std::endl;
            return false;
        }
    }
    
    void shutdown() {
        if (audio) {
            if (audio->isStreamRunning()) {
                try {
                    audio->stopStream();
                }
                catch (RtAudioError& e) {
                    std::cerr << "Error stopping stream: " << e.getMessage() << std::endl;
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

} // namespace AIMusicHardware