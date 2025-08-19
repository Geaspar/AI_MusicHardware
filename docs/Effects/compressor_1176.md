## 1176‑Style FET Compressor — Fast Dynamics & All‑Buttons (Aug 2025)

A musical FET compressor inspired by the UREI 1176 (Rev A/D/E). Focus on fast attack/release, stepped ratios (4/8/12/20:1), and the famous “all‑buttons‑in” behavior. Adds modern niceties (SC HP, mix, stereo link) while preserving punch. Vital reference: sample‑accurate smoothing, stable parameter mapping, and RT‑safe processing patterns.

### Goals
- Authentic 1176 feel: ultra‑fast attack, snappy release, envelope vibe
- Stepped ratios incl. All‑Buttons; program‑dependent release flavor
- Low CPU, RT‑safe; optional transformer/FET color
- Parallel/sidechain conveniences for modern mixes

---

## Core Features
- Ratios: 4:1, 8:1, 12:1, 20:1, ALL (All‑Buttons)
- Attack: 20 µs … 800 µs (1176 tradition: “1”=slow, “7”=fast)
- Release: 50 ms … 1100 ms (program‑shaped tail)
- Detector: feedback topology (post‑gain) with log‑domain gain computer
- Knee: medium‑hard; softening under ALL mode
- SC filters: high‑pass (20–250 Hz), optional tilt (presence save)
- Stereo link: average or max GR link
- Mix (parallel) and Output Trim
- Color: optional input/output transformer and FET asymmetry (subtle 2nd/3rd)

Enhancements (optional)
- Look‑ahead micro‑delay (0–1 ms) to catch transients in modern workflows (OFF by default)
- GR hold (0–10 ms) to stabilize percussive tails
- Adaptive release (two‑time‑constant blend like classic program behavior)

---

## Parameters
- Ratio (enum): 4, 8, 12, 20, ALL
- Attack (µs): 20 … 800 (reversed pot law option)
- Release (ms): 50 … 1100
- Input/Threshold (dB): emulate 1176 input‑driven threshold (or explicit Threshold mode)
- Sidechain HP (Hz): 20 … 250
- Presence Tilt (−3 … +3 dB) — SC only
- Mix (0–1, constant‑power)
- Color (0–1): transformer/FET coloration amount (OS engages at higher values)
- Look‑Ahead (ms): 0 … 1 (optional)
- GR Hold (ms): 0 … 10
- Stereo Link (enum): AVG, MAX
- Output Trim (dB): −12 … +12
- Meter (enum): IN, OUT, GR

Suggested UI (2 pages)
- Page 1: Ratio, Attack, Release, Input/Threshold, Mix, Output, Meter
- Page 2: SC HP, Presence Tilt, Look‑Ahead, GR Hold, Stereo Link, Color

---

## Architecture
- Topology: feedback compressor (feed the level after gain into detector)
- Detector: RMS/peak hybrid; log mapping to GR with ratio and knee
- Time constants: attack microseconds (sample‑rate aware), release milliseconds with program‑dependent curve
- Sidechain: HP and tilt before detector; ALL mode reshapes knee and slightly shifts ratio law
- Gain computer:
  - 1176‑style input: drive into fixed threshold, or alternate explicit threshold mode
  - Knee: two‑segment piecewise soft knee around threshold
- Color path: light transformer shelf/odd/even harmonic via waveshaper at in/out; oversampling for color if heavy
- Mix law: constant‑power parallel (sine/cos)

Stereo link
- AVG: average detector from L/R for GR
- MAX: choose stronger GR (safer for image)

Safeguards
- Smoothing on all controls (10–30 ms); ratio and mode switches crossfade GR over ~10 ms
- Denormal guards; no allocations in process; optional look‑ahead via tiny delay line

---

## Modes / Models
- Rev A “Blue Stripe”: brighter tone, slightly different attack curve (option)
- Rev D/E: smoother action; default
- ALL: emulates all ratio buttons engaged — higher knee curvature, edgy saturation (mild), higher effective ratio

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/Comp1176.h`, `src/effects/Comp1176.cpp`; register in factory; UI wiring (2 pages)

Phase 1 — Core Compression (1–1.5 days)
- Feedback path, detector (RMS/peak mix), ratio/knee, attack/release in µs/ms (SR aware)
- Mix/Trim, meters (IN/OUT/GR); SC HP/Tilt; stereo link; smoothing

Phase 2 — Modes & Color (0.5–1 day)
- ALL mode knee/ratio curve; Rev A vs D/E attack curve; optional Color path (light waveshaper) w/ OS

Phase 3 — Integration & Safety (0.5 day)
- Mod matrix destinations; preset round‑trip; look‑ahead and GR hold; crossfade on mode switches

Phase 4 — Verification & Tuning (0.5–1 day)
- Match behavior against references (attack/release timing and GR at common drive)
- Listen on drums/bass/vocals; CPU profiling; finalize ranges

---

## Math & Pseudocode

Detector (RMS with peak assist)
```cpp
float sc = hp(tilt(in));
float rms = sqrtf(blockAvg(sc*sc));
float peak = maxAbs(scWindow);
float level = 0.7f*rms + 0.3f*peak; // dB convert for GC
```

Gain computer (dB domain)
```cpp
float x_dB = lin2dB(level * inputGain);
float over = x_dB - thresh;
float knee = softKnee(over, kneeWidth);
float y_dB = thresh + knee/ratio; // compress above threshold
float gr_dB = y_dB - x_dB; // negative
```

Attack/Release smoothing (feedback)
```cpp
// target is gr_dB; one-pole smoothing with attack/release constants
float coeffA = expf(-1.0f/(att_us * 1e-6f * sr));
float coeffR = expf(-1.0f/(rel_ms * 1e-3f * sr));
if (gr < target) gr = coeffA*gr + (1-coeffA)*target; else gr = coeffR*gr + (1-coeffR)*target;
```

Apply GR with mix
```cpp
float g = dB2lin(gr);
float y = g * x;
float wetK = sinf(0.5f*M_PI*mix);
float dryK = cosf(0.5f*M_PI*mix);
out = dryK*x + wetK*y;
```

ALL mode knee tweak
```cpp
// increase knee curvature and effective ratio; add slight color
```

---

## References
- UREI 1176 (Rev A/D/E) behavior, timing, and ratios
- DSP compressor design (feedback topology, soft knee, program-dependent release)
- Vital: parameter smoothing and modulation plumbing
