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
    
    // Callback for value changes
    using ValueChangeCallback = std::function<void(float)>;
    void setValueChangeCallback(ValueChangeCallback callback);
    
    // UIComponent overrides
    virtual void update(float deltaTime) override;
    virtual void render(DisplayManager* display) override;
    virtual bool handleInput(const InputEvent& event) override;
    
private:
    std::string label_;
    float value_;
    float minValue_;
    float maxValue_;
    float step_;
    bool showValue_;
    Color color_;
    Color backgroundColor_;
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

} // namespace AIMusicHardware