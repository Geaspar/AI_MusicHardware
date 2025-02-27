#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class Distortion : public Effect {
public:
    enum class Type {
        Soft,     // Soft clipping (smooth)
        Hard,     // Hard clipping (abrupt)
        Fuzz,     // Fuzz-like distortion (aggressive)
        Tube      // Tube-like saturation (warm, asymmetric)
    };
    
    Distortion(int sampleRate = 44100);
    ~Distortion() override;
    
    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "Distortion"; }
    
private:
    // Distortion curve functions
    float softClip(float input) const;
    float hardClip(float input) const;
    float fuzzClip(float input) const;
    float tubeClip(float input) const;
    
    float drive_;      // Distortion amount (pre-gain)
    float level_;      // Output level (post-gain)
    float tone_;       // Tone control (0-1)
    float mix_;        // Wet/dry mix
    Type type_;        // Distortion type
    
    // Tone filter (simple one-pole filter)
    float toneFilter_[2]; // Left/right channels
};

} // namespace AIMusicHardware