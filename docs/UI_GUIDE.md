# User Interface Guide

## Overview

The AIMusicHardware project features a comprehensive UI system inspired by the Teenage Engineering OP-1 and modern software synthesizers like Vital. The system combines hardware-style direct control with professional software features.

## Design Philosophy

- **Minimal, iconic visual language**: Clean, recognizable interface elements
- **Direct hardware integration**: Seamless hardware control mapping
- **Performance-focused implementation**: Real-time audio thread safety
- **Clear visual feedback**: Immediate response to user interactions
- **Intuitive workflow**: Logical navigation and control flow

## Quick Start

### Running UI Tests

The project includes comprehensive test applications:

```bash
# Basic parameter binding test
cd build
./bin/EnhancedUIIntegrationTest

# Full UI component demonstration
./bin/ComprehensiveUITest
```

### Basic Usage

```cpp
#include "ui/UIContext.h"
#include "ui/SynthKnob.h"

// Create UI context
auto uiContext = std::make_unique<UIContext>();
uiContext->initialize(1280, 800);

// Create a synthesizer knob
auto cutoffKnob = SynthKnobFactory::createFrequencyKnob("Cutoff", 50, 80);
cutoffKnob->bindToParameter(cutoffParameter, ParameterBridge::ScaleType::Exponential);
```

## Core UI Components

### 1. Enhanced Controls

#### SynthKnob
Professional knob control with advanced features:

```cpp
// Create frequency knob with exponential scaling
auto freqKnob = SynthKnobFactory::createFrequencyKnob("Frequency", x, y);
freqKnob->bindToParameter(oscFreqParam, ParameterBridge::ScaleType::Exponential);

// Create resonance knob with quadratic scaling
auto resKnob = SynthKnobFactory::createResonanceKnob("Resonance", x, y);
resKnob->bindToParameter(filterResParam, ParameterBridge::ScaleType::Quadratic);

// Custom knob with fine control
auto customKnob = std::make_unique<SynthKnob>("Custom", x, y, size, min, max, defaultVal);
customKnob->setFineControlEnabled(true);  // Shift-drag for fine control
customKnob->setModulationAmount(0.3f);    // Show modulation visualization
```

**Features:**
- Modulation amount visualization
- Fine control mode (shift-drag)
- Value tooltips with custom formatting
- Parameter binding with automatic scaling
- Factory methods for common parameter types

#### Button Controls
```cpp
auto button = std::make_unique<Button>("play", "Play");
button->setToggleMode(true);
button->setClickCallback([]() {
    // Handle button click
});
```

#### Label and Text
```cpp
auto label = std::make_unique<Label>("title", "Synthesizer");
label->setTextColor(Color(200, 220, 255));
label->setPosition(x, y);
```

### 2. Preset Management

#### PresetBrowserUI
Complete preset management system:

```cpp
auto presetBrowser = std::make_unique<PresetBrowserUI>("preset_browser");
presetBrowser->initialize(presetManager.get(), presetDatabase.get());
presetBrowser->setParameterManager(&paramManager);

// Set up callbacks
presetBrowser->setPresetLoadCallback([](const PresetInfo& preset) {
    std::cout << "Loading preset: " << preset.name << std::endl;
});
```

**Features:**
- Search functionality with real-time filtering
- Category-based organization
- Preset information display (name, author, description)
- Load/Save/Delete operations
- Integration with parameter system

### 3. Visualization Components

#### WaveformVisualizer
Real-time audio visualization:

```cpp
auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Waveform);
waveform->setWaveformColor(Color(0, 255, 128));

// Multiple display modes available
waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Spectrum);    // FFT
waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Waterfall);   // Spectrogram
waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Lissajous);   // X-Y display
```

#### EnvelopeVisualizer
Interactive ADSR envelope display with 4-point editing:

```cpp
auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);  // Attack, Decay, Sustain, Release
envelope->setEditable(true);  // Enable drag editing with all 4 handles

// Callback for parameter changes
envelope->setParameterChangeCallback([](float a, float d, float s, float r) {
    // Update synthesizer parameters
});
```

**Features:**
- Full 4-point ADSR editing (including release handle)
- Real-time envelope visualization
- Drag handles for intuitive control
- Visual feedback during parameter changes

#### LevelMeter
Audio level monitoring:

```cpp
auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
levelMeter->setMeterColors(
    Color(0, 200, 0),    // Green (low)
    Color(200, 200, 0),  // Yellow (mid)
    Color(200, 0, 0)     // Red (high)
);

// Update from audio thread
levelMeter->setLevel(rmsLevel);  // 0.0 to 1.0
```

#### PhaseMeter
Stereo phase correlation display:

```cpp
auto phaseMeter = std::make_unique<PhaseMeter>("phase");
phaseMeter->setTraceColor(Color(0, 255, 128));

// Update with stereo samples
phaseMeter->pushSamples(leftBuffer, rightBuffer, numSamples);
```

#### FilterVisualizer
Vital-style frequency response display with interactive control:

```cpp
auto filterViz = std::make_unique<FilterVisualizer>("filter_viz");
filterViz->setFilterType(FilterVisualizer::FilterType::LowPass);
filterViz->setCutoff(0.5f);  // Normalized 0-1
filterViz->setResonance(0.7f);

// Bind to filter parameters
filterViz->bindCutoffParameter(filterCutoffParam);
filterViz->bindResonanceParameter(filterResonanceParam);

// Handle parameter changes
filterViz->setParameterChangeCallback([](float cutoff, float resonance) {
    // Update synthesizer filter
});
```

**Features:**
- Real-time frequency response visualization
- Interactive cutoff and resonance control
- Logarithmic frequency scaling (20Hz-20kHz)
- Visual feedback with handles and grid
- Feedback loop prevention between sliders and visualizer

### 4. Modulation System

#### Modulation Routing Interface
Visual modulation matrix with source/destination routing:

```cpp
// Create modulation section
auto modSection = std::make_unique<Panel>("mod_section");

// Source dropdown
auto modSource = std::make_unique<Dropdown>("mod_source");
modSource->addOption("LFO 1");
modSource->addOption("LFO 2");
modSource->addOption("Envelope 1");
modSource->addOption("Envelope 2");

// Destination dropdown
auto modDest = std::make_unique<Dropdown>("mod_dest");
modDest->addOption("Filter Cutoff");
modDest->addOption("Filter Resonance");
modDest->addOption("Oscillator Pitch");
modDest->addOption("Oscillator Shape");

// Amount knob
auto modAmount = std::make_unique<SynthKnob>("mod_amount", 0, 0, 50, -1.0f, 1.0f, 0.0f);
modAmount->setLabel("Amount");
```

**Features:**
- Intuitive source/destination dropdowns
- Bidirectional modulation amounts (-100% to +100%)
- Multiple routing slots
- Visual connection indicators

### 5. Effects Processing

#### Effects Chain UI
Reorderable effects chain with bypass and mix controls:

```cpp
// Create effects chain container
auto effectsChain = std::make_unique<Panel>("effects_chain");

// Individual effect slot
auto reverbSlot = std::make_unique<Panel>("reverb_slot");

// Effect controls
auto reverbBypass = std::make_unique<Button>("reverb_bypass", "Bypass");
reverbBypass->setToggleMode(true);
reverbBypass->setSize(60, 25);

auto reverbMix = std::make_unique<Slider>("reverb_mix");
reverbMix->setRange(0.0f, 1.0f);
reverbMix->setValue(0.3f);
reverbMix->setLabel("Mix");

// Effect-specific parameters
auto reverbSize = std::make_unique<SynthKnob>("reverb_size", 0, 0, 50, 0.0f, 1.0f, 0.5f);
reverbSize->setLabel("Size");

auto reverbDamping = std::make_unique<SynthKnob>("reverb_damp", 0, 0, 50, 0.0f, 1.0f, 0.7f);
reverbDamping->setLabel("Damping");
```

**Features:**
- Visual effects chain representation
- Per-effect bypass toggle
- Dry/wet mix control for each effect
- Drag-and-drop reordering support
- Effect-specific parameter controls

## Parameter Binding System

### ValueBridge Architecture
Inspired by Vital's parameter binding system:

```cpp
// Bind UI control to synthesizer parameter
auto bridge = std::make_unique<ParameterBridge>(
    parameter,                              // Target parameter
    ParameterBridge::ScaleType::Exponential // Scaling function
);

// Set value from UI (normalized 0-1)
bridge->setValueFromUI(0.5f, ParameterBridge::ChangeSource::UserInterface);

// Set value from engine (actual parameter value)
bridge->setValueFromEngine(440.0f, ParameterBridge::ChangeSource::Engine);
```

### Scaling Types
Multiple scaling functions available:

- **Linear**: Direct 1:1 mapping
- **Quadratic**: x² scaling for smooth curves
- **Cubic**: x³ scaling for sharp curves
- **Exponential**: exp(x) for frequency parameters
- **Logarithmic**: log(x) for level parameters
- **Power**: x^n with custom exponent
- **Decibel**: 20*log10(x) for gain parameters

### Thread-Safe Updates
Lock-free queues for real-time safety:

```cpp
// UI to Engine updates
ParameterUpdateQueue<1024> uiToEngine;
ParameterUpdateQueue<1024> engineToUI;

// Push update from UI thread
ParameterChange change{parameterId, newValue, ChangeSource::UserInterface};
uiToEngine.push(change);

// Process in audio thread
ParameterChange change;
while (uiToEngine.pop(change)) {
    // Update parameter safely
    updateParameter(change.parameterId, change.value);
}
```

## Screen Management

### UIContext
Central UI coordination:

```cpp
auto uiContext = std::make_unique<UIContext>();
uiContext->setDisplayManager(displayManager);
uiContext->initialize(width, height);

// Create and add screens
auto mainScreen = std::make_unique<Screen>("main");
mainScreen->addChild(std::move(knob));
mainScreen->addChild(std::move(button));

uiContext->addScreen(std::move(mainScreen));
uiContext->setActiveScreen("main");
```

### Event Handling
Input processing with event delegation:

```cpp
// Translate platform events to UI events
InputEvent translateSDLEvent(const SDL_Event& sdlEvent) {
    InputEvent event;
    switch (sdlEvent.type) {
        case SDL_MOUSEBUTTONDOWN:
            event.type = InputEventType::TouchPress;
            event.value = sdlEvent.button.x;
            event.value2 = sdlEvent.button.y;
            break;
        // ... handle other events
    }
    return event;
}

// Process in main loop
while (SDL_PollEvent(&sdlEvent)) {
    InputEvent inputEvent = translateSDLEvent(sdlEvent);
    uiContext->handleInput(inputEvent);
}
```

## Testing and Development

### Mock Implementation
For development without hardware:

```cpp
// Use dummy IoT interface for testing
auto dummyIoT = std::make_unique<DummyIoTInterface>();
paramManager.connectIoTInterface(dummyIoT.get());
```

### Debug Features
Built-in debugging support:

```cpp
// Enable parameter debugging
#define PARAMETER_DEBUG 1

// Enable UI event logging
#define UI_DEBUG 1
```

### Performance Monitoring
Real-time performance tracking:

```cpp
// Frame rate monitoring
auto lastTime = std::chrono::steady_clock::now();
float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();

// Update UI
uiContext->update(deltaTime);
uiContext->render();
```

## Integration Examples

### Complete Synthesizer Interface

```cpp
// Create main synthesizer interface
auto synthScreen = std::make_unique<Screen>("synthesizer");
synthScreen->setBackgroundColor(Color(20, 20, 25));

// Oscillator section
auto oscFreq = SynthKnobFactory::createFrequencyKnob("Frequency", 50, 80);
oscFreq->bindToParameter(paramManager.findParameter("osc_freq"));

auto oscDetune = std::make_unique<SynthKnob>("Detune", 180, 80, 80, -50.0f, 50.0f, 0.0f);
oscDetune->bindToParameter(paramManager.findParameter("osc_detune"));

// Filter section
auto filterCutoff = SynthKnobFactory::createFrequencyKnob("Cutoff", 310, 80);
filterCutoff->bindToParameter(paramManager.findParameter("filter_cutoff"));

auto filterRes = SynthKnobFactory::createResonanceKnob("Resonance", 440, 80);
filterRes->bindToParameter(paramManager.findParameter("filter_res"));

// Visualization
auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
waveform->setPosition(50, 200);
waveform->setSize(300, 150);

auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
envelope->setPosition(400, 200);
envelope->setSize(250, 150);
envelope->setEditable(true);

// Add all components
synthScreen->addChild(std::move(oscFreq));
synthScreen->addChild(std::move(oscDetune));
synthScreen->addChild(std::move(filterCutoff));
synthScreen->addChild(std::move(filterRes));
synthScreen->addChild(std::move(waveform));
synthScreen->addChild(std::move(envelope));
```

### Real-time Audio Integration

```cpp
// Audio thread simulation
void audioThreadSimulation(bool& running, WaveformVisualizer* waveform) {
    const int sampleRate = 44100;
    const int bufferSize = 256;
    std::vector<float> audioBuffer(bufferSize * 2);  // Stereo
    
    while (running) {
        // Generate audio samples
        generateAudioSamples(audioBuffer.data(), bufferSize);
        
        // Update visualization (thread-safe)
        if (waveform) {
            waveform->pushSamples(audioBuffer.data(), bufferSize, 2);
        }
        
        // Simulate audio processing time
        std::this_thread::sleep_for(
            std::chrono::microseconds(bufferSize * 1000000 / sampleRate));
    }
}
```

## Troubleshooting

### Common Issues

**UI not responding:**
- Check event polling in main loop
- Verify input event translation
- Ensure UI update() is called regularly

**Parameter updates not working:**
- Verify parameter binding
- Check thread safety of updates
- Confirm scaling function is appropriate

**Rendering issues:**
- Check DisplayManager initialization
- Verify render() is called after update()
- Ensure proper color format

**Performance problems:**
- Reduce visualization update rates
- Use appropriate buffer sizes
- Check for memory leaks in UI objects

### Debug Output

Enable debug logging for detailed information:

```cpp
#define UI_DEBUG 1
#define PARAMETER_DEBUG 1
#define VISUALIZATION_DEBUG 1
```

The UI system provides a professional-grade interface suitable for both hardware and software implementations, with comprehensive testing and development tools.