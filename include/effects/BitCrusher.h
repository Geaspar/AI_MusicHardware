#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class BitCrusher : public Effect {
public:
    BitCrusher(int sampleRate = 44100);
    ~BitCrusher() override;
    
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "BitCrusher"; }
    
private:
    float bitDepth_;             // Bit depth for quantization (1-16)
    float sampleRateReduction_;  // Sample rate reduction factor (0-1)
    float mix_;                  // Wet/dry mix (0-1)
    float drive_;                // Input drive (0-1)
    
    int holdCounter_;            // Counter for sample rate reduction
    float holdL_;                // Last sampled value (left)
    float holdR_;                // Last sampled value (right)
};

} // namespace AIMusicHardware