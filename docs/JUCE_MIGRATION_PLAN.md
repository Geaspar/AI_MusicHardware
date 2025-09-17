# JUCE Migration Plan for Elk OS

This plan outlines how to migrate the current AIMusicHardware engine (AudioEngine, Synthesizer, Sequencer) to a JUCE-based application and plugin, optimized for running on Elk OS. It consolidates the options, phases, tasks, risks, and validation strategy discussed.

## 1) Context & Rationale
- Elk OS integrates best with JUCE-based apps/plugins, especially headless VST3 with real-time constraints.
- Our core DSP + Sequencer are now robust (adaptive epsilon, dedupe lifecycle, stable probability, loop-wrap, external clock, host sync scaffolds), making them good candidates to host in JUCE.

## 2) Goals
- Primary: Produce a headless JUCE VST3 plugin that hosts our engine on Elk OS with low latency and stable transport/host sync.
- Secondary: Provide a minimal JUCE standalone app for bring-up and diagnostics on Elk hardware and desktop.
- Optional: Provide a small JUCE editor for desktop use (transport + simple grid) to facilitate testing and debugging.

## 3) Deliverables
- JUCE VST3 plugin target (headless) with:
  - Audio rendering via `AudioProcessor::processBlock()`
  - Host tempo/position sync wired to our `HostSync` and `Sequencer::synchronizeWithAudioEngine()`
  - Exposed parameters via `AudioProcessorValueTreeState` (APVTS)
  - MIDI input support and optional MIDI output
- JUCE standalone app target with the same engine wiring (optional tiny UI for bring-up)
- CMake integration with JUCE (desktop and cross-compile toolchains)
- Deployment and validation scripts/instructions for Elk OS

## 4) Phased Plan & Estimates

- Phase 1: JUCE Standalone Spike (2–4 days)
  - Add JUCE via submodule or `find_package(JUCE CONFIG)`
  - Implement `juce::AudioAppComponent` or `AudioDeviceManager + AudioSourcePlayer` wrapper around our engine
  - Replace RtMidi with JUCE `MidiInput/MidiOutput` in this target only
  - Drive tempo internally or via `ClockSource`; expose basic CLI flags
  - Outcome: Headless/tiny UI app runs at 48kHz small buffers on Linux

- Phase 2: JUCE VST3 Plugin MVP (4–7 days)
  - `juce_add_plugin` target (VST3), no heavy UI (headless or minimal editor)
  - Map `prepareToPlay`, `processBlock`, `releaseResources` to AudioEngine/Synthesizer/Sequencer
  - Host sync: read `AudioPlayHead::CurrentPositionInfo`, feed `HostSync` and call `synchronizeWithAudioEngine`
  - Parameters: APVTS for tempo, swing, loop, probability mode, per-loop dedupe, section jumps
  - MIDI I/O: map host MIDI to the engine, ensure deterministic latency
  - Outcome: Loads in Elk’s plugin host, renders audio correctly, responds to host transport

- Phase 3: Minimal JUCE UI (3–6 days, optional for Elk)
  - Editor with transport controls, small step grid for one pattern, and section select
  - Thread-safe bridges to our engine (we already have mutex/atomic design)
  - Outcome: Desktop-friendly editor for development; not required for Elk deployment

- Phase 4: Full UI Parity (2–4 weeks, optional)
  - Rebuild multi-page UI (grid, sections, sensors, effects, preset browser) with JUCE Components
  - Improve look & feel and persistence

## 5) Technical Design

### 5.1 Engine Wiring in JUCE
- Standalone:
  - `AudioDeviceManager` → callback → compute `deltaTime` and call `Sequencer::process(deltaBeats)`
  - Use our `ClockSource` for internal/external pulses when not hosted
  - Use JUCE `MidiInput` callbacks → forward to our MIDI layer or Synth
- Plugin:
  - `AudioProcessor::prepareToPlay(sampleRate, blockSize)` → give to AudioEngine/Synth
  - `AudioProcessor::processBlock(AudioBuffer<float>&, MidiBuffer&)`
    - Derive `deltaTime` from `numSamples/sampleRate`
    - Obtain `AudioPlayHead::CurrentPositionInfo cpi`; call `hostSync.updateFromHost(cpi.bpm, cpi.ppqPosition, sampleRate)`
    - `sequencer.synchronizeWithAudioEngine(cpi.timeInSeconds, sampleRate)`
    - Drive `sequencer.process(adjustedDeltaBeats)` and render audio

### 5.2 Parameter Bridge (APVTS)
- Expose minimal controls initially:
  - Transport: play, stop, loop
  - Tempo (if not host-driven), swing
  - Probability: mode (PerHitRandom/PerLoopStable), global chance (optional)
  - Dedupe: per-loop dedupe enabled
  - Section: jump/next/prev
- Use APVTS attachments or direct parameter reads, map to existing setter APIs

### 5.3 MIDI & External Clock
- MIDI input: map host `MidiBuffer` to our existing MIDI manager or direct note callbacks
- External clock: optional—convert MIDI Clock or host pulses to `ClockSource` edges if needed

### 5.4 Build & Packaging
- CMake with JUCE:
  - Add JUCE as submodule or use `find_package(JUCE CONFIG)` if installed
  - Targets: `juce_add_plugin(AIMH_Plugin ...)` and `juce_add_gui_app(AIMH_Standalone ...)`
  - Link our `AIMusicCore` static lib
- Cross-compile for Elk:
  - Toolchain: aarch64-linux-gnu; set sysroot
  - Configure Release flags: `-O3 -fno-exceptions` (if feasible), LTO optional
  - Disable or ifdef any platform-specific code paths not needed on Elk

## 6) Real-time Constraints & Practices
- No heap allocations in the audio thread (we’ve added reserves, removed rand())
- Avoid locking where possible in the audio path; our design already minimizes lock time and defers callbacks outside locks
- Disable denormals; consider fast-math where safe; profile on target
- Keep UI/editor out of the audio thread (JUCE model already enforces this)

## 7) Validation & Test Plan
- Desktop validation:
  - Build the plugin; run in a DAW or JUCE Plugin Host
  - Verify host tempo/transport sync (start/stop, on-beat/on-bar section jumps)
  - Validate no boundary double-triggers (dedupe on) and stable probability per loop
- Elk OS validation:
  - Deploy headless VST3; route MIDI and transport messages; verify latency and CPU
  - Verify 48 kHz, small buffers, and long-run stability
- Automated checks:
  - Reuse existing headless examples and timing tests (Sequencer loop/timing regression tests)

## 8) Risks & Mitigations
- Host sync variance across hosts → abstract through `HostSync`, add fallback to internal `ClockSource`
- Parameter explosion → start minimal APVTS set; expand iteratively
- UI cost/time → keep Elk build headless; maintain desktop editor for dev only
- Cross-compile friction → start with desktop builds; then add aarch64 toolchain and CI recipe

## 9) Licensing
- JUCE: GPL for open-source or a commercial license for closed-source distribution
- Our current codebase: compatible with JUCE GPL if we remain open-source; review external licenses (e.g., Vital snippets are GPLv3; RtMidi is MIT-like)

## 10) Open Questions
- Targets: VST3 plugin only, or plugin + standalone?
- UI: Headless on Elk; how much desktop editor do we want in phase 3?
- MIDI clock support on Elk: required initially or post-MVP?
- Deployment pipeline for Elk: preferred packaging method?

## 11) Milestones & Acceptance
- M1 (Standalone Spike): Audio renders, MIDI in, internal/ClockSource tempo; runs at 48kHz small buffers (2–4 days)
- M2 (Plugin MVP): VST3 renders in host; host sync drives sequencer timing; parameters exposed & functional (4–7 days)
- M3 (Elk Bring-up): Plugin runs on Elk, passes timing/loop boundary tests under host control (1–3 days)
- M4 (Optional UI): Desktop JUCE editor with transport + grid for development (3–6 days)

## 12) Next Steps
- Confirm scope (plugin vs standalone, desktop editor) and target OS matrix
- Add JUCE to the repo (submodule or package) and scaffold CMake targets
- Implement AudioProcessor wrapper + APVTS, wire HostSync/ClockSource
- Validate on desktop; then cross-compile and deploy to Elk

