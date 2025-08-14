#pragma once

#include "EffectProcessor.h"
#include "audio/EarlyReflections.h"
#include "audio/AllpassDiffuser.h"
#include <map>
#include <string>

namespace AIMusicHardware {

class FDNReverb : public Effect {
public:
    explicit FDNReverb(int sampleRate = 44100);
    ~FDNReverb() override;

    void process(float* buffer, int numFrames) override;
    void setSampleRate(int sampleRate) override { Effect::setSampleRate(sampleRate); er_.setSampleRate(sampleRate); }
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
};

} // namespace AIMusicHardware
