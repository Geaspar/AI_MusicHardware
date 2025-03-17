#include "../../include/ui/UIComponents.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace AIMusicHardware {

//
// Label implementation
//
Label::Label(const std::string& id, const std::string& text)
    : UIComponent(id), text_(text), fontName_("default"), 
      textColor_(Color::White()), alignment_(0) {
}

Label::~Label() {
}

void Label::setText(const std::string& text) {
    text_ = text;
}

const std::string& Label::getText() const {
    return text_;
}

void Label::setFontName(const std::string& fontName) {
    fontName_ = fontName;
}

const std::string& Label::getFontName() const {
    return fontName_;
}

void Label::setTextColor(const Color& color) {
    textColor_ = color;
}

const Color& Label::getTextColor() const {
    return textColor_;
}

void Label::setTextAlignment(int alignment) {
    alignment_ = std::clamp(alignment, 0, 2);
}

int Label::getTextAlignment() const {
    return alignment_;
}

void Label::update(float deltaTime) {
    // Labels typically don't need updating
}

void Label::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Get font to use for rendering
    Font* font = nullptr;
    // TODO: Get font from context
    
    if (font) {
        // Calculate text width for alignment
        int textWidth = 0;
        int textHeight = 0;
        font->getTextDimensions(text_, textWidth, textHeight);
        
        int x = x_;
        if (alignment_ == 1) { // Center
            x = x_ + (width_ - textWidth) / 2;
        } else if (alignment_ == 2) { // Right
            x = x_ + width_ - textWidth;
        }
        
        // Calculate vertical centering
        int y = y_ + (height_ - textHeight) / 2;
        
        // Draw the text
        display->drawText(x, y, text_, font, textColor_);
    } else {
        // Fallback rendering if no font is available
        // Simple rectangle with different colors for debugging
        display->fillRect(x_, y_, width_, height_, Color(50, 50, 50));
        display->drawRect(x_, y_, width_, height_, textColor_);
    }
    
    // Render children
    renderChildren(display);
}

bool Label::handleInput(const InputEvent& event) {
    // Labels don't typically handle input
    return handleChildrenInput(event);
}

//
// Icon implementation
//
Icon::Icon(const std::string& id, int iconCode)
    : UIComponent(id), iconCode_(iconCode), color_(Color::White()), scale_(1.0f) {
}

Icon::~Icon() {
}

void Icon::setIconCode(int iconCode) {
    iconCode_ = iconCode;
}

int Icon::getIconCode() const {
    return iconCode_;
}

void Icon::setColor(const Color& color) {
    color_ = color;
}

const Color& Icon::getColor() const {
    return color_;
}

void Icon::setScale(float scale) {
    scale_ = std::max(0.1f, scale);
}

float Icon::getScale() const {
    return scale_;
}

void Icon::update(float deltaTime) {
    // Icons typically don't need updating
}

void Icon::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Get icon font
    Font* iconFont = nullptr;
    // TODO: Get icon font from context
    
    if (iconFont) {
        // Convert icon code to string
        char iconChar = static_cast<char>(iconCode_);
        std::string iconText(1, iconChar);
        
        // Calculate scaling and positioning
        int centerX = x_ + width_ / 2;
        int centerY = y_ + height_ / 2;
        
        // Render the icon
        display->drawText(centerX, centerY, iconText, iconFont, color_);
    } else {
        // Fallback rendering if no font is available
        display->fillRect(x_, y_, width_, height_, Color(60, 60, 60));
        display->drawRect(x_, y_, width_, height_, color_);
    }
    
    // Render children
    renderChildren(display);
}

bool Icon::handleInput(const InputEvent& event) {
    // Icons don't typically handle input
    return handleChildrenInput(event);
}

//
// Button implementation
//
Button::Button(const std::string& id, const std::string& text)
    : UIComponent(id), text_(text), iconCode_(0), 
      textColor_(Color::White()), backgroundColor_(Color(60, 60, 60)), 
      highlightColor_(Color(100, 100, 100)),
      pressed_(false), toggleMode_(false), toggled_(false),
      clickCallback_(nullptr) {
}

Button::~Button() {
}

void Button::setText(const std::string& text) {
    text_ = text;
}

const std::string& Button::getText() const {
    return text_;
}

void Button::setIconCode(int iconCode) {
    iconCode_ = iconCode;
}

int Button::getIconCode() const {
    return iconCode_;
}

void Button::setTextColor(const Color& color) {
    textColor_ = color;
}

void Button::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void Button::setHighlightColor(const Color& color) {
    highlightColor_ = color;
}

void Button::setClickCallback(ClickCallback callback) {
    clickCallback_ = callback;
}

void Button::setPressed(bool pressed) {
    pressed_ = pressed;
}

bool Button::isPressed() const {
    return pressed_;
}

void Button::setToggleMode(bool toggleMode) {
    toggleMode_ = toggleMode;
}

bool Button::isToggleMode() const {
    return toggleMode_;
}

void Button::setToggled(bool toggled) {
    toggled_ = toggled;
}

bool Button::isToggled() const {
    return toggled_;
}

void Button::update(float deltaTime) {
    // Update button state
}

void Button::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Determine button color based on state
    Color bgColor = backgroundColor_;
    if (pressed_ || toggled_) {
        bgColor = highlightColor_;
    }
    
    // Draw button background
    display->fillRect(x_, y_, width_, height_, bgColor);
    display->drawRect(x_, y_, width_, height_, textColor_);
    
    // Draw button text
    Font* font = nullptr;
    // TODO: Get font from context
    
    if (font) {
        // Calculate text dimensions for centering
        int textWidth = 0;
        int textHeight = 0;
        font->getTextDimensions(text_, textWidth, textHeight);
        
        int textX = x_ + (width_ - textWidth) / 2;
        int textY = y_ + (height_ - textHeight) / 2;
        
        // Draw text
        display->drawText(textX, textY, text_, font, textColor_);
    } else {
        // Fallback rendering if no font available
        display->drawRect(x_ + 2, y_ + 2, width_ - 4, height_ - 4, textColor_);
    }
    
    // Render children
    renderChildren(display);
}

bool Button::handleInput(const InputEvent& event) {
    bool handled = false;
    
    if (event.type == InputEventType::ButtonPress || 
        event.type == InputEventType::TouchPress) {
        // Check if click is within button bounds
        if (event.value >= x_ && event.value < x_ + width_ &&
            event.value2 >= y_ && event.value2 < y_ + height_) {
            pressed_ = true;
            handled = true;
        }
    } else if (event.type == InputEventType::ButtonRelease ||
               event.type == InputEventType::TouchRelease) {
        // Check if release is within button bounds
        if (pressed_ && event.value >= x_ && event.value < x_ + width_ &&
            event.value2 >= y_ && event.value2 < y_ + height_) {
            
            // Handle toggle mode
            if (toggleMode_) {
                toggled_ = !toggled_;
            }
            
            // Execute callback
            if (clickCallback_) {
                clickCallback_();
            }
            
            handled = true;
        }
        
        pressed_ = false;
    }
    
    if (!handled) {
        handled = handleChildrenInput(event);
    }
    
    return handled;
}

//
// Knob implementation
//
Knob::Knob(const std::string& id, const std::string& label)
    : UIComponent(id), label_(label), value_(0.5f), 
      minValue_(0.0f), maxValue_(1.0f), step_(0.01f), showValue_(true),
      color_(Color(200, 200, 200)), backgroundColor_(Color(40, 40, 40)),
      valueChangeCallback_(nullptr) {
}

Knob::~Knob() {
}

void Knob::setValue(float value) {
    // Clamp value to range
    value_ = std::clamp(value, minValue_, maxValue_);
    
    // Round to nearest step
    if (step_ > 0.0f) {
        value_ = std::round(value_ / step_) * step_;
    }
    
    // Notify callback of value change
    if (valueChangeCallback_) {
        valueChangeCallback_(value_);
    }
}

float Knob::getValue() const {
    return value_;
}

void Knob::setRange(float min, float max) {
    minValue_ = min;
    maxValue_ = max;
    
    // Ensure value is within new range
    setValue(value_);
}

void Knob::getRange(float& min, float& max) const {
    min = minValue_;
    max = maxValue_;
}

void Knob::setStep(float step) {
    step_ = step;
}

float Knob::getStep() const {
    return step_;
}

void Knob::setLabel(const std::string& label) {
    label_ = label;
}

const std::string& Knob::getLabel() const {
    return label_;
}

void Knob::setValueFormatter(std::function<std::string(float)> formatter) {
    valueFormatter_ = formatter;
}

void Knob::showValue(bool show) {
    showValue_ = show;
}

bool Knob::isShowingValue() const {
    return showValue_;
}

void Knob::setColor(const Color& color) {
    color_ = color;
}

void Knob::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void Knob::setValueChangeCallback(ValueChangeCallback callback) {
    valueChangeCallback_ = callback;
}

void Knob::update(float deltaTime) {
    // Knobs typically don't need per-frame updates
}

void Knob::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Calculate knob dimensions
    int centerX = x_ + width_ / 2;
    int centerY = y_ + height_ / 2;
    int radius = std::min(width_, height_) / 2 - 4;
    
    // Draw knob background
    display->fillCircle(centerX, centerY, radius, backgroundColor_);
    display->drawCircle(centerX, centerY, radius, color_);
    
    // Calculate normalized value (0.0 - 1.0)
    float normalizedValue = (value_ - minValue_) / (maxValue_ - minValue_);
    
    // Draw indicator line
    float angle = (normalizedValue * 270.0f - 135.0f) * 3.14159f / 180.0f;
    int indicatorX = centerX + static_cast<int>(std::cos(angle) * radius * 0.8f);
    int indicatorY = centerY + static_cast<int>(std::sin(angle) * radius * 0.8f);
    display->drawLine(centerX, centerY, indicatorX, indicatorY, color_);
    
    // Draw label and value text
    Font* font = nullptr;
    // TODO: Get font from context
    
    if (font) {
        // Draw label text
        display->drawText(centerX - label_.length() * 4, y_ + height_ + 5, label_, font, color_);
        
        // Draw value text if enabled
        if (showValue_) {
            std::string valueText;
            if (valueFormatter_) {
                valueText = valueFormatter_(value_);
            } else {
                // Default formatting
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(2) << value_;
                valueText = ss.str();
            }
            
            display->drawText(centerX - valueText.length() * 4, y_ - 15, valueText, font, color_);
        }
    }
    
    // Render children
    renderChildren(display);
}

bool Knob::handleInput(const InputEvent& event) {
    bool handled = false;
    
    // Handle rotary encoder input
    if (event.type == InputEventType::EncoderRotate) {
        // Calculate delta based on step size
        float delta = event.value * step_;
        
        // Update value
        setValue(value_ + delta);
        
        handled = true;
    }
    
    // Handle touch/drag input
    else if (event.type == InputEventType::TouchMove) {
        // Check if within knob bounds
        int centerX = x_ + width_ / 2;
        int centerY = y_ + height_ / 2;
        int radius = std::min(width_, height_) / 2;
        
        int dx = event.value - centerX;
        int dy = event.value2 - centerY;
        float distance = std::sqrt(dx*dx + dy*dy);
        
        if (distance <= radius) {
            // Calculate angle
            float angle = std::atan2(dy, dx) * 180.0f / 3.14159f;
            
            // Normalize angle to 0-270 degrees clockwise starting from -135
            if (angle < -135) angle += 360;
            float normalizedAngle = (angle + 135) / 270.0f;
            
            // Clamp to 0-1 range
            normalizedAngle = std::clamp(normalizedAngle, 0.0f, 1.0f);
            
            // Set value based on angle
            float newValue = minValue_ + normalizedAngle * (maxValue_ - minValue_);
            setValue(newValue);
            
            handled = true;
        }
    }
    
    if (!handled) {
        handled = handleChildrenInput(event);
    }
    
    return handled;
}

//
// WaveformDisplay implementation
//
WaveformDisplay::WaveformDisplay(const std::string& id)
    : UIComponent(id), maxSamples_(512), 
      waveformColor_(Color(0, 200, 0)), backgroundColor_(Color(10, 10, 10)),
      gridColor_(Color(40, 40, 40)), showGrid_(true) {
}

WaveformDisplay::~WaveformDisplay() {
}

void WaveformDisplay::setSamples(const std::vector<float>& samples) {
    // Clear existing samples
    samples_.clear();
    
    // Copy new samples, limiting to maxSamples_
    for (size_t i = 0; i < samples.size() && samples_.size() < maxSamples_; i++) {
        samples_.push_back(samples[i]);
    }
}

void WaveformDisplay::addSample(float sample) {
    // Add new sample
    samples_.push_back(sample);
    
    // Remove oldest sample if we've exceeded maxSamples_
    if (samples_.size() > maxSamples_) {
        samples_.pop_front();
    }
}

void WaveformDisplay::clearSamples() {
    samples_.clear();
}

void WaveformDisplay::setWaveformColor(const Color& color) {
    waveformColor_ = color;
}

void WaveformDisplay::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void WaveformDisplay::setGridColor(const Color& color) {
    gridColor_ = color;
}

void WaveformDisplay::showGrid(bool show) {
    showGrid_ = show;
}

void WaveformDisplay::update(float deltaTime) {
    // No update needed for display-only component
}

void WaveformDisplay::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    
    // Draw grid if enabled
    if (showGrid_) {
        // Horizontal lines (center and +/- 0.5)
        int centerY = y_ + height_ / 2;
        display->drawLine(x_, centerY, x_ + width_, centerY, gridColor_);
        
        int upperY = y_ + height_ / 4;
        int lowerY = y_ + 3 * height_ / 4;
        display->drawLine(x_, upperY, x_ + width_, upperY, gridColor_);
        display->drawLine(x_, lowerY, x_ + width_, lowerY, gridColor_);
        
        // Vertical lines (10% intervals)
        for (int i = 1; i < 10; i++) {
            int vertX = x_ + (i * width_) / 10;
            display->drawLine(vertX, y_, vertX, y_ + height_, gridColor_);
        }
    }
    
    // Draw waveform if we have samples
    if (!samples_.empty()) {
        int centerY = y_ + height_ / 2;
        float verticalScale = height_ / 2.0f;
        
        float horizontalScale = static_cast<float>(width_) / samples_.size();
        
        // Draw waveform connecting the points
        for (size_t i = 1; i < samples_.size(); i++) {
            int x1 = x_ + static_cast<int>((i - 1) * horizontalScale);
            int y1 = centerY - static_cast<int>(samples_[i - 1] * verticalScale);
            
            int x2 = x_ + static_cast<int>(i * horizontalScale);
            int y2 = centerY - static_cast<int>(samples_[i] * verticalScale);
            
            display->drawLine(x1, y1, x2, y2, waveformColor_);
        }
    }
    
    // Draw border
    display->drawRect(x_, y_, width_, height_, waveformColor_);
    
    // Render children
    renderChildren(display);
}

bool WaveformDisplay::handleInput(const InputEvent& event) {
    // Handle child input
    return handleChildrenInput(event);
}

//
// EnvelopeEditor implementation
//
EnvelopeEditor::EnvelopeEditor(const std::string& id)
    : UIComponent(id), 
      attack_(0.1f), decay_(0.2f), sustain_(0.7f), release_(0.5f),
      lineColor_(Color(200, 200, 0)), backgroundColor_(Color(20, 20, 20)),
      handleColor_(Color(255, 100, 0)),
      envelopeChangeCallback_(nullptr), activeHandle_(0) {
}

EnvelopeEditor::~EnvelopeEditor() {
}

void EnvelopeEditor::setAttack(float attack) {
    attack_ = std::clamp(attack, 0.01f, 1.0f);
    if (envelopeChangeCallback_) {
        envelopeChangeCallback_(attack_, decay_, sustain_, release_);
    }
}

float EnvelopeEditor::getAttack() const {
    return attack_;
}

void EnvelopeEditor::setDecay(float decay) {
    decay_ = std::clamp(decay, 0.01f, 1.0f);
    if (envelopeChangeCallback_) {
        envelopeChangeCallback_(attack_, decay_, sustain_, release_);
    }
}

float EnvelopeEditor::getDecay() const {
    return decay_;
}

void EnvelopeEditor::setSustain(float sustain) {
    sustain_ = std::clamp(sustain, 0.0f, 1.0f);
    if (envelopeChangeCallback_) {
        envelopeChangeCallback_(attack_, decay_, sustain_, release_);
    }
}

float EnvelopeEditor::getSustain() const {
    return sustain_;
}

void EnvelopeEditor::setRelease(float release) {
    release_ = std::clamp(release, 0.01f, 1.0f);
    if (envelopeChangeCallback_) {
        envelopeChangeCallback_(attack_, decay_, sustain_, release_);
    }
}

float EnvelopeEditor::getRelease() const {
    return release_;
}

void EnvelopeEditor::setLineColor(const Color& color) {
    lineColor_ = color;
}

void EnvelopeEditor::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void EnvelopeEditor::setHandleColor(const Color& color) {
    handleColor_ = color;
}

void EnvelopeEditor::setEnvelopeChangeCallback(EnvelopeChangeCallback callback) {
    envelopeChangeCallback_ = callback;
}

void EnvelopeEditor::update(float deltaTime) {
    // No continuous updates needed
}

void EnvelopeEditor::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    
    // Calculate envelope points
    float totalEnvTime = attack_ + decay_ + release_ + 0.5f; // 0.5f is sustain width
    float timeScale = width_ / totalEnvTime;
    float heightScale = height_;
    
    int startX = x_;
    int startY = y_ + height_; // Start at bottom (0 value)
    
    // Attack point
    int attackX = startX + static_cast<int>(attack_ * timeScale);
    int attackY = y_; // Top of the envelope
    
    // Decay point
    int decayX = attackX + static_cast<int>(decay_ * timeScale);
    int sustainY = y_ + static_cast<int>((1.0f - sustain_) * heightScale);
    
    // Sustain end point
    int sustainEndX = decayX + static_cast<int>(0.5f * timeScale); // Fixed sustain width
    
    // Release end point
    int releaseX = sustainEndX + static_cast<int>(release_ * timeScale);
    int releaseY = y_ + height_;
    
    // Draw envelope line
    display->drawLine(startX, startY, attackX, attackY, lineColor_);
    display->drawLine(attackX, attackY, decayX, sustainY, lineColor_);
    display->drawLine(decayX, sustainY, sustainEndX, sustainY, lineColor_);
    display->drawLine(sustainEndX, sustainY, releaseX, releaseY, lineColor_);
    
    // Draw handle points
    int handleRadius = 5;
    display->fillCircle(attackX, attackY, handleRadius, handleColor_);
    display->fillCircle(decayX, sustainY, handleRadius, handleColor_);
    display->fillCircle(sustainEndX, sustainY, handleRadius, handleColor_);
    display->fillCircle(releaseX, releaseY, handleRadius, handleColor_);
    
    // Draw border
    display->drawRect(x_, y_, width_, height_, lineColor_);
    
    // Render children
    renderChildren(display);
}

bool EnvelopeEditor::handleInput(const InputEvent& event) {
    bool handled = false;
    
    // Calculate envelope points for hit testing
    float totalEnvTime = attack_ + decay_ + release_ + 0.5f; // 0.5f is sustain width
    float timeScale = width_ / totalEnvTime;
    float heightScale = height_;
    
    int startX = x_;
    int startY = y_ + height_;
    
    int attackX = startX + static_cast<int>(attack_ * timeScale);
    int attackY = y_;
    
    int decayX = attackX + static_cast<int>(decay_ * timeScale);
    int sustainY = y_ + static_cast<int>((1.0f - sustain_) * heightScale);
    
    int sustainEndX = decayX + static_cast<int>(0.5f * timeScale);
    
    int releaseX = sustainEndX + static_cast<int>(release_ * timeScale);
    int releaseY = y_ + height_;
    
    // Handle for touch/drag interaction
    if (event.type == InputEventType::TouchPress) {
        // Hit testing for handles
        int hitRadius = 10; // Larger hit area for touch
        
        // Check each handle
        if (std::abs(event.value - attackX) < hitRadius && 
            std::abs(event.value2 - attackY) < hitRadius) {
            activeHandle_ = 1; // Attack
            handled = true;
        } else if (std::abs(event.value - decayX) < hitRadius && 
                  std::abs(event.value2 - sustainY) < hitRadius) {
            activeHandle_ = 2; // Decay/Sustain
            handled = true;
        } else if (std::abs(event.value - sustainEndX) < hitRadius && 
                  std::abs(event.value2 - sustainY) < hitRadius) {
            activeHandle_ = 3; // Sustain/Release
            handled = true;
        } else if (std::abs(event.value - releaseX) < hitRadius && 
                  std::abs(event.value2 - releaseY) < hitRadius) {
            activeHandle_ = 4; // Release end
            handled = true;
        }
    } else if (event.type == InputEventType::TouchMove) {
        // Handle dragging of active handle
        if (activeHandle_ == 1) { // Attack
            // Calculate attack time from x position
            float newAttack = (event.value - startX) / timeScale;
            setAttack(newAttack);
            handled = true;
        } else if (activeHandle_ == 2) { // Decay/Sustain
            // Calculate decay time from x position
            float newDecay = (event.value - attackX) / timeScale;
            setDecay(newDecay);
            
            // Calculate sustain level from y position
            float newSustain = 1.0f - (event.value2 - y_) / heightScale;
            setSustain(newSustain);
            handled = true;
        } else if (activeHandle_ == 3) { // Sustain/Release
            // For simplicity, only allow changing sustain level
            float newSustain = 1.0f - (event.value2 - y_) / heightScale;
            setSustain(newSustain);
            handled = true;
        } else if (activeHandle_ == 4) { // Release end
            // Calculate release time from x position
            float newRelease = (event.value - sustainEndX) / timeScale;
            setRelease(newRelease);
            handled = true;
        }
    } else if (event.type == InputEventType::TouchRelease) {
        activeHandle_ = 0; // Clear active handle
    }
    
    if (!handled) {
        handled = handleChildrenInput(event);
    }
    
    return handled;
}

//
// SequencerGrid implementation
//
SequencerGrid::SequencerGrid(const std::string& id, int rows, int columns)
    : UIComponent(id), rows_(rows), columns_(columns),
      cursorRow_(0), cursorColumn_(0), playbackPosition_(-1),
      activeColor_(Color(0, 200, 0)), inactiveColor_(Color(40, 40, 40)),
      cursorColor_(Color(200, 200, 0)), gridLineColor_(Color(60, 60, 60)),
      cellChangeCallback_(nullptr) {
    
    // Initialize grid with all cells inactive
    cells_.resize(rows_ * columns_, false);
}

SequencerGrid::~SequencerGrid() {
}

void SequencerGrid::setGridSize(int rows, int columns) {
    // Resize grid
    rows_ = std::max(1, rows);
    columns_ = std::max(1, columns);
    
    // Resize cells array
    cells_.resize(rows_ * columns_, false);
    
    // Reset cursor if needed
    cursorRow_ = std::min(cursorRow_, rows_ - 1);
    cursorColumn_ = std::min(cursorColumn_, columns_ - 1);
    
    // Reset playback position if needed
    if (playbackPosition_ >= columns_) {
        playbackPosition_ = -1;
    }
}

void SequencerGrid::getGridSize(int& rows, int& columns) const {
    rows = rows_;
    columns = columns_;
}

void SequencerGrid::setCellActive(int row, int column, bool active) {
    if (row >= 0 && row < rows_ && column >= 0 && column < columns_) {
        int index = getCellIndex(row, column);
        cells_[index] = active;
        
        // Notify callback
        if (cellChangeCallback_) {
            cellChangeCallback_(row, column, active);
        }
    }
}

bool SequencerGrid::isCellActive(int row, int column) const {
    if (row >= 0 && row < rows_ && column >= 0 && column < columns_) {
        return cells_[getCellIndex(row, column)];
    }
    return false;
}

void SequencerGrid::clearAllCells() {
    std::fill(cells_.begin(), cells_.end(), false);
}

void SequencerGrid::setActiveColor(const Color& color) {
    activeColor_ = color;
}

void SequencerGrid::setInactiveColor(const Color& color) {
    inactiveColor_ = color;
}

void SequencerGrid::setCursorColor(const Color& color) {
    cursorColor_ = color;
}

void SequencerGrid::setGridLineColor(const Color& color) {
    gridLineColor_ = color;
}

void SequencerGrid::setPlaybackPosition(int column) {
    playbackPosition_ = column;
}

int SequencerGrid::getPlaybackPosition() const {
    return playbackPosition_;
}

void SequencerGrid::setCellChangeCallback(CellChangeCallback callback) {
    cellChangeCallback_ = callback;
}

void SequencerGrid::update(float deltaTime) {
    // No continuous updates needed
}

void SequencerGrid::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Calculate cell dimensions
    float cellWidth = static_cast<float>(width_) / columns_;
    float cellHeight = static_cast<float>(height_) / rows_;
    
    // Draw grid cells
    for (int row = 0; row < rows_; row++) {
        for (int col = 0; col < columns_; col++) {
            int cellX = x_ + static_cast<int>(col * cellWidth);
            int cellY = y_ + static_cast<int>(row * cellHeight);
            int cellW = static_cast<int>(cellWidth);
            int cellH = static_cast<int>(cellHeight);
            
            // Determine cell color
            Color cellColor = isCellActive(row, col) ? activeColor_ : inactiveColor_;
            
            // Highlight current playback position
            if (col == playbackPosition_) {
                // Blend with playback indicator color
                cellColor.r = (cellColor.r + 100) / 2;
                cellColor.g = (cellColor.g + 100) / 2;
                cellColor.b = (cellColor.b + 255) / 2;
            }
            
            // Highlight cursor position
            if (row == cursorRow_ && col == cursorColumn_) {
                // Draw cell
                display->fillRect(cellX, cellY, cellW, cellH, cellColor);
                
                // Draw cursor highlight (inner rectangle)
                int innerMargin = 2;
                display->drawRect(cellX + innerMargin, cellY + innerMargin, 
                               cellW - 2 * innerMargin, cellH - 2 * innerMargin, 
                               cursorColor_);
            } else {
                // Draw normal cell
                display->fillRect(cellX, cellY, cellW, cellH, cellColor);
            }
            
            // Draw cell border
            display->drawRect(cellX, cellY, cellW, cellH, gridLineColor_);
        }
    }
    
    // Draw outer border
    display->drawRect(x_, y_, width_, height_, gridLineColor_);
    
    // Render children
    renderChildren(display);
}

bool SequencerGrid::handleInput(const InputEvent& event) {
    bool handled = false;
    
    // Calculate cell dimensions
    float cellWidth = static_cast<float>(width_) / columns_;
    float cellHeight = static_cast<float>(height_) / rows_;
    
    // Handle touch input
    if (event.type == InputEventType::TouchPress) {
        // Check if touch is within grid bounds
        if (event.value >= x_ && event.value < x_ + width_ &&
            event.value2 >= y_ && event.value2 < y_ + height_) {
            
            // Calculate which cell was touched
            int col = static_cast<int>((event.value - x_) / cellWidth);
            int row = static_cast<int>((event.value2 - y_) / cellHeight);
            
            // Move cursor to this cell
            cursorRow_ = row;
            cursorColumn_ = col;
            
            // Toggle cell state
            toggleCurrentCell();
            
            handled = true;
        }
    }
    // Handle button/keyboard navigation
    else if (event.type == InputEventType::ButtonPress) {
        // Simple arrow key navigation (assuming event.id encodes direction)
        if (event.id == 0) { // Up
            moveCursor(-1, 0);
            handled = true;
        } else if (event.id == 1) { // Down
            moveCursor(1, 0);
            handled = true;
        } else if (event.id == 2) { // Left
            moveCursor(0, -1);
            handled = true;
        } else if (event.id == 3) { // Right
            moveCursor(0, 1);
            handled = true;
        } else if (event.id == 4) { // Enter/Space/Toggle
            toggleCurrentCell();
            handled = true;
        }
    }
    
    if (!handled) {
        handled = handleChildrenInput(event);
    }
    
    return handled;
}

int SequencerGrid::getCellIndex(int row, int column) const {
    return row * columns_ + column;
}

void SequencerGrid::moveCursor(int deltaRow, int deltaColumn) {
    // Calculate new cursor position
    cursorRow_ = (cursorRow_ + deltaRow) % rows_;
    if (cursorRow_ < 0) cursorRow_ += rows_;
    
    cursorColumn_ = (cursorColumn_ + deltaColumn) % columns_;
    if (cursorColumn_ < 0) cursorColumn_ += columns_;
}

void SequencerGrid::toggleCurrentCell() {
    bool currentState = isCellActive(cursorRow_, cursorColumn_);
    setCellActive(cursorRow_, cursorColumn_, !currentState);
}

//
// IconSelector implementation
//
IconSelector::IconSelector(const std::string& id, int columns)
    : UIComponent(id), columns_(columns), selectedIndex_(0),
      selectedColor_(Color(200, 150, 0)), unselectedColor_(Color(150, 150, 150)),
      backgroundColor_(Color(30, 30, 30)), selectionChangeCallback_(nullptr) {
}

IconSelector::~IconSelector() {
}

void IconSelector::addIcon(int iconCode, const std::string& tooltip) {
    IconItem item;
    item.iconCode = iconCode;
    item.tooltip = tooltip;
    icons_.push_back(item);
}

void IconSelector::clearIcons() {
    icons_.clear();
    selectedIndex_ = 0;
}

int IconSelector::getSelectedIconIndex() const {
    return selectedIndex_;
}

void IconSelector::setSelectedIconIndex(int index) {
    if (index >= 0 && index < static_cast<int>(icons_.size())) {
        selectedIndex_ = index;
        
        // Notify callback
        if (selectionChangeCallback_) {
            selectionChangeCallback_(selectedIndex_);
        }
    }
}

void IconSelector::setSelectedColor(const Color& color) {
    selectedColor_ = color;
}

void IconSelector::setUnselectedColor(const Color& color) {
    unselectedColor_ = color;
}

void IconSelector::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void IconSelector::setSelectionChangeCallback(SelectionChangeCallback callback) {
    selectionChangeCallback_ = callback;
}

void IconSelector::update(float deltaTime) {
    // No continuous updates needed
}

void IconSelector::render(DisplayManager* display) {
    if (!display || icons_.empty()) {
        return;
    }
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    
    // Calculate grid layout
    int rows = (icons_.size() + columns_ - 1) / columns_; // Ceiling division
    float cellWidth = static_cast<float>(width_) / columns_;
    float cellHeight = static_cast<float>(height_) / rows;
    
    // Draw icons
    for (size_t i = 0; i < icons_.size(); i++) {
        int col = i % columns_;
        int row = i / columns_;
        
        int iconX = x_ + static_cast<int>(col * cellWidth + cellWidth / 2);
        int iconY = y_ + static_cast<int>(row * cellHeight + cellHeight / 2);
        
        // Draw selection background if this is the selected icon
        if (static_cast<int>(i) == selectedIndex_) {
            int cellX = x_ + static_cast<int>(col * cellWidth);
            int cellY = y_ + static_cast<int>(row * cellHeight);
            int cellW = static_cast<int>(cellWidth);
            int cellH = static_cast<int>(cellHeight);
            
            // Draw selection highlight
            display->fillRect(cellX, cellY, cellW, cellH, Color(60, 60, 60));
            display->drawRect(cellX, cellY, cellW, cellH, selectedColor_);
        }
        
        // Get icon font
        Font* iconFont = nullptr;
        // TODO: Get icon font from context
        
        // Draw the icon
        if (iconFont) {
            // Convert icon code to string
            char iconChar = static_cast<char>(icons_[i].iconCode);
            std::string iconText(1, iconChar);
            
            // Determine color based on selection state
            Color iconColor = (static_cast<int>(i) == selectedIndex_) ? selectedColor_ : unselectedColor_;
            
            // Draw icon
            display->drawText(iconX, iconY, iconText, iconFont, iconColor);
        } else {
            // Fallback rendering if no font is available
            int iconSize = std::min(static_cast<int>(cellWidth), static_cast<int>(cellHeight)) / 2;
            
            // Determine color based on selection state
            Color iconColor = (static_cast<int>(i) == selectedIndex_) ? selectedColor_ : unselectedColor_;
            
            // Draw a placeholder square
            display->fillRect(iconX - iconSize / 2, iconY - iconSize / 2, iconSize, iconSize, iconColor);
        }
    }
    
    // Draw outer border
    display->drawRect(x_, y_, width_, height_, unselectedColor_);
    
    // Render children
    renderChildren(display);
}

bool IconSelector::handleInput(const InputEvent& event) {
    bool handled = false;
    
    // Handle touch input
    if (event.type == InputEventType::TouchPress) {
        // Check if touch is within selector bounds
        if (event.value >= x_ && event.value < x_ + width_ &&
            event.value2 >= y_ && event.value2 < y_ + height_) {
            
            // Calculate grid layout
            int rows = (icons_.size() + columns_ - 1) / columns_; // Ceiling division
            float cellWidth = static_cast<float>(width_) / columns_;
            float cellHeight = static_cast<float>(height_) / rows;
            
            // Calculate which cell was touched
            int col = static_cast<int>((event.value - x_) / cellWidth);
            int row = static_cast<int>((event.value2 - y_) / cellHeight);
            
            // Calculate index
            int index = row * columns_ + col;
            
            // Check if valid index
            if (index >= 0 && index < static_cast<int>(icons_.size())) {
                setSelectedIconIndex(index);
                handled = true;
            }
        }
    }
    // Handle button navigation (assuming arrow keys)
    else if (event.type == InputEventType::ButtonPress) {
        if (event.id == 0) { // Up
            int newIndex = selectedIndex_ - columns_;
            if (newIndex >= 0) {
                setSelectedIconIndex(newIndex);
                handled = true;
            }
        } else if (event.id == 1) { // Down
            int newIndex = selectedIndex_ + columns_;
            if (newIndex < static_cast<int>(icons_.size())) {
                setSelectedIconIndex(newIndex);
                handled = true;
            }
        } else if (event.id == 2) { // Left
            if (selectedIndex_ > 0) {
                setSelectedIconIndex(selectedIndex_ - 1);
                handled = true;
            }
        } else if (event.id == 3) { // Right
            if (selectedIndex_ < static_cast<int>(icons_.size()) - 1) {
                setSelectedIconIndex(selectedIndex_ + 1);
                handled = true;
            }
        }
    }
    
    if (!handled) {
        handled = handleChildrenInput(event);
    }
    
    return handled;
}

//
// VUMeter implementation
//
VUMeter::VUMeter(const std::string& id, bool stereo)
    : UIComponent(id), stereo_(stereo), 
      leftLevel_(0.0f), rightLevel_(0.0f),
      leftPeakLevel_(0.0f), rightPeakLevel_(0.0f),
      peakHoldTime_(2.0f), leftPeakHoldTimer_(0.0f), rightPeakHoldTimer_(0.0f),
      backgroundColor_(Color(20, 20, 20)), 
      levelColor_(Color(0, 200, 0)), 
      peakColor_(Color(255, 0, 0)) {
}

VUMeter::~VUMeter() {
}

void VUMeter::setLevel(float level) {
    level = std::clamp(level, 0.0f, 1.0f);
    leftLevel_ = level;
    rightLevel_ = level;
    
    // Update peak levels
    if (level > leftPeakLevel_) {
        leftPeakLevel_ = level;
        leftPeakHoldTimer_ = peakHoldTime_;
    }
    
    if (level > rightPeakLevel_) {
        rightPeakLevel_ = level;
        rightPeakHoldTimer_ = peakHoldTime_;
    }
}

void VUMeter::setLevels(float leftLevel, float rightLevel) {
    leftLevel = std::clamp(leftLevel, 0.0f, 1.0f);
    rightLevel = std::clamp(rightLevel, 0.0f, 1.0f);
    
    leftLevel_ = leftLevel;
    rightLevel_ = rightLevel;
    
    // Update peak levels
    if (leftLevel > leftPeakLevel_) {
        leftPeakLevel_ = leftLevel;
        leftPeakHoldTimer_ = peakHoldTime_;
    }
    
    if (rightLevel > rightPeakLevel_) {
        rightPeakLevel_ = rightLevel;
        rightPeakHoldTimer_ = peakHoldTime_;
    }
}

float VUMeter::getLevel() const {
    return (leftLevel_ + rightLevel_) / 2.0f;
}

void VUMeter::getLevels(float& leftLevel, float& rightLevel) const {
    leftLevel = leftLevel_;
    rightLevel = rightLevel_;
}

void VUMeter::setStereo(bool stereo) {
    stereo_ = stereo;
}

bool VUMeter::isStereo() const {
    return stereo_;
}

void VUMeter::setPeakHoldTime(float seconds) {
    peakHoldTime_ = seconds;
}

float VUMeter::getPeakHoldTime() const {
    return peakHoldTime_;
}

void VUMeter::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void VUMeter::setLevelColor(const Color& color) {
    levelColor_ = color;
}

void VUMeter::setPeakColor(const Color& color) {
    peakColor_ = color;
}

void VUMeter::update(float deltaTime) {
    // Update peak hold timers
    if (leftPeakHoldTimer_ > 0.0f) {
        leftPeakHoldTimer_ -= deltaTime;
        if (leftPeakHoldTimer_ <= 0.0f) {
            leftPeakHoldTimer_ = 0.0f;
            leftPeakLevel_ = leftLevel_; // Reset to current level
        }
    }
    
    if (rightPeakHoldTimer_ > 0.0f) {
        rightPeakHoldTimer_ -= deltaTime;
        if (rightPeakHoldTimer_ <= 0.0f) {
            rightPeakHoldTimer_ = 0.0f;
            rightPeakLevel_ = rightLevel_; // Reset to current level
        }
    }
}

void VUMeter::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Draw background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    
    // Calculate meter dimensions
    if (stereo_) {
        // Stereo meter (two horizontal bars)
        int meterHeight = height_ / 2 - 2; // Half height with small gap
        
        // Left channel
        int leftMeterY = y_ + 1;
        int leftMeterWidth = static_cast<int>(width_ * leftLevel_);
        int leftPeakX = x_ + static_cast<int>(width_ * leftPeakLevel_);
        
        display->fillRect(x_, leftMeterY, leftMeterWidth, meterHeight, levelColor_);
        display->fillRect(leftPeakX - 2, leftMeterY, 2, meterHeight, peakColor_);
        
        // Right channel
        int rightMeterY = y_ + meterHeight + 3; // Position below left meter
        int rightMeterWidth = static_cast<int>(width_ * rightLevel_);
        int rightPeakX = x_ + static_cast<int>(width_ * rightPeakLevel_);
        
        display->fillRect(x_, rightMeterY, rightMeterWidth, meterHeight, levelColor_);
        display->fillRect(rightPeakX - 2, rightMeterY, 2, meterHeight, peakColor_);
    } else {
        // Mono meter (one horizontal bar)
        float level = (leftLevel_ + rightLevel_) / 2.0f;
        float peakLevel = (leftPeakLevel_ + rightPeakLevel_) / 2.0f;
        
        int meterWidth = static_cast<int>(width_ * level);
        int peakX = x_ + static_cast<int>(width_ * peakLevel);
        
        display->fillRect(x_, y_, meterWidth, height_, levelColor_);
        display->fillRect(peakX - 2, y_, 2, height_, peakColor_);
    }
    
    // Draw level graduations
    for (int i = 1; i < 10; i++) {
        int markX = x_ + (i * width_) / 10;
        display->drawLine(markX, y_, markX, y_ + 3, Color(150, 150, 150));
        display->drawLine(markX, y_ + height_ - 3, markX, y_ + height_, Color(150, 150, 150));
    }
    
    // Draw outer border
    display->drawRect(x_, y_, width_, height_, Color(150, 150, 150));
    
    // Render children
    renderChildren(display);
}

bool VUMeter::handleInput(const InputEvent& event) {
    // VU meters don't typically handle input
    return handleChildrenInput(event);
}

} // namespace AIMusicHardware