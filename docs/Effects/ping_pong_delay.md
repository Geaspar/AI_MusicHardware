## Ping‑Pong Delay — Tempo‑Synced Stereo Echo (Aug 2025)

A musical stereo delay where echoes alternate (ping‑pong) between left and right channels. Includes tempo sync, time modulation, feedback filtering, cross‑feedback/widening, ducking, and smear (diffusion) for softer tails. Vital reference: modulation smoothing and RT‑safe parameter handling.

### Goals
- Clear, tempo‑locked stereo repeats with controllable width
- Stable feedback with filters and optional diffusion
- Low CPU and click‑free under automation

---

## Core Features
- Time: ms or tempo divisions (note, dotted, triplet)
- Ping‑Pong: alternate L/R with cross‑feedback; stereo offset (ms) for feel
- Feedback: 0 … 0.95 with safety clamp
- Feedback Filters: HP/LP in feedback path
- Tone: tilt or simple shelf to age repeats
- Modulation: time modulation (wow/chorus) depth/rate with stereo phase offset
- Ducking: auto‑duck repeats when input present (sidechain detector)
- Smear/Diffusion: small diffuser in feedback loop to soften taps
- Width: scale of ping‑pong offset; mono‑safe at width 0
- Mix (constant‑power) and Output Trim

---

## Parameters
- Mix (0–1, constant‑power)
- Time (ms): 10 … 2000 or Sync Division (enum): 1/64 … 2 bars + dotted/triplet
- Stereo Offset (ms): −20 … +20 (pre‑delay difference L/R)
- Ping‑Pong (bool): ON alternates sides; OFF leaves per‑channel delays
- Feedback: 0 … 0.95
- HP (Hz): 20 … 2k (feedback path)
- LP (Hz): 1k … 20k (feedback path)
- Tone Tilt (dB): −6 … +6 on repeats
- Mod Rate (Hz): 0.05 … 5.0
- Mod Depth (ms): 0 … 10
- Mod Stereo Phase (deg): 0 … 180
- Ducking Amount (dB): 0 … 24; Ducking Release (ms): 50 … 1000
- Smear (0 … 1): diffusion amount in feedback loop
- Width (0 … 1)
- Output Trim (dB): −12 … +12

Suggested UI (2 pages)
- Page 1: Mix, Time/Sync, Feedback, HP/LP, Ping‑Pong, Width
- Page 2: Stereo Offset, Mod Rate/Depth/Phase, Ducking (Amt/Release), Smear, Tone Tilt, Output Trim

---

## Architecture

Delay lines
- Two delays (DL, DR) with fractional reads; ping‑pong alternates feedback routing L↔R
- Stereo offset adds small pre‑delay difference

Feedback path
- Apply HP/LP filters and tone tilt to feedback signal; optional diffusion (2–4 allpasses) for smear
- Feedback clamp and gain compensation to prevent runaway

Modulation
- Modulate delay time with low depth; apply cross‑faded fractional read to avoid clicks
- Stereo phase offset for L/R modulation (wider motion)

Ducking
- Detector on input; when above threshold, reduce wet gain by Ducking Amount; release back smoothly

Width & mix
- Width scales side contribution; ping‑pong alternation plus width control governs stereo image
- Constant‑power wet/dry mix; Output Trim after

Smoothing & safety
- Smooth Time/Feedback/Filters/Mod 10–50 ms; crossfade time changes
- Denormal guards; no allocations in audio thread

---

## Mod Matrix Destinations
- PingPong Mix, Time, Feedback, HP, LP, Tone Tilt, Stereo Offset, Ping‑Pong (stepped), Width, Mod Rate/Depth/Phase, Ducking Amount/Release, Smear, Output Trim

---

## Implementation Plan

Phase 0 — Scaffolding (0.5 day)
- Add `include/effects/PingPongDelay.h`, `src/effects/PingPongDelay.cpp`; register; UI mapping (2 pages)

Phase 1 — Core Stereo Delay (1 day)
- Dual fractional delays with ping‑pong cross‑feedback; Time/Sync; Feedback with HP/LP and Tilt; Width; Mix/Trim; smoothing

Phase 2 — Modulation, Ducking, Smear (0.5–1 day)
- Time modulation with xfade read; ducking detector; diffusion allpasses in feedback path

Phase 3 — Integration & Tuning (0.5 day)
- Mod destinations; preset persistence; listening tests; CPU profiling; finalize ranges

---

## Math & Pseudocode

Ping‑pong cross feedback
```cpp
// yL = DL(inL + fbL), yR = DR(inR + fbR)
// ping-pong routing
fbL = feedback * (pingPong ? yR : yL);
fbR = feedback * (pingPong ? yL : yR);
```

Time modulation (xfade)
```cpp
float yA = readFrac(delay, tA);
float yB = readFrac(delay, tB);
float y  = (1-x)*yA + x*yB; // xfade during retime
```

Ducking
```cpp
float det = detector(abs(inL)+abs(inR));
float duckGain = dBToLin(-duckAmt) when det>thresh, released with one-pole to 1.0
wet *= duckEnv;
```

Constant‑power mix
```cpp
wetK = sinf(0.5f*M_PI*mix); dryK = cosf(0.5f*M_PI*mix);
outL = dryK*inL + wetK*yL; outR = dryK*inR + wetK*yR;
```

---

## References
- Stereo ping‑pong delay designs; Haas/chorus modulation techniques
- Vital: smoothing/mix patterns; allpass diffusers
