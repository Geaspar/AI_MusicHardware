## Envelope Follower — Audio-to-Control Modulator (Aug 2025)

A high-quality envelope follower that converts audio amplitude into a smooth control signal for modulation or dynamic effects (e.g., auto-wah, ducking, tremolo bias). Can run standalone as an effect (to modulate gain/tone) or as a modulation source in the matrix (EnvFollow). Vital reference: smoothing and modulation routing patterns.

### Goals
- Low-latency, click-free envelope suitable for fast percussive sources
- Configurable detector behavior (peak/RMS/absolute), attack/release/hold
- Sidechain support and frequency-selective detection
- Scalable, offsettable output for flexible modulation mapping

---

## Core Features
- Detector Modes: Peak, RMS, Absolute (rectified)
- Attack/Release times with optional Hold
- Sensitivity (pre-gain) and Threshold/Gate floor
- Sidechain input (external or internal bus)
- SC EQ: HP/LP or tilt emphasis
- Output shaping: range (0–1), offset (−1…+1), curve (exp/lin/log), inversion
- Tempo-synced smoothing option (per beat)
- Standalone effect mode: map envelope to Gain (auto-duck/boost) or Tone (tilt), with Mix and Output Trim

---

## Parameters
- Mode (enum): Peak, RMS, Absolute
- Attack (ms): 0.1 … 100
- Release (ms): 5 … 5000
- Hold (ms): 0 … 200
- Sensitivity (dB): −24 … +24 (SC pre-gain)
- Threshold (dBFS): −80 … 0 (gate floor)
- SC HP (Hz): 20 … 500
- SC LP (Hz): 2k … 20k
- Output Range (0 … 1): scales envelope to [0..Range]
- Output Offset (−1 … +1): shifts envelope baseline
- Output Curve (−1 … +1): log ↔ lin ↔ exp mapping
- Invert (bool)
- Tempo Smoothing (bool)
- Mix (0–1, constant-power) [effect mode]
- Target (enum) [effect mode]: Gain, Tone Tilt
- Output Trim (dB): −12 … +12

Suggested UI (2 pages)
- Page 1: Mode, Attack/Release/Hold, Sensitivity, Threshold, Invert
- Page 2: SC HP/LP, Range/Offset/Curve, Tempo Smoothing, [Effect Target], Mix, Output Trim

---

## Architecture
- Sidechain path: input (or SC bus) → SC HP/LP → detector preprocessing (abs/rectify)
- Detector:
  - Peak: exponential attack/release on absolute value
  - RMS: block RMS with one-pole smoothing (AR)
  - Absolute: one-pole smoothing of |x|
- Apply Hold (sample countdown) after attack onset
- Gate floor: if below Threshold, decay toward 0 with Release
- Shaping: scale to Range, apply Offset, Curve mapping (pow/log), optional inversion
- Output: publish to modulation system as source “EnvFollow” and (in effect mode) drive gain/tilt with Mix law

Smoothing & safety
- Attack/Release implemented as one-pole AR (coeffs from time constants); denormal guards; no allocations
- Parameter smoothing (10–30 ms); mode switches crossfade over ~10 ms

Latency
- Zero additional latency (no look-ahead); optional block RMS introduces negligible block latency only for RMS calculation

---

## Mod Matrix Destinations / Sources
- Source: EnvFollow (0..1 after shaping)
- If used as effect: Destinations (Gain, Tone Tilt) with Mix/Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/EnvelopeFollower.h`, `src/effects/EnvelopeFollower.cpp`; register in factory; UI mapping (2 pages)

Phase 1 — Detector & Shaping (1 day)
- Implement Peak/RMS/Absolute detector, AR/Hold, shaping (Range/Offset/Curve/Invert); SC HP/LP; publish modulation source

Phase 2 — Effect Mode (0.5 day)
- Map envelope to Gain or Tone Tilt via Mix; constant-power mix; Output Trim

Phase 3 — Integration & Tuning (0.5 day)
- Mod source registration; preset save/load; listening tests; CPU profiling; finalize ranges

---

## Math & Pseudocode

One-pole AR envelope
```cpp
float x = preGain * filter(scIn);
float aA = expf(-1.0f/(att_ms*1e-3f*sr));
float aR = expf(-1.0f/(rel_ms*1e-3f*sr));
if (x > env) env = aA*env + (1-aA)*x; else env = aR*env + (1-aR)*x;
```

Shaping
```cpp
float e = clamp(env - gateFloor, 0.0f, 1.0f);
if (invert) e = 1.0f - e;
// curve: c in [-1,1] -> exponent k
float k = (c >= 0) ? (1.0f + 4.0f*c) : (1.0f/(1.0f - 0.9f*c));
e = powf(e, k);
e = offset + range * e;
```

Effect mode (gain)
```cpp
float g = dBToLin(gainDb * e);
float y = g * x;
float wetK = sinf(0.5f*M_PI*mix), dryK = cosf(0.5f*M_PI*mix);
out = dryK*x + wetK*y;
```

---

## References
- Envelope follower designs (peak vs RMS), compressor sidechain theory
- Vital: envelope and smoothing patterns
