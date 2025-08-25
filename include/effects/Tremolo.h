#pragma once

#include "EffectProcessor.h"
#include <string>

namespace AIMusicHardware {

class Tremolo : public Effect {
public:
    Tremolo(int sampleRate = 44100);
    ~Tremolo() override;

    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "Tremolo"; }
    void setSampleRate(int sampleRate) override;

private:
    float rateHz_ = 2.0f;   // LFO rate
    float depth_ = 0.5f;    // 0..1
    int   shape_ = 0;       // 0: sine, 1: triangle, 2: square
    float mix_ = 1.0f;      // 0..1
    float stereoPhaseDeg_ = 0.0f; // 0..180 deg phase offset for R channel

    float lfoPhase_ = 0.0f; // 0..1

    float nextLfoSample();
    float lfoAtPhase(float phase) const; // shape function mapping phase -> 0..1
};

} // namespace AIMusicHardware
