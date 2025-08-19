## ER Designer — Geometry‑Seeded Early Reflections (Aug 2025)

A fast, musical early‑reflections (ER) generator that creates tap clusters from lightweight geometric hints ("seeds"). Use stand‑alone for tight rooms and slap, or place before algorithmic late tails (Hall/Plate/PrismVerb) to add spatial cues and depth. The system combines image‑source approximations, randomized jitter, and stereo decorrelation with psychoacoustic safeguards. Vital reference: smoothing and RT‑safe modulation.

### Concept
- Provide a few high‑level geometry seeds (Room, Hall, Plate Chamber, Stairwell, Studio Booth) and generate a clustered ER pattern (tap times, gains, widths, HF damping) instantly.
- Tweak with Size/Aspect/Absorption and Randomness; lock to Predelay and ER Level/Width.

---

## Feature Overview
- Geometry Seeds (enum):
  - Booth (small/short), Room (medium), Hall (large), Stairwell (multi‑bounce feel), Plate Chamber (dense short)
- Controls:
  - Predelay (ms), ER Level, ER Width
  - Size (m), Aspect (W/L ratio), Height (m)
  - Absorption (0..1): maps to HF damping per bounce order
  - Randomness (0..1): timing and gain jitter
  - Density (taps): 8 .. 64 (seed‑dependent default)
  - Stereo Decorrelate (0..1): inter‑aural decorrelation strength
- Filters:
  - HF Damping per tap order; global low‑cut (10..80 Hz)
  - Optional notch/tilt to avoid harsh combing
- Mix (constant‑power) and Output Trim

Enhancements (optional)
- Mic Position (x,y,z) inside room fraction coordinates
- Wall Material preset (Plaster, Brick, Curtain) mapping to absorption curves
- Seed Shuffle (reroll tap jitter with same room)

---

## Parameters
- Mix (0–1, constant‑power)
- Predelay (ms): 0 .. 100
- ER Level (0..1)
- ER Width (0..1)
- Seed (enum): Booth, Room, Hall, Stairwell, PlateChamber
- Size (m): 3 .. 60
- Aspect (W/L): 0.5 .. 2.0
- Height (m): 2 .. 12
- Absorption (0..1)
- Density (taps): 8 .. 64
- Randomness: 0 .. 1
- Decorrelate: 0 .. 1
- LowCut (Hz): 10 .. 80
- Output Trim (dB): −12 .. +12

Suggested UI (2 pages)
- Page 1: Mix, Predelay, ER Level/Width, Seed, Density, Randomness, Decorrelate
- Page 2: Size, Aspect, Height, Absorption, LowCut, Output Trim

---

## Architecture

Tap Synthesis (image‑source inspired)
- Compute base delays from simple room model: direct + early order reflections (up to 2nd/3rd order)
- Times t_i ≈ (|R·n + p| / c) where c is sound speed, R room dims, p source/listener vectors, n reflection index
- Map geometry to a canonical tap list (L/R) with per‑tap gain g_i ∝ 1/d_i and absorption^order
- Apply Randomness:
  - Timing: ±J ms jitter (seeded RNG) per tap order
  - Gain: small dB variations; ensure monotonic decay on average
- Apply Decorrelate: slightly different jitter and gains per channel; small allpass on one side if needed

Psychoacoustic safeguards
- Enforce minimum spacing between strong taps to avoid harsh comb filters
- Apply HF damping increasing with order; optional low‑cut on the sum
- Normalize ER Level; ensure unity gain at 0 dB ER Level when Mix=100%

Processing
- Build tap buffer each block or when params change; render by reading a short multi‑tap delay line with per‑tap gains and per‑channel pans
- Predelay applied to whole cluster
- Constant‑power Mix + Output Trim

Smoothing & safety
- Smooth ER Level/Width/Predelay/absorption (10–30 ms); rebuild taps with crossfade when geometry or density changes
- Denormal guards; no allocations in audio thread; pre‑allocated tap pool up to max Density

Integration
- Place before late tail (Hall/Plate/PrismVerb) for best spatial realism
- Can be chained with Texture Tail Engine for air

---

## Mod Matrix Destinations
- ERDesigner Mix, Predelay, ER Level, ER Width, Seed (stepped), Size, Aspect, Height, Absorption, Density, Randomness, Decorrelate, LowCut, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/ERDesigner.h`, `src/effects/ERDesigner.cpp`; factory; UI mapping (2 pages)

Phase 1 — Tap Model (1 day)
- Geometry→tap synthesis (first/second order); Randomness/Decorrelate; Predelay/ER Level/Width; Mix/Trim; smoothing

Phase 2 — Safeguards & Filters (0.5 day)
- HF damping vs order; LowCut; spacing rules; crossfade on rebuild; seed shuffle

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset persistence; listening tests with late tails; CPU profiling; finalize ranges

---

## Math & Pseudocode

Tap time and gain (sketch)
```cpp
for (order=0..K) for each wall combo:
  float d = distanceForOrder(order, size, aspect, height, micPos);
  float t = d / c + predelay;
  float g = baseGain / (1.0f + d) * powf(1.0f - absorption, order);
  t += jitterMs(order); g *= gainJitter(order);
  taps.push({t,g,pan(order)});
```

Cluster render
```cpp
float erL=0, erR=0;
for (tap : taps) {
  float s = readFrac(delayBuffer, writeIndex - t*sr);
  erL += tap.gL * s;
  erR += tap.gR * s;
}
// width and decorrelation
```

Mix (constant‑power)
```cpp
wet = sinf(0.5f*M_PI*mix); dry = cosf(0.5f*M_PI*mix);
outL = dry*inL + wet*erL; outR = dry*inR + wet*erR;
```

---

## References
- Image‑source method (Allen & Berkley) simplified for low orders
- ER design in algorithmic reverbs; psychoacoustic spacing and HF damping
- Vital: smoothing and RT patterns
