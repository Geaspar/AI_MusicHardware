# Advanced Filter System Documentation

## Overview

The Advanced Filter System provides a versatile and extensible architecture for audio filtering with multiple filter types, smooth parameter control, and filter blending capabilities. This system significantly enhances the sound design possibilities in the AIMusicHardware project by offering professional-quality filters inspired by classic hardware and modern software synthesizers.

## Architecture

The system is designed around a modular architecture with these key components:

1. **FilterModel (Base Class)**: The core interface for all filter implementations
2. **AdvancedFilter**: The main effect class that manages filter models and provides blending
3. **Specific Filter Implementations**:
   - BiquadFilterModel: Standard biquad filters (lowpass, highpass, bandpass, notch)
   - LadderFilterModel: Moog-style ladder filter with resonance and drive
   - CombFilterModel: Comb filter with feedback and modulation for phaser/flanger effects
   - FormantFilterModel: Vowel formant filter for vocal-like sounds

### Class Hierarchy

```
Effect (base class)
└── AdvancedFilter
    ├── FilterModel (abstract base class)
    │   ├── BiquadFilterModel
    │   ├── LadderFilterModel
    │   ├── CombFilterModel
    │   └── FormantFilterModel
    └── [future filter models]
```

## Filter Types

### Biquad Filters

The standard biquad filters provide basic filtering capabilities:

- **Low Pass**: Allows frequencies below the cutoff to pass through
- **High Pass**: Allows frequencies above the cutoff to pass through
- **Band Pass**: Allows a band of frequencies around the cutoff to pass through
- **Notch**: Attenuates a band of frequencies around the cutoff

Parameters:
- Frequency (20Hz-20kHz)
- Resonance (0.1-10.0)
- Mix (0.0-1.0)

### Ladder Filter

The ladder filter implements a classic 4-pole Moog-style filter with resonance compensation:

- **Ladder Low Pass**: 24dB/octave lowpass inspired by classic analog synthesizers
- **Ladder High Pass**: 24dB/octave highpass with similar character

Parameters:
- Frequency (20Hz-20kHz)
- Resonance (0.0-1.0)
- Drive (0.5-10.0): Adds subtle saturation for analog-like warmth
- Poles (1-4): Number of filter stages/poles (6dB/octave per pole)
- Mix (0.0-1.0)

### Comb Filter

The comb filter provides both feed-forward and feedback implementations, which can be used for phaser, flanger, and chorus-like effects:

- **Comb Filter (IIR)**: Feedback comb filter that creates resonant peaks
- **Phaser**: Feed-forward comb filter with modulation that creates notches

Parameters:
- Delay Time (0.1-100ms): Delay time in milliseconds
- Feedback (-0.99-0.99): Amount of feedback (negative values create phase inversion)
- Modulation Amount (0.0-20.0ms): How much delay time is modulated
- Modulation Rate (0.01-10.0Hz): Speed of modulation
- Direct Mix (0.0-1.0): Mix of direct signal into the output
- Mix (0.0-1.0): Overall wet/dry mix

### Formant Filter

The formant filter simulates vocal tract resonances to create vowel-like sounds:

Parameters:
- Vowel (0.0-1.0): Selects between five vowel sounds (A, E, I, O, U)
- Morph (0.0-1.0): Enables smooth morphing between vowels
- Gender (0.0-1.0): Shifts formant frequencies up for feminine characteristics
- Resonance (0.0-1.0): Adjusts the prominence/resonance of the formant bands
- Mix (0.0-1.0): Overall wet/dry mix

## Filter Blending

The system supports blending between any two filter types, creating unique hybrid filters:

- Enable/disable blending
- Select a second filter type to blend with
- Adjust blend amount (0.0 = only primary filter, 1.0 = only secondary filter)

Blend examples:
- Low Pass + High Pass = Band Pass with adjustable width
- Ladder + Formant = Vocal-like filtered sounds
- Comb + Formant = Talking flanger effect

## Usage Examples

### Basic Filter Usage

```cpp
// Create a ladder filter
auto filter = std::make_unique<AdvancedFilter>(sampleRate, AdvancedFilter::Type::LadderLowPass);

// Set parameters
filter->setParameter("frequency", 1000.0f);  // 1kHz cutoff
filter->setParameter("resonance", 0.7f);     // Moderate resonance
filter->setParameter("drive", 1.5f);         // Slight overdrive

// Process audio
filter->process(audioBuffer, numFrames);
```

### Filter Blending

```cpp
// Create a filter
auto filter = std::make_unique<AdvancedFilter>(sampleRate, AdvancedFilter::Type::LowPass);

// Set up blending
filter->setBlendMode(true);                           // Enable blending
filter->setBlendType(AdvancedFilter::Type::Formant);  // Blend with formant filter
filter->setParameter("blend_amount", 0.3f);           // 30% blend

// Process audio
filter->process(audioBuffer, numFrames);
```

### Formant Filter for Vocal Effects

```cpp
// Create a formant filter
auto filter = std::make_unique<AdvancedFilter>(sampleRate, AdvancedFilter::Type::Formant);

// Set parameters for "ooh" vowel sound
filter->setParameter("vowel", 0.8f);      // Closer to 'U'
filter->setParameter("gender", 0.3f);     // Slightly masculine
filter->setParameter("resonance", 0.9f);  // Strong resonance

// Process audio
filter->process(audioBuffer, numFrames);
```

## Demo Application

The `AdvancedFilterDemo` application demonstrates all filter types and blending capabilities:

- Interactive command-line interface for adjusting parameters
- Multiple source waveforms for testing filters
- Real-time parameter control
- MIDI note input for musical testing
- Blend mode demonstration

To run the demo:
```
./bin/AdvancedFilterDemo
```

## Implementation Details

### Signal Processing Quality

Special care has been taken to ensure high-quality audio processing:

- Anti-aliasing considerations in filter design
- Smooth parameter transitions to prevent clicks and pops
- Proper normalization to prevent clipping
- Efficient processing for real-time performance

### Extensibility

The system is designed for easy extension:

1. To add a new filter type:
   - Create a new class derived from FilterModel
   - Implement the required process() and parameter methods
   - Add the new type to the AdvancedFilter::Type enum
   - Update AdvancedFilter::createFilterModel()

2. To add new parameters to existing filters:
   - Add parameter to the respective FilterModel subclass
   - Implement parameter getter/setter methods
   - Update coefficient calculation if needed

## Future Enhancements

Potential future additions to the filter system:

1. **State Variable Filter**: A versatile filter with simultaneous outputs
2. **Multi-Mode Filter**: Morphable filter between different responses
3. **Diode Ladder Filter**: Alternative ladder filter with different distortion characteristics
4. **Buchla-style Filter**: Low pass gate with VCA characteristics
5. **Spectral Filter**: FFT-based filtering for more complex spectral effects
6. **Granular Filter**: Grain-based filtering effects
7. **Parameter Modulation**: Built-in modulation sources for filter parameters