# Sequencer Guide

## Overview

The AIMusicHardware sequencer system provides comprehensive pattern-based sequencing inspired by industry-leading hardware and software sequencers. It features adaptive capabilities, multi-track support, and game audio-inspired dynamic behavior.

## Key Features

- **Multi-pattern system** with 64+ pattern storage
- **Variable pattern lengths** (1-64 steps, up to 8 bars)
- **8 simultaneous tracks** with independent patterns
- **Real-time recording and step entry**
- **Parameter automation** with smooth curves
- **Adaptive sequencing** based on game audio concepts
- **Performance controls** for live manipulation
- **Horizontal re-sequencing** and vertical remixing

## Quick Start

### Basic Pattern Creation

```cpp
#include "sequencer/Sequencer.h"

// Create sequencer
auto sequencer = std::make_unique<Sequencer>();
sequencer->initialize(44100, 256);  // Sample rate, buffer size

// Create a basic pattern
auto pattern = sequencer->createPattern("basic_pattern", 16);  // 16 steps

// Add notes to the pattern
pattern->setNote(0, 60, 127, 1.0f);   // Step 0: C4, velocity 127, full gate
pattern->setNote(4, 64, 100, 0.5f);   // Step 4: E4, velocity 100, half gate
pattern->setNote(8, 67, 110, 0.75f);  // Step 8: G4, velocity 110, 3/4 gate

// Start playback
sequencer->play();
```

### Real-time Recording

```cpp
// Enable real-time recording
sequencer->setRecording(true);
pattern->setRecordMode(Pattern::RecordMode::Overdub);

// Notes played will be automatically recorded to the active pattern
```

## Core Components

### 1. Pattern System

#### Pattern Creation and Management
```cpp
// Create patterns with different lengths
auto drumPattern = sequencer->createPattern("drums", 16);
auto bassPattern = sequencer->createPattern("bass", 32);
auto leadPattern = sequencer->createPattern("lead", 8);

// Pattern variations
pattern->createVariation("A");
pattern->createVariation("B");
pattern->switchToVariation("B");
```

#### Pattern Properties
```cpp
// Set pattern properties
pattern->setLength(32);                    // 32 steps
pattern->setSwing(0.1f);                  // 10% swing
pattern->setTranspose(2);                 // Transpose up 2 semitones
pattern->setScale(Scale::Minor);          // Force to minor scale
pattern->setProbability(0.8f);            // 80% trigger probability
```

### 2. Track Management

#### Multi-Track Setup
```cpp
// Create different track types
auto drumTrack = sequencer->createTrack("drums", Track::Type::Drum);
auto bassTrack = sequencer->createTrack("bass", Track::Type::Melodic);
auto automationTrack = sequencer->createTrack("filter_auto", Track::Type::Automation);

// Assign patterns to tracks
drumTrack->assignPattern(drumPattern);
bassTrack->assignPattern(bassPattern);
```

#### Track Coupling
```cpp
// Link tracks for synchronized operations
sequencer->linkTracks({"drums", "bass"});  // Synchronized playback
sequencer->setTrackGroup("rhythm", {"drums", "bass", "percussion"});
```

### 3. Step Programming

#### Note Entry
```cpp
// Basic note entry
step->setNote(pitch, velocity, gate);
step->setTrigger(true);
step->setMicrotiming(0.05f);  // Slight timing offset

// Advanced step properties
step->setProbability(0.7f);           // 70% chance to trigger
step->setCondition(Condition::Every2nd); // Only every 2nd time
step->setRetrigger(2, 0.25f);         // 2 retriggers at 1/4 intervals
```

#### Parameter Automation
```cpp
// Automate synthesizer parameters
auto filterAuto = pattern->getAutomationTrack("filter_cutoff");
filterAuto->setAutomationPoint(0, 0.2f);   // Step 0: Low cutoff
filterAuto->setAutomationPoint(8, 0.8f);   // Step 8: High cutoff
filterAuto->setAutomationPoint(15, 0.3f);  // Step 15: Medium cutoff

// Set interpolation curve
filterAuto->setCurveType(CurveType::Exponential);
```

### 4. Performance Controls

#### Real-time Manipulation
```cpp
// Pattern chaining
sequencer->queuePattern("intro", PatternTransition::Immediate);
sequencer->queuePattern("verse", PatternTransition::OnBar);
sequencer->queuePattern("chorus", PatternTransition::OnBeat);

// Live muting/soloing
sequencer->muteTrack("drums");
sequencer->soloTrack("lead");

// Parameter locks (per-step parameter changes)
step->addParameterLock("filter_resonance", 0.8f);
step->addParameterLock("osc_detune", 0.2f);
```

#### Groove and Timing
```cpp
// Apply groove template
sequencer->setGrooveTemplate(GrooveTemplate::Swing16);
sequencer->setGrooveAmount(0.6f);

// Manual timing adjustment
pattern->setStepTiming(4, 0.03f);   // Step 4 slightly late
pattern->setStepTiming(12, -0.02f); // Step 12 slightly early
```

## Adaptive Sequencing

### Game Audio Integration

The sequencer includes adaptive capabilities inspired by game audio middleware:

```cpp
// State-based sequencing
sequencer->defineState("calm", {
    .tempoRange = {80, 100},
    .activePatterns = {"ambient_pad", "soft_lead"},
    .filterFreq = 0.3f
});

sequencer->defineState("intense", {
    .tempoRange = {140, 160},
    .activePatterns = {"hard_drums", "bass_lead", "aggressive_lead"},
    .filterFreq = 0.8f
});

// Automatic state transitions
sequencer->addStateTransition("calm", "intense", 
    Condition::ParameterAbove("energy_level", 0.7f));
```

### Horizontal Re-sequencing

Dynamic pattern modification based on context:

```cpp
// Enable horizontal re-sequencing
pattern->enableHorizontalResequencing(true);

// Define re-sequencing rules
pattern->addResequenceRule(ResequenceRule{
    .condition = "energy > 0.8",
    .action = ResequenceAction::DoubleTime,
    .probability = 0.6f
});

pattern->addResequenceRule(ResequenceRule{
    .condition = "tension < 0.3",
    .action = ResequenceAction::HalfTime,
    .probability = 0.4f
});
```

### Vertical Remixing

Layer-based dynamic arrangement:

```cpp
// Define arrangement layers
sequencer->createLayer("foundation", {"kick", "bass"});
sequencer->createLayer("rhythm", {"snare", "hihat"});
sequencer->createLayer("harmony", {"pad", "lead"});
sequencer->createLayer("decoration", {"percussion", "fx"});

// Dynamic layer control
sequencer->setLayerIntensity("foundation", 1.0f);  // Always active
sequencer->setLayerIntensity("rhythm", 0.8f);      // Mostly active
sequencer->setLayerIntensity("harmony", 0.6f);     // Context-dependent
sequencer->setLayerIntensity("decoration", 0.3f);  // Sparse

// Automatic intensity mapping
sequencer->mapParameterToLayerIntensity("user_energy", "rhythm");
sequencer->mapParameterToLayerIntensity("musical_tension", "harmony");
```

## MIDI Integration

### External Synchronization
```cpp
// MIDI clock sync
sequencer->enableMIDISync(true);
sequencer->setMIDISyncMode(MIDISyncMode::External);

// MIDI note input
sequencer->enableMIDIInput(true);
sequencer->setMIDIInputChannel(1);  // Channel 1 for note input
```

### Pattern Triggering
```cpp
// Map MIDI notes to pattern triggers
sequencer->mapNoteToPattern(36, "kick_pattern");    // C1 triggers kick
sequencer->mapNoteToPattern(38, "snare_pattern");   // D1 triggers snare
sequencer->mapNoteToPattern(42, "hihat_pattern");   // F#1 triggers hi-hat

// Program change for pattern switching
sequencer->enableProgramChangePatterns(true);
```

## Advanced Features

### Polyrhythms and Odd Time Signatures
```cpp
// Create polyrhythmic patterns
auto pattern3_4 = sequencer->createPattern("waltz", 12);  // 3/4 time
pattern3_4->setTimeSignature(3, 4);

auto pattern7_8 = sequencer->createPattern("complex", 14); // 7/8 time
pattern7_8->setTimeSignature(7, 8);

// Run multiple patterns simultaneously
sequencer->playPatternsConcurrently({"pattern_4_4", "pattern3_4", "pattern7_8"});
```

### Euclidean Rhythms
```cpp
// Generate Euclidean rhythms
auto euclidean = pattern->generateEuclideanRhythm(
    16,  // Total steps
    5,   // Hits
    3    // Rotation
);

// Apply to track
drumTrack->applyRhythm(euclidean);
```

### Conditional Logic
```cpp
// Advanced conditional triggers
step->setCondition(Condition::Custom([&]() {
    return (sequencer->getPlaybackPosition() % 4 == 0) && 
           (random() < 0.6f);
}));

// Probability curves
pattern->setProbabilityCurve(ProbabilityCurve::Exponential);
pattern->setProbabilityRange(0.2f, 0.9f);
```

### Sample Accurate Timing
```cpp
// Precise timing control
sequencer->setSampleAccurateTiming(true);
sequencer->setLookaheadSamples(64);  // 64 samples lookahead

// Micro-timing adjustments
step->setMicroTiming(-0.001f);  // 1ms early
step->setTimingVariation(0.002f); // ±2ms random variation
```

## Testing and Development

### Pattern Debugging
```cpp
// Enable debug output
sequencer->setDebugMode(true);

// Pattern analysis
auto analysis = pattern->analyze();
std::cout << "Pattern density: " << analysis.density << std::endl;
std::cout << "Rhythmic complexity: " << analysis.complexity << std::endl;

// Real-time monitoring
sequencer->setStepCallback([](int track, int step, const StepData& data) {
    std::cout << "Track " << track << " Step " << step 
              << " Note: " << data.note << std::endl;
});
```

### Performance Testing
```cpp
// Performance monitoring
auto stats = sequencer->getPerformanceStats();
std::cout << "CPU usage: " << stats.cpuUsage << "%" << std::endl;
std::cout << "Memory usage: " << stats.memoryUsage << " MB" << std::endl;
std::cout << "Latency: " << stats.latency << " ms" << std::endl;
```

## Integration Examples

### Complete Song Structure
```cpp
// Create a complete song arrangement
auto song = sequencer->createSong("example_song");

// Define sections
song->addSection("intro", {"intro_drums", "soft_pad"}, 8);  // 8 bars
song->addSection("verse", {"verse_drums", "bass", "lead"}, 16);
song->addSection("chorus", {"chorus_drums", "bass", "lead", "harmony"}, 16);
song->addSection("bridge", {"minimal_drums", "ambient_pad"}, 8);
song->addSection("outro", {"outro_drums", "soft_pad"}, 8);

// Set up transitions
song->addTransition("intro", "verse", TransitionType::Crossfade, 1.0f);
song->addTransition("verse", "chorus", TransitionType::Cut, 0.0f);
song->addTransition("chorus", "verse", TransitionType::Filter, 2.0f);

// Start playback
song->play();
```

### Live Performance Setup
```cpp
// Configure for live performance
sequencer->setPerformanceMode(true);
sequencer->enableQuantizedLaunching(true);
sequencer->setQuantization(Quantization::Bar);

// Set up scene launching
sequencer->createScene("minimal", {"kick", "bass"});
sequencer->createScene("building", {"kick", "snare", "bass", "lead"});
sequencer->createScene("full", {"kick", "snare", "bass", "lead", "pad", "fx"});

// Map to controllers
sequencer->mapControllerToScene(0, "minimal");
sequencer->mapControllerToScene(1, "building");
sequencer->mapControllerToScene(2, "full");
```

The sequencer system provides professional-grade functionality suitable for both studio production and live performance, with adaptive capabilities that respond intelligently to musical context and user input.