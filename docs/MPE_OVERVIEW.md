# What is MPE?

MPE (MIDI Polyphonic Expression) is an official extension of the MIDI 1.0 specification that allows expressive instruments like LinnStrument to have three dimensions of continuous touch control (pressure, left/right and front/back movement) independently for each of simultaneous touches.

This overcomes a fundamental limitation of MIDI: when sending over a single MIDI channel, Pitch Bend messages (used for left/right finger movement) and Control Change messages (used for front/back finger movement) must apply to all touches played, preventing independent 3D control over each touch of a chord. MPE gets around this by sending each touch over a separate MIDI channel.

## MPE Specification Overview

There are two MPE zones, the Lower and Upper Zones:

### Lower Zone
- **Master Channel** (called Main in LinnStrument): 1
- **Member Channels** (called Per-Note in LinnStrument): 2 through as high as 16

### Upper Zone
- **Master Channel** (called Main in LinnStrument): 16
- **Member Channels** (called Per-Note in LinnStrument): 15 through as low as 2

The **Master channel** is used to send messages that apply to all touches, like:
- Sustain Pedal
- Program Change
- Volume/CC7
- And other global controls

The **Member channels** are used to send 5 messages that apply to a single touch:
1. Note On
2. Note Off
3. Pitch Bend for left/right movement
4. CC74 for front/back movement
5. Channel Pressure for finger pressure

Each touch is sent over one of these Member channels, rotating through the assigned Member channels.

## Common Configurations

### Example 1: Single MPE Sound
For playing a single MPE sound across the entire playing surface, use the Lower Zone with:
- Master Channel: 1
- Member channels: 2 through as high as 16 (commonly channels 2 though 8)

### Example 2: Split Keyboard Mode with MPE on Both Splits
For playing in Split Keyboard mode with MPE on both splits:

**Left Split (Lower Zone):**
- Master Channel: 1
- Member channels: 2 through 8

**Right Split (Upper Zone):**
- Master Channel: 16
- Member Channels: 9 through 15

## Key Messages for MPE Expression

MPE uses these specific MIDI messages for expressive control:

1. **Pitch Bend**: Used for horizontal movement (left/right), typically representing pitch changes
2. **Channel Pressure (Aftertouch)**: Used for pressure/force of touch
3. **CC74 (Timbre/Brightness)**: Used for vertical movement (forward/backward)

## Benefits of MPE

- Independent pitch bends for each note in a chord
- Independent pressure/timbre control for each note
- Allows for highly expressive performances that were impossible with traditional MIDI
- Better representation of acoustic instrument expressivity
- Ideal for continuous controllers like ribbon controllers, expressive grid instruments, and multi-touch surfaces

## Implementation Notes

- MPE-compatible synthesizers must respond to messages on multiple MIDI channels as a single instrument
- Each note should maintain its own state for pitch bend, pressure, and timbre
- Notes are distributed across channels, and when a note ends, its channel becomes available for new notes
- The synthesizer should use the same timbral settings for all channels in an MPE zone