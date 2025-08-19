## Auto Wah — Envelope/Tempo-Driven Resonant Filter (Aug 2025)

A musical auto‑wah effect driven by an envelope follower and/or tempo‑synced LFO. Classic quack/talk tones with resonant band‑pass/low‑pass filters, pick sensitivity, and direction controls. Vital reference: envelope smoothing, filter building blocks, and modulation plumbing.

### Goals
- Responsive, click‑free envelope follower controlling a resonant filter
- Musical LP/BP/HP modes with adjustable Q (resonance)
- Pick sensitivity, attack/decay times, and sweep direction (up/down)
- Optional tempo‑sync LFO blend for rhythmic wah
- Low CPU, RT‑safe

---

## Core Features
- Modes: LP, BP, HP (primary: BP for classic auto‑wah)
- Resonance (Q) up to near self‑oscillation (safe clamp)
- Envelope Follower: attack/release, sensitivity/threshold, bias
- Sweep Range: min/max cutoff (Hz) or center + width (for BP)
- Direction: Up (open with louder input) / Down (close with louder input)
- LFO Blend: rate (Hz/sync), depth; stereo phase offset for auto‑pan like motion
- Tone/Tilt: pre/post emphasis to tame harshness or muddy lows
- Mix (constant‑power) and Output Trim

---

## Parameters
- Mix (0–1, constant‑power)
- Mode (enum): LP, BP, HP
- Resonance (0 … 1.0)
- Min Cutoff (Hz): 50 … 5k (LP/BP)
- Max Cutoff (Hz): 500 … 20k (LP/BP)
- For HP mode, interpret as Min/Max HP cutoff range
- Sensitivity (dB): −24 … +24 (envelope input gain)
- Attack (ms): 0.5 … 50
- Release (ms): 10 … 1000
- Direction (enum): Up, Down
- Bias (−1 … +1): offsets the envelope before mapping
- LFO Rate (Hz) / Sync Division (enum)
- LFO Depth (0 … 1)
- Stereo Phase (deg, 0–180) for LFO
- Tone Tilt (−6 … +6 dB)
- Output Trim (dB): −12 … +12

Suggested UI (2 pages)
- Page 1: Mix, Mode, Resonance, Min/Max, Direction, Sensitivity, Attack/Release
- Page 2: LFO Rate/Sync, LFO Depth, Stereo Phase, Tone Tilt, Output Trim

---

## Architecture
- Filter Core: ZDF SVF configured for LP/BP/HP with resonance Q
- Envelope Follower: rectifier + one‑pole AR smoothing; sensitivity and bias applied pre/post
- Mapping: env → cutoff within [Min, Max] (or center/width); apply Direction and Bias
- LFO: optional additive modulation to cutoff; stereo phase offsets R for width
- Tone Tilt: simple HP/LP tilt pre or post filter depending on Mode
- Mix: constant‑power law; Output Trim after

Smoothing & safety
- Smooth all params (10–30 ms); clamp cutoff to valid range; denormal guards
- Crossfade on Mode switch to prevent clicks

---

## Mod Matrix Destinations
- AutoWah Mix, Mode (stepped), Resonance, Min/Max Cutoff, Sensitivity, Attack/Release, Direction (stepped), Bias, LFO Rate/Depth, Stereo Phase, Tone Tilt, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/AutoWah.h`, `src/effects/AutoWah.cpp`; register in factory; UI mapping (2 pages)

Phase 1 — Core Env→Filter (1 day)
- SVF core, envelope follower with AR smoothing, mapping env→cutoff (incl. Direction/Bias), Mix/Trim; smoothing; denormals

Phase 2 — LFO & Stereo (0.5 day)
- Add LFO rate/depth with sync and stereo phase option; Tone Tilt; mode crossfade

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset persistence; listening tests on guitar/bass/synth; CPU profiling; finalize ranges

---

## Math & Pseudocode

Envelope follower
```cpp
float x = fabsf(in) * sensLin;
// one-pole attack/release
float coeffA = expf(-1.0f/(att_ms*1e-3f*sr));
float coeffR = expf(-1.0f/(rel_ms*1e-3f*sr));
if (x > env) env = coeffA*env + (1-coeffA)*x; else env = coeffR*env + (1-coeffR)*x;
```

Cutoff mapping
```cpp
float e = clamp(env + bias, 0.0f, 1.0f);
float cut = (direction==Up) ? lerp(minHz, maxHz, e) : lerp(maxHz, minHz, e);
// add LFO: cut += depth * lfo();  // convert to Hz appropriately
```

SVF update (simplified)
```cpp
// compute yLP,yBP,yHP with ZDF SVF
y = (mode==LP)? yLP : (mode==BP? yBP : yHP);
```

Mix (constant‑power)
```cpp
wet = sinf(0.5f*M_PI*mix); dry = cosf(0.5f*M_PI*mix);
out = dry*in + wet*y;
```

---

## References
- Classic auto‑wah pedals (Mutron, Q‑Tron, Cry Baby Auto Wah)
- Zavalishin: The Art of VA Filter Design (SVF)
- Vital: envelope and filter building blocks
