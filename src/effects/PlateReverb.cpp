#include "../../include/effects/PlateReverb.h"
#include "../../include/effects/EffectUtils.h"

namespace AIMusicHardware {

PlateReverb::PlateReverb(int sampleRate) : Effect(sampleRate), er_(sampleRate),
    inDiffL1_(sampleRate), inDiffL2_(sampleRate), inDiffR1_(sampleRate), inDiffR2_(sampleRate) {
    parameters_["mix"] = mix_;
    parameters_["predelay_ms"] = 0.0f;
    parameters_["er_level"] = 0.0f;
    parameters_["er_width"] = 1.0f;
    parameters_["diffusion"] = 0.5f;
    parameters_["mod_rate"] = 0.15f;
    parameters_["mod_depth"] = 0.2f;
}

PlateReverb::~PlateReverb() = default;

void PlateReverb::process(float* buffer, int numFrames) {
    const float wet = clamp(mix_, 0.0f, 1.0f);
    const float dry = 1.0f - wet;

    // Phase 1: apply ER
    er_.setPredelayMs(parameters_.at("predelay_ms"));
    er_.setLevel(parameters_.at("er_level"));
    er_.setWidth(parameters_.at("er_width"));
    er_.process(buffer, numFrames);

    // Phase 1: input diffusers
    const float diffusion = parameters_.at("diffusion");
    const float apGain = 0.3f + diffusion * (0.75f - 0.3f);
    const float modRate = parameters_.at("mod_rate");
    const float modDepthPct = parameters_.at("mod_depth");
    const float d1 = 0.0025f * static_cast<float>(getSampleRate());
    const float d2 = 0.0065f * static_cast<float>(getSampleRate());
    const float depthSamples1 = (modDepthPct * 0.01f) * d1;
    const float depthSamples2 = (modDepthPct * 0.01f) * d2;

    inDiffL1_.setGain(apGain); inDiffL2_.setGain(apGain);
    inDiffR1_.setGain(apGain); inDiffR2_.setGain(apGain);
    inDiffL1_.setDelaySamples(d1); inDiffL2_.setDelaySamples(d2);
    inDiffR1_.setDelaySamples(d1 * 1.07f); inDiffR2_.setDelaySamples(d2 * 0.93f);
    inDiffL1_.setModulation(modRate, depthSamples1);
    inDiffL2_.setModulation(modRate * 0.6f, depthSamples2);
    inDiffR1_.setModulation(modRate * 1.15f, depthSamples1);
    inDiffR2_.setModulation(modRate * 0.85f, depthSamples2);

    for (int i = 0; i < numFrames * 2; i += 2) {
        float l = buffer[i];
        float r = buffer[i + 1];
        l = inDiffL2_.processSample(inDiffL1_.processSample(l));
        r = inDiffR2_.processSample(inDiffR1_.processSample(r));
        buffer[i] = l;
        buffer[i + 1] = r;
    }

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
