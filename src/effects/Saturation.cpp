#include "../../include/effects/Saturation.h"
#include "../../include/effects/EffectUtils.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

Saturation::Saturation(int sampleRate)
    : Effect(sampleRate),
      drive_(2.0f),    // Drive amount
      tone_(0.5f),     // Tone control (high shelf)
      mix_(1.0f),      // 100% wet
      type_(Type::Soft) {
    
    // Initialize high shelf filter for tone control
    calculateToneCoefficients();
    
    // Initialize filter state
    for (int i = 0; i < 2; ++i) {
        x1_[i] = 0.0f;
        y1_[i] = 0.0f;
    }
}

Saturation::~Saturation() {
}

void Saturation::process(float* buffer, int numFrames) {
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Get input samples
        float inputL = buffer[i];
        float inputR = buffer[i + 1];
        
        // Apply drive (pre-gain)
        float drivenL = inputL * drive_;
        float drivenR = inputR * drive_;
        
        // Apply saturation based on type
        float saturatedL, saturatedR;
        
        switch (type_) {
            case Type::Soft:
                saturatedL = softSaturate(drivenL);
                saturatedR = softSaturate(drivenR);
                break;
                
            case Type::Tube:
                saturatedL = tubeSaturate(drivenL);
                saturatedR = tubeSaturate(drivenR);
                break;
                
            case Type::Tape:
                saturatedL = tapeSaturate(drivenL);
                saturatedR = tapeSaturate(drivenR);
                break;
                
            case Type::Analog:
                saturatedL = analogSaturate(drivenL);
                saturatedR = analogSaturate(drivenR);
                break;
                
            default:
                saturatedL = drivenL;
                saturatedR = drivenR;
        }
        
        // Apply tone control (high shelf filter)
        // Process left channel
        float toneL = b0_ * saturatedL + b1_ * x1_[0] - a1_ * y1_[0];
        x1_[0] = saturatedL;
        y1_[0] = toneL;
        
        // Process right channel
        float toneR = b0_ * saturatedR + b1_ * x1_[1] - a1_ * y1_[1];
        x1_[1] = saturatedR;
        y1_[1] = toneR;
        
        // Apply compensation gain to avoid clipping
        float compensationGain = 1.0f / (0.5f + 0.5f * drive_);
        toneL *= compensationGain;
        toneR *= compensationGain;
        
        // Mix dry and wet signals
        buffer[i] = inputL * (1.0f - mix_) + toneL * mix_;
        buffer[i + 1] = inputR * (1.0f - mix_) + toneR * mix_;
    }
}

float Saturation::softSaturate(float input) const {
    // Simple tanh-based soft clipping
    return std::tanh(input);
}

float Saturation::tubeSaturate(float input) const {
    // Tube-like saturation with asymmetric response
    if (input > 0.0f) {
        return 1.0f - std::exp(-input);
    } else {
        return -1.0f + std::exp(input * 0.5f);
    }
}

float Saturation::tapeSaturate(float input) const {
    // Tape-like saturation with soft compression
    if (input > 0.0f) {
        return input / (1.0f + input);
    } else {
        return input / (1.0f - input);
    }
}

float Saturation::analogSaturate(float input) const {
    // Analog-style saturation with harmonics
    float sign = (input > 0.0f) ? 1.0f : -1.0f;
    float abs_input = std::abs(input);
    
    // Apply cubic soft clipping (adds odd harmonics)
    if (abs_input <= 1.0f/3.0f) {
        return input * 2.0f;
    } else if (abs_input <= 2.0f/3.0f) {
        return sign * (3.0f - (2.0f - 3.0f * abs_input) * (2.0f - 3.0f * abs_input)) / 3.0f;
    } else {
        return sign;
    }
}

void Saturation::calculateToneCoefficients() {
    // High shelf filter parameters
    float frequency = 2000.0f; // Fixed shelf frequency at 2kHz
    float gain = -15.0f + 30.0f * tone_; // -15dB to +15dB range based on tone param
    
    // Convert gain to linear
    float A = std::pow(10.0f, gain / 40.0f); // Divide by 40 instead of 20 for shelving filter
    
    // Compute filter coefficients
    float w0 = 2.0f * PI * frequency / sampleRate_;
    float cosw0 = std::cos(w0);
    float alpha = std::sin(w0) * 0.5f;
    
    float beta = std::sqrt(A) / 1.414f;
    
    // Compute high shelf coefficients
    float b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + beta * std::sin(w0));
    float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
    float b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - beta * std::sin(w0));
    float a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + beta * std::sin(w0);
    float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
    float a2 = (A + 1.0f) - (A - 1.0f) * cosw0 - beta * std::sin(w0);
    
    // Normalize coefficients by a0
    b0_ = b0 / a0;
    b1_ = b1 / a0;
    a1_ = a1 / a0;
    
    // We're using a simplified first-order filter, so b2 and a2 aren't used
}

void Saturation::setParameter(const std::string& name, float value) {
    if (name == "drive") {
        drive_ = clamp(value, 1.0f, 20.0f);
    }
    else if (name == "tone") {
        tone_ = clamp(value, 0.0f, 1.0f);
        calculateToneCoefficients();
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

float Saturation::getParameter(const std::string& name) const {
    if (name == "drive") {
        return drive_;
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