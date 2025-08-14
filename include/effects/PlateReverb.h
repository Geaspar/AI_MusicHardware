#pragma once

#include "EffectProcessor.h"
#include <map>
#include <string>

namespace AIMusicHardware {

class PlateReverb : public Effect {
public:
    explicit PlateReverb(int sampleRate = 44100);
    ~PlateReverb() override;

    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "PlateReverb"; }

private:
    std::map<std::string, float> parameters_;
    float mix_ = 0.25f;
};

} // namespace AIMusicHardware
