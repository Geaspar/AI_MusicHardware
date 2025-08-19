## WestFold Wavefolder — West Coast/Buchla‑Style Folding Distortion (Aug 2025)

A musical wavefolder inspired by West‑Coast synthesis (Buchla/Serge). Unlike clipping, folding reflects the waveform back on itself when it exceeds a threshold, creating rich odd/even harmonics that track with input level. Designed to work on synths and guitars; includes symmetry and bias for timbral control, plus oversampling for artifact reduction. Vital reference: smoothing/RT patterns and constant‑power mix.

### Why this complements existing distortions
- We already have Soft/Hard/Fuzz/Tube (Distortion) and Soft/Tube/Tape/Analog (Saturation). A wavefolder adds a new timbral family (reflective nonlinearity) distinct from clipping/saturation.

---

## Core Features
- Drive (pre‑gain) and Fold Amount (threshold & repetition control)
- Symmetry (0–1): even‑vs‑odd harmonic emphasis (asym fold)
- Bias (−1 … +1): DC offset before folding for timbral shift
- Stages (1–4): cascade multiple folds for richer spectra
- Tone: post tilt/shelf and optional low‑pass tame
- DC Block: one‑pole HP around 5–20 Hz to remove offset
- Mix (constant‑power) and Output Trim
- Oversampling (Off/2×/4×) for high Drive/Fold

---

## Parameters
- Mix (0–1)
- Drive (dB): 0 … +24
- Fold Amount (0 … 1): maps to threshold & repeats
- Symmetry (0 … 1)
- Bias (−1 … +1)
- Stages (enum): 1, 2, 3, 4
- Tone LP (Hz): 2 k … 20 k
- Tone Tilt (−6 … +6 dB)
- DC Block (Hz): 5 … 20
- Oversampling (enum): Off, 2×, 4×
- Output Trim (dB): −24 … +24

Suggested UI (2 pages)
- Page 1: Mix, Drive, Fold Amount, Symmetry, Bias, Stages
- Page 2: Tone LP, Tone Tilt, DC Block, Oversampling, Output Trim

---

## Architecture & Algorithms

Folding function
- Base reflect: reflect x around ±T (threshold) repeatedly (saw‑to‑triangle style)
- Practical function (continuous):
```cpp
// foldAround implements reflective folding with period 2T
float foldAround(float x, float T) {
  float p = 2.0f * T;
  // wrap to [-T, +T]
  float y = fmodf(x + T, p);
  if (y < 0) y += p;
  y -= T; // now in [-T, +T]
  // reflect upper half to form triangle
  return (y > 0 ? (T - y) : (-T - y));
}
```
- Symmetry: apply different thresholds for +/− or mix folded/unfolded halves
```cpp
float Tp = T * (1.0f + symSkew); // positive threshold
float Tn = T * (1.0f - symSkew); // negative threshold
// piecewise fold using Tp/Tn
```
- Staging: cascade N stages with slight LP in between to control brightness

Signal flow
- x → preGain(Drive) → add Bias → for s in Stages: fold(x, T) → tone LP/tilt → DC block → Mix → Output Trim

Oversampling
- When Drive+Fold exceed a threshold, process folding core at 2×/4× OS to reduce aliasing; linear/min‑phase up/down kernels

Smoothing & safety
- Smooth Drive/Fold/Symmetry/Bias/Tone 10–30 ms; denormal guards; no allocations in process

Mix law
- Constant‑power: wet = sin(π/2·Mix), dry = cos(π/2·Mix)

---

## Mod Matrix Destinations
- Wavefolder Mix, Drive, Fold Amount, Symmetry, Bias, Stages (stepped), Tone LP/Tilt, DC Block (stepped), Oversampling (stepped), Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/Wavefolder.h`, `src/effects/Wavefolder.cpp`; factory + UI mapping (2 pages)

Phase 1 — Core Folding (1 day)
- Implement reflective foldAround with symmetric/asymmetric thresholds; Drive/Bias; Stages cascade; DC block; Tone LP/Tilt; Mix/Trim; smoothing

Phase 2 — Oversampling & Polish (0.5 day)
- 2×/4× OS around folding core with mode switch/crossfade; parameter caps for stability

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset persistence; listening tests on bass/leads/drums; CPU profiling; finalize ranges

---

## References
- Buchla/Serge wavefolders; Eurorack wavefolder modules (for response inspiration)
- DSP folding techniques (reflective mapping vs polynomial approximations)
- Vital: smoothing and mix patterns
