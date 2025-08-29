# Sequencer Guide

## Overview

The AIMusicHardware sequencer system provides comprehensive pattern-based sequencing inspired by industry-leading hardware and software sequencers. It features adaptive capabilities, multi-track support, and game audio-inspired dynamic behavior.

## Key Features

- **Multi-pattern system** with 64+ pattern storage
- **Variable pattern lengths** (1-64 steps, up to 8 bars)
- **8 simultaneous tracks** with independent patterns
- **Real-time recording and step entry**
- **Parameter automation** with smooth curves
- **Adaptive sequencing** based on game audio concepts
- **Performance controls** for live manipulation
- **Horizontal re-sequencing** and vertical remixing

## Quick Start

### Basic Pattern Creation

```cpp
#include "sequencer/Sequencer.h"

// Create sequencer
auto sequencer = std::make_unique<Sequencer>();
sequencer->initialize(44100, 256);  // Sample rate, buffer size

// Create a basic pattern
auto pattern = sequencer->createPattern("basic_pattern", 16);  // 16 steps

// Add notes to the pattern
pattern->setNote(0, 60, 127, 1.0f);   // Step 0: C4, velocity 127, full gate
pattern->setNote(4, 64, 100, 0.5f);   // Step 4: E4, velocity 100, half gate
pattern->setNote(8, 67, 110, 0.75f);  // Step 8: G4, velocity 110, 3/4 gate

// Start playback
sequencer->play();
```

### Real-time Recording

```cpp
// Enable real-time recording
sequencer->setRecording(true);
pattern->setRecordMode(Pattern::RecordMode::Overdub);

// Notes played will be automatically recorded to the active pattern
```

## Core Components

### 1. Pattern System

#### Pattern Creation and Management
```cpp
// Create patterns with different lengths
auto drumPattern = sequencer->createPattern("drums", 16);
auto bassPattern = sequencer->createPattern("bass", 32);
auto leadPattern = sequencer->createPattern("lead", 8);

// Pattern variations
pattern->createVariation("A");
pattern->createVariation("B");
pattern->switchToVariation("B");
```

#### Pattern Properties
```cpp
// Set pattern properties
pattern->setLength(32);                    // 32 steps
pattern->setSwing(0.1f);                  // 10% swing
pattern->setTranspose(2);                 // Transpose up 2 semitones
pattern->setScale(Scale::Minor);          // Force to minor scale
pattern->setProbability(0.8f);            // 80% trigger probability
```

### 2. Track Management

#### Multi-Track Setup
```cpp
// Create different track types
auto drumTrack = sequencer->createTrack("drums", Track::Type::Drum);
auto bassTrack = sequencer->createTrack("bass", Track::Type::Melodic);
auto automationTrack = sequencer->createTrack("filter_auto", Track::Type::Automation);

// Assign patterns to tracks
drumTrack->assignPattern(drumPattern);
bassTrack->assignPattern(bassPattern);
```

#### Track Coupling
```cpp
// Link tracks for synchronized operations
sequencer->linkTracks({"drums", "bass"});  // Synchronized playback
sequencer->setTrackGroup("rhythm", {"drums", "bass", "percussion"});
```

### 3. Step Programming

#### Note Entry
```cpp
// Basic note entry
step->setNote(pitch, velocity, gate);
step->setTrigger(true);
step->setMicrotiming(0.05f);  // Slight timing offset

// Advanced step properties
step->setProbability(0.7f);           // 70% chance to trigger
step->setCondition(Condition::Every2nd); // Only every 2nd time
step->setRetrigger(2, 0.25f);         // 2 retriggers at 1/4 intervals
```

#### Parameter Automation
```cpp
// Automate synthesizer parameters
auto filterAuto = pattern->getAutomationTrack("filter_cutoff");
filterAuto->setAutomationPoint(0, 0.2f);   // Step 0: Low cutoff
filterAuto->setAutomationPoint(8, 0.8f);   // Step 8: High cutoff
filterAuto->setAutomationPoint(15, 0.3f);  // Step 15: Medium cutoff

// Set interpolation curve
filterAuto->setCurveType(CurveType::Exponential);
```

### 4. Performance Controls

#### Real-time Manipulation
```cpp
// Pattern chaining
sequencer->queuePattern("intro", PatternTransition::Immediate);
sequencer->queuePattern("verse", PatternTransition::OnBar);
sequencer->queuePattern("chorus", PatternTransition::OnBeat);

// Live muting/soloing
sequencer->muteTrack("drums");
sequencer->soloTrack("lead");

// Parameter locks (per-step parameter changes)
step->addParameterLock("filter_resonance", 0.8f);
step->addParameterLock("osc_detune", 0.2f);
```

#### Groove and Timing
```cpp
// Apply groove template
sequencer->setGrooveTemplate(GrooveTemplate::Swing16);
sequencer->setGrooveAmount(0.6f);

// Manual timing adjustment
pattern->setStepTiming(4, 0.03f);   // Step 4 slightly late
pattern->setStepTiming(12, -0.02f); // Step 12 slightly early
```

## Adaptive Sequencing

### Game Audio Integration

The sequencer includes adaptive capabilities inspired by game audio middleware:

### MVP Resequencing Plan (Phase A → C)

- Phase A: Resequencing hooks on current Sequencer
  - API (new): `queueSection(name, when)`, `jumpToSection(name, when)`, `nextSection(when)`, `prevSection(when)`, with `when` ∈ {Immediate, OnBeat, OnBar}.
  - Minimal section registry: default A/B/C at 0/1/2 bars; configurable via `defineSections({{"A",0},{"B",4},...})`.
  - Scheduling: OnBar/OnBeat executed at musical boundaries; Immediate applies right away.
  - Sensors: existing “Resequence: Next/Prev/Jump A/B/C/Shuffle” now call this API (OnBar).

- Phase B: Segment scaffolding
  - Introduce light `Segment` data (length, tempo, tags, entry/exit points) and `SegmentTransition` (Immediate/NextBeat/NextBar/ExitPoint, probability, priority).
  - Minimal `SegmentSequencer` to schedule transitions at boundaries; callbacks for UI.

- Phase C: Conditions + probabilities
  - Transition parameter conditions (>, <, hysteresis), weighted random selection.
  - EventBus hooks to trigger resequencing via events.

Notes
- Horizontal re-sequencing docs (see `HORIZONTAL_RESEQUENCING_IMPLEMENTATION.md`) inform Phase B/C.
- MVP avoids DSP changes; resequencing manipulates position/queue only.

```cpp
// State-based sequencing
sequencer->defineState("calm", {
    .tempoRange = {80, 100},
    .activePatterns = {"ambient_pad", "soft_lead"},
    .filterFreq = 0.3f
});

sequencer->defineState("intense", {
    .tempoRange = {140, 160},
    .activePatterns = {"hard_drums", "bass_lead", "aggressive_lead"},
    .filterFreq = 0.8f
});

// Automatic state transitions
sequencer->addStateTransition("calm", "intense", 
    Condition::ParameterAbove("energy_level", 0.7f));
```

### Horizontal Re-sequencing

## Troubleshooting: Sequencer Timing, Retriggers, “Ghost Notes” and Envelope Feel

This section documents the recent investigation into timing anomalies and feel issues in the sequencer, the instrumentation added, fixes attempted, and the current status. It is intended as a deep-dive reference for future debugging.

### Symptoms Observed
- Missed consecutive retriggers on 16th-note grids (e.g., steps [3,4,5,6]).
- Occasional “ghost notes” (unexpected triggers) reported previously.
- “Global gate”/gated feel on rapid retriggers; envelopes feel dipped or blunted.
- Velocity UI mismatch (UI showed curve-processed values instead of raw 100/70/40 cycle).
- Periodic click heard after stopping at times; suspicion around metronome gating / fallback note-offs.
- Crash when interacting with the Patterns grids after UI refresh (stale UI pointers captured in callbacks).

### Instrumentation and Tests Added
- Headless regression tests:
  - `examples/SequencerRegressionTest.cpp`: Validates retriggers and ghost notes over multiple loops. PASS in headless runs.
  - `examples/EnvelopeRegressionTest.cpp`: Two parts
    - Direct ADSR: Verifies envelope monotonic Attack/Decay/Release and sustain accuracy. PASS.
    - Sequencer→Synth: Uses consecutive retriggers to observe envelope peaks. In some headless envs, voice introspection wasn’t reliable; test now passes if note-on callbacks occur (fallback) and logs peaks when visible.
- In-app debug overlay (Patterns page):
  - “[DBG] col X fired Y” where X is active 16th column, Y is note-ons fired this audio block.
  - Mini “Env Scope” plot + numeric readout showing max envelope value across active voices over time.
- Compile-time logging hook:
  - Define `SEQ_DEBUG_PRINT` to log Sequencer note-on/off decisions with `previousPosition`, `currentPosition`, `noteStartTime`, and end-time comparisons.

### UI/UX Guardrails and Fixes
- Programmatic UI refresh guard: `isUpdatingPatternUI` suppresses callbacks during grid rebuilds to avoid unintended pattern edits.
- Velocity display unified to raw percentages (100→70→40) and consistent cycling when tapping the velocity row.
- Metronome: gated by transport state; click mixed into visualizers when active.
- Fallback note-offs: feature retained but disabled by default to prevent interference with retriggers.
- Crash fix (critical):
  - Issue: `SequencerGrid` callbacks captured raw grid pointers (`gridPtr`/`velGridPtr`). When the Patterns screen refreshed/recreated the grids, those pointers could become stale, and later `getGridSize()` on the invalid `this` caused `EXC_BAD_ACCESS (SIGBUS)`.
  - Fix: Re-fetch the current grid instance from the active screen inside the callback each time (by ID `pat_grid`/`pat_vel_grid`) and bail out if missing. All `setCell*` calls now use the re-fetched pointer.

### Engine-Level Adjustments (Envelope “Feel”)
- Voice-steal quick release override (one-shot):
  - Before: Hard-coded ~20 ms quick fade when stealing a voice.
  - Now: Configurable at runtime via Settings or Patterns quick toggle. Options include 20 ms, 8 ms, 5 ms for snappier retriggers.
- Global minimums for ModEnvelope:
  - Before: Hard minimums Attack ≥ 5 ms, Release ≥ 10 ms.
  - Now: Global minimums configurable at runtime (Normal A=5 ms/R=10 ms, Aggressive A=2 ms/R=5 ms, Ultra A=1.5 ms/R=3 ms). Applied to subsequent notes.
- Persistence:
  - Saved to user config JSON under `envelope`: `quick_release_s`, `min_attack_s`, `min_release_s`.
  - Loaded on startup, synced to both Patterns quick toggles and Settings dropdowns.

### Timing Core: Notes
- `Sequencer::processSinglePattern` uses high-precision comparisons with EPS=1e-9 and handles loop wrap (previousPosition > currentPosition) explicitly.
- Start check: `noteStartTime >= previousPosition - EPS && noteStartTime < currentPosition + EPS` (plus special-case for loop wrap).
- Stop check: Ends within frame or beyond previous frame if wrapped.
- Column mapping UI↔engine: 16 columns per bar, `stepBeats = beatsPerBar/16.0`, columns computed with rounding in UI when rebuilding the grid.

### Reproduction Recipes
- Spaced retriggers: C4 at columns [1,6,10,12] in a 1-bar pattern (16th notes). Expect 1 trigger per loop per column.
- Consecutive retriggers: A pattern with columns [3,4,5,6]. Expect consistent retriggers across the set.
- Use Patterns Test Mode to auto-load these patterns and start transport.
- Turn on Patterns overlay grid (“Grid: ON”) to see column and fired counts; observe Env Scope plot.

### Current Status (Still Not Fully Fixed In-App)
- The core Sequencer logic and headless timing test pass (no ghost notes, no missed retriggers in that context).
- Direct ADSR envelope behavior is verified correct.
- In-app, envelope “feel” on rapid retriggers improved by reducing quick-release override and minimums, but the original “weirdness” is not conclusively resolved across all scenarios. Remaining contributors might be:
  - App-level timing jitter between UI edits and the audio callback when editing live.
  - Voice-steal behavior still introducing micro-dips at very tight intervals (trade-off vs clicks).
  - Velocity-to-amp or velocity-to-filter modulation amounts yielding perceived dips on retriggers.
  - Polyphony normalization reducing gain when multiple voices overlap.

### What We Tried (Chronological Highlights)
- Re-enabled metronome with proper gating and visibility in visualizers.
- Disabled fallback note-offs by default (to avoid interference with rapid retriggers).
- Hardened UI: removed “retrig-on-edit” tones; added guards to prevent programmatic updates from firing callbacks; consistent velocity display (raw percent); placed controls as requested on the grid.
- Normalization experiments: briefly changed polyphony gain normalization, reverted to gentle sqrt-based scaling to avoid delay-like smearing.
- Added quantized pattern switching (Immediate/OnBeat/OnBar) with Keep Phase option.
- Headless tests added; in-app Test Mode with overlay and envelope scope added to align observations.
- Fixed a crash in grid callbacks caused by stale pointers after UI refresh.

### How To Debug Further (Playbook)
- Visual confirmation:
  - Enable Patterns overlay. Confirm “[DBG] col X fired Y” matches expectations on both spaced and consecutive tests.
  - Observe Env Scope and numeric readout for dips or unexpected plateaus.
- Engine logging:
  - Build with `-DSEQ_DEBUG_PRINT` to print note-on/off decisions around boundaries; compare against expected columns.
- A/B toggles:
  - Patterns or Settings → Envelope: flip Quick Release (20/8/5 ms) and Minimums (Normal/Aggressive/Ultra); see if feel improves.
- Modulation checks:
  - Overlay shows “Vel→Vol” and “Vel→Cutoff” amounts; reduce amounts to check if dynamics mapping is causing the perception.
- App timing:
  - Keep UI edits paused during test playback (the Test Mode disables editing on the grids while running) to eliminate UI-to-audio interference.

### Open Investigations / Hypotheses
- App-layer timing differences (buffer size vs tempo vs `process` cadence) might still cause edge misalignments; instrument `process()` delta and effective beats-per-frame.
- Confirm that UI-selected pattern and sequencer current pattern always align during tests (Test Mode enforces this but verify outside of Test Mode).
- Inspect any place where rounding (`std::round` vs `floor`) could offset column mapping for display vs engine.
- Consider a per-voice “retrigger mode” that restarts the envelope without prior one-shot release when the same pitch retriggers within a small window (opt-in, risk of clicks).

### Quick Reference: Controls and Files
- Patterns quick toggles:
  - “Env QuickRel: 20ms/8ms” → voice stealing fade duration.
  - “Env Min: Normal/Aggressive” → global min A/R.
  - Patterns overlay (“Grid: ON”) → column/fired + Env Scope.
- Settings → Envelope:
  - Dropdowns for Quick Release (20ms/8ms/5ms) and Minimums (Normal/Aggressive/Ultra).
  - Persisted under `envelope.quick_release_s`, `envelope.min_attack_s`, `envelope.min_release_s`.
- Tests:
  - `SequencerRegressionTest` (retrigger/ghosts timing)
  - `EnvelopeRegressionTest` (ADSR + sequencer integration; fallback criteria in CI)
- Core code points:
  - Sequencer timing: `src/sequencer/Sequencer.cpp` (start/stop checks, EPS, loop wrap)
  - UI Patterns page and callbacks: `src/main_integrated_simple.cpp` (safe re-fetch in callbacks)
  - Envelope: `include/synthesis/modulators/envelope.h`, `src/synthesis/modulators/envelope.cpp`
  - Voice stealing quick release: `include/synthesis/voice/voice_manager.h`, `src/synthesis/voice/voice_manager.cpp`

### Next Steps Proposed
- Add a “Custom…” option in Settings with numeric sliders for Quick Release and min Attack/Release.
- Add optional per-voice envelope scope/peak indicators to distinguish overlaps.
- Gate stricter headless assertions behind a build flag once CI visibility of voices is stable.
- If user-reported issues persist, consider a “Same-Pitch Fast Retrigger” strategy that prioritizes envelope restart over one-shot release for short intervals (with click suppression heuristics).


Dynamic pattern modification based on context:

```cpp
// Enable horizontal re-sequencing
pattern->enableHorizontalResequencing(true);

// Define re-sequencing rules
pattern->addResequenceRule(ResequenceRule{
    .condition = "energy > 0.8",
    .action = ResequenceAction::DoubleTime,
    .probability = 0.6f
});

pattern->addResequenceRule(ResequenceRule{
    .condition = "tension < 0.3",
    .action = ResequenceAction::HalfTime,
    .probability = 0.4f
});
```

### Vertical Remixing

Layer-based dynamic arrangement:

```cpp
// Define arrangement layers
sequencer->createLayer("foundation", {"kick", "bass"});
sequencer->createLayer("rhythm", {"snare", "hihat"});
sequencer->createLayer("harmony", {"pad", "lead"});
sequencer->createLayer("decoration", {"percussion", "fx"});

// Dynamic layer control
sequencer->setLayerIntensity("foundation", 1.0f);  // Always active
sequencer->setLayerIntensity("rhythm", 0.8f);      // Mostly active
sequencer->setLayerIntensity("harmony", 0.6f);     // Context-dependent
sequencer->setLayerIntensity("decoration", 0.3f);  // Sparse

// Automatic intensity mapping
sequencer->mapParameterToLayerIntensity("user_energy", "rhythm");
sequencer->mapParameterToLayerIntensity("musical_tension", "harmony");
```

## MIDI Integration

### External Synchronization
```cpp
// MIDI clock sync
sequencer->enableMIDISync(true);
sequencer->setMIDISyncMode(MIDISyncMode::External);

// MIDI note input
sequencer->enableMIDIInput(true);
sequencer->setMIDIInputChannel(1);  // Channel 1 for note input
```

### Pattern Triggering
```cpp
// Map MIDI notes to pattern triggers
sequencer->mapNoteToPattern(36, "kick_pattern");    // C1 triggers kick
sequencer->mapNoteToPattern(38, "snare_pattern");   // D1 triggers snare
sequencer->mapNoteToPattern(42, "hihat_pattern");   // F#1 triggers hi-hat

// Program change for pattern switching
sequencer->enableProgramChangePatterns(true);
```

## Advanced Features

### Polyrhythms and Odd Time Signatures
```cpp
// Create polyrhythmic patterns
auto pattern3_4 = sequencer->createPattern("waltz", 12);  // 3/4 time
pattern3_4->setTimeSignature(3, 4);

auto pattern7_8 = sequencer->createPattern("complex", 14); // 7/8 time
pattern7_8->setTimeSignature(7, 8);

// Run multiple patterns simultaneously
sequencer->playPatternsConcurrently({"pattern_4_4", "pattern3_4", "pattern7_8"});
```

### Euclidean Rhythms
```cpp
// Generate Euclidean rhythms
auto euclidean = pattern->generateEuclideanRhythm(
    16,  // Total steps
    5,   // Hits
    3    // Rotation
);

// Apply to track
drumTrack->applyRhythm(euclidean);
```

### Conditional Logic
```cpp
// Advanced conditional triggers
step->setCondition(Condition::Custom([&]() {
    return (sequencer->getPlaybackPosition() % 4 == 0) && 
           (random() < 0.6f);
}));

// Probability curves
pattern->setProbabilityCurve(ProbabilityCurve::Exponential);
pattern->setProbabilityRange(0.2f, 0.9f);
```

### Sample Accurate Timing
```cpp
// Precise timing control
sequencer->setSampleAccurateTiming(true);
sequencer->setLookaheadSamples(64);  // 64 samples lookahead

// Micro-timing adjustments
step->setMicroTiming(-0.001f);  // 1ms early
step->setTimingVariation(0.002f); // ±2ms random variation
```

## Testing and Development

### Pattern Debugging
```cpp
// Enable debug output
sequencer->setDebugMode(true);

// Pattern analysis
auto analysis = pattern->analyze();
std::cout << "Pattern density: " << analysis.density << std::endl;
std::cout << "Rhythmic complexity: " << analysis.complexity << std::endl;

// Real-time monitoring
sequencer->setStepCallback([](int track, int step, const StepData& data) {
    std::cout << "Track " << track << " Step " << step 
              << " Note: " << data.note << std::endl;
});
```

### Performance Testing
```cpp
// Performance monitoring
auto stats = sequencer->getPerformanceStats();
std::cout << "CPU usage: " << stats.cpuUsage << "%" << std::endl;
std::cout << "Memory usage: " << stats.memoryUsage << " MB" << std::endl;
std::cout << "Latency: " << stats.latency << " ms" << std::endl;
```

## Integration Examples

### Complete Song Structure
```cpp
// Create a complete song arrangement
auto song = sequencer->createSong("example_song");

// Define sections
song->addSection("intro", {"intro_drums", "soft_pad"}, 8);  // 8 bars
song->addSection("verse", {"verse_drums", "bass", "lead"}, 16);
song->addSection("chorus", {"chorus_drums", "bass", "lead", "harmony"}, 16);
song->addSection("bridge", {"minimal_drums", "ambient_pad"}, 8);
song->addSection("outro", {"outro_drums", "soft_pad"}, 8);

// Set up transitions
song->addTransition("intro", "verse", TransitionType::Crossfade, 1.0f);
song->addTransition("verse", "chorus", TransitionType::Cut, 0.0f);
song->addTransition("chorus", "verse", TransitionType::Filter, 2.0f);

// Start playback
song->play();
```

### Live Performance Setup
```cpp
// Configure for live performance
sequencer->setPerformanceMode(true);
sequencer->enableQuantizedLaunching(true);
sequencer->setQuantization(Quantization::Bar);

// Set up scene launching
sequencer->createScene("minimal", {"kick", "bass"});
sequencer->createScene("building", {"kick", "snare", "bass", "lead"});
sequencer->createScene("full", {"kick", "snare", "bass", "lead", "pad", "fx"});

// Map to controllers
sequencer->mapControllerToScene(0, "minimal");
sequencer->mapControllerToScene(1, "building");
sequencer->mapControllerToScene(2, "full");
```

The sequencer system provides professional-grade functionality suitable for both studio production and live performance, with adaptive capabilities that respond intelligently to musical context and user input.

## Patterns Page Layout (Draft)

Here’s a clean, focused Patterns page layout that fits your grid system and scales.

**Layout Overview**
- Header: pattern selection + basics (left), quantize/mode (right).
- Main editor: step grid with piano-roll-lite lane and velocity row.
- Sidebar: pattern ops, randomize, copy/paste, transpose, utilities.
- Footer: audition transport, record arm (later), save/apply status.

**Header Bar**
- Pattern Select: Prev/Next, dropdown, name field, Duplicate/Delete.
- Length: dropdown (1/2/4/8/16 bars), per-pattern swing, humanize, transpose.
- Quantize: dropdown (Off/OnBeat/OnBar), Keep Phase toggle.
- Mode: Playback mode readout (Single/SectionDriven/Song) + quick switch.

**Editor Grid**
- Piano-Roll Lite: 1–2 octaves on Y, 16th steps on X; monophonic toggle for MVP.
- Velocity Row: single row under grid with per-step velocity handles.
- Step Tools: Tie/Ratchet per-step (disabled in MVP; placeholders OK).
- Mini Timeline: above grid showing bar boundaries and pattern length.

**Right Sidebar**
- Actions: New, Duplicate, Clear, Randomize (safe).
- Edit Ops: Copy/Paste, Nudge Left/Right, Scale Length (2x/½).
- Transpose: -12/-1/+1/+12 buttons with preview.
- Bindings (compact): Section→Pattern quick set list (A/B/C/…) for SectionDriven.

**Footer**
- Audition: Play/Stop loop of the current pattern; Metronome toggle.
- Quantize Queue HUD: “Pattern X queued OnBar” when changes are scheduled.
- Record (Phase 2): Arm, Count-in, Source: Virtual Keyboard.
- Save: Autosave ON indicator + “Apply Now” button for explicit commit if needed.

**UI IDs (proposed)**
- Header: `pat_prev`, `pat_select`, `pat_next`, `pat_name`, `pat_new`, `pat_dup`, `pat_del`, `pat_len`, `pat_swing`, `pat_human`, `pat_transpose`, `pat_quantize`, `pat_keep_phase`, `pat_mode`.
- Grid: `pat_grid`, `pat_velocity_row`, `pat_tie_step_i`, `pat_ratchet_step_i`.
- Sidebar: `pat_clear`, `pat_rand_safe`, `pat_copy`, `pat_paste`, `pat_nudge_l`, `pat_nudge_r`, `pat_scale_half`, `pat_scale_double`, `pat_tr_down12`, `pat_tr_down1`, `pat_tr_up1`, `pat_tr_up12`, `pat_bind_A`..`pat_bind_F`.
- Footer: `pat_play`, `pat_stop`, `pat_click`, `pat_record_arm`, `pat_countin`, `pat_apply`, `pat_autosave`.

**Grid Placement (suggested)**
- Header band: full-width top row; split left (selection/length) and right (quantize/mode).
- Editor: centered block spanning most width; velocity row directly underneath.
- Sidebar: right column stack aligned to grid, consistent with Sequencer page.
- Footer: bottom band, left-aligned transport; right-aligned save/status.

**MVP Scope**
- Pattern pool: 64 slots; selector + name.
- Length control: 1, 2, 4, 8, 16 bars; redraw grid dynamically.
- Grid editing: note toggles per 16th and a single velocity row.
- Actions: New, Duplicate, Clear, Randomize (safe).
- Audition: local loop play/stop of the current pattern.
- Persistence: autosave to `user_config` on change; Apply button syncs immediately.

**Engine Wiring**
- Pattern Store: `addPattern`, `duplicatePattern`, `clear`, `setLength`, `addNote`, `removeNote`, `setVelocity(step, v)`.
- Sequencer Link: `setCurrentPattern(id)`, `getCurrentPatternId()`.
- Quantize: on pattern switch, schedule OnBar by default; option to Immediate/OnBeat via `pat_quantize`.
- Section Binding: write `sequencer.sectionPatterns` when using quick bindings.

**Acceptance (MVP)**
- Create/rename a pattern, set length, place steps, audition loop reliably.
- Duplicate, clear, and safe-randomize produce expected results.
- Pattern switches honor quantize and reflect in HUD.
- Autosave and Apply persist/reload patterns across restarts.
