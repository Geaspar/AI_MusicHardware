#pragma once

#include "parameters/Parameter.h"
#include "UIComponents.h"
#include <atomic>
#include <functional>
#include <string>
#include <memory>

namespace AIMusicHardware {

/**
 * @brief Bridge between UI controls and synthesizer parameters
 * 
 * Inspired by Vital's ValueBridge, this class provides thread-safe
 * communication between UI components and audio engine parameters.
 */
class ParameterBridge {
public:
    /**
     * @brief Scaling types for parameter values
     */
    enum class ScaleType {
        Linear,
        Quadratic,
        Cubic,
        Exponential,
        Logarithmic,
        Decibel
    };

    /**
     * @brief Source of parameter change
     */
    enum class ChangeSource {
        UI,
        MIDI,
        IoT,
        Automation,
        Preset,
        Internal
    };

    /**
     * @brief Constructor
     * @param parameter The parameter to bridge
     * @param scaleType The scaling type for value conversion
     */
    ParameterBridge(Parameter* parameter, ScaleType scaleType = ScaleType::Linear);
    
    ~ParameterBridge();

    /**
     * @brief Bind a UI control to this parameter
     * @param control The UI control to bind
     */
    void bindControl(UIComponent* control);

    /**
     * @brief Unbind the current UI control
     */
    void unbindControl();

    /**
     * @brief Get the bound parameter
     * @return The parameter this bridge controls
     */
    Parameter* getParameter() const { return parameter_; }

    /**
     * @brief Get the bound UI control
     * @return The UI control bound to this bridge
     */
    UIComponent* getControl() const { return control_; }

    /**
     * @brief Convert parameter value to normalized (0-1) range
     * @param value The actual parameter value
     * @return Normalized value (0-1)
     */
    float toNormalized(float value) const;

    /**
     * @brief Convert normalized value to parameter range
     * @param normalized The normalized value (0-1)
     * @return Actual parameter value
     */
    float fromNormalized(float normalized) const;

    /**
     * @brief Set value from UI (normalized)
     * @param normalized The normalized value (0-1)
     * @param source The source of the change
     */
    void setValueFromUI(float normalized, ChangeSource source = ChangeSource::UI);

    /**
     * @brief Set value from engine (actual value)
     * @param value The actual parameter value
     * @param source The source of the change
     */
    void setValueFromEngine(float value, ChangeSource source = ChangeSource::Internal);

    /**
     * @brief Get current normalized value
     * @return Current value in 0-1 range
     */
    float getNormalizedValue() const;

    /**
     * @brief Get current actual value
     * @return Current parameter value
     */
    float getValue() const;

    /**
     * @brief Set the scaling type
     * @param scaleType New scaling type
     */
    void setScaleType(ScaleType scaleType) { scaleType_ = scaleType; }

    /**
     * @brief Get the scaling type
     * @return Current scaling type
     */
    ScaleType getScaleType() const { return scaleType_; }

    /**
     * @brief Enable/disable smoothing
     * @param enable Whether to enable value smoothing
     * @param smoothingTime Time constant for smoothing (seconds)
     */
    void setSmoothing(bool enable, float smoothingTime = 0.05f);

    /**
     * @brief Process smoothing (call from audio thread)
     * @param deltaTime Time since last update
     */
    void processSmoothing(float deltaTime);

    /**
     * @brief Add listener for value changes
     * @param listener Callback function
     */
    void addValueChangeListener(std::function<void(float, ChangeSource)> listener);

    /**
     * @brief Clear all value change listeners
     */
    void clearValueChangeListeners();

    /**
     * @brief Get display string for current value
     * @return Formatted string for UI display
     */
    std::string getDisplayString() const;

    /**
     * @brief Set custom display formatter
     * @param formatter Function to format values for display
     */
    void setDisplayFormatter(std::function<std::string(float)> formatter);

private:
    Parameter* parameter_;
    UIComponent* control_;
    ScaleType scaleType_;
    
    // Thread-safe value storage
    std::atomic<float> currentValue_;
    std::atomic<float> targetValue_;
    std::atomic<bool> hasNewValue_;
    
    // Smoothing
    bool smoothingEnabled_ = false;
    float smoothingTime_ = 0.05f;
    float smoothingCoeff_ = 0.0f;
    
    // Listeners
    std::vector<std::function<void(float, ChangeSource)>> valueChangeListeners_;
    
    // Display formatting
    std::function<std::string(float)> displayFormatter_;
    
    // Helper methods
    void notifyListeners(float value, ChangeSource source);
    void updateControlValue(float normalized);
    void updateParameterValue(float value);
    float applyCurve(float normalized) const;
    float applyInverseCurve(float value) const;
    void calculateSmoothingCoeff();
};

/**
 * @brief Manager for parameter bridges
 * 
 * Centralized management of all parameter-UI bindings
 */
class ParameterBridgeManager {
public:
    static ParameterBridgeManager& getInstance();

    /**
     * @brief Create a parameter bridge
     * @param parameter The parameter to bridge
     * @param scaleType The scaling type
     * @return Shared pointer to the created bridge
     */
    std::shared_ptr<ParameterBridge> createBridge(
        Parameter* parameter, 
        ParameterBridge::ScaleType scaleType = ParameterBridge::ScaleType::Linear
    );

    /**
     * @brief Get bridge for a parameter
     * @param parameterId The parameter ID
     * @return Bridge for the parameter (nullptr if not found)
     */
    std::shared_ptr<ParameterBridge> getBridge(const Parameter::ParameterId& parameterId);

    /**
     * @brief Remove a bridge
     * @param parameterId The parameter ID
     */
    void removeBridge(const Parameter::ParameterId& parameterId);

    /**
     * @brief Process all smoothing (call from audio thread)
     * @param deltaTime Time since last update
     */
    void processAllSmoothing(float deltaTime);

    /**
     * @brief Clear all bridges
     */
    void clear();

private:
    ParameterBridgeManager() = default;
    ~ParameterBridgeManager() = default;
    
    std::map<Parameter::ParameterId, std::shared_ptr<ParameterBridge>> bridges_;
    std::mutex bridgesMutex_;
};

} // namespace AIMusicHardware