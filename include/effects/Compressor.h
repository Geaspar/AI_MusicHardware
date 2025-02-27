#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class Compressor : public Effect {
public:
    Compressor(int sampleRate = 44100);
    ~Compressor() override;
    
    void setSampleRate(int sampleRate) override;
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "Compressor"; }
    
private:
    void calculateCoefficients();
    float computeGain(float inputLevel);
    
    float threshold_;  // Threshold in dB
    float ratio_;      // Compression ratio (1:n)
    float attack_;     // Attack time in seconds
    float release_;    // Release time in seconds
    float makeup_;     // Makeup gain in dB
    float knee_;       // Knee width in dB
    
    // Smoothing coefficients for envelope follower
    float attackCoeff_;
    float releaseCoeff_;
    
    // Envelope follower state
    float envelope_[2]; // Left and right channels
    
    // Peak detector
    float peakEnv_;
};

} // namespace AIMusicHardware