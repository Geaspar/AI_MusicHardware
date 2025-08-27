#include "../../include/effects/Tremolo.h"
#include <cmath>
#include <algorithm>

namespace AIMusicHardware {

Tremolo::Tremolo(int sampleRate) : Effect(sampleRate) {}
Tremolo::~Tremolo() {}

void Tremolo::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
}

float Tremolo::nextLfoSample() {
    // Advance phase and return current LFO sample (0..1)
    float phaseInc = rateHz_ / static_cast<float>(sampleRate_);
    lfoPhase_ += phaseInc;
    if (lfoPhase_ >= 1.0f) lfoPhase_ -= 1.0f;
    return lfoAtPhase(lfoPhase_);
}

float Tremolo::lfoAtPhase(float phase) const {
    float p = phase - std::floor(phase); // wrap 0..1
    if (shape_ == 1) {
        // triangle 0..1
        return p < 0.5f ? (p * 2.0f) : (2.0f - p * 2.0f);
    } else if (shape_ == 2) {
        // square 0..1
        return (p < 0.5f) ? 1.0f : 0.0f;
    }
    // sine 0..1
    return 0.5f + 0.5f * std::sin(2.0f * 3.14159265359f * p);
}

void Tremolo::process(float* buffer, int numFrames) {
    for (int i = 0; i < numFrames; ++i) {
        // Advance and compute LFO for left channel
        float inL = buffer[2*i + 0];
        float inR = buffer[2*i + 1];

        if (shape_ == 3) {
            // Pan mode: constant-power left/right crossfade driven by LFO
            float lfo = nextLfoSample(); // 0..1
            float pos = (lfo * 2.0f - 1.0f) * depth_; // -1..1 scaled by depth
            float theta = (pos + 1.0f) * 3.14159265359f * 0.25f; // map [-1,1] -> [0, pi/2]
            float gL = std::cos(theta);
            float gR = std::sin(theta);
            float wetL = inL * gL;
            float wetR = inR * gR;
            buffer[2*i + 0] = inL * (1.0f - mix_) + wetL * mix_;
            buffer[2*i + 1] = inR * (1.0f - mix_) + wetR * mix_;
        } else {
            // Amplitude tremolo with optional stereo phase offset
            float lfoL = nextLfoSample(); // 0..1
            float gainL = (1.0f - depth_) + depth_ * lfoL; // (1-depth)..1

            // Right channel gets a phase offset (0..180 deg)
            float phaseOffset = std::clamp(stereoPhaseDeg_, 0.0f, 180.0f) / 360.0f; // normalize to 0..0.5
            float phaseR = lfoPhase_ + phaseOffset;
            if (phaseR >= 1.0f) phaseR -= 1.0f;
            float lfoR = lfoAtPhase(phaseR);
            float gainR = (1.0f - depth_) + depth_ * lfoR;

            float wetL = inL * gainL;
            float wetR = inR * gainR;
            buffer[2*i + 0] = inL * (1.0f - mix_) + wetL * mix_;
            buffer[2*i + 1] = inR * (1.0f - mix_) + wetR * mix_;
        }
    }
}

void Tremolo::setParameter(const std::string& name, float value) {
    if (name == "rate") {
        rateHz_ = std::clamp(value, 0.1f, 15.0f);
    } else if (name == "depth") {
        depth_ = std::clamp(value, 0.0f, 1.0f);
    } else if (name == "mix") {
        mix_ = std::clamp(value, 0.0f, 1.0f);
    } else if (name == "shape") {
        shape_ = std::clamp(static_cast<int>(value), 0, 3);
    } else if (name == "stereo_phase_deg") {
        stereoPhaseDeg_ = std::clamp(value, 0.0f, 180.0f);
    }
}

float Tremolo::getParameter(const std::string& name) const {
    if (name == "rate") return rateHz_;
    if (name == "depth") return depth_;
    if (name == "mix") return mix_;
    if (name == "shape") return static_cast<float>(shape_);
    if (name == "stereo_phase_deg") return stereoPhaseDeg_;
    return 0.0f;
}

} // namespace AIMusicHardware
