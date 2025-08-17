#pragma once

#include "EffectProcessor.h"
#include "audio/EarlyReflections.h"
#include "audio/AllpassDiffuser.h"
#include <map>
#include <string>
#include <vector>

namespace AIMusicHardware {

class PlateReverb : public Effect {
public:
    explicit PlateReverb(int sampleRate = 44100);
    ~PlateReverb() override;

    void process(float* buffer, int numFrames) override;
    void setSampleRate(int sampleRate) override {
        Effect::setSampleRate(sampleRate);
        er_.setSampleRate(sampleRate);
        ensureTankCapacity();
        // Reset normalization and filters
        wetRms_ = 0.0f;
        wetNormGainSmoothed_ = 1.0f;
        lpYL_ = lpYR_ = 0.0f;
        lfoPhaseL_ = 0.0f; lfoPhaseR_ = 1.2345f;
    }
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "PlateReverb"; }

private:
    std::map<std::string, float> parameters_;
    float mix_ = 0.25f;

    // Phase 1: Early Reflections (scaffolded)
    EarlyReflections er_;

    // Phase 1: Input diffusers (2 stages per channel for now)
    AllpassDiffuser inDiffL1_;
    AllpassDiffuser inDiffL2_;
    AllpassDiffuser inDiffR1_;
    AllpassDiffuser inDiffR2_;

    // Phase 2/3: Plate tank (simplified two-tank network with cross-feedback)
    std::vector<float> tankL_;
    std::vector<float> tankR_;
    size_t wL_ = 0;
    size_t wR_ = 0;
    // One-pole HF damping states
    float lpYL_ = 0.0f;
    float lpYR_ = 0.0f;
    // Modulation phases
    float lfoPhaseL_ = 0.0f;
    float lfoPhaseR_ = 0.0f;
    // Wet normalization
    float wetRms_ = 0.0f;
    float wetNormGainSmoothed_ = 1.0f;
    float outputTrimDb_ = 0.0f;

    void ensureTankCapacity();
    inline float readFrac(const std::vector<float>& buf, float index) const;
};

} // namespace AIMusicHardware
