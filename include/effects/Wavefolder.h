#pragma once

#include "EffectProcessor.h"

namespace AIMusicHardware {

class Wavefolder : public Effect {
public:
    Wavefolder(int sampleRate = 44100);
    ~Wavefolder() override;

    void process(float* buffer, int numFrames) override;
    void setParameter(const std::string& name, float value) override;
    float getParameter(const std::string& name) const override;
    std::string getName() const override { return "Wavefolder"; }

private:
    float drive_ = 2.0f;   // input gain before folding
    float bias_ = 0.0f;    // DC offset before folding (-1..1)
    float asym_ = 0.0f;    // asymmetry 0..1
    float mix_ = 1.0f;     // 0..1
    float outputTrimDb_ = -6.0f; // output trim in dB

    static inline float fold(float x) {
        // Hard fold: reflect over ±1 bounds repeatedly
        while (x > 1.0f || x < -1.0f) {
            x = (x > 1.0f) ? (2.0f - x) : (-2.0f - x);
        }
        return x;
    }
};

} // namespace AIMusicHardware
