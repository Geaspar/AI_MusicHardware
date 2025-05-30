#include "../../include/ui/ParameterBridge.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace AIMusicHardware {

ParameterBridge::ParameterBridge(Parameter* parameter, ScaleType scaleType)
    : parameter_(parameter)
    , control_(nullptr)
    , scaleType_(scaleType)
    , currentValue_(0.0f)
    , targetValue_(0.0f)
    , hasNewValue_(false) {
    
    if (parameter_) {
        // Initialize with current parameter value
        float value = 0.0f;
        if (auto* floatParam = dynamic_cast<FloatParameter*>(parameter_)) {
            value = floatParam->getValue();
        } else if (auto* intParam = dynamic_cast<IntParameter*>(parameter_)) {
            value = static_cast<float>(intParam->getValue());
        }
        currentValue_ = value;
        targetValue_ = value;
        
        // Set up parameter change listener
        parameter_->addChangeObserver([this](const Parameter* param) {
            if (auto* floatParam = dynamic_cast<const FloatParameter*>(param)) {
                setValueFromEngine(floatParam->getValue(), ChangeSource::Internal);
            } else if (auto* intParam = dynamic_cast<const IntParameter*>(param)) {
                setValueFromEngine(static_cast<float>(intParam->getValue()), ChangeSource::Internal);
            }
        });
    }
    
    calculateSmoothingCoeff();
}

ParameterBridge::~ParameterBridge() {
    if (control_) {
        unbindControl();
    }
}

void ParameterBridge::bindControl(UIComponent* control) {
    if (control_ != control) {
        if (control_) {
            unbindControl();
        }
        
        control_ = control;
        
        if (control_) {
            // Update control with current value
            updateControlValue(getNormalizedValue());
            
            // Set up control change listener if it's a knob
            if (auto* knob = dynamic_cast<Knob*>(control_)) {
                knob->setValueChangeCallback([this](float value) {
                    setValueFromUI(value, ChangeSource::UI);
                });
            }
        }
    }
}

void ParameterBridge::unbindControl() {
    if (control_) {
        // Remove callback if it's a knob
        if (auto* knob = dynamic_cast<Knob*>(control_)) {
            knob->setValueChangeCallback(nullptr);
        }
        control_ = nullptr;
    }
}

float ParameterBridge::toNormalized(float value) const {
    if (!parameter_) return 0.0f;
    
    float min = 0.0f, max = 1.0f;
    
    if (auto* floatParam = dynamic_cast<FloatParameter*>(parameter_)) {
        // Access member variables directly as they're protected/private
        // Use default range for now - we'll need to add getter methods later
        min = 0.0f;
        max = 1.0f;
    } else if (auto* intParam = dynamic_cast<IntParameter*>(parameter_)) {
        // Same for IntParameter
        min = 0.0f;
        max = 127.0f;
    }
    
    if (max <= min) return 0.0f;
    
    float normalized = (value - min) / (max - min);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    
    return applyInverseCurve(normalized);
}

float ParameterBridge::fromNormalized(float normalized) const {
    if (!parameter_) return 0.0f;
    
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    float curved = applyCurve(normalized);
    
    float min = 0.0f, max = 1.0f;
    
    if (auto* floatParam = dynamic_cast<FloatParameter*>(parameter_)) {
        // Access member variables directly as they're protected/private
        // Use default range for now - we'll need to add getter methods later
        min = 0.0f;
        max = 1.0f;
    } else if (auto* intParam = dynamic_cast<IntParameter*>(parameter_)) {
        // Same for IntParameter
        min = 0.0f;
        max = 127.0f;
    }
    
    return min + (max - min) * curved;
}

void ParameterBridge::setValueFromUI(float normalized, ChangeSource source) {
    float value = fromNormalized(normalized);
    targetValue_ = value;
    hasNewValue_ = true;
    
    if (!smoothingEnabled_) {
        currentValue_ = value;
        updateParameterValue(value);
    }
    
    notifyListeners(value, source);
}

void ParameterBridge::setValueFromEngine(float value, ChangeSource source) {
    targetValue_ = value;
    hasNewValue_ = true;
    
    if (!smoothingEnabled_) {
        currentValue_ = value;
        updateControlValue(toNormalized(value));
    }
    
    notifyListeners(value, source);
}

float ParameterBridge::getNormalizedValue() const {
    return toNormalized(currentValue_.load());
}

float ParameterBridge::getValue() const {
    return currentValue_.load();
}

void ParameterBridge::setSmoothing(bool enable, float smoothingTime) {
    smoothingEnabled_ = enable;
    smoothingTime_ = smoothingTime;
    calculateSmoothingCoeff();
}

void ParameterBridge::processSmoothing(float deltaTime) {
    if (smoothingEnabled_ && hasNewValue_) {
        float current = currentValue_.load();
        float target = targetValue_.load();
        
        if (std::abs(current - target) > 0.0001f) {
            // Exponential smoothing
            float alpha = 1.0f - std::exp(-deltaTime / smoothingTime_);
            float newValue = current + alpha * (target - current);
            currentValue_ = newValue;
            
            // Update parameter and control
            updateParameterValue(newValue);
            updateControlValue(toNormalized(newValue));
        } else {
            currentValue_ = target;
            hasNewValue_ = false;
        }
    }
}

void ParameterBridge::addValueChangeListener(std::function<void(float, ChangeSource)> listener) {
    if (listener) {
        valueChangeListeners_.push_back(listener);
    }
}

void ParameterBridge::clearValueChangeListeners() {
    valueChangeListeners_.clear();
}

std::string ParameterBridge::getDisplayString() const {
    if (displayFormatter_) {
        return displayFormatter_(getValue());
    }
    
    if (!parameter_) return "0";
    
    // Default formatting based on parameter type
    if (auto* floatParam = dynamic_cast<FloatParameter*>(parameter_)) {
        std::stringstream ss;
        float value = floatParam->getValue();
        
        // Use appropriate precision based on value magnitude
        if (std::abs(value) < 1.0f) {
            ss << std::fixed << std::setprecision(3) << value;
        } else if (std::abs(value) < 100.0f) {
            ss << std::fixed << std::setprecision(2) << value;
        } else {
            ss << std::fixed << std::setprecision(1) << value;
        }
        
        // Units not currently supported in base Parameter class
        // TODO: Add unit support to parameters
        
        return ss.str();
    } else if (auto* intParam = dynamic_cast<IntParameter*>(parameter_)) {
        std::stringstream ss;
        ss << intParam->getValue();
        
        // Units not currently supported in base Parameter class
        // TODO: Add unit support to parameters
        
        return ss.str();
    }
    
    return "0";
}

void ParameterBridge::setDisplayFormatter(std::function<std::string(float)> formatter) {
    displayFormatter_ = formatter;
}

void ParameterBridge::notifyListeners(float value, ChangeSource source) {
    for (auto& listener : valueChangeListeners_) {
        listener(value, source);
    }
}

void ParameterBridge::updateControlValue(float normalized) {
    if (control_) {
        if (auto* knob = dynamic_cast<Knob*>(control_)) {
            knob->setValue(normalized);
        }
        // Add support for other control types as needed
    }
}

void ParameterBridge::updateParameterValue(float value) {
    if (parameter_) {
        if (auto* floatParam = dynamic_cast<FloatParameter*>(parameter_)) {
            floatParam->setValue(value);
        } else if (auto* intParam = dynamic_cast<IntParameter*>(parameter_)) {
            intParam->setValue(static_cast<int>(std::round(value)));
        }
    }
}

float ParameterBridge::applyCurve(float normalized) const {
    switch (scaleType_) {
        case ScaleType::Linear:
            return normalized;
            
        case ScaleType::Quadratic:
            return normalized * normalized;
            
        case ScaleType::Cubic:
            return normalized * normalized * normalized;
            
        case ScaleType::Exponential:
            return (std::exp(normalized) - 1.0f) / (std::exp(1.0f) - 1.0f);
            
        case ScaleType::Logarithmic:
            return std::log10(1.0f + 9.0f * normalized);
            
        case ScaleType::Decibel:
            // Convert linear to dB scale (-60dB to 0dB)
            if (normalized <= 0.0f) return -60.0f;
            return 20.0f * std::log10(normalized);
            
        default:
            return normalized;
    }
}

float ParameterBridge::applyInverseCurve(float value) const {
    switch (scaleType_) {
        case ScaleType::Linear:
            return value;
            
        case ScaleType::Quadratic:
            return std::sqrt(value);
            
        case ScaleType::Cubic:
            return std::cbrt(value);
            
        case ScaleType::Exponential:
            return std::log(1.0f + value * (std::exp(1.0f) - 1.0f));
            
        case ScaleType::Logarithmic:
            return (std::pow(10.0f, value) - 1.0f) / 9.0f;
            
        case ScaleType::Decibel:
            // Convert dB to linear scale
            return std::pow(10.0f, value / 20.0f);
            
        default:
            return value;
    }
}

void ParameterBridge::calculateSmoothingCoeff() {
    // Calculate coefficient for exponential smoothing
    // This is used for more efficient smoothing calculation
    if (smoothingTime_ > 0.0f) {
        smoothingCoeff_ = 1.0f - std::exp(-1.0f / smoothingTime_);
    } else {
        smoothingCoeff_ = 1.0f;
    }
}

// ParameterBridgeManager implementation

ParameterBridgeManager& ParameterBridgeManager::getInstance() {
    static ParameterBridgeManager instance;
    return instance;
}

std::shared_ptr<ParameterBridge> ParameterBridgeManager::createBridge(
    Parameter* parameter, 
    ParameterBridge::ScaleType scaleType) {
    
    if (!parameter) return nullptr;
    
    std::lock_guard<std::mutex> lock(bridgesMutex_);
    
    auto bridge = std::make_shared<ParameterBridge>(parameter, scaleType);
    bridges_[parameter->getId()] = bridge;
    
    return bridge;
}

std::shared_ptr<ParameterBridge> ParameterBridgeManager::getBridge(const Parameter::ParameterId& parameterId) {
    std::lock_guard<std::mutex> lock(bridgesMutex_);
    
    auto it = bridges_.find(parameterId);
    if (it != bridges_.end()) {
        return it->second;
    }
    
    return nullptr;
}

void ParameterBridgeManager::removeBridge(const Parameter::ParameterId& parameterId) {
    std::lock_guard<std::mutex> lock(bridgesMutex_);
    bridges_.erase(parameterId);
}

void ParameterBridgeManager::processAllSmoothing(float deltaTime) {
    std::lock_guard<std::mutex> lock(bridgesMutex_);
    
    for (auto& [id, bridge] : bridges_) {
        if (bridge) {
            bridge->processSmoothing(deltaTime);
        }
    }
}

void ParameterBridgeManager::clear() {
    std::lock_guard<std::mutex> lock(bridgesMutex_);
    bridges_.clear();
}

} // namespace AIMusicHardware