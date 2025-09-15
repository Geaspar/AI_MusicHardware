# Creating a Synthesizer Step Sequencer in C++

This document summarizes practical design patterns, data structures, timing strategies, and integration points for building a synthesizer step sequencer in C++. It references open-source C++ code where useful and relevant, and points to local code in this repository for concrete examples.

- Audience: C++ audio developers building a real‑time note sequencer for a synth.
- Scope: Note/event step sequencer with pattern editing, probability, swing, song/arrangement mode, MIDI out, and engine synchronization.

Local open-source mirrors: see `References/OpenSource/` for browsable checkouts of Helm, Surge XT, VCV Fundamental, and Tracktion Engine, and `References/OpenSource/README.md` for licensing notes.


## 1) What “Sequencer” Means Here

Sequencers come in a few flavors:
- Note step sequencer: schedules note on/off events on a grid (primary focus).
- Modulation step sequencer: per‑step values modulate parameters (Vital’s LFO grid is a good reference for UI/UX of a grid, even though it isn’t a note sequencer).
- Arpeggiators: pattern generators reacting to held notes; not covered in depth here but many patterns overlap (clocking, probability, swing, humanize).

Answering your question about Vital: the local Vital code folder does not include a note sequencer implementation; it does include a grid‑based LFO editor that can inspire step‑style UI/UX for modulation lanes.


## 2) Minimal Architecture

Key components you will typically need:
- Data model: event or grid representation of notes; patterns; an arrangement layer (song mode); transport state.
- Timing: tempo, beats per bar, beat time, sample/host synchronization; precise half‑open event windows; loop boundary behavior.
- Engine bridge: noteOn/noteOff callbacks into the synth voice manager; thread-safety for UI vs audio.
- I/O: MIDI out (e.g., RtMidi) and optional MIDI file export.
- UI: grid editing with velocity/gate/probability per step; live transport feedback.

The local implementation in `include/sequencer/Sequencer.h` and `src/sequencer/Sequencer.cpp` is a concrete, working reference that covers the above. See also `docs/SEQUENCER_TECHNICAL_SPEC.md` and `docs/SEQUENCER_GUIDE.md` for additional local context.


## 3) Data Model Patterns

Two common representations:
- Event list: vector of Note {pitch, velocity, startBeat, duration, channel, probability}.
- Fixed grid: rows=pitches, columns=steps; each cell stores on/off and per‑cell attributes (velocity, gate, chance, ratchets, micro‑offset).

Local reference (event‑list pattern):

```cpp
// include/sequencer/Sequencer.h (excerpt)
struct Envelope { float attack, decay, sustain, release; };
struct Note {
  int pitch; float velocity; double startTime; double duration; int channel;
  Envelope env; float chance; // 0..1 per-note probability
};

class Pattern {
 public:
  // addNote/removeNote/quantize/applySwing + length in beats
};

class Sequencer {
 public:
  using NoteOnCallback = std::function<void(int, float, int, const Envelope&)>;
  using NoteOffCallback = std::function<void(int, int)>;
  // Pattern pool, playback mode (pattern vs song), transport, etc.
};
```

Notes:
- Event lists are compact and flexible for arbitrary rhythms and durations.
- Grid UIs can still write into event lists (see `Sequencer::setStep()` and helpers).
- Keep pattern length in beats; quantize/swing transform note start/duration.


## 4) Timing and Scheduling

Goals:
- Sample‑accurate scheduling without double triggers or missed events.
- Stable tempo/beat time with limited drift.
- Clean loop boundaries that don’t leave “stuck” notes.

Useful tactics (local reference implementation):
- Use half‑open scheduling windows: [prev − eps, current).
- Treat loop wraps as the union of [prev − eps, end) ∪ [0, current).
- Explicitly stop all active notes at loop points before jumping position.
- Accumulate and correct timing error; keep tempo as atomic and beat time cached.

Local code excerpts (logic points, simplified):

```cpp
// src/sequencer/Sequencer.cpp (excerpt)
// Half-open window to prevent double triggers at boundaries
bool noteStartsInFrame = (globalStart >= previousPosition - EPSILON) &&
                         (globalStart < currentPosition);
if (previousPosition > currentPosition) { // looped this frame
  noteStartsInFrame = (globalStart >= previousPosition - EPSILON) ||
                      (globalStart < currentPosition);
}

// Clean loop: stop any active notes before resetting position
if (looping && currentPosition >= songLength - EPS) {
  stopAllActiveNotes();
  positionInBeats_ = fmod(currentPosition, songLength);
}
```

Audio engine synchronization (local bridge): the sequencer stores a beat‑time cache and an audio time offset so it can answer “where am I in beats?” in a way that is coherent with the audio callback. See `AudioEngine::synchronizeSequencer()` and `Sequencer::synchronizeWithAudioEngine()`.


## 5) Probability, Swing, and Humanize

- Per‑note probability: useful for variations.
- Swing: delay off‑beats by a proportion of the grid step.
- Humanize: subtle random micro‑timing and velocity variations (add as needed).

Local references:
- Probability gate in `Sequencer::process()` checks `Note::chance` (0..1) before triggering.
- `Pattern::applySwing()` offsets odd grid positions by a fraction of step size.

```cpp
// src/sequencer/Sequencer.cpp (excerpt)
float prob = std::clamp(note->chance, 0.0f, 1.0f);
if (prob < 1.0f) {
  if (((double)rand() / (double)RAND_MAX) > prob) continue; // skip trigger
}
```


## 6) Avoiding “double triggers” at loop boundaries

Even with half‑open windows, edge cases happen with floating‑point drift. Add a small epsilon and one of:
- Per‑loop dedupe by (column,pitch,channel) keys.
- Clear active notes and dedupe set at loop start.

Local pattern:

```cpp
// include/sequencer/Sequencer.h (excerpt)
static inline uint64_t makeDedupeKey(int col16, int pitch, int channel) { /*...*/ }
std::unordered_set<uint64_t> firedThisLoop_;
std::atomic<bool> perLoopDedupeEnabled_{false};
```


## 7) Transport and Thread Safety

- Keep transport state atomic where possible (tempo, playing, looping, current pattern index).
- Guard shared structures (patterns, active notes, position) with dedicated mutexes.
- Never call noteOn/noteOff while holding locks; collect work and fire callbacks after unlocking.

Local patterns:
- `tempo_`, `isPlaying_`, `looping_`, `currentPatternIndex_` are atomics.
- Separate mutexes: `patternMutex_`, `arrangementMutex_`, `positionMutex_`, `activeNotesMutex_`, `syncMutex_`.


## 8) MIDI Output (Open‑Source Reference: RtMidi)

Use [RtMidi] (MIT‑style license) for portable MIDI I/O. The repository includes it under `rtmidi/`.

Small, representative excerpt from `rtmidi/tests/sysextest.cpp` showing basic open and send usage:

```cpp
// rtmidi/tests/sysextest.cpp (excerpt)
RtMidiOut* midiout = new RtMidiOut(api);
// ... choose/open a port ...
std::vector<unsigned char> message;
message.push_back(0x90);  // Note On, channel 1
message.push_back(60);    // Middle C
message.push_back(100);   // Velocity
midiout->sendMessage(&message);
```

Tips:
- Time stamping and scheduling are on you; RtMidi sends immediately.
- For input, set a callback with `RtMidiIn::setCallback(...)` and parse messages.


## 9) UI: Grid Editing (Open‑Source Reference: Vital’s LFO Grid)

While not a note sequencer, Vital’s LFO editor demonstrates a robust grid UI (GPLv3). The local Vital tree has an `LfoEditor` section that configures grid X/Y density controls:

```cpp
// SynthExample-Vital/src/interface/editor_sections/lfo_section.cpp (excerpt)
static constexpr int kDefaultGridSizeX = 8;
static constexpr int kDefaultGridSizeY = 1;

grid_size_x_ = std::make_unique<SynthSlider>("grid_size_x");
grid_size_x_->setRange(1.0, LfoEditor::kMaxGridSizeX, 1.0);
grid_size_x_->setValue(kDefaultGridSizeX);
addSlider(grid_size_x_.get());

grid_size_y_ = std::make_unique<SynthSlider>("grid_size_y");
grid_size_y_->setRange(1.0, LfoEditor::kMaxGridSizeY, 1.0);
grid_size_y_->setValue(kDefaultGridSizeY);
addSlider(grid_size_y_.get());
```

For a note sequencer UI, use a grid like `include/ui/UIComponents.h::SequencerGrid` to toggle cells, render a playhead, and reflect per‑cell intensity (velocity) or text (ratchets, chance).


## 10) Arrangement (Song Mode)

Beyond looping a single pattern, many sequencers support a song mode: place patterns on a timeline.

Local pattern:
- `struct PatternInstance { size_t patternIndex; double startBeat; double endBeat; }`
- Sorted `songArrangement_`, with `processSongArrangement()` that resolves active instances per time slice and triggers notes in timeline space.

Key behaviors:
- On each callback, compute previous/current position in beats and iterate active pattern instances only.
- Transform pattern‑local times to global timeline times before scheduling.


## 11) Engine Integration

Bridge the sequencer to the synth’s voice manager via callbacks, and synchronize clocks to ensure sample‑accurate timing.

- Register callbacks: `setNoteCallbacks(noteOn, noteOff)`.
- Call `sequencer.process(deltaBeats)` each audio callback (or at fixed sub‑steps) where `deltaBeats = numFrames / sampleRate * bpm / 60`.
- Periodically call `synchronizeWithAudioEngine(audioTimeSeconds, sampleRate)` to align beat‑time with the engine.

Minimal loop example:

```cpp
// Pseudocode: audio callback
void audioCallback(float* out, int numFrames, double sampleRate) {
  const double bpm = sequencer.getTempo();
  const double deltaBeats = (numFrames / sampleRate) * (bpm / 60.0);
  sequencer.process(deltaBeats);
  // render synth voices ...
}
```


## 12) MIDI File Export (Optional)

Exporting patterns to SMF is handy for offline inspection. See the local utility in `include/sequencer/MidiFile.h` and `src/sequencer/MidiFile.cpp` (simple Format 1 writer, PPQN=480). This is orthogonal to real‑time playback but shares the same event ordering and note‑off hygiene.


## 13) Common Pitfalls and Fixes

- Missed/double triggers at boundaries: use half‑open windows and an epsilon; consider per‑loop dedupe.
- Stuck notes on loop: ensure all active notes are turned off before resetting position.
- Floating‑point drift: accumulate and correct; avoid repeated conversions in tight loops.
- UI/editor races: lock pattern data during edits; avoid holding locks while calling into the audio or synth.
- Tempo changes mid‑playback: cache beat time, update atomically, and recompute offsets on sync.


## 14) Additional Open‑Source References (no code excerpts here)

These are well‑known C++ projects with sequencer/step concepts worth studying:
- Helm (GPLv3) by Matt Tytel: modular synth; inspect modulation + timing patterns.
- Surge XT (GPLv3): modulators and timing utilities; rich scheduling patterns.
- VCV Rack Fundamental SEQ‑3 (Apache/BSD‑style in many modules): classic 3‑row step sequencer logic in C++.
- Tracktion Engine (GPL/commercial dual‑license): arrangement/clip scheduling at DAW scale.

Please verify licenses and version history when you pull code; keep GPL compatibility in mind if you plan to redistribute binaries.


## 15) Suggested Next Steps

- Lock down whether you want a fixed grid or free event list as the internal model (grid can still write to events).
- Implement minimal MVP using the local `Sequencer` as reference: pattern add/toggle, probability, swing, loop, transport.
- Wire MIDI out via RtMidi for external synths.
- Add grid UI with velocity/gate, then extend with ratchets and micro‑timing.
- Add song/arrangement mode and persistence.


## 16) License Notes for Referenced Code

- RtMidi is MIT‑style; attribution recommended.
- Vital code here is GPLv3; any code you copy into a redistributable program would make your program GPLv3 unless isolated appropriately.
- Projects like Helm and Surge XT are GPLv3; VCV Rack modules vary (check per‑module license).

If you only borrow ideas and write your own code, you can keep your original license; if you copy code, follow the original project’s license obligations.


## 17) Quick Answers

- Does the local Vital code contain a note sequencer? No; it has a grid‑based LFO editor (modulation step editor), not a note/event sequencer.
- Where to see a working note sequencer? See this repo’s `include/sequencer/` + `src/sequencer/` for a complete implementation, and RtMidi tests for MIDI I/O usage.


---
Open‑source excerpts used in this document:
- RtMidi (LICENSE: MIT‑style) — `rtmidi/tests/sysextest.cpp` usage snippet.
- Vital (LICENSE: GPLv3) — `SynthExample-Vital/src/interface/editor_sections/lfo_section.cpp` grid snippet.


## 18) Reference Implementation: Classes and Responsibilities

Below is a robust, production‑friendly class layout you can adopt. It mirrors patterns we saw in the local Sequencer implementation and in the referenced open‑source projects (Helm’s step sequencer UI and tempo controls, VCV Fundamental’s SEQ‑3 clocking and step advance, and Tracktion/ JUCE transport semantics).

### 18.1 Clocking and Transport

```cpp
// Provides a stable musical clock from either internal tempo or external pulses.
class ClockSource {
public:
  enum class Mode { Internal, External };
  void setMode(Mode m);
  void setTempo(double bpm);              // internal
  void setSampleRate(double sr);
  void setPPQ(double ppq);                // pulses-per-quarter for external
  void processAudio(double numSamples);   // advance internal phase
  void onExternalClockEdge(double volts); // edge-detect external clock
  double getBeatTimeSeconds() const;      // seconds per beat
  double popBeatIncrements();             // how many beats elapsed since last read
private:
  // Edge detection + timing (cf. VCV Fundamental::SchmittTrigger + Timer)
};

// High-level musical transport, bar/beat math, looping, and position jumps.
class Transport {
public:
  void start(); void stop(); void pause();
  bool isPlaying() const;
  void setLoop(bool loop); bool isLooping() const;
  void setTempo(double bpm); double getTempo() const;
  void setBeatsPerBar(int bpb); int getBeatsPerBar() const;
  void setPositionBeats(double pos); double getPositionBeats() const;
  void jumpToBar(int bar, Sequencer::Timing when); // use beat/bar boundaries
  // Host/audio sync bridge
  void synchronize(double engineTimeSec, double sampleRate);
};
```

Notes and references:
- VCV Fundamental SEQ‑3 uses `dsp::SchmittTrigger`, `dsp::Timer`, and internal phase to support both external and internal clocks. Mirror that approach in `ClockSource` to debounce and detect rising edges reliably.
- Tracktion/JUCE transport concepts (e.g., `AudioTransportSource::start/stop/setPosition/isLooping`) map nicely onto `Transport`’s responsibilities and are similar to what we already expose in our `Sequencer`.

### 18.2 Core Data Structures

```cpp
struct Envelope { float attack, decay, sustain, release; };

struct Note {
  int pitch; float velocity; int channel;
  double startBeat; double durationBeats;
  float chance; Envelope env;
  // Optional microtiming offset, ratchets, etc.
  float microOffset = 0.f; int ratchets = 1;
};

// Free-form event list pattern (already implemented locally).
class Pattern {
public:
  void addNote(const Note& n); void removeNote(size_t i); void clear();
  size_t getNumNotes() const; Note* getNote(size_t i);
  void setLengthBeats(double len); double getLengthBeats() const;
  void quantize(double gridBeats); void applySwing(double swing, double gridBeats);
};

// Optional fixed grid wrapper for UI-friendly editing, writing into Pattern.
class StepGrid {
public:
  StepGrid(int rows, int cols); // rows=pitches (or lanes), cols=16ths
  void toggle(int row, int col); void set(int row, int col, bool on);
  void setVelocity(int row, int col, float vel);
  void setGate(int row, int col, float gateLen);
  void setChance(int row, int col, float chance);
  void toPattern(Pattern& out, int basePitch, int beatsPerBar) const; // export
};
```

Notes and references:
- The local `Pattern`/`Note` match this shape. Use `StepGrid` as a thin UI model that emits events into `Pattern` (similar to Helm’s `GraphicalStepSequencer` + `StepSequencerSection` pairing for grid and controls).

### 18.3 Tracks, Lanes, Humanize, Groove

```cpp
// Lane processors apply transformations during scheduling.
struct GrooveTemplate { std::array<float, 16> offsets16ths; }; // +/- fraction of step

class ProbabilityLane { public: float scale = 1.f; /* apply per-note chance */ };
class VelocityLane    { public: float scale = 1.f; /* multiply velocities  */ };
class GateLane        { public: float scale = 1.f; /* gate length scale    */ };
class RatchetLane     { public: int   max   = 1;   /* per-step subdivisions*/ };
class Humanizer {
public:
  float timeJitterMs=0.f, velJitter=0.f;
  void apply(Note& n, double beatTimeSec) const;
};

class Track {
public:
  void setChannel(int ch);
  void setSwing(float s); void setGroove(const GrooveTemplate& g);
  void setLanes(ProbabilityLane, VelocityLane, GateLane, RatchetLane);
  void setHumanizer(const Humanizer& h);
  void setActivePattern(size_t idx); Pattern* getPattern(size_t idx);
  // Pattern library per track, mute/solo, etc.
};
```

Notes and references:
- Helm exposes tempo‑sync and step count in its `StepSequencerSection` alongside “retrigger” and “smoothing” parameters; mirror those as lane‑like per‑track controls.
- Groove/swing mirrors our local `Pattern::applySwing()` and can be extended to a 16‑step template.

### 18.4 Arrangement (Song Mode)

```cpp
struct PatternInstance { size_t patternIndex; double startBeat; double endBeat; };

class Arrangement {
public:
  void add(size_t patIdx, double startBeat, double lengthBeats);
  void remove(size_t instanceIdx);
  void clear();
  std::vector<PatternInstance> getActive(double prevBeat, double curBeat) const;
};
```

Notes and references:
- This mirrors our local `songArrangement_` approach and is conceptually similar to Tracktion’s clip/timeline scheduling at a DAW scale.

### 18.5 Scheduling Engine

```cpp
// Converts Transport deltas into scheduled note on/off callbacks.
class SequencerEngine {
public:
  using NoteOn  = std::function<void(int pitch, float vel, int ch, const Envelope&)>;
  using NoteOff = std::function<void(int pitch, int ch)>;

  void setTransport(Transport* t); void setClock(ClockSource* c);
  void setCallbacks(NoteOn on, NoteOff off);
  void setArrangement(Arrangement* arr); void setTrack(Track* track);

  // Call from audio thread with elapsed beats.
  void process(double deltaBeats);

  // Resequencing helpers (on-beat/on-bar jumps)
  void defineSections(const std::vector<std::pair<std::string,double>>&);
  void jumpToSection(const std::string& name, Sequencer::Timing when);

private:
  // Half-open window scheduling with loop wrap handling
  void schedulePattern(const Pattern& p, double patStartGlobal,
                       double prevPos, double curPos);
  // Active notes, dedupe set, and precise stop logic
};
```

Implementation notes:
- Use a half‑open window `[prev − eps, cur)` and at loop wraps take the union `[prev − eps, end) ∪ [0, cur)` (VCV’s SEQ‑3 shows clean clock edge gating; we apply similar rigor to event windows).
- Maintain `activeNotes_` and stop them before loop‑backs to avoid stuck notes (our local code already does this robustly).
- Add optional per‑loop dedupe keyed by `(column16, pitch, channel)` to avoid boundary retriggers.

### 18.6 MIDI I/O and Host Sync

```cpp
class MidiOut {
public:
  bool open(int portIndex); void close();
  void noteOn(int ch, int pitch, int vel); void noteOff(int ch, int pitch);
  // Wraps RtMidiOut usage
};

class HostSync {
public:
  // If hosted, query host tempo/ppq position and feed Transport/ClockSource
  void updateFromHost(double hostBpm, double ppqPos, double sampleRate);
};
```

Notes and references:
- See `rtmidi/tests/sysextest.cpp` for basic send logic; remember RtMidi sends immediately, so all timing must be handled by `SequencerEngine`.
- If embedding in a DAW, follow host callbacks to drive `Transport` and `ClockSource` instead of a free‑running internal clock.

### 18.7 UI and Editing

```cpp
// Bridges UI widgets (grid, sliders) to Pattern/Track data safely.
class SequencerEditor {
public:
  void bindGrid(StepGrid* grid); void bindPattern(Pattern* pattern);
  void setCallbacks(std::function<void(int row,int col,bool)> onToggle,
                    std::function<void(int col,float)> onVelocity,
                    std::function<void(int col,float)> onChance);
};
```

Notes and references:
- Helm’s `GraphicalStepSequencer` + `StepSequencerSection` demonstrate a clean pairing of grid UI with tempo sync controls and per‑step sliders. Our local `UIComponents.h::SequencerGrid` fills this role on embedded displays.

### 18.8 Persistence and Export

```cpp
class ProjectSerializer {
public:
  bool saveJson(const std::string& path);
  bool loadJson(const std::string& path);
};

class MidiFileExporter { /* see include/sequencer/MidiFile.h */ };
```

Notes and references:
- Our local `MidiFile` implementation writes SMF Format 1 with a tempo track and per‑pattern tracks. Mirror that if you extract sequences to a DAW.

### 18.9 Concurrency Model

- Atomics: tempo, play/loop flags, current pattern index.
- Mutexes: separate locks for position, pattern data, arrangement, active notes, and sync state.
- Never call noteOn/noteOff while holding locks; accumulate and fire callbacks after releasing locks (our local implementation follows this strictly).

### 18.10 Putting It Together (call order)

1) Audio callback computes `deltaBeats` from frames and BPM.
2) `SequencerEngine::process(deltaBeats)` advances transport and schedules notes.
3) Engine renders voices, noteOn/noteOff come via callbacks.
4) UI edits call editor helpers which lock only pattern data, never the audio thread.

This mirrors our local `Sequencer::process()` flow and is consistent with the VCV SEQ‑3 style of deterministic clocking and Tracktion’s transport semantics.


## 19) Comparative Analysis: Local Sequencer vs. Referenced Projects

This section analyzes how our current implementation aligns with, and differs from, the referenced open‑source projects (VCV Fundamental SEQ‑3, Helm, Tracktion Engine, and RtMidi usage patterns). It highlights strengths, gaps, and specific enhancements to consider.

### 19.1 Data Model and Editing
- Local: Event‑list Pattern with `Note{ pitch, velocity, startBeat, durationBeats, channel, Envelope, chance }`; 16th‑grid helpers (`setStep`, `toggleStep`, `setColumnVelocity`, `setColumnChance`). UI has `SequencerGrid` for grid editing.
- Helm: Graphical step sequencer UI (modulation stepper), robust per‑step sliders and tempo sync UI, but not a note event engine.
- VCV SEQ‑3: Step/gate CV sequencer with per‑step gate states, variable steps, clocked behavior; not a note list but analogous to a fixed grid.
Alignment: Strong — our event list is flexible and our grid helpers make it easy to build step UIs (mirroring Helm’s UI). Consider adding per‑step attributes (ratchets, accent, micro‑offset) to close the “expressive grid” gap.

### 19.2 Timing, Clocking, and Transport
- Local: Beat‑based scheduler with half‑open windows, explicit loop handling, per‑loop dedupe (optional), and audio engine synchronization via `synchronizeWithAudioEngine()`. Internal tempo; no external MIDI clock.
- VCV SEQ‑3: Clean internal clock or external clock edge detection (`SchmittTrigger` + `Timer`), step advance tied to edges, optional clock pass‑through.
- Tracktion/JUCE: Rich host/transport semantics with `AudioTransportSource::start/stop/setPosition/isLooping` and host sync constructs.
Alignment: Good internal clock and transport semantics; robust loop boundary hygiene. Gaps: no external clock/MIDI clock input; no host tempo/ppq integration (beyond generic audio engine time offset). Action: add `ClockSource` with external edge processing, and optional MIDI clock/MTC follower; add a `HostSync` bridge for plugin hosts.

### 19.3 Scheduling Correctness and Loop Boundaries
- Local: Uses `[prev − eps, cur)` windows; on wrap, unions ranges and stops all active notes before resetting; optional per‑loop dedupe keyed by `(col16, pitch, channel)`. Regression tests exist (e.g., `examples/SequencerLoopBoundaryRegressionTest.cpp`, `SequencerTimingDetailedTest.cpp`).
- VCV SEQ‑3: Edges define step transitions and gates, naturally preventing double clocks when debounced.
Alignment: Strong — our approach is robust and well‑tested. Suggestion: enable per‑loop dedupe by default when using strict 16th‑grid playback, exposed as a user toggle (currently default is disabled to avoid edge retriggers).

### 19.4 Musical Features (Swing, Probability, Humanize)
- Local: Swing at pattern level; per‑note probability (`chance`) gating; no explicit humanize/velocity randomization lanes.
- Helm: UI patterns for tempo sync, retriggering, smoothing; modulation lanes conceptually similar to velocity/gate probability lanes.
Alignment: Good baseline. Gaps: humanize (time/velocity jitter), groove templates, per‑step ratchets/accents. Action: add simple `Humanizer` and `GrooveTemplate` plus per‑step ratchet count.

### 19.5 Tracks, Multi‑channel, and Arrangement
- Local: Single sequencer with a pool of patterns, playback mode `SinglePattern` or `Song`. Per‑note channel supports basic multi‑channel output; no explicit Track abstraction. Song arrangement with `PatternInstance` and sorted timeline; resequencing sections (A/B/C) with on‑beat/bar jumps.
- Tracktion: Formal track/clip/timeline model; section navigation and arrangement are large in scope.
Alignment: Adequate for embedded/semi‑DAW use. Gaps: track abstraction (mute/solo/lanes), multi‑pattern per‑track management, and richer section metadata. Action: optional `Track` and `Arrangement` layers as outlined in Section 18.

### 19.6 MIDI I/O, Export, and Host Integration
- Local: `setNoteCallbacks` feeds synth/voice manager; `MidiFile` exporter writes SMF Format 1 with tempo and track names. No direct MIDI clock in, and MIDI out is via callbacks (RtMidi usage outside sequencer scope).
- RtMidi: Immediate send semantics; the app must schedule precisely.
Alignment: Fine. Action: add a minimal `MidiOut` wrapper for direct external synth playback in examples; add optional MIDI clock input to drive `ClockSource`.

### 19.7 Concurrency and Thread Safety
- Local: Clear separation of concerns — atomics for high‑frequency flags (tempo, play, loop, active pattern), dedicated mutexes for position, patterns, arrangement, active notes, and sync; noteOn/noteOff never fired while holding locks. This matches best practice seen across pro audio codebases.
Alignment: Strong.
Minor improvement: use a lightweight, reentrant RNG per sequencer for probability gating (replace `rand()` with a small, fast PRNG that is thread‑local and deterministic if desired).

### 19.8 Performance Considerations
- Local: Schedules via per‑call temporary vectors (`notesToStart`, `notesToStop`); acceptable for typical step rates. Active notes stored in a vector; linear scans are small. Beat math uses doubles with epsilons; good balance of precision and performance.
Potential enhancements:
- Reserve vectors to reduce occasional reallocations under heavy loads.
- Optional small‑buffer optimization for `notesToStart`/`notesToStop` or reuse per‑audio block storage.

### 19.9 UI and UX
- Local: `SequencerGrid` for embedded UI with playhead and per‑cell intensity; ties into `UIContext`. No full desktop editor here, but patterns mirror Helm’s approach.
- Helm: Excellent model for step editor ergonomics and tempo‑sync controls; good to mirror for desktop builds.
Alignment: Solid for embedded; if targeting a desktop UI, adopt Helm‑style controls (grid size, retrigger, smoothing, per‑step sliders).

### 19.10 Testing and Diagnostics
- Local: Rich set of examples/tests around timing, loop boundaries, and ghost‑note issues (`examples/Sequencer*Test.cpp`, plus `VoiceManagerGhostNoteTest.cpp`). This is a strength versus many open‑source references where explicit loop‑boundary tests are scarce.
Alignment: Strong.

### 19.11 Summary of Gaps and Actionable Enhancements
- External/host clocking: add `ClockSource` (internal/external) + MIDI clock/MTC follower; add `HostSync` for plugin hosts.
- Expressive lanes: groove templates, humanize, per‑step ratchets/accents/micro‑offset.
- Tracks: optional `Track` abstraction with per‑track pattern pools, mute/solo, channel mapping.
- Probability RNG: replace `rand()` with a lightweight PRNG (e.g., xoshiro/xorshift) with per‑sequencer seed.
- MIDI out convenience: small `MidiOut` wrapper for examples; document latency expectations.
- Minor perf: reserve vectors, reuse buffers.

These changes keep the current architecture intact while matching or exceeding behaviors found in VCV SEQ‑3 (clocking), Helm (UI), and Tracktion (transport/arrangement paradigms) for a robust, production‑grade sequencer.


## 20) Targeted Fixes for Our Current Sequencer

This section captures concrete improvements discovered during analysis which directly address typical sequencer issues (boundary doubles, drift, clocking, edge cases). Each item includes the rationale and implementation notes mapped to our code.

### 20.1 Clear Per‑Loop Dedupe Set on Loop and Jumps
- Problem: `firedThisLoop_` can suppress legitimate retriggers after a loop or fail to suppress boundary doubles if not cleared precisely.
- Fix: clear `firedThisLoop_` whenever a loop occurs and after any position/section jump.
- Where:
  - In both single‑pattern and song processing, when `loopedThisCall_ = true`, call `firedThisLoop_.clear()`.
  - In `setPositionInBeats(...)`, clear after updating position and stopping active notes.
  - After servicing a pending section jump (OnBeat/OnBar), clear the dedupe set as well.

### 20.2 Robust Dedupe Key for Multi‑Bar Patterns
- Problem: Current key uses a 0..15 column assumption (1 bar of 16ths). On multi‑bar patterns, distinct steps alias, causing over‑suppression.
- Fix: compute dynamic columns from pattern length, or base the key on a fixed tick resolution.
- Example (dynamic 16th columns):
```
double stepBeats = (double)beatsPerBar_ / 16.0;
int cols = std::max(1, (int) llround(currentPattern->getLength() / stepBeats));
int col = ((int) llround(fmod(noteStartTime, currentPattern->getLength()) / stepBeats)) % cols;
uint64_t key = makeDedupeKey(col, note->pitch, note->channel);
```
- Alternative: use a stable PPQN tick index (e.g., 960 PPQN) derived from `noteStartTime` for the dedupe key.

### 20.3 Probability RNG and Modes
- Problem: `rand()` in the audio path is low quality and non‑deterministic across runs.
- Fix: replace with a small, fast PRNG (e.g., xoshiro/PCG) seeded per sequencer; store state as a member.
- Enhancement: add a “probability mode” switch:
  - Per‑hit random (current behavior), or
  - Per‑loop stable: draw once per step per loop to avoid “shifting” patterns across loops.

### 20.4 External/Host Clocking (Follow vs. Free‑run)
- Need: Slave to external clock pulses or host tempo/PPQ.
- Add `ClockSource` with Schmitt‑trigger‑style edge detection (as in VCV SEQ‑3: `dsp::SchmittTrigger` + `Timer`).
- Add `HostSync` bridge to update tempo/position from host callbacks, mapping PPQ to beats and feeding `Transport`.

### 20.5 Epsilon Tuning for Timing Comparisons
- Problem: Fixed `EPSILON = 1e-6` beats can be too small at high tempos or large buffers.
- Fix: tie epsilon to beat time or a minimum time quantum (e.g., 0.25 ms):
```
double beatTimeSec = getPreciseBeatTime();
double epsBeats = std::max(1e-6, 0.00025 / beatTimeSec); // ~0.25ms in beats
```

### 20.6 Long‑Note Handling at Loop Boundaries
- Current: clamp long note ends to song end on loop (safe but chops gates).
- Enhancement: optional “wrap long notes across loop” mode that splits across boundary (tail before loop, head after).

### 20.7 Performance Polishing
- Reserve `notesToStart`/`notesToStop` to reduce allocations; optionally reuse per‑block scratch buffers.
- Keep `activeNotes_` small; consider erase‑remove with small‑vector optimization only if profiling shows issues.

### 20.8 Editing Safety
- If rapid UI edits ever cause ABA‑style races, add a simple pattern version counter; reject schedule reads that span a version change.

### 20.9 Implementation Priority
- High‑impact quick fixes (low risk):
  - 20.1 Clear dedupe on loop/jumps.
  - 20.2 Robust dedupe key (dynamic columns or tick index).
  - 20.3 Replace `rand()` with PRNG; optional per‑loop stable probability mode.
- Medium scope:
  - 20.4 External clock + Host sync; 20.5 epsilon tuning.
  - 20.6 Long‑note wrap mode; 20.7 perf polish; 20.8 edit versioning.

### 20.10 Verification
- Loop boundary tests: reuse `examples/SequencerLoopBoundaryRegressionTest.cpp` and `examples/SequencerTimingDetailedTest.cpp` to ensure no regressions after dedupe/key fixes.
- Probability determinism: run a fixed‑seed harness to confirm stable patterns in “per‑loop” mode.
- External clock: inject synthetic clock edges (like VCV’s Timer + SchmittTrigger) to validate debouncing and step advance.
