## Reverb DSP: State of the Art and Practical Design Notes

This document surveys best‑in‑class algorithmic reverb design, compares classic and modern methods (e.g., Lexicon‑style, Dattorro plate, Valhalla‑style modern algorithms), and outlines a practical implementation plan suitable for AIMusicHardware.

### Goals for a modern, musical reverb
- Lush, dense late field without metallic ringing
- Smooth, time‑variant tails (anti‑static) with tasteful modulation
- Natural early reflections with controllable stereo width
- Frequency‑dependent decay (e.g., longer lows, tamed highs)
- Stable, low‑CPU operation with sample‑rate invariance and click‑free parameter changes

## Taxonomy of reverb approaches

### 1) Algorithmic reverbs (IIR/FIR delay networks)
- Schroeder/Moorer: Cascades of allpass + feedback combs, plus early reflection generator. Fast, but prone to coloration/metallic artifacts if not time‑variant.
- Dattorro Plate (1997): High‑quality plate topology with nested allpasses, diffusers, and modulation; basis for many musical “plate/room” sounds.
- FDN (Feedback Delay Networks): N×N delay lines with an orthonormal/energy‑preserving feedback matrix (e.g., Householder, Hadamard, circulant/Givens). Scales well, yields very dense, smooth tails when coupled with diffusion and time variance.
- Velvet noise / sparsified approaches: Use sparse, randomized impulse sequences to approximate dense late fields efficiently.

### 2) Convolution reverbs
- Use measured/synthesized impulse responses; modern implementations use partitioned FFT convolution (uniform or non‑uniform) to keep latency low.
- Pros: Highly realistic IRs (halls, plates, springs). Cons: Static by default; lacks time variance/modulation unless using time‑varying IRs/hybrids; CPU/memory can be high for long IRs.

### 3) Hybrid approaches
- Algorithmic late tail + convolution early reflections (or vice versa)
- Time‑varying convolution segments to inject life/modulation

## What makes “best in class” algorithmic reverbs

- Time variance everywhere it matters:
  - Subtle delay modulation in diffusers/FDN loops (low‑rate LFOs with randomization) prevents combing/metallic resonances.
  - Randomized/allpass parameter dither (“wander/spin” in classic Lexicon lore) to avoid stationarity.
- High diffusion density:
  - Multiple diffusion stages (serial allpasses or diffuser lattices) before and within the late reverb network.
  - Carefully chosen delay lengths (co‑prime/primeish, uneven distribution, multiscale) to avoid mode clustering.
- Orthonormal feedback matrices in FDN:
  - Householder, Hadamard, or Jacobi/Givens rotations keep energy distribution even and stable; enable long, smooth decays.
- Frequency‑dependent damping/decay shaping:
  - Per‑band or per‑loop filters (one‑pole LP/HP, shelving, tilt, or simple 2nd‑order sections) to control RT60 vs frequency (e.g., lows longer, highs shorter).
- Early reflections that feel “real” and wide:
  - Simple image‑method approximations or tuned tap clusters; stereo width via decorrelated tap sets per channel.
- Stereo decorrelation:
  - Slightly different delay sets and modulations per channel; cross‑mixing to avoid dual‑mono feel.
- Parameter smoothing and gain staging:
  - Exponential smoothing of user params; internal gain normalization to keep output consistent across size/RT60 changes.

## Classic exemplars and techniques

### Lexicon‑style (early digital classics)
- Heavily modulated allpass/comb networks with time‑varying delay lengths.
- “Spin”/“Wander” concepts: ultra‑slow random LFO perturbations to parameters.
- Diffusion blocks at input and within tank; shelving/tilt damping.
- Gentle nonlinearities (soft clip/saturation) inside feedback paths to stabilize peaks and add warmth.

### Dattorro Plate (1997)
- Widely cited plate algorithm with nested allpasses (“diffusers”), modulated delays, and figure‑8 feedback routing.
- Fast to implement; high density with musical dispersion; great as a starting point for plate verbs.

### Valhalla‑style modern algorithmics (conceptually)
- High diffusion density + tasteful, wideband modulation for lushness.
- Multiple reverb “modes” (plate/room/hall/shimmer) built by varying diffuser depths, feedback matrices, tone curves, and modulation characteristics.
- Very careful time variance and parameter smoothing to avoid grain.
- Frequency‑dependent decay sculpted for mix‑friendliness; bass multipliers and treble damping commonly exposed.

## Building blocks in detail

### Early reflections (ER)
- Options: 
  - Tap clusters with randomized jitter and per‑channel decorrelation
  - Simple image‑method for rectangular spaces (fast approximation)
- Controls: Pre‑delay, ER level, stereo width, room size bias

### Diffusers
- Serial allpass sections with small/medium delays, modest gains (0.5–0.75), and light modulation (e.g., 0.05–0.2% at <1 Hz)
- Two to four stages at input; optionally more around/inside the late tank.

### Late reverb core
- FDN (e.g., 8×8 or 16×16):
  - Delays: co‑prime set, spanning ~15–160 ms; fractional delay with Lagrange/Thiran interpolation for modulation.
  - Feedback matrix: Householder or Hadamard (orthonormal) for energy preservation.
  - Per‑loop filters: low/high shelving and one‑poles to set frequency‑dependent decay; optional tilt filter.
  - Modulation: low‑rate LFOs per delay (phase‑offset/randomized) to decorrelate modes.
  - Optional cross‑channel mixing or stereo widening at output.

### Frequency‑dependent decay (RT60 shaping)
- Map user RT60 (sec) to per‑loop feedback gains using exp(−3 ln(10) * T / delay) style relationships.
- Apply low‑/high‑frequency multipliers (e.g., Bass Multiplier, High Damping) by inserting simple filters in feedback paths.

### Tone & bandwidth
- Input lowpass/highpass to define bandwidth; shelving/tilt in tank to keep highs smooth and lows supportive.

### Time variance & anti‑metallic strategies
- Multi‑rate modulation (very slow “wander” + slow LFO + slight random jitter)
- Occasional micro‑randomization of delay taps (bounded) for long sessions

### Nonlinearities (optional)
- Very gentle saturation in feedback to tame peaks and add warmth—use with care to avoid build‑up.

## Practical implementation plan for AIMusicHardware

Phase 1 — High‑quality algorithmic reverb (Room/Hall/Plate)
1. Early reflections generator:
   - Tap‑cluster ER with stereo decorrelation; params: Predelay, ER Level, Width.
2. Diffusion front‑end:
   - 2–4 modulated allpass diffusers; param: Diffusion (gains), Diffusion Depth.
3. Late tail (FDN‑8):
   - 8 delays (15–160 ms), per‑delay LFO (0.05–0.5 Hz, small depth), Householder matrix.
   - Per‑loop damping filters: low/high shelves; global tilt.
   - Params: Size, Decay (RT60), Bass Mult, High Damping, Mod Rate/Depth, Stereo Width, Mix.
4. Stability & quality:
   - Denormal prevention, parameter smoothing, sample‑rate invariance, block processing.

Phase 2 — Plate & Vintage mode (Dattorro‑inspired)
- Dedicated plate mode: nested allpasses + modulated tank; adjustable metallic‑to‑lush balance.
- Vintage mode: increased time variance and diffusion coloration akin to classic units.

Phase 3 — Advanced modes (Shimmer/Space)
- Pitch‑shift in feedback (for shimmer) and specialized diffusion/FDN layouts.

Phase 4 — Performance & testing
- Energy Decay Relief (EDR) plots, mode density checks, RT60 accuracy vs frequency bands.
- CPU profiling; optional SIMD in inner loops.

## Parameters & UI suggestions
- Predelay (ms)
- Early Level / Width
- Size (maps to delay scaling)
- Decay (RT60, seconds)
- Bass Mult (× decay)
- High Damping (0–1)
- Diffusion (0–1)
- Mod Rate (Hz) / Mod Depth (%)
- Stereo Width (0–1)
- Mix (0–1)

## Engineering considerations
- Smoothing: exp smoothing (time constants per parameter); click‑free changes.
- Sample‑rate invariance: scale delay lengths and filters with SR.
- Fractional delays: 1st–3rd order Lagrange or Thiran; keep modulation shallow to minimize interpolation blur.
- Feedback gain calibration: compute from desired RT60; verify stability margins.
- CPU scaling: allow lower FDN sizes or diffusion counts for constrained targets.

## References / Further reading (non‑exhaustive)
- M. R. Schroeder (1962): Natural Sounding Artificial Reverberation.
- J. A. Moorer (1979): About this reverberation business.
- J. O. Smith III: Physical Audio Signal Processing (online text) — sections on reverberation, FDNs, allpasses.
- J. Dattorro (1997): Effect Design part 1/2 — Plate reverb topology and diffusers.
- Jot & Chaigne (1991/1992): FDNs with frequency‑dependent decay; energy‑preserving feedback matrices.
- ValhallaDSP blog (Sean Costello): numerous posts on algorithms, modulation, diffusion, and reverb design philosophy.

---

Note: Commercial products like Lexicon and Valhalla employ proprietary techniques and extensive tuning. The designs above are widely published building blocks that, when carefully combined and tuned (time variance, diffusion, damping, and gain staging), can achieve comparable quality and musicality.
