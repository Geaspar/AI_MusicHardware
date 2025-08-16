#include "../../include/effects/PlateReverb.h"
#include "../../include/effects/EffectUtils.h"

namespace AIMusicHardware {

PlateReverb::PlateReverb(int sampleRate) : Effect(sampleRate), er_(sampleRate),
    inDiffL1_(sampleRate), inDiffL2_(sampleRate), inDiffR1_(sampleRate), inDiffR2_(sampleRate) {
    parameters_["mix"] = mix_;
    parameters_["predelay_ms"] = 0.0f;
    parameters_["er_level"] = 0.0f;
    parameters_["er_width"] = 1.0f;
    parameters_["decay_rt60_s"] = 1.5f;
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

    // Phase 2/3: simple plate tank (two delay lines with cross feedback, HF damping, light modulation)
    ensureTankCapacity();

    const float decayS = std::max(0.2f, parameters_.at("decay_rt60_s"));
    const float highDamp = clamp(parameters_.at("high_damping"), 0.0f, 1.0f);
    const float width = clamp(parameters_.at("er_width"), 0.0f, 1.0f);
    const float bassMult = std::max(0.5f, std::min(2.0f, parameters_.at("bass_mult")));
    const float sr = static_cast<float>(getSampleRate());
    const float fc = 2000.0f + highDamp * (12000.0f - 2000.0f);
    const float aLp = std::exp(-(2.0f * 3.14159265358979323846f * fc) / sr);
    const float baseL = 0.0253f * sr; // ~25.3 ms
    const float baseR = 0.0311f * sr; // ~31.1 ms
    const float modRate2 = std::max(0.05f, parameters_.at("mod_rate"));
    const float modDepth = std::max(0.0f, parameters_.at("mod_depth")) * 0.01f;
    const float twoPiOverSr = (2.0f * 3.14159265358979323846f) / sr;

    for (int n = 0; n < numFrames; ++n) {
        float inL = buffer[2*n + 0];
        float inR = buffer[2*n + 1];
        const float xin = 0.5f * (inL + inR);

        // Read current tank outputs with fractional modulation
        lfoPhaseL_ += twoPiOverSr * modRate2;
        if (lfoPhaseL_ > 2.0f * 3.14159265358979323846f) lfoPhaseL_ -= 2.0f * 3.14159265358979323846f;
        lfoPhaseR_ += twoPiOverSr * modRate2 * 1.1f;
        if (lfoPhaseR_ > 2.0f * 3.14159265358979323846f) lfoPhaseR_ -= 2.0f * 3.14159265358979323846f;
        const float delayL = baseL + modDepth * baseL * std::sin(lfoPhaseL_);
        const float delayR = baseR + modDepth * baseR * std::sin(lfoPhaseR_);

        float readIdxL = static_cast<float>(wL_) - delayL;
        while (readIdxL < 0.0f) readIdxL += static_cast<float>(tankL_.size());
        float readIdxR = static_cast<float>(wR_) - delayR;
        while (readIdxR < 0.0f) readIdxR += static_cast<float>(tankR_.size());
        float yL = readFrac(tankL_, readIdxL);
        float yR = readFrac(tankR_, readIdxR);

        // One-pole HF damping inside the tank
        lpYL_ = aLp * lpYL_ + (1.0f - aLp) * yL;
        lpYR_ = aLp * lpYR_ + (1.0f - aLp) * yR;

        // RT60-based loop gain per branch; add slight LF emphasis via bassMult
        const float TdL = delayL / sr;
        const float TdR = delayR / sr;
        float gL = std::pow(10.0f, -3.0f * (TdL / decayS)) * std::pow(bassMult, 0.1f);
        float gR = std::pow(10.0f, -3.0f * (TdR / decayS)) * std::pow(bassMult, 0.1f);

        // Cross-feedback with slight width control
        const float fbL = gL * lpYR_;
        const float fbR = gR * lpYL_;
        const float writeL = xin + (0.5f + 0.5f * width) * fbL + (0.5f - 0.5f * width) * lpYL_;
        const float writeR = xin + (0.5f + 0.5f * width) * fbR + (0.5f - 0.5f * width) * lpYR_;

        tankL_[wL_] = writeL + 1e-20f; // denormal guard
        tankR_[wR_] = writeR + 1e-20f;
        wL_ = (wL_ + 1) % tankL_.size();
        wR_ = (wR_ + 1) % tankR_.size();

        // Wet mix from tank outputs (post-normalization)
        float wetL = lpYL_;
        float wetR = lpYR_;

        // Wet normalization similar to FDN
        const float wetMono = 0.5f * (wetL + wetR);
        wetRms_ = 0.995f * wetRms_ + 0.005f * (wetMono * wetMono);
        const float targetGain = 0.7f / std::sqrt(std::max(wetRms_, 1e-6f));
        wetNormGainSmoothed_ = 0.99f * wetNormGainSmoothed_ + 0.01f * targetGain;
        wetL = std::tanh(wetL * wetNormGainSmoothed_);
        wetR = std::tanh(wetR * wetNormGainSmoothed_);

        buffer[2*n + 0] = dry * inL + wet * wetL;
        buffer[2*n + 1] = dry * inR + wet * wetR;
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

void PlateReverb::ensureTankCapacity() {
    // Allocate modest-length buffers for plate tank
    const float sr = static_cast<float>(getSampleRate());
    size_t needL = static_cast<size_t>(std::ceil(0.040f * sr)); // ~40 ms
    size_t needR = static_cast<size_t>(std::ceil(0.055f * sr)); // ~55 ms
    needL = std::max<size_t>(needL, 64);
    needR = std::max<size_t>(needR, 64);
    if (tankL_.size() != needL) { tankL_.assign(needL, 0.0f); wL_ = 0; lpYL_ = 0.0f; }
    if (tankR_.size() != needR) { tankR_.assign(needR, 0.0f); wR_ = 0; lpYR_ = 0.0f; }
}

inline float PlateReverb::readFrac(const std::vector<float>& buf, float index) const {
    const size_t size = buf.size();
    int i0 = static_cast<int>(std::floor(index)) % static_cast<int>(size);
    if (i0 < 0) i0 += static_cast<int>(size);
    int i1 = i0 + 1; if (i1 >= static_cast<int>(size)) i1 = 0;
    const float frac = index - std::floor(index);
    return buf[static_cast<size_t>(i0)] * (1.0f - frac) + buf[static_cast<size_t>(i1)] * frac;
}

} // namespace AIMusicHardware
