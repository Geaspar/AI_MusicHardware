# MIDI Effect Control System

This document describes how to use the MIDI effect control system in the AI Music Hardware project.

## Overview

The MIDI effect control system allows you to control effect parameters in real-time using MIDI controllers. This enables expressive performance control over effects such as filter cutoff, delay time, distortion amount, and other effect parameters.

## Features

- MIDI learn capability for mapping controllers to effect parameters
- Parameter scaling based on effect type
- Real-time parameter updates during performance
- Support for using multiple MIDI controllers simultaneously
- Persistence of MIDI mappings across sessions

## Implementation Details

The MIDI effect control system consists of three main components:

1. **MidiManager** - Handles MIDI input and output, message processing, and parameter mapping.
2. **ReorderableEffectsChain** - Manages the chain of audio effects that process the audio signal.
3. **MidiEffectControl** - Bridges MidiManager and ReorderableEffectsChain to allow MIDI control of effect parameters.

### Parameter Naming Convention

Effect parameters use a standardized naming convention for consistency and to facilitate MIDI mapping:

- `effect{index}_{paramName}` - Where `index` is the position in the effects chain and `paramName` is the parameter name.

For example, `effect0_delayTime` refers to the delay time parameter of the first effect in the chain.

## Using MIDI Learn

To assign a MIDI controller to an effect parameter:

1. Open the effects menu with `e`
2. Select MIDI learn option with `l`
3. Choose the effect index and parameter name
4. Move the desired MIDI controller (knob, slider, etc.)
5. The parameter will now respond to that controller

## Effect Parameter Ranges and Scaling

Different effect parameters have different value ranges and response curves:

- **Frequency parameters** (filter cutoff, EQ bands): Logarithmic scaling from 20Hz to 20kHz
- **Time parameters** (delay time, reverb decay): Exponential scaling for more sensitivity at lower values
- **Percentage parameters** (mix, feedback): Linear scaling from 0 to 1 (or 0% to 100%)
- **Gain parameters** (drive, makeup gain): Linear or exponential scaling depending on the parameter

## Example: Filter Cutoff Control with MIDI CC

A common use case is controlling a filter's cutoff frequency with a MIDI controller:

1. Add a LowPassFilter effect to the chain
2. Use MIDI learn to map a knob to the "frequency" parameter
3. As you turn the knob, the filter cutoff will change in real-time
4. This creates expressive filter sweeps for performance

## Implemented Effect Parameters

| Effect Type     | Parameters                                        |
|-----------------|---------------------------------------------------|
| Delay           | delayTime, feedback, mix                          |
| Reverb          | roomSize, damping, wetLevel, dryLevel, width      |
| LowPassFilter   | frequency, resonance, mix                         |
| HighPassFilter  | frequency, resonance, mix                         |
| BandPassFilter  | frequency, resonance, mix                         |
| NotchFilter     | frequency, resonance, mix                         |
| Distortion      | drive, tone, mix                                  |
| Compressor      | threshold, ratio, attack, release, makeup         |
| BitCrusher      | bitDepth, sampleRateReduction, mix                |
| Phaser          | rate, depth, feedback, mix                        |
| EQ              | lowGain, midGain, highGain, lowFreq, highFreq, q  |

## Future Enhancements

- Support for MIDI automation recording and playback
- Mapping multiple parameters to a single controller
- More complex parameter modulation based on MIDI velocity and aftertouch
- Preset system for saving and recalling MIDI mappings