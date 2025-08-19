## Transient Shaper — Attack/Sustain Sculptor (Aug 2025)

A musical transient designer for shaping percussive punch and sustain. Uses band‑weighted dual‑envelope detection with flexible time constants, stereo link options, and frequency‑selective processing. Vital reference: smoothing/RT patterns and modulation plumbing.

### Goals
- Clear, click‑free control over attack and sustain
- Frequency‑aware detection to avoid pumping or LF mud
- Parallel and M/S options for modern mixes
- Low CPU, RT‑safe

---

## Core Features
- Dual envelopes: fast (attack) and slow (sustain) with independent attack/release
- Band emphasis: sidechain tilt or split band for frequency‑aware shaping
- Attack gain: boost/cut in dB (−12 … +12)
- Sustain gain: boost/cut in dB (−24 … +24)
- Transient window (ms): focus time for attack extraction
- Clip guard and output trim
- Stereo link (AVG/MAX) and Mid/Side mode
- Mix (constant‑power) for parallel shaping

Enhancements (optional)
- “Snap” curve: nonlinear mapping to exaggerate very short attacks
- Auto‑sustain floor: prevents “sucking” in very quiet tails
- Detector floor/noise‑gate to ignore bleed

---

## Parameters
- Mix (0–1, constant‑power)
- Attack Gain (dB): −12 … +12
- Sustain Gain (dB): −24 … +24
- Fast Env Attack/Release (ms): 0.1 … 50 / 5 … 200
- Slow Env Attack/Release (ms): 5 … 200 / 50 … 2000
- Transient Window (ms): 1 … 50
- Band Emphasis (Tilt dB): −6 … +6 (SC tilt) or Split Freq (Hz): 120 … 4000 (when split enabled)
- Mode (enum): SC Tilt, Split Band
- M/S Mode (enum): Off, Mid, Side
- Stereo Link (enum): AVG, MAX
- Snap (0–1), Sustain Floor (dB): −60 … −30
- Output Trim (dB): −12 … +12

Suggested UI (2 pages)
- Page 1: Mix, Attack Gain, Sustain Gain, Fast/Slow Env A/R, Transient Window
- Page 2: Mode (Tilt/Split) + parameter, M/S, Stereo Link, Snap, Sustain Floor, Output Trim

---

## Architecture
- SC path: optional tilt or band split → detector
- Detector: absolute value or RMS with separate fast/slow envelopes (one‑pole smoothers)
- Transient extraction: T = max(0, fast − slow) within TransientWindow shaping
- Sustain extraction: S = slow (optionally minus very slow floor)
- Gain mapping: G = attackGain(T) + sustainGain(S) in dB mapped to linear
- M/S routing: apply per selected channel; stereo link on detector or gains
- Mix: constant‑power with wet shaped signal

Smoothing and safety
- Smooth all parameters 10–50 ms; denormal guards; no allocations in process
- Clip guard after gain map (soft limit) to avoid extreme boosts

---

## Mod Matrix Destinations
- Transient Mix, Attack Gain, Sustain Gain, Fast/Slow A/R, Transient Window, Tilt/Split Freq, M/S Mode (stepped), Stereo Link (stepped), Snap, Sustain Floor, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/TransientShaper.h`, `src/effects/TransientShaper.cpp`; factory + UI mapping

Phase 1 — Detector & Shaping (1 day)
- Implement fast/slow envelopes, transient window, gain mapping for attack/sustain; Mix/Trim; smoothing; denormals

Phase 2 — Frequency Awareness & Routing (0.5 day)
- SC Tilt and Split Band mode; M/S processing; stereo link options; Snap and Sustain Floor

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset round‑trip; listening tests on drums/bass/guitars; CPU profiling; finalize ranges

---

## Math & Pseudocode

Envelopes
```cpp
// one-pole envelope with separate attack/release
float envOnePole(float x, float aA, float aR, float prev) {
  float a = (x > prev) ? aA : aR; // coefficients in 0..1
  return a*prev + (1-a)*x;
}
```

Transient & sustain
```cpp
float fast = envOnePole(|x|, aA_fast, aR_fast, fastPrev);
float slow = envOnePole(|x|, aA_slow, aR_slow, slowPrev);
float T = max(0.0f, fast - slow); // transient energy
float S = slow;                   // sustain energy
```

Gain mapping
```cpp
float attackDb  = attackGainDb * shape(T, windowMs);
float sustainDb = sustainGainDb * norm(S);
float g = dBToLin(attackDb + sustainDb);
```

Constant‑power mix
```cpp
float wetK = sinf(0.5f*M_PI*mix), dryK = cosf(0.5f*M_PI*mix);
out = dryK*in + wetK*(g*in_ms_routed);
```

---

## References
- Transient designers (SPL, NI Transient Master, FabFilter Saturn transient section)
- Vital: smoothing and modulation patterns
