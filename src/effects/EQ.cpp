#include "../../include/effects/EQ.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

EQ::EQ(int sampleRate)
    : Effect(sampleRate),
      lowGain_(0.0f),     // 0 dB (no change)
      midGain_(0.0f),     // 0 dB (no change)
      highGain_(0.0f),    // 0 dB (no change)
      lowFreq_(200.0f),   // Typical crossover point
      highFreq_(2000.0f) { // Typical crossover point
    
    // Initialize filter state
    for (int i = 0; i < 2; ++i) {  // stereo
        lowpass_x1_[i] = lowpass_x2_[i] = lowpass_y1_[i] = lowpass_y2_[i] = 0.0f;
        highpass_x1_[i] = highpass_x2_[i] = highpass_y1_[i] = highpass_y2_[i] = 0.0f;
    }
    
    // Calculate initial coefficients
    calculateCoefficients();
}

EQ::~EQ() {
}

void EQ::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
    calculateCoefficients();
}

void EQ::calculateCoefficients() {
    // Calculate low-pass filter coefficients
    float omega = 2.0f * PI * lowFreq_ / sampleRate_;
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);
    float alpha = sinOmega / (2.0f * 0.707f); // Q = 0.707 (Butterworth)
    
    b0_low_ = (1.0f - cosOmega) / 2.0f;
    b1_low_ = 1.0f - cosOmega;
    b2_low_ = (1.0f - cosOmega) / 2.0f;
    float a0_low = 1.0f + alpha;
    a1_low_ = -2.0f * cosOmega;
    a2_low_ = 1.0f - alpha;
    
    // Normalize coefficients
    b0_low_ /= a0_low;
    b1_low_ /= a0_low;
    b2_low_ /= a0_low;
    a1_low_ /= a0_low;
    a2_low_ /= a0_low;
    
    // Calculate high-pass filter coefficients
    omega = 2.0f * PI * highFreq_ / sampleRate_;
    sinOmega = std::sin(omega);
    cosOmega = std::cos(omega);
    alpha = sinOmega / (2.0f * 0.707f); // Q = 0.707 (Butterworth)
    
    b0_high_ = (1.0f + cosOmega) / 2.0f;
    b1_high_ = -(1.0f + cosOmega);
    b2_high_ = (1.0f + cosOmega) / 2.0f;
    float a0_high = 1.0f + alpha;
    a1_high_ = -2.0f * cosOmega;
    a2_high_ = 1.0f - alpha;
    
    // Normalize coefficients
    b0_high_ /= a0_high;
    b1_high_ /= a0_high;
    b2_high_ /= a0_high;
    a1_high_ /= a0_high;
    a2_high_ /= a0_high;
}

void EQ::process(float* buffer, int numFrames) {
    // Convert dB gains to linear
    float lowGainLinear = dbToGain(lowGain_);
    float midGainLinear = dbToGain(midGain_);
    float highGainLinear = dbToGain(highGain_);
    
    for (int i = 0; i < numFrames * 2; i += 2) {
        // Process stereo samples
        for (int ch = 0; ch < 2; ++ch) {
            int idx = i + ch; // Current sample index
            
            float input = buffer[idx];
            
            // Process through low-pass filter (for low band)
            float lowBand = b0_low_ * input + b1_low_ * lowpass_x1_[ch] + b2_low_ * lowpass_x2_[ch] - 
                           a1_low_ * lowpass_y1_[ch] - a2_low_ * lowpass_y2_[ch];
            
            // Update low-pass state
            lowpass_x2_[ch] = lowpass_x1_[ch];
            lowpass_x1_[ch] = input;
            lowpass_y2_[ch] = lowpass_y1_[ch];
            lowpass_y1_[ch] = lowBand;
            
            // Process through high-pass filter (for high band)
            float highBand = b0_high_ * input + b1_high_ * highpass_x1_[ch] + b2_high_ * highpass_x2_[ch] - 
                            a1_high_ * highpass_y1_[ch] - a2_high_ * highpass_y2_[ch];
            
            // Update high-pass state
            highpass_x2_[ch] = highpass_x1_[ch];
            highpass_x1_[ch] = input;
            highpass_y2_[ch] = highpass_y1_[ch];
            highpass_y1_[ch] = highBand;
            
            // Mid band = input - (low + high)
            float midBand = input - (lowBand + highBand);
            
            // Apply gains and sum bands
            buffer[idx] = lowBand * lowGainLinear + midBand * midGainLinear + highBand * highGainLinear;
        }
    }
}

void EQ::setParameter(const std::string& name, float value) {
    bool recalculate = false;
    
    if (name == "lowGain") {
        lowGain_ = clamp(value, -24.0f, 24.0f); // -24dB to +24dB
    }
    else if (name == "midGain") {
        midGain_ = clamp(value, -24.0f, 24.0f);
    }
    else if (name == "highGain") {
        highGain_ = clamp(value, -24.0f, 24.0f);
    }
    else if (name == "lowFreq") {
        lowFreq_ = clamp(value, 20.0f, 1000.0f);
        recalculate = true;
    }
    else if (name == "highFreq") {
        highFreq_ = clamp(value, 1000.0f, 20000.0f);
        recalculate = true;
    }
    
    if (recalculate) {
        calculateCoefficients();
    }
}

float EQ::getParameter(const std::string& name) const {
    if (name == "lowGain") {
        return lowGain_;
    }
    else if (name == "midGain") {
        return midGain_;
    }
    else if (name == "highGain") {
        return highGain_;
    }
    else if (name == "lowFreq") {
        return lowFreq_;
    }
    else if (name == "highFreq") {
        return highFreq_;
    }
    return 0.0f;
}

} // namespace AIMusicHardware