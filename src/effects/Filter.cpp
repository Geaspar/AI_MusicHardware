#include "../../include/effects/Filter.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

Filter::Filter(int sampleRate, Type type)
    : Effect(sampleRate),
      type_(type),
      frequency_(1000.0f),
      resonance_(0.707f),  // Butterworth default
      gain_(0.0f),
      mix_(1.0f) {
    
    // Initialize filter state
    for (int i = 0; i < 2; ++i) {
        x1_[i] = x2_[i] = y1_[i] = y2_[i] = 0.0f;
    }
    
    // Calculate initial coefficients
    calculateCoefficients();
}

Filter::~Filter() {
}

void Filter::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
    calculateCoefficients();
}

std::string Filter::getName() const {
    switch (type_) {
        case Type::LowPass: return "LowPassFilter";
        case Type::HighPass: return "HighPassFilter";
        case Type::BandPass: return "BandPassFilter";
        case Type::Notch: return "NotchFilter";
        default: return "Filter";
    }
}

void Filter::process(float* buffer, int numFrames) {
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Get input samples
        float inputL = buffer[i];
        float inputR = buffer[i + 1];
        
        // Process left channel
        float outputL = b0_ * inputL + b1_ * x1_[0] + b2_ * x2_[0] - a1_ * y1_[0] - a2_ * y2_[0];
        
        // Update filter state (left)
        x2_[0] = x1_[0];
        x1_[0] = inputL;
        y2_[0] = y1_[0];
        y1_[0] = outputL;
        
        // Process right channel
        float outputR = b0_ * inputR + b1_ * x1_[1] + b2_ * x2_[1] - a1_ * y1_[1] - a2_ * y2_[1];
        
        // Update filter state (right)
        x2_[1] = x1_[1];
        x1_[1] = inputR;
        y2_[1] = y1_[1];
        y1_[1] = outputR;
        
        // Mix dry and wet signals
        buffer[i] = inputL * (1.0f - mix_) + outputL * mix_;
        buffer[i + 1] = inputR * (1.0f - mix_) + outputR * mix_;
    }
}

void Filter::calculateCoefficients() {
    // Normalized frequency (0 to π)
    float omega = 2.0f * PI * frequency_ / sampleRate_;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    
    // Alpha for bandwidth/resonance
    float alpha = sinOmega / (2.0f * resonance_);
    
    // Intermediate coefficient
    float A = std::pow(10.0f, gain_ / 40.0f); // Convert dB to linear gain
    
    // Calculate biquad filter coefficients based on filter type
    switch (type_) {
        case Type::LowPass:
            b0_ = (1.0f - cosOmega) / 2.0f;
            b1_ = 1.0f - cosOmega;
            b2_ = (1.0f - cosOmega) / 2.0f;
            a0_ = 1.0f + alpha;
            a1_ = -2.0f * cosOmega;
            a2_ = 1.0f - alpha;
            break;
            
        case Type::HighPass:
            b0_ = (1.0f + cosOmega) / 2.0f;
            b1_ = -(1.0f + cosOmega);
            b2_ = (1.0f + cosOmega) / 2.0f;
            a0_ = 1.0f + alpha;
            a1_ = -2.0f * cosOmega;
            a2_ = 1.0f - alpha;
            break;
            
        case Type::BandPass:
            b0_ = alpha;
            b1_ = 0.0f;
            b2_ = -alpha;
            a0_ = 1.0f + alpha;
            a1_ = -2.0f * cosOmega;
            a2_ = 1.0f - alpha;
            break;
            
        case Type::Notch:
            b0_ = 1.0f;
            b1_ = -2.0f * cosOmega;
            b2_ = 1.0f;
            a0_ = 1.0f + alpha;
            a1_ = -2.0f * cosOmega;
            a2_ = 1.0f - alpha;
            break;
    }
    
    // Normalize coefficients
    b0_ /= a0_;
    b1_ /= a0_;
    b2_ /= a0_;
    a1_ /= a0_;
    a2_ /= a0_;
}

void Filter::setParameter(const std::string& name, float value) {
    bool recalculate = false;
    
    if (name == "frequency") {
        frequency_ = clamp(value, 20.0f, 20000.0f);
        recalculate = true;
    }
    else if (name == "resonance") {
        resonance_ = clamp(value, 0.1f, 10.0f);
        recalculate = true;
    }
    else if (name == "gain") {
        gain_ = clamp(value, -24.0f, 24.0f);
        recalculate = true;
    }
    else if (name == "mix") {
        mix_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "type") {
        int typeInt = static_cast<int>(value);
        if (typeInt >= 0 && typeInt <= 3) {
            type_ = static_cast<Type>(typeInt);
            recalculate = true;
        }
    }
    
    if (recalculate) {
        calculateCoefficients();
    }
}

float Filter::getParameter(const std::string& name) const {
    if (name == "frequency") {
        return frequency_;
    }
    else if (name == "resonance") {
        return resonance_;
    }
    else if (name == "gain") {
        return gain_;
    }
    else if (name == "mix") {
        return mix_;
    }
    else if (name == "type") {
        return static_cast<float>(type_);
    }
    return 0.0f;
}

} // namespace AIMusicHardware