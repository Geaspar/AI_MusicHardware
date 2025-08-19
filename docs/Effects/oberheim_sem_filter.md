## Oberheim SEM‑Style Filter — 12 dB State‑Variable With Morph (Aug 2025)

A classic Oberheim SEM‑inspired multimode filter: 12 dB/oct state‑variable core with continuously variable mode morph (LP ↔ Notch ↔ HP) and dedicated Band‑Pass tap. Musical resonance that doesn’t thin too much, smooth modulation, and optional non‑linearities for vintage vibe. Vital reference: ZDF SVF patterns, smoothing, and drive staging.

### Goals
- Authentic SEM feel: liquid response, continuous mode morph, stable resonance
- 12 dB/oct slope with LP/HP/Notch morph and independent BP level
- Key tracking and envelope modulation; stereo‑safe variant
- Optional OTA/nonlinear flavor; oversampling when pushed

---

## Feature Overview
- Core: 2‑pole (12 dB/oct) ZDF state‑variable filter producing LP, BP, HP simultaneously
- Mode Morph: continuous control LP ↔ Notch ↔ HP (0.0…1.0), with BP mix control
- Resonance (Q): musical up to near‑oscillation (self‑osc optional)
- Drive: input and core drive; subtle OTA‑style saturation option
- Key Track (0–100%) and EG Amount (±)
- Stereo: linked L/R, optional micro detune/offset for width
- Mix (parallel) and Output Trim
- Oversampling: Off/2× auto when Drive+Res are hot

---

## Parameters
- Mix (0–1, constant‑power)
- Cutoff (Hz): 20 … 20 kHz (internally warped for ZDF)
- Resonance (0 … 1.1)
- Morph (0 … 1): LP (0) ↔ Notch (0.5) ↔ HP (1)
- BP Level (−inf … 0 dB): band‑pass contribution level
- Drive (dB): 0 … +24
- Flavor (enum): Clean, OTA (soft sat)
- Key Track (%): 0 … 100
- EG Amount (−1 … +1)
- Stereo Offset (0 … 0.5%): tiny L/R cutoff tolerance
- Oversampling (enum): Off, 2×
- Output Trim (dB): −24 … +24

Suggested UI (2 pages)
- Page 1: Mix, Cutoff, Resonance, Morph, BP Level, Drive
- Page 2: Flavor, Key Track, EG Amount, Stereo Offset, Oversampling, Output Trim

---

## Architecture

ZDF State‑Variable Core
- Use TPT/ZDF SVF equations producing yLP, yBP, yHP each sample
- Resonance via normalized feedback (stable up to oscillation);
- Cutoff mapping: g = tan(π f_c / f_s)

Mode Morph & Output Mix
- Notch = LP + HP with variable balance
- Morph control m ∈ [0,1]:
  - wLP = clamp(1 − 2m, 0, 1)
  - wHP = clamp(2m − 1, 0, 1)
  - wNotch = 1 − |2m − 1|  (crossfade LP/HP around 0.5)
- Output = norm( wLP*yLP + wHP*yHP + wNotch*(α*LP + (1−α)*HP) ) + bpGain*yBP
  - Choose α = 0.5 for symmetrical notch; scale to preserve loudness

Non‑linearities & Drive
- Pre‑drive (dB) into SVF; optional OTA soft clip inside integrator update or feedback term
- Oversampling around non‑linear core when Drive+Res high; minimum‑phase up/down to reduce latency

Modulation & Tracking
- KeyTrack multiplier: 2^(track%*(note−ref)/12)
- EG Amount applied additively to cutoff; smoothed 10–30 ms

Stereo & Safety
- Optional tiny cutoff offsets per channel for width; mono‑safe
- Clamp resonance to stable range in non‑OS mode; denormal guards; no allocations in process

Mix Law
- Constant‑power parallel mix (sine/cosine)

---

## Mod Matrix Destinations
- SEM Mix, Cutoff, Resonance, Morph, BP Level, Drive, Flavor (stepped), Key Track, EG Amount, Stereo Offset, Oversampling (stepped), Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/SemFilter.h`, `src/effects/SemFilter.cpp`; factory + UI mapping

Phase 1 — SVF Core & Morph (1 day)
- Implement ZDF SVF (LP/BP/HP); morphing mixer (LP↔Notch↔HP) + BP level; smoothing; Mix/Trim

Phase 2 — Character & OS (0.5–1 day)
- Add OTA flavor nonlinearity and oversampling around core when pushed; stereo offset option

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset round‑trip; key tracking/EG mapping; listening tests and CPU profiling; finalize ranges

---

## Math & Pseudocode

SVF (simplified)
```cpp
float g = tanf(PI * fc / fs);
float R = 1.0f / Q; // resonance mapping
// TPT SVF update computing yLP,yBP,yHP with feedback normalization
```

Morph mixer
```cpp
float m = morph; // 0..1
float wLP = clamp(1.0f - 2.0f*m, 0.0f, 1.0f);
float wHP = clamp(2.0f*m - 1.0f, 0.0f, 1.0f);
float wN  = 1.0f - fabsf(2.0f*m - 1.0f);
float notch = 0.5f*yLP + 0.5f*yHP; // symmetric notch
float out = norm(wLP*yLP + wHP*yHP + wN*notch) + bpGain*yBP;
```

Mix (constant‑power)
```cpp
float wet = sinf(0.5f*M_PI*mix), dry = cosf(0.5f*M_PI*mix);
out = dry*in + wet*out;
```

---

## References
- Oberheim SEM filter analyses (continuous mode morphing)
- Zavalishin: The Art of VA Filter Design (TPT/ZDF SVF)
- Vital: SVF/drive/smoothing practices
