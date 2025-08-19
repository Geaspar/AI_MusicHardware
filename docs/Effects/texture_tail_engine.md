## Texture Tail Engine — Sparkle & Air Layer (Aug 2025)

A lightweight tail texturizer that layers sparse, randomized “velvet‑noise” clusters or micro‑grains behind an existing reverb/delay to add airy motion and perceived detail without raising density or decay. Designed as a post‑reverb insert (or internal sub‑module) with RT‑safe scheduling and bounded CPU.

### Goals
- Subtle, musical ‘sparkle’ and shimmer‑like air without pitch shifters
- Zero‑click operation; integrates cleanly after Hall/Plate/PrismVerb tails
- Very low CPU via sparse events and simple windows

---

## What it is (concept)
- Emits short, low‑level micro‑events (taps or 5–25 ms grains) at controlled density
- Each event is filtered (band‑pass / shelf) to sit in high bands (e.g., 4–16 kHz) or chosen band
- Timing and level are jittered slightly for organic motion; stereo placement randomized within width bounds
- Optionally ducks on input transients so texture blooms in gaps

---

## Parameters
- Mix (0–1, constant‑power)
- Density (events/s): 5 … 200 (bounded)
- Grain/Tap Length (ms): 5 … 50
- Band Center (Hz): 2 k … 16 k
- Band Width (oct): 0.3 … 2.0 (maps to BPF Q)
- Tone Tilt (−6 … +6 dB): high‑air emphasis vs sibilance guard
- Random Level (dB): 0 … 12 (per‑event amplitude variation)
- Jitter (ms): 0 … 30 (timing variation)
- Width (0 … 1): stereo spread of events
- Ducking (dB): 0 … 24 and Release (ms): 50 … 1000
- Sidechain Source (enum): Input, Post‑Reverb, External
- Mode (enum): Velvet (discrete taps), MicroGrain (windowed grains)
- Window (enum, micrograin): Hann, Triangle
- Output Trim (dB): −12 … +12

Suggested UI (2 pages)
- Page 1: Mix, Density, Length, Band Center/Width, Tone Tilt, Mode/Window
- Page 2: Random Level, Jitter, Width, Ducking Amt/Release, Sidechain, Output Trim

---

## Architecture

Event Scheduler
- Runs at block‑rate; schedules events using Poisson or jittered periodic process
- Enforces a max concurrent events cap for bounded CPU

Event Renderer
- Velvet mode: single tap shaped by a tiny 1‑pole or triangular window and BPF
- MicroGrain mode: short windowed grain with per‑grain BPF and pan; linear resample at 1.0 (no pitch)
- Per‑event level and pan randomized within bounds; width controls M/S spread

Filtering & Tone
- Band‑pass biquad (RBJ) with center/width; optional tilt post stage for air control
- Global low shelf or de‑esser option (future) to avoid sibilance buildup

Ducking
- Sidechain detector (input/post‑reverb/external) reduces texture wet gain during loud passages; smooth release

Mix & Output
- Accumulate event sum into wet buffer; constant‑power Mix with dry; Output Trim

Smoothing & safety
- Smooth parameters (10–50 ms); denormal guards; pre‑allocate event pool; no RT allocations
- Density cap and envelope windows prevent clicks and CPU spikes

---

## Mod Matrix Destinations
- Texture Mix, Density, Length, Band Center/Width, Tone Tilt, Random Level, Jitter, Width, Ducking Amt/Release, Mode (stepped), Window (stepped), Output Trim

---

## Integration Patterns
- As insert after Hall/Plate/PrismVerb
- As an internal sub‑path inside reverb cores, mixed post‑FDN (per‑band in PrismVerb for targeted air)

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/TextureTail.h`, `src/effects/TextureTail.cpp`; register; UI mapping (2 pages)

Phase 1 — Scheduler & Velvet Mode (1 day)
- Event pool, Poisson/jitter scheduling, tap renderer with BPF and pan; Mix/Trim; smoothing

Phase 2 — MicroGrain Mode & Ducking (0.5–1 day)
- Windowed micro‑grain with Hann/Tri; sidechain ducking; width control

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset persistence; CPU profiling; listening tests with Hall/Plate/PrismVerb; defaults

---

## Math & Pseudocode

Event scheduling (Poisson‑like)
```cpp
float lambda = density; // events per second
float dt = rngExp(lambda); // exponential inter-arrival
timeToNext -= blockSec;
if (timeToNext <= 0 && liveEvents < cap) spawnEvent(), timeToNext += dt;
```

Tap/grain window
```cpp
float w = hann(n/N); // or triangle
float y = w * bpf.process(inputTapOrNoise);
outL += panL * y; outR += panR * y;
```

Constant‑power mix
```cpp
wet = sinf(0.5f*M_PI*mix); dry = cosf(0.5f*M_PI*mix);
out = dry*x + wet*texture;
```

---

## References
- Velvet noise applications in reverb enhancement
- Granular synthesis basics for micro-grains
- RBJ filters; Vital: smoothing and modulation plumbing
