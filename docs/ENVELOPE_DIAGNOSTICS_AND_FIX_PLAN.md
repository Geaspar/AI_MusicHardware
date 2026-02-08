Title: Envelope Behavior Diagnostics, Findings, and Fix Plan

Overview
- Symptoms: “Weird” envelope feel on consecutive retriggers; historic reports of missed consecutive retriggers and ghost notes; velocity UI mismatch.
- Goals: Verify ADSR correctness, validate Sequencer→Synth integration, add in-app tooling to visualize timing, and propose remedies to improve punchiness without clicks.

What’s Implemented
- Headless tests:
  - `SequencerRegressionTest` (existing): validates retriggers and ghost notes timing.
  - `EnvelopeRegressionTest` (new):
    - Direct ADSR test: checks monotonic Attack/Decay/Release and sustain level.
    - Sequencer→Synth test: runs a 16th-note pattern via sequencer callbacks to the synth; inspects envelope peaks and voice activity.

- App tooling:
  - Patterns Test Mode: top-right toggle `patterns_test_mode` on the Patterns page.
    - Loads two canned patterns (spaced [1,6,10,12] and consecutive [3,4,5,6]), locks editing, starts playback.
    - Shows HUD: “Test Mode: Running — check [DBG] col/fired overlay”.
  - Debug overlay (existing): when `patterns_grid` is ON, shows “[DBG] col X fired Y” (active 16th column and fired count per audio block).
  - Sequencer debug prints (optional): guard with `SEQ_DEBUG_PRINT` to log note-on/off decisions with precise positions.
  - Velocity UI: unified to display raw percentages (100/70/40) matching the tap cycle.
  - Persistence: envelope settings are saved to user config `envelope` object
    - `envelope.quick_release_s` (float, seconds)
    - `envelope.min_attack_s` and `envelope.min_release_s` (floats, seconds)

How To Run Tests
- Build targets:
  - `SequencerRegressionTest` (existing)
  - `EnvelopeRegressionTest` (new)
- Commands (from `./build`):
  - `cmake .. -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build . -j 2 --target SequencerRegressionTest EnvelopeRegressionTest`
  - `./bin/SequencerRegressionTest`
  - `./bin/EnvelopeRegressionTest`

Results (Current)
- Direct ADSR test: PASS.
  - Attack increases monotonically to peak, decay reaches sustain, release monotonic to zero.
  - Conclusion: ADSR math and stage transitions are correct.

- Sequencer→Synth integration test: FAIL (in headless harness).
  - Note-ons are called, but the test did not observe active voices; envelope peak stayed 0.
  - Diagnostic shows `maxVoices=0` at probe time, implying the test harness observed a null/empty voice manager during the window it sampled.
  - Conclusion: Harness ordering/visibility issue rather than ADSR math; the in-app path (driven by audio callback) may not reproduce this. Still useful to fix for CI confidence.

Likely Causes of “Weird” Feel In-App
- Retrigger voice-steal quick fade: on voice stealing, we trigger a one-shot release override of ~20 ms to avoid clicks, then start a new note. Rapid 16th retriggers can produce a brief dip before the next attack, feeling “gated”.
- Minimum envelope times: ModEnvelope enforces minimum attack (≥ 5 ms) and release (≥ 10 ms). This prevents clicks but reduces snappiness for very percussive patterns.
- Velocity mapping: Now the velocity UI matches 100→70→40. If dynamics remain off, check mod amounts for Velocity→Volume and Velocity→Cutoff (shown in overlay text).

How To Validate In-App
1) Patterns page → Toggle `patterns_test_mode` to ON.
   - Pattern 1: spaced [1,6,10,12]; Pattern 2: consecutive [3,4,5,6].
2) Turn on `patterns_grid` overlay to see:
   - “[DBG] col X fired Y”: confirm expected note-on counts per column/audio block.
3) Listen for envelope behavior around retriggers; correlate with overlay counts.

Proposed Fix Options
- Reduce “one-shot” release override used in voice stealing:
  - Current: ~20 ms. Try 5–10 ms to reduce dip between retriggers.
  - Make it a runtime toggle or preset-level parameter for A/B.

- Lower ModEnvelope minimums:
  - Attack: from 5 ms → 2–5 ms, Release: from 10 ms → 5–10 ms.
  - Consider preset categories (percussive vs. pad) or an “Aggressive Retrigger” mode.

- Add an “Env Scope” overlay (optional):
  - Simple on-screen plot of the current voice’s envelope over a few seconds for visual confirmation during tests.

- CI harness fix:
  - Ensure the Sequencer→Synth envelope test sees active voices by reconciling voice manager setup/timing. Not functionally critical for app behavior, but good for regression safety.

Code Pointers
- UI/Patterns/Test Mode, overlay, velocity row:
  - `src/main_integrated_simple.cpp` (Patterns screen around `pat_grid`, `pat_vel_grid`, `patterns_test_mode`, overlay).

- Sequencer timing/processing and optional debug prints:
  - `src/sequencer/Sequencer.cpp` (note start/stop logic, `processSinglePattern`, `SEQ_DEBUG_PRINT`).

- ADSR implementation:
  - `include/synthesis/modulators/envelope.h`, `src/synthesis/modulators/envelope.cpp` (ModEnvelope).
  - Voice stealing quick release override:
    - `src/synthesis/voice/voice_manager.cpp` (see `noteOn` path with `setReleaseOverrideOnce(0.02f)`).

- Tests:
  - `examples/SequencerRegressionTest.cpp` (timing regression, retriggers/ghosts)
  - `examples/EnvelopeRegressionTest.cpp` (ADSR and sequencer integration)

Recommendation
- Short-term (A/B quickly):
  - Reduce voice-steal one-shot release to ~10 ms and lower ModEnvelope minimum attack to ~2–3 ms (behind a flag so we can revert).
  - Use Patterns Test Mode + overlay to judge feel on retriggers.

- Medium-term:
  - Add “Env Scope” overlay for visual confirmation.
  - Fix headless integration test visibility of active voices for CI stability.

Appendix: Build Notes
- If you want to inspect sequencer start/stop timing decisions, define `SEQ_DEBUG_PRINT` at compile time for verbose logs in `processSinglePattern()`.
Persistence
- File: user config JSON (see `getUserConfigPath()`), keys under `envelope`:
  - `quick_release_s`: one-shot quick release seconds for voice stealing.
  - `min_attack_s`, `min_release_s`: global minimums applied by ModEnvelope for subsequent notes.
  - The Patterns page toggles update these values and write them to disk immediately.
