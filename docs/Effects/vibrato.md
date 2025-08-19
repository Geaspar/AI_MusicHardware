## Vibrato — Musical Pitch Modulation Effect (Aug 2025)

A lightweight vibrato modeled on classic rack/pedal units and tape wow/flutter, with modern stability and stereo options. Implemented as a high‑quality modulated delay (fractional) with SR‑invariant depth and click‑free behavior. Vital reference: uses clean LFOs and SR‑aware modulation paths; we adopt similar smoothing and stereo phase handling.

### Goals
- Smooth, musical pitch modulation with no clicks or zipper noise
- SR‑invariant depth (cents/semitones) and precise rate control
- Stereo/bi‑phase modes for width without mono issues
- Optional tape/BBD flavors (wow/flutter/soft HF roll‑off)
- CPU‑lean and RT‑safe

---

## Core Features
- Depth: in cents (±0…±100 cents) or semitones (0…2 st)
- Rate: Hz (0.05–12 Hz), tempo sync (note divisions)
- Waveform: Sine, Triangle, Square (band‑limited), Ramp Up/Down (band‑limited), Random S&H (smoothed)
- Stereo Phase: 0–180° between L/R
- Mix: 0–1 (for chorus‑like blends); 1.0 = pure vibrato (no dry)
- Pre‑emphasis/Tone: gentle LP tilt for tape/BBD vibe (optional)
- Humanize: subtle random drift (wow/flutter layer)

Enhancements (optional)
- Tape Mode: slow wow (0.2–0.6 Hz) + flutter (6–8 Hz) low‑depth blend; LP tilt ~6–10 kHz
- BBD Mode: mild HF roll‑off and tiny clock jitter emulation

---

## Parameters
- Mix (0–1)
- Depth (cents): 0–100 (map to samples via SR & base delay)
- Rate (Hz) / Sync Division (enum)
- Waveform (enum): Sine/Tri/Square/RampUp/RampDown/Random
- Stereo Phase (deg, 0–180)
- Humanize (0–0.5%) — rate drift
- Mode (enum): Clean, Tape, BBD
- Tone (0–1) — LP tilt amount (mode‑dependent)
- Output Trim (dB, −12…+6)

Suggested UI (2 pages)
- Page 1: Mix, Depth, Rate/Sync, Waveform, Stereo Phase
- Page 2: Humanize, Mode, Tone, Output Trim

Tempo sync
- Divisions: 1/64 … 8 bars + dotted/triplet variants

---

## Architecture
- Modulated Delay per channel with fractional read (Lagrange3 or linear fallback)
- Base delay d0 small (e.g., 6–12 ms) to avoid comb coloration
- Instantaneous delay d(t) = d0 + depthSamples · m(t)
  - depthSamples = centsToSamples(Depth, SR) with small‑angle approx
  - m(t) ∈ [−1, 1] from LFO (per‑block advanced); right channel LFO phase = left + StereoPhase
- Humanize: slow noise/wander added to rate or phase at low depth; optional flutter layer in Tape mode
- Tone: 1‑pole LP on wet to emulate bandwidth limits (mode dependent)
- Mix: dry/wet crossfade (constant‑power)
- Output Trim: post wet/dry mix

SR‑invariant depth mapping
- Pitch shift in semitones s(t) relates to delay modulation approximately by
  - instantaneous fractional change ≈ d'(t)/d0 ⇒ Δf/f ≈ −d'(t)/d0
  - For small vibrato depth, calibrate depthSamples so perceived cents match across SR. In practice: depthSamples = k·d0 where k chosen so ±100 cents ≈ target depth for sine LFO. Provide ear‑tuned constant.

Anti‑click policies
- Smooth all parameters (Depth/Rate/Mix/Phase/Wave) 10–50 ms
- Band‑limited edges for Square/Ramps (minBLEP or short crossfade)
- Denormal guards; no allocations in process

---

## Mod Matrix Destinations
- Vibrato Mix, Depth, Rate, Waveform, Stereo Phase, Humanize, Tone, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/Vibrato.h`, `src/effects/Vibrato.cpp` and register in factory; basic UI (two pages)

Phase 1 — Core Modulated Delay (1 day)
- Fractional delay line with readFrac (Lagrange3)
- LFO (sine/tri/square/ramp/random), stereo phase, depth mapping to samples
- Mix (constant‑power), Tone LP, Output Trim

Phase 2 — Modes & Humanize (0.5–1 day)
- Tape/BBD modes; add wow/flutter blend; rate drift
- Band‑limited transitions for non‑sine shapes

Phase 3 — Integration & Safety (0.5 day)
- Mod matrix destinations; preset save/load; smoothing; denormal guards

Phase 4 — Verification & Tuning (0.5 day)
- Depth vs cents calibration across SR
- Subjective tests versus known plugins; mono compatibility and stereo width
- CPU profiling; finalize ranges

---

## Math & Pseudocode

LFO advance and stereo phase
```cpp
phaseL += twoPi * rateHz * (blockSize / sr);
phaseL = fmodf(phaseL, twoPi);
phaseR = fmodf(phaseL + degToRad(phaseDeg), twoPi);
```

Fractional read
```cpp
float delayL = baseSamp + depthSamp * lfo(phaseL);
float delayR = baseSamp + depthSamp * lfo(phaseR);
float yL = delayLineL.readFrac(writeIndexL - delayL); // wraps
float yR = delayLineR.readFrac(writeIndexR - delayR);
```

Mix (constant‑power)
```cpp
float wetK = sinf(0.5f * M_PI * mix);
float dryK = cosf(0.5f * M_PI * mix);
outL = dryK * inL + wetK * yL;
outR = dryK * inR + wetK * yR;
```

Mode tone shaping
```cpp
if (mode == Tape || mode == BBD) {
  yL = onePoleLP.process(yL, toneFc);
  yR = onePoleLP.process(yR, toneFc);
}
```

Humanize
```cpp
float drift = noiseLFO.next() * humanizePct; // ±pct
rateHzEff = rateHz * (1.0f + drift);
```

---

## References
- Vital: SR‑aware LFOs and modulation smoothing; stereo phase practices
- Classic vibrato pedals/racks; tape wow/flutter literature
- Julius O. Smith: Fractional delay interpolation (Lagrange/Thiran)
