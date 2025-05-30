# UI Technical Documentation

## Architecture Overview

The UI system follows a modern, thread-safe architecture inspired by professional audio software and hardware synthesizers:

```
Presentation Layer
├── UIContext (Screen Management)
├── Screen (Layout Container)
└── UIComponents (Controls & Widgets)

Parameter Layer
├── ParameterBridge (Value Mapping)
├── ParameterUpdateQueue (Thread-Safe Communication)
└── EnhancedParameterManager (Central Registry)

Rendering Layer
├── DisplayManager (Hardware Abstraction)
├── Color (Color Management)
└── Font (Text Rendering)

Input Layer
├── InputEvent (Event System)
├── InputEventType (Event Classification)
└── Hardware Integration
```

## Core Classes

### UIContext

Central coordinator for the entire UI system:

```cpp
class UIContext {
public:
    bool initialize(int width, int height);
    void shutdown();
    
    // Screen management
    void addScreen(std::unique_ptr<Screen> screen);
    void setActiveScreen(const std::string& screenId);
    Screen* getActiveScreen() const;
    Screen* getScreen(const std::string& id) const;
    
    // Main loop integration
    void update(float deltaTime);
    void render();
    bool handleInput(const InputEvent& event);
    
    // Display management
    void setDisplayManager(std::shared_ptr<DisplayManager> displayManager);
    DisplayManager* getDisplayManager() const;
    
private:
    std::shared_ptr<DisplayManager> displayManager_;
    std::map<std::string, std::unique_ptr<Screen>> screens_;
    std::string activeScreenId_;
    
    // Input state tracking
    bool isMousePressed_ = false;
    int lastMouseX_ = 0, lastMouseY_ = 0;
};
```

### UIComponent (Base Class)

All UI elements inherit from this base class:

```cpp
class UIComponent {
public:
    UIComponent(const std::string& id) : id_(id) {}
    virtual ~UIComponent() = default;
    
    // Core interface
    virtual void update(float deltaTime) = 0;
    virtual void render(DisplayManager* display) = 0;
    virtual bool handleInput(const InputEvent& event) = 0;
    
    // Properties
    const std::string& getId() const { return id_; }
    void setPosition(int x, int y) { x_ = x; y_ = y; }
    void setSize(int width, int height) { width_ = width; height_ = height; }
    void setVisible(bool visible) { visible_ = visible; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
    // Bounds checking
    bool isPointInside(int x, int y) const {
        return x >= x_ && x < x_ + width_ && y >= y_ && y < y_ + height_;
    }
    
protected:
    std::string id_;
    int x_ = 0, y_ = 0;
    int width_ = 100, height_ = 50;
    bool visible_ = true;
    bool enabled_ = true;
};
```

### Screen (Container)

Manages collections of UI components:

```cpp
class Screen : public UIComponent {
public:
    explicit Screen(const std::string& id) : UIComponent(id) {}
    
    // Child management
    void addChild(std::unique_ptr<UIComponent> child);
    void removeChild(const std::string& id);
    UIComponent* getChild(const std::string& id) const;
    
    // Background
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }
    
    // Component interface
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
private:
    std::vector<std::unique_ptr<UIComponent>> children_;
    std::map<std::string, UIComponent*> childMap_;  // For fast lookup
    Color backgroundColor_{20, 20, 20};
};
```

## Parameter Binding System

### ParameterBridge

Handles bidirectional communication between UI controls and synthesis parameters:

```cpp
class ParameterBridge {
public:
    enum class ScaleType {
        Linear,        // y = x
        Quadratic,     // y = x²
        Cubic,         // y = x³
        Exponential,   // y = exp(x)
        Logarithmic,   // y = log(x)
        Power,         // y = x^n
        Decibel        // y = 20*log10(x)
    };
    
    enum class ChangeSource {
        UserInterface,
        Engine,
        Automation,
        MIDI,
        IoT
    };
    
    ParameterBridge(Parameter* parameter, ScaleType scaleType = ScaleType::Linear);
    
    // Value conversion
    void setValueFromUI(float normalized, ChangeSource source);
    void setValueFromEngine(float value, ChangeSource source);
    
    float toNormalized(float value) const;
    float fromNormalized(float normalized) const;
    
    // Smoothing
    void setSmoothing(bool enabled, float timeConstant = 0.01f);
    void updateSmoothing(float deltaTime);
    
private:
    Parameter* parameter_;
    ScaleType scaleType_;
    float powerExponent_ = 2.0f;
    
    // Smoothing
    bool smoothingEnabled_ = false;
    float smoothingTimeConstant_ = 0.01f;
    float currentValue_ = 0.0f;
    float targetValue_ = 0.0f;
    
    // Conversion functions
    float applyScale(float normalized) const;
    float reverseScale(float scaled) const;
};
```

### ParameterUpdateQueue

Lock-free queue for thread-safe parameter updates:

```cpp
template<size_t Capacity = 1024>
class ParameterUpdateQueue {
public:
    struct ParameterChange {
        Parameter::ParameterId parameterId;
        float value;
        ParameterBridge::ChangeSource source;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    // Thread-safe operations
    bool push(const ParameterChange& change);
    bool pop(ParameterChange& change);
    
    // Statistics
    size_t size() const;
    bool isEmpty() const;
    bool isFull() const;
    
    // Performance monitoring
    struct Statistics {
        uint64_t totalPushes = 0;
        uint64_t totalPops = 0;
        uint64_t droppedUpdates = 0;
        float averageLatency = 0.0f;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
private:
    // Lock-free ring buffer
    std::array<ParameterChange, Capacity> buffer_;
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    
    // Statistics (atomic for thread safety)
    mutable Statistics stats_;
    std::atomic<uint64_t> pushCount_{0};
    std::atomic<uint64_t> popCount_{0};
};
```

## Enhanced UI Controls

### SynthKnob

Professional knob control with modulation visualization:

```cpp
class SynthKnob : public Knob {
public:
    SynthKnob(const std::string& id, int x, int y, int size,
              float minValue, float maxValue, float defaultValue);
    
    // Parameter binding
    void bindToParameter(Parameter* parameter, 
                        ParameterBridge::ScaleType scaleType = ParameterBridge::ScaleType::Linear);
    
    // Modulation visualization
    void setModulationAmount(float amount);  // -1.0 to 1.0
    void setModulationColor(const Color& color);
    
    // Interaction modes
    void setFineControlEnabled(bool enable);
    void setDoubleClickReset(bool enable);
    
    // Visual customization
    void setKnobColor(const Color& color);
    void setIndicatorColor(const Color& color);
    void setValueFormatter(std::function<std::string(float)> formatter);
    
    // Component interface
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
private:
    std::unique_ptr<ParameterBridge> parameterBridge_;
    
    // Modulation
    float modulationAmount_ = 0.0f;
    Color modulationColor_{255, 150, 0};
    
    // Interaction state
    bool isDragging_ = false;
    bool fineControlEnabled_ = true;
    bool doubleClickReset_ = true;
    int lastDragY_ = 0;
    std::chrono::steady_clock::time_point lastClickTime_;
    
    // Visual state
    float displayValue_ = 0.0f;  // Smoothed for animation
    std::function<std::string(float)> valueFormatter_;
    
    // Rendering
    void drawKnobBody(DisplayManager* display);
    void drawIndicator(DisplayManager* display);
    void drawModulationRing(DisplayManager* display);
    void drawValueTooltip(DisplayManager* display);
};
```

### Factory Methods

Convenient factory methods for common parameter types:

```cpp
class SynthKnobFactory {
public:
    // Frequency knobs (logarithmic scaling)
    static std::unique_ptr<SynthKnob> createFrequencyKnob(
        const std::string& label, int x, int y, 
        float minFreq = 20.0f, float maxFreq = 20000.0f);
    
    // Gain knobs (decibel scaling)
    static std::unique_ptr<SynthKnob> createGainKnob(
        const std::string& label, int x, int y,
        float minDB = -96.0f, float maxDB = 12.0f);
    
    // Resonance knobs (quadratic scaling)
    static std::unique_ptr<SynthKnob> createResonanceKnob(
        const std::string& label, int x, int y);
    
    // Time knobs (exponential scaling)
    static std::unique_ptr<SynthKnob> createTimeKnob(
        const std::string& label, int x, int y,
        float minTime = 0.001f, float maxTime = 10.0f);
    
    // Percentage knobs (linear scaling)
    static std::unique_ptr<SynthKnob> createPercentageKnob(
        const std::string& label, int x, int y);
};
```

## Visualization Components

### WaveformVisualizer

Real-time audio visualization with multiple modes:

```cpp
class WaveformVisualizer : public UIComponent {
public:
    enum class DisplayMode {
        Waveform,      // Oscilloscope view
        Spectrum,      // FFT magnitude
        Waterfall,     // Scrolling spectrogram
        Lissajous      // X-Y stereo display
    };
    
    WaveformVisualizer(const std::string& id, int bufferSize = 1024);
    
    // Audio input (thread-safe)
    void pushSamples(const float* samples, int numSamples, int channels = 1);
    
    // Display configuration
    void setDisplayMode(DisplayMode mode);
    void setZoomLevel(float zoom);
    void setYScale(float scale);
    
    // Visual settings
    void setWaveformColor(const Color& color);
    void setBackgroundColor(const Color& color);
    void setGridColor(const Color& color);
    void showGrid(bool show);
    
private:
    // Thread-safe audio buffer
    std::vector<float> audioBuffer_;
    int bufferSize_;
    std::atomic<int> writePos_{0};
    std::atomic<int> readPos_{0};
    mutable std::mutex bufferMutex_;
    
    // Display settings
    DisplayMode displayMode_ = DisplayMode::Waveform;
    float zoomLevel_ = 1.0f;
    float yScale_ = 1.0f;
    
    // FFT data
    std::vector<float> fftMagnitudes_;
    std::vector<float> fftPhases_;
    
    // Waterfall history
    std::deque<std::vector<float>> waterfallHistory_;
    static constexpr int WATERFALL_HISTORY_SIZE = 100;
    
    // Rendering methods
    void drawWaveform(DisplayManager* display);
    void drawSpectrum(DisplayManager* display);
    void drawWaterfall(DisplayManager* display);
    void drawLissajous(DisplayManager* display);
    void performFFT();
};
```

### EnvelopeVisualizer

Interactive ADSR envelope display:

```cpp
class EnvelopeVisualizer : public UIComponent {
public:
    using ParameterChangeCallback = std::function<void(float attack, float decay, float sustain, float release)>;
    
    EnvelopeVisualizer(const std::string& id);
    
    // Envelope parameters
    void setADSR(float attack, float decay, float sustain, float release);
    void setCurrentPhase(int phase);  // 0=off, 1=attack, 2=decay, 3=sustain, 4=release
    void setPhaseProgress(float progress);  // 0-1 within current phase
    
    // Interaction
    void setEditable(bool editable);
    void setParameterChangeCallback(ParameterChangeCallback callback);
    
private:
    // Envelope parameters
    float attack_ = 0.01f, decay_ = 0.1f, sustain_ = 0.7f, release_ = 0.5f;
    int currentPhase_ = 0;
    float phaseProgress_ = 0.0f;
    
    // Interaction state
    bool isEditable_ = false;
    int dragHandle_ = -1;  // Which handle is being dragged
    ParameterChangeCallback parameterChangeCallback_;
    
    // Rendering
    void calculateEnvelopePoints(std::vector<Point>& points);
    void drawEnvelope(DisplayManager* display, const std::vector<Point>& points);
    void drawHandles(DisplayManager* display, const std::vector<Point>& points);
    void drawPhaseIndicator(DisplayManager* display, const std::vector<Point>& points);
    
    // Interaction
    int getHandleAtPosition(int x, int y, const std::vector<Point>& points);
    void updateParameterFromHandle(int handle, int x, int y);
};
```

## Preset Management UI

### PresetBrowserUI

Complete preset management interface:

```cpp
class PresetBrowserUI : public UIComponent {
public:
    PresetBrowserUI(const std::string& id);
    
    // Initialization
    void initialize(PresetManager* presetManager, PresetDatabase* database);
    void setParameterManager(EnhancedParameterManager* paramManager);
    
    // Operations
    void refresh();
    void loadSelectedPreset();
    void saveAsNewPreset(const std::string& name, const std::string& category);
    
    // Callbacks
    using PresetLoadCallback = std::function<void(const PresetInfo&)>;
    using PresetSaveCallback = std::function<void(const std::string&, const std::string&)>;
    
    void setPresetLoadCallback(PresetLoadCallback callback);
    void setPresetSaveCallback(PresetSaveCallback callback);
    
private:
    // Sub-components
    std::unique_ptr<PresetSearchBox> searchBox_;
    std::unique_ptr<PresetCategoryFilter> categoryFilter_;
    std::unique_ptr<PresetListView> listView_;
    std::unique_ptr<Label> presetInfoLabel_;
    std::unique_ptr<Button> loadButton_, saveButton_, deleteButton_;
    
    // Data
    PresetManager* presetManager_ = nullptr;
    PresetDatabase* database_ = nullptr;
    EnhancedParameterManager* parameterManager_ = nullptr;
    
    std::vector<PresetInfo> filteredPresets_;
    PresetInfo selectedPreset_;
    
    // Internal methods
    void filterPresets();
    void layoutComponents();
};
```

## Display Management

### DisplayManager (Abstract)

Hardware abstraction for rendering:

```cpp
class DisplayManager {
public:
    virtual ~DisplayManager() = default;
    
    // Initialization
    virtual bool initialize(int width, int height) = 0;
    virtual void shutdown() = 0;
    
    // Frame management
    virtual void clear(const Color& color = Color(0, 0, 0)) = 0;
    virtual void swapBuffers() = 0;
    
    // Primitive drawing
    virtual void drawLine(int x1, int y1, int x2, int y2, const Color& color) = 0;
    virtual void drawRect(int x, int y, int width, int height, const Color& color) = 0;
    virtual void fillRect(int x, int y, int width, int height, const Color& color) = 0;
    
    // Text rendering
    virtual void drawText(int x, int y, const std::string& text, Font* font, const Color& color) = 0;
    
    // Properties
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
};
```

### Color System

```cpp
struct Color {
    uint8_t r, g, b, a;
    
    Color(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
    
    // HSV conversion
    static Color fromHSV(float hue, float saturation, float value, float alpha = 1.0f);
    void toHSV(float& hue, float& saturation, float& value) const;
    
    // Predefined colors
    static const Color Black, White, Red, Green, Blue, Yellow, Cyan, Magenta;
    
    // Color operations
    Color lerp(const Color& other, float t) const;
    Color multiply(float factor) const;
    Color blend(const Color& other, float alpha) const;
};
```

## Input System

### InputEvent

Unified input event system:

```cpp
enum class InputEventType {
    TouchPress,
    TouchRelease,
    TouchMove,
    ButtonPress,
    ButtonRelease,
    EncoderRotate,
    None
};

struct InputEvent {
    InputEventType type = InputEventType::None;
    int id = 0;          // Touch ID, button ID, or encoder ID
    float value = 0.0f;  // X coordinate, encoder delta
    float value2 = 0.0f; // Y coordinate
    
    InputEvent() = default;
    InputEvent(InputEventType t, int i = 0, float v = 0.0f, float v2 = 0.0f)
        : type(t), id(i), value(v), value2(v2) {}
};
```

## Performance Optimization

### Memory Management

```cpp
// Object pooling for frequently allocated objects
template<typename T, size_t PoolSize = 256>
class ObjectPool {
public:
    T* acquire();
    void release(T* obj);
    
private:
    std::array<T, PoolSize> pool_;
    std::array<bool, PoolSize> used_;
    size_t nextIndex_ = 0;
};

// Pre-allocated update queues
static constexpr size_t PARAMETER_QUEUE_SIZE = 1024;
static constexpr size_t EVENT_QUEUE_SIZE = 512;
```

### Rendering Optimization

```cpp
class RenderBatcher {
public:
    // Batch similar render operations
    void addRect(int x, int y, int w, int h, const Color& color);
    void addLine(int x1, int y1, int x2, int y2, const Color& color);
    
    // Flush batched operations
    void flush(DisplayManager* display);
    
private:
    std::vector<RectOp> rectOps_;
    std::vector<LineOp> lineOps_;
    static constexpr size_t MAX_BATCH_SIZE = 1000;
};
```

## Integration Points

### Audio Thread Integration

```cpp
// Safe parameter updates from audio thread
class AudioThreadParameterUpdater {
public:
    void updateParameters(ParameterUpdateQueue<>& queue) {
        ParameterUpdateQueue<>::ParameterChange change;
        while (queue.pop(change)) {
            auto* param = parameterManager_.findParameter(change.parameterId);
            if (param) {
                param->setValue(change.value);
            }
        }
    }
    
private:
    EnhancedParameterManager& parameterManager_;
};
```

### MIDI Integration

```cpp
// MIDI learn functionality
class MIDILearnManager {
public:
    void enableLearn(UIComponent* component, Parameter* parameter);
    void handleMIDIMessage(const MIDIMessage& message);
    
private:
    struct LearnMapping {
        UIComponent* component;
        Parameter* parameter;
        int midiCC;
        int midiChannel;
    };
    
    std::vector<LearnMapping> learnMappings_;
    bool learningEnabled_ = false;
};
```

This technical documentation provides the complete implementation details for understanding and extending the UI system.