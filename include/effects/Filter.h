#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class Filter : public Effect {
public:
    enum class Type {
        LowPass,
        HighPass,
        BandPass,
        Notch
    };
    
    Filter(int sampleRate = 44100, Type type = Type::LowPass);
    ~Filter() override;
    
    void setSampleRate(int sampleRate) override;
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    
    std::string getName() const override;
    
private:
    void calculateCoefficients();
    
    Type type_;           // Filter type
    float frequency_;     // Cutoff/center frequency (20-20000 Hz)
    float resonance_;     // Resonance (Q factor)
    float gain_;          // Gain for peak/shelf filters (dB)
    float mix_;           // Wet/dry mix
    
    // Biquad filter coefficients
    float a0_, a1_, a2_, b0_, b1_, b2_;
    
    // Filter state variables
    float x1_[2], x2_[2]; // Previous input samples (left and right)
    float y1_[2], y2_[2]; // Previous output samples (left and right)
};

} // namespace AIMusicHardware