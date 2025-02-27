#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class EQ : public Effect {
public:
    EQ(int sampleRate = 44100);
    ~EQ() override;
    
    void setSampleRate(int sampleRate) override;
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "EQ"; }
    
private:
    void calculateCoefficients();
    
    // EQ bands
    float lowGain_;      // Gain for low frequencies (dB)
    float midGain_;      // Gain for mid frequencies (dB)
    float highGain_;     // Gain for high frequencies (dB)
    
    // Crossover frequencies
    float lowFreq_;      // Crossover frequency between low and mid (Hz)
    float highFreq_;     // Crossover frequency between mid and high (Hz)
    
    // Low-pass filter (for low band)
    float b0_low_, b1_low_, b2_low_, a1_low_, a2_low_;
    float lowpass_x1_[2], lowpass_x2_[2], lowpass_y1_[2], lowpass_y2_[2];
    
    // High-pass filter (for high band)
    float b0_high_, b1_high_, b2_high_, a1_high_, a2_high_;
    float highpass_x1_[2], highpass_x2_[2], highpass_y1_[2], highpass_y2_[2];
};

} // namespace AIMusicHardware