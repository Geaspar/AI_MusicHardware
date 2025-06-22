#pragma once

#include "voice_manager.h"
#include "../oscillators/band_limited_oscillator.h"
#include "../wavetable/band_limited_wavetable.h"
#include <memory>

namespace AIMusicHardware {

/**
 * @class BandLimitedVoice
 * @brief Voice implementation using band-limited oscillators for alias-free synthesis
 *
 * This voice class replaces the standard WavetableOscillator with a BandLimitedOscillator
 * to provide professional-quality, alias-free audio generation.
 */
class BandLimitedVoice : public Voice {
public:
    /**
     * @brief Constructor
     * @param sampleRate Audio sample rate
     * @param enableOversampling Whether to enable oversampling for additional quality
     */
    BandLimitedVoice(int sampleRate = 44100, bool enableOversampling = false);
    
    /**
     * @brief Destructor
     */
    ~BandLimitedVoice() override;
    
    /**
     * @brief Generate a single sample using band-limited oscillator
     * @return The generated audio sample
     */
    float generateSample() override;
    
    /**
     * @brief Process a block of samples
     * @param buffer Output buffer
     * @param numFrames Number of frames to process
     */
    void process(float* buffer, int numFrames) override;
    
    /**
     * @brief Set the waveform type
     * @param waveType The waveform type to use
     */
    void setWaveform(BandLimitedWavetable::WaveType waveType);
    
    /**
     * @brief Get the current waveform type
     * @return The current waveform type
     */
    BandLimitedWavetable::WaveType getWaveform() const;
    
    /**
     * @brief Enable or disable oversampling
     * @param enable Whether to enable oversampling
     */
    void setOversamplingEnabled(bool enable);
    
    /**
     * @brief Check if oversampling is enabled
     * @return Whether oversampling is enabled
     */
    bool isOversamplingEnabled() const;
    
    /**
     * @brief Set oversampling factor
     * @param factor The oversampling factor (x1, x2, x4, x8)
     */
    void setOversamplingFactor(OversamplingProcessor::Factor factor);
    
    /**
     * @brief Get the current oversampling factor
     * @return The current oversampling factor
     */
    OversamplingProcessor::Factor getOversamplingFactor() const;
    
    /**
     * @brief Override to bypass wavetable setting (uses band-limited wavetables instead)
     * @param wavetable Ignored parameter for compatibility
     */
    void setWavetable(std::shared_ptr<Wavetable> wavetable) override;
    
    /**
     * @brief Update sample rate for band-limited oscillator
     * @param sampleRate New sample rate
     */
    void setSampleRate(int sampleRate) override;

    /**
     * @brief Override note on to properly initialize
     * @param midiNote MIDI note number
     * @param velocity Note velocity (0-1)
     */
    void noteOn(int midiNote, float velocity);

private:
    // Band-limited oscillator instance
    std::unique_ptr<BandLimitedOscillator> bandLimitedOscillator_;
    
    // Current waveform type
    BandLimitedWavetable::WaveType currentWaveform_;
    
    // Oversampling state
    bool oversamplingEnabled_;
    OversamplingProcessor::Factor oversamplingFactor_;
    
    // DC blocker filter state
    float dcBlockerX1_;  // Previous input
    float dcBlockerY1_;  // Previous output
};

/**
 * @class BandLimitedVoiceManager
 * @brief Voice manager that creates BandLimitedVoice instances
 *
 * This specialized voice manager creates voices with band-limited oscillators
 * for professional-quality, alias-free synthesis.
 */
class BandLimitedVoiceManager : public VoiceManager {
public:
    /**
     * @brief Constructor
     * @param sampleRate Audio sample rate
     * @param maxVoices Maximum number of voices
     * @param enableOversampling Whether to enable oversampling by default
     */
    BandLimitedVoiceManager(int sampleRate = 44100, int maxVoices = 16, bool enableOversampling = false);
    
    /**
     * @brief Destructor
     */
    ~BandLimitedVoiceManager();
    
    /**
     * @brief Set the waveform for all voices
     * @param waveType The waveform type to use
     */
    void setWaveform(BandLimitedWavetable::WaveType waveType);
    
    /**
     * @brief Enable or disable oversampling for all voices
     * @param enable Whether to enable oversampling
     */
    void setOversamplingEnabled(bool enable);
    
    /**
     * @brief Set oversampling factor for all voices
     * @param factor The oversampling factor
     */
    void setOversamplingFactor(OversamplingProcessor::Factor factor);

protected:
    /**
     * @brief Create a band-limited voice
     * @return A new band-limited voice
     */
    std::unique_ptr<Voice> createVoice() override;

private:
    // Default settings for new voices
    BandLimitedWavetable::WaveType defaultWaveform_;
    bool defaultOversamplingEnabled_;
    OversamplingProcessor::Factor defaultOversamplingFactor_;
};

} // namespace AIMusicHardware