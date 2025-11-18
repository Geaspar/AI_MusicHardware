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

Given the project’s primary goal (a hardware IoT synth with game-audio style vertical/horizontal resequencing), JUCE is strictly a portability/hosting layer to get the **existing engine** onto Elk OS/RPi-class boards. We do **not** want to rewrite the engine, just wrap it.

- JUCE plugin/standalone targets that:
  - Treat the current engine (`AIMusicCore` static lib: Sequencer, Synthesizer, IoT, Segment/vertical remix) as a black-box library.
  - Provide audio/MIDI/host sync integration for Elk OS and desktop.
- JUCE VST3 plugin target (headless or minimal editor) with:
  - Audio rendering via `AudioProcessor::processBlock()`.
  - Host tempo/position sync wired to `HostSync`, `ClockSource`, and `Sequencer::synchronizeWithAudioEngine()`.
  - Minimal exposed parameters via `AudioProcessorValueTreeState` (APVTS) for transport/tempo/sections.
  - MIDI input support (and optional MIDI output) for external controllers.
- JUCE standalone app target with the same engine wiring (optional tiny UI for bring-up and diagnostics).
- CMake integration with JUCE (desktop and cross-compile toolchains) that links against the existing `AIMusicCore` library.
- Deployment and validation scripts/instructions for Elk OS.

## 4) Phased Plan & Estimates

### Phase 1: Branch & Minimal JUCE Host (Standalone Spike, 2–4 days)

- Create a `juce-migration` branch from the current sequencer-stable baseline (tagged in `PROJECT_STATUS.md`).
- Add JUCE (submodule or `find_package(JUCE CONFIG)`).
- Implement a minimal **standalone JUCE host** that wraps the existing engine:
  - Use `juce::AudioAppComponent` or `AudioDeviceManager + AudioSourcePlayer`.
  - Link against `libAIMusicCore.a` and instantiate `AudioEngine`, `Synthesizer`, `Sequencer`, `EffectProcessor` exactly as in `src/main_integrated_simple.cpp`, but without SDL UI.
  - In `prepareToPlay`, call the same initialization paths (sample rate, voice count, etc.).
  - In `getNextAudioBlock`:
    - Compute `dt = numSamples / sampleRate`, derive `deltaBeats` as we do now, and call `Sequencer::process(deltaBeats)` (or reuse the existing `audioCallback` helper).
    - Call `synthesizer->process(buffer, numSamples)` to render into JUCE’s `AudioBuffer`.
  - Wire JUCE `MidiInput`/`MidiOutput` to the existing MIDI handler or directly to `synthesizer->noteOn/noteOff` + `Sequencer` as in the current main app.
- Outcome: Desktop JUCE app that proves our engine runs correctly under JUCE’s audio/MIDI model, without changing engine code.

### Phase 2: JUCE VST3 Plugin MVP for Elk (4–7 days)

- Add a JUCE `juce_add_plugin` target (VST3) that:
  - Links against `AIMusicCore`.
  - Implements `AudioProcessor::prepareToPlay`, `processBlock`, `releaseResources` by delegating to `AudioEngine`/`Synthesizer`/`Sequencer`.
  - In `processBlock`:
    - Derive `dt = numSamples / sampleRate`.
    - Call `AudioPlayHead::getCurrentPosition()` to obtain `CurrentPositionInfo`:
      - Feed `HostSync` with `bpm`, `ppqPosition`, `sampleRate`.
      - Call `Sequencer::synchronizeWithAudioEngine(timeInSeconds, sampleRate)` if appropriate.
      - Drive `Sequencer::process(deltaBeats)` based on host tempo/position.
    - Map `MidiBuffer` events to our existing MIDI handler or directly to synth/sequencer.
- Parameters (APVTS) — start minimal:
  - Transport: play/stop/loop.
  - Tempo (if not host-driven), swing.
  - Probability mode (PerHitRandom/PerLoopStable), per-loop dedupe enable.
  - Section navigation: jump/next/prev for vertical resequencing segments.
- Outcome: VST3 plugin loads in JUCE Plugin Host/DAW and, later, in Elk’s plugin host; audio and timing behave identically to the current RtAudio app.

### Phase 3: Minimal JUCE UI (Desktop Dev Tool, 3–6 days; optional for Elk)

Even though Elk can run headless, for development we want a small JUCE editor that mirrors the **Patterns** and **Sequencer** essentials:

- Editor layout:
  - Transport controls: Play/Stop, Loop.
  - Tempo slider: calls `sequencer->setTempo()` and optionally updates host tempo.
  - Section controls: a simple selector + Jump/Next/Prev, wired to `defineSections` / `jumpToSection` (`Timing::OnBeat`/`OnBar`).
  - Patterns Test Mode button: calls a JUCE equivalent of `runSequencerUITests` that fills `Pattern` objects with the spaced and consecutive 16th-note tests.
  - Simple debug labels (no heavy graphics) that replicate:
    - `[DBG] col X fired Y` for the current 16th column and per-loop fired count.
    - A minimal envelope scope/voice count if needed.
- This editor is for **desktop debugging only**; Elk builds can remain headless or use a very small control panel.

### Phase 4: Optional Full UI Port (2–4 weeks, nice-to-have)

- If we later decide that JUCE should be the **primary** UI (replacing SDL on desktop):
  - Port the existing grid-based Patterns page, Sections/Segments editor, Sensors mappings, and Preset browser to JUCE `Component`s.
  - Reuse existing engine APIs and data models (Pattern/Sequencer/SegmentSequencer) without changing their semantics.
  - Keep Elk deployments minimal; full UI would primarily benefit desktop and possibly richer front panels.

## 5) Technical Design

### 5.1 Engine Wiring in JUCE

The guiding principle: **keep the current engine intact** (Sequencer, SegmentSequencer, IoT/MQTT, Synthesizer, vertical/vertical remix) and make JUCE call into it, just like the current RtAudio/SDL front-end.

- Standalone (desktop + Elk bring-up):
  - `AudioDeviceManager` / `AudioAppComponent` → audio callback:
    - Compute `deltaTime = numSamples / sampleRate`.
    - Convert to beats using `tempo` / `beatTimeSeconds` and call `Sequencer::process(deltaBeats)` (or reuse the existing `audioCallback` helper that already does this).
    - Call `synthesizer->process(buffer, numSamples)` to fill the JUCE audio buffer.
  - Use JUCE `MidiInput` callbacks to feed the existing MIDI handler:
    - Note on/off → `synthesizer->noteOn/noteOff` and/or `Sequencer` as we already do in `main_integrated_simple.cpp`.
  - Use `ClockSource` for internal/external pulses when the app is not hosted by a DAW.

- Plugin (for Elk host and DAWs):
  - `AudioProcessor::prepareToPlay(sampleRate, blockSize)`:
    - Initialize `AudioEngine`/`Synthesizer`/`Sequencer` with the given `sampleRate`.
  - `AudioProcessor::processBlock(AudioBuffer<float>&, MidiBuffer&)`:
    - Compute `deltaTime = numSamples / sampleRate`.
    - Query `AudioPlayHead::CurrentPositionInfo`:
      - Feed `HostSync` (BPM, PPQ position, sampleRate).
      - Optionally call `Sequencer::synchronizeWithAudioEngine(timeInSeconds, sampleRate)` if we want engine-side alignment.
    - Use `ClockSource` + host tempo to derive `deltaBeats` and call `Sequencer::process(deltaBeats)`.
    - Map `MidiBuffer` to our MIDI handler (or directly to `Synthesizer` note on/off).
    - Render audio via `synthesizer->process()` into the JUCE buffer.

### 5.5 Proposed File Layout for JUCE Targets

To keep the JUCE layer clearly separated from the core engine, add a `juce/` subtree with the following structure:

- `juce/CMakeLists.txt`
  - Defines JUCE targets and links against `AIMusicCore` from the top-level build.
  - Example targets:
    - `AIMH_JuceStandalone` — minimal desktop/Elk bring-up app.
    - `AIMH_JucePlugin` — VST3 plugin for Elk host + desktop testing.
- `juce/ElkSynthStandaloneApp.cpp`
  - Implements a small `juce::AudioAppComponent` (or `AudioDeviceManager` + `AudioSourcePlayer`) that:
    - Owns `std::unique_ptr<AIMusicHardware::Synthesizer>`, `Sequencer`, and optional `EffectProcessor`.
    - In `prepareToPlay`, configures the engine with the JUCE sample rate.
    - In `getNextAudioBlock`, calls the existing engine callback logic (sequencer + synth) and writes into the JUCE buffer.
- `juce/ElkSynthPluginProcessor.h/.cpp`
  - `class ElkSynthProcessor : public juce::AudioProcessor`:
    - Owns the same engine objects (or a wrapper struct, e.g. `EngineContext`).
    - Implements `prepareToPlay`, `processBlock`, `releaseResources`.
    - Exposes minimal APVTS parameters for transport/tempo/sections.
- `juce/ElkSynthPluginEditor.h/.cpp` (optional, desktop-only)
  - Minimal editor providing:
    - Transport buttons, tempo slider.
    - Section selector and jump/next/prev controls.
    - Patterns Test Mode button.
    - A text overlay for `[DBG] col X fired Y`.

The top-level `CMakeLists.txt` can then `add_subdirectory(juce)` when JUCE is available on the system.

### 5.6 `processBlock` Bridge Sketch (Plugin)

Below is a concrete sketch of how the plugin’s `processBlock` bridges JUCE’s `AudioBuffer`/`MidiBuffer` to the existing engine. This is illustrative; actual types (e.g., `AudioEngine`) can be adjusted to your current architecture.

```cpp
// juce/ElkSynthPluginProcessor.h (sketch)
class ElkSynthProcessor : public juce::AudioProcessor {
public:
  ElkSynthProcessor();
  ~ElkSynthProcessor() override = default;

  void prepareToPlay(double newSampleRate, int samplesPerBlock) override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  void releaseResources() override;

  // ... standard JUCE boilerplate ...

private:
  double sampleRate_ = 44100.0;
  std::unique_ptr<AIMusicHardware::Synthesizer> synth_;
  std::unique_ptr<AIMusicHardware::Sequencer>   seq_;
  std::unique_ptr<AIMusicHardware::HostSync>    hostSync_;
  std::unique_ptr<AIMusicHardware::ClockSource> clockSource_;
};
```

```cpp
// juce/ElkSynthPluginProcessor.cpp (sketch)
ElkSynthProcessor::ElkSynthProcessor() {
  synth_     = std::make_unique<AIMusicHardware::Synthesizer>(44100);
  seq_       = std::make_unique<AIMusicHardware::Sequencer>(120.0, 4);
  hostSync_  = std::make_unique<AIMusicHardware::HostSync>();
  clockSource_ = std::make_unique<AIMusicHardware::ClockSource>();

  seq_->attachHostSync(hostSync_.get());
  seq_->attachClockSource(clockSource_.get());

  seq_->setNoteCallbacks(
    [this](int pitch, float vel, int ch, const AIMusicHardware::Envelope& env) {
      if (synth_) synth_->noteOn(pitch, vel, env, ch);
    },
    [this](int pitch, int ch) {
      if (synth_) synth_->noteOff(pitch, ch);
    }
  );
}

void ElkSynthProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock) {
  sampleRate_ = newSampleRate > 0.0 ? newSampleRate : 44100.0;
  if (synth_) synth_->setSampleRate((int)sampleRate_);
  if (seq_)   seq_->synchronizeWithAudioEngine(0.0, sampleRate_);
}

void ElkSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midi) {
  juce::ScopedNoDenormals noDenormals;
  const int numSamples = buffer.getNumSamples();

  // Clear buffer first; our engine writes full-range audio.
  buffer.clear();

  // 1) Handle MIDI: map host MIDI to engine
  for (const auto metadata : midi) {
    const auto msg  = metadata.getMessage();
    const int  ch   = msg.getChannel() - 1;

    if (msg.isNoteOn()) {
      const int   note = msg.getNoteNumber();
      const float vel  = msg.getVelocity(); // 0..1 in JUCE
      if (synth_) synth_->noteOn(note, vel, ch);
      // Optionally also route into Sequencer for live recording, etc.
    } else if (msg.isNoteOff()) {
      const int note = msg.getNoteNumber();
      if (synth_) synth_->noteOff(note, ch);
    }
    // Handle CC/pitch bend/aftertouch as needed.
  }

  // 2) Host sync → HostSync/Sequencer
  if (auto* playHead = getPlayHead()) {
    juce::AudioPlayHead::CurrentPositionInfo cpi;
    if (playHead->getCurrentPosition(cpi)) {
      const double bpm  = cpi.bpm > 1.0 ? cpi.bpm : seq_->getTempo();
      const double ppq  = cpi.ppqPosition;
      const double sr   = sampleRate_;

      hostSync_->updateFromHost(bpm, ppq, sr);
      // Optional: align beat time to host seconds
      seq_->synchronizeWithAudioEngine(cpi.timeInSeconds, sr);
    }
  }

  // 3) Advance Sequencer in beats using either ClockSource or internal tempo
  const double dtSeconds   = (double)numSamples / sampleRate_;
  const double secPerBeat  = seq_->getPreciseBeatTime(); // or 60.0 / tempo
  const double beatsPerSec = secPerBeat > 0.0 ? 1.0 / secPerBeat : 0.0;
  const double deltaBeats  = dtSeconds * beatsPerSec;
  seq_->process(deltaBeats);

  // 4) Render audio from Synthesizer into JUCE buffer
  if (synth_) {
    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    // Use a temporary interleaved buffer if the engine expects interleaved stereo.
    std::vector<float> tempInterleaved((size_t)numSamples * 2, 0.0f);
    synth_->process(tempInterleaved.data(), numSamples);

    for (int i = 0; i < numSamples; ++i) {
      const float l = tempInterleaved[2 * i + 0];
      const float r = tempInterleaved[2 * i + 1];
      left[i]  += l;
      if (right) right[i] += r;
    }
  }
}
```

The standalone JUCE host’s audio callback (`getNextAudioBlock`) would look almost identical, except it would use `AudioSourceChannelInfo` instead of `AudioProcessor::processBlock`, and it wouldn’t need to query `AudioPlayHead` (unless we simulate host transport there as well).

### 5.7 Plugin vs Standalone Performance on Elk OS

From a DSP/CPU perspective, a JUCE **plugin** and a JUCE **standalone app** that both:

- Run in Release on the same hardware,
- Use the same buffer sizes and sample rates,
- And simply call into your existing engine (`Sequencer::process`, `Synthesizer::process`) once per audio block,

will have essentially the **same hardware performance**. The main differences are integration and deployment:

- On Elk OS, the system is optimized around **plugins**:
  - The Elk host already manages the audio device, buffer sizes, and RT scheduling.
  - A single headless plugin adds only a very small wrapper cost (host → `processBlock` call, parameter/state handling) compared to your DSP.
- A JUCE **standalone** app can be just as efficient if it’s headless and configured identically, but on Elk you would then be re‑implementing what the host already provides (device management, routing, etc.).
- UI cost (especially on RPi‑class hardware) dominates any plugin vs standalone overhead. Elk builds should remain headless or use minimal controls regardless of plugin/standalone choice.

**Conclusion for this project:**

- **Best long‑term target on Elk OS**: a **headless JUCE plugin** (VST3/LV2 per Elk’s recommendation) that wraps the existing engine. It integrates cleanly with Elk’s audio engine and has no meaningful performance penalty versus a standalone app.
- **Best for development on desktop**: keep a small **JUCE standalone host** for quick bring‑up and debugging (Patterns, vertical resequencing, IoT behavior), but treat the plugin build as the final deployment form for Elk hardware.

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
