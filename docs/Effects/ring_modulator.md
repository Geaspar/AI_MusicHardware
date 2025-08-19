## Ring Modulator — Balanced/Unbalanced AM for Sound Design (Aug 2025)

A musical ring modulator and AM effect with balanced and unbalanced modes, stereo options, tracking, and creative variants. Vital reference: clean modulation paths, SR-aware smoothing, and stable parameterization; we follow similar practices.

### Goals
- Classic AM (tremolo at low rates) and balanced ring modulation (no carrier bleed)
- Clean, alias‑resistant sidebands with perceptual controls
- Stereo‑aware: independent L/R carriers, M/S routing modes
- CPU‑lean, RT‑safe, click‑free under automation

---

## Core Concepts
- Unbalanced AM (Amplitude Modulation): y = (1 + d·m) · x
  - At low rate: tremolo; at audio‑rate: adds carrier plus sidebands
- Balanced Ring Modulation (RM): y = d·m · x
  - Removes dry/carrier, leaving only sum/difference sidebands (classic RM sound)
- Carrier m(t): typically sine in [−1, 1], at frequency f_c; phase accumulators for L/R
- Creative carriers: triangle/square/ramp (band‑limited), noise (smoothed) — optional due to aliasing risk

---

## Features
- Modes: AM (unbalanced), RM (balanced); optionally Rectified (|m|) and Quadrature RM
- Carrier frequency: Hz (0.1–5000 Hz), tempo sync divisions
- Carrier wave: Sine (default), Triangle, Square (band‑limited), Ramp Up/Down, Noise (smoothed)
- Depth: 0–1 (AM depth / RM wetness)
- Stereo Phase: 0–180° (R carrier offset). 180° yields auto‑pan for low rates in AM mode
- Tracking: Key‑track or MIDI note tracking (carrier follows pitch with ratio)
- DC/HP filtering: post stage HP at 10–40 Hz to remove DC and slow drift
- Tone: simple tilt/LP to tame highs when using non‑sine carriers
- Mix: constant‑power crossfade dry/wet
- Output Trim: −12…+6 dB

Creative options (optional)
- M/S Routing: apply RM to Mid or Side only for widening/texture
- Sideband Tilt: gentle EQ to bias lower or upper sideband prominence (psychoacoustic)
- Fold Guard: soft clipper after RM to catch overs

---

## Parameters
- Mix (0–1, constant‑power)
- Mode (AM, RM, Rectified, Quadrature)
- Depth (0–1)
- Carrier Freq (Hz) / Sync Division (enum)
- Carrier Wave (Sine/Tri/Square/RampUp/RampDown/Noise)
- Stereo Phase (deg, 0–180)
- Track Mode (Off / KeyTrack / MIDINote)
- Track Ratio (0.25×–4×)
- Tone (0–1) — tilt/LP amount
- HP Cut (Hz, 10–40)
- Output Trim (dB, −12..+6)

Suggested UI (2 pages)
- Page 1: Mix, Mode, Depth, Carrier Freq/Sync, Wave, Stereo Phase
- Page 2: Track Mode, Track Ratio, Tone, HP Cut, Output Trim

Tempo sync
- Divisions: 1/64 … 4 bars + dotted/triplet

---

## Architecture
- Carrier Oscillators (L/R): sine (default) with phase accum; stereo phase offset for R
- AM vs RM
  - AM: y = (1 − d)·x + d·(1 + m)·x → simple depth blend; clamp to avoid negatives
  - RM: y = (1 − w)·x + w·(m·x), with w from Mix/Depth
- Band‑limited carriers for non‑sine shapes (minBLEP or short crossfades on edges)
- Post HP filter (IIR one‑pole) and optional tone tilt/LP
- Constant‑power Mix law
- Optional M/S matrix for routing

Smoothing and anti‑click policies
- Smooth Depth/Mode/Wave/StereoPhase/Mix: 10–30 ms; Freq: 20–80 ms
- Switches (Mode/Wave) perform short crossfades to prevent clicks
- Denormal guards; no allocations in `process`

Stereo & tracking
- Stereo Phase offsets the R carrier phase
- KeyTrack: carrier f_c = baseRatio × f0 (extract from MIDI or synth pitch)
- MIDINote Track: f_c from last received note (Hz)

---

## Mod Matrix Destinations
- RM Mix, Mode, Depth, Carrier Freq, Carrier Wave, Stereo Phase, Track Ratio, Tone, HP Cut, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/RingModulator.h`, `src/effects/RingModulator.cpp`; factory + UI wiring (2 pages)

Phase 1 — Core AM/RM (1 day)
- Sine carrier; AM and RM math; Mix; Depth; Stereo Phase; constant‑power mix
- Post HP filter and Output Trim; smoothing

Phase 2 — Waves & Sync (0.5–1 day)
- Add carrier waves (Tri/Square/Ramp with band‑limits) and tempo sync
- Add Tone tilt/LP

Phase 3 — Tracking & Stereo (0.5 day)
- KeyTrack/MIDINote tracking; Track Ratio; M/S routing options

Phase 4 — Integration & Safety (0.5 day)
- Mod matrix destinations; preset round‑trip; denormal guards; mode switch crossfades

Phase 5 — Verification & Tuning (0.5 day)
- Compare against classic ring modulators for sideband balance
- Test mono compatibility and stereo width behaviors
- CPU profiling; finalize ranges

---

## Math & Pseudocode

Carrier advancement
```cpp
phaseL += twoPi * fcL * (blockSize / sr);
phaseL = fmodf(phaseL, twoPi);
phaseR = fmodf(phaseL + degToRad(phaseDeg), twoPi);
float mL = carrier(phaseL); // [-1,1]
float mR = carrier(phaseR);
```

AM and RM
```cpp
// AM (unbalanced):
float yL_am = (1.0f + depth * mL) * inL;
float yR_am = (1.0f + depth * mR) * inR;

// RM (balanced):
float yL_rm = (depth * mL) * inL;
float yR_rm = (depth * mR) * inR;
```

Mix (constant‑power)
```cpp
float wetK = sinf(0.5f * M_PI * mix);
float dryK = cosf(0.5f * M_PI * mix);
outL = dryK * inL + wetK * yL;
outR = dryK * inR + wetK * yR;
```

HP filter (one‑pole)
```cpp
// y = a*y + b*(x - xz1)
```

M/S routing
```cpp
mid = 0.5f*(L+R); side = 0.5f*(L-R);
// apply RM/AM to chosen channel(s)
L = mid + side; R = mid - side;
```

---

## References
- Vital: modulation smoothing, stereo phase, SR‑aware LFOs
- Classic ring modulators (Moogerfooger MF‑102, software emulations)
- Julius O. Smith: band‑limited waveforms and filter basics
