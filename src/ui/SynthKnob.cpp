#include "../../include/ui/SynthKnob.h"
#include "../../include/ui/ParameterUpdateQueue.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace AIMusicHardware {

SynthKnob::SynthKnob(const std::string& label, int x, int y, int size,
                     float minValue, float maxValue, float defaultValue)
    : Knob(label + "_knob", label)  // id and label
    , defaultValue_(defaultValue)
    , animatedValue_(defaultValue)
    , lastInteractionTime_(std::chrono::steady_clock::now()) {
    
    // Set position and size
    x_ = x;
    y_ = y;
    width_ = size;
    height_ = size;
    
    // Set range
    setRange(minValue, maxValue);
    
    // Normalize default value
    defaultValue_ = (defaultValue - minValue) / (maxValue - minValue);
    animatedValue_ = defaultValue_;
    setValue(defaultValue_);
}

SynthKnob::~SynthKnob() = default;

void SynthKnob::bindToParameter(Parameter* parameter, ParameterBridge::ScaleType scaleType) {
    if (!parameter) {
        parameterBridge_.reset();
        return;
    }
    
    // Create parameter bridge
    parameterBridge_ = ParameterBridgeManager::getInstance().createBridge(parameter, scaleType);
    
    if (parameterBridge_) {
        // Bind this control to the bridge
        parameterBridge_->bindControl(this);
        
        // Set initial value from parameter
        setValue(parameterBridge_->getNormalizedValue());
        
        // Add listener for external parameter changes
        parameterBridge_->addValueChangeListener(
            [this](float value, ParameterBridge::ChangeSource source) {
                if (source != ParameterBridge::ChangeSource::UI) {
                    // Update knob value if change came from elsewhere
                    setValue(parameterBridge_->toNormalized(value));
                    
                    // Show automation indicator if from automation
                    if (source == ParameterBridge::ChangeSource::Automation) {
                        setAutomated(true);
                    }
                }
            }
        );
    }
}

void SynthKnob::setModulationAmount(float amount) {
    modulationAmount_ = std::clamp(amount, -1.0f, 1.0f);
}

void SynthKnob::setFineControlEnabled(bool enable) {
    fineControlEnabled_ = enable;
}

void SynthKnob::setFineControlMultiplier(float multiplier) {
    fineControlMultiplier_ = std::clamp(multiplier, 0.01f, 1.0f);
}

bool SynthKnob::handleInput(const InputEvent& event) {
    // Check for fine control modifier (e.g., shift key)
    if (event.type == InputEventType::ButtonPress && event.id == 1073742049) { // SDLK_LSHIFT
        fineControlActive_ = true;
        return true;
    } else if (event.type == InputEventType::ButtonRelease && event.id == 1073742049) {
        fineControlActive_ = false;
        return true;
    }
    
    // Update last interaction time
    if (event.type == InputEventType::TouchPress || 
        event.type == InputEventType::TouchMove) {
        lastInteractionTime_ = std::chrono::steady_clock::now();
    }
    
    // Handle base knob input with fine control adjustment
    // Fine control is handled differently now
    // We'll process it in the base class handleInput
    
    // Double-click to reset to default
    static auto lastClickTime = std::chrono::steady_clock::time_point();
    bool inBounds = (event.value >= x_ && event.value < x_ + width_ &&
                     event.value2 >= y_ && event.value2 < y_ + height_);
    if (event.type == InputEventType::TouchPress && inBounds) {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastClickTime).count();
        
        if (timeSinceLastClick < 300) { // Double-click detected
            resetToDefault();
            lastClickTime = std::chrono::steady_clock::time_point(); // Reset
            return true;
        }
        
        lastClickTime = now;
    }
    
    return Knob::handleInput(event);
}

void SynthKnob::update(float deltaTime) {
    // Update animation
    if (animateValueChanges_) {
        updateAnimation(deltaTime);
    }
    
    // Update tooltip visibility
    updateTooltipVisibility(deltaTime);
    
    // Clear automation flag after a brief period
    if (isAutomated_) {
        static float automationTimer = 0.0f;
        automationTimer += deltaTime;
        if (automationTimer > 2.0f) {
            isAutomated_ = false;
            automationTimer = 0.0f;
        }
    }
}

void SynthKnob::render(DisplayManager* displayManager) {
    // Base knob rendering
    Knob::render(displayManager);
    
    // Get knob center for additional drawing
    int centerX = x_ + width_ / 2;
    int centerY = y_ + height_ / 2;
    
    // Draw additional enhancements
    drawKnob(displayManager, centerX, centerY);
}

void SynthKnob::resetToDefault() {
    setValue(defaultValue_);
    
    // Trigger animation
    if (animateValueChanges_) {
        animationStartValue_ = animatedValue_;
        animationTargetValue_ = defaultValue_;
        animationProgress_ = 0.0f;
    }
}

std::string SynthKnob::getValueDisplayString() const {
    if (parameterBridge_) {
        return parameterBridge_->getDisplayString();
    }
    
    // Default formatting
    std::stringstream ss;
    float minVal, maxVal;
    getRange(minVal, maxVal);
    float actualValue = minVal + (maxVal - minVal) * getValue();
    
    if (std::abs(maxVal - minVal) < 10.0f) {
        ss << std::fixed << std::setprecision(2) << actualValue;
    } else {
        ss << std::fixed << std::setprecision(1) << actualValue;
    }
    
    return ss.str();
}

void SynthKnob::onValueChanged() {
    // Update parameter through bridge
    if (parameterBridge_) {
        parameterBridge_->setValueFromUI(getValue(), ParameterBridge::ChangeSource::UI);
    }
    
    // Trigger animation
    if (animateValueChanges_) {
        animationStartValue_ = animatedValue_;
        animationTargetValue_ = getValue();
        animationProgress_ = 0.0f;
    } else {
        // Immediate update
        animatedValue_ = getValue();
    }
}

void SynthKnob::drawKnob(DisplayManager* displayManager, int centerX, int centerY) {
    // This is now a helper method, not an override
    // Base knob drawing is handled by Knob::render()
    
    // Draw modulation ring
    if (std::abs(modulationAmount_) > 0.01f) {
        drawModulationRing(displayManager, centerX, centerY);
    }
    
    // Draw automation indicator
    if (isAutomated_) {
        drawAutomationIndicator(displayManager, centerX, centerY);
    }
    
    // Draw fine control indicator
    if (fineControlActive_) {
        drawFineControlIndicator(displayManager, centerX, centerY);
    }
    
    // Draw value tooltip
    if (showValueTooltip_ && tooltipAlpha_ > 0.01f) {
        drawValueTooltip(displayManager);
    }
}

void SynthKnob::drawModulationRing(DisplayManager* displayManager, int centerX, int centerY) {
    int radius = width_ / 2 + MODULATION_RING_WIDTH + 2;
    
    // Calculate modulation arc
    float baseAngle = calculateKnobAngle(animatedValue_);
    float modAngle = baseAngle + (modulationAmount_ * 2.0f * M_PI * 0.75f);
    
    // Draw modulation arc
    Color modColor = modulationColor_;
    modColor.a = static_cast<uint8_t>(200 * std::abs(modulationAmount_));
    
    // Simple arc approximation with lines
    const int segments = 32;
    float startAngle = std::min(baseAngle, modAngle);
    float endAngle = std::max(baseAngle, modAngle);
    
    for (int i = 0; i < segments; ++i) {
        float t1 = static_cast<float>(i) / segments;
        float t2 = static_cast<float>(i + 1) / segments;
        
        float angle1 = startAngle + t1 * (endAngle - startAngle);
        float angle2 = startAngle + t2 * (endAngle - startAngle);
        
        int x1 = centerX + static_cast<int>(radius * std::cos(angle1));
        int y1 = centerY + static_cast<int>(radius * std::sin(angle1));
        int x2 = centerX + static_cast<int>(radius * std::cos(angle2));
        int y2 = centerY + static_cast<int>(radius * std::sin(angle2));
        
        displayManager->drawLine(x1, y1, x2, y2, modColor);
    }
}

void SynthKnob::drawValueTooltip(DisplayManager* displayManager) {
    std::string valueStr = getValueDisplayString();
    
    // Calculate tooltip position (above the knob)
    int tooltipX = x_ + width_ / 2;
    int tooltipY = y_ - 25;
    
    // Estimate text width (rough approximation)
    int textWidth = static_cast<int>(valueStr.length() * 8);
    int tooltipWidth = textWidth + 16;
    int tooltipHeight = 20;
    
    // Draw tooltip background
    Color bgColor(40, 40, 40, static_cast<uint8_t>(200 * tooltipAlpha_));
    displayManager->fillRect(tooltipX - tooltipWidth / 2, tooltipY, 
                            tooltipWidth, tooltipHeight, bgColor);
    
    // Draw tooltip border
    Color borderColor(100, 100, 100, static_cast<uint8_t>(255 * tooltipAlpha_));
    displayManager->drawRect(tooltipX - tooltipWidth / 2, tooltipY, 
                            tooltipWidth, tooltipHeight, borderColor);
    
    // Draw value text
    Color textColor(255, 255, 255, static_cast<uint8_t>(255 * tooltipAlpha_));
    displayManager->drawText(tooltipX - textWidth / 2, tooltipY + 2, 
                            valueStr, nullptr, textColor);
}

void SynthKnob::drawAutomationIndicator(DisplayManager* displayManager, int centerX, int centerY) {
    // Draw small dot to indicate automation
    int indicatorX = centerX + width_ / 2 - AUTOMATION_INDICATOR_SIZE;
    int indicatorY = centerY - height_ / 2;
    
    Color automationColor(255, 100, 100); // Red for automation
    displayManager->fillRect(indicatorX, indicatorY, 
                            AUTOMATION_INDICATOR_SIZE, AUTOMATION_INDICATOR_SIZE, 
                            automationColor);
}

void SynthKnob::drawFineControlIndicator(DisplayManager* displayManager, int centerX, int centerY) {
    // Draw "FINE" text near the knob
    Color fineColor(100, 200, 255); // Light blue
    displayManager->drawText(centerX - 15, centerY + height_ / 2 + 5, 
                            "FINE", nullptr, fineColor);
}

float SynthKnob::calculateKnobAngle(float normalizedValue) const {
    // Convert normalized value to angle (270-degree sweep)
    const float startAngle = -225.0f * M_PI / 180.0f;
    const float sweepAngle = 270.0f * M_PI / 180.0f;
    return startAngle + normalizedValue * sweepAngle;
}

void SynthKnob::updateAnimation(float deltaTime) {
    if (animationProgress_ < 1.0f) {
        animationProgress_ += deltaTime / animationDuration_;
        animationProgress_ = std::min(1.0f, animationProgress_);
        
        // Smooth interpolation (ease-out cubic)
        float t = animationProgress_;
        float easedT = 1.0f - std::pow(1.0f - t, 3.0f);
        
        animatedValue_ = animationStartValue_ + 
                        (animationTargetValue_ - animationStartValue_) * easedT;
    }
}

void SynthKnob::updateTooltipVisibility(float deltaTime) {
    if (isRecentlyInteracted()) {
        // Fade in tooltip
        tooltipAlpha_ = std::min(1.0f, tooltipAlpha_ + deltaTime / TOOLTIP_FADE_TIME);
    } else {
        // Fade out tooltip
        tooltipAlpha_ = std::max(0.0f, tooltipAlpha_ - deltaTime / TOOLTIP_FADE_TIME);
    }
}

bool SynthKnob::isRecentlyInteracted() const {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceInteraction = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastInteractionTime_).count() / 1000.0f;
    return timeSinceInteraction < 1.0f; // Show for 1 second after interaction
}

// SynthKnobFactory implementations

std::unique_ptr<SynthKnob> SynthKnobFactory::createFrequencyKnob(
    const std::string& label, int x, int y, int size) {
    
    auto knob = std::make_unique<SynthKnob>(label, x, y, size, 20.0f, 20000.0f, 1000.0f);
    knob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 1000.0f) {
            ss << std::fixed << std::setprecision(1) << value << " Hz";
        } else {
            ss << std::fixed << std::setprecision(2) << (value / 1000.0f) << " kHz";
        }
        return ss.str();
    });
    return knob;
}

std::unique_ptr<SynthKnob> SynthKnobFactory::createResonanceKnob(
    const std::string& label, int x, int y, int size) {
    
    auto knob = std::make_unique<SynthKnob>(label, x, y, size, 0.0f, 1.0f, 0.0f);
    knob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << (value * 100.0f) << "%";
        return ss.str();
    });
    return knob;
}

std::unique_ptr<SynthKnob> SynthKnobFactory::createVolumeKnob(
    const std::string& label, int x, int y, int size) {
    
    auto knob = std::make_unique<SynthKnob>(label, x, y, size, 0.0f, 1.0f, 0.75f);
    knob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value <= 0.0f) {
            ss << "-∞ dB";
        } else {
            float db = 20.0f * std::log10(value);
            ss << std::fixed << std::setprecision(1) << db << " dB";
        }
        return ss.str();
    });
    return knob;
}

std::unique_ptr<SynthKnob> SynthKnobFactory::createPanKnob(
    const std::string& label, int x, int y, int size) {
    
    auto knob = std::make_unique<SynthKnob>(label, x, y, size, -1.0f, 1.0f, 0.0f);
    knob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (std::abs(value) < 0.01f) {
            ss << "C";
        } else if (value < 0) {
            ss << std::fixed << std::setprecision(0) << std::abs(value * 100.0f) << "L";
        } else {
            ss << std::fixed << std::setprecision(0) << (value * 100.0f) << "R";
        }
        return ss.str();
    });
    return knob;
}

std::unique_ptr<SynthKnob> SynthKnobFactory::createTimeKnob(
    const std::string& label, int x, int y, int size, float maxTime) {
    
    auto knob = std::make_unique<SynthKnob>(label, x, y, size, 0.0f, maxTime, 0.1f);
    knob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << (value * 1000.0f) << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    return knob;
}

} // namespace AIMusicHardware