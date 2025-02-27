#include "../../include/effects/Distortion.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

Distortion::Distortion(int sampleRate)
    : Effect(sampleRate),
      drive_(2.0f),    // Moderate drive
      level_(0.7f),    // 70% output level to avoid excessive volume
      tone_(0.5f),     // Neutral tone
      mix_(1.0f),      // 100% wet
      type_(Type::Soft) {
    
    // Initialize tone filter
    toneFilter_[0] = toneFilter_[1] = 0.0f;
}

Distortion::~Distortion() {
}

void Distortion::process(float* buffer, int numFrames) {
    // Calculate tone filter coefficient based on tone control
    // Higher values emphasize highs, lower values emphasize lows
    float toneCoeff = 0.1f + 0.8f * tone_; // Range from 0.1 to 0.9
    
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Get input samples
        float inputL = buffer[i];
        float inputR = buffer[i + 1];
        
        // Apply pre-gain (drive)
        float preGainL = inputL * drive_;
        float preGainR = inputR * drive_;
        
        // Apply distortion based on selected type
        float distortedL, distortedR;
        
        switch (type_) {
            case Type::Soft:
                distortedL = softClip(preGainL);
                distortedR = softClip(preGainR);
                break;
                
            case Type::Hard:
                distortedL = hardClip(preGainL);
                distortedR = hardClip(preGainR);
                break;
                
            case Type::Fuzz:
                distortedL = fuzzClip(preGainL);
                distortedR = fuzzClip(preGainR);
                break;
                
            case Type::Tube:
                distortedL = tubeClip(preGainL);
                distortedR = tubeClip(preGainR);
                break;
                
            default:
                distortedL = preGainL;
                distortedR = preGainR;
        }
        
        // Apply tone control (simple one-pole low-pass/high-pass blend)
        // This creates a tilt EQ effect - boosting highs cuts lows and vice versa
        toneFilter_[0] = toneFilter_[0] * (1.0f - toneCoeff) + distortedL * toneCoeff;
        toneFilter_[1] = toneFilter_[1] * (1.0f - toneCoeff) + distortedR * toneCoeff;
        
        float toneL = toneFilter_[0] * tone_ + distortedL * (1.0f - tone_);
        float toneR = toneFilter_[1] * tone_ + distortedR * (1.0f - tone_);
        
        // Apply level control
        toneL *= level_;
        toneR *= level_;
        
        // Mix dry and wet signals
        buffer[i] = inputL * (1.0f - mix_) + toneL * mix_;
        buffer[i + 1] = inputR * (1.0f - mix_) + toneR * mix_;
    }
}

float Distortion::softClip(float input) const {
    // Soft clipping using tanh
    return std::tanh(input);
}

float Distortion::hardClip(float input) const {
    // Hard clipping
    return clamp(input, -1.0f, 1.0f);
}

float Distortion::fuzzClip(float input) const {
    // Fuzz-like distortion (more aggressive)
    float sign = (input > 0.0f) ? 1.0f : -1.0f;
    float absInput = std::abs(input);
    
    // Shaped response curve for fuzz-like effect
    float output = 1.0f - std::exp(-absInput * 3.0f);
    
    return sign * output;
}

float Distortion::tubeClip(float input) const {
    // Tube-like saturation with asymmetric response
    // Different behavior for positive and negative signals
    if (input > 0.0f) {
        return 1.0f - std::exp(-input);
    } else {
        return -1.0f + std::exp(input * 0.5f);
    }
}

void Distortion::setParameter(const std::string& name, float value) {
    if (name == "drive") {
        drive_ = clamp(value, 1.0f, 20.0f);
    }
    else if (name == "level") {
        level_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "tone") {
        tone_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "mix") {
        mix_ = clamp(value, 0.0f, 1.0f);
    }
    else if (name == "type") {
        int typeInt = static_cast<int>(value);
        if (typeInt >= 0 && typeInt <= 3) {
            type_ = static_cast<Type>(typeInt);
        }
    }
}

float Distortion::getParameter(const std::string& name) const {
    if (name == "drive") {
        return drive_;
    }
    else if (name == "level") {
        return level_;
    }
    else if (name == "tone") {
        return tone_;
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