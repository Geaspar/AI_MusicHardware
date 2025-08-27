#pragma once

#include "EffectProcessor.h"
#include <vector>
#include <string>
#include <memory>

namespace AIMusicHardware
{

class PingPongDelay : public Effect
{
public:
    PingPongDelay(int sampleRate);
    ~PingPongDelay() override;
    std::string getName() const override { return "PingPongDelay"; }

    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;

private:
    // Parameters
    float mix_ = 0.5f;
    float timeMs_ = 500.0f;
    float feedback_ = 0.5f;
    float hpFreq_ = 20.0f;
    float lpFreq_ = 20000.0f;
    bool pingPong_ = true;
    float width_ = 1.0f;
    float outputTrimDb_ = 0.0f;

    // Internal state
    std::vector<float> delayBufferL_;
    std::vector<float> delayBufferR_;
    size_t writeIndexL_ = 0;
    size_t writeIndexR_ = 0;
    size_t delaySamples_ = 0;

    // Simple 1-pole HP/LP filters on feedback path
    float hpAlpha_ = 0.0f; // coefficient for HP
    float lpAlpha_ = 0.0f; // coefficient for LP
    // HP states (per channel)
    float hpStateL_ = 0.0f;
    float hpStateR_ = 0.0f;
    float hpPrevInL_ = 0.0f;
    float hpPrevInR_ = 0.0f;
    // LP states (per channel)
    float lpStateL_ = 0.0f;
    float lpStateR_ = 0.0f;

    // Helper functions
    void updateDelayTime();
    float readFromDelay(const std::vector<float>& buffer, size_t index, float fractionalDelay);
    void updateFilterCoeffs();
    void setSampleRate(int sampleRate) override { Effect::setSampleRate(sampleRate); updateFilterCoeffs(); updateDelayTime(); }
};

} // namespace AIMusicHardware
