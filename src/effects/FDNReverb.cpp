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
    parameters_["size"] = 1.0f;
    parameters_["high_damping"] = 0.3f;
    parameters_["bass_mult"] = 1.0f;
    parameters_["stereo_width"] = 1.0f;

    // Randomize per-line LFO phases for decorrelation
    for (int k = 0; k < kNumDelays_; ++k) {
        // Simple deterministic phase seeds
        fdnLfoPhase_[k] = std::fmod(0.37f * (k + 1) * 2.0f * 3.14159265358979323846f, 2.0f * 3.14159265358979323846f);
        fdnLfoRateMul_[k] = 0.9f + 0.2f * ((k % 3) / 2.0f); // 0.9, 1.0, 1.1 pattern
    }
}

FDNReverb::~FDNReverb() = default;

void FDNReverb::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
    er_.setSampleRate(sampleRate);
    // Recompute capacity for current size value
    const float sizeScale = std::clamp(parameters_.count("size") ? parameters_.at("size") : 1.0f, 0.5f, 2.0f);
    ensureFdnCapacity(sizeScale);
    // Reset normalization state
    wetRms_ = 0.0f;
    wetNormGainSmoothed_ = 1.0f;
}

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
    // Smooth key parameters
    const float sizeTarget = std::clamp(parameters_.at("size"), 0.5f, 2.0f);
    const float decayTarget = std::max(0.2f, parameters_.at("decay_rt60_s"));
    const float highDampTarget = std::clamp(parameters_.at("high_damping"), 0.0f, 1.0f);
    const float bassMultTarget = std::clamp(parameters_.at("bass_mult"), 0.5f, 2.0f);
    const float widthTarget = std::clamp(parameters_.at("stereo_width"), 0.0f, 1.0f);

    // Smoothing factors per block (assume ~64-sample blocks typical). Here we approximate with per-call smoothing.
    auto smooth = [](float current, float target, float alpha){ return current + alpha * (target - current); };
    sizeSmoothed_ = smooth(sizeSmoothed_, sizeTarget, 0.1f);
    decaySmoothed_ = smooth(decaySmoothed_, decayTarget, 0.05f);
    highDampSmoothed_ = smooth(highDampSmoothed_, highDampTarget, 0.03f);
    bassMultSmoothed_ = smooth(bassMultSmoothed_, bassMultTarget, 0.03f);
    widthSmoothed_ = smooth(widthSmoothed_, widthTarget, 0.15f);

    ensureFdnCapacity(sizeSmoothed_);
    const float decayS = decaySmoothed_;
    // Householder shortcut precompute u dot x factor will be added later

    // Precompute per-line constants for this block
    float baseDelaySamplesArr[kNumDelays_];
    float gLoopArr[kNumDelays_];
    float aLpArr[kNumDelays_];
    float lfoOmegaArr[kNumDelays_];
    const float twoPiOverSr = (2.0f * 3.14159265358979323846f) / static_cast<float>(getSampleRate());
    for (int k = 0; k < kNumDelays_; ++k) {
        baseDelaySamplesArr[k] = (fdnBaseDelayMs_[k] * sizeSmoothed_) * (static_cast<float>(getSampleRate())/1000.0f);
        const float Td = baseDelaySamplesArr[k] / static_cast<float>(getSampleRate());
        float gLoop = std::pow(10.0f, -3.0f * (Td / decayS));
        gLoop *= std::pow(bassMultSmoothed_, 0.2f);
        gLoopArr[k] = gLoop;
        const float fc = 2000.0f + highDampSmoothed_ * (12000.0f - 2000.0f);
        aLpArr[k] = std::exp(-(2.0f * 3.14159265358979323846f * fc) / static_cast<float>(getSampleRate()));
        const float lfoRate = std::max(0.05f, parameters_.at("mod_rate")) * fdnLfoRateMul_[k];
        lfoOmegaArr[k] = twoPiOverSr * lfoRate;
    }

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

            const float baseDelaySamples = baseDelaySamplesArr[k];
            const float lfoDepth = std::max(0.0f, parameters_.at("mod_depth")) * 0.01f; // percent
            fdnLfoPhase_[k] += lfoOmegaArr[k];
            if (fdnLfoPhase_[k] > 2.0f * 3.14159265358979323846f) fdnLfoPhase_[k] -= 2.0f * 3.14159265358979323846f;
            const float modSamples = baseDelaySamples * lfoDepth * std::sin(fdnLfoPhase_[k]);
            float readIndex = static_cast<float>(w) - (baseDelaySamples + modSamples);
            while (readIndex < 0.0f) readIndex += static_cast<float>(buf.size());
            const float yk = readFrac(buf, readIndex);

            // HF damping (one-pole LP)
            fdnLpA_[k] = aLpArr[k];
            fdnLpY_[k] = fdnLpA_[k] * fdnLpY_[k] + (1.0f - fdnLpA_[k]) * yk;

            y[k] = yk;
            z[k] = gLoopArr[k] * fdnLpY_[k];
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

        // Output: mid/side mix for stereo width
        float sumEven = 0.0f, sumOdd = 0.0f;
        for (int k = 0; k < kNumDelays_; ++k) {
            if ((k & 1) == 0) sumEven += y[k]; else sumOdd += y[k];
        }
        const float mid = (sumEven + sumOdd) / static_cast<float>(kNumDelays_);
        const float side = (sumEven - sumOdd) / static_cast<float>(kNumDelays_);
        float wetL = mid + widthSmoothed_ * side;
        float wetR = mid - widthSmoothed_ * side;
        // Wet normalization (simple RMS-based gain target ~0.45 for headroom)
        const float wetMono = 0.5f * (wetL + wetR);
        wetRms_ = 0.995f * wetRms_ + 0.005f * (wetMono * wetMono);
        const float targetGain = 0.45f / std::sqrt(std::max(wetRms_, 1e-6f));
        wetNormGainSmoothed_ = 0.99f * wetNormGainSmoothed_ + 0.01f * targetGain;
        wetL *= wetNormGainSmoothed_;
        wetR *= wetNormGainSmoothed_;
        // Gentle safety limiter
        wetL = std::tanh(wetL);
        wetR = std::tanh(wetR);

        const float outL = dry * xinL + wet * wetL;
        const float outR = dry * xinR + wet * wetR;
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

void FDNReverb::ensureFdnCapacity(float sizeScale) {
    // Allocate buffers based on base delays and sample rate (~2x margin)
    for (int k = 0; k < kNumDelays_; ++k) {
        const float baseDelaySamples = (fdnBaseDelayMs_[k] * sizeScale) * (static_cast<float>(getSampleRate())/1000.0f);
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
