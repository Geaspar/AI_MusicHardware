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

## Practical implementation plan (step‑by‑step)

### Phase 1 — High‑quality Room/Hall with FDN‑8
1) ER taps + Predelay (stereo decorrelated)
2) Input diffusers (2–4 modulated allpasses)
3) FDN‑8 tank:
   - Delays scaled by Size (S): D_i’ = S · D_i
   - Feedback matrix: Householder (precompute per block)
   - Per‑loop filters: HF one‑pole in loop; bass shelf on recirculation path
   - Per‑delay modulation: LFO_i with depth_i, rate_i
   - RT60 mapping: compute g_i from Decay and D_i’
4) Output tone/width:
   - Stereo cross‑mix / width control
   - Optional final LP/HP shelves
5) Parameters: Predelay, ER Level, Size, Decay, Bass Mult, High Damping, Diffusion, Mod Rate, Mod Depth, Stereo Width, Mix

### Phase 2 — Plate & Vintage (Dattorro‑inspired)
- Plate mode: nested allpasses and figure‑8 feedback; brighter, faster build‑up; stronger diffusion.
- Vintage mode: heavier time variance (wander), warmer shelves, bit of tank saturation.

### Phase 3 — Advanced modes
- Shimmer: pitch‑shift (up a 5th/octave) in feedback path for ethereal tails.
- Space/Nonlinear: gated/ducked envelope shapes, reverse tails, etc.

### Phase 4 — Performance & stability
- Block processing (e.g., 64/128 samples) with denormal prevention (bias or flush‑to‑zero)
- SIMD for inner loops (optional), cache‑friendly delay line layouts
- Sample‑rate invariance: scale all delays/filters; validate at 44.1/48/96 kHz

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
```
// T60: desired RT60 (sec); Td: loop delay (sec)
// 60 dB decay ⇒ amplitude ratio 10^(−3)
float loopGain(float Td, float T60) {
    return powf(10.0f, -3.0f * (Td / T60));
}
```

### One‑pole HF damping in feedback path
```
// y[n] = a*y[n-1] + (1-a)*x[n]
struct OnePoleLP {
    float a = 0.0f, y1 = 0.0f;
    void setCutoff(float fc, float sr) {
        float x = expf(-TWO_PI * fc / sr);
        a = x; // 0..1
    }
    inline float process(float x) {
        y1 = a * y1 + (1 - a) * x;
        return y1;
    }
};
```

### Fractional delay (3rd‑order Lagrange) for modulation
```
// delayLine.readFrac(pos) uses 3rd‑order Lagrange around integer index
// depth expressed in samples, rate in Hz, phase randomized per line
float fracIndex = baseDelay + depth * sinf(phase);
float y = delayLine.readFrac(fracIndex);
phase += rate * (2π / sr) * blockSize; // block‑rate update
```

### Householder feedback matrix (FDN)
```
// H = I − 2*u*u^T; u is N‑vector with 1/√N entries
// Efficient form: y = x − 2 * u * (u^T x)  (one dot product + scaled add)
```

---

## QA & verification
- Objective:
  - Energy Decay Relief (EDR) plots: ensure smooth exponential decay without ridges
  - Band‑limited RT60: verify requested RT60 within tolerance across bands
  - Mode density checks: no sparse “ringy” regions
- Subjective:
  - A/B against known good verbs (plates/rooms/halls)
  - Parameter sweeps: ensure no zippering/clicks; no metallic build‑ups
  - Mix tests: synth bass, plucks, pads, percussion; stereo image holds

---

## CPU & memory considerations
- FDN‑8 with modulated fractional delays: ~low‑mid CPU on modern CPUs at 48 kHz
- Memory: O(ΣD_i) for delay lines + overhead for diffusers/ER; modest (< few MB)
- SIMD: accelerate interpolations and matrix ops; interleave delay buffers for cache locality
- Quality modes: N=4 (Eco), N=8 (Normal), N=16 (High) with adjustable diffuser count

---

## Modes to offer (initial set)
- Room: shorter delays, lower diffusion depth, more ER prominence, higher HF damping
- Hall: larger Size, longer RT60, heavier diffusion, gentler HF damping, subtle modulation
- Plate: Dattorro‑inspired diffusers and tank; brighter, fast build‑up; optional nonlinear sheen
- Vintage: extra time variance (“wander”), warmer tilt/shelves, slightly saturated tank
- Shimmer (Phase 3): pitch‑shift feedback path (+5th/+octave), longer tails

Each mode presets sensible defaults and parameter ranges; users can still tweak core parameters.

---

## Engineering checklist (for implementation)
- [ ] ER generator with stereo tap sets and Predelay/Level/Width
- [ ] 2–4 modulated diffusers (configurable) with Diffusion control
- [ ] FDN‑8 core with Householder feedback and per‑loop damping (HF/LF)
- [ ] Fractional delays with low‑depth modulation; Lagrange/Thiran interpolation
- [ ] RT60 mapping per loop; Size scaling of delays
- [ ] Parameter smoothing per category
- [ ] Denormal prevention and block‑based processing
- [ ] Sample‑rate scaling for delays/filters
- [ ] Stereo width control at output
- [ ] Quality modes (Eco/Normal/High)
- [ ] Tests: EDR plots, band RT60, CPU profiles

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

## Next steps for AIMusicHardware
1) Prototype a Plate (Dattorro‑style) and an FDN‑8 Hall as separate effects
2) Build parameter UI per spec; add Mode dropdown and sensible defaults
3) Validate with EDR and band RT60 tests; tune diffusion/modulation
4) Optimize: SIMD interpolation, buffer layouts; add quality modes
5) Add Shimmer variant in Phase 3

With careful tuning of diffusion, time‑variance, and frequency‑dependent decay, these designs can approach the lushness and mix‑friendliness associated with top‑tier products (e.g., Valhalla) while remaining efficient and flexible for real‑time use.
