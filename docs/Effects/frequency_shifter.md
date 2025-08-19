## Frequency Shifter — Linear Hz Shifting (Aug 2025)

A high‑quality frequency shifter for barber‑pole effects, sideband animation, and subtle stereo motion. Implements classic Hilbert‑transform quadrature approach with precise LFO sync, stereo options, and psychoacoustic helpers. Vital reference: clean modulation/smoothing patterns and SR‑invariant control mapping.

### Goals
- Clean linear (Hz) shifts with minimal aliasing/bleed
- Musical range from sub‑Hz phasing to kHz robotic shifts
- Stereo/bi‑phase and mid/side routing for width tricks
- Tempo sync for rhythmic phasing
- RT‑safe, CPU‑lean variant and HQ variant (longer Hilbert filters)

---

## Core Features
- Shift Amount: ±0 … ±5000 Hz (fine + coarse), tempo‑sync modulation (LFO)
- Modes: Up (upper sideband), Down (lower), Dual (sum of up+down), Stereo (L/R opposite directions), M/S routing
- Drive (pre) and Tone (post LP/Tilt) to tame brightness
- Mix: 0–100% (constant‑power) and Output Trim
- Anti‑bleed: carrier cancellation tuning + DC/HP guards
- LFO Mod: shift can be modulated by internal LFO (Hz) or external mod matrix (semantics in Hz)

---

## Parameters
- Mix (0–1)
- Shift (Hz): −5000 … +5000
- Mode (Up/Down/Dual/Stereo/MS‑Mid/MS‑Side)
- LFO Rate (Hz) / Sync Division (enum)
- LFO Depth (Hz)
- Stereo Phase (deg, 0–180) for LFO
- Drive (x, 1.0–3.0)
- Tone (0–1) — LP tilt amount post stage
- HP Cut (Hz, 10–80)
- Output Trim (dB, −12..+6)

Suggested UI (2 pages)
- Page 1: Mix, Shift, Mode, LFO Rate/Sync, LFO Depth
- Page 2: Stereo Phase, Drive, Tone, HP Cut, Output Trim

Tempo sync
- Divisions: 1/64 … 4 bars + dotted/triplet

---

## Architecture

Approach A (classic) — Hilbert Quadrature
- Compute 90° phase‑shifted version of input via Hilbert transformer (IIR allpass chain or FIR)
- Form analytic signal: x_a = I + jQ
- Multiply by complex carrier e^{j 2π f_s t} to shift spectrum
  - Up: Re{ x_a · e^{j ωt} } + cancel mirror
  - Down: Re{ x_a · e^{−j ωt} }
- Implementation choices:
  - IIR allpass cascades (light CPU, careful design) or FIR Parks–McClellan (HQ, more CPU)
  - Block‑rate carrier phase advance; stereo carrier with phase offset

Approach B — Phase Vocoder (optional HQ)
- STFT, increment phase per bin by Δω; more expensive but very clean
- Start with Approach A; offer HQ mode later if needed

Safeguards & polish
- DC/HP after shifter (10–80 Hz)
- Constant‑power mix law; output trim
- Pre‑drive and post‑tone tilt; denormal guards
- Parameter smoothing (Shift/LFO/Mode/Mix): 10–50 ms; mode/wave switches crossfade

Stereo & routing
- Stereo mode: L shifts +f, R shifts −f for widening
- M/S mode: shift Mid or Side only

---

## Mod Matrix Destinations
- FreqShifter Mix, Shift, LFO Rate, LFO Depth, Stereo Phase, Drive, Tone, HP Cut, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/FrequencyShifter.h`, `src/effects/FrequencyShifter.cpp`; factory + UI wiring

Phase 1 — Hilbert Core (1–1.5 days)
- Implement IIR allpass Hilbert pair (e.g., 6–8 biquads per branch) or FIR prototype
- Carrier oscillator and complex multiply for Up/Down; DC/HP and mix

Phase 2 — Stereo/Modes & LFO (0.5–1 day)
- Stereo opposite shifts, M/S routing; LFO modulation of Shift with sync and stereo phase
- Drive/Tone/Trim polish

Phase 3 — Integration & Safety (0.5 day)
- Destinations in mod matrix; preset round‑trip; denormal guards; switch crossfades

Phase 4 — Verification & Tuning (0.5 day)
- Sweep tests: verify clean sidebands, low carrier bleed, proper Up/Down behavior
- Mono compatibility checks; CPU profiling; finalize ranges

---

## Math & Pseudocode

Analytic signal and complex multiply (concept)
```cpp
// Hilbert: produce I (original) and Q (90° shifted)
float I = x[n];
float Q = hilbertQ.process(x[n]); // ~90° shift across band

// Complex carrier
phase += twoPi * shiftHz * (blockSize / sr);
float c = cosf(phase), s = sinf(phase);

// Up/Down shifts
float up   = I * c - Q * s; // Re{ (I + jQ) * (c + js) }
float down = I * c + Q * s; // Re{ (I + jQ) * (c - js) }
```

Stereo opposite shifts
```cpp
L_out = shift(L_in, +f);
R_out = shift(R_in, -f);
```

Constant‑power mix
```cpp
float wetK = sinf(0.5f * M_PI * mix);
float dryK = cosf(0.5f * M_PI * mix);
out = dryK * in + wetK * shifted;
```

---

## References
- Vital: modulation smoothing, SR‑aware LFOs, stereo phase handling
- Smith, Julius O.: Hilbert transform, analytic signals
- Classic software: Valhalla Freq Echo, Ableton Frequency Shifter (for UX inspirations)
