# FX Research — Reverbs and Creative DSP (Aug 2025)

This doc surveys notable reverb architectures and adjacent creative FX relevant to sound design. It highlights where we can surpass existing tools and proposes new ideas (flagged as MY IDEA).

## Goals
- High musicality with strong time-variance and density
- Parameter sets that invite exploration yet remain stable
- RT-safe, CPU-efficient designs suitable for live performance

## Algorithmic Reverbs (state of the art)

- FDN (Feedback Delay Networks)
  - Strengths: dense late fields, energy stability with orthonormal matrices (Householder/Hadamard), easy parameterization of RT60 and damping. Modern staple (Valhalla room/hall styles, academic literature).
  - Weak spots in market: many plugins still trade density vs modulation; some tails get static at long decays or collapse under heavy modulation.
  - What we can do better:
    - Multi-rate modulation (ultra-slow wander + slow LFO) with decorrelated phases per line, capped depth per line expressed in samples (SR-invariant).
    - Per-band RT60 shaping in-loop (simple first pass) without resorting to heavy multiband crossovers.
    - Normalization with musical target and safety limiting that preserves punch without pumping.

- Dattorro-style Plate
  - Strengths: characteristic bright, fast buildup; classic sound for synths and percussion.
  - Market gaps: plates often sound too static or too metallic when pushed; width handling can be crude.
  - What we can do better:
    - Decorrelation strategies that avoid mono collapse at small sizes (mid/side aware crossfeed with parametric width).
    - Modulation restricted to fractional reads with small, sample-rate-aware depth to prevent chorusing artifacts.
    - Constant-power mix and calibrated output trims.

- Moorer/Schroeder Hybrids
  - Strengths: light CPU; controllable early/late balance.
  - Limitation: can sound dated/metallic without strong diffusion and modulation.
  - Use: lightweight “roomers” and pre-delay fields layered before FDNs for instrument placement.

## Convolution & Hybrid IR approaches
- Partitioned convolution IRs (uniform/mixed) for long tails; hybrid ER + algorithmic late tail.
- Opportunity: procedural IR synthesis for stylized spaces (plates, springs) with parameter morphing.
- Cost: convolution is predictable but static—best as layer or for realism; sound design often benefits more from lively algorithmics.

## Creative Reverb Concepts (sound design focus)

- MY IDEA: Spectral FDN (SFDN)
  - Concept: split signal into 3–5 coarse bands (Linkwitz–Riley or simple shelving) and run a small FDN per band with band-coupled parameters. Cross-feedback limited to neighboring bands for cohesion.
  - Why: proper low-end bloom without mud, airy highs with controlled damping, and purposeful mid density; avoids heavy multi-band overhead.
  - Implementation: lightweight filters per band + N=4/6 FDNs; global householder per band; global width; shared wander LFO; one safety limiter on wet sum.

- MY IDEA: Motion‑Mapped Reverb
  - Concept: map LFO/envelope followers to reverb structure (e.g., Size, Width, ER Width, Diffusion) using tempo-synced paths; offer morph lanes (A↔B) with ramps.
  - Why: sound design needs planned evolution; current reverbs give knobs but not motion design.
  - Implementation: parameter lanes with sync, bounded ranges; RT-safe smoothing; snapshot morphing with 2–3 scenes.

- MY IDEA: Texture Tail Engine
  - Concept: inject sparse, randomized velvet-noise clusters into late field, blended behind the FDN tail to add airy granularity at low CPU.
  - Why: subtle “sparkle” and motion without chorus smear; useful for pads and shimmery ambiences.
  - Implementation: block-rate velvet taps normalized per block, post-FDN mix with band-pass shaping; depth kept low to avoid grit.

- MY IDEA: ER Designer with Geometry Seeds
  - Concept: stochastic ER generator seeded by simple “room hints” (size class, material brightness); one-click variations.
  - Why: fast ideation of front-end spatial cues tailored to source; many plugins bury ER behind presets.
  - Implementation: tap clusters with jitter rules; width decorrelation; probabilistic patterns saved as seeds.

## Adjacent FX Opportunities (high leverage)

- Diffusion Shaper
  - A standalone “diffuser” effect (serial/allpass cluster with light modulation) used pre- or post-reverb to quickly add density to synths.
  - Market gap: quick, low-CPU thickener that is musical, not a reverb.

- Modulated Delay Designer
  - Multi-tap delay with per-tap diffusion and micro-mod; integrates constant-power cross-mix for widening; doubles as chorus/ensemble.

- Spectral Tilt & Bloom (in reverb loop)
  - Single control shapes decay tone without multi-band splits; maps to HF damping and LF shelf interaction.

## Implementation guidelines for our stack
- RT-safety: precompute per-block constants; avoid allocations in process; clamp ranges; denormal guards.
- Smoothing: fast (10–30 ms) for mix/diffusion; medium (100–250 ms) for size/decay; slow (250–750 ms) for damping/width/tilt.
- Mix law: constant-power sine/cosine.
- CPU: keep N=8 FDN as default; add Eco/High quality modes later.

## Initial Roadmap from this research
- Phase A: Finalize Hall/Plate SR-invariance audit and IR/EDR verification; add presets.
- Phase B: Prototype Texture Tail Engine (blend behind Hall); add single knob “Sparkle”.
- Phase C: ER Designer with Geometry Seeds; expose “Var” button for quick reshuffle.
- Phase D: Diffusion Shaper as standalone FX; integrate into modulation destinations.
- Phase E: Motion‑Mapped Reverb lanes with two scenes and tempo sync.
