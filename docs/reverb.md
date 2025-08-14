## Reverb DSP: State of the Art and Practical Design Notes

This document surveys best‑in‑class algorithmic reverb design, compares classic and modern methods (e.g., Lexicon‑style, Dattorro plate, Valhalla‑style modern algorithms), and outlines a practical implementation plan suitable for AIMusicHardware. It includes suggested parameters, math for stable RT60 mapping, modulation strategies, CPU considerations, and verification techniques.

### Design goals for a modern, musical reverb
- Lush, dense late field without metallic ringing or flutter echoes
- Smooth, time‑variant tails (anti‑static) with tasteful modulation
- Natural, decorrelated early reflections with controllable stereo width
- Frequency‑dependent decay (e.g., longer lows, tamed highs) that sits in a mix
- Stable, click‑free parameter changes and sample‑rate invariance
- Predictable gain staging: consistent perceived loudness across Decay/Size settings
- Lean CPU cost with graceful quality/perf trade‑offs

---

## Reverb approaches: overview and trade‑offs

### 1) Algorithmic reverbs (IIR/FIR delay networks)
- Schroeder/Moorer: cascades of allpass + feedback combs with optional ER section. Lightweight, but can sound metallic if static.
- Dattorro Plate (1997): highly musical plate topology using nested allpasses (diffusers), modulated delays, and simple damping.
- Feedback Delay Networks (FDNs): N×N delay lines with an orthonormal (energy‑preserving) feedback matrix (e.g., Householder, Hadamard). Excellent late‑field density and stability; modern staple.
- Velvet noise / sparse methods: efficient stochastic late tails with convincing density per CPU.

Pros (algorithmic): time variance, low latency, small memory, high expressiveness. Cons: needs careful tuning to avoid coloration.

### 2) Convolution reverbs
- Use measured/synthesized IRs; efficient implementations use partitioned FFT convolution (uniform or mixed partitioning) to minimize latency.
- Pros: authentic spaces, linear and predictable. Cons: static by default; lacks modulation/liveliness unless hybridized; memory/CPU for long IRs.

### 3) Hybrid approaches
- Algorithmic late tail + convolution ERs (or vice versa)
- Time‑varying convolution (IR morphing or time segmentation) for liveliness

For a synth‑centric engine, a well‑designed algorithmic reverb with FDN core and strong time variance is the most flexible, lowest‑latency path to “best in class.”

---

## What separates “great” from “good” algorithmic reverbs

- Time variance everywhere it matters:
  - Subtle delay modulation in diffusers and FDN loops (ultra‑slow + slow LFOs)
  - Randomized micro‑perturbations (a la Lexicon “spin/wander”) to avoid stationary combing
- High diffusion density:
  - Multiple diffusers (serial allpasses) before and within the tank
  - Uneven, co‑prime delay sets spanning multiple time scales
- Energy‑preserving feedback matrices in FDNs:
  - Householder/Hadamard/Givens ensure stability and even energy spread
- Frequency‑dependent decay:
  - Per‑loop HF damping and LF shaping to achieve musically useful RT60 vs frequency
- Stereo decorrelation and width control:
  - Per‑channel delay sets and modulation phases; controlled cross‑mix
- Gain staging and smoothing:
  - Internal level normalization; exponential smoothing of user parameters to avoid zipper noise

---

## Building blocks (with practical details)

### Early Reflections (ER)
- Purpose: spatial cues and sense of “room” before late tail arrives.
- Implementations:
  - Tap clusters: 8–24 taps per channel with randomized jitter (±1–3 ms) and per‑channel decorrelation.
  - Image‑method approximation for rectangular rooms (fast, parameterized reflections); optional.
- Controls: Predelay (0–100 ms), ER Level (0–1), ER Width (0–1).

Suggested tap times (ms) per channel example:
- L: 2.9, 5.1, 7.4, 11.7, 13.2, 17.9, 23.6, 31.8
- R: 3.3, 6.0, 8.7, 12.1, 14.5, 19.4, 26.1, 34.0
Apply small random jitter per instance to avoid sameness.

### Diffusers (input and intra‑tank)
- Serial allpass filters with short/medium delays (3–20 ms) and gains 0.5–0.75.
- Light modulation (0.02–0.2% fractional) at ultra‑slow rates (0.05–0.5 Hz) removes static coloration.
- 2–4 stages at input; optional intra‑tank diffusers before/after the feedback network.

Allpass formula (sample n):
- y[n] = −g·x[n] + x[n−D] + g·y[n−D]
- Choose D as fractional delay with interpolation to enable modulation.

### FDN Late Reverb Core
- Choose N=8 (good balance of quality/CPU; later allow N=16 for “High Quality” mode).
- Delays example (ms at 48 kHz, scaled with Size): 15.3, 19.7, 23.1, 29.9, 37.1, 51.7, 67.9, 89.7
  - Ensure co‑prime / uneven distribution; avoid shared factors.
- Feedback matrix: Householder (H = I − 2uuᵀ with u = 1/√N [1…1]) or normalized Hadamard.
- Per‑loop filters:
  - HF damping: one‑pole lowpass inside each loop (cutoff maps from High Damping param)
  - LF shaping: low shelf to extend/shorten low‑frequency decay (Bass Mult param)
- Modulation:
  - Low‑rate LFOs per delay with randomized phase; depth ~0.02–0.15% of delay
  - Fractional delay with 1st/3rd‑order Lagrange or Thiran allpass interpolation

#### Mapping Decay (RT60) to feedback gains
For a loop with delay time T_d, the per‑loop feedback gain g to achieve RT60 of T_60 is approximately:
- g = 10^(−3 · T_d / T_60)
Then apply HF/LF shaping via filters in the loop. Global Decay controls T_60; Bass Mult and High Damping modify loop filters to tilt decay vs frequency.

### Tone & bandwidth controls
- Input HP/LP filters define bandwidth before entering diffusers/tank.
- Tank shelves/tilt shape overall coloration (e.g., damp highs, support lows).
- Optional gentle saturation in tank to prevent wild peaks (use sparingly).

### Parameter smoothing
- Apply exponential smoothing with time constants per parameter category:
  - Fast (5–20 ms): Mix, ER Level
  - Medium (50–200 ms): Size, Decay
  - Slow (250–750 ms): Damping/Bass shaping (to avoid audible shifts)

---

## Time variance strategies (anti‑metallic)
- Multi‑rate LFO scheme:
  - Ultra‑slow “wander”: 0.01–0.03 Hz low‑depth random walk on select delays
  - Slow modulation: 0.05–0.5 Hz sinus/triangle per delay with small depth
  - Random phase per delay line; low cross‑correlation between channels
- Micro‑randomization:
  - Occasional ±1 sample retune (bounded) with crossfades to avoid clicks; use sparingly

---

## Practical implementation plan (step‑by‑step granular)

### Phase 0 — Scaffolding and Basic Integration (1 day)
1.  **Create new effect class files:**
    *   `include/effects/FDNReverb.h`
    *   `src/effects/FDNReverb.cpp`
    *   `include/effects/PlateReverb.h`
    *   `src/effects/PlateReverb.cpp`
2.  **Define basic class structure for `FDNReverb` and `PlateReverb`:**
    *   Inherit from `Effect`.
    *   Implement constructor, destructor, `process(float* buffer, int numFrames)`, `setParameter(const std::string& name, float value)`, `getParameter(const std::string& name) const`, and `getName() const`.
    *   For `process`, initially implement a no-op (pass-through) or simple dry/wet mix.
    *   For `setParameter`/`getParameter`, use an internal `std::map<std::string, float>` to store parameters (e.g., `mix_`, `predelay_ms_`).
    *   Add include guards (`#pragma once`).
3.  **Wire new effects to the effect factory (`include/effects/AllEffects.h`):**
    *   Include `FDNReverb.h` and `PlateReverb.h`.
    *   In `createEffectComplete()`, add `else if` branches to return `std::make_unique<FDNReverb>(sampleRate)` and `std::make_unique<PlateReverb>(sampleRate)`.
    *   Add "FDNReverb (Hall)" and "PlateReverb" to `getAvailableEffects()`.
4.  **Add basic UI mapping (`src/main_integrated_simple.cpp`):**
    *   In `configureSlotParams()`, add `else if (type == "FDNReverb (Hall)")` and `else if (type == "PlateReverb")`.
    *   For each, map `mix` to `slotV1Slider` (or `slotMixSlider` if preferred for consistency).
    *   Set default ranges and initial values (e.g., `mix->setRange(0.0f, 1.0f); mix->setValue(0.25f);`).
    *   Disable other parameter sliders (`disableParam(slotV2Label[s], slotV2Slider[s]);` etc.) for now.
5.  **Build and verify:**
    *   Run `./build.sh`.
    *   Launch `build/bin/AIMusicHardwareIntegrated`.
    *   Go to Effects tab, select "FDNReverb (Hall)" or "PlateReverb".
    *   Confirm the effect appears, and the Mix slider is visible and functional (even if audio is just dry/wet).
    *   Confirm no crashes when selecting or adjusting parameters.

### Phase 1 — Early Reflections + Input Diffusion (2–3 days)
1.  **Implement `EarlyReflections` class/module:**
    *   Create `include/audio/EarlyReflections.h` and `src/audio/EarlyReflections.cpp`.
    *   Manage stereo tap delays (e.g., 8–16 taps per channel).
    *   Implement fractional delay lines (e.g., using 1st-order linear interpolation for simplicity, upgrade to 3rd-order Lagrange later).
    *   Add internal random seed for tap jitter.
    *   Methods: `process(float* interleavedStereoBuffer, int numFrames)`, `setPredelay(ms)`, `setERLevel(0-1)`, `setERWidth(0-1)`, `reset()`.
    *   Start with no-op pass-through to verify plumbing.
2.  **Integrate `EarlyReflections` into `FDNReverb` and `PlateReverb`:**
    *   Add `EarlyReflections` member to each reverb class.
    *   Call `er.process()` before main reverb tank.
    *   Wire `predelay_ms`, `er_level`, `er_width` parameters.
3.  **Implement `AllpassDiffuser` class/module:**
    *   Create `include/audio/AllpassDiffuser.h` and `src/audio/AllpassDiffuser.cpp`.
    *   Implement allpass filter with fractional delay and modulation.
    *   Methods: `process(float x)`, `setDelay(samples)`, `setGain(g)`, `setModulation(rate, depth)`, `reset()`.
4.  **Integrate input diffusers into `FDNReverb` and `PlateReverb`:**
    *   Add 2–4 `AllpassDiffuser` instances in series at the input stage.
    *   Apply light modulation to their delay times.
5.  **Parameters and UI mapping:**
    *   Add `predelay_ms`, `er_level`, `er_width`, `diffusion` (maps to allpass gains), `mod_rate`, `mod_depth` to `FDNReverb` and `PlateReverb` parameter maps.
    *   Update `configureSlotParams()` in `src/main_integrated_simple.cpp` to expose these parameters on the UI.
    *   Use appropriate value formatters (e.g., `ms` for predelay).
6.  **Build and verify:**
    *   Run `./build.sh`.
    *   Test in `AIMusicHardwareIntegrated`: check for audible early reflections and initial diffusion. Listen for metallic ringing (should be minimal with modulation).

### Phase 2 — FDN‑8 Late Tail (Hall/Room) (4–6 days)
1.  **Implement `FDNReverb` core logic:**
    *   Declare 8 delay lines (`std::vector<float>`) as members.
    *   Define 8 co-prime/uneven base delay lengths (e.g., 15.3, 19.7, ..., 89.7 ms).
    *   Implement fractional delay reading for each line (using 1st-order interpolation for now).
    *   Implement Householder or Hadamard feedback matrix (precompute `u` vector, then `y = x - 2 * u * (u^T x)`).
    *   Integrate per-loop one-pole lowpass filters for HF damping.
    *   Integrate optional low-shelf filters for LF shaping.
2.  **RT60 mapping and gain control:**
    *   Implement `loopGain(Td, T60)` function.
    *   Map `decay_rt60_s` parameter to individual loop gains.
    *   Map `high_damping` to LP filter cutoffs.
    *   Map `bass_mult` to LF shelf gain/frequency.
    *   Implement internal gain normalization to maintain consistent output level.
3.  **Modulation for FDN delays:**
    *   Add per-delay LFOs (sine/triangle) with randomized phases.
    *   Apply small modulation depth (e.02-0.15% of delay) to fractional delay reads.
4.  **Parameters and UI mapping:**
    *   Add `size`, `decay_rt60_s`, `bass_mult`, `high_damping`, `stereo_width` to `FDNReverb` parameters.
    *   Update `configureSlotParams()` to expose these.
    *   Use log scale for `decay_rt60_s` and `size` sliders.
5.  **Build and verify:**
    *   Run `./build.sh`.
    *   Test `FDNReverb (Hall)`: listen for lush, dense tails. Sweep parameters (Size, Decay, Damping) and check for stability, no clicks, and smooth transitions.

### Phase 3 — Plate Reverb (Dattorro‑style) (3–5 days)
1.  **Implement `PlateReverb` core logic:**
    *   Follow Dattorro’s topology: input allpasses, then main plate tank with figure-8 feedback routing.
    *   Use specific delay lengths and allpass gains as per Dattorro’s paper.
    *   Implement modulated delays within the plate tank.
    *   Add tone control (e.g., simple LP/HP or shelving filters) within the feedback loops.
2.  **Parameters and UI mapping:**
    *   Add `size`, `decay_rt60_s`, `tone_high`, `diffusion`, `mod_rate`, `mod_depth` to `PlateReverb` parameters.
    *   Update `configureSlotParams()` for `PlateReverb`.
3.  **Build and verify:**
    *   Run `./build.sh`.
    *   Test `PlateReverb`: listen for characteristic bright, dense plate sound. Verify parameter responses.

### Phase 4 — UI Integration & Polish (1–2 days)
1.  **Refine UI mappings (`src/main_integrated_simple.cpp`):**
    *   Ensure all parameters for `FDNReverb` and `PlateReverb` are exposed with correct ranges and formatters.
    *   Implement `Mix` parameter for both reverbs (if not already done).
    *   Ensure Bypass functionality works correctly (sets `mix` to 0).
    *   Add sensible default values for all parameters in `createEffectWithDefaults()`.
2.  **Add quality modes (optional, if time permits):**
    *   Implement `setQualityMode(Eco/Normal/High)` in reverb classes.
    *   Map to FDN size (N=4/8/16) and diffuser count.
3.  **Build and verify:**
    *   Run `./build.sh`.
    *   Thorough UI testing: all sliders, dropdowns, and buttons work as expected for both reverbs.

### Phase 5 — Performance & Stability (2–3 days)
1.  **Optimize `process()` methods:**
    *   Ensure block processing is efficient.
    *   Implement denormal prevention (e.g., adding tiny DC offset or flush-to-zero).
    *   Consider SIMD for inner loops (e.g., delay line interpolation, matrix multiplication).
2.  **Sample-rate invariance:**
    *   Verify all delay times and filter coefficients scale correctly with `sampleRate_`.
    *   Test at different sample rates (e.g., 44.1kHz, 48kHz, 96kHz).
3.  **Gain staging:**
    *   Fine-tune internal gain normalization to prevent clipping and maintain consistent perceived loudness.
4.  **Build and verify:**
    *   Run `./build.sh`.
    *   Stress test with multiple reverb instances and high polyphony. Monitor CPU usage.

### Phase 6 — Verification & Tuning (2–4 days)
1.  **Objective tests:**
    *   Implement simple impulse response generation for reverbs.
    *   Generate Energy Decay Relief (EDR) plots to visualize decay smoothness.
    *   Measure RT60 across frequency bands to verify damping/shaping.
    *   Check mode density (listen for metallic ringing, flutter echoes).
2.  **Subjective tests:**
    *   A/B against reference reverbs (e.g., Valhalla, Lexicon emulations).
    *   Test with various synth sounds (bass, plucks, pads, percussion) and full mixes.
    *   Sweep parameters to check for zippering, clicks, or undesirable artifacts.
3.  **Refine parameters:**
    *   Adjust default values and ranges for optimal musicality.

### Phase 7 — Polish & Presets (1–2 days)
1.  **Create presets:**
    *   Develop a set of musically useful presets (e.g., Small Room, Large Hall, Bright Plate, Dark Plate, Vintage).
2.  **Update documentation:**
    *   Add implementation details, tuning notes, and user-facing tips to `reverb.md`.

---

## Parameter mapping and ranges (recommended)
- Predelay: 0–100 ms (log taper)
- ER Level: 0–1 (linear)
- Size: 0.5–2.0 (scales D_i); UI as 0–100 with perceptual mapping
- Decay (RT60): 0.2–20 s (log taper)
- Bass Mult: 0.5–2.0 (multiplies low‑band RT60)
- High Damping: 0–1 (maps to LP cutoff 2 kHz…12 kHz)
- Diffusion: 0–1 (sets allpass gains 0.3…0.75)
- Mod Rate: 0.05–1.0 Hz
- Mod Depth: 0–0.25% of delay (cap small)
- Stereo Width: 0–1 (0=mono, 1=wide decorrelated)
- Mix: 0–1 (map to wet/dry with constant power law if desired)

Constant‑power Wet/Dry mapping (optional):
- wet = sin(π/2 · Mix); dry = cos(π/2 · Mix)

---

## Math snippets & pseudocode

### RT60 → loop gain
```cpp
// T60: desired RT60 (sec); Td: loop delay (sec)
// 60 dB decay ⇒ amplitude ratio 10^(−3)
float loopGain(float Td, float T60) {
    if (T60 <= 0.0f) return 0.0f; // Instant decay
    return powf(10.0f, -3.0f * (Td / T60));
}
```

### One‑pole HF damping in feedback path
```cpp
// y[n] = a*y[n-1] + (1-a)*x[n]
struct OnePoleLP {
    float a = 0.0f, y1 = 0.0f;
    void setCutoff(float fc, float sr) {
        if (sr <= 0.0f) sr = 44100.0f; // Safety guard
        float x = expf(-TWO_PI * fc / sr);
        a = x; // 0..1
    }
    inline float process(float x) {
        y1 = a * y1 + (1 - a) * x;
        return y1;
    }
    void reset() { y1 = 0.0f; }
};
```

### Fractional delay (3rd‑order Lagrange) for modulation
```cpp
// delayLine.readFrac(pos) uses 3rd‑order Lagrange around integer index
// depth expressed in samples, rate in Hz, phase randomized per line
float fracIndex = baseDelay + depth * sinf(phase);
float y = delayLine.readFrac(fracIndex); // Requires a DelayLine class with readFrac
phase += rate * (TWO_PI / sr) * blockSize; // block‑rate update
```

### Householder feedback matrix (FDN)
```cpp
// H = I − 2*u*u^T; u is N‑vector with 1/√N entries
// Efficient form: y = x − 2 * u * (u^T x)  (one dot product + scaled add)
// where x is input vector, y is output vector
// u_dot_x = 0; for i=0..N-1: u_dot_x += u[i] * x[i];
// for i=0..N-1: y[i] = x[i] - 2 * u[i] * u_dot_x;
```

---

## Time‑boxed granular checklist (engineering)
- [ ] Phase 0: Scaffolding (classes, factory, UI wiring, build)
  - [x] Create `FDNReverb.h/.cpp`, `PlateReverb.h/.cpp` with `Effect` inheritance
  - [x] Implement no‑op `process` and basic parameter map with `mix`
  - [x] Register in `AllEffects.h` and `getAvailableEffects()`
  - [x] Add UI mapping in `src/main_integrated_simple.cpp` (Mix only)
  - [x] Build and smoke‑test
- [ ] Phase 1: ER + Input Diffusion
  - [ ] Implement `EarlyReflections` (scaffold, pass‑through)
  - [ ] Integrate ER into FDN/Plate (params: `predelay_ms`, `er_level`, `er_width`)
  - [ ] Implement `AllpassDiffuser` (scaffold + modulation)
  - [ ] Integrate 2–4 diffusers into inputs; expose `diffusion`, `mod_rate`, `mod_depth`
  - [ ] Build and audition ER/diffusion
- [ ] Phase 2: FDN‑8 Late Tail
  - [ ] Delay lines + fractional read; Householder/Hadamard matrix
  - [ ] Per‑loop HF/LF filters; RT60 gain mapping
  - [ ] Per‑delay LFOs; parameters: `size`, `decay_rt60_s`, `bass_mult`, `high_damping`, `stereo_width`
  - [ ] Gain normalization; build and audition
- [ ] Phase 3: Plate (Dattorro)
  - [ ] Topology; tuned delays/allpasses; modulation; tone shaping
  - [ ] Parameters exposed; build and audition
- [ ] Phase 4: UI Polish
  - [ ] Ranges/formatters; constant‑power `mix`; defaults in `createEffectWithDefaults()`
- [ ] Phase 5: Performance/Stability
  - [ ] 3rd‑order Lagrange/Thiran; denormals; block processing; sample‑rate invariance; limiter
- [ ] Phase 6: Verification/Tuning
  - [ ] IR/EDR tooling; band RT60; subjective sweeps; tuning
- [ ] Phase 7: Presets/Docs
  - [ ] Presets; finalize docs with tuning notes

---

## References (expanded)
- M. R. Schroeder (1962): Natural Sounding Artificial Reverberation
- J. A. Moorer (1979): About this reverberation business
- J. Dattorro (1997): Effect Design Part 1/2 (Journal of the AES)
- Jot, Chaigne, and others (1991–1992): Frequency‑dependent decay and energy‑preserving FDNs
- Julius O. Smith III: Physical Audio Signal Processing (online book)
- Valhalla DSP blog (Sean Costello): extensive posts on modulation, diffusion, vintage coloration
- FDN literature on Householder/Hadamard matrices and stability

---

Note: Commercial products like Lexicon and Valhalla employ proprietary techniques and extensive tuning. The designs above are widely published building blocks that, when carefully combined and tuned (time variance, diffusion, damping, and gain staging), can achieve comparable quality and musicality.
