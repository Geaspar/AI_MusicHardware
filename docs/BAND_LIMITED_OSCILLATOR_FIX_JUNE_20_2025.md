# Band-Limited Oscillator Waveform Fix
**Date: June 20, 2025**
**Updated: June 22, 2025**

## Issue Description

The band-limited oscillators were producing incorrect waveforms. When playing a sine wave, the user reported seeing "loads of little sine waves between multiple lines" in the visualizer, indicating the generated frequency was much higher than expected.

## Root Cause Analysis

### 1. Frequency Update Inconsistency
The `BandLimitedVoice::updateOscillatorFrequency()` method was overriding the base class implementation and calculating frequency independently:

```cpp
// OLD CODE - Incorrect
void BandLimitedVoice::updateOscillatorFrequency() {
    float totalPitch = getTotalPitch();
    float frequency = 440.0f * std::pow(2.0f, (totalPitch - 69.0f) / 12.0f);
    bandLimitedOscillator_->setFrequency(frequency);
    frequency_ = frequency;
    baseFrequency_ = frequency;
}
```

This bypassed the base class's pitch modulation smoothing system, which uses `pitchMod_.smoothedPitch` for proper audio-rate modulation.

### 2. Wavetable Interpolation Bug
The wavetable interpolation was incorrectly calculating the sample index:

```cpp
// OLD CODE - Incorrect
float indexFloat = phase * (samples.size() - 1);
```

This would cause the phase to not fully span the wavetable, potentially causing frequency artifacts.

## Solution Implemented

### 1. Fixed Frequency Update
Modified `BandLimitedVoice::updateOscillatorFrequency()` to properly use the base class implementation:

```cpp
// NEW CODE - Correct
void BandLimitedVoice::updateOscillatorFrequency() {
    // Call base class implementation to update frequency_ and base oscillator
    Voice::updateOscillatorFrequency();
    
    // Also update our band-limited oscillator with the same frequency
    bandLimitedOscillator_->setFrequency(frequency_);
}
```

This ensures:
- Both oscillators use the same frequency
- Pitch modulation smoothing is properly applied
- The frequency calculation is consistent with the base Voice class

### 2. Fixed Wavetable Interpolation
Corrected the sample index calculation in `BandLimitedWavetable::interpolateSample()`:

```cpp
// NEW CODE - Correct
float BandLimitedWavetable::interpolateSample(const std::vector<float>& samples, float phase) const {
    // Convert phase to sample index
    float indexFloat = phase * samples.size();
    int index1 = static_cast<int>(indexFloat) % samples.size();
    int index2 = (index1 + 1) % samples.size();
    
    // Fractional part for interpolation
    float frac = indexFloat - std::floor(indexFloat);
    
    // Linear interpolation
    return samples[index1] * (1.0f - frac) + samples[index2] * frac;
}
```

## Files Modified

1. `/src/synthesis/voice/band_limited_voice.cpp` - Fixed updateOscillatorFrequency() method
2. `/src/synthesis/wavetable/band_limited_wavetable.cpp` - Fixed interpolateSample() method

## Testing Recommendations

1. Test sine wave generation at various frequencies (220Hz, 440Hz, 880Hz)
2. Verify waveform selector properly switches between Sine, Saw, Square, and Triangle
3. Check that pitch bend and modulation work correctly with band-limited oscillators
4. Confirm the visualizer shows the correct waveform shape

## Technical Details

The band-limited oscillator system uses:
- 11 frequency bands, each covering one octave from 20Hz to 40.96kHz
- Pre-computed wavetables with harmonic limiting based on Nyquist frequency
- Linear interpolation between samples and optional interpolation between bands
- Each voice has both a base WavetableOscillator and a BandLimitedOscillator

The fix ensures proper synchronization between these oscillators and correct phase accumulation for accurate frequency generation.

## Implementation Status (June 22, 2025)

Both fixes have been successfully implemented:

1. **BandLimitedVoice::generateSample()** - Now properly delegates to the base class first to handle pitch modulation and frequency updates, then syncs the band-limited oscillator with the updated frequency. This ensures the pitch modulation smoothing system works correctly.

2. **BandLimitedWavetable::interpolateSample()** - Already corrected to use `phase * samples.size()` for proper wavetable interpolation.

The code compiles successfully and test programs (TestBandLimitedOscillator, TestAudio) run without issues. To enable band-limited oscillators in the synthesizer, use:
```cpp
synthesizer->enableBandLimitedOscillators(true);
```