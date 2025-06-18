#pragma once

#include "../wavetable/band_limited_wavetable.h"
#include <memory>

namespace AIMusicHardware {

/**
 * High-quality oscillator using band-limited wavetables
 * with optional oversampling for pristine audio quality
 */
class BandLimitedOscillator {
public:
    BandLimitedOscillator(float sampleRate = 44100.0f);
    ~BandLimitedOscillator();
    
    /**
     * Set the waveform type
     */
    void setWaveform(BandLimitedWavetable::WaveType type);
    
    /**
     * Set oscillator frequency in Hz
     */
    void setFrequency(float frequency);
    
    /**
     * Set phase offset (0.0 - 1.0)
     */
    void setPhase(float phase);
    
    /**
     * Reset phase to 0
     */
    void resetPhase();
    
    /**
     * Set amplitude/volume (0.0 - 1.0)
     */
    void setAmplitude(float amplitude);
    
    /**
     * Enable/disable oversampling
     */
    void setOversamplingEnabled(bool enable);
    
    /**
     * Set oversampling factor
     */
    void setOversamplingFactor(OversamplingProcessor::Factor factor);
    
    /**
     * Generate next sample
     */
    float generateSample();
    
    /**
     * Generate a block of samples
     */
    void generateBlock(float* output, int numSamples);
    
    /**
     * Update sample rate
     */
    void setSampleRate(float sampleRate);
    
    /**
     * Get current frequency
     */
    float getFrequency() const { return frequency_; }
    
    /**
     * Get current phase
     */
    float getPhase() const { return phase_; }
    
private:
    // Core oscillator state
    float frequency_;
    float phase_;
    float phaseIncrement_;
    float amplitude_;
    float sampleRate_;
    
    // Wavetable and oversampling
    std::unique_ptr<BandLimitedWavetable> wavetable_;
    std::unique_ptr<OversamplingProcessor> oversampler_;
    bool oversamplingEnabled_;
    
    // Update phase increment when frequency or sample rate changes
    void updatePhaseIncrement();
    
    // Generate a single sample without oversampling
    float generateSampleDirect();
};

/**
 * Factory for creating oscillators with different configurations
 */
class OscillatorFactory {
public:
    /**
     * Create a high-quality band-limited oscillator
     */
    static std::unique_ptr<BandLimitedOscillator> createBandLimitedOscillator(
        float sampleRate = 44100.0f,
        BandLimitedWavetable::WaveType waveform = BandLimitedWavetable::WaveType::Saw,
        bool enableOversampling = false);
    
    /**
     * Create an oscillator optimized for LFO use (lower quality, more efficient)
     */
    static std::unique_ptr<BandLimitedOscillator> createLFO(
        float sampleRate = 44100.0f,
        BandLimitedWavetable::WaveType waveform = BandLimitedWavetable::WaveType::Sine);
};

} // namespace AIMusicHardware