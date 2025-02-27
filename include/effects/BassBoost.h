#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class BassBoost : public Effect {
public:
    BassBoost(int sampleRate = 44100);
    ~BassBoost() override;
    
    void process(float* buffer, int numFrames) override;
    void setSampleRate(int sampleRate) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "BassBoost"; }
    
private:
    void calculateCoefficients();
    
    float frequency_;  // Center frequency for boost
    float gain_;       // Boost amount in dB
    float width_;      // Width of the boost (inverse of Q)
    float drive_;      // Additional drive for harmonics
    
    // Biquad filter coefficients
    float b0_, b1_, b2_, a1_, a2_;
    
    // Filter state
    float x1_[2], x2_[2], y1_[2], y2_[2]; // Left and right channels
};

} // namespace AIMusicHardware