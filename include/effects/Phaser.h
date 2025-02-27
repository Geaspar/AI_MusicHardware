#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class Phaser : public Effect {
public:
    Phaser(int sampleRate = 44100);
    ~Phaser() override;
    
    void setSampleRate(int sampleRate) override;
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "Phaser"; }
    
private:
    float rate_;          // LFO rate in Hz
    float depth_;         // Depth of the effect (0-1)
    float feedback_;      // Feedback amount (0-1)
    float mix_;           // Wet/dry mix (0-1)
    int stages_;          // Number of allpass stages (2-12)
    
    float lfoPhase_;      // Current LFO phase (0-1)
    
    float minFreq_;       // Minimum filter frequency
    float maxFreq_;       // Maximum filter frequency
    
    static const int MAX_STAGES = 12;
    float allpassFilters_[MAX_STAGES * 2];  // Allpass filters for stereo
    float allpassCoeffs_[MAX_STAGES];       // Coefficients for allpass filters
    
    float lastOutput_[2]; // Last output value for feedback (stereo)
};

} // namespace AIMusicHardware