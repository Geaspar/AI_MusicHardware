#include "../../include/effects/Phaser.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

Phaser::Phaser(int sampleRate)
    : Effect(sampleRate),
      rate_(0.5f),         // 0.5 Hz, completes cycle every 2 seconds
      depth_(0.7f),        // 70% depth
      feedback_(0.7f),     // 70% feedback
      mix_(0.5f),          // 50% wet/dry
      stages_(6),          // 6 allpass stages
      lfoPhase_(0.0f) {
    
    // Initialize all-pass filters
    for (int i = 0; i < MAX_STAGES * 2; ++i) {
        allpassFilters_[i] = 0.0f;
    }
    
    for (int i = 0; i < MAX_STAGES; ++i) {
        allpassCoeffs_[i] = 0.0f;
    }
    
    // Set min/max filter frequencies
    minFreq_ = 200.0f;
    maxFreq_ = 2000.0f;
    
    // Initialize filter buffer
    for (int i = 0; i < 2; ++i) {  // stereo
        lastOutput_[i] = 0.0f;
    }
}

Phaser::~Phaser() {
}

void Phaser::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
    // No additional action needed for sample rate change
}

void Phaser::process(float* buffer, int numFrames) {
    // Process phaser effect
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Calculate LFO value (sine wave)
        float lfoValue = 0.5f + 0.5f * std::sin(lfoPhase_ * TWO_PI);
        
        // Update LFO phase
        lfoPhase_ += rate_ / sampleRate_;
        if (lfoPhase_ >= 1.0f) {
            lfoPhase_ -= 1.0f;
        }
        
        // Map LFO value to frequency range
        float frequency = minFreq_ + (maxFreq_ - minFreq_) * lfoValue * depth_;
        
        // Calculate allpass coefficients for this frequency
        for (int j = 0; j < stages_; ++j) {
            // Distribute filters evenly across frequency range
            float filterFreq = frequency * (1.0f + 0.25f * j);
            
            // Limit frequency to prevent tan() from producing extreme values
            filterFreq = std::min(filterFreq, sampleRate_ * 0.49f); // Below Nyquist
            
            // Convert frequency to coefficient for allpass filter
            // alpha = tan(pi * f/fs)
            float alpha = std::tan(PI * filterFreq / sampleRate_);
            
            // Clamp alpha to prevent extreme coefficients
            alpha = clamp(alpha, -10.0f, 10.0f);
            
            allpassCoeffs_[j] = (alpha - 1.0f) / (alpha + 1.0f);
        }
        
        // Process left channel
        float inputL = buffer[i];
        float outputL = inputL;
        
        // Apply feedback from previous output
        outputL += feedback_ * lastOutput_[0];
        
        // Apply allpass filters in series
        for (int j = 0; j < stages_; ++j) {
            float temp = allpassFilters_[j];
            float newOut = allpassCoeffs_[j] * outputL + temp;
            allpassFilters_[j] = outputL - allpassCoeffs_[j] * newOut;
            outputL = newOut;
            
            // Prevent runaway oscillation
            if (!std::isfinite(outputL)) {
                outputL = 0.0f;
                allpassFilters_[j] = 0.0f;
            }
        }
        
        // Store output for feedback in next block (clamped)
        lastOutput_[0] = clamp(outputL, -2.0f, 2.0f);
        
        // Mix dry and wet signals
        buffer[i] = inputL * (1.0f - mix_) + outputL * mix_;
        
        // Process right channel (similar to left)
        float inputR = buffer[i + 1];
        float outputR = inputR;
        
        // Apply feedback
        outputR += feedback_ * lastOutput_[1];
        
        // Apply allpass filters in series
        for (int j = 0; j < stages_; ++j) {
            float temp = allpassFilters_[j + stages_];
            float newOut = allpassCoeffs_[j] * outputR + temp;
            allpassFilters_[j + stages_] = outputR - allpassCoeffs_[j] * newOut;
            outputR = newOut;
            
            // Prevent runaway oscillation
            if (!std::isfinite(outputR)) {
                outputR = 0.0f;
                allpassFilters_[j + stages_] = 0.0f;
            }
        }
        
        // Store output for feedback (clamped)
        lastOutput_[1] = clamp(outputR, -2.0f, 2.0f);
        
        // Mix dry and wet signals
        buffer[i + 1] = inputR * (1.0f - mix_) + outputR * mix_;
    }
}

void Phaser::setParameter(const std::string& name, float value) {
    if (name == "rate") {
        rate_ = clamp(value, 0.05f, 10.0f);  // 0.05Hz to 10Hz
    }
    else if (name == "depth") {
        depth_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "feedback") {
        feedback_ = clamp(value, 0.0f, 0.9f);  // Limit to avoid instability
    }
    else if (name == "mix") {
        mix_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "stages") {
        stages_ = static_cast<int>(clamp(value, 2.0f, static_cast<float>(MAX_STAGES)));
    }
}

float Phaser::getParameter(const std::string& name) const {
    if (name == "rate") {
        return rate_;
    }
    else if (name == "depth") {
        return depth_;
    }
    else if (name == "feedback") {
        return feedback_;
    }
    else if (name == "mix") {
        return mix_;
    }
    else if (name == "stages") {
        return static_cast<float>(stages_);
    }
    return 0.0f;
}

} // namespace AIMusicHardware