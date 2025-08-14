#pragma once

#include <vector>
#include <cstdint>

namespace AIMusicHardware {

class EarlyReflections {
public:
    explicit EarlyReflections(int sampleRate = 48000);

    void setSampleRate(int sampleRate);

    // Parameters (normalized unless noted)
    void setPredelayMs(float milliseconds);
    void setLevel(float level01);
    void setWidth(float width01);

    // Process interleaved stereo buffer in-place (pass-through initial impl)
    void process(float* interleavedStereo, int numFrames);

    void reset();

private:
    int sampleRate_;
    float predelayMs_;
    float level_;
    float width_;
};

} // namespace AIMusicHardware
