#pragma once

#include "EffectProcessor.h"
#include "audio/EarlyReflections.h"
#include "audio/AllpassDiffuser.h"
#include <map>
#include <string>
#include <vector>

namespace AIMusicHardware {

class FDNReverb : public Effect {
public:
    explicit FDNReverb(int sampleRate = 44100);
    ~FDNReverb() override;

    void process(float* buffer, int numFrames) override;
    void setSampleRate(int sampleRate) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "FDNReverb (Hall)"; }

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

    // Phase 2: FDN-8 late tail
    static constexpr int kNumDelays_ = 8;
    std::vector<float> fdnBuffer_[kNumDelays_];
    size_t fdnWriteIndex_[kNumDelays_] = {0,0,0,0,0,0,0,0};
    float fdnBaseDelayMs_[kNumDelays_] = {15.3f, 19.7f, 23.1f, 29.9f, 37.1f, 51.7f, 67.9f, 89.7f};
    float fdnLfoPhase_[kNumDelays_] = {0,0,0,0,0,0,0,0};
    float fdnLfoRateMul_[kNumDelays_] = {1,1,1,1,1,1,1,1};
    float fdnLpA_[kNumDelays_] = {0,0,0,0,0,0,0,0};
    float fdnLpY_[kNumDelays_] = {0,0,0,0,0,0,0,0};
    void ensureFdnCapacity(float sizeScale);
    inline float readFrac(const std::vector<float>& buf, float index) const;

    // Smoothed parameters for click-free updates
    float sizeSmoothed_ = 1.0f;
    float decaySmoothed_ = 1.5f;
    float highDampSmoothed_ = 0.3f;
    float bassMultSmoothed_ = 1.0f;
    float widthSmoothed_ = 1.0f;

    // Wet normalization
    float wetRms_ = 0.0f;
    float wetNormGainSmoothed_ = 1.0f;
    // Output trim (dB)
    float outputTrimDb_ = 0.0f;
};

} // namespace AIMusicHardware
