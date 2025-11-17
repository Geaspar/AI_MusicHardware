# Creating a Synthesizer Step Sequencer in C++

This document summarizes practical design patterns, data structures, timing strategies, and integration points for building a synthesizer step sequencer in C++. It references open-source C++ code where useful and relevant, and points to local code in this repository for concrete examples.

- Audience: C++ audio developers building a real‑time note sequencer for a synth.
- Scope: Note/event step sequencer with pattern editing, probability, swing, song/arrangement mode, MIDI out, and engine synchronization.


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




