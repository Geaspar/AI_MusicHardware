#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>

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
    
    int getSampleRate() const { return sampleRate_; }
    int getBufferSize() const { return bufferSize_; }
    
private:
    int sampleRate_;
    int bufferSize_;
    bool isInitialized_;
    AudioCallback callback_;
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace AIMusicHardware