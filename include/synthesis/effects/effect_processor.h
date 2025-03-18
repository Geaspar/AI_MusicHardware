#pragma once

#include "../framework/processor.h"
#include <string>
#include <memory>
#include <vector>

namespace AIMusicHardware {

/**
 * Base class for audio effects using the new architecture
 */
class EffectProcessor : public Processor {
public:
    EffectProcessor(int sampleRate = 44100) 
        : Processor(sampleRate) {
    }
    
    virtual ~EffectProcessor() = default;
    
    // Processor interface
    virtual void process(float* buffer, int numFrames) override = 0;
    virtual std::string getName() const override = 0;
    
    // Get wet/dry mix level (0-1)
    float getMix() const { return mix_; }
    
    // Set wet/dry mix level (0-1)
    void setMix(float mix) { mix_ = std::clamp(mix, 0.0f, 1.0f); }
    
protected:
    float mix_ = 1.0f;  // Wet/dry mix (0=dry, 1=wet)
    
    // Helper to mix wet and dry signals
    void mixWetDry(float* buffer, float* wetBuffer, int numFrames) {
        if (mix_ >= 0.999f) {
            // Full wet - just copy
            std::copy(wetBuffer, wetBuffer + numFrames * 2, buffer);
        }
        else if (mix_ <= 0.001f) {
            // Full dry - do nothing
            return;
        }
        else {
            // Mix wet and dry
            for (int i = 0; i < numFrames * 2; ++i) {
                buffer[i] = buffer[i] * (1.0f - mix_) + wetBuffer[i] * mix_;
            }
        }
    }
};

} // namespace AIMusicHardware