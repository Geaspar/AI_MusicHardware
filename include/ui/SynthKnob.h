#pragma once

#include "UIComponents.h"
#include "ParameterBridge.h"
#include <memory>
#include <chrono>

namespace AIMusicHardware {

/**
 * @brief Enhanced knob control with parameter binding and modulation display
 * 
 * Inspired by Vital's SynthSlider, this provides professional features
 * including modulation visualization, fine control, and smooth animations.
 */
class SynthKnob : public Knob {
public:
    /**
     * @brief Constructor
     * @param label Label text
     * @param x X position
     * @param y Y position
     * @param size Diameter of the knob
     * @param minValue Minimum value
     * @param maxValue Maximum value
     * @param defaultValue Default/initial value
     */
    SynthKnob(const std::string& label, int x, int y, int size,
              float minValue = 0.0f, float maxValue = 1.0f, float defaultValue = 0.5f);
    
    ~SynthKnob() override;

    /**
     * @brief Bind to a parameter
     * @param parameter The parameter to bind to
     * @param scaleType Value scaling type
     */
    void bindToParameter(Parameter* parameter, 
                        ParameterBridge::ScaleType scaleType = ParameterBridge::ScaleType::Linear);

    /**
     * @brief Get the parameter bridge
     * @return Pointer to parameter bridge (may be null)
     */
    ParameterBridge* getParameterBridge() const { return parameterBridge_.get(); }

    /**
     * @brief Set modulation amount
     * @param amount Modulation amount (-1 to 1)
     */
    void setModulationAmount(float amount);

    /**
     * @brief Get current modulation amount
     * @return Modulation amount
     */
    float getModulationAmount() const { return modulationAmount_; }

    /**
     * @brief Set modulation color
     * @param color Color for modulation display
     */
    void setModulationColor(const Color& color) { modulationColor_ = color; }

    /**
     * @brief Show/hide modulation routing
     * @param show Whether to show routing visualization
     */
    void showModulationRouting(bool show) { showModulationRouting_ = show; }

    /**
     * @brief Enable/disable fine control mode
     * @param enable Whether to enable fine control
     */
    void setFineControlEnabled(bool enable);

    /**
     * @brief Set fine control multiplier
     * @param multiplier Speed multiplier for fine control (0.01 - 1.0)
     */
    void setFineControlMultiplier(float multiplier);

    /**
     * @brief Set whether to show value tooltip
     * @param show Whether to show tooltip
     */
    void setShowValueTooltip(bool show) { showValueTooltip_ = show; }

    /**
     * @brief Enable/disable value change animation
     * @param enable Whether to animate value changes
     */
    void setAnimateValueChanges(bool enable) { animateValueChanges_ = enable; }

    /**
     * @brief Set animation duration
     * @param duration Animation duration in seconds
     */
    void setAnimationDuration(float duration) { animationDuration_ = duration; }

    /**
     * @brief Handle input events
     * @param event The input event
     * @return true if event was handled
     */
    bool handleInput(const InputEvent& event) override;

    /**
     * @brief Update the knob
     * @param deltaTime Time since last update
     */
    void update(float deltaTime) override;

    /**
     * @brief Render the knob
     * @param displayManager The display manager
     */
    void render(DisplayManager* displayManager) override;

    /**
     * @brief Reset to default value
     */
    void resetToDefault();

    /**
     * @brief Set whether knob is being automated
     * @param automated Whether knob is automated
     */
    void setAutomated(bool automated) { isAutomated_ = automated; }
    
    /**
     * @brief Check if knob is currently automated
     * @return true if knob is being automated
     */
    bool isAutomated() const { return isAutomated_; }
    
    /**
     * @brief Set value from automation (with visual feedback)
     * @param value Normalized value from automation
     */
    void setValueFromAutomation(float value);

    /**
     * @brief Get display string for current value
     * @return Formatted value string
     */
    std::string getValueDisplayString() const;

protected:
    // Override base class methods
    void onValueChanged();
    void drawKnob(DisplayManager* displayManager, int centerX, int centerY);
    
    // Additional rendering methods
    void drawModulationRing(DisplayManager* displayManager, int centerX, int centerY);
    void drawValueTooltip(DisplayManager* displayManager);
    void drawAutomationIndicator(DisplayManager* displayManager, int centerX, int centerY);
    void drawFineControlIndicator(DisplayManager* displayManager, int centerX, int centerY);

private:
    // Parameter binding
    std::shared_ptr<ParameterBridge> parameterBridge_;
    
    // Modulation display
    float modulationAmount_ = 0.0f;
    Color modulationColor_{0, 255, 128}; // Default green
    bool showModulationRouting_ = false;
    
    // Fine control
    bool fineControlEnabled_ = false;
    float fineControlMultiplier_ = 0.1f;
    bool fineControlActive_ = false;
    
    // Value display
    bool showValueTooltip_ = true;
    float tooltipAlpha_ = 0.0f;
    std::chrono::steady_clock::time_point lastInteractionTime_;
    
    // Animation
    bool animateValueChanges_ = true;
    float animationDuration_ = 0.1f;
    float animatedValue_ = 0.5f;
    float animationProgress_ = 1.0f;
    float animationStartValue_ = 0.5f;
    float animationTargetValue_ = 0.5f;
    
    // State
    bool isAutomated_ = false;
    float defaultValue_ = 0.5f;
    std::atomic<float> automation_value_{0.0f};
    bool is_being_automated_ = false;
    
    // Visual parameters
    static constexpr int MODULATION_RING_WIDTH = 3;
    static constexpr int AUTOMATION_INDICATOR_SIZE = 4;
    static constexpr float TOOLTIP_FADE_TIME = 0.5f;
    static constexpr float TOOLTIP_SHOW_DELAY = 0.2f;
    
    // Helper methods
    float calculateKnobAngle(float normalizedValue) const;
    void updateAnimation(float deltaTime);
    void updateTooltipVisibility(float deltaTime);
    bool isRecentlyInteracted() const;
};

/**
 * @brief Factory class for creating configured SynthKnobs
 */
class SynthKnobFactory {
public:
    /**
     * @brief Create a frequency knob (20Hz - 20kHz, exponential)
     */
    static std::unique_ptr<SynthKnob> createFrequencyKnob(
        const std::string& label, int x, int y, int size = 80);
    
    /**
     * @brief Create a resonance knob (0-1, quadratic)
     */
    static std::unique_ptr<SynthKnob> createResonanceKnob(
        const std::string& label, int x, int y, int size = 80);
    
    /**
     * @brief Create a volume knob (0-1, logarithmic/decibel)
     */
    static std::unique_ptr<SynthKnob> createVolumeKnob(
        const std::string& label, int x, int y, int size = 80);
    
    /**
     * @brief Create a pan knob (-1 to 1, linear)
     */
    static std::unique_ptr<SynthKnob> createPanKnob(
        const std::string& label, int x, int y, int size = 80);
    
    /**
     * @brief Create a time knob (0-10s, exponential)
     */
    static std::unique_ptr<SynthKnob> createTimeKnob(
        const std::string& label, int x, int y, int size = 80,
        float maxTime = 10.0f);
};

} // namespace AIMusicHardware