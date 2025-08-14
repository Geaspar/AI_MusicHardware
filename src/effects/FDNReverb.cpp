#include "../../include/effects/FDNReverb.h"
#include "../../include/effects/EffectUtils.h"

namespace AIMusicHardware {

FDNReverb::FDNReverb(int sampleRate) : Effect(sampleRate), er_(sampleRate) {
    parameters_["mix"] = mix_;
    parameters_["predelay_ms"] = 0.0f;
    parameters_["er_level"] = 0.0f;
    parameters_["er_width"] = 1.0f;
}

FDNReverb::~FDNReverb() = default;

void FDNReverb::process(float* buffer, int numFrames) {
    const float wet = clamp(mix_, 0.0f, 1.0f);
    const float dry = 1.0f - wet;

    // Phase 1: apply ER (currently pass-through but wired for params)
    er_.setPredelayMs(parameters_.at("predelay_ms"));
    er_.setLevel(parameters_.at("er_level"));
    er_.setWidth(parameters_.at("er_width"));
    er_.process(buffer, numFrames);

    for (int i = 0; i < numFrames * 2; i += 2) {
        const float inL = buffer[i];
        const float inR = buffer[i + 1];
        // Placeholder: wet path equals ER output (currently equal to input)
        const float wetL = inL;
        const float wetR = inR;
        buffer[i]     = dry * inL + wet * wetL;
        buffer[i + 1] = dry * inR + wet * wetR;
    }
}

void FDNReverb::setParameter(const std::string& name, float value) {
    if (name == "mix") mix_ = clamp(value, 0.0f, 1.0f);
    parameters_[name] = value;
}

float FDNReverb::getParameter(const std::string& name) const {
    auto it = parameters_.find(name);
    if (it != parameters_.end()) return it->second;
    return 0.0f;
}

} // namespace AIMusicHardware
