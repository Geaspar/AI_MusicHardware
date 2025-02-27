#include "../../include/effects/BassBoost.h"
#include "../../include/effects/EffectUtils.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

BassBoost::BassBoost(int sampleRate)
    : Effect(sampleRate),
      frequency_(100.0f), // Bass frequency to boost
      gain_(6.0f),        // Boost amount in dB
      width_(1.0f),       // Width of boost (Q factor)
      drive_(1.0f) {      // Additional drive
    
    // Initialize filter state
    for (int i = 0; i < 2; ++i) { // stereo
        x1_[i] = x2_[i] = y1_[i] = y2_[i] = 0.0f;
    }
    
    // Calculate initial filter coefficients
    calculateCoefficients();
}

BassBoost::~BassBoost() {
}

void BassBoost::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
    calculateCoefficients();
}

void BassBoost::process(float* buffer, int numFrames) {
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Get input samples
        float inputL = buffer[i];
        float inputR = buffer[i + 1];
        
        // Apply slight drive for harmonics enhancement
        if (drive_ > 1.0f) {
            inputL = std::tanh(inputL * drive_) / drive_;
            inputR = std::tanh(inputR * drive_) / drive_;
        }
        
        // Process left channel with peak filter
        float outputL = b0_ * inputL + b1_ * x1_[0] + b2_ * x2_[0] - a1_ * y1_[0] - a2_ * y2_[0];
        
        // Update filter state (left)
        x2_[0] = x1_[0];
        x1_[0] = inputL;
        y2_[0] = y1_[0];
        y1_[0] = outputL;
        
        // Process right channel with peak filter
        float outputR = b0_ * inputR + b1_ * x1_[1] + b2_ * x2_[1] - a1_ * y1_[1] - a2_ * y2_[1];
        
        // Update filter state (right)
        x2_[1] = x1_[1];
        x1_[1] = inputR;
        y2_[1] = y1_[1];
        y1_[1] = outputR;
        
        // Store output
        buffer[i] = outputL;
        buffer[i + 1] = outputR;
    }
}

void BassBoost::calculateCoefficients() {
    // Calculate peak EQ filter coefficients
    float A = std::pow(10.0f, gain_ / 40.0f); // Convert dB to linear
    float omega = 2.0f * PI * frequency_ / sampleRate_;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    
    float alpha = sinOmega / (2.0f * width_);
    
    // Calculate biquad peak filter coefficients
    b0_ = 1.0f + alpha * A;
    b1_ = -2.0f * cosOmega;
    b2_ = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    a1_ = -2.0f * cosOmega;
    a2_ = 1.0f - alpha / A;
    
    // Normalize by a0
    b0_ /= a0;
    b1_ /= a0;
    b2_ /= a0;
    a1_ /= a0;
    a2_ /= a0;
}

void BassBoost::setParameter(const std::string& name, float value) {
    bool recalculate = false;
    
    if (name == "frequency") {
        frequency_ = clamp(value, 20.0f, 500.0f); // Limit to bass frequencies
        recalculate = true;
    }
    else if (name == "gain") {
        gain_ = clamp(value, 0.0f, 24.0f); // 0 to 24 dB
        recalculate = true;
    }
    else if (name == "width") {
        width_ = clamp(value, 0.1f, 5.0f); // Q factor
        recalculate = true;
    }
    else if (name == "drive") {
        drive_ = clamp(value, 1.0f, 3.0f); // Additional saturation
    }
    
    if (recalculate) {
        calculateCoefficients();
    }
}

float BassBoost::getParameter(const std::string& name) const {
    if (name == "frequency") {
        return frequency_;
    }
    else if (name == "gain") {
        return gain_;
    }
    else if (name == "width") {
        return width_;
    }
    else if (name == "drive") {
        return drive_;
    }
    return 0.0f;
}

} // namespace AIMusicHardware