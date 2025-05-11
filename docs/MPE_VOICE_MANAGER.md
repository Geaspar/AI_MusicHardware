# MPE-Aware Voice Manager

This document describes the implementation of the MPE-aware Voice Manager, a key component for supporting MIDI Polyphonic Expression (MPE) in our synthesizer system.

## Overview

The MPE-aware Voice Manager extends our standard voice allocation system with specialized handling for the three MPE expression dimensions:

1. **Pitch Bend (X-axis)** - Per-note pitch control
2. **Timbre (Y-axis)** - Per-note timbre/brightness control (typically CC74)
3. **Pressure (Z-axis)** - Per-note pressure/aftertouch control

While the standard Voice Manager is designed for traditional MIDI with channel-wide expression, the MPE-aware Voice Manager handles expression on a per-note basis by mapping notes to specific MIDI channels in MPE zones.

## Key Classes

### MpeAwareVoiceManager

Extends the standard `VoiceManager` with MPE-specific functionality:

- Specialized note allocation that works with MPE channel assignments
- Expression parameter management for each note/channel
- Zone-specific pitch bend range handling
- Translation between MPE messages and voice parameters

### MpeVoice

Extends the standard `Voice` with MPE-specific capabilities:

- Timbre parameter (Y-axis) implementation via filter modulation
- Enhanced pressure handling for amplitude modulation
- Integrated expression control for all three dimensions

## Implementation Details

### Channel-to-Voice Mapping

In MPE, each active note is assigned to a dedicated MIDI channel (member channel). The MPE-aware Voice Manager maintains a direct mapping between these channels and the voices playing notes on them:

```cpp
std::unordered_map<int, MpeVoice*> channelToVoiceMap_;
```

This allows for efficient lookup when receiving expression messages on a specific channel.

### Expression Parameter Scaling

Different MPE controllers may use different ranges for expression parameters. The MPE Voice Manager handles the scaling of these values:

- Pitch bend values are scaled based on the zone's pitch bend range
- Timbre values (CC74) are normalized to 0.0-1.0
- Pressure values are normalized to 0.0-1.0

### Thread Safety

Since MPE involves rapid updates to multiple expression parameters, thread safety is essential. The MPE Voice Manager uses mutex locking to ensure consistent state when updating parameters:

```cpp
std::mutex mpeMutex_;
```

This prevents race conditions between audio processing and MIDI message handling.

### Zone Management

MPE defines two possible zones:

- **Lower Zone**: Master channel 0, member channels 1-n
- **Upper Zone**: Master channel 15, member channels 14-(15-n)

The MPE Voice Manager respects zone configuration when processing expression messages.

## Integration with Multi-Timbral System

The MPE-aware Voice Manager is designed to work seamlessly with the Multi-Timbral system:

1. The `MultiTimbralMidiRouter` handles MPE message routing and channel allocation
2. The `MultiTimbralEngine` creates and manages one or more `MpeAwareVoiceManager` instances
3. MPE expression messages are routed to the appropriate voice manager through the channel mapping

## Usage Example

Basic usage of the MPE-aware Voice Manager:

```cpp
// Create MPE configuration
MpeConfiguration mpeConfig;
mpeConfig.setLowerZone(true, 7);  // Enable lower zone with 7 member channels

// Create MPE channel allocator
MpeChannelAllocator allocator(mpeConfig);

// Create MPE-aware voice manager
MpeAwareVoiceManager voiceManager(44100, 16, mpeConfig);

// Allocate a channel for a note
int channel = allocator.allocateChannel(60, 100, true);  // Note 60, velocity 100, lower zone

// Play the note with initial expression values
voiceManager.noteOnWithExpression(60, 0.8f, channel, 0.0f, 0.5f, 0.0f);

// Update expression parameters
voiceManager.updateNoteTimbre(channel, 0.7f);        // Increase brightness
voiceManager.updateNotePitchBend(channel, 0.25f);    // Bend up slightly
voiceManager.updateNotePressure(channel, 0.8f);      // Add pressure

// Later, release the note
voiceManager.noteOff(60, channel);
allocator.releaseChannel(channel);
```

## Testing

A dedicated test application (`TestMpeVoiceManager`) demonstrates the MPE capabilities:

- Playing notes with different expression values
- Real-time modulation of all three expression dimensions
- Visualization of parameter changes

## Future Improvements

1. **Performance Optimization**: Further optimize the voice allocation algorithm for rapid MPE messages
2. **Expression Mapping**: Add customizable mapping for expression dimensions
3. **Polyphony Management**: Implement intelligent polyphony reduction for MPE voices
4. **Zone Switching**: Add dynamic zone configuration during performance