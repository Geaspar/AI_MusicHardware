#pragma once

#include "UIContext.h"
#include <functional>
#include <string>
#include <vector>
#include <deque>

namespace AIMusicHardware {

// Label - Simple text display
class Label : public UIComponent {
public:
    Label(const std::string& id, const std::string& text = "");
    virtual ~Label();
    
    // Text properties
    void setText(const std::string& text);
    const std::string& getText() const;
    void setFontName(const std::string& fontName);
    const std::string& getFontName() const;
    void setTextColor(const Color& color);
    const Color& getTextColor() const;
    void setTextAlignment(int alignment); // 0 = left, 1 = center, 2 = right
    int getTextAlignment() const;
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    std::string text_;
    std::string fontName_;
    Color textColor_;
    int alignment_;
};

// Icon - Simple icon display (single character from icon font)
class Icon : public UIComponent {
public:
    Icon(const std::string& id, int iconCode = 0);
    virtual ~Icon();
    
    // Icon properties
    void setIconCode(int iconCode);
    int getIconCode() const;
    void setColor(const Color& color);
    const Color& getColor() const;
    void setScale(float scale);
    float getScale() const;
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    int iconCode_;
    Color color_;
    float scale_;
};

// Button - Clickable button with optional icon and label
class Button : public UIComponent {
public:
    Button(const std::string& id, const std::string& text = "");
    virtual ~Button();
    
    // Text and icon properties
    void setText(const std::string& text);
    const std::string& getText() const;
    void setIconCode(int iconCode);
    int getIconCode() const;
    void setTextColor(const Color& color);
    void setBackgroundColor(const Color& color);
    void setHighlightColor(const Color& color);
    
    // Callback for click events
    using ClickCallback = std::function<void()>;
    void setClickCallback(ClickCallback callback);
    
    // State
    void setPressed(bool pressed);
    bool isPressed() const;
    void setToggleMode(bool toggleMode);
    bool isToggleMode() const;
    void setToggled(bool toggled);
    bool isToggled() const;
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    std::string text_;
    int iconCode_;
    Color textColor_;
    Color backgroundColor_;
    Color highlightColor_;
    bool pressed_;
    bool toggleMode_;
    bool toggled_;
    ClickCallback clickCallback_;
};

// Knob - Rotary control for parameter adjustment
class Knob : public UIComponent {
public:
    Knob(const std::string& id, const std::string& label = "");
    virtual ~Knob();
    
    // Value properties
    void setValue(float value);
    float getValue() const;
    void setRange(float min, float max);
    void getRange(float& min, float& max) const;
    void setStep(float step);
    float getStep() const;
    
    // Display properties
    void setLabel(const std::string& label);
    const std::string& getLabel() const;
    void setValueFormatter(std::function<std::string(float)> formatter);
    void showValue(bool show);
    bool isShowingValue() const;
    
    // Appearance
    void setColor(const Color& color);
    void setBackgroundColor(const Color& color);
    
    // Modulation
    void setModulationAmount(float amount);
    float getModulationAmount() const;
    void setModulationColor(const Color& color);
    const Color& getModulationColor() const;
    
    // Parameter binding
    void setParameterId(const std::string& parameterId);
    const std::string& getParameterId() const;
    
    // MIDI Learn
    void setMidiLearnEnabled(bool enabled);
    bool isMidiLearnEnabled() const;
    void setMidiControlNumber(int ccNumber);
    int getMidiControlNumber() const;
    
    // Callback for value changes
    using ValueChangeCallback = std::function<void(float)>;
    void setValueChangeCallback(ValueChangeCallback callback);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    std::string label_;
    std::string parameterId_;
    float value_;
    float minValue_;
    float maxValue_;
    float step_;
    bool showValue_;
    Color color_;
    Color backgroundColor_;
    float modulationAmount_;
    Color modulationColor_;
    bool midiLearnEnabled_;
    int midiControlNumber_;
    ValueChangeCallback valueChangeCallback_;
    std::function<std::string(float)> valueFormatter_;
};

// WaveformDisplay - Shows audio waveform
class WaveformDisplay : public UIComponent {
public:
    WaveformDisplay(const std::string& id);
    virtual ~WaveformDisplay();
    
    // Data input
    void setSamples(const std::vector<float>& samples);
    void addSample(float sample);
    void clearSamples();
    
    // Display properties
    void setWaveformColor(const Color& color);
    void setBackgroundColor(const Color& color);
    void setGridColor(const Color& color);
    void showGrid(bool show);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    std::deque<float> samples_;
    size_t maxSamples_;
    Color waveformColor_;
    Color backgroundColor_;
    Color gridColor_;
    bool showGrid_;
};

// EnvelopeEditor - ADSR envelope editor
class EnvelopeEditor : public UIComponent {
public:
    EnvelopeEditor(const std::string& id);
    virtual ~EnvelopeEditor();
    
    // Envelope parameters
    void setAttack(float attack);
    float getAttack() const;
    void setDecay(float decay);
    float getDecay() const;
    void setSustain(float sustain);
    float getSustain() const;
    void setRelease(float release);
    float getRelease() const;
    
    // Appearance
    void setLineColor(const Color& color);
    void setBackgroundColor(const Color& color);
    void setHandleColor(const Color& color);
    
    // Callback for parameter changes
    using EnvelopeChangeCallback = std::function<void(float, float, float, float)>;
    void setEnvelopeChangeCallback(EnvelopeChangeCallback callback);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    float attack_;
    float decay_;
    float sustain_;
    float release_;
    Color lineColor_;
    Color backgroundColor_;
    Color handleColor_;
    EnvelopeChangeCallback envelopeChangeCallback_;
    int activeHandle_; // 0=none, 1=attack, 2=decay, 3=sustain, 4=release
};

// SequencerGrid - Grid for sequencer pattern editing
class SequencerGrid : public UIComponent {
public:
    SequencerGrid(const std::string& id, int rows = 8, int columns = 16);
    virtual ~SequencerGrid();
    
    // Grid properties
    void setGridSize(int rows, int columns);
    void getGridSize(int& rows, int& columns) const;
    
    // Cell access
    void setCellActive(int row, int column, bool active);
    bool isCellActive(int row, int column) const;
    void clearAllCells();
    
    // Appearance
    void setActiveColor(const Color& color);
    void setInactiveColor(const Color& color);
    void setCursorColor(const Color& color);
    void setGridLineColor(const Color& color);
    
    // Playback
    void setPlaybackPosition(int column);
    int getPlaybackPosition() const;
    
    // Callbacks
    using CellChangeCallback = std::function<void(int, int, bool)>;
    void setCellChangeCallback(CellChangeCallback callback);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    int rows_;
    int columns_;
    std::vector<bool> cells_; // row-major order
    int cursorRow_;
    int cursorColumn_;
    int playbackPosition_;
    Color activeColor_;
    Color inactiveColor_;
    Color cursorColor_;
    Color gridLineColor_;
    CellChangeCallback cellChangeCallback_;
    
    // Utility functions
    int getCellIndex(int row, int column) const;
    void moveCursor(int deltaRow, int deltaColumn);
    void toggleCurrentCell();
};

// IconSelector - Grid of icons for mode selection
class IconSelector : public UIComponent {
public:
    IconSelector(const std::string& id, int columns = 4);
    virtual ~IconSelector();
    
    // Icons
    void addIcon(int iconCode, const std::string& tooltip = "");
    void clearIcons();
    int getSelectedIconIndex() const;
    void setSelectedIconIndex(int index);
    
    // Appearance
    void setSelectedColor(const Color& color);
    void setUnselectedColor(const Color& color);
    void setBackgroundColor(const Color& color);
    
    // Callback
    using SelectionChangeCallback = std::function<void(int)>;
    void setSelectionChangeCallback(SelectionChangeCallback callback);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    struct IconItem {
        int iconCode;
        std::string tooltip;
    };
    
    std::vector<IconItem> icons_;
    int columns_;
    int selectedIndex_;
    Color selectedColor_;
    Color unselectedColor_;
    Color backgroundColor_;
    SelectionChangeCallback selectionChangeCallback_;
};

// VUMeter - Volume level meter
class VUMeter : public UIComponent {
public:
    VUMeter(const std::string& id, bool stereo = false);
    virtual ~VUMeter();
    
    // Level setting
    void setLevel(float level);
    void setLevels(float leftLevel, float rightLevel);
    float getLevel() const;
    void getLevels(float& leftLevel, float& rightLevel) const;
    
    // Display properties
    void setStereo(bool stereo);
    bool isStereo() const;
    void setPeakHoldTime(float seconds);
    float getPeakHoldTime() const;
    
    // Appearance
    void setBackgroundColor(const Color& color);
    void setLevelColor(const Color& color);
    void setPeakColor(const Color& color);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    bool stereo_;
    float leftLevel_;
    float rightLevel_;
    float leftPeakLevel_;
    float rightPeakLevel_;
    float peakHoldTime_;
    float leftPeakHoldTimer_;
    float rightPeakHoldTimer_;
    Color backgroundColor_;
    Color levelColor_;
    Color peakColor_;
};

// ParameterBinding - Connects UI components to synth parameters
class ParameterBinding {
public:
    ParameterBinding(UIComponent* component, const std::string& parameterId);
    ~ParameterBinding();
    
    // Connect to a Knob component
    void connectToKnob(Knob* knob);
    
    // Update UI from parameter value
    void updateUI();
    
    // Update parameter from UI value
    void updateParameter();
    
    // Set and get parameter value
    void setValue(float value);
    float getValue() const;
    
    // Set and get modulation amount
    void setModulationAmount(float amount);
    float getModulationAmount() const;
    
    // Get parameter ID
    const std::string& getParameterId() const;
    
    // MIDI mapping
    void setMidiControlNumber(int ccNumber);
    int getMidiControlNumber() const;
    void clearMidiMapping();
    
private:
    UIComponent* component_;  // The UI component bound to the parameter
    Knob* knobComponent_;     // Knob-specific interface if applicable
    std::string parameterId_; // Identifier for the parameter in the synth
    float value_;             // Current parameter value
    float modulationAmount_;  // Current modulation amount
    int midiControlNumber_;   // MIDI CC number (-1 if not mapped)
};

// ParameterPanel - Grid of organized parameter controls
class ParameterPanel : public UIComponent {
public:
    ParameterPanel(const std::string& id, const std::string& title = "");
    virtual ~ParameterPanel();
    
    // Panel properties
    void setTitle(const std::string& title);
    const std::string& getTitle() const;
    void setGridSize(int columns, int rows);
    void getGridSize(int& columns, int& rows) const;
    
    // Add parameters
    Knob* addParameter(const std::string& parameterId, const std::string& label, 
                      float minValue, float maxValue, float defaultValue,
                      int gridX, int gridY);
    
    // Access parameters
    Knob* getParameterKnob(const std::string& parameterId);
    ParameterBinding* getParameterBinding(const std::string& parameterId);
    
    // Get all parameter bindings
    std::vector<ParameterBinding*> getAllParameterBindings();
    
    // MIDI Learn
    void enableMidiLearnForParameter(const std::string& parameterId);
    void disableMidiLearnForAllParameters();
    void setMidiMapping(const std::string& parameterId, int ccNumber);
    void clearAllMidiMappings();
    
    // Appearance
    void setBackgroundColor(const Color& color);
    void setTitleColor(const Color& color);
    void setBorderColor(const Color& color);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    struct ParameterItem {
        std::string parameterId;
        std::string label;
        Knob* knob;
        std::unique_ptr<ParameterBinding> binding;
        int gridX;
        int gridY;
    };
    
    std::string title_;
    int columns_;
    int rows_;
    int cellWidth_;
    int cellHeight_;
    Color backgroundColor_;
    Color titleColor_;
    Color borderColor_;
    std::vector<ParameterItem> parameters_;
    
    // Calculate positions based on grid
    void updateLayout();
    
    // Find parameter by ID
    ParameterItem* findParameter(const std::string& parameterId);
};

// TabView - Container with tabs to organize content
class TabView : public UIComponent {
public:
    TabView(const std::string& id);
    virtual ~TabView();
    
    // Add a new tab
    void addTab(const std::string& tabId, const std::string& tabName);
    
    // Tab content management
    void addComponentToTab(const std::string& tabId, UIComponent* component);
    void removeComponentFromTab(const std::string& tabId, UIComponent* component);
    void clearTabContent(const std::string& tabId);
    
    // Tab access and control
    int getTabCount() const;
    std::string getTabId(int index) const;
    std::string getTabName(int index) const;
    void setTabName(const std::string& tabId, const std::string& newName);
    void setActiveTab(const std::string& tabId);
    std::string getActiveTabId() const;
    int getActiveTabIndex() const;
    
    // Tab navigation
    void nextTab();
    void previousTab();
    
    // Appearance
    void setTabBackgroundColor(const Color& color);
    void setTabActiveColor(const Color& color);
    void setTabTextColor(const Color& color);
    void setContentBackgroundColor(const Color& color);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    struct Tab {
        std::string id;
        std::string name;
        std::vector<UIComponent*> components;
    };
    
    std::vector<Tab> tabs_;
    int activeTabIndex_;
    
    Color tabBackgroundColor_;
    Color tabActiveColor_;
    Color tabTextColor_;
    Color contentBackgroundColor_;
    
    // Utility functions
    int findTabIndex(const std::string& tabId) const;
    void updateTabLayout();
    void updateContentVisibility();
};

} // namespace AIMusicHardware