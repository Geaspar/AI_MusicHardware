# LFO Rate Correction Summary

## Problem
LFO rate was running slower than expected. For example, when set to 1 Hz, it was actually running at approximately 1/64 Hz.

## Root Cause
The LFO phase increment calculation assumed the `update()` method was called once per sample, but it's actually called once per audio block (every 64 samples).

```cpp
// Original calculation:
float phaseIncrement = frequency_ / sampleRate_;
```

This would advance the phase by 1/44100 for a 1 Hz LFO, but since it's only called every 64 samples, the actual rate was 64 times slower.

## Solution
Multiply the phase increment by the number of samples per update (64):

```cpp
// Fixed calculation:
const float samplesPerUpdate = 64.0f;
float phaseIncrement = (frequency_ * samplesPerUpdate) / sampleRate_;
```

## Technical Details

### Audio Processing Flow
1. `Synthesizer::process()` processes audio in blocks of 64 samples
2. For each block, `modulationMatrix_.update()` is called once
3. This calls `update()` on all modulation sources including LFOs
4. The LFO advances its phase based on the frequency

### Why Block Processing?
- More efficient than per-sample updates
- Reduces CPU overhead
- Prevents crashes from too frequent modulation updates
- Still provides smooth modulation at audio rates

## Result
- LFO now runs at the correct speed
- 1 Hz setting produces 1 cycle per second
- All LFO rates (0.1 Hz to 20 Hz) work as expected
- Maintains stability with block-based processing