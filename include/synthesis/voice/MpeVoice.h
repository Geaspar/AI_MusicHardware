#pragma once

#include "voice_manager.h"
#include <memory>

namespace AIMusicHardware {

// Forward declarations
class Filter;
class ModEnvelope;
class WavetableOscillator;

/**
 * @class MpeVoice
 * @brief Extended voice class for MPE support
 *
 * This class extends the base Voice class with additional support for
 * MPE expression parameters, particularly the timbre dimension (CC74).
 */
class MpeVoice : public Voice {
public:
    /**
     * @brief Constructor
     *
     * @param sampleRate Audio sample rate
     */
    MpeVoice(int sampleRate = 44100);

    /**
     * @brief Destructor
     */
    ~MpeVoice();

    /**
     * @brief Set the timbre value (Y-axis in MPE)
     *
     * @param value Timbre value (0.0 to 1.0)
     */
    void setTimbre(float value);

    /**
     * @brief Get the current timbre value
     *
     * @return Timbre value (0.0 to 1.0)
     */
    float getTimbre() const;

    /**
     * @brief Update all MPE expression parameters at once
     *
     * @param pitchBend Pitch bend in semitones
     * @param timbre Timbre value (0.0 to 1.0)
     * @param pressure Pressure value (0.0 to 1.0)
     */
    void updateExpression(float pitchBend, float timbre, float pressure);

    /**
     * @brief Generate a single audio sample
     *
     * Extends the base class to implement MPE-aware sample generation
     *
     * @return The generated audio sample
     */
    float generateSample();

private:
    // MPE-specific parameters
    float timbre_ = 0.5f;  // Normalized timbre (CC74), centered at 0.5

    // Filter for timbre control
    Filter* filter_ = nullptr;

    // Access to base oscillator and envelope values
    float getOscillatorSample() const;
    float getEnvelopeValue() const;
    float getVelocity() const;

    // Process timbre modulation
    float processTimbreModulation() const;

    // Process pressure modulation
    float processPressureModulation() const;

    // Calculate filter cutoff frequency based on timbre
    float calculateFilterCutoff() const;

    // Apply filter to a sample
    float applyFilter(float sample);
};

} // namespace AIMusicHardware