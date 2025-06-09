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
    if (!display || !visible_) {
        return;
    }
    
    // Get font to use for rendering
    // TODO: In a real implementation, this would come from UIContext
    // For now, provide a better fallback rendering approach
    Font* font = nullptr;
    
    // ALWAYS call drawText directly - our custom implementation will handle it
    
    // Calculate position based on alignment
    int textX = x_;
    int textY = y_;
    
    // For now, just draw at the label position
    // Our custom drawText will make it visible
    // Note: fontName_ will be passed to the display manager for size determination
    display->drawText(textX, textY, text_, font, textColor_);
    
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
    : UIComponent(id), label_(label), parameterId_(""), value_(0.5f), 
      minValue_(0.0f), maxValue_(1.0f), step_(0.01f), showValue_(true),
      color_(Color(200, 200, 200)), backgroundColor_(Color(40, 40, 40)),
      modulationAmount_(0.0f), modulationColor_(Color(0, 150, 255)),
      midiLearnEnabled_(false), midiControlNumber_(-1),
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

void Knob::setModulationAmount(float amount) {
    modulationAmount_ = std::clamp(amount, 0.0f, 1.0f);
}

float Knob::getModulationAmount() const {
    return modulationAmount_;
}

void Knob::setModulationColor(const Color& color) {
    modulationColor_ = color;
}

const Color& Knob::getModulationColor() const {
    return modulationColor_;
}

void Knob::setParameterId(const std::string& parameterId) {
    parameterId_ = parameterId;
}

const std::string& Knob::getParameterId() const {
    return parameterId_;
}

void Knob::setMidiLearnEnabled(bool enabled) {
    midiLearnEnabled_ = enabled;
}

bool Knob::isMidiLearnEnabled() const {
    return midiLearnEnabled_;
}

void Knob::setMidiControlNumber(int ccNumber) {
    midiControlNumber_ = ccNumber;
}

int Knob::getMidiControlNumber() const {
    return midiControlNumber_;
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
    
    // Draw modulation ring if modulation is active
    if (modulationAmount_ > 0.0f) {
        // Calculate modulation start and end angles
        float normalizedValue = (value_ - minValue_) / (maxValue_ - minValue_);
        float modulatedValue = std::clamp(normalizedValue + modulationAmount_, 0.0f, 1.0f);
        
        float startAngle = (normalizedValue * 270.0f - 135.0f) * 3.14159f / 180.0f;
        float endAngle = (modulatedValue * 270.0f - 135.0f) * 3.14159f / 180.0f;
        
        // Draw arc for modulation range - use multiple line segments since drawArc is not available
        int segments = 20;
        float angleStep = (endAngle - startAngle) / segments;
        
        for (int i = 0; i < segments; i++) {
            float angle1 = startAngle + i * angleStep;
            float angle2 = startAngle + (i + 1) * angleStep;
            
            int x1 = centerX + static_cast<int>(std::cos(angle1) * (radius - 2));
            int y1 = centerY + static_cast<int>(std::sin(angle1) * (radius - 2));
            int x2 = centerX + static_cast<int>(std::cos(angle2) * (radius - 2));
            int y2 = centerY + static_cast<int>(std::sin(angle2) * (radius - 2));
            
            display->drawLine(x1, y1, x2, y2, modulationColor_);
        }
    }
    
    // Draw knob border
    display->drawCircle(centerX, centerY, radius, color_);
    
    // Draw MIDI learn indicator if active
    if (midiLearnEnabled_) {
        // Draw pulsing highlight
        int pulseRadius = radius + 3;
        Color pulseColor = Color(255, 100, 0); // Orange pulse for MIDI learn
        display->drawCircle(centerX, centerY, pulseRadius, pulseColor);
    }
    else if (midiControlNumber_ >= 0) {
        // Draw small indicator showing this knob has MIDI mapping
        int indicatorSize = 3;
        Color mappedColor = Color(0, 255, 0); // Green dot for mapped
        display->fillCircle(x_ + indicatorSize, y_ + indicatorSize, indicatorSize, mappedColor);
    }
    
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
    
    // Always render labels and values for better visibility
    // Draw label text at bottom
    if (!label_.empty()) {
        display->drawText(centerX - label_.length() * 4, y_ + height_ + 10, label_, font, color_);
    }
    
    // Draw value text at top if enabled
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
    
    // Draw MIDI CC number if mapped
    if (midiControlNumber_ >= 0 && !midiLearnEnabled_) {
        std::ostringstream ccText;
        ccText << "CC" << midiControlNumber_;
        std::string ccString = ccText.str();
        
        display->drawText(centerX - ccString.length() * 4, y_ + height_ + 25, 
                       ccString, font, Color(0, 200, 0));
    }
    
    // Render children
    renderChildren(display);
}

bool Knob::handleInput(const InputEvent& event) {
    bool handled = false;
    
    // Used in multiple event types
    int centerX = x_ + width_ / 2;
    int centerY = y_ + height_ / 2;
    int radius = std::min(width_, height_) / 2;
    
    // Track if we're currently interacting with this knob
    static bool isDragging = false;
    static std::string activeDragKnob = "";
    
    // Handle rotary encoder input
    if (event.type == InputEventType::EncoderRotate) {
        // Calculate delta based on step size
        float delta = event.value * step_;
        
        // Update value
        setValue(value_ + delta);
        
        handled = true;
    }
    
    // Handle touch press (start dragging)
    else if (event.type == InputEventType::TouchPress) {
        int dx = event.value - centerX;
        int dy = event.value2 - centerY;
        float distance = std::sqrt(dx*dx + dy*dy);
        
        // Check if press is within knob bounds
        if (distance <= radius) {
            // Start drag operation
            isDragging = true;
            activeDragKnob = id_;
            
            // Store the initial touch position for better relative movement
            // in the TouchMove handler (this helps with logarithmic knobs like filter cutoff)
            
            // Optional: Double-click detection could toggle MIDI learn mode
            // For now, we'll use a regular click to toggle MIDI learn
            setMidiLearnEnabled(!midiLearnEnabled_);
            
            handled = true;
        }
    }
    
    // Handle touch/drag input - improved for smoother control
    else if (event.type == InputEventType::TouchMove) {
        // Only process move events if we're currently dragging this specific knob
        // This prevents erratic behavior when moving between knobs
        if (isDragging && activeDragKnob == id_) {
            int dx = event.value - centerX;
            int dy = event.value2 - centerY;
            
            // Two control modes based on the parameter:
            // 1. Angle-based for standard knobs (normal parameters)
            // 2. Vertical-drag for filter controls (makes frequency easier to control)
            
            // For filter cutoff, use a logarithmic mapping with vertical drag
            if (parameterId_ == "filter_cutoff" || label_ == "Cutoff") {
                // Use vertical position for logarithmic parameter control
                // Map vertical position to logarithmic value (better for filter cutoff)
                // Higher on screen = higher cutoff
                
                // Calculate a normalized vertical position (-1 to 1)
                float verticalPosition = std::clamp(
                    (centerY - dy) / static_cast<float>(height_) * 2.0f, 
                    -1.0f, 1.0f
                );
                
                // Use logistic mapping for smoother control
                float normalizedValue;
                if (dy < 0) {  // Above center - map to 0.5-1.0
                    normalizedValue = 0.5f + (verticalPosition * 0.5f);
                } else {       // Below center - map to 0.0-0.5
                    normalizedValue = 0.5f - (verticalPosition * -0.5f);
                }
                
                // Apply logarithmic scaling for filter cutoff
                float newValue;
                if (minValue_ > 0 && maxValue_ > minValue_) {
                    // For frequency-like parameters (20-20000Hz), use logarithmic scale
                    float logMin = std::log(minValue_);
                    float logMax = std::log(maxValue_);
                    float logValue = logMin + normalizedValue * (logMax - logMin);
                    newValue = std::exp(logValue);
                } else {
                    // Linear mapping for other parameters
                    newValue = minValue_ + normalizedValue * (maxValue_ - minValue_);
                }
                
                // Apply step quantization
                if (step_ > 0.0f) {
                    newValue = std::round(newValue / step_) * step_;
                }
                
                // Set the value
                setValue(newValue);
            } 
            else {
                // Standard angle-based control for regular knobs
                constexpr float PI = 3.14159265358979323846f;
                float angle = std::atan2(dy, dx);
                
                // Convert to degrees and normalize
                float angleDegrees = angle * 180.0f / PI;
                if (angleDegrees < 0) angleDegrees += 360.0f;
                
                // Better mapping to knob's rotation range (270 degrees)
                float normalizedAngle;
                if (angleDegrees >= 225.0f && angleDegrees <= 360.0f) {
                    // Top-left quadrant, map 225-360 to 0-0.5
                    normalizedAngle = (angleDegrees - 225.0f) / 270.0f;
                } else if (angleDegrees >= 0.0f && angleDegrees <= 135.0f) {
                    // Top-right and bottom-right quadrants, map 0-135 to 0.5-1.0
                    normalizedAngle = (angleDegrees + 135.0f) / 270.0f;
                } else {
                    // Positions outside the rotation range - maintain current
                    normalizedAngle = (value_ - minValue_) / (maxValue_ - minValue_);
                }
                
                // Make sure it's in valid range
                normalizedAngle = std::clamp(normalizedAngle, 0.0f, 1.0f);
                
                // Set value with quantization
                float newValue = minValue_ + normalizedAngle * (maxValue_ - minValue_);
                if (step_ > 0.0f) {
                    newValue = std::round(newValue / step_) * step_;
                }
                setValue(newValue);
            }
            
            handled = true;
        }
    }
    
    // Handle touch release (end dragging)
    else if (event.type == InputEventType::TouchRelease) {
        // End drag if this is the active knob
        if (isDragging && activeDragKnob == id_) {
            isDragging = false;
            activeDragKnob = "";
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
        
        // Performance optimization: Determine visible range and skip invisible sections
        // Calculate sample index range that will be visible
        size_t firstVisibleSample = 0;
        size_t lastVisibleSample = samples_.size() - 1;
        
        // Only draw samples that will actually be visible on screen
        int prevX = -1;
        int prevY = -1;
        
        for (size_t i = firstVisibleSample; i <= lastVisibleSample; i++) {
            // Calculate screen coordinates
            int x2 = x_ + static_cast<int>(i * horizontalScale);
            int y2 = centerY - static_cast<int>(samples_[i] * verticalScale);
            
            // Skip drawing if this pixel is at the same position as the previous one
            // This avoids drawing overlapping lines when there are many samples
            if (x2 != prevX || y2 != prevY) {
                if (prevX >= 0) {
                    display->drawLine(prevX, prevY, x2, y2, waveformColor_);
                }
                prevX = x2;
                prevY = y2;
            }
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

//
// ParameterBinding implementation
//
ParameterBinding::ParameterBinding(UIComponent* component, const std::string& parameterId)
    : component_(component), knobComponent_(nullptr), parameterId_(parameterId),
      value_(0.0f), modulationAmount_(0.0f), midiControlNumber_(-1) {
    
    // Check if component is a Knob and store specialized pointer if so
    if (component) {
        Knob* knob = dynamic_cast<Knob*>(component);
        if (knob) {
            knobComponent_ = knob;
            knobComponent_->setParameterId(parameterId);
        }
    }
}

ParameterBinding::~ParameterBinding() {
    // No ownership of component, so just clear reference
    component_ = nullptr;
    knobComponent_ = nullptr;
}

void ParameterBinding::connectToKnob(Knob* knob) {
    // Store component and specialized knob pointer
    component_ = knob;
    knobComponent_ = knob;
    
    if (knobComponent_) {
        // Set parameter ID in the knob
        knobComponent_->setParameterId(parameterId_);
        
        // Set MIDI mapping if available
        if (midiControlNumber_ >= 0) {
            knobComponent_->setMidiControlNumber(midiControlNumber_);
        }
        
        // Set modulation amount
        knobComponent_->setModulationAmount(modulationAmount_);
        
        // Set initial value
        knobComponent_->setValue(value_);
    }
}

void ParameterBinding::updateUI() {
    // Update UI component based on current parameter value
    if (knobComponent_) {
        knobComponent_->setValue(value_);
        knobComponent_->setModulationAmount(modulationAmount_);
        knobComponent_->setMidiControlNumber(midiControlNumber_);
    }
}

void ParameterBinding::updateParameter() {
    // Update parameter value based on UI component
    if (knobComponent_) {
        value_ = knobComponent_->getValue();
    }
}

void ParameterBinding::setValue(float value) {
    value_ = value;
    updateUI();
}

float ParameterBinding::getValue() const {
    return value_;
}

void ParameterBinding::setModulationAmount(float amount) {
    modulationAmount_ = std::clamp(amount, 0.0f, 1.0f);
    
    if (knobComponent_) {
        knobComponent_->setModulationAmount(modulationAmount_);
    }
}

float ParameterBinding::getModulationAmount() const {
    return modulationAmount_;
}

const std::string& ParameterBinding::getParameterId() const {
    return parameterId_;
}

void ParameterBinding::setMidiControlNumber(int ccNumber) {
    midiControlNumber_ = ccNumber;
    
    if (knobComponent_) {
        knobComponent_->setMidiControlNumber(midiControlNumber_);
        // Turn off MIDI learn mode if it was active
        knobComponent_->setMidiLearnEnabled(false);
    }
}

int ParameterBinding::getMidiControlNumber() const {
    return midiControlNumber_;
}

void ParameterBinding::clearMidiMapping() {
    midiControlNumber_ = -1;
    
    if (knobComponent_) {
        knobComponent_->setMidiControlNumber(-1);
    }
}

//
// ParameterPanel implementation
//
ParameterPanel::ParameterPanel(const std::string& id, const std::string& title)
    : UIComponent(id), title_(title), columns_(4), rows_(4),
      cellWidth_(80), cellHeight_(100),
      backgroundColor_(Color(30, 30, 30)), 
      titleColor_(Color(200, 200, 200)),
      borderColor_(Color(100, 100, 100)) {
}

ParameterPanel::~ParameterPanel() {
    // Parameters will be automatically cleaned up via unique_ptr
}

void ParameterPanel::setTitle(const std::string& title) {
    title_ = title;
}

const std::string& ParameterPanel::getTitle() const {
    return title_;
}

void ParameterPanel::setGridSize(int columns, int rows) {
    columns_ = std::max(1, columns);
    rows_ = std::max(1, rows);
    updateLayout();
}

void ParameterPanel::getGridSize(int& columns, int& rows) const {
    columns = columns_;
    rows = rows_;
}

Knob* ParameterPanel::addParameter(const std::string& parameterId, const std::string& label,
                                  float minValue, float maxValue, float defaultValue,
                                  int gridX, int gridY) {
    // Check if parameter with this ID already exists
    if (findParameter(parameterId)) {
        return nullptr; // Parameter already exists
    }
    
    // Create parameter item
    ParameterItem item;
    item.parameterId = parameterId;
    item.label = label;
    item.gridX = std::min(gridX, columns_ - 1);
    item.gridY = std::min(gridY, rows_ - 1);
    
    // Create knob with a unique ID based on parameter ID
    std::string knobId = getId() + "_" + parameterId;
    item.knob = new Knob(knobId, label);
    item.knob->setRange(minValue, maxValue);
    item.knob->setValue(defaultValue);
    
    // Create binding
    item.binding = std::make_unique<ParameterBinding>(item.knob, parameterId);
    
    // Add knob as child component - since we don't have ownership transfer with unique_ptr in this case,
    // the ParameterPanel will retain direct ownership of the knob pointers
    children_.push_back(std::unique_ptr<UIComponent>(item.knob));
    
    // Store in parameters list
    parameters_.push_back(std::move(item));
    
    // Update layout
    updateLayout();
    
    // Return pointer to the knob
    return parameters_.back().knob;
}

Knob* ParameterPanel::getParameterKnob(const std::string& parameterId) {
    ParameterItem* item = findParameter(parameterId);
    return item ? item->knob : nullptr;
}

ParameterBinding* ParameterPanel::getParameterBinding(const std::string& parameterId) {
    ParameterItem* item = findParameter(parameterId);
    return item ? item->binding.get() : nullptr;
}

std::vector<ParameterBinding*> ParameterPanel::getAllParameterBindings() {
    std::vector<ParameterBinding*> bindings;
    bindings.reserve(parameters_.size());
    
    for (auto& param : parameters_) {
        bindings.push_back(param.binding.get());
    }
    
    return bindings;
}

void ParameterPanel::enableMidiLearnForParameter(const std::string& parameterId) {
    // Disable MIDI learn for all parameters first
    disableMidiLearnForAllParameters();
    
    // Enable for the specified parameter
    ParameterItem* item = findParameter(parameterId);
    if (item && item->knob) {
        item->knob->setMidiLearnEnabled(true);
    }
}

void ParameterPanel::disableMidiLearnForAllParameters() {
    for (auto& param : parameters_) {
        if (param.knob) {
            param.knob->setMidiLearnEnabled(false);
        }
    }
}

void ParameterPanel::setMidiMapping(const std::string& parameterId, int ccNumber) {
    ParameterItem* item = findParameter(parameterId);
    if (item && item->binding) {
        item->binding->setMidiControlNumber(ccNumber);
    }
}

void ParameterPanel::clearAllMidiMappings() {
    for (auto& param : parameters_) {
        if (param.binding) {
            param.binding->clearMidiMapping();
        }
    }
}

void ParameterPanel::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void ParameterPanel::setTitleColor(const Color& color) {
    titleColor_ = color;
}

void ParameterPanel::setBorderColor(const Color& color) {
    borderColor_ = color;
}

void ParameterPanel::update(float deltaTime) {
    // Update children (knobs)
    for (auto& child : children_) {
        if (child) {
            child->update(deltaTime);
        }
    }
}

void ParameterPanel::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Draw panel background
    display->fillRect(x_, y_, width_, height_, backgroundColor_);
    display->drawRect(x_, y_, width_, height_, borderColor_);
    
    // Draw title if we have one
    if (!title_.empty()) {
        // Get font for rendering
        Font* font = nullptr;
        // TODO: Get font from context
        
        if (font) {
            // Draw title at top of panel
            display->drawText(x_ + 10, y_ + 15, title_, font, titleColor_);
            
            // Draw horizontal line under title
            display->drawLine(x_ + 5, y_ + 25, x_ + width_ - 5, y_ + 25, borderColor_);
        } else {
            // Fallback rendering for title
            display->fillRect(x_ + 5, y_ + 5, width_ - 10, 20, titleColor_);
        }
    }
    
    // Render children (knobs)
    renderChildren(display);
}

bool ParameterPanel::handleInput(const InputEvent& event) {
    // Let children (knobs) handle input first
    if (handleChildrenInput(event)) {
        return true;
    }
    
    // Handle specific panel input if needed
    return false;
}

void ParameterPanel::updateLayout() {
    // Calculate cell dimensions
    cellWidth_ = width_ / columns_;
    cellHeight_ = height_ / rows_;
    
    // Adjust for title area if we have a title
    int titleHeight = title_.empty() ? 0 : 30;
    int contentHeight = height_ - titleHeight;
    cellHeight_ = contentHeight / rows_;
    
    // Position all knobs according to their grid coordinates
    for (auto& param : parameters_) {
        if (param.knob) {
            // Calculate position within grid
            int knobX = x_ + param.gridX * cellWidth_;
            int knobY = y_ + titleHeight + param.gridY * cellHeight_;
            
            // Set knob size and position
            int knobSize = std::min(cellWidth_ - 20, cellHeight_ - 40);
            param.knob->setPosition(knobX + (cellWidth_ - knobSize) / 2, knobY + 10);
            param.knob->setSize(knobSize, knobSize);
        }
    }
}

ParameterPanel::ParameterItem* ParameterPanel::findParameter(const std::string& parameterId) {
    for (auto& param : parameters_) {
        if (param.parameterId == parameterId) {
            return &param;
        }
    }
    return nullptr;
}

//
// TabView implementation
//
TabView::TabView(const std::string& id)
    : UIComponent(id), activeTabIndex_(0),
      tabBackgroundColor_(Color(50, 50, 50)),
      tabActiveColor_(Color(80, 80, 100)),
      tabTextColor_(Color(220, 220, 220)),
      contentBackgroundColor_(Color(40, 40, 40)) {
}

TabView::~TabView() {
    // Child components are cleaned up by UIComponent
}

void TabView::addTab(const std::string& tabId, const std::string& tabName) {
    // Check if tab already exists
    if (findTabIndex(tabId) >= 0) {
        return; // Tab already exists
    }
    
    // Create new tab
    Tab newTab;
    newTab.id = tabId;
    newTab.name = tabName;
    
    // Add tab to list
    tabs_.push_back(newTab);
    
    // If this is the first tab, make it active
    if (tabs_.size() == 1) {
        activeTabIndex_ = 0;
    }
    
    // Update layout
    updateTabLayout();
    updateContentVisibility();
}

void TabView::addComponentToTab(const std::string& tabId, UIComponent* component) {
    if (!component) {
        return;
    }
    
    // Find tab
    int tabIndex = findTabIndex(tabId);
    if (tabIndex < 0) {
        return; // Tab not found
    }
    
    // Add component to tab
    tabs_[tabIndex].components.push_back(component);
    
    // In this implementation, the TabView just maintains a list of UIComponent pointers
    // but does not own them through the UIComponent's children_ vector.
    // Instead, it handles them directly in render and update methods.
    // This avoids ownership conflicts with the caller.
    
    // Update visibility based on active tab
    updateContentVisibility();
}

void TabView::removeComponentFromTab(const std::string& tabId, UIComponent* component) {
    if (!component) {
        return;
    }
    
    // Find tab
    int tabIndex = findTabIndex(tabId);
    if (tabIndex < 0) {
        return; // Tab not found
    }
    
    // Remove component from tab
    auto& components = tabs_[tabIndex].components;
    components.erase(std::remove(components.begin(), components.end(), component), components.end());
    
    // Since we don't store components in children_ vector anymore, 
    // we don't need to remove them from there
}

void TabView::clearTabContent(const std::string& tabId) {
    // Find tab
    int tabIndex = findTabIndex(tabId);
    if (tabIndex < 0) {
        return; // Tab not found
    }
    
    // Simply clear the components list
    // We don't own these components, so we don't delete them
    // The caller is responsible for component lifecycle management
    tabs_[tabIndex].components.clear();
}

int TabView::getTabCount() const {
    return static_cast<int>(tabs_.size());
}

std::string TabView::getTabId(int index) const {
    if (index >= 0 && index < static_cast<int>(tabs_.size())) {
        return tabs_[index].id;
    }
    return "";
}

std::string TabView::getTabName(int index) const {
    if (index >= 0 && index < static_cast<int>(tabs_.size())) {
        return tabs_[index].name;
    }
    return "";
}

void TabView::setTabName(const std::string& tabId, const std::string& newName) {
    int tabIndex = findTabIndex(tabId);
    if (tabIndex >= 0) {
        tabs_[tabIndex].name = newName;
    }
}

void TabView::setActiveTab(const std::string& tabId) {
    int tabIndex = findTabIndex(tabId);
    if (tabIndex >= 0 && tabIndex != activeTabIndex_) {
        activeTabIndex_ = tabIndex;
        updateContentVisibility();
    }
}

std::string TabView::getActiveTabId() const {
    if (activeTabIndex_ >= 0 && activeTabIndex_ < static_cast<int>(tabs_.size())) {
        return tabs_[activeTabIndex_].id;
    }
    return "";
}

int TabView::getActiveTabIndex() const {
    return activeTabIndex_;
}

void TabView::nextTab() {
    if (tabs_.empty()) {
        return;
    }
    
    activeTabIndex_ = (activeTabIndex_ + 1) % tabs_.size();
    updateContentVisibility();
}

void TabView::previousTab() {
    if (tabs_.empty()) {
        return;
    }
    
    activeTabIndex_ = (activeTabIndex_ + tabs_.size() - 1) % tabs_.size();
    updateContentVisibility();
}

void TabView::setTabBackgroundColor(const Color& color) {
    tabBackgroundColor_ = color;
}

void TabView::setTabActiveColor(const Color& color) {
    tabActiveColor_ = color;
}

void TabView::setTabTextColor(const Color& color) {
    tabTextColor_ = color;
}

void TabView::setContentBackgroundColor(const Color& color) {
    contentBackgroundColor_ = color;
}

void TabView::update(float deltaTime) {
    // Update all tab components, regardless of active tab
    // This ensures animations continue even on hidden tabs
    for (auto& tab : tabs_) {
        for (auto* component : tab.components) {
            if (component) {
                component->update(deltaTime);
            }
        }
    }
}

void TabView::render(DisplayManager* display) {
    if (!display) {
        return;
    }
    
    // Draw content background
    display->fillRect(x_, y_ + 30, width_, height_ - 30, contentBackgroundColor_);
    
    // Draw tab bar background
    display->fillRect(x_, y_, width_, 30, tabBackgroundColor_);
    
    // Draw tabs
    if (!tabs_.empty()) {
        int tabWidth = width_ / std::min(static_cast<int>(tabs_.size()), 6);
        int tabHeight = 30;
        
        // Draw up to 6 tabs at a time
        int startTab = 0;
        if (tabs_.size() > 6) {
            // If active tab would be outside visible range, adjust start tab
            if (activeTabIndex_ >= startTab + 6) {
                startTab = std::max(0, activeTabIndex_ - 5);
            }
        }
        
        for (int i = startTab; i < std::min(startTab + 6, static_cast<int>(tabs_.size())); i++) {
            int tabX = x_ + (i - startTab) * tabWidth;
            int tabY = y_;
            
            // Draw tab background (different color for active tab)
            Color tabColor = (i == activeTabIndex_) ? tabActiveColor_ : tabBackgroundColor_;
            display->fillRect(tabX, tabY, tabWidth, tabHeight, tabColor);
            
            // Draw tab border
            display->drawRect(tabX, tabY, tabWidth, tabHeight, Color(100, 100, 100));
            
            // Draw tab text
            Font* font = nullptr;
            // TODO: Get font from context
            
            if (font) {
                // Center text in tab
                int textWidth = 0;
                int textHeight = 0;
                font->getTextDimensions(tabs_[i].name, textWidth, textHeight);
                
                int textX = tabX + (tabWidth - textWidth) / 2;
                int textY = tabY + (tabHeight - textHeight) / 2;
                
                display->drawText(textX, textY, tabs_[i].name, font, tabTextColor_);
            } else {
                // Fallback rendering - just draw a colored rectangle for active tab
                if (i == activeTabIndex_) {
                    display->fillRect(tabX + 5, tabY + tabHeight - 5, tabWidth - 10, 3, tabTextColor_);
                }
            }
        }
        
        // If we have more tabs than can fit, show navigation arrows
        if (tabs_.size() > 6) {
            // Left arrow
            if (startTab > 0) {
                display->fillTriangle(x_ + 10, y_ + 15, 
                                   x_ + 20, y_ + 5, 
                                   x_ + 20, y_ + 25, 
                                   tabTextColor_);
            }
            
            // Right arrow
            if (startTab + 6 < static_cast<int>(tabs_.size())) {
                display->fillTriangle(x_ + width_ - 10, y_ + 15, 
                                   x_ + width_ - 20, y_ + 5, 
                                   x_ + width_ - 20, y_ + 25, 
                                   tabTextColor_);
            }
        }
    }
    
    // Render components on the active tab
    if (activeTabIndex_ >= 0 && activeTabIndex_ < static_cast<int>(tabs_.size())) {
        for (auto* component : tabs_[activeTabIndex_].components) {
            if (component && component->isVisible()) {
                component->render(display);
            }
        }
    }
}

bool TabView::handleInput(const InputEvent& event) {
    // First check if tab bar was clicked
    if (event.type == InputEventType::TouchPress &&
        event.value >= x_ && event.value < x_ + width_ &&
        event.value2 >= y_ && event.value2 < y_ + 30) {
        
        if (!tabs_.empty()) {
            int tabWidth = width_ / std::min(static_cast<int>(tabs_.size()), 6);
            
            // Calculate which tab was clicked
            int clickedTab = (event.value - x_) / tabWidth;
            
            // Adjust for scrolling if we have more than 6 tabs
            int startTab = 0;
            if (tabs_.size() > 6) {
                if (activeTabIndex_ >= startTab + 6) {
                    startTab = std::max(0, activeTabIndex_ - 5);
                }
                
                // Check if arrows were clicked
                if (event.value < x_ + 20 && startTab > 0) {
                    // Left arrow clicked
                    startTab--;
                    return true;
                } else if (event.value > x_ + width_ - 20 && startTab + 6 < static_cast<int>(tabs_.size())) {
                    // Right arrow clicked
                    startTab++;
                    return true;
                }
            }
            
            // Adjust clicked tab for scrolling
            clickedTab += startTab;
            
            // Switch to the clicked tab if it's valid
            if (clickedTab >= 0 && clickedTab < static_cast<int>(tabs_.size())) {
                setActiveTab(tabs_[clickedTab].id);
                return true;
            }
        }
    }
    
    // Let components on the active tab handle input
    if (activeTabIndex_ >= 0 && activeTabIndex_ < static_cast<int>(tabs_.size())) {
        for (auto* component : tabs_[activeTabIndex_].components) {
            if (component && component->isVisible() && component->isEnabled()) {
                if (component->handleInput(event)) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

int TabView::findTabIndex(const std::string& tabId) const {
    for (size_t i = 0; i < tabs_.size(); i++) {
        if (tabs_[i].id == tabId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void TabView::updateTabLayout() {
    // No specific layout updates needed for tabs currently
}

void TabView::updateContentVisibility() {
    // Hide all components first
    for (int i = 0; i < static_cast<int>(tabs_.size()); i++) {
        for (auto* component : tabs_[i].components) {
            component->setVisible(false);
        }
    }
    
    // Show components of active tab
    if (activeTabIndex_ >= 0 && activeTabIndex_ < static_cast<int>(tabs_.size())) {
        for (auto* component : tabs_[activeTabIndex_].components) {
            component->setVisible(true);
        }
    }
}

} // namespace AIMusicHardware