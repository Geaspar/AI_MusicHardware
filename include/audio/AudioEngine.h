#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>

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
    
private:
    int sampleRate_;
    int bufferSize_;
    int numChannels_ = 2;  // Default to stereo
    std::atomic<bool> isInitialized_{false};
    
    // Thread-safe callback handling
    mutable std::mutex callbackMutex_;
    AudioCallback callback_;
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace AIMusicHardware