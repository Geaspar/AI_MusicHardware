#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class Saturation : public Effect {
public:
    enum class Type {
        Soft,     // Soft saturation using tanh
        Tube,     // Tube-like saturation (asymmetric)
        Tape,     // Tape-like saturation
        Analog    // Analog-style saturation with harmonics
    };
    
    Saturation(int sampleRate = 44100);
    ~Saturation() override;
    
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "Saturation"; }
    
private:
    // Saturation functions
    float softSaturate(float input) const;
    float tubeSaturate(float input) const;
    float tapeSaturate(float input) const;
    float analogSaturate(float input) const;
    
    // Calculate tone filter coefficients
    void calculateToneCoefficients();
    
    float drive_;      // Drive amount
    float tone_;       // Tone control (0-1)
    float mix_;        // Wet/dry mix
    Type type_;        // Saturation type
    
    // Tone control filter coefficients (high shelf)
    float b0_, b1_, a1_;
    float x1_[2], y1_[2]; // Filter state for left and right channels
};

} // namespace AIMusicHardware