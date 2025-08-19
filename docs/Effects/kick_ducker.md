## KickDucker — Musical Sidechain Ducking/Pumper (Aug 2025)

A tempo-aware, kick-triggered ducking effect for clear low-end and rhythmic pump. Works with external sidechain (kick input) or internal envelope (tempo patterns). Designed to be transparent at modest settings and musical at extreme pump settings. Vital reference: smoothing and modulation routing patterns; adopt constant‑power mix and SR‑aware envelopes.

### Goals
- Fast, musical duck on kick hits with adjustable recovery
- Works with external sidechain or internal trigger pattern
- Zero-click gain moves; stereo-link and look‑ahead options
- Visual clarity: easy to dial in timing vs groove

---

## Core Features
- Sidechain Input: external (kick bus) with threshold, or internal pattern (1/4 notes etc.)
- Envelope Shaper: attack (hold), fall (release), curve (exp/lin) per trigger
- Depth: max gain reduction range (dB)
- Look‑Ahead: 0–10 ms (aligns duck w/ transient)
- Tempo Sync: pattern gate when no sidechain present (1/4, 1/8, triplet, dotted; swing)
- Frequency‑Selective Duck: sidechain HP/LP to focus kick band
- Stereo Link: linked gain for L/R or dual‑mono
- Makeup/Trim and Mix (for parallel pumping)

---

## Parameters
- Mode: External (Sidechain) / Internal (Pattern)
- Mix (0–1, constant‑power)
- Depth (dB): 0–24
- Attack/Hold (ms): 0–20 (pre-hold before release)
- Release (ms): 20–2000
- Curve (exp ↔ lin ↔ log): −1 … +1
- Look‑Ahead (ms): 0–10
- SC HP (Hz): 20–200
- SC LP (Hz): 2 k–12 k
- Threshold (dBFS): −60 … 0 (external mode)
- Pattern (enum): 1/4, 1/8, 1/8T, 1/4., 1/2, 1 bar, etc. (internal mode)
- Swing (%): 0–30
- Stereo Link (bool)
- Makeup (dB): −12 … +12
- Output Trim (dB): −12 … +6

Suggested UI (2 pages)
- Page 1: Mode, Mix, Depth, Attack/Hold, Release, Curve, Look‑Ahead
- Page 2: Threshold (ext), SC HP/LP, Pattern+Swing (int), Stereo Link, Makeup, Output Trim

---

## Architecture
- Sidechain path: external input → SC HP/LP → detector (peak/RMS hybrid) → gate
- Internal path: tempo clock → pattern gate generator (with swing) → detector envelope
- Gain computer: per trigger, apply hold then exponential/linear release to target GR (Depth), mapped by Curve param
- Look‑ahead: short delay line on main signal to align with computed gain
- Stereo: link option (use max detector of L/R) or dual‑mono
- Mix: constant‑power crossfade between dry and ducked (parallel pump option)
- Output: makeup gain and trim; denormal guards

Smoothing & safety
- All params smoothed (10–50 ms) except Pattern/Mode which crossfade gain over ~10 ms to avoid clicks
- Release envelope computed at block‑rate; gain updated per sample for smoothness
- Clamp GR to avoid overs; no allocations in process

---

## Mod Matrix Destinations
- KickDucker Mix, Depth, Attack, Release, Curve, Look‑Ahead, Threshold, SC HP/LP, Pattern, Swing, Makeup, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/KickDucker.h`, `src/effects/KickDucker.cpp`; factory + UI mapping

Phase 1 — External Sidechain (1 day)
- SC input bus plumbing; HP/LP; detector; look‑ahead delay; gain computer (Depth/Attack/Hold/Release/Curve)
- Mix/Makeup/Trim; stereo link

Phase 2 — Internal Pattern (0.5 day)
- Tempo divisions w/ swing; gate events mapped into envelope triggers

Phase 3 — Integration & Safety (0.5 day)
- Destinations; preset round‑trip; smoothing; crossfades for mode switch; denormals

Phase 4 — Verification & Tuning (0.5 day)
- Match common pump grooves; ensure kick clarity with 40–100 Hz heavy content; CPU/latency audit

---

## Math & Pseudocode

Detector (RMS w/ peak assist)
```cpp
float sc = hpLp(scIn);
float rms = sqrtf(avg(sc*sc)); // block smoother
float peak = maxAbs(scWindow);
float det = 0.7f*rms + 0.3f*peak;
```

Gain envelope
```cpp
if (triggered) {
  env = 1.0f; // full duck
  holdSamples = msToSamples(hold);
} else if (holdSamples > 0) {
  --holdSamples;
} else {
  // exponential/linear release towards 0.0
  float a = expf(-1.0f / (releaseMs*0.001f*sr));
  env = a*env; // exp; mix with linear based on Curve
}
```

Gain reduction and mix
```cpp
float grDb = -depthDb * env; // 0…-Depth
float grLin = dBToLinear(grDb);
// look-ahead delayed signal
float y = grLin * delayedX;
// constant-power parallel mix
float wetK = sinf(0.5f*M_PI*mix);
float dryK = cosf(0.5f*M_PI*mix);
out = dryK*x + wetK*y;
```

Pattern gate (internal mode)
```cpp
// generate triggers on musical grid with swing applied to off-beats
```

---

## References
- Vital: modulation smoothing and SR‑aware envelopes
- Sidechain duckers/pumpers (Xfer LFO Tool, Nicky Romero KICKSTART, DAW sidechain comps)
- Psychoacoustics of masking: focus duck on kick band using SC HP/LP
