#include "../../include/effects/Compressor.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

Compressor::Compressor(int sampleRate)
    : Effect(sampleRate),
      threshold_(-24.0f),  // dB
      ratio_(4.0f),        // 4:1 compression ratio
      attack_(0.01f),      // 10ms attack
      release_(0.2f),      // 200ms release
      makeup_(6.0f),       // 6dB makeup gain
      knee_(6.0f),         // 6dB soft knee
      peakEnv_(0.0f) {
    
    // Initialize envelope follower
    envelope_[0] = envelope_[1] = 0.0f;
    
    // Calculate initial coefficients
    calculateCoefficients();
}

Compressor::~Compressor() {
}

void Compressor::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
    calculateCoefficients();
}

void Compressor::calculateCoefficients() {
    // Calculate smoothing coefficients for envelope follower
    attackCoeff_ = std::exp(-1.0f / (attack_ * sampleRate_));
    releaseCoeff_ = std::exp(-1.0f / (release_ * sampleRate_));
}

float Compressor::computeGain(float inputLevel) {
    // Convert input level from dB to the range where we apply compression
    float gainReduction = 0.0f;
    
    // Soft knee implementation
    if (inputLevel <= threshold_ - knee_ / 2.0f) {
        // Below threshold - no compression
        gainReduction = 0.0f;
    } 
    else if (inputLevel >= threshold_ + knee_ / 2.0f) {
        // Above threshold + knee - full compression
        gainReduction = (inputLevel - threshold_) * (1.0f - 1.0f / ratio_);
    } 
    else {
        // In the knee region - smooth transition
        float kneeDbAboveThresh = inputLevel - (threshold_ - knee_ / 2.0f);
        float kneeCompressRatio = 1.0f + ((ratio_ - 1.0f) * kneeDbAboveThresh / knee_);
        gainReduction = kneeDbAboveThresh * (1.0f - 1.0f / kneeCompressRatio);
    }
    
    // Return gain in linear scale with makeup gain applied
    return dbToGain(makeup_ - gainReduction);
}

void Compressor::process(float* buffer, int numFrames) {
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Get input samples
        float inputL = buffer[i];
        float inputR = buffer[i + 1];
        
        // Calculate input level (use max of both channels for stereo linking)
        float inputLeveldB = -144.0f; // Very low default value
        
        // Use peak value for both channels
        float absL = std::abs(inputL);
        float absR = std::abs(inputR);
        float peakValue = std::max(absL, absR);
        
        // Avoid log(0) and convert to dB
        if (peakValue > 1.0e-6f) {
            inputLeveldB = 20.0f * std::log10(peakValue);
        }
        
        // Envelope follower
        if (inputLeveldB > peakEnv_) {
            // Attack phase
            peakEnv_ = attackCoeff_ * peakEnv_ + (1.0f - attackCoeff_) * inputLeveldB;
        } else {
            // Release phase
            peakEnv_ = releaseCoeff_ * peakEnv_ + (1.0f - releaseCoeff_) * inputLeveldB;
        }
        
        // Compute gain reduction based on envelope
        float gain = computeGain(peakEnv_);
        
        // Apply gain to output
        buffer[i] = inputL * gain;
        buffer[i + 1] = inputR * gain;
    }
}

void Compressor::setParameter(const std::string& name, float value) {
    bool recalculate = false;
    
    if (name == "threshold") {
        threshold_ = clamp(value, -60.0f, 0.0f);
    }
    else if (name == "ratio") {
        ratio_ = clamp(value, 1.0f, 20.0f);
    }
    else if (name == "attack") {
        attack_ = clamp(value, 0.001f, 1.0f);
        recalculate = true;
    }
    else if (name == "release") {
        release_ = clamp(value, 0.01f, 3.0f);
        recalculate = true;
    }
    else if (name == "makeup") {
        makeup_ = clamp(value, 0.0f, 24.0f);
    }
    else if (name == "knee") {
        knee_ = clamp(value, 0.0f, 24.0f);
    }
    
    if (recalculate) {
        calculateCoefficients();
    }
}

float Compressor::getParameter(const std::string& name) const {
    if (name == "threshold") {
        return threshold_;
    }
    else if (name == "ratio") {
        return ratio_;
    }
    else if (name == "attack") {
        return attack_;
    }
    else if (name == "release") {
        return release_;
    }
    else if (name == "makeup") {
        return makeup_;
    }
    else if (name == "knee") {
        return knee_;
    }
    return 0.0f;
}

} // namespace AIMusicHardware