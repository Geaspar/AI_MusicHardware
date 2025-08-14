#include "../../include/effects/PlateReverb.h"
#include "../../include/effects/EffectUtils.h"

namespace AIMusicHardware {

PlateReverb::PlateReverb(int sampleRate) : Effect(sampleRate) {
    parameters_["mix"] = mix_;
}

PlateReverb::~PlateReverb() = default;

void PlateReverb::process(float* buffer, int numFrames) {
    const float wet = clamp(mix_, 0.0f, 1.0f);
    const float dry = 1.0f - wet;
    for (int i = 0; i < numFrames * 2; i += 2) {
        const float inL = buffer[i];
        const float inR = buffer[i + 1];
        // Placeholder: wet path equals dry input
        const float wetL = inL;
        const float wetR = inR;
        buffer[i]     = dry * inL + wet * wetL;
        buffer[i + 1] = dry * inR + wet * wetR;
    }
}

void PlateReverb::setParameter(const std::string& name, float value) {
    if (name == "mix") mix_ = clamp(value, 0.0f, 1.0f);
    parameters_[name] = value;
}

float PlateReverb::getParameter(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end()) return it->second;
    return 0.0f;
}

} // namespace AIMusicHardware
