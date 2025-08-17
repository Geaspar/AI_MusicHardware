# Hybrid Spectral + Caching Wavetable Architecture

Status: Proposed
Owner: Audio/DSP
Last updated: 2025-08-17

## 1) Purpose and goals

Deliver Vital-class spectral flexibility with practical CPU cost by combining:
- Frequency-domain “master” representation per wavetable frame (magnitude + phase)
- On-demand IFFT to time-domain single-cycle buffers
- Per-voice caching, throttling, and seamless swaps without phase resets
- Transparent fallback to current precomputed multi-band (time-domain) tables

Key capabilities:
- Spectral morphing (tilt, formant shift, even/odd balance, band shelves, harmonic warping)
- Real-time modulation of spectral parameters at control rate
- Zero-click buffer swaps; phase continuity; consistent loudness
- Quality modes and async rendering to keep CPU predictable

Non-goals (initially): granular resynthesis, per-partial envelopes, phase vocoder time-stretching.


## 2) High-level architecture

- Storage/import
  - Load time-domain frames (wavetable) or spectral data. If time-domain, pre-FFT to spectral frames.
- Render pipeline (per voice)
  - Control-rate evaluation computes a cache key from (pitch band, morph position, spectral ops, size, SR, quality)
  - Look up time-domain buffer from cache; if miss, enqueue async IFFT job; use fallback table meanwhile
  - When the job completes, crossfade to the new buffer over 1–2 cycles without resetting phase
- Caching
  - Global LRU cache and per-voice tiny cache for last few keys
  - Invalidation with hysteresis and quantization to avoid thrash under modulation


## 3) Data model

```cpp
// Magnitude + phase per bin (N/2+1 bins for real IFFT)
struct SpectralBin { float mag; float phase; };

struct SpectralFrame {
  std::vector<SpectralBin> bins;   // size N/2+1
  int fftSize;                     // 1024/2048/4096
  int sampleRate;                  // authoring SR
  float normRms;                   // normalization reference
  uint64_t checksum;               // quick content ID
};

struct SpectralTable {
  std::vector<SpectralFrame> frames;    // wavetable frames
  std::string id;                       // stable table ID
  int defaultFftSize = 2048;            // recommended size
};

// Single-cycle, DC-free, normalized, ready for oscillator
struct WavetableBuffer {
  std::vector<float> samples; // length=N, mono cycle
  int fftSize;
  float rms;                  // computed rms for gain staging
  uint64_t keyHash;           // cache key baked into artifact
};
```

Notes:
- Magnitude-phase avoids complex discontinuities during morph; consider minimum-phase preprocessing for consistency.
- Store `normRms` per spectral frame for consistent loudness after IFFT.


## 4) Spectral operations (modulatable)

Parameters (per-voice, control-rate):
- tiltDbPerOct ∈ [-24, +24]
- formantShiftSemis ∈ [-24, +24] (log-frequency remap of magnitudes)
- evenOddBalance ∈ [-1, +1] (suppress even or odd partials)
- lowShelf: {gainDb, cutoffHz}
- highShelf: {gainDb, cutoffHz}
- harmonicWarp ∈ [0, 1] (nonlinear partial remap; optional)

Order of operations:
1) Tilt (slope vs log-frequency)
2) Formant shift (resample magnitudes on log axis)
3) Even/odd balance (multiply magnitudes of even/odd partials)
4) Shelves (low/high) via smooth magnitude curves
5) Warp (optional) mapping k -> f(k)

Implementation detail:
- Operate in magnitude domain; preserve phase (or apply minimum-phase if enabled)
- Guard denormals; clamp magnitudes to safe floor (e.g., 1e-6) and ceiling


## 5) Morphing model

- Primary morph: frame index t ∈ [0, 1] across frames (linear or spline index)
- Spectral morph: interpolate magnitudes and phases coherently
  - Magnitude: linear in dB or sqrt domain
  - Phase: unwrap, then linear interpolate; optionally minimum-phase per frame to stabilize
- Secondary morph axis (optional): blends between spectral op parameter sets or table pairs

Quantization for caching:
- Morph quantize to 128 steps (configurable)
- Snap hysteresis (e.g., 1 step deadband) to reduce churn under LFO


## 6) Band-limiting and pitch handling

- Compute Nyquist mask from note pitch and SR (partial cutoff)
- Two approaches:
  1) Mask magnitudes above Nyquist during IFFT build
  2) Rely on FFT size and anti-aliasing of endpoint; (1) is preferred
- Quantize pitch to semitone or Nyquist index for cache keys (prevents recomputation every cent)


## 7) Cache design

Key fields:
```
(tableId, morphQ, pitchBand, spectralOpsHash, fftSize, sampleRate, qualityMode)
```

- Global cache: LRU with a soft limit (e.g., 256 buffers, ~2–8 MB)
- Per-voice micro-cache: last 2–4 keys for instant reuse during small oscillations
- Eviction: LRU, plus purge on SR/quality mode change
- Spectral ops hash: 64-bit rolling hash of quantized params

Invalidation and throttling:
- Evaluate keys at control rate (e.g., 250 Hz)
- Only enqueue a new job if key differs from current/pending and last enqueue > 10 ms ago
- Pitch band hysteresis: require ≥ 1/12 octave change to switch bands
- Backpressure: if worker queue depth > threshold, stop enqueuing (use fallback), or switch to lower quality size


## 8) Async IFFT worker

- Thread pool size: 1–2 threads (configurable)
- MPSC lock-free queue of jobs `{Key, frameSpec}`
- Job processing:
  1) Build target spectral frame (morph + ops + Nyquist mask)
  2) IFFT (kissfft/fftw/vDSP) into time-domain buffer
  3) Remove DC (subtract mean)
  4) Normalize to target RMS (use frame normRms baseline)
  5) Store `WavetableBuffer` in cache and notify waiting voices (atomic publish)
- Cancellation: each voice tracks generation; superseded keys are ignored on delivery


## 9) Voice integration

- Each voice holds:
  - `currentBuffer` (shared_ptr<WavetableBuffer>)
  - `pendingBuffer` (optional)
  - `phase` ∈ [0, 1) accumulator
  - `crossfadeSamplesRemaining` (0 when idle)
- At control-rate update:
  - Compute key (quantized); if cached -> set `pendingBuffer`
  - If not cached -> enqueue; if no pending, keep using `currentBuffer` or fallback multi-band table
- In audio thread per-sample:
  - Read from `currentBuffer` at `phase`, increment phase by `frequency / sampleRate`
  - If `pendingBuffer` present:
    - Crossfade `out = (1-x)*cur + x*pend` over one cycle length (N samples) or shorter (configurable)
    - When done, swap `currentBuffer = pendingBuffer`, clear pending
  - Never reset phase during swap


## 10) Quality modes

- FFT size: 1024 (Eco), 2048 (Default), 4096 (High)
- Optional oversampling 2x/4x for heavy spectral ops (budget-gated); downsample after rendering
- Adaptive downgrade: on worker overload, auto select smaller FFT for new jobs (with smoothing)


## 11) Engine and UI integration

Engine:
- New `RealtimeWavetableVoiceV2` using this pipeline
- VoiceManager selects render mode (Precomputed / Hybrid / Realtime) per patch
- Global services: spectral cache, worker pool (shared across voices)

Mod Matrix:
- Add destinations: `WT Position`, `WT Tilt`, `WT Formant`, `WT Even/Odd`, `WT Macro`, `WT QualityMode` (stepped)
- Smoothing: per-destination alpha tuned to 10–30 ms; updates clamped to control rate to align with cache cadence

UI:
- Wavetable page with:
  - Position slider (0–1), spectral controls (tilt, formant, even/odd, shelves, macro)
  - Quality dropdown (Eco/Default/High; Oversampling Off/2x/4x)
  - Import panel (wav/wt), normalization, phase-align toggle
  - Debug overlay: cache hit rate, jobs/s, IFFT time, pending queue depth

Persistence:
- Save table ID, morph pos, spectral ops, render mode, quality settings, import path


## 12) Performance targets

- Control rate: ≤ 250 Hz for key recompute/update
- IFFT budget: ≤ 0.2 ms (1024), ≤ 0.6 ms (2048), ≤ 1.5 ms (4096) on Apple M-series
- Max jobs per second (steady): 100–200 (shared), burst protected via backpressure
- Cache hit rate typical: ≥ 85% under LFO on position @ 2–4 Hz with 128-step quant
- Audio thread: 0 allocs, constant-time per-sample


## 13) Testing strategy

Correctness:
- FFT round-trip tests (spectral <-> time) within numerical tolerance
- Morph continuity: no clicks; spectral interpolation produces expected magnitude curves
- Phase continuity: phase accumulator swaps produce < -60 dB artifacts
- Band-limiting: no alias bins above Nyquist in rendered buffers

Load/Stress:
- Modulate Position 0–1 at 2–8 Hz; verify cache behavior and CPU
- Sweep Tilt/Formant with rapid UI moves; ensure throttling/backpressure prevent xruns
- Many-voice polyphony (e.g., 32 voices) with unison; ensure worker keeps up or gracefully falls back

UI/Integration:
- Quality changes at runtime; observe adaptive downgrade/upgrade
- Import various tables; verify normalization and phase alignment toggle


## 14) Benchmarks & telemetry

- Instrument worker: job queue depth, IFFT durations (p50/p95), jobs/sec
- Cache: hit/miss, evictions, memory footprint
- Voices: swap counts, crossfade durations, fallback ratio
- Export CSV/JSON for profiling; optional on-screen overlay


## 15) Implementation details

FFT library:
- Prefer kissfft (portable, no allocations after plan) or vDSP on macOS; create reusable plans per size
- Preallocate working buffers; never allocate in audio thread

Threading:
- MPSC lock-free queue (e.g., ring buffer) for jobs; atomic publish for results
- Job coalescing: drop older keys for same voice if a newer supersedes

Memory:
- Pool allocators for WavetableBuffer of sizes {1024, 2048, 4096}
- DC removal and normalization in-place

Key hashing:
```cpp
struct CacheKey {
  uint64_t tableIdHash;
  uint16_t morphQ;       // 0..127
  uint16_t pitchBand;    // semitone or nyquist index
  uint16_t opsHash16;    // quantized spectral ops hash
  uint8_t  fftSizeCode;  // 0:1024,1:2048,2:4096
  uint8_t  quality;      // oversampling flags
  uint16_t sampleRateQ;  // SR/100 or similar
};
uint64_t hash(const CacheKey&);
```

Crossfade:
- Linear or equal-power over one cycle (N samples). For detuned voices or pitch glide, shorten to ≤ 0.5 N for snappier response.


## 16) Backward compatibility & fail-safe

- If no cached hybrid buffer available, use current precomputed multi-band table
- On worker saturation: pause enqueues; continue using last good buffer/fallback
- Feature flag per patch; global enable/disable in Settings


## 17) Rollout plan

- Phase gate behind a Settings toggle: Hybrid (Default) / Precomputed (Legacy) / Realtime (Debug)
- Start with 2048 default; expose Eco/High once stable
- Add debug overlay to iterate on cache quantization and thresholds

Deterministic & safety modes:
- Deterministic test mode (single worker thread, fixed scheduling) for reproducible tests
- Safe mode toggle: force Legacy fallback if worker overload detected or cache hit rate < threshold


## 18) Timeline (rough)

- Week 1: Data model, cache, keys, invalidation + unit tests
- Week 2: Worker + voice integration + crossfade; fallback wiring
- Week 3: Spectral ops + morphing + modulation; profiling pass
- Week 4: Quality modes, UI, persistence; full test/bench suite
- Week 5: Polish, telemetry, edge-case hardening, docs


## 19) Downsides, risks & mitigations

1) Increased complexity (more code paths: spectral ops, cache, worker, swaps)
   - Mitigations: strict module boundaries; unit + integration tests per module; debug overlay; feature flag to disable Hybrid.

2) CPU spikes/backpressure (bursts of IFFT jobs)
   - Mitigations: control‑rate throttling; job coalescing/cancellation; enqueue hysteresis; adaptive FFT size and quality; hard job budget per block; fallback to Legacy until backlog clears.

3) Latency to “lock in” sound (cache miss while modulating)
   - Mitigations: per‑voice micro‑cache; pre‑warm common keys on noteOn; quantize/hysteresis on morph/pitch; optional prefetch next band.

4) Memory footprint (many cached variants)
   - Mitigations: global LRU caps; pooled buffers per FFT size; telemetry to tune limits; optional on‑disk cache between runs.

5) Potential artifacts (clicks/comb/DC)
   - Mitigations: one‑cycle equal‑power crossfades; DC removal; RMS normalization; optional minimum‑phase preprocessing/phase alignment at import.

6) Determinism & debugging difficulties (async pipeline)
   - Mitigations: deterministic test mode; worker tracing; on‑screen metrics; feature flag to force Legacy.

7) Portability/licensing of FFT backends
   - Mitigations: kissfft default; vDSP on macOS via abstraction; avoid GPL‑licensed FFT; unit tests for each backend.

8) UI/UX “stepiness” from quantization/hysteresis
   - Mitigations: expose step size per control; smooth UI display while quantizing engine updates; document trade‑offs in Settings.

9) Patch consistency vs legacy tables (timbre differences)
   - Mitigations: per‑patch render mode; migration helper that renders and stores baked tables; A/B compare in UI.

10) Build size/startup time (import to spectral)
   - Mitigations: async precompute; on‑disk spectral cache serialization; progress indicator in UI.


## 20) Minimal API sketch

```cpp
// Loading
SpectralTable loadSpectralTable(const std::string& path);
SpectralTable fftFromTimeDomain(const TimeDomainTable& tdt, int fftSize);

// Spectral ops
SpectralFrame applySpectralOps(const SpectralFrame& in, const SpectralOps& ops);
SpectralFrame morphSpectral(const SpectralFrame& a, const SpectralFrame& b, float t);

// Rendering (worker)
WavetableBuffer renderTimeDomain(const SpectralFrame& f, int fftSize, int sr);

// Cache
std::shared_ptr<WavetableBuffer> getCached(const CacheKey& key);
void enqueueRender(const CacheKey& key, const SpectralSpec& spec);

// Voice
class RealtimeWavetableVoiceV2 : public Voice {
  void setTable(const SpectralTable*);
  void setRenderMode(RenderMode);
  void setQuality(QualityMode);
  void setSpectralOps(const SpectralOps&);
  void setMorph(float t);
  float processSample() override; // uses current/pending buffers, phase-accum
};
```


## 21) Deliverables

- Engine: new voice + global cache/worker, integrated in `Synthesizer`/`VoiceManager`
- UI: Wavetable editor panel; quality control; debug overlay; persistence
- Tests: unit + integration + performance; CI targets
- Docs: this spec; quickstart for importing wavetables; developer guide for extending spectral ops
