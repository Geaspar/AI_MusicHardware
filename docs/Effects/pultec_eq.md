## Pultec‑Style EQ — EQP‑1A Program Equalizer (Aug 2025)

A musical “Pultec” program EQ inspired by the EQP‑1A: passive EQ curves with tube‑style makeup gain. Signature features include the low‑frequency Boost/Atten interaction (the famous Pultec trick), smooth high‑shelf Boost with Bandwidth (Q), and high‑cut attenuation. Adds modern conveniences (mix, trim, M/S) while respecting the classic tonality. Vital reference: filter/saturation building blocks and smoothing.

### Goals
- Capture the broad, gentle curves and interactive behavior
- Implement stepped frequency selections and Bandwidth control
- Offer subtle tube/transformer color with optional oversampling
- Low CPU, RT‑safe; stable gain staging

---

## Feature Overview
- Low Band (shelf region)
  - Frequencies (stepped): 20, 30, 60, 100 Hz
  - Boost (0 … +12 dB)
  - Attenuate (0 … 12 dB)
  - Interaction: Boost and Attenuate use different curves → classic tight low‑end trick

- High Band Boost (peak‑ish / shelf‑ish)
  - Frequencies (stepped): 3, 4, 5, 8, 10, 12, 16 kHz
  - Boost (0 … +16 dB)
  - Bandwidth (0 … 10): wider to narrower (Pultec “Bandwidth” control)

- High Band Attenuation (high‑shelf cut)
  - Frequencies (stepped): 5, 10, 20 kHz
  - Attenuate (0 … 16 dB)

- Output/Color
  - Makeup gain stage emulating tube makeup amp (subtle THD 2nd/3rd), optional transformer contour
  - Oversampling (Off/2×/4×) around the color stage only

Enhancements (optional, default OFF)
- M/S processing: apply EQ to Mid or Side or both
- Mix (parallel) and Output Trim
- Linear mode (no color) for clean mastering curves

---

## Parameters
- Mode (enum): Normal, Linear (no color)
- Mix (0–1, constant‑power)
- Low Freq (enum): 20/30/60/100 Hz
- Low Boost (dB): 0 … +12
- Low Atten (dB): 0 … 12
- High Boost Freq (enum): 3/4/5/8/10/12/16 kHz
- High Boost (dB): 0 … +16
- Bandwidth (0 … 10): Pultec style (0=narrow, 10=wide) — reversed feel
- High Atten Freq (enum): 5/10/20 kHz
- High Atten (dB): 0 … 16
- Color (0–1)
- Oversampling (enum): Off/2×/4×
- M/S Mode (enum): Off/Mid/Side
- Output Trim (dB): −24 … +24

Suggested UI (2 pages)
- Page 1: Low (freq/boost/atten), High Boost (freq/boost/bandwidth)
- Page 2: High Atten (freq/atten), Mode, Color, OS, M/S, Mix, Output Trim

---

## Architecture
- Passive EQ approximation with RBJ/analog‑style biquads arranged to mimic Pultec curves:
  - Low Boost: low‑shelf with gentle Q and stepped Fc
  - Low Atten: separate low‑shelf cut with different Q/shape; combined interaction yields classic tightness
  - High Boost: peaking/shelving hybrid; Bandwidth maps to Q with inversion per tradition (wider bandwidth toward CW)
  - High Atten: high‑shelf attenuation with stepped Fc
- Color stage: mild tube/transformer model (even/odd THD) with OS if Color>0.2; bypass in Linear mode
- M/S: route mid or side through EQ, recombine; Mix uses constant‑power law
- Smoothing: 10–30 ms on gains and Color; stepped switches crossfade ~10 ms to avoid clicks
- Denormal guards; no allocations in process

Gain staging
- Internal trim between EQ and color to preserve headroom; makeup trim at end

---

## Mod Matrix Destinations
- Pultec Mix, Low Freq/Boost/Atten, High Boost Freq/Boost/Bandwidth, High Atten Freq/Atten, Mode (stepped), Color, OS (stepped), M/S Mode (stepped), Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/PultecEQ.h`, `src/effects/PultecEQ.cpp`; register in factory; UI wiring

Phase 1 — EQ Curves (1 day)
- Implement low boost/cut shelves with differing Q; high boost with bandwidth mapping; high attenuation
- Stepped frequency lists and crossfades on changes; smoothing and headroom management

Phase 2 — Color & Routing (0.5–1 day)
- Tube/transformer color block with OS; M/S routing and constant‑power Mix

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset persistence; listening tests vs reference; CPU profiling; finalize ranges

---

## Math & Pseudocode

Low shelves
```cpp
// RBJ low-shelf with Fc from {20,30,60,100} Hz
```

High boost with bandwidth
```cpp
// Peaking filter where bandwidth knob maps to Q inversely
Q = mapBandwidthToQ(bw); // e.g., Q = 1.0 / (0.1 + 0.09*bw)
```

High attenuation shelf
```cpp
// RBJ high-shelf cut with Fc from {5k,10k,20k}
```

Color (mild)
```cpp
// soft saturation: y = tanh(a*x) + k*(x - tanh(x)); even/odd mix by Color
```

Constant‑power Mix
```cpp
wet = sinf(0.5f*M_PI*mix); dry = cosf(0.5f*M_PI*mix);
out = dry*x + wet*eq_color(x);
```

---

## References
- Pultec EQP‑1A curves and frequency steps (public documents/analyses)
- RBJ Audio EQ Cookbook
- Tube/transformer coloration behavior
- Vital: smoothing and filter building blocks
