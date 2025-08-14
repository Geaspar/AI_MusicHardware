#include "../../include/effects/FDNReverb.h"
#include "../../include/effects/EffectUtils.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

FDNReverb::FDNReverb(int sampleRate) : Effect(sampleRate), er_(sampleRate),
    inDiffL1_(sampleRate), inDiffL2_(sampleRate), inDiffR1_(sampleRate), inDiffR2_(sampleRate) {
    parameters_["mix"] = mix_;
    parameters_["predelay_ms"] = 0.0f;
    parameters_["er_level"] = 0.0f;
    parameters_["er_width"] = 1.0f;
    parameters_["decay_rt60_s"] = 1.5f;
    parameters_["diffusion"] = 0.5f;     // maps to allpass gain (0.3..0.75)
    parameters_["mod_rate"] = 0.15f;     // Hz
    parameters_["mod_depth"] = 0.2f;     // % of delay -> map to samples
}

FDNReverb::~FDNReverb() = default;

void FDNReverb::process(float* buffer, int numFrames) {
    const float wet = clamp(mix_, 0.0f, 1.0f);
    const float dry = 1.0f - wet;

    // Phase 1: apply ER
    er_.setPredelayMs(parameters_.at("predelay_ms"));
    er_.setLevel(parameters_.at("er_level"));
    er_.setWidth(parameters_.at("er_width"));
    er_.process(buffer, numFrames);

    // Phase 1: input diffusers (process in-place L/R separately)
    const float diffusion = parameters_.at("diffusion");
    const float apGain = 0.3f + diffusion * (0.75f - 0.3f);
    const float modRate = parameters_.at("mod_rate");
    const float modDepthPct = parameters_.at("mod_depth");
    // Base delay lengths in samples for diffusers (~3ms, ~7ms)
    const float d1 = 0.003f * static_cast<float>(getSampleRate());
    const float d2 = 0.007f * static_cast<float>(getSampleRate());
    const float depthSamples1 = (modDepthPct * 0.01f) * d1;
    const float depthSamples2 = (modDepthPct * 0.01f) * d2;

    inDiffL1_.setGain(apGain); inDiffL2_.setGain(apGain);
    inDiffR1_.setGain(apGain); inDiffR2_.setGain(apGain);
    inDiffL1_.setDelaySamples(d1); inDiffL2_.setDelaySamples(d2);
    inDiffR1_.setDelaySamples(d1 * 1.1f); inDiffR2_.setDelaySamples(d2 * 0.9f);
    inDiffL1_.setModulation(modRate, depthSamples1);
    inDiffL2_.setModulation(modRate * 0.7f, depthSamples2);
    inDiffR1_.setModulation(modRate * 1.1f, depthSamples1);
    inDiffR2_.setModulation(modRate * 0.9f, depthSamples2);

    for (int i = 0; i < numFrames * 2; i += 2) {
        float l = buffer[i];
        float r = buffer[i + 1];
        l = inDiffL2_.processSample(inDiffL1_.processSample(l));
        r = inDiffR2_.processSample(inDiffR1_.processSample(r));
        buffer[i] = l;
        buffer[i + 1] = r;
    }

    // Phase 2: simple FDN-8 scaffold (no matrix yet): sum diffused input into a bank of delays
    ensureFdnCapacity();
    const float decayS = std::max(0.2f, parameters_.at("decay_rt60_s"));
    // Householder shortcut precompute u dot x factor will be added later

    for (int n = 0; n < numFrames; ++n) {
        float xinL = buffer[2*n + 0];
        float xinR = buffer[2*n + 1];
        const float xin = 0.5f * (xinL + xinR);

        // Read all delay lines (with modulation), apply HF damping, and compute per-loop gains
        float y[kNumDelays_];
        float z[kNumDelays_];
        float sumZ = 0.0f;
        for (int k = 0; k < kNumDelays_; ++k) {
            auto& buf = fdnBuffer_[k];
            const size_t w = fdnWriteIndex_[k];

            const float size = 1.0f; // placeholder for future Size control
            const float baseDelaySamples = (fdnBaseDelayMs_[k] * size) * (static_cast<float>(getSampleRate())/1000.0f);
            const float lfoRate = std::max(0.05f, parameters_.at("mod_rate"));
            const float lfoDepth = std::max(0.0f, parameters_.at("mod_depth")) * 0.01f; // percent
            fdnLfoPhase_[k] += (2.0f * 3.14159265358979323846f) * (lfoRate / static_cast<float>(getSampleRate()));
            if (fdnLfoPhase_[k] > 2.0f * 3.14159265358979323846f) fdnLfoPhase_[k] -= 2.0f * 3.14159265358979323846f;
            const float modSamples = baseDelaySamples * lfoDepth * std::sin(fdnLfoPhase_[k]);
            float readIndex = static_cast<float>(w) - (baseDelaySamples + modSamples);
            while (readIndex < 0.0f) readIndex += static_cast<float>(buf.size());
            const float yk = readFrac(buf, readIndex);

            // HF damping (one-pole LP) placeholder controlled by high_damping (0..1)
            const float highDamp = 0.2f + 0.6f * std::clamp(parameters_.count("high_damping") ? parameters_.at("high_damping") : 0.3f, 0.0f, 1.0f);
            fdnLpA_[k] = highDamp;
            fdnLpY_[k] = fdnLpA_[k] * fdnLpY_[k] + (1.0f - fdnLpA_[k]) * yk;

            // RT60 gain per loop (Schroeder approximation)
            const float Td = baseDelaySamples / static_cast<float>(getSampleRate());
            const float gLoop = std::pow(10.0f, -3.0f * (Td / decayS));

            y[k] = yk;
            z[k] = gLoop * fdnLpY_[k];
            sumZ += z[k];
        }

        // Householder feedback mixing: A = I - 2*u*u^T, u_i=1/sqrt(N)
        const float twoOverN = 2.0f / static_cast<float>(kNumDelays_);
        for (int k = 0; k < kNumDelays_; ++k) {
            auto& buf = fdnBuffer_[k];
            size_t w = fdnWriteIndex_[k];
            const float mixed = z[k] - twoOverN * sumZ; // since u_i = 1/sqrt(N)
            const float writeVal = xin + mixed;
            buf[w] = writeVal + 1e-20f; // denormal guard
            fdnWriteIndex_[k] = (w + 1) % buf.size();
        }

        // Output: average of current y (pre-feedback) as wet contribution
        float fdnOut = 0.0f;
        for (int k = 0; k < kNumDelays_; ++k) fdnOut += y[k];
        fdnOut /= static_cast<float>(kNumDelays_);

        const float outL = dry * xinL + wet * fdnOut;
        const float outR = dry * xinR + wet * fdnOut;
        buffer[2*n + 0] = outL;
        buffer[2*n + 1] = outR;
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

void FDNReverb::ensureFdnCapacity() {
    // Allocate buffers based on base delays and sample rate (~2x margin)
    for (int k = 0; k < kNumDelays_; ++k) {
        const float baseDelaySamples = (fdnBaseDelayMs_[k]) * (static_cast<float>(getSampleRate())/1000.0f);
        size_t need = static_cast<size_t>(std::ceil(baseDelaySamples * 2.5f));
        need = std::max<size_t>(need, 64);
        if (fdnBuffer_[k].size() != need) {
            fdnBuffer_[k].assign(need, 0.0f);
            fdnWriteIndex_[k] = 0;
            fdnLpY_[k] = 0.0f;
            fdnLfoPhase_[k] = 0.0f;
        }
    }
}

inline float FDNReverb::readFrac(const std::vector<float>& buf, float index) const {
    const size_t size = buf.size();
    int i0 = static_cast<int>(std::floor(index)) % static_cast<int>(size);
    if (i0 < 0) i0 += static_cast<int>(size);
    int i1 = i0 + 1;
    if (i1 >= static_cast<int>(size)) i1 = 0;
    const float frac = index - std::floor(index);
    return buf[static_cast<size_t>(i0)] * (1.0f - frac) + buf[static_cast<size_t>(i1)] * frac;
}

} // namespace AIMusicHardware
