#include "../../include/audio/AllpassDiffuser.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

AllpassDiffuser::AllpassDiffuser(int sampleRate)
    : sampleRate_(sampleRate), writeIndex_(0), delaySamples_(48.0f), gain_(0.6f), modRateHz_(0.2f), modDepthSamples_(0.0f), lfoPhase_(0.0f) {
    const size_t maxSamples = static_cast<size_t>(std::ceil(sampleRate_ * 0.1f)); // up to 100ms
    buffer_.assign(std::max<size_t>(64, maxSamples), 0.0f);
}

void AllpassDiffuser::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    const size_t maxSamples = static_cast<size_t>(std::ceil(sampleRate_ * 0.1f));
    buffer_.assign(std::max<size_t>(64, maxSamples), 0.0f);
    writeIndex_ = 0;
}

void AllpassDiffuser::setDelaySamples(float delaySamples) {
    delaySamples_ = std::max(1.0f, delaySamples);
}

void AllpassDiffuser::setGain(float g) {
    gain_ = std::clamp(g, 0.0f, 0.99f);
}

void AllpassDiffuser::setModulation(float rateHz, float depthSamples) {
    modRateHz_ = std::max(0.0f, rateHz);
    modDepthSamples_ = std::max(0.0f, depthSamples);
}

float AllpassDiffuser::readFrac(const std::vector<float>& buf, float index) const {
    const size_t size = buf.size();
    int i0 = static_cast<int>(std::floor(index)) % static_cast<int>(size);
    if (i0 < 0) i0 += static_cast<int>(size);
    int i1 = i0 + 1;
    if (i1 >= static_cast<int>(size)) i1 = 0;
    const float frac = index - std::floor(index);
    return buf[static_cast<size_t>(i0)] * (1.0f - frac) + buf[static_cast<size_t>(i1)] * frac;
}

void AllpassDiffuser::reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writeIndex_ = 0;
    lfoPhase_ = 0.0f;
}

} // namespace AIMusicHardware
