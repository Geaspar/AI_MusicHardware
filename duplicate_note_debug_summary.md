# Duplicate Note Debug Summary

## Problem
Notes seem to play again about a second after being triggered, as if clicked twice or with a delay effect.

## Changes Made for Debugging

### 1. Sequencer Disabled
- Added `sequencer->stop()` after initialization to ensure it's not playing
- Commented out `sequencer->process()` in the audio callback
- This eliminates the sequencer as a potential source of duplicate notes

### 2. Added Timestamps to Note Events
- Added millisecond timestamps to both Note On and Note Off events
- This will help identify if notes are being triggered multiple times and when

### 3. Debug Output Enhanced
The console will now show:
```
[timestamp ms] Keyboard Note On: C4 (note 60) velocity 100 normalized: 0.787402
Audio Engine - Sample Rate: 44100, Buffer Size: 512, Stream Time: 1.234
Master Volume: 0.7
Filter Cutoff: 1.0
Oscillator Type: 0
[timestamp ms] Keyboard Note Off: C4 (note 60)
```

## What to Look For

When testing, watch for:
1. **Multiple Note On events** with the same note number
2. **Time difference** between duplicate notes (if ~1000ms, confirms the delay)
3. **Missing Note Off events** that might cause notes to hang
4. **Any patterns** in the timestamps

## Possible Causes to Investigate

1. **Voice Stealing**: Check if voice management is retriggering notes
2. **MIDI Echo**: External MIDI device might be echoing notes
3. **UI Double-Click**: Mouse events might be triggering twice
4. **Envelope Retrigger**: Envelope might be retriggering after release

## Next Steps

1. Run the program and trigger a single note
2. Check console output for duplicate Note On events
3. Note the timestamps to confirm timing
4. Based on findings, investigate the specific subsystem causing the issue

## To Re-enable Sequencer

Once the issue is found, uncomment line 253 in main_integrated_simple.cpp:
```cpp
sequencer->process(static_cast<float>(numFrames) / audioEngine->getSampleRate());
```