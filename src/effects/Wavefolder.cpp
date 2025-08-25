#include "../../include/effects/Wavefolder.h"
#include <algorithm>

namespace AIMusicHardware {

Wavefolder::Wavefolder(int sampleRate) : Effect(sampleRate) {}
Wavefolder::~Wavefolder() {}

void Wavefolder::process(float* buffer, int numFrames) {
    // Simple wavefolder: apply drive and bias, optional asymmetry, then hard-fold to [-1, 1]
    // Apply a mild auto-gain compensation and user output trim. Mix dry/wet per sample.
    const float drive = std::max(0.1f, drive_);
    // Mild drive-based auto-gain compensation to tame loudness
    const float autoGain = std::max(0.1f, 1.0f / (1.0f + 0.5f * (drive - 1.0f)));
    const float postGain = std::pow(10.0f, outputTrimDb_ / 20.0f);
    for (int i = 0; i < numFrames; ++i) {
        // Left and right
        for (int ch = 0; ch < 2; ++ch) {
            float in = buffer[2 * i + ch];
            // Sign-dependent gain to introduce asymmetry
            const float gPos = drive * (1.0f + 0.8f * std::clamp(asym_, 0.0f, 1.0f));
            const float gNeg = drive * (1.0f + 0.8f * (1.0f - std::clamp(asym_, 0.0f, 1.0f)));
            float g = (in >= 0.0f) ? gPos : gNeg;

            float x = (in + std::clamp(bias_, -1.0f, 1.0f)) * g;
            float y = fold(x);
            // Soft limit extreme cases to avoid NaNs, then apply compensation + trim
            y = std::clamp(y, -1.2f, 1.2f) * autoGain * postGain;
            float out = in * (1.0f - mix_) + y * mix_;
            // Gentle safety soft clip on effect output
            out = std::tanh(out * 0.95f);
            buffer[2 * i + ch] = out;
        }
    }
}

void Wavefolder::setParameter(const std::string& name, float value) {
    if (name == "drive") {
        drive_ = std::clamp(value, 0.1f, 20.0f);
    } else if (name == "bias") {
        bias_ = std::clamp(value, -1.0f, 1.0f);
    } else if (name == "asym") {
        asym_ = std::clamp(value, 0.0f, 1.0f);
    } else if (name == "mix") {
        mix_ = std::clamp(value, 0.0f, 1.0f);
    } else if (name == "output_trim_db") {
        outputTrimDb_ = std::clamp(value, -24.0f, 12.0f);
    }
}

float Wavefolder::getParameter(const std::string& name) const {
    if (name == "drive") return drive_;
    if (name == "bias")  return bias_;
    if (name == "asym")  return asym_;
    if (name == "mix")   return mix_;
    if (name == "output_trim_db") return outputTrimDb_;
    return 0.0f;
}

} // namespace AIMusicHardware
