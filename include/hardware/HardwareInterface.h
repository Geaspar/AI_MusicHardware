#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <map>

namespace AIMusicHardware {

enum class ControllerType {
    Knob,
    Slider,
    Button,
    Pad,
    Encoder,
    TouchSurface,
    Display,
    LED
};

struct ControllerInfo {
    int id;
    std::string name;
    ControllerType type;
    float minValue;
    float maxValue;
    float defaultValue;
    bool isBipolar;
    bool isMomentary;
    
    ControllerInfo() : id(0), type(ControllerType::Knob), minValue(0.0f), maxValue(1.0f), 
                       defaultValue(0.0f), isBipolar(false), isMomentary(false) {}
};

class HardwareInterface {
public:
    using ControlChangeCallback = std::function<void(int controllerId, float value)>;
    using ButtonCallback = std::function<void(int buttonId, bool isPressed)>;
    using PadCallback = std::function<void(int padId, float pressure)>;
    
    HardwareInterface();
    ~HardwareInterface();
    
    bool initialize();
    void shutdown();
    
    // Controller discovery
    std::vector<ControllerInfo> discoverControllers();
    bool hasController(int controllerId) const;
    ControllerInfo getControllerInfo(int controllerId) const;
    
    // Callback registration
    void setControlChangeCallback(ControlChangeCallback callback);
    void setButtonCallback(ButtonCallback callback);
    void setPadCallback(PadCallback callback);
    
    // Output to hardware
    void setLED(int ledId, int r, int g, int b);
    void setDisplayText(int displayId, const std::string& text);
    void setDisplayValue(int displayId, float value);
    
    // Metadata and labeling
    void setControllerLabel(int controllerId, const std::string& label);
    std::string getControllerLabel(int controllerId) const;
    
    // Parameter mapping
    void mapControllerToParameter(int controllerId, const std::string& parameterName);
    std::string getMappedParameter(int controllerId) const;
    
    // Save/load mappings
    bool saveMappings(const std::string& filename) const;
    bool loadMappings(const std::string& filename);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    std::map<int, ControllerInfo> controllers_;
    std::map<int, std::string> controllerLabels_;
    std::map<int, std::string> parameterMappings_;
    
    ControlChangeCallback controlChangeCallback_;
    ButtonCallback buttonCallback_;
    PadCallback padCallback_;
};

} // namespace AIMusicHardware