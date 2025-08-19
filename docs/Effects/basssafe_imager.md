## BassSafe Imager — Stereo Width with Bass Mono (Aug 2025)

A musical stereo imaging effect that widens or narrows the image while keeping low frequencies mono‑safe. Uses mid/side processing with a bass crossover, optional stereoizer (Haas/allpass), and gentle tone options. Vital reference: constant‑power mix and RT‑safe modulation patterns.

### Goals
- Intuitive width control that preserves mono compatibility
- Bass mono fold‑down with adjustable crossover and slope
- Optional stereoize for narrow sources without phasey artifacts
- Low CPU, RT‑safe; click‑free under automation

---

## Core Features
- Width: 0% (mono) … 200% (extra wide) via mid/side scaling
- Bass Mono: fold low band to mono with adjustable crossover (40–300 Hz) and slope (6/12/24 dB)
- Balance: M/S or L/R balance/pan options
- Stereoize: Haas micro‑delays and/or allpass decorrelation (time 0–20 ms, depth 0–1)
- Side HP: remove low frequencies from S channel to reduce smear
- Mid Tilt: gentle tilt EQ on M to counteract perceived dullness when narrowing
- Mix (parallel) and Output Trim

Enhancements (optional)
- HF Widen: extra widening above a second crossover (e.g., >6 kHz)
- Correlation Guard: soft limiter on side level to keep correlation above a floor
- M/S EQ trims: small dB trims for M and S

---

## Parameters
- Mix (0–1, constant‑power)
- Width (%): 0 … 200
- Balance (dB or %): L↔R or M↔S bias
- Bass Mono Freq (Hz): 40 … 300
- Bass Mono Slope (enum): 6, 12, 24 dB/oct
- Stereoize Time (ms): 0 … 20 (0 disables)
- Stereoize Depth (0 … 1)
- Side HP (Hz): 20 … 300
- Mid Tilt (dB): −3 … +3
- HF Widen Freq (Hz): 6k … 12k (optional)
- HF Widen Amount (0 … 1)
- M Trim (dB): −6 … +6
- S Trim (dB): −6 … +6
- Output Trim (dB): −12 … +12

Suggested UI (2 pages)
- Page 1: Mix, Width, Balance, Bass Mono Freq/Slope, Side HP, Output Trim
- Page 2: Stereoize Time/Depth, Mid Tilt, HF Widen Freq/Amount, M/S Trims

---

## Architecture

Mid/Side matrix
- mid = 0.5·(L + R), side = 0.5·(L − R)
- Width scales side: side' = widthScale · side (widthScale from %)
- Balance applies either MS bias (mid/side trim) or LR pan as desired

Bass mono fold‑down
- Split mid/side (or LR) with low‑pass at Bass Mono Freq (stepped slopes)
- Force low band to mono: set side_low = 0; recombine with untouched mids/highs
- Optionally reduce S via Side HP

Stereoize
- Apply micro‑delay or short allpass to one channel (or to S) with small time (≤20 ms) and depth
- Randomize phase seeds minimally for decorrelation without flanging

HF widen (optional)
- Above HF Widen Freq, apply a gentle shelf to S or small extra width scale

Mix law and output
- Constant‑power parallel mix; Output Trim post‑mix

Smoothing & safety
- Smooth all controls (10–30 ms); denormal guards; no allocations
- Clamp width to safe scale; correlation guard on extremes if enabled

---

## Mod Matrix Destinations
- Imager Mix, Width, Balance, Bass Mono Freq/Slope (stepped), Stereoize Time/Depth, Side HP, Mid Tilt, HF Widen Freq/Amount, M Trim, S Trim, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/BassSafeImager.h`, `src/effects/BassSafeImager.cpp`; register in factory; UI mapping (2 pages)

Phase 1 — Core MS & Bass Mono (1 day)
- Implement MS matrix, width scaling, bass mono crossover/slope, side HP; Mix/Trim; smoothing; denormals

Phase 2 — Stereoize & Enhancements (0.5 day)
- Haas/allpass stereoizer; Mid Tilt; optional HF widen; M/S trims

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset persistence; correlation checks; listening tests and CPU profiling; finalize ranges

---

## Math & Pseudocode

Mid/Side and width
```cpp
mid = 0.5f*(L+R); side = 0.5f*(L-R);
side *= widthScale; // width 0..2 maps to 0..2 scale
Lw = mid + side; Rw = mid - side;
```

Bass mono
```cpp
// split into low/high via LP/HP at fc with chosen slope
side_low = lowpass(side, fc, slope);
side_high = side - side_low;
// force low to mono by zeroing side_low
side_low = 0.0f;
// recombine
L = mid + (side_low + side_high);
R = mid - (side_low + side_high);
```

Stereoize (Haas)
```cpp
R = delay(R, t_ms); // small delay
// or apply short allpass to S
```

Constant‑power mix
```cpp
wet = sinf(0.5f*M_PI*mix); dry = cosf(0.5f*M_PI*mix);
outL = dry*inL + wet*L;
outR = dry*inR + wet*R;
```

---

## References
- M/S processing techniques; mastering imager design
- Psychoacoustics of stereo width and bass localization (mono low frequencies)
- Vital: smoothing/mix patterns
