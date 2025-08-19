## Tremolo — Design Target: Pecheneg Tremolo (Aug 2025)

This document specifies a tremolo effect modeled after Pecheneg Tremolo’s feel and feature set, with several enhancements for modern synth workflows. Goal: indistinguishable behavior at common settings, plus extras that remain invisible unless enabled.

### Product Goals
- Sound and response that match Pecheneg Tremolo at equivalent settings
- Zero‑click operation across parameter changes (depth/rate/shape)
- Tempo sync with musical note divisions, triplets, dotted, swing
- Stereo/auto‑pan modes with precise phase control (0–180°)
- CPU‑lean and RT‑safe

---

## Core Features (parity with Pecheneg Tremolo)
- Rate: Hz and tempo‑sync divisions (1/64 … 8 bars, dotted, triplet)
- Depth: 0–100%
- Waveform/Shape:
  - Sine, Triangle, Square, Ramp Up, Ramp Down, Random (S&H)
  - Shape/Hardness: smoothly morph sinus ↔ triangle ↔ square (duty/pulse width for square/ramp)
- Phase: 0–180° (stereo offset). 180° = auto‑pan when right channel is inverted phase of left.
- Bias/Offset: centers modulation above/below 0.5 to emphasize dips or peaks (classic “chop” vs “bloom”) 
- Level/Output: make‑up to preserve perceived loudness
- Sync: re‑start phase on host transport start (optional), free‑run otherwise

Behavioral notes
- Sine/triangle modes are smooth; square/ramp modes use band‑limited transitions to avoid clicks.
- Depth changes are smoothed (10–30 ms) to avoid zippering.

---

## Enhancements (optional, default OFF)
- Harmonic Tremolo: dual‑band split (LP/HP) modulated in opposite polarity (brownface amp style)
- Envelope‑Biased Depth: envelope follower nudges depth ±20% for dynamic feel
- Swing: shift the second half‑cycle timing 0–30%
- Humanize: subtle random rate drift (±0.5%) with slow wander
- Constant‑Power Law: perceptually stable loudness by mapping modulator to amplitude with sqrt/DB law

---

## Parameters
- Mix (0–1, constant‑power)
- Depth (0–1)
- Rate (Hz) / Sync Division (enum)
- Waveform (enum): Sine, Triangle, Square, RampUp, RampDown, Random (S&H)
- Shape/Skew (0–1): waveform morph / duty / slope asymmetry
- Phase (deg, 0–180): stereo phase between L/R
- Bias (−1…+1): shifts modulation center (emphasize cut vs boost)
- Swing (0–0.3)
- Harmonic Mode (bool): enable dual‑band opposite‑polarity trem
- Xover (Hz, 100–2000): for harmonic mode split
- Envelope Depth Assist (0–0.2)
- Output Trim (dB, −12…+6)

Suggested UI (2 pages)
- Page 1: Mix, Depth, Rate/Sync, Waveform, Shape, Phase
- Page 2: Bias, Swing, Harmonic Mode + Xover, Env Assist, Output Trim

Tempo sync
- Divisions: 1/64, 1/32, 1/16, 1/8, 1/4, 1/2, 1, 2, 4, 8 bars
- Dot/Triplet variants per division

---

## Architecture
- Modulator LFO: phase accumulator at block‑rate advanced with Hz or host tempo
- Wave shaper: generates base waveform; shape/skew morphs between families
- Stereo: right channel phase = left phase + PhaseDeg (wrapped)
- Harmonic Mode: LR both split by a 4th‑order Linkwitz‑Riley @ Xover; HP branch gets inverted mod sign
- Envelope Assist: follower (fast attack, medium release) scaled into Depth
- Mix: constant‑power crossfade dry/wet or amplitude‑domain law depending on Mix model (see below)

Amplitude mapping models
- Simple: gain = 1 − Depth + Depth * mod (mod ∈ [0,1])
- Constant‑Power (preferred): map mod to dB or sqrt domain: gain = 10^{(g_db/20)} with g_db shaped so RMS stays stable; or g = sqrt(1 − d + d*m)
- Bias: apply bias in mod domain before law

Anti‑click policies
- Square/ramp band‑limited transitions (minBLEP or short window crossfades)
- Parameter smoothing: Depth/Shape/Phase/Swing 10–30 ms, Rate 30–100 ms
- DC guard: optional high‑pass at 5–20 Hz after amplitude stage

---

## Mod Matrix Destinations
- Tremolo Mix, Depth, Rate, Shape, Phase, Bias, Swing, Output Trim
- (When Harmonic Mode): Xover, Env Assist

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/Tremolo.h`, `src/effects/Tremolo.cpp`, register in factory, UI wiring (2 pages)

Phase 1 — Core LFO + Shapes (1 day)
- Phase accumulator, tempo sync, waveform gen (sine/tri/square/ramp/random)
- Shape morph/duty; stereo phase; depth/bias; mix (constant‑power)
- Band‑limited transitions for square/ramp

Phase 2 — Musical Options (0.5–1 day)
- Swing timing and phase restart options
- Humanize (low‑depth slow random drift)
- Envelope‑biased depth

Phase 3 — Harmonic Tremolo (0.5 day)
- 4th‑order LR split; invert mod sign on HP branch; recombine

Phase 4 — Integration & Safety (0.5 day)
- Expose mod destinations; preset save/load
- Denormal guards; smoothing; no allocations

Phase 5 — Verification & Matching (0.5–1 day)
- Match Pecheneg Tremolo at key settings (sine/tri/square at 1/4, 1/8, with/without phase)
- Check click‑free behavior; ensure stereo autopan at 180° feels identical
- CPU profile; finalize ranges

---

## Math & Pseudocode

LFO advance (block‑rate)
```cpp
phase += twoPi * rateHz * (blockSize / sr);
phase = fmodf(phase, twoPi);
```

Shape morph (sine↔triangle↔square)
```cpp
float s = sine(phase);
float t = triangle(phase);
float q = smoothSquare(phase, hardness); // band-limited
float m1 = lerp(s, t, shape); // 0..1
float m  = lerp(m1, q, powf(shape, 2.0f));
```

Amplitude mapping with bias & depth (constant‑power approx)
```cpp
// mod in [0,1]; bias in [-1,1]
float mb = clamp(mod + 0.5f * bias, 0.0f, 1.0f);
float d  = depth; // 0..1 (smoothed)
float g  = sqrtf((1.0f - d) + d * mb); // perceptual
outL = g * inL; outR = gR * inR; // with stereo phase offset for R
```

Harmonic mode split/recombine
```cpp
lr.split(inL, inR, lowL, lowR, highL, highR);
float gLow = gainFromMod(mod);
float gHigh = gainFromMod(1.0f - mod); // inverted
outL = gLow*lowL + gHigh*highL;
```

---

## References
- Pecheneg Tremolo: UI/behavior reference target
- Vital: LFO/tempo sync patterns, smoothing policies, stereo phase handling
- Valhalla/Goodhertz style constant‑power mappings for perceptual stability
