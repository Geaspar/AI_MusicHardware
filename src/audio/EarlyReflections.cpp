#include "../../include/audio/EarlyReflections.h"

namespace AIMusicHardware {

EarlyReflections::EarlyReflections(int sampleRate)
    : sampleRate_(sampleRate), predelayMs_(0.0f), level_(0.0f), width_(1.0f) {}

void EarlyReflections::setSampleRate(int sampleRate) { sampleRate_ = sampleRate; }

void EarlyReflections::setPredelayMs(float milliseconds) { predelayMs_ = milliseconds; }

void EarlyReflections::setLevel(float level01) { level_ = level01; }

void EarlyReflections::setWidth(float width01) { width_ = width01; }

void EarlyReflections::process(float* interleavedStereo, int numFrames) {
    // Pass-through placeholder for scaffolding
    (void)interleavedStereo;
    (void)numFrames;
}

void EarlyReflections::reset() {
    // No state yet
}

} // namespace AIMusicHardware
