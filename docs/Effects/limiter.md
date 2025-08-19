## Limiter — True-Peak Brickwall with Musical Release (Aug 2025)

A modern limiter for master bus or track safety. Combines look‑ahead brickwall limiting with true‑peak (ISP) detection, soft clip pre‑stage, and program‑dependent release. Designed to be transparent at moderate GR and graceful when pushed. Vital reference: per‑block smoothing and RT‑safe patterns.

### Goals
- True‑peak (inter‑sample) safety with oversampling ISP detector
- Clean brickwall ceiling with minimal distortion and pumping
- Soft‑clip pre‑stage for tone and additional headroom
- Low latency options and CPU‑aware oversampling

---

## Core Features
- Look‑Ahead: 0–10 ms (delay main path while predicting peaks)
- Ceiling: −12 … 0 dBFS (output ceiling)
- Threshold: −48 … 0 dBFS (or Input gain)
- Release: 10–2000 ms with program dependence (two‑time‑constant blend)
- ISP Detection: 2×/4×/8× oversampled true‑peak prediction
- Soft Clip: pre‑limiter waveshaper with drive (ON/OFF)
- Channel Link: AVG or MAX linking for stereo
- Dither/Noise‑floor guard (optional, off by default)
- Mix (parallel) and Output Trim
- GR Metering (peak and short‑term average), TP meter

Modes
- Eco: 2× ISP, low CPU
- Normal: 4× ISP (default)
- High: 8× ISP + enhanced release smoothing

---

## Parameters
- Mix (0–1, constant‑power)
- Threshold (dB): −48 … 0 (alt: Input Gain)
- Ceiling (dB): −12 … 0
- Look‑Ahead (ms): 0 … 10
- Release (ms): 10 … 2000
- Release Character (0–1): lean toward fast/slow or dual‑time behavior
- ISP Mode (enum): Off, 2×, 4×, 8×
- Soft Clip (bool) and Clip Drive (dB): 0 … 6
- Channel Link (enum): AVG, MAX
- Output Trim (dB): −12 … +6
- Meter (enum): IN, OUT, GR, TP

Suggested UI (2 pages)
- Page 1: Threshold, Ceiling, Look‑Ahead, Release, Release Character, Mix
- Page 2: ISP Mode, Soft Clip/Drive, Channel Link, Output Trim, Meter

---

## Architecture
- Split Path: main signal delayed by Look‑Ahead; side path computes predicted peak/true‑peak
- ISP Detection: oversample side path with polyphase FIR; compute envelope/peaks
- Gain Computer: compute required gain to keep output ≤ Ceiling; apply program‑dependent release
- Soft Clip Pre‑Stage: optional; applied before limiter gain for tone and headroom (with OS)
- Channel Link: link gain based on AVG or MAX of required L/R gain reductions
- Mix: constant‑power parallel; Output Trim after

Program‑dependent release
- Blend fast and slow envelopes based on recent GR; reduce pumping while releasing quickly after transients

Safeguards
- Denormal guards; no allocations in `process`
- Crossfade on mode changes (ISP factor/soft‑clip toggle)
- Clamp gain to safe ranges; meter true‑peak overs

Latency
- Look‑Ahead + ISP filter group delay (report total latency if host queried)

---

## Mod Matrix Destinations
- Limiter Mix, Threshold, Ceiling, Look‑Ahead, Release, Release Character, ISP Mode (stepped), Soft Clip/Drive, Channel Link, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/Limiter.h`, `src/effects/Limiter.cpp`; factory + UI mapping

Phase 1 — Core Limiter (1–1.5 days)
- Look‑ahead delay; peak/TP detection (ISP 2×/4×/8×); gain computer; release curve
- Mix/Trim; channel link; metering; smoothing; denormals

Phase 2 — Soft Clip & Modes (0.5 day)
- Pre‑clipper with OS; mode switching/crossfades; latency reporting hook

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset round‑trip; listening tests; CPU profiling; finalize ranges

---

## Math & Pseudocode

True‑peak detection (concept)
```cpp
// Oversample x[n] -> x_os[k] (polyphase FIR, factor R)
float peak = 0.0f;
for (k in block) peak = max(peak, fabsf(x_os[k]));
```

Required gain and smoothing
```cpp
float target = dbToLin(ceilingDb) / max(1e-6f, peak);
// release with dual time constants
if (g > target) g = target; // attack is instantaneous due to look-ahead
else g = aSlow*g + (1-aSlow)*target; // blend with fast based on GR
```

Apply with look‑ahead and mix
```cpp
float y = g * delay.read();
float wetK = sinf(0.5f*M_PI*mix), dryK = cosf(0.5f*M_PI*mix);
out = dryK*x + wetK*y;
```

---

## References
- ITU‑R BS.1770/1771, EBU R128 (true‑peak concepts)
- FabFilter Pro‑L2, Limiter nº6: behavior inspirations
- Vital: smoothing/RT patterns for stable modulation
