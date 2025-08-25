#include "../../include/effects/RingModulator.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

RingModulator::RingModulator(int sampleRate) : Effect(sampleRate) {}
RingModulator::~RingModulator() {}

void RingModulator::setSampleRate(int sampleRate) {
    Effect::setSampleRate(sampleRate);
}

void RingModulator::process(float* buffer, int numFrames) {
    const float sr = static_cast<float>(sampleRate_);
    const float phaseInc = std::clamp(freqHz_, 0.1f, 2000.0f) / sr; // supports LFO to audio-rate
    const float phaseOffsetR = std::clamp(stereoPhaseDeg_, 0.0f, 180.0f) / 360.0f; // 0..0.5
    const float amt = std::clamp(depth_, 0.0f, 1.0f);
    const float mix = std::clamp(mix_, 0.0f, 1.0f);

    for (int i = 0; i < numFrames; ++i) {
        // advance base phase
        phase_ += phaseInc;
        if (phase_ >= 1.0f) phase_ -= 1.0f;
        float modL = std::sin(2.0f * 3.14159265359f * phase_); // -1..1
        float pr = phase_ + phaseOffsetR; if (pr >= 1.0f) pr -= 1.0f;
        float modR = std::sin(2.0f * 3.14159265359f * pr);

        float inL = buffer[2*i + 0];
        float inR = buffer[2*i + 1];

        // Ring modulation: multiply with bipolar carrier; blend toward dry with depth
        float wetL = inL * ((1.0f - amt) + amt * modL);
        float wetR = inR * ((1.0f - amt) + amt * modR);

        buffer[2*i + 0] = inL * (1.0f - mix) + wetL * mix;
        buffer[2*i + 1] = inR * (1.0f - mix) + wetR * mix;
    }
}

void RingModulator::setParameter(const std::string& name, float value) {
    if (name == "freq_hz") {
        freqHz_ = std::clamp(value, 0.1f, 2000.0f);
    } else if (name == "depth") {
        depth_ = std::clamp(value, 0.0f, 1.0f);
    } else if (name == "mix") {
        mix_ = std::clamp(value, 0.0f, 1.0f);
    } else if (name == "stereo_phase_deg") {
        stereoPhaseDeg_ = std::clamp(value, 0.0f, 180.0f);
    }
}

float RingModulator::getParameter(const std::string& name) const {
    if (name == "freq_hz") return freqHz_;
    if (name == "depth") return depth_;
    if (name == "mix") return mix_;
    if (name == "stereo_phase_deg") return stereoPhaseDeg_;
    return 0.0f;
}

} // namespace AIMusicHardware

