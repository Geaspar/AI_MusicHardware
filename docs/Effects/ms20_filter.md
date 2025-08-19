## MS‑20 Style Filter — Dual HPF→LPF with Screaming Resonance (Aug 2025)

A character filter inspired by the Korg MS‑20: a 12 dB/oct High‑Pass feeding a 12 dB/oct Low‑Pass, both with strong resonance (“Peak”) capable of self‑oscillation. The hallmark tone comes from non‑linearities in the feedback path and gain staging around the filter cores. We reproduce the serial HP→LP topology, aggressive resonance, and drive coloration with modern stability. Vital reference: ZDF SVF patterns, drive stages, and per‑block smoothing.

### Goals
- Authentic MS‑20 topology (HP → LP serial) with independent Peak controls
- Non‑linear feedback path for “scream” and musical self‑oscillation
- Key tracking and env mod friendliness; stereo‑safe variant
- Optional OTA/diode models and oversampling for high‑resonance stability

---

## Feature Overview
- Topology: 2‑pole (12 dB/oct) HPF feeding 2‑pole LPF, each with Peak (resonance)
- Drive: input drive and per‑stage drive; optional asymmetric clip in feedback path
- Modes (optional):
  - Classic (Korg35‑style nonlinearity; hotter resonance)
  - OTA (cleaner resonance, slightly different gain law)
  - Clean (linear resonance path; modern stability)
- Key Track: 0–100%
- Env Mod: positive/negative EG amount to HP and/or LP (also via mod matrix)
- Stereo: linked L/R with optional subtle channel offsets for width
- Mix (parallel) and Output Trim
- Oversampling: Off/2× (auto when resonance/drive are hot)

---

## Parameters
- Mix (0–1, constant‑power)
- HP Cutoff (Hz): 10 … 5000
- HP Peak (resonance): 0 … 1.2 (self‑osc near 1.0+)
- LP Cutoff (Hz): 20 … 20000
- LP Peak (resonance): 0 … 1.2
- Drive (dB): 0 … +24 (pre), Stage Drive (0–1) (between HP and LP)
- Model (enum): Classic, OTA, Clean
- Key Track (%): 0 … 100
- EG Amount (dB/Oct proxy): −1 … +1 (mapped to cutoff)
- Oversampling (enum): Off, 2×
- Output Trim (dB): −24 … +24

Suggested UI (2 pages)
- Page 1: Mix, HP Cutoff/Peak, LP Cutoff/Peak, Drive, Stage Drive
- Page 2: Model, Key Track, EG Amount, Oversampling, Output Trim

---

## Architecture

Filter cores
- Use ZDF SVF or state‑variable biquads configured for HP (first stage) and LP (second stage)
- Implement 12 dB/oct behavior by using 2‑pole cores; ensure correct gain normalization

Non‑linear feedback
- Apply saturation in the resonance feedback path:
  - Classic: diode‑like clip (tanh with asymmetry)
  - OTA: symmetric soft clip with lower gain
- Stage drive between HP and LP to emulate inter‑stage clipping and tone shift
- Optional small DC block around feedback loop to avoid bias creep

Serial routing & gain staging
- Input → Pre‑drive → HPF(+nonlinear feedback) → Stage drive → LPF(+nonlinear feedback) → Trim
- Calibrate internal levels so Peak ranges reach self‑oscillation musically
- Oversampling only around non‑linear sections when engaged

Key tracking & envelopes
- KeyTrack: cutoff multiplier ~ 2^(track% * (note−ref)/12)
- EG Amount: add ± mapping to both (or selected) cutoffs; expose finer control via mod matrix

Smoothing & safety
- Smooth all params (cutoffs/peaks/drive) 10–30 ms
- Clamp resonance to prevent runaway in non‑OS mode; auto‑engage 2× OS at high Peak+Drive
- Denormal guards; no allocations in `process`

Stereo
- Default linked L/R; optional tiny cutoff offsets (<0.3%) for width; maintain mono compatibility

Mix law
- Constant‑power parallel mix for creative blends

---

## Mod Matrix Destinations
- MS20 Mix, HP Cutoff, HP Peak, LP Cutoff, LP Peak, Drive, Stage Drive, Model (stepped), Key Track, EG Amount, Oversampling (stepped), Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/MS20Filter.h`, `src/effects/MS20Filter.cpp`; register in factory; UI mapping (2 pages)

Phase 1 — Cores & Serial Path (1 day)
- ZDF SVF HP → LP with resonance; gain staging; Mix/Trim; smoothing; denormals

Phase 2 — Character (0.5–1 day)
- Nonlinear feedback (Classic/OTA); pre/stage drive; oversampling path when needed

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset round‑trip; key tracking/EG mapping; listening tests and CPU profiling

---

## Math & Pseudocode

ZDF SVF update (sketch)
```cpp
// HP stage
a = computeCoeff(HP_cutoff);
// nonlinear feedback term
float fb = sat(res_HP * hp_out);
// SVF step with feedback injection
```

Key tracking
```cpp
float cMul = powf(2.0f, keyTrack * (note - ref) / 12.0f);
HP_cut = baseHP * cMul + egAmt*EG;
LP_cut = baseLP * cMul + egAmt*EG;
```

Saturation
```cpp
float satClassic(float x){ return tanhf(x) + 0.1f*(x - tanhf(x)); }
```

Constant‑power mix
```cpp
wet = sinf(0.5f*M_PI*mix); dry = cosf(0.5f*M_PI*mix);
out = dry*x + wet*y;
```

---

## References
- Korg MS‑20 filter analyses (HP→LP serial topology, resonance behavior)
- Zavalishin: The Art of VA Filter Design (ZDF SVF techniques)
- Vital: filter cores and drive paths; parameter smoothing
