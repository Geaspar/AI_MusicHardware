#pragma once

#include <string>
#include <functional>
#include <vector>
#include <any>
#include <stdexcept>
#include <memory>
#include <map>
#include <iostream>

namespace AIMusicHardware {

/**
 * @brief Base class for parameters
 * 
 * The Parameter class is the foundation of the parameter system. It provides
 * a common interface for accessing and modifying parameter values, regardless
 * of their type. Parameters can be observed for changes, serialized, and can
 * have metadata like descriptions and visibility flags.
 */
class Parameter {
public:
    // Parameter types
    enum class Type {
        FLOAT,
        INT,
        BOOL,
        ENUM,
        TRIGGER
    };
    
    // Identifier types
    using ParameterId = std::string;
    
    // Observer callback type
    using ChangeCallback = std::function<void(Parameter*)>;
    
    /**
     * @brief Constructor
     * 
     * @param id Unique identifier for the parameter
     * @param name Display name for the parameter
     * @param type Parameter type
     */
    Parameter(const ParameterId& id, const std::string& name, Type type)
        : id_(id), name_(name), type_(type) {
    }
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Parameter() = default;
    
    // Core properties
    
    /**
     * @brief Get the parameter ID
     * 
     * @return ParameterId The parameter identifier
     */
    ParameterId getId() const { return id_; }
    
    /**
     * @brief Get the display name
     * 
     * @return std::string The parameter display name
     */
    std::string getName() const { return name_; }
    
    /**
     * @brief Get the parameter type
     * 
     * @return Type The parameter type
     */
    Type getType() const { return type_; }
    
    /**
     * @brief Get the parameter description
     * 
     * @return std::string The parameter description
     */
    std::string getDescription() const { return description_; }
    
    /**
     * @brief Set the parameter description
     * 
     * @param description The description text
     */
    void setDescription(const std::string& description) { description_ = description; }
    
    // Generic value access (for serialization)
    
    /**
     * @brief Get the parameter value as std::any
     * 
     * @return std::any The parameter value
     */
    std::any getValueAsAny() const { return value_; }
    
    /**
     * @brief Set the parameter value from std::any
     * 
     * @param value The new value
     * @param notifyObservers Whether to notify observers of the change
     * @return true if value was set successfully
     */
    virtual bool setValueFromAny(const std::any& value, bool notifyObservers = true) {
        if (!validateValue(value)) {
            return false;
        }
        
        value_ = value;
        
        if (notifyObservers) {
            notifyValueChanged();
        }
        
        return true;
    }
    
    // Type-specific value access
    
    /**
     * @brief Get parameter value with type conversion
     * 
     * @tparam T The expected value type
     * @return T The parameter value
     * @throws std::bad_any_cast if type conversion fails
     */
    template<typename T>
    T getValue() const {
        try {
            return std::any_cast<T>(value_);
        } catch (const std::bad_any_cast&) {
            std::cerr << "Type conversion failed for parameter " << id_ << std::endl;
            throw;
        }
    }
    
    /**
     * @brief Set parameter value with type checking
     * 
     * @tparam T The value type
     * @param value The new value
     * @param notifyObservers Whether to notify observers of the change
     */
    template<typename T>
    void setValue(const T& value, bool notifyObservers = true) {
        std::any newValue = value;
        if (validateValue(newValue)) {
            value_ = newValue;
            
            if (notifyObservers) {
                notifyValueChanged();
            }
        }
    }
    
    // Metadata
    
    /**
     * @brief Check if parameter is visible in the UI
     * 
     * @return true if visible
     */
    bool isVisible() const { return isVisible_; }
    
    /**
     * @brief Set parameter visibility
     * 
     * @param visible Whether the parameter should be visible
     */
    void setVisible(bool visible) { isVisible_ = visible; }
    
    /**
     * @brief Check if parameter can be automated
     * 
     * @return true if automatable
     */
    bool isAutomatable() const { return isAutomatable_; }
    
    /**
     * @brief Set whether parameter can be automated
     * 
     * @param automatable Whether the parameter can be automated
     */
    void setAutomatable(bool automatable) { isAutomatable_ = automatable; }
    
    // Observer pattern
    
    /**
     * @brief Add a change observer
     * 
     * @param callback Function to call when parameter changes
     */
    void addChangeObserver(ChangeCallback callback) {
        if (callback) {
            changeObservers_.push_back(callback);
        }
    }
    
    /**
     * @brief Remove change observer by owner pointer
     * 
     * @param owner Pointer to the owner object
     */
    void removeChangeObserver(void* owner) {
        // Implementation would remove by owner if tracking was added
    }
    
    /**
     * @brief Get value as normalized float (0-1)
     * 
     * @return float Normalized value
     */
    virtual float getNormalizedValue() const {
        return 0.0f; // Default implementation
    }
    
    /**
     * @brief Set value from normalized float (0-1)
     * 
     * @param normalizedValue Value between 0 and 1
     * @param notifyObservers Whether to notify observers
     */
    virtual void setFromNormalizedValue(float normalizedValue, bool notifyObservers = true) {
        // Default implementation - override in derived classes
    }
    
    /**
     * @brief Format value as string for display
     * 
     * @return std::string Formatted value
     */
    virtual std::string getValueAsString() const {
        return ""; // Default implementation - override in derived classes
    }
    
protected:
    ParameterId id_;
    std::string name_;
    std::string description_;
    Type type_;
    std::any value_;
    bool isVisible_ = true;
    bool isAutomatable_ = true;
    
    std::vector<ChangeCallback> changeObservers_;
    
    /**
     * @brief Notify observers of value change
     */
    virtual void notifyValueChanged() {
        for (const auto& observer : changeObservers_) {
            if (observer) {
                observer(this);
            }
        }
    }
    
    /**
     * @brief Validate value before setting
     * 
     * @param value The value to validate
     * @return true if valid
     */
    virtual bool validateValue(const std::any& value) const {
        return true; // Default implementation accepts any value
    }
};

/**
 * @brief Float parameter with range and smoothing
 */
class FloatParameter : public Parameter {
public:
    /**
     * @brief Constructor
     * 
     * @param id Parameter ID
     * @param name Display name
     * @param defaultValue Default value
     */
    FloatParameter(const ParameterId& id, const std::string& name, float defaultValue = 0.0f)
        : Parameter(id, name, Type::FLOAT),
          minValue_(0.0f),
          maxValue_(1.0f),
          defaultValue_(defaultValue),
          currentValue_(defaultValue),
          targetValue_(defaultValue) {
        value_ = defaultValue;
    }
    
    /**
     * @brief Set the value range
     * 
     * @param min Minimum value
     * @param max Maximum value
     */
    void setRange(float min, float max) {
        if (min < max) {
            minValue_ = min;
            maxValue_ = max;
            
            // Clamp current value to new range
            float currentVal = std::any_cast<float>(value_);
            if (currentVal < minValue_ || currentVal > maxValue_) {
                setValue(std::clamp(currentVal, minValue_, maxValue_));
            }
        }
    }
    
    /**
     * @brief Set the default value
     * 
     * @param defaultValue New default value
     */
    void setDefaultValue(float defaultValue) {
        defaultValue_ = std::clamp(defaultValue, minValue_, maxValue_);
    }
    
    /**
     * @brief Get the default value
     * 
     * @return float Default value
     */
    float getDefaultValue() const { return defaultValue_; }
    
    /**
     * @brief Get the current value
     * 
     * @return float Parameter value
     */
    float getValue() const {
        if (smoothingEnabled_) {
            return currentValue_;
        } else {
            return std::any_cast<float>(value_);
        }
    }
    
    /**
     * @brief Set the parameter value
     * 
     * @param value New value
     * @param notifyObservers Whether to notify observers
     */
    void setValue(float value, bool notifyObservers = true) {
        // Clamp to valid range
        value = std::clamp(value, minValue_, maxValue_);
        
        if (smoothingEnabled_) {
            targetValue_ = value;
            if (!notifyObservers) {
                // If we don't want to notify, set immediately
                currentValue_ = value;
            }
        } else {
            value_ = value;
            if (notifyObservers) {
                notifyValueChanged();
            }
        }
    }
    
    /**
     * @brief Get normalized value (0-1)
     * 
     * @return float Normalized value
     */
    float getNormalizedValue() const override {
        float val = getValue();
        if (std::abs(maxValue_ - minValue_) < 0.00001f) {
            return 0.0f;
        }
        return (val - minValue_) / (maxValue_ - minValue_);
    }
    
    /**
     * @brief Set from normalized value
     * 
     * @param normalizedValue Value between 0-1
     * @param notifyObservers Whether to notify observers
     */
    void setFromNormalizedValue(float normalizedValue, bool notifyObservers = true) override {
        normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
        float value = minValue_ + normalizedValue * (maxValue_ - minValue_);
        setValue(value, notifyObservers);
    }
    
    /**
     * @brief Enable value smoothing
     * 
     * @param timeInSeconds Smoothing time in seconds
     */
    void enableSmoothing(float timeInSeconds) {
        smoothingTimeSeconds_ = timeInSeconds;
        smoothingEnabled_ = true;
        
        // Initialize smoothing values
        currentValue_ = std::any_cast<float>(value_);
        targetValue_ = currentValue_;
    }
    
    /**
     * @brief Disable value smoothing
     */
    void disableSmoothing() {
        smoothingEnabled_ = false;
        
        // Set current value immediately
        if (currentValue_ != targetValue_) {
            value_ = targetValue_;
            currentValue_ = targetValue_;
        }
    }
    
    /**
     * @brief Update smoothing (call in audio thread)
     * 
     * @param deltaTime Time since last update in seconds
     */
    void updateSmoothing(float deltaTime) {
        if (!smoothingEnabled_ || std::abs(currentValue_ - targetValue_) < 0.0001f) {
            return;
        }
        
        // Simple low-pass filter
        float smoothingFactor = deltaTime / smoothingTimeSeconds_;
        if (smoothingFactor > 1.0f) smoothingFactor = 1.0f;
        
        currentValue_ += smoothingFactor * (targetValue_ - currentValue_);
        
        // Update the stored value
        value_ = currentValue_;
        
        // Notify observers
        notifyValueChanged();
    }
    
    /**
     * @brief Format value as string
     * 
     * @return std::string Formatted value
     */
    std::string getValueAsString() const override {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f", getValue());
        return buffer;
    }
    
protected:
    float minValue_;
    float maxValue_;
    float defaultValue_;
    
    // Smoothing
    bool smoothingEnabled_ = false;
    float smoothingTimeSeconds_ = 0.1f;
    float targetValue_;
    float currentValue_;
    
    /**
     * @brief Validate float value
     * 
     * @param value Value to validate
     * @return true if valid
     */
    bool validateValue(const std::any& value) const override {
        try {
            float floatValue = std::any_cast<float>(value);
            return floatValue >= minValue_ && floatValue <= maxValue_;
        } catch (const std::bad_any_cast&) {
            return false;
        }
    }
};

/**
 * @brief Integer parameter with range
 */
class IntParameter : public Parameter {
public:
    /**
     * @brief Constructor
     * 
     * @param id Parameter ID
     * @param name Display name
     * @param defaultValue Default value
     */
    IntParameter(const ParameterId& id, const std::string& name, int defaultValue = 0)
        : Parameter(id, name, Type::INT),
          minValue_(0),
          maxValue_(127),
          defaultValue_(defaultValue) {
        value_ = defaultValue;
    }
    
    /**
     * @brief Set the value range
     * 
     * @param min Minimum value
     * @param max Maximum value
     */
    void setRange(int min, int max) {
        if (min < max) {
            minValue_ = min;
            maxValue_ = max;
            
            // Clamp current value to new range
            int currentVal = std::any_cast<int>(value_);
            if (currentVal < minValue_ || currentVal > maxValue_) {
                setValue(std::clamp(currentVal, minValue_, maxValue_));
            }
        }
    }
    
    /**
     * @brief Set the default value
     * 
     * @param defaultValue New default value
     */
    void setDefaultValue(int defaultValue) {
        defaultValue_ = std::clamp(defaultValue, minValue_, maxValue_);
    }
    
    /**
     * @brief Get the default value
     * 
     * @return int Default value
     */
    int getDefaultValue() const { return defaultValue_; }
    
    /**
     * @brief Get the current value
     * 
     * @return int Parameter value
     */
    int getValue() const {
        return std::any_cast<int>(value_);
    }
    
    /**
     * @brief Set the parameter value
     * 
     * @param value New value
     * @param notifyObservers Whether to notify observers
     */
    void setValue(int value, bool notifyObservers = true) {
        // Clamp to valid range
        value = std::clamp(value, minValue_, maxValue_);
        
        value_ = value;
        
        if (notifyObservers) {
            notifyValueChanged();
        }
    }
    
    /**
     * @brief Get normalized value (0-1)
     * 
     * @return float Normalized value
     */
    float getNormalizedValue() const override {
        int val = getValue();
        if (maxValue_ == minValue_) {
            return 0.0f;
        }
        return static_cast<float>(val - minValue_) / static_cast<float>(maxValue_ - minValue_);
    }
    
    /**
     * @brief Set from normalized value
     * 
     * @param normalizedValue Value between 0-1
     * @param notifyObservers Whether to notify observers
     */
    void setFromNormalizedValue(float normalizedValue, bool notifyObservers = true) override {
        normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
        int range = maxValue_ - minValue_;
        int value = minValue_ + static_cast<int>(normalizedValue * range + 0.5f);
        setValue(value, notifyObservers);
    }
    
    /**
     * @brief Format value as string
     * 
     * @return std::string Formatted value
     */
    std::string getValueAsString() const override {
        return std::to_string(getValue());
    }
    
protected:
    int minValue_;
    int maxValue_;
    int defaultValue_;
    
    /**
     * @brief Validate integer value
     * 
     * @param value Value to validate
     * @return true if valid
     */
    bool validateValue(const std::any& value) const override {
        try {
            int intValue = std::any_cast<int>(value);
            return intValue >= minValue_ && intValue <= maxValue_;
        } catch (const std::bad_any_cast&) {
            return false;
        }
    }
};

/**
 * @brief Boolean parameter
 */
class BoolParameter : public Parameter {
public:
    /**
     * @brief Constructor
     * 
     * @param id Parameter ID
     * @param name Display name
     * @param defaultValue Default value
     */
    BoolParameter(const ParameterId& id, const std::string& name, bool defaultValue = false)
        : Parameter(id, name, Type::BOOL),
          defaultValue_(defaultValue) {
        value_ = defaultValue;
    }
    
    /**
     * @brief Set the default value
     * 
     * @param defaultValue New default value
     */
    void setDefaultValue(bool defaultValue) {
        defaultValue_ = defaultValue;
    }
    
    /**
     * @brief Get the default value
     * 
     * @return bool Default value
     */
    bool getDefaultValue() const { return defaultValue_; }
    
    /**
     * @brief Get the current value
     * 
     * @return bool Parameter value
     */
    bool getValue() const {
        return std::any_cast<bool>(value_);
    }
    
    /**
     * @brief Set the parameter value
     * 
     * @param value New value
     * @param notifyObservers Whether to notify observers
     */
    void setValue(bool value, bool notifyObservers = true) {
        value_ = value;
        
        if (notifyObservers) {
            notifyValueChanged();
        }
    }
    
    /**
     * @brief Toggle parameter value
     * 
     * @param notifyObservers Whether to notify observers
     * @return bool New value
     */
    bool toggle(bool notifyObservers = true) {
        bool newValue = !getValue();
        setValue(newValue, notifyObservers);
        return newValue;
    }
    
    /**
     * @brief Get normalized value (0-1)
     * 
     * @return float Normalized value (0.0 or 1.0)
     */
    float getNormalizedValue() const override {
        return getValue() ? 1.0f : 0.0f;
    }
    
    /**
     * @brief Set from normalized value
     * 
     * @param normalizedValue Value between 0-1
     * @param notifyObservers Whether to notify observers
     */
    void setFromNormalizedValue(float normalizedValue, bool notifyObservers = true) override {
        setValue(normalizedValue >= 0.5f, notifyObservers);
    }
    
    /**
     * @brief Format value as string
     * 
     * @return std::string Formatted value
     */
    std::string getValueAsString() const override {
        return getValue() ? "On" : "Off";
    }
    
protected:
    bool defaultValue_;
    
    /**
     * @brief Validate boolean value
     * 
     * @param value Value to validate
     * @return true if valid
     */
    bool validateValue(const std::any& value) const override {
        try {
            std::any_cast<bool>(value);
            return true;
        } catch (const std::bad_any_cast&) {
            return false;
        }
    }
};

/**
 * @brief Enumeration parameter
 */
class EnumParameter : public Parameter {
public:
    /**
     * @brief Enum value entry
     */
    struct EnumValue {
        int value;              ///< Numeric value
        std::string name;       ///< Display name
        std::string description; ///< Description
    };
    
    /**
     * @brief Constructor
     * 
     * @param id Parameter ID
     * @param name Display name
     */
    EnumParameter(const ParameterId& id, const std::string& name)
        : Parameter(id, name, Type::ENUM),
          defaultValueIndex_(0) {
        value_ = 0; // Default to first value (which doesn't exist yet)
    }
    
    /**
     * @brief Add a value to the enumeration
     * 
     * @param value Numeric value
     * @param name Display name
     * @param description Description
     */
    void addValue(int value, const std::string& name, const std::string& description = "") {
        EnumValue enumValue;
        enumValue.value = value;
        enumValue.name = name;
        enumValue.description = description;
        
        enumValues_.push_back(enumValue);
        
        // If this is the first value, set it as current
        if (enumValues_.size() == 1) {
            value_ = value;
        }
    }
    
    /**
     * @brief Set the default value index
     * 
     * @param index Index in the enum values array
     */
    void setDefaultValueIndex(int index) {
        if (index >= 0 && index < static_cast<int>(enumValues_.size())) {
            defaultValueIndex_ = index;
        }
    }
    
    /**
     * @brief Get the default value index
     * 
     * @return int Default value index
     */
    int getDefaultValueIndex() const { return defaultValueIndex_; }
    
    /**
     * @brief Get the current value
     * 
     * @return int Current value
     */
    int getValue() const {
        return std::any_cast<int>(value_);
    }
    
    /**
     * @brief Set the parameter value
     * 
     * @param value New value
     * @param notifyObservers Whether to notify observers
     */
    void setValue(int value, bool notifyObservers = true) {
        // Check if value exists in enum
        bool validValue = false;
        for (const auto& enumValue : enumValues_) {
            if (enumValue.value == value) {
                validValue = true;
                break;
            }
        }
        
        if (validValue) {
            value_ = value;
            
            if (notifyObservers) {
                notifyValueChanged();
            }
        }
    }
    
    /**
     * @brief Get the current value name
     * 
     * @return std::string Value name
     */
    std::string getCurrentValueName() const {
        int currentValue = getValue();
        for (const auto& enumValue : enumValues_) {
            if (enumValue.value == currentValue) {
                return enumValue.name;
            }
        }
        return ""; // Not found
    }
    
    /**
     * @brief Set value by name
     * 
     * @param name Value name
     * @param notifyObservers Whether to notify observers
     * @return true if value was found and set
     */
    bool setValueByName(const std::string& name, bool notifyObservers = true) {
        for (const auto& enumValue : enumValues_) {
            if (enumValue.name == name) {
                setValue(enumValue.value, notifyObservers);
                return true;
            }
        }
        return false; // Name not found
    }
    
    /**
     * @brief Get the number of values
     * 
     * @return int Number of values
     */
    int getValueCount() const { 
        return static_cast<int>(enumValues_.size()); 
    }
    
    /**
     * @brief Get value at index
     * 
     * @param index Value index
     * @return const EnumValue& Value entry
     */
    const EnumValue& getValueAtIndex(int index) const {
        if (index >= 0 && index < static_cast<int>(enumValues_.size())) {
            return enumValues_[index];
        }
        throw std::out_of_range("Enum index out of range");
    }
    
    /**
     * @brief Get current value index
     * 
     * @return int Current index (-1 if not found)
     */
    int getCurrentIndex() const {
        int currentValue = getValue();
        for (size_t i = 0; i < enumValues_.size(); i++) {
            if (enumValues_[i].value == currentValue) {
                return static_cast<int>(i);
            }
        }
        return -1; // Not found
    }
    
    /**
     * @brief Set value by index
     * 
     * @param index Value index
     * @param notifyObservers Whether to notify observers
     */
    void setValueIndex(int index, bool notifyObservers = true) {
        if (index >= 0 && index < static_cast<int>(enumValues_.size())) {
            setValue(enumValues_[index].value, notifyObservers);
        }
    }
    
    /**
     * @brief Get normalized value (0-1)
     * 
     * @return float Normalized value
     */
    float getNormalizedValue() const override {
        int index = getCurrentIndex();
        if (index < 0 || enumValues_.empty()) {
            return 0.0f;
        }
        return static_cast<float>(index) / static_cast<float>(enumValues_.size() - 1);
    }
    
    /**
     * @brief Set from normalized value
     * 
     * @param normalizedValue Value between 0-1
     * @param notifyObservers Whether to notify observers
     */
    void setFromNormalizedValue(float normalizedValue, bool notifyObservers = true) override {
        if (enumValues_.empty()) {
            return;
        }
        
        normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
        int index = static_cast<int>(normalizedValue * (enumValues_.size() - 1) + 0.5f);
        setValueIndex(index, notifyObservers);
    }
    
    /**
     * @brief Format value as string
     * 
     * @return std::string Formatted value
     */
    std::string getValueAsString() const override {
        return getCurrentValueName();
    }
    
protected:
    std::vector<EnumValue> enumValues_;
    int defaultValueIndex_;
    
    /**
     * @brief Validate enum value
     * 
     * @param value Value to validate
     * @return true if valid
     */
    bool validateValue(const std::any& value) const override {
        try {
            int intValue = std::any_cast<int>(value);
            for (const auto& enumValue : enumValues_) {
                if (enumValue.value == intValue) {
                    return true;
                }
            }
            return false; // Value not found in enum
        } catch (const std::bad_any_cast&) {
            return false;
        }
    }
};

/**
 * @brief Trigger parameter
 */
class TriggerParameter : public Parameter {
public:
    /**
     * @brief Constructor
     * 
     * @param id Parameter ID
     * @param name Display name
     */
    TriggerParameter(const ParameterId& id, const std::string& name)
        : Parameter(id, name, Type::TRIGGER) {
        value_ = false; // Triggers are boolean but transient
    }
    
    /**
     * @brief Trigger the parameter
     */
    void trigger() {
        // Set to true temporarily
        value_ = true;
        
        // Notify observers
        notifyValueChanged();
        
        // Notify trigger listeners
        for (const auto& listener : triggerListeners_) {
            if (listener) {
                listener();
            }
        }
        
        // Reset to false
        value_ = false;
    }
    
    /**
     * @brief Add trigger listener
     * 
     * @param callback Function to call when triggered
     */
    void addTriggerListener(std::function<void()> callback) {
        if (callback) {
            triggerListeners_.push_back(callback);
        }
    }
    
    /**
     * @brief Get normalized value
     * 
     * @return float Always 0.0 for triggers
     */
    float getNormalizedValue() const override {
        return 0.0f; // Triggers don't have persistent value
    }
    
    /**
     * @brief Set from normalized value
     * 
     * @param normalizedValue Value between 0-1
     * @param notifyObservers Whether to notify observers
     */
    void setFromNormalizedValue(float normalizedValue, bool notifyObservers = true) override {
        if (normalizedValue > 0.5f) {
            trigger();
        }
    }
    
    /**
     * @brief Format value as string
     * 
     * @return std::string Always "Trigger" for triggers
     */
    std::string getValueAsString() const override {
        return "Trigger";
    }
    
protected:
    std::vector<std::function<void()>> triggerListeners_;
};

} // namespace AIMusicHardware