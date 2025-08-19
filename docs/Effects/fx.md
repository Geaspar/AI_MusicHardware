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

## Best-in-class approaches and how we improve (by effect)

- Reverb (Room/Hall/Plate/Spring)
  - Best-in-class: Valhalla, FabFilter Pro-R (ER shaping, decay control), Lexicon heritage algorithms; Vital leverages simple diffusion + plates for character.
  - Our edge: FDN with per-band shaping (SFDN idea), motion-mapped parameters, constant-power mix, SR-invariant modulation depths, calibrated normalization.

- Delay (Mono/Stereo/Ping‑Pong)
  - Best-in-class: SoundToys EchoBoy (tone/character modes), FabFilter Timeless (mod matrix); Vital uses clean synced delays with feedback filters.
  - Our edge: per-tap diffusion and micro‑mod, constant‑power crossfeed for ping‑pong, ducking sidechain from input, and safe feedback clamps.

- Chorus/Ensemble
  - Best: TAL‑Chorus‑LX (Juno style), Valhalla UberMod variants; Vital: multiple delay voices with phase offsets.
  - Our edge: multi‑voice with decorrelated LFOs, slight random step jitter, tone filter per voice, stereo‑aware spread with constant‑power panning.

- Flanger
  - Best: Through‑zero flanging (TZF) with delay sign flip; BBD emulations add character.
  - Our edge: TZF mode plus gentle nonlinearity and DC guard, optional diffusion for thicker sweeps.

- Phaser
  - Best: Multi‑stage allpass with matched poles/zeros; envelope and LFO modulation; Vital includes a clean phaser.
  - Our edge: 8–12 stages with anti‑zipper smoothing, stereo phase offset, optional feedback soft‑limit, musical notch spacing presets.

- Tremolo (AM)
  - Best: Soft‑shaped LFOs, stereo phase offsets, bias controls.
  - Our edge: dual‑LFO crossfade for rhythmic patterns, envelope follower to bias depth, constant‑power amplitude mapping.

- Vibrato (PM)
  - Best: High‑quality fractional delay; anti‑alias LFO shapes.
  - Our edge: SR‑invariant depth in samples, noise‑shaped LFO for analog drift flavor.

- Ring Modulator
  - Best: Clean carrier, optional DC/high‑pass post stage.
  - Our edge: stereo carriers with slight decorrelation, pitch‑tracked option, foldback protection.

- Frequency Shifter
  - Best: Quadrature Hilbert transform approach; barber‑pole illusions.
  - Our edge: psychoacoustic helpers (mix law, anti‑chirp smoothing), M/S routing for creative width.

- Saturation/Tape
  - Best: Soft‑clip curves, hysteresis, frequency‑dependent drive; Vital includes nonlinear drive in some FX.
  - Our edge: tone‑compensated drive, oversampling on hot modes, output trim with RMS normalization window.

- Overdrive/Distortion/Waveshaper
  - Best: Curated transfer functions (Tanh/Asym/Hard/Diode), pre/post EQ; stageable.
  - Our edge: dynamic bias for touch sensitivity, tilt EQ auto‑comp, transient‑aware stage chaining.

- Bitcrusher/SRR
  - Best: Fractional bit‑depth with noise shaping, sample‑rate decimation with interpolation options; Vital’s BitCrusher is straightforward.
  - Our edge: adaptive dither/noise‑shape mix, post‑drive and trim, stereo decorrelation to reduce grittiness.

- Compressor
  - Best: Feed‑forward/feedback variants, sidechain filter, soft knee; look‑ahead for limiting.
  - Our edge: program‑dependent release (two‑pole), auto gain with RMS target, simple GR meter for UI.

- Limiter
  - Best: Look‑ahead brickwall, ISP protection.
  - Our edge: low‑latency mode with soft‑clip pre‑stage, musical release to reduce pumping.

- Transient Shaper
  - Best: dual‑envelope attack/sustain detection with frequency weighting.
  - Our edge: tilt‑weighted detection, sidechain HP/LP, stereo‑linked or dual‑mono modes.

- EQ (Parametric/Tilt)
  - Best: RBJ biquads, proportional‑Q, linear‑phase options.
  - Our edge: proportional‑Q parametrics with musical ranges, macro tilt for fast shaping.

- Filters (LP/HP/BP/Notch/Formant)
  - Best: ZDF ladders and state‑variable filters with drive; Vital’s filters are musical and varied.
  - Our edge: ZDF SVF with morphing modes, formant sets, drive and key‑track built‑in.

- Auto‑Wah/Envelope Filter
  - Best: Envelope follower mapped to resonant LP/BP with bias and attack/release.
  - Our edge: follower with adaptive time constants, sidechain input, and optional LFO blend.

- Stereo Imager/Width (M/S)
  - Best: Mid/Side split with safe mono compatibility and linear‑phase options.
  - Our edge: constant‑power width, bass‑safe narrowing, transient‑aware widening.

- Granular/Freeze/Shimmer
  - Best: Grain engines (Clouds/Portal), shimmer via pitch‑shifting feedback paths.
  - Our edge: low‑CPU shimmer via octave‑tap into FDN tail; freeze with diffusion maintenance to avoid static loops.

References to our code and Vital
- Our current implementation: see `src/effects/*` (Modulation/Chorus, BitCrusher, Phaser, Distortion, Saturation, Reverb classes), and UI wiring in `src/main_integrated_simple.cpp`.
- Vital approach: Vital uses high‑quality filters, diffusion, and modulation; for effect‑specific inspiration, reference Vital’s open components (e.g., SVF filters, phaser, chorus) and adapt patterns (per‑block smoothing, modulation routing, CPU budgeting). Where applicable, we mirrored constant‑power mix, SR‑invariance, and parameter smoothing.

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
