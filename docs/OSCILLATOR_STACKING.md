# Oscillator Stacking and Unison

This document provides an overview of the oscillator stacking features implemented in the AIMusicHardware project, including unison configuration and sound design capabilities.

## Overview

Oscillator stacking allows for richer, fuller sounds by layering multiple oscillators for each note. This implementation provides up to 8 oscillators per voice with configurable detune, stereo width, and level convergence to create a wide range of timbral possibilities.

## Key Components

### OscillatorStack

The `OscillatorStack` class manages a collection of up to 8 wavetable oscillators with individual parameters:

- **Oscillator Count**: Number of oscillators in the stack (1-8)
- **Detune Spread**: The frequency spread between oscillators in cents
- **Detune Type**: Different distributions of detuning (even, center-weighted, alternating)
- **Stereo Width**: Controls stereo image through individual oscillator panning
- **Level Convergence**: Enables louder center oscillators for a more focused sound

### StackedVoice

`StackedVoice` extends the base `Voice` class to use `OscillatorStack` for sound generation:

- Processes multiple oscillators per voice
- Supports stereo output
- Provides simplified unison configuration
- Maintains compatibility with the existing voice architecture

### StackedVoiceManager

`StackedVoiceManager` extends `VoiceManager` to create and manage `StackedVoice` instances:

- Global control of oscillator count, detune, and stereo parameters
- Automatic configuration of new voices with current settings
- Maintains the same interface as the standard VoiceManager

## Unison Presets

The implementation includes several built-in detune distribution types:

1. **Even**: Oscillators are evenly distributed across the detune range
2. **Center Weighted**: Oscillators are clustered toward the center frequency
3. **Alternating**: Oscillators alternate between positive and negative detune values
4. **Random**: Oscillators use random detune values within the specified range

## Usage Examples

### Basic Unison Setup

```cpp
// Create a stacked voice manager with 16 voices, 3 oscillators per voice
auto voiceManager = std::make_unique<StackedVoiceManager>(44100, 16, 3);

// Configure unison with 20 cents detune, 0.7 stereo width, and 0.3 convergence
voiceManager->configureUnison(3, 20.0f, 0.7f, 0.3f);

// Play a note
voiceManager->noteOn(60, 100);
```

### Changing Detune Distribution

```cpp
// Set to center-weighted detune distribution
voiceManager->setDetuneType(DetuneType::CenterWeighted);
```

### Adjusting Parameters in Real-time

All parameters can be adjusted in real-time for expressive sound design:

```cpp
// Change settings during playback
voiceManager->setDetuneSpread(15.0f);  // 15 cents of detune
voiceManager->setStereoWidth(0.5f);    // Moderate stereo width
voiceManager->setConvergence(0.0f);    // Equal level for all oscillators
```

## Integration with MPE

The stacked voice architecture is designed to work seamlessly with MPE (MIDI Polyphonic Expression):

```cpp
// Create an MPE-aware stacked voice manager
auto mpeStackedManager = std::make_unique<MpeAwareStackedVoiceManager>(44100, 16, 3);
mpeStackedManager->configureUnison(3, 15.0f, 0.5f, 0.2f);
```

## Testing

The `OscillatorStackDemo` application demonstrates the oscillator stacking features:

1. Compile the project with CMake
2. Run `./bin/OscillatorStackDemo`
3. Use the interactive commands to adjust parameters and play notes

## Computational Considerations

- Each additional oscillator increases CPU usage
- Performance scales linearly with oscillator count
- Memory usage is minimal compared to CPU impact
- For resource-constrained systems, limiting to 2-4 oscillators is recommended

## Future Enhancements

- Oscillator phase offsets for additional timbral variation
- Individual waveform selection per oscillator
- Modulation of detune spread and stereo width
- Pitch drift for analog-style instability
- Sub-oscillator configuration within the stack

## Technical Implementation Details

The implementation uses composition rather than inheritance for oscillator management, resulting in a flexible and maintainable architecture that can be extended with new features.