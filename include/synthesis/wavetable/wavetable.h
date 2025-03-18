#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cmath>

namespace AIMusicHardware {

/**
 * WaveFrame represents a single cycle of audio in a wavetable.
 */
class WaveFrame {
public:
    WaveFrame(int size = 2048);
    ~WaveFrame();
    
    // Initializes the frame with basic waveforms
    void initSine();
    void initSaw();
    void initSquare();
    void initTriangle();
    void initNoise();
    
    // Initializes from data
    void setData(const float* data, int size);
    
    // Access the data
    float* getData() { return data_.data(); }
    const float* getData() const { return data_.data(); }
    int getSize() const { return static_cast<int>(data_.size()); }
    
    // Sample the waveform at a specific phase (0-1)
    float getSample(float phase) const;
    
private:
    std::vector<float> data_;
};

/**
 * Wavetable contains multiple WaveFrames for interpolating between.
 */
class Wavetable {
public:
    Wavetable(int frameSize = 2048, int numFrames = 1);
    ~Wavetable();
    
    // Frame management
    void addFrame(std::unique_ptr<WaveFrame> frame);
    void setFrame(int index, std::unique_ptr<WaveFrame> frame);
    WaveFrame* getFrame(int index);
    int getNumFrames() const { return static_cast<int>(frames_.size()); }
    int getFrameSize() const { return frames_.empty() ? 0 : frames_[0]->getSize(); }
    
    // Initialize with common waveforms
    void initBasicWaveforms(int numFrames = 5);
    
    // Get an interpolated sample at position (frame) and phase
    float getSample(float framePosition, float phase) const;
    
private:
    std::vector<std::unique_ptr<WaveFrame>> frames_;
};

/**
 * WavetableOscillator uses a Wavetable to generate audio.
 */
class WavetableOscillator {
public:
    WavetableOscillator(int sampleRate = 44100);
    ~WavetableOscillator();
    
    // Set the wavetable to use
    void setWavetable(std::shared_ptr<Wavetable> wavetable);
    
    // Set parameters
    void setFrequency(float frequency);
    void setFramePosition(float position); // 0.0 - 1.0 for position in wavetable
    void setPhase(float phase); // 0.0 - 1.0
    void resetPhase();
    
    // Generate a sample
    float generateSample();
    
    // Set sample rate
    void setSampleRate(int sampleRate);
    
private:
    std::shared_ptr<Wavetable> wavetable_;
    float frequency_;
    float phase_;
    float framePosition_;
    int sampleRate_;
    
    // Prevent aliasing with oversampling
    bool oversample_;
    int oversamplingFactor_;
};

} // namespace AIMusicHardware