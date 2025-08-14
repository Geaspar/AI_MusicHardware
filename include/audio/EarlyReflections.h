#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

namespace AIMusicHardware {

class EarlyReflections {
public:
    explicit EarlyReflections(int sampleRate = 48000);

    void setSampleRate(int sampleRate);

    // Parameters (normalized unless noted)
    void setPredelayMs(float milliseconds);
    void setLevel(float level01);
    void setWidth(float width01);

    // Process interleaved stereo buffer in-place: adds ER signal
    void process(float* interleavedStereo, int numFrames);

    void reset();

private:
    void computeTapSamples();
    inline float lerp(float a, float b, float t) const { return a + (b - a) * t; }

    int sampleRate_;
    float predelayMs_;
    float level_;
    float width_;

    // Delay buffers per channel
    std::vector<float> bufferL_;
    std::vector<float> bufferR_;
    size_t writeIndex_;
    int maxDelaySamples_;

    // Tap offsets in samples (excludes predelay)
    std::vector<int> tapsLSamples_;
    std::vector<int> tapsRSamples_;
};

} // namespace AIMusicHardware
