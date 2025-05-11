#include "../../../include/synthesis/voice/MpeVoice.h"
#include "../../../include/effects/Filter.h"
#include "../../../include/synthesis/modulators/envelope.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

MpeVoice::MpeVoice(int sampleRate)
    : Voice(sampleRate),
      timbre_(0.5f) {

    // Create a filter for timbre control
    filter_ = new Filter(sampleRate, Filter::Type::LowPass);
    filter_->setParameter("frequency", 1000.0f);  // Default cutoff
    filter_->setParameter("resonance", 0.5f);
    filter_->setParameter("mix", 1.0f);  // 100% wet
}

MpeVoice::~MpeVoice() {
    if (filter_) {
        delete filter_;
        filter_ = nullptr;
    }
}

void MpeVoice::setTimbre(float value) {
    // Clamp to valid range
    timbre_ = std::clamp(value, 0.0f, 1.0f);
}

float MpeVoice::getTimbre() const {
    return timbre_;
}

void MpeVoice::updateExpression(float pitchBend, float timbre, float pressure) {
    // Update pitch bend
    setPitchBend(pitchBend);

    // Update timbre
    setTimbre(timbre);

    // Update pressure
    setPressure(pressure);
}

float MpeVoice::generateSample() {
    // First call the base Voice generateSample method to handle basic voice processing
    float baseSample = Voice::generateSample();

    // Skip further processing if voice is inactive
    if (getState() == State::Inactive || getState() == State::Finished) {
        return 0.0f;
    }

    // Apply MPE-specific processing (filter based on timbre)
    if (filter_) {
        // Set filter cutoff based on timbre
        float cutoff = calculateFilterCutoff();
        filter_->setParameter("frequency", cutoff);

        // Apply filter
        float filteredSample = applyFilter(baseSample);

        // Apply pressure modulation
        float amplitudeScale = processPressureModulation();

        return filteredSample * amplitudeScale;
    }

    // If no filter, just apply pressure modulation
    return baseSample * processPressureModulation();
}

float MpeVoice::getOscillatorSample() const {
    // This is a fallback method since we can't directly access the oscillator
    // In a real implementation, you would add methods to Voice to expose this
    return 0.0f;
}

float MpeVoice::getEnvelopeValue() const {
    // This is a fallback method since we can't directly access the envelope
    // In a real implementation, you would add methods to Voice to expose this
    return 0.0f;
}

float MpeVoice::getVelocity() const {
    // This is a fallback method since we can't directly access the velocity
    // In a real implementation, you would add methods to Voice to expose this
    return getCurrentAmplitude();
}

float MpeVoice::calculateFilterCutoff() const {
    // Map timbre value (0.0-1.0) to filter cutoff modulation
    // Center point (0.5) means no modulation
    float modAmount = (timbre_ - 0.5f) * 2.0f;  // Range: -1.0 to 1.0

    // Scale to frequency range using an exponential curve
    // for a more musical response
    float baseFreq = 1000.0f;  // 1kHz baseline
    float modFreq;

    if (modAmount >= 0.0f) {
        // Upward modulation (brighter) - range 1kHz to 20kHz
        modFreq = baseFreq * std::pow(20.0f, modAmount);
    } else {
        // Downward modulation (darker) - range 200Hz to 1kHz
        modFreq = baseFreq * std::pow(0.2f, -modAmount);
    }

    return modFreq;
}

float MpeVoice::applyFilter(float sample) {
    // Create a temporary buffer for the sample
    float buffer[2] = {sample, sample};  // Stereo

    // Process the buffer
    filter_->process(buffer, 1);

    // Return the processed sample (first channel)
    return buffer[0];
}

float MpeVoice::processTimbreModulation() const {
    // Calculate the filter cutoff based on timbre
    return calculateFilterCutoff();
}

float MpeVoice::processPressureModulation() const {
    // Get pressure value from base class
    float pressure = 0.0f;

    // Use reflection or other means to get the pressure value
    // This is a simplified implementation
    // In a real implementation, Voice would need to expose pressure_ or have a getPressure() method

    // Map pressure (0.0-1.0) to amplitude modulation
    // Start from 0.7 to ensure even zero pressure produces significant sound
    float ampScale = 0.7f + (pressure * 0.3f);  // Range: 0.7 to 1.0

    return ampScale;
}

} // namespace AIMusicHardware