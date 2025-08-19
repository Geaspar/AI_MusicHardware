## OrbitVerb — Motion‑Mapped Reverb (Aug 2025)

A performance‑first reverb where motion is the instrument. OrbitVerb adds motion lanes and scenes on top of our reverb cores (Hall/Plate/PrismVerb), allowing tempo‑locked parameter paths for evolving spaces: pulsing width, breathing size, drifting diffusion, morphing damping. Designed for expressive sound design and live sets with RT‑safe, click‑free lanes. Vital reference: SR‑aware LFOs, smoothing, and modulation plumbing.

### Concept
- Traditional reverbs offer static knobs; OrbitVerb offers motion lanes for selected parameters, synced to tempo or driven by envelopes.
- Scenes (A/B) let you morph entire reverb states on a timeline or with a macro control.

---

## Motion System Overview
- Lanes: up to 4 editable parameter lanes (choose targets per lane)
  - Targets (per core):
    - Hall/Plate: Predelay, Size, Diffusion, Mod Rate, Decay (RT60), High Damping, Bass Mult, Width, Output Trim
    - PrismVerb: Global Size/Width/Coupling, Per‑Band Decay/HiDamp/BassMult/Diffusion/Level
  - Curves: Step, Linear, Exponential, S‑Curve
  - Timing: tempo divisions (1/64 … 8 bars), free Hz, or one‑shot (attack/decay shape)
  - Depth and Offset per lane; per‑lane phase; loop or one‑shot retrigger
- Scenes: A/B snapshots of all targeted parameters with morph control (Macro knob or timed crossfade)
- Mod Sources: LFO blend, Envelope follower, MIDI CC/MPE; lanes can sum with sources

---

## Parameters
- Core Select (enum): Hall, Plate, PrismVerb (uses existing cores)
- Mix (0–1, constant‑power), Output Trim (dB)
- Scene A/B selector and Morph (0–1) / Morph Time (ms)
- Lanes (x4):
  - Enabled (bool), Target (enum), Range (min..max scaling), Depth (0..1), Offset, Phase (deg)
  - Curve (enum): Step/Linear/Exp/S‑Curve; Slew (ms) for extra smoothing
  - Timing: Rate (Hz) or Sync Division (enum) and Length (bars)
  - Mode: Loop, One‑Shot (with Retrigger), Ping‑Pong
- Global sources: LFO Rate/Depth, Env Follow amount, MIDI CC map (optional)

UI (Pages)
- Page 1 (Core): Core type, key core params (contextual) + Mix/Trim
- Page 2 (Lanes 1–2): lane editors (target/range/curve/timing)
- Page 3 (Lanes 3–4 + Sources): lane editors + global LFO/EnvFollow
- Page 4 (Scenes): capture A/B, set Morph or auto‑morph time

Quality & Safety
- Quality: Eco/Normal/High routes to chosen reverb core mode (e.g., Hall smoothing resolution; PrismVerb bands/lines)
- All lane outputs smoothed (10–50 ms) before mapping to core to ensure click‑free motion

---

## Architecture

Core Integration
- OrbitVerb wraps a selected reverb core instance (Hall/Plate/PrismVerb)
- A motion engine runs per block to update target parameters, writing to the core via RT‑safe setters

Lane Engine
- Each lane produces a normalized motion signal m(t) in [0,1] from its timing/curve spec
- Range maps m(t) to parameter domain [pMin, pMax]; Offset shifts
- Per‑lane Phase and Mode (Loop/One‑Shot/Ping‑Pong) control playback behavior
- Final target value = Base (scene) + Depth · (RangeMap(m)) (clamped)

Scenes & Morph
- Scene A/B hold full parameter baselines for targeted params
- Morph crossfades baselines before lane modulation is applied
- Morph can be manual (macro) or timed (auto‑morph over Morph Time)

Sources & Summing
- Optional global LFO and Env Follow are added (with weights) into each lane’s normalized m(t) before RangeMap
- External modulation (mod matrix) can address lane Depth/Phase/Rate or core params directly

Smoothing & RT safety
- Lane signals computed at block‑rate; parameter smoothing (slew limiter) applied before calling core setters
- No allocations during audio; denormal guards present

---

## Mod Matrix Destinations
- OrbitVerb: Mix, Output Trim, Core Select (stepped), Quality (stepped)
- Scenes: Morph, Morph Time; Capture triggers (stepped)
- Lanes (1–4): Enabled, Target (stepped), Depth, Offset, Phase, Rate, Sync Division (stepped), Curve (stepped), Range Min/Max, Mode (stepped), Slew
- Sources: LFO Rate/Depth, Env Follow amount

---

## Implementation Plan

Phase 0 — Scaffolding (0.5–1 day)
- Add `include/effects/OrbitVerb.h`, `src/effects/OrbitVerb.cpp`; factory; UI (multi‑page)

Phase 1 — Lane Engine (1 day)
- Implement lane shapes, timing (sync/Hz), phase, ping‑pong, one‑shot; smoothing & clamping; Range mapping

Phase 2 — Scene System (0.5–1 day)
- Capture/recall A/B baselines; Morph control (manual & timed)

Phase 3 — Core Glue (0.5–1 day)
- Map lane outputs to selected reverb core setters; ensure order of ops (Scene → Lanes → Core); RT‑safe

Phase 4 — Sources & Integration (0.5 day)
- Global LFO and EnvFollow blend into lanes; mod matrix endpoints; preset persistence

Phase 5 — Tuning & UX (0.5 day)
- Defaults and templates (Pulse Width, Breathing Hall, Shimmer Sway); CPU profiling; polish value formatters

---

## Math & Pseudocode

Lane shape
```cpp
float phase = (t * rate + phaseOffset) mod 1.0f;
float m = applyCurve(phase, curve); // 0..1, step/lin/exp/s-curve
```

Range mapping & smoothing
```cpp
float target = lerp(pMin, pMax, m) * depth + offset;
float smoothed = slewLimiter.process(target, slewMs, sr);
core.setParameter(paramId, smoothed);
```

Scene morph
```cpp
base = (1-morph) * sceneA[param] + morph * sceneB[param];
value = clamp(base + laneContribution, min,max);
```

---

## References
- Our Hall/Plate/PrismVerb implementations
- Automation lane UX from DAWs; macro morph controls (e.g., X/Y pads)
- Vital: LFOs, envelopes, smoothing and modulation plumbing
