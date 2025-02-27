#pragma once

#include <cmath>
#include <algorithm>
#include <string>

namespace AIMusicHardware {

// Common constants and utility functions for audio effects

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;

// Convert MIDI note to frequency (A4 = 69 = 440Hz)
inline float midiNoteToFrequency(int midiNote) {
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

// Convert frequency to MIDI note (may not be an integer)
inline float frequencyToMidiNote(float frequency) {
    return 69.0f + 12.0f * std::log2(frequency / 440.0f);
}

// Convert dB to linear gain
inline float dbToGain(float dB) {
    return std::pow(10.0f, dB / 20.0f);
}

// Convert linear gain to dB
inline float gainToDb(float gain) {
    return 20.0f * std::log10(gain);
}

// Linear interpolation
inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Map a value from one range to another
inline float mapValue(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

// Clamp a value between min and max
inline float clamp(float value, float min, float max) {
    return std::max(min, std::min(max, value));
}

// Soft clipping function
inline float softClip(float sample) {
    return std::tanh(sample);
}

// Hard clipping function
inline float hardClip(float sample, float threshold = 1.0f) {
    return std::clamp(sample, -threshold, threshold);
}

// First-order low-pass filter
class OnePoleFilter {
public:
    OnePoleFilter() : a0_(1.0f), b1_(0.0f), y1_(0.0f) {}
    
    void setCoefficient(float coeff) {
        b1_ = std::clamp(coeff, 0.0f, 1.0f);
        a0_ = 1.0f - b1_;
    }
    
    void setCutoff(float cutoff, float sampleRate) {
        float x = std::exp(-TWO_PI * cutoff / sampleRate);
        setCoefficient(x);
    }
    
    float process(float input) {
        y1_ = a0_ * input + b1_ * y1_;
        return y1_;
    }
    
    void reset() {
        y1_ = 0.0f;
    }
    
private:
    float a0_;  // Input coefficient
    float b1_;  // Feedback coefficient
    float y1_;  // Previous output
};

} // namespace AIMusicHardware