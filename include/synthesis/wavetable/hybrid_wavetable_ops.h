#pragma once

#include "hybrid_wavetable.h"
#include <vector>

namespace AIMusicHardware {

class Wavetable; // fwd decl

// Build a spectral frame after morph and ops; applies Nyquist mask
SpectralFrame buildSpectralFrame(const SpectralTable& table,
                                 float morph01,
                                 const SpectralOps& ops,
                                 int fftSize,
                                 int sampleRate);

// Render a time-domain buffer from a spectral frame (IFFT + DC removal + RMS norm)
WavetableBuffer renderTimeDomain(const SpectralFrame& spectral,
                                 int sampleRate);

// Utility: remove DC and normalize to target RMS (in place)
void removeDcAndNormalize(std::vector<float>& samples, float targetRms);

// Convert a time-domain Wavetable into a SpectralTable (one spectral frame per wave frame)
std::shared_ptr<SpectralTable> spectralFromWavetable(const Wavetable& wt, int sampleRate);

} // namespace AIMusicHardware
