# Effects Processing Guide

This document outlines the effects processing system implemented in the AIMusicHardware synthesizer.

## Architecture Overview

The effects processing system is designed with inspiration from professional synthesizers like Vital, featuring:

1. **ReorderableEffectsChain**: A flexible chain of audio effects that can be:
   - Reordered to customize the signal path
   - Enabled/disabled individually
   - Configured with different parameters

2. **Effects Base Class**: All effects inherit from a common `Effect` base class that defines:
   - Parameter handling (`setParameter`/`getParameter`)
   - Audio processing (`process`)
   - Sample rate management

3. **Standard Effects**: The system includes several standard effect types:
   - Time-based effects: Delay, Reverb
   - Filters: LowPass, HighPass, BandPass, Notch
   - Distortion effects: Distortion, BitCrusher, Saturation
   - Dynamics: Compressor
   - Modulation: Phaser, Flanger (via Modulation class)

## Using the Effects System

### Creating Effects

You can create effects using the `createEffect` method:

```cpp
auto effectsChain = std::make_unique<ReorderableEffectsChain>(sampleRate);
auto delay = effectsChain->createEffect("Delay");
effectsChain->addEffect(std::move(delay));
```

### Setting Parameters

Each effect type has specific parameters that can be set:

```cpp
// Get pointer to the effect
Effect* effect = effectsChain->getEffect(0);

// Set parameters based on effect type
if (effect->getName() == "Delay") {
    effect->setParameter("delayTime", 0.5f);  // 500ms delay
    effect->setParameter("feedback", 0.7f);   // 70% feedback
    effect->setParameter("mix", 0.3f);        // 30% wet
}
```

### Common Effect Parameters

#### Delay
- **delayTime**: Delay time in seconds (0.02 - 2.0)
- **feedback**: Feedback amount (0.0 - 0.95)
- **mix**: Wet/dry mix (0.0 - 1.0)

#### Reverb
- **roomSize**: Size of the reverb space (0.0 - 1.0)
- **damping**: High frequency damping (0.0 - 1.0)
- **wetLevel**: Wet level (0.0 - 1.0)
- **dryLevel**: Dry level (0.0 - 1.0)
- **width**: Stereo width (0.0 - 1.0)

#### Filter (LowPass, HighPass, BandPass, Notch)
- **frequency**: Cutoff/center frequency in Hz (20 - 20000)
- **resonance**: Resonance/Q factor (0.1 - 10.0)
- **mix**: Wet/dry mix (0.0 - 1.0)

#### Distortion
- **drive**: Distortion amount (1.0 - 50.0)
- **tone**: Tone control (0.0 - 1.0)
- **mix**: Wet/dry mix (0.0 - 1.0)

#### Compressor
- **threshold**: Threshold in dB (-60 - 0)
- **ratio**: Compression ratio (1.0 - 20.0)
- **attack**: Attack time in ms (0.1 - 100.0)
- **release**: Release time in ms (10.0 - 1000.0)
- **makeup**: Makeup gain in dB (0.0 - 24.0)

## Effects Chain Management

The `ReorderableEffectsChain` class provides methods for managing the effects chain:

### Adding Effects

```cpp
// Add to the end of the chain
int index = effectsChain->addEffect(std::move(effect));

// Insert at a specific position
int index = effectsChain->addEffect(std::move(effect), 2); // Insert as 3rd effect
```

### Removing Effects

```cpp
// Remove an effect by index
effectsChain->removeEffect(1); // Remove the 2nd effect
```

### Reordering Effects

```cpp
// Move effect from position 0 to position 2
effectsChain->moveEffect(0, 2);
```

### Enabling/Disabling Effects

```cpp
// Disable an effect
effectsChain->setEffectEnabled(1, false);

// Enable an effect
effectsChain->setEffectEnabled(1, true);
```

### Processing Audio

The effects chain processes audio in the order of the effects:

```cpp
// Process audio through the entire chain
effectsChain->process(buffer, numFrames);
```

Only enabled effects are processed; disabled effects are bypassed.

## Design Inspiration from Vital

The effects system is inspired by the modular effects architecture of Vital synthesizer:

1. **Effects as Modules**: Each effect is encapsulated in its own class with clean interfaces
2. **Reorderable Chain**: Effects can be arranged in any order for creative sound design
3. **Parameter Management**: Effects have configurable parameters with appropriate ranges
4. **Type-Safe Effects Creation**: Factory pattern for creating effects of various types

## Implementation Notes

### Thread Safety

The effects processing is designed to be thread-safe for audio processing:
- Parameter changes are atomic and safe to call from any thread
- Processing is lock-free during audio rendering
- Effects management (adding/removing/reordering) should be done outside the audio thread

### Performance Considerations

For optimal performance:
- Effects are processed in series to avoid unnecessary buffer copying
- Optimized algorithms are used for time-critical effects
- Disabled effects are completely bypassed without processing overhead

## Future Enhancements

Planned enhancements to the effects system include:

1. **MIDI Control**: Map MIDI controllers directly to effect parameters
2. **Modulation System**: Allow parameters to be modulated by LFOs and envelopes
3. **Parameter Interpolation**: Smooth changes to avoid clicks and pops
4. **Effect Presets**: Save and recall effect settings
5. **Visual Feedback**: Add visualization of effect parameters and signal flow