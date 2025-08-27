#pragma once

#include "EffectProcessor.h"
#include <string>

namespace AIMusicHardware {

class RingModulator : public Effect {
public:
    RingModulator(int sampleRate = 44100);
    ~RingModulator() override;

    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "RingModulator"; }
    void setSampleRate(int sampleRate) override;

private:
    float freqHz_ = 30.0f;        // Carrier frequency
    float depth_ = 1.0f;          // 0..1 amount of modulation
    float mix_ = 0.5f;            // 0..1
    float stereoPhaseDeg_ = 90.0f;// 0..180 deg offset for right channel

    float phase_ = 0.0f;          // 0..1
};

} // namespace AIMusicHardware

