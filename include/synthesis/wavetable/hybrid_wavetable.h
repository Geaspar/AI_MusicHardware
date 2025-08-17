#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <limits>

namespace AIMusicHardware {

// --- Spectral domain primitives (single-cycle periodic signals) ---
struct SpectralBin {
    float magnitude = 0.0f;  // linear magnitude
    float phase = 0.0f;      // radians, [-pi, pi]
};

struct SpectralFrame {
    std::vector<SpectralBin> bins; // size = fftSize/2 + 1
    int fftSize = 2048;
    int sampleRate = 44100;
    float normalizationRms = 1.0f;
    uint64_t checksum = 0; // content ID (optional)
};

struct SpectralTable {
    std::vector<SpectralFrame> frames; // wavetable frames
    std::string id;                    // stable identifier
    int defaultFftSize = 2048;
};

// --- Time-domain single-cycle buffer (ready for oscillator) ---
struct WavetableBuffer {
    std::vector<float> samples; // length = fftSize
    int fftSize = 2048;
    float rms = 1.0f;
    uint64_t keyHash = 0; // cache key baked into artifact
};

// --- Spectral operations (modulatable, control-rate) ---
struct SpectralShelves {
    float lowGainDb = 0.0f;   // [-24, +24]
    float lowCutoffHz = 200.0f;
    float highGainDb = 0.0f;  // [-24, +24]
    float highCutoffHz = 4000.0f;
};

struct SpectralOps {
    float tiltDbPerOct = 0.0f;          // [-24, +24]
    float formantShiftSemitones = 0.0f; // [-24, +24]
    float evenOddBalance = 0.0f;        // [-1, +1]
    float harmonicWarp = 0.0f;          // [0, 1]
    SpectralShelves shelves;
};

// --- Cache key (quantized) ---
struct CacheKey {
    uint64_t tableIdHash = 0;
    uint16_t morphQ = 0;     // 0..127
    uint16_t pitchBand = 0;  // semitone or nyquist index
    uint16_t opsHash16 = 0;  // quantized spectral ops hash
    uint8_t fftSizeCode = 1; // 0:1024,1:2048,2:4096
    uint8_t quality = 0;     // oversampling flags
    uint16_t sampleRateQ = 441; // SR/100
};

inline uint64_t hash64(uint64_t x) {
    // SplitMix64
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    return x;
}

inline uint64_t hash(const CacheKey& k) {
    uint64_t h = 0;
    h ^= hash64(k.tableIdHash + 0x1234);
    h ^= hash64((static_cast<uint64_t>(k.morphQ) << 16) | k.pitchBand);
    h ^= hash64((static_cast<uint64_t>(k.opsHash16) << 32) | (static_cast<uint64_t>(k.fftSizeCode) << 8) | k.quality);
    h ^= hash64(k.sampleRateQ);
    return h;
}

// Quantization helpers (engine/control-rate side)
inline uint16_t quantizeMorph01(float t) {
    if (!std::isfinite(t)) return 0;
    t = std::fmax(0.0f, std::fmin(1.0f, t));
    return static_cast<uint16_t>(std::lround(t * 127.0f));
}

inline uint16_t quantizePitchBand(float semitone) {
    // Map to unsigned band with floor; clamp to 0..2047
    if (!std::isfinite(semitone)) return 0;
    int v = static_cast<int>(std::floor(semitone + 0.5f));
    if (v < 0) v = 0; if (v > 2047) v = 2047;
    return static_cast<uint16_t>(v);
}

inline uint16_t quantizeOpsHash(const SpectralOps& ops) {
    // Cheap mixed hash from quantized params
    auto q = [](float v, float scale) -> uint32_t { return static_cast<uint32_t>(std::lround(v * scale)); };
    uint32_t a = q(ops.tiltDbPerOct, 4.0f) & 0xFF;
    uint32_t b = q(ops.formantShiftSemitones, 2.0f) & 0xFF;
    uint32_t c = q(ops.evenOddBalance, 64.0f) & 0xFF;
    uint32_t d = q(ops.harmonicWarp, 128.0f) & 0xFF;
    uint32_t e = (q(ops.shelves.lowGainDb, 2.0f) & 0x1F) | ((q(ops.shelves.highGainDb, 2.0f) & 0x1F) << 5);
    return static_cast<uint16_t>(((a ^ (b << 3) ^ (c << 5) ^ (d << 7)) & 0xFF) | ((e & 0x3F) << 8));
}

inline uint8_t fftSizeToCode(int n) {
    switch (n) { case 1024: return 0; case 2048: return 1; case 4096: return 2; default: return 1; }
}

} // namespace AIMusicHardware
