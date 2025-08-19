## PrismVerb — Spectral FDN Reverb (Aug 2025)

A multi‑band Feedback Delay Network (SFDN) reverb that splits the signal into coarse spectral bands and runs a compact FDN per band with band‑coupled parameters. Delivers articulate low‑end bloom, airy highs, and controlled mid density without heavy multiband crossover complexity. Inspired by modern algorithmic reverbs and our FDN Hall; extends with per‑band RT60 shaping and optional cross‑band coupling.

### Why PrismVerb
- Great reverbs often need different decay behaviors by frequency. SFDN provides that directly while keeping late‑field density and stability.
- Compared to single‑band FDNs with per‑loop filters, SFDN offers more intuitive per‑band control and clearer artistic outcomes.

---

## Core Architecture
- Band Split: 3–5 bands via LR‑tilt + shelves or 24 dB/oct LR (Linkwitz–Riley) crossovers for phase‑coherent recombine.
- Per‑Band FDN: N=4/6/8 delay lines, Householder/Hadamard mixing, per‑loop HF damping and LF shaping.
- Modulation: per‑delay LFOs with decorrelated phases and small depth.
- Cross‑Band Coupling (optional): limited crossfeed between adjacent bands for cohesion.
- Output: band sums with width control and wet normalization; constant‑power Mix; Output Trim.

CPU Modes
- Eco: 3 bands × N=4, lighter modulation
- Normal: 4 bands × N=6 (default)
- High: 5 bands × N=8, higher prewarm and smoothing

---

## Parameters
- Mix (0–1, constant‑power), Output Trim (dB)
- Size (0.5 … 2.0): scales delays across all bands
- Width (0 … 1): stereo field shaping
- Bands (enum): 3, 4, 5 (quality mode)
- Crossover Freqs: B1/B2/B3 (exposed when Bands>3)
- Cross‑Coupling (0 … 1): adjacent band crossfeed amount

Per‑Band (x #Bands)
- Decay (RT60 s): 0.2 … 20
- High Damping (0 … 1): maps to LP cutoff 2–12 kHz
- Bass Mult (0.5 … 2.0): LF tilt for decay
- Diffusion (0 … 1): input allpass gain mapping
- Mod Rate (Hz): 0.05 … 1.0, Mod Depth (%): 0 … 0.25
- Level (dB): −12 … +12 (per‑band makeup)

Advanced
- Pre‑Delay (ms): 0 … 100
- ER Level (0 … 1) and Width (0 … 1) (optional ER block)
- Quality (enum): Eco, Normal, High (sets bands × lines, prewarm)

---

## UI & Pages
- Page 1 (Global): Mix, Size, Width, Bands, Cross‑Coupling, Predelay, Quality
- Page 2..(2+Bands−1): Per‑Band pages with Decay/HiDamp/BassMult/Diffusion/Mod/Level and crossover readouts

---

## DSP Details

Band Split / Recombine
- Prefer LR4 (24 dB/oct) Linkwitz–Riley filters for band edges to maintain in‑phase recombination.
- Alternative: simple shelving + tilt for lower CPU with acceptable phase trade‑offs (Eco).

Per‑Band FDN
- Delay set D_i scaled by Size and band; ensure unequal/co‑prime patterns per band.
- Householder mixing (energy‑preserving) and per‑loop RT60 mapping:
```cpp
// RT60 mapping per loop
g_i = powf(10.0f, -3.0f * (Td_i / T60_band));
// HF damping (one‑pole)
a_i = expf(-2π * fc / sr);
y_lp_i = a_i * y_lp_i + (1 - a_i) * y_i;
```
- Modulation per line with decorrelated rates and small depths; depth expressed as fraction of delay (SR‑invariant).

Cross‑Band Coupling
- Inject a small, low‑passed portion of band k into k±1 inputs to add coherence without smearing band character.

Wet Normalization
- Track wet RMS per band and globally; apply gentle normalization and limiter for consistent perceived loudness.

Constant‑Power Mix
```cpp
float wet = sinf(0.5f*M_PI*mix), dry = cosf(0.5f*M_PI*mix);
out = dry*in + wet*sumBands;
```

Smoothing & Safety
- Smooth global and band parameters (10–50 ms). Clamp extremes; denormal guards; pre‑allocate buffers; no RT allocations.

---

## Mod Matrix Destinations
- PrismVerb Mix/Size/Width/Cross‑Coupling/Predelay/Quality (stepped)
- Per‑Band: Decay, High Damping, Bass Mult, Diffusion, Mod Rate, Mod Depth, Level

---

## Implementation Plan

Phase 0 — Scaffolding (0.5–1 day)
- Add `include/effects/PrismVerb.h`, `src/effects/PrismVerb.cpp`; factory; UI (multi‑page)

Phase 1 — Bands & Split (1 day)
- Implement LR4 band split/recombine; 3/4/5 bands; smoothing and parameter guards

Phase 2 — FDN per Band (1–1.5 days)
- N=6 default Householder FDN per band with RT60 mapping, HF damping, modulation; normalization; Mix/Trim

Phase 3 — Cross‑Band & ER (0.5 day)
- Cross‑coupling control (0..1); optional ER block with Predelay/Level/Width

Phase 4 — Integration & Tuning (0.5–1 day)
- Mod destinations; preset persistence; CPU profiling; listening tests; default band presets (Drum, Vocal, Pad)

---

## References
- Our FDN Hall implementation and notes
- Julius O. Smith: FDNs, Hilbert, and damping
- LR crossover design (Linkwitz–Riley)
- Valhalla‑style discussions on multi‑band late tails
