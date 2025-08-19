## Granular — Real‑Time Grain Processor (Aug 2025)

A creative granular effect for time‑stretch, pitch‑shift, freeze, and texture. Real‑time circular buffer captures input and spawns overlapped grains with windowing, randomization, and modulation. Vital reference: RT‑safe modulation patterns and parameter smoothing.

### Goals
- Smooth, click‑free grains with high‑quality windows and overlap
- Musical controls: density, size, pitch, spray/jitter, position, freeze
- Stereo‑aware grains and width options
- RT‑safe with bounded CPU; graceful under heavy modulation

---

## Core Features
- Buffer: circular audio buffer (0.5–8.0 s) with read position control
- Grain Engine:
  - Size (ms): 10 … 500 (windowed; Hann/Blackman/Hamming)
  - Density (grains/s): 1 … 200 (bounded)
  - Overlap: automatic from size/density to avoid holes/clips
  - Pitch (semitones): −24 … +24 (resample or phase‑vocoder per grain)
  - Position (0–1) with Scan Rate for slow motion; or Spray/Jitter (ms)
  - Randomization: size/pitch/pan variations
  - Stereo: per‑grain pan or independent L/R spawn
- Freeze: locks write head; position scans within frozen buffer
- Feedback: route portion of output back into buffer (with tone)
- Tone: LP/HP per grain or global; width control
- Mix (constant‑power) and Output Trim

Modes
- Texture: high density, small grains, random spray
- Stretch: position locked + grains queued; pitch independent (phase‑consistent)
- Scatter: position jittered, wider pitch spread

---

## Parameters
- Mix (0–1)
- Buffer Length (s): 0.5 … 8.0
- Size (ms): 10 … 500
- Density (grains/s): 1 … 200
- Overlap (0.5 … 8.0): max concurrent grains multiplier (safety cap)
- Pitch (st): −24 … +24; Fine (cents): −100 … +100
- Position (0–1) and Scan Rate (Hz): 0 … 2
- Spray/Jitter (ms): 0 … 200 (position noise)
- Random Size (%): 0 … 100
- Random Pitch (st): 0 … 12
- Random Pan (%): 0 … 100
- Window (enum): Hann, Hamming, Blackman, Triangle
- Freeze (bool)
- Feedback (0 … 0.95) and Feedback Tone (LP/HP tilt)
- Stereo Width (0 … 1)
- Tone LP/HP (Hz)
- Output Trim (dB)

Suggested UI (2 pages)
- Page 1: Mix, Size, Density, Pitch/Fine, Position, Spray, Freeze
- Page 2: Random (Size/Pitch/Pan), Window, Scan Rate, Feedback/Tone, Width, LP/HP, Output Trim

---

## Architecture

Real‑time Buffer
- Interleaved stereo circular buffer with write pointer; freeze toggles write stop
- Read positions derived from Position + Spray; wrap with modulo

Grain Scheduler
- Spawns grains at rate = Density with randomization; respects Overlap cap
- Each grain stores read pointer, size, pitch factor, pan, window choice

Grain Renderer
- Per‑grain resampling: linear/Lagrange (Eco) or small phase‑vocoder (HQ)
- Apply window over grain lifespan; pan; mix into output with accumulation buffer
- Optional per‑grain LP/HP or global tone after mix

Feedback
- Mix portion of output back into buffer with tone to avoid howl

Smoothing & safety
- Smooth parameters 10–50 ms; clamp Density/Overlap; denormal guards; no allocations in audio thread (pre‑allocate grain pool)

CPU modes
- Eco: linear resample, Hann window, lower density cap
- High: Lagrange3 or small PV grains, Blackman window, higher cap

Mix law
- Constant‑power wet/dry; Output Trim post

---

## Mod Matrix Destinations
- Granular Mix, Size, Density, Pitch/Fine, Position, Spray, Random Size/Pitch/Pan, Window (stepped), Freeze (stepped), Scan Rate, Feedback, Width, LP/HP, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5–1 day)
- Add `include/effects/Granular.h`, `src/effects/Granular.cpp`; factory + UI; pre‑allocate grain pool

Phase 1 — Buffer & Scheduler (1 day)
- Circular buffer with freeze; scheduler spawning per Density with randomization and caps

Phase 2 — Grain Rendering (1–1.5 days)
- Resampling per grain (Eco/High), windows, panning; accumulation buffer; stereo width

Phase 3 — Feedback & Tone (0.5 day)
- Feedback loop with tone; global LP/HP; Mix/Trim; smoothing/denormals

Phase 4 — Integration & Tuning (0.5–1 day)
- Mod destinations; preset persistence; stress tests for CPU; audible tuning for modes (Texture/Stretch/Scatter)

---

## Math & Pseudocode

Spawn timing
```cpp
spawnInterval = sr / max(1.0f, density);
nextSpawnIn -= blockSize;
if (nextSpawnIn <= 0 && liveGrains < maxGrains) spawnGrain();
```

Grain read and window
```cpp
for (n=0; n<grainLen; ++n){
  float t = n / (grainLen-1);
  float w = windowFn(t);
  float sample = readBuffer(readPos);
  sample = resample(sample, pitchFactor);
  outL += panL * w * sample;
  outR += panR * w * sample;
  readPos += pitchFactor; // wrap
}
```

Constant‑power mix
```cpp
wet = sinf(0.5f*M_PI*mix); dry = cosf(0.5f*M_PI*mix);
outL = dry*inL + wet*granL; outR = dry*inR + wet*granR;
```

---

## References
- Roads, Curtis: Microsound (granular synthesis)
- TDR/Output/AudioDamage granular tools (UX inspiration)
- Vital: modulation and smoothing patterns
