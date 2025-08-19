## Tape — Saturation, Compression, and Wow/Flutter (Aug 2025)

A musical tape coloration effect combining soft saturation, dynamic compression, head‑bump EQ, and wow/flutter modulation. Inspired by classic tape decks (15/30 ips), with modern stability and low latency. Vital reference: nonlinear drive/filters and per‑block smoothing patterns.

### Goals
- Authentic tape tone: soft clipping, bass head‑bump, gentle HF roll‑off
- Program‑dependent compression and transient rounding
- Wow/Flutter for motion (optional), stereo‑aware
- Oversampling when needed, RT‑safe behavior, stable output level

---

## Core Features
- IPS Modes: 15 ips, 30 ips (changes head‑bump, HF roll‑off, wow/flutter ranges)
- Drive: 0–24 dB into saturation
- Bias: −6 … +6 dB (pre‑emphasis tilt into waveshaper)
- Tone: Tilt EQ (bass↔air) and HF shelf
- Head Bump: Resonant low‑shelf ~40–80 Hz (amount/freq depends on IPS)
- Wow/Flutter: dual‑rate modulation (Wow 0.1–1.0 Hz; Flutter 4–10 Hz) low depth
- Comp/Soft Knee: program‑dependent compression preceding saturation
- Hysteresis (optional): simple stateful asymmetry for magnetic memory feel
- Stereo Link: link detection or dual‑mono character
- Mix (constant‑power) and Output Trim
- Oversampling: Off/2×/4× (auto engages for hot drive)

---

## Parameters
- Mix (0–1)
- Drive (dB): 0–24
- Bias (dB): −6…+6 (pre‑emphasis tilt)
- IPS (enum): 15, 30
- Head Bump (amount 0–1), Head Bump Freq (Hz)
- Tone Tilt (−6…+6 dB), HF Shelf (0–1)
- Wow (Hz 0.1–1.0), Wow Depth (% 0–0.5)
- Flutter (Hz 4–10), Flutter Depth (% 0–0.2)
- Comp Amount (0–1), Knee (0–1), Stereo Link (bool)
- Hysteresis (0–1)
- Oversampling (Off/2×/4×)
- Output Trim (dB, −12…+6)

Suggested UI (2 pages)
- Page 1: Mix, Drive, Bias, IPS, Head Bump (Amt/Freq), Tone Tilt, HF Shelf
- Page 2: Wow (rate/depth), Flutter (rate/depth), Comp, Knee, Hysteresis, Stereo Link, Oversampling, Output Trim

---

## Architecture
- Pre‑emphasis: tilt EQ + bias gain → feeds program detector and waveshaper
- Detector: RMS/peak hybrid with attack/release → comp gain (soft knee)
- Waveshaper: soft‑clip curve with asymmetry (tanh/sinh mix), optional state‑dependent hysteresis term
- Head bump: resonant low‑shelf or peaking EQ at 40–80 Hz (IPS‑dependent)
- HF response: gentle LP/shelf (IPS‑dependent), tone tilt applied post‑clip for coloration
- Wow/Flutter: modulate a short fractional delay (few samples) or phase for subtle pitch drift; stereo phases decorrelated
- Mix: constant‑power crossfade; Output Trim
- Oversampling: polyphase up/down with linear‑phase or minimum‑phase kernels; only waveshaper in OS domain

Safeguards & polish
- Clamp drive into OS path; denormal guards; no allocations in process
- Per‑block smoothing for Drive/Bias/Comp/Tilt; wow/flutter depth tiny and SR‑invariant
- Stereo link option on detector; otherwise dual‑mono

---

## Waveshaper Curve (example)
- y = tanh(a·x) + k·(x − tanh(x)) for soft but present harmonics
- Asymmetry via biasing and small odd term: y += h * tanh(b·(x + o)) − h * tanh(b·(x − o))
- Calibrate a,k,h,b,o vs Drive and Hysteresis

Head Bump EQ (biquad outline)
- Peaking/low‑shelf with Q ~0.7–1.2, Fc from IPS or parameter

Wow/Flutter
- wow = sine LFO 0.1–1.0 Hz, flutter = sine/noise LFO 4–10 Hz
- Apply to micro delay (2–8 samples) or fractional resampler on wet path

---

## Mod Matrix Destinations
- Tape Mix, Drive, Bias, Head Bump Amt/Freq, Tone Tilt, HF Shelf, Wow/Flutter Rate/Depth, Comp, Knee, Hysteresis, Output Trim, Oversampling (stepped)

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/Tape.h`, `src/effects/Tape.cpp`; factory + UI mapping

Phase 1 — Core Tone (1 day)
- Pre‑emphasis tilt, detector (RMS), soft‑knee compressor, soft‑clipper, head‑bump, HF shelf; Mix/Trim; smoothing & denormals

Phase 2 — Character & Motion (0.5–1 day)
- Hysteresis asymmetry; wow/flutter micro‑delay with stereo phase options
- IPS presets (15/30) mapping defaults for bump/HF and wow/flutter ranges

Phase 3 — Oversampling & Safety (0.5 day)
- 2×/4× OS for clipper path; latency reporting (if needed) and CPU guard

Phase 4 — Integration & Tuning (0.5–1 day)
- Mod destinations; preset round‑trip; listening tests; calibration of Drive/Trim for unity feels

---

## Math & Pseudocode

Soft‑knee compressor
```cpp
// standard soft-knee mapping from threshold/ratio/knee to gain
```

Tilt EQ (simple)
```cpp
// y = LP(x)*gLow + HP(x)*gHigh with gLow/gHigh from tilt dB
```

Waveshaper
```cpp
y = tanhf(a*x) + k*(x - tanhf(x));
y += h * (tanhf(b*(x+o)) - tanhf(b*(x-o)));
```

Head bump biquad
```cpp
// design low-shelf/peaking biquad (Fc,Q,Gain)
```

Wow/Flutter delay
```cpp
float frac = base + wowDepth*sin(wPhase) + flutterDepth*sin(fPhase);
out = delay.readFrac(writeIndex - frac);
```

---

## References
- Vital: saturation and filter building blocks; smoothing
- Classic tape literature (head bump, bias, wow/flutter)
- Airwindows/TDR/Goodhertz tapes (behavioral inspiration)
