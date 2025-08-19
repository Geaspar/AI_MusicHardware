## Moog‑Style Ladder Filter — 24 dB/oct Transistor Ladder (Aug 2025)

A classic 4‑pole Moog ladder low‑pass filter with musical drive and resonance. Implements a zero‑delay‑feedback (ZDF/TPT) topology for stability at high resonance and accurate self‑oscillation. Includes key‑tracking, envelope amount, and optional non‑linearities in each stage and feedback path. Vital reference: ZDF SVF/TPT patterns, smoothing, and drive staging.

### Goals
- Authentic ladder sound: warm drive, round low‑end, creamy resonance
- Stable self‑oscillation with correct pitch (ZDF)
- Musical non‑linearities in feedback and/or stage cores
- Stereo‑safe variant with tiny tolerance offsets (optional)

---

## Feature Overview
- Topology: 4 cascaded 1‑pole stages (24 dB/oct) in a feedback ladder
- Modes (optional): 24 dB LP, 18 dB LP (tap after 3rd pole), 12 dB LP (after 2nd pole), BP/HP derived by taps (optional)
- Drive: input and ladder feedback drive; asymmetric option
- Resonance: up to self‑oscillation with pitch‑stable sine
- Key Tracking: 0–100%
- Envelope Amount: EG → cutoff modulation (±)
- Stereo: linked L/R with optional tiny component offsets for width
- Mix (parallel) and Output Trim
- Oversampling: Off/2× (auto engage when Drive+Resonance are hot)

---

## Parameters
- Mix (0–1, constant‑power)
- Cutoff (Hz): 20 … 20 kHz (internally warped via TPT)
- Resonance (0 … 1.2): self‑osc near 1.0
- Drive (dB): 0 … +24 (pre)
- Feedback Drive (0–1): nonlinearity in feedback path
- Mode (enum): LP24, LP18, LP12 (optional BP/HP)
- Key Track (%): 0 … 100
- EG Amount (−1 … +1)
- Stereo Offset (0 … 0.5%): tiny L/R cutoff tolerance for width (optional)
- Oversampling (enum): Off, 2×
- Output Trim (dB): −24 … +24

Suggested UI (2 pages)
- Page 1: Mix, Cutoff, Resonance, Drive, Feedback Drive
- Page 2: Mode, Key Track, EG Amount, Stereo Offset, Oversampling, Output Trim

---

## Architecture

Zero‑Delay Feedback (TPT) Ladder
- Use TPT integrators for each pole (4x), solve feedback implicitly each sample
- Core equations (sketch):
  - v_in = sat(in + drive) − k * y4_nl  (k = resonance gain)
  - For i=1..4: y_i = y_i + g * (v_prev − nl_i(y_i))  (g from bilinear transform)
  - Output taps: y4 for LP24, y3 for LP18, y2 for LP12; optional BP/HP mixes
- Non‑linearities:
  - Stage tanh soft clip: nl_i(x) = tanh(α_i x)
  - Feedback saturation: y4_nl = tanh(β * y4)
- Calibrate resonance gain vs cutoff for self‑oscillation stability

Cutoff mapping & modulation
- TPT bilinear: g = tan(π f_c / f_s)
- KeyTrack: f_c *= 2^(track% * (note − ref)/12)
- EG Amount: f_c += m_eg * EG  (mapped in musical range)
- Smooth cutoff/resonance/drive (10–30 ms); denormal guards

Drive and staging
- Pre‑drive (dB) into ladder; feedback drive adjusts β in tanh; optional per‑stage α_i scaling
- Oversampling around ladder if Drive/Res exceeds threshold; linear‑phase up/down or min‑phase kernels

Stereo behavior
- Linked parameters; optional tiny cutoff offsets for width; maintain mono compatibility

Mix law
- Constant‑power parallel blend: wet = sin(π/2·Mix), dry = cos(π/2·Mix)

---

## Mod Matrix Destinations
- Ladder Mix, Cutoff, Resonance, Drive, Feedback Drive, Mode (stepped), Key Track, EG Amount, Stereo Offset, Oversampling (stepped), Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/MoogLadder.h`, `src/effects/MoogLadder.cpp`; register in factory; UI mapping (2 pages)

Phase 1 — ZDF Ladder Core (1–1.5 days)
- Implement 4‑pole TPT ladder with implicit feedback solve; cutoff/resonance mapping; taps for LP12/18/24; smoothing and denormals

Phase 2 — Character & Stability (0.5–1 day)
- Add stage and feedback tanh non‑linearities; calibrate resonance vs cutoff for pitch‑stable self‑osc; oversampling path when needed

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset round‑trip; key tracking/EG mapping; stereo offset; listening tests and CPU profiling; finalize ranges

---

## Math & Pseudocode

TPT integrator and ladder update (high‑level)
```cpp
float g = tanf(PI * fc / fs);
// feedback solve (simple Newton or closed form for tanh-less case)
float u = sat(in * preGain) - k * sat(y4 * fbGain);
float v1 = tpt(u, s1, g);
float v2 = tpt(v1, s2, g);
float v3 = tpt(v2, s3, g);
float v4 = tpt(v3, s4, g);
// states s1..s4 updated inside tpt()
```

Self‑oscillation calibration
```cpp
// adjust k vs fc to keep sine at unity; empirical table or small function
k = res * kNorm(fc);
```

Constant‑power mix
```cpp
float wet = sinf(0.5f*M_PI*mix), dry = cosf(0.5f*M_PI*mix);
out = dry*in + wet*tap(mode, v2,v3,v4);
```

---

## References
- Stilson/Smith, Huovilainen: Moog ladder ZDF designs
- Vadim Zavalishin: The Art of VA Filter Design (TPT/ZDF theory)
- Vital: filter drive/smoothing patterns
