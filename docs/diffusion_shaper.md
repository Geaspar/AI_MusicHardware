## Diffusion Shaper — Design, DSP, and Implementation Plan (Aug 2025)

This document defines a lightweight, musical "diffuser" effect focused on adding density, width, and motion without obvious reverb tails. It is intended as a standalone creative processor and as a utility block before/after reverbs, delays, and modulation FX.

### Why a Diffusion Shaper
- Thicken synths, pads, and plucks without long tails or obvious repeats
- Add width and motion with low CPU
- Pre-condition sources for reverbs to reduce metallic coloration
- Post-condition reverb/delay to increase perceived density without raising decay/mix

---

## Design Goals
- Lush, smear‑without‑mud: rapid build of micro‑echo density while preserving musical transients when desired
- Time variance: subtle modulation to avoid stationarity
- Stereo savvy: decorrelated L/R paths and width control with mono compatibility
- Stable and RT‑safe: no clicks/denormals, clamped feedback, predictable gain
- CPU‑lean: suitable for always‑on usage

---

## High‑Level Approaches

1) Serial Allpass Diffusers (primary)
- 2–4 first‑order allpasses per channel, short/medium base delays (2–20 ms)
- Gains 0.3–0.75 mapped from a "Diffusion" macro
- Fractional delay with light modulation (0–0.2% depth), decorrelated per stage/channel
- Optional feedback around the entire stack for more bloom (carefully clamped)

2) Parallel Micro‑Taps (optional layer)
- Sparse velvet‑noise style cluster mixed at low level post‑diffusers for subtle texture

3) Pre/Post Emphasis (tone)
- Gentle pre‑emphasis HP/Tilt into diffusers; de‑emphasis after to keep tone balanced at higher diffusion

Vital reference: Vital uses diffuser stages inside reverbs and modulation FX for density. We adopt similar serial allpass stages with per‑block parameter smoothing and SR‑invariant modulation depth.

---

## Parameters (v1)
- Mix (0–1, constant‑power)
- Diffusion (0–1 → maps to allpass gain 0.3…0.75)
- Time/Size (ms, 1–20 ms): scales base delays across stages
- Mod Rate (Hz, 0.05–2.0)
- Mod Depth (% of delay, 0–0.25)
- Stereo Width (0–1): decorrelated stage offsets and L/R cross mix
- Pre‑Emphasis (Tilt, −6…+6 dB across band)
- Drive (x) (1.0–3.0): subtle saturation post‑sum
- Output Trim (dB, −12…+6)
- Mode (selector):
  - Warm (lower fc emphasis, slower mod)
  - Airy (higher fc emphasis, faster/lighter mod)
  - Thick (adds mild feedback around stack)
  - Transient‑Safe (attacks preserved via envelope weighting)

MY IDEA: Transient‑Safe Diffusion — detect fast transients (envelope follower with short attack/fast release) and reduce diffusion gain for ~10–30 ms, then ramp back. Adds sheen without blurring attacks.

---

## Architecture & Signal Flow

- Input → Pre‑emphasis Tilt → [Allpass1 → Allpass2 → Allpass3 → Allpass4] (+ optional feedback) → Soft drive → De‑emphasis Tilt → Stereo width mix → Constant‑power Mix → Output

Implementation notes
- Each allpass stage:
  - First‑order allpass with fractional read; linear/3rd‑order Lagrange interpolation
  - Per‑stage base delay d_k = base_ms_k × SR; stage offsets differ per channel (×0.9…1.1)
  - Gain g_k mapped from Diffusion (0.3…0.75)
  - Modulation: phase‑decorrelated, depth as samples (ModDepth% × d_k)
- Global safeguards:
  - Clamp mode feedback ≤ 0.25
  - Denormal guards (+1e‑20f) at writes
  - RT‑safe smoothing (10–30 ms) for mix/diffusion/time, (100–250 ms) for width/tilt/drive

---

## UI & Pages (Effects Tab)
- Page 1: Mix, Diffusion, Time (ms), Stereo Width
- Page 2: Mod Rate (Hz), Mod Depth (%), Pre‑Emphasis (Tilt), Output Trim (dB)
- Mode selector (small dropdown)

---

## Parameter Mapping and Ranges
- Diffusion → allpass gain g = 0.3 + Diffusion × (0.75 − 0.3)
- Time/Size (ms) → per‑stage base delays (e.g., [3, 7, 11, 15] ms × Size)
- Mod depth (%) → samples = depth% × delaySamples
- Stereo width → mid/side shaper at output + per‑stage L/R offsets
- Constant‑power Mix: wet = sin(π/2·Mix), dry = cos(π/2·Mix)

---

## Implementation Plan (Phases)

Phase 0 — Scaffolding (0.5–1 day)
- Create `include/effects/DiffusionShaper.h`, `src/effects/DiffusionShaper.cpp`
- Inherit `Effect`; implement `process`, `setParameter`, `getParameter`, `getName`
- Register in `AllEffects.h` and add to `getAvailableEffects()`
- Add UI mapping in `src/main_integrated_simple.cpp` (two pages)

Phase 1 — Core Serial Diffusers (1 day)
- Implement 2–4 allpass stages per channel with fractional delay and modulation
- Map Diffusion, Time, Mod Rate/Depth; add Width and Mix (constant‑power)
- Add pre/de‑emphasis tilt (simple 1‑pole shelf/tilt)

Phase 2 — Modes & Safety (0.5–1 day)
- Add Mode switch (Warm/Airy/Thick/Transient‑Safe)
- Implement optional stack feedback (Thick), clamped and smoothed
- Add envelope follower and diffusion gain scaling for Transient‑Safe
- Add Drive and Output Trim

Phase 3 — Integration & Mod Matrix (0.5 day)
- Expose as modulation destinations: Diffusion, Time, Mod Rate, Mod Depth, Width, Drive, Trim, Mix
- Persist parameters in presets

Phase 4 — Performance & Stability (0.5 day)
- Denormal guards; per‑block precomputes; no allocations in `process`
- CPU profiling and small optimizations (hoist trig, cache coeffs)

Phase 5 — Verification & Tuning (0.5–1 day)
- IR of the shaper (short window) to verify smooth diffusion build
- Subjective tests on bass/plucks/pads; check attack preservation in Transient‑Safe mode
- Tune ranges/formatters

---

## Math Snippets

Allpass (first order)
```cpp
// y[n] = -g*x[n] + x[n-D] + g*y[n-D]
```

Fractional delay (3rd‑order Lagrange)
```cpp
float fracIndex = baseDelay + depth * sin(phase);
float y = delay.readFrac(fracIndex); // Lagrange3/linear fallback
```

Constant‑power Mix
```cpp
const float wet = sinf(0.5f * M_PI * mix);
const float dry = cosf(0.5f * M_PI * mix);
out = dry*in + wet*processed;
```

Transient‑Safe envelope scaling (concept)
```cpp
float env = attackFollower.process(fabsf(in));
float diffGain = lerp(minGain, baseGain, smoothStep(0, thresh, env));
```

---

## Time‑boxed Checklist
- [ ] Phase 0: Files, factory, UI wiring
- [ ] Phase 1: 4‑stage serial diffusers with modulation + tilt
- [ ] Phase 2: Modes (Warm/Airy/Thick/Transient‑Safe), drive, trim
- [ ] Phase 3: Mod matrix destinations, preset persistence
- [ ] Phase 4: CPU/RT safeguards; per‑block precomputes
- [ ] Phase 5: IR + listening tests; range polish

---

## References
- Vital: diffuser use inside reverb/modulation; per‑block smoothing patterns; SR‑aware modulation
- Dattorro (1997): diffuser and plate topologies
- Valhalla DSP posts: diffusion strategies and time variance
- Julius O. Smith: delay line interpolation and allpass structures
