#pragma once

#include <vector>
#include <cstddef>

namespace AIMusicHardware {

class AllpassDiffuser {
public:
    explicit AllpassDiffuser(int sampleRate = 48000);

    void setSampleRate(int sampleRate);
    void setDelaySamples(float delaySamples);
    void setGain(float g);                // typical 0.3..0.75
    void setModulation(float rateHz, float depthSamples);

    inline float processSample(float x);
    void reset();

private:
    float readFrac(const std::vector<float>& buf, float index) const;

    int sampleRate_;
    std::vector<float> buffer_;
    size_t writeIndex_;

    float delaySamples_;   // base delay in samples (can be fractional)
    float gain_;
    float modRateHz_;
    float modDepthSamples_;
    float lfoPhase_;
};

inline float AllpassDiffuser::processSample(float x) {
    // Compute modulated delay in samples
    const float modOffset = modDepthSamples_ * std::sin(lfoPhase_);
    float totalDelay = delaySamples_ + modOffset;
    if (totalDelay < 1.0f) totalDelay = 1.0f;

    // Read delayed value with linear interpolation
    const float readIndex = static_cast<float>(writeIndex_) - totalDelay;
    float wrapped = readIndex;
    while (wrapped < 0.0f) wrapped += static_cast<float>(buffer_.size());
    float d = readFrac(buffer_, wrapped);

    // Allpass structure
    const float y = -gain_ * x + d;
    const float toWrite = x + gain_ * y;

    buffer_[writeIndex_] = toWrite;
    ++writeIndex_;
    if (writeIndex_ >= buffer_.size()) writeIndex_ = 0;

    // Advance LFO phase at sample-rate
    lfoPhase_ += (2.0f * 3.14159265358979323846f) * (modRateHz_ / static_cast<float>(sampleRate_));
    if (lfoPhase_ > 2.0f * 3.14159265358979323846f) lfoPhase_ -= 2.0f * 3.14159265358979323846f;

    // Denormal guard
    return y + 1e-20f;
}

} // namespace AIMusicHardware
