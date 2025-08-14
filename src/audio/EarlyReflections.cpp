#include "../../include/audio/EarlyReflections.h"
#include <algorithm>
#include <cmath>

namespace AIMusicHardware {

EarlyReflections::EarlyReflections(int sampleRate)
    : sampleRate_(sampleRate), predelayMs_(0.0f), level_(0.0f), width_(1.0f), writeIndex_(0), maxDelaySamples_(0) {
    computeTapSamples();
    // Allocate buffers for up to 200 ms total delay (predelay + taps)
    maxDelaySamples_ = std::max(1, static_cast<int>(std::ceil(0.2f * sampleRate_)));
    bufferL_.assign(maxDelaySamples_, 0.0f);
    bufferR_.assign(maxDelaySamples_, 0.0f);
}

void EarlyReflections::setSampleRate(int sampleRate) {
    sampleRate_ = sampleRate;
    computeTapSamples();
    maxDelaySamples_ = std::max(1, static_cast<int>(std::ceil(0.2f * sampleRate_)));
    bufferL_.assign(maxDelaySamples_, 0.0f);
    bufferR_.assign(maxDelaySamples_, 0.0f);
    writeIndex_ = 0;
}

void EarlyReflections::setPredelayMs(float milliseconds) { predelayMs_ = std::max(0.0f, milliseconds); }

void EarlyReflections::setLevel(float level01) { level_ = std::clamp(level01, 0.0f, 1.0f); }

void EarlyReflections::setWidth(float width01) { width_ = std::clamp(width01, 0.0f, 1.0f); }

void EarlyReflections::computeTapSamples() {
    // Base tap times in ms for L and R (decorrelated)
    const float tapsLms[] = {2.9f, 5.1f, 7.4f, 11.7f, 13.2f, 17.9f, 23.6f, 31.8f};
    const float tapsRms[] = {3.3f, 6.0f, 8.7f, 12.1f, 14.5f, 19.4f, 26.1f, 34.0f};
    tapsLSamples_.clear();
    tapsRSamples_.clear();
    for (float t : tapsLms) tapsLSamples_.push_back(static_cast<int>(std::round((t / 1000.0f) * sampleRate_)));
    for (float t : tapsRms) tapsRSamples_.push_back(static_cast<int>(std::round((t / 1000.0f) * sampleRate_)));
}

void EarlyReflections::process(float* interleavedStereo, int numFrames) {
    if (!interleavedStereo || numFrames <= 0) return;

    // Predelay in samples
    const int predelaySamples = std::max(0, static_cast<int>(std::round((predelayMs_ / 1000.0f) * sampleRate_)));
    const float centerGain = level_;
    const float sideGain = level_ * width_;

    for (int n = 0; n < numFrames; ++n) {
        const int base = n * 2;
        const float inL = interleavedStereo[base];
        const float inR = interleavedStereo[base + 1];

        // Write input into delay buffers (simple average as mono feed for ER taps)
        const float monoIn = 0.5f * (inL + inR);
        bufferL_[writeIndex_] = monoIn;
        bufferR_[writeIndex_] = monoIn;

        // Read ER contributions
        float erL = 0.0f;
        float erR = 0.0f;

        // Predelay + taps for Left
        for (int dt : tapsLSamples_) {
            int readIndex = writeIndex_ - (predelaySamples + dt);
            while (readIndex < 0) readIndex += maxDelaySamples_;
            erL += bufferL_[static_cast<size_t>(readIndex)];
        }
        // Predelay + taps for Right
        for (int dt : tapsRSamples_) {
            int readIndex = writeIndex_ - (predelaySamples + dt);
            while (readIndex < 0) readIndex += maxDelaySamples_;
            erR += bufferR_[static_cast<size_t>(readIndex)];
        }

        // Normalize by number of taps to keep level controlled
        if (!tapsLSamples_.empty()) erL /= static_cast<float>(tapsLSamples_.size());
        if (!tapsRSamples_.empty()) erR /= static_cast<float>(tapsRSamples_.size());

        // Apply width: center goes to both; side spreads L/R
        const float erCenter = 0.5f * (erL + erR) * centerGain;
        const float erSide   = 0.5f * ( erL - erR) * sideGain;
        const float addL = erCenter + erSide;
        const float addR = erCenter - erSide;

        // Mix ER into the buffer
        interleavedStereo[base]     += addL;
        interleavedStereo[base + 1] += addR;

        // Advance ring index
        ++writeIndex_;
        if (writeIndex_ >= static_cast<size_t>(maxDelaySamples_)) writeIndex_ = 0;
    }
}

void EarlyReflections::reset() {
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0f);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0f);
    writeIndex_ = 0;
}

} // namespace AIMusicHardware
