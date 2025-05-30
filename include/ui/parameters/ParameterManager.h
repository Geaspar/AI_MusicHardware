#pragma once

#include "Parameter.h"
#include "ParameterGroup.h"
#include "../../iot/IoTEventAdapter.h"
#include <map>
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace AIMusicHardware {

// Forward declarations
class IoTInterface;
class Synthesizer;

/**
 * @brief Central manager for parameters with IoT support
 * 
 * This class provides a global registry and access point for
 * parameters and parameter groups. It also includes support
 * for IoT integration.
 */
class EnhancedParameterManager {
public:
    /**
     * @brief Get singleton instance
     * 
     * @return EnhancedParameterManager& The singleton instance
     */
    static EnhancedParameterManager& getInstance();
    
    /**
     * @brief Get root parameter group
     * 
     * @return ParameterGroup* Pointer to root group
     */
    ParameterGroup* getRootGroup() { return &rootGroup_; }
    
    /**
     * @brief Register parameter with global registry
     * 
     * @param parameter Parameter to register
     */
    void registerParameter(Parameter* parameter);
    
    /**
     * @brief Unregister parameter from global registry
     * 
     * @param parameter Parameter to unregister
     */
    void unregisterParameter(Parameter* parameter);
    
    /**
     * @brief Find parameter by ID
     * 
     * @param id Parameter ID
     * @return Parameter* Pointer to parameter (nullptr if not found)
     */
    Parameter* findParameter(const Parameter::ParameterId& id);
    
    /**
     * @brief Get parameter by path
     * 
     * @param path Path to parameter (e.g., "synth/oscillator1/detune")
     * @return Parameter* Pointer to parameter (nullptr if not found)
     */
    Parameter* getParameterByPath(const std::string& path);
    
    /**
     * @brief Get group by path
     * 
     * @param path Path to group (e.g., "synth/oscillator1")
     * @return ParameterGroup* Pointer to group (nullptr if not found)
     */
    ParameterGroup* getGroupByPath(const std::string& path);
    
    /**
     * @brief Map parameter to MIDI controller
     * 
     * @param parameter Parameter to map
     * @param controller MIDI CC number
     * @param channel MIDI channel (0-15)
     */
    void mapParameterToMidi(Parameter* parameter, int controller, int channel = 0);
    
    /**
     * @brief Unmap parameter from MIDI
     * 
     * @param parameter Parameter to unmap
     */
    void unmapParameterFromMidi(Parameter* parameter);
    
    /**
     * @brief Get parameter for MIDI CC
     * 
     * @param controller MIDI CC number
     * @param channel MIDI channel (0-15)
     * @return Parameter* Mapped parameter (nullptr if not found)
     */
    Parameter* getParameterForMidiCC(int controller, int channel = 0);
    
    /**
     * @brief Connect to synthesizer
     * 
     * @param synth Pointer to synthesizer
     */
    void connectSynthesizer(Synthesizer* synth);
    
    /**
     * @brief Get connected synthesizer
     * 
     * @return Synthesizer* Pointer to connected synthesizer
     */
    Synthesizer* getSynthesizer() const { return synth_; }
    
    // IoT integration
    
    /**
     * @brief Connect to IoT interface
     * 
     * @param iotInterface Pointer to IoT interface
     */
    void connectIoTInterface(IoTInterface* iotInterface);
    
    /**
     * @brief Get connected IoT interface
     * 
     * @return IoTInterface* Pointer to connected IoT interface
     */
    IoTInterface* getIoTInterface() const { return iotInterface_; }
    
    /**
     * @brief Get IoT event adapter
     * 
     * @return IoTEventAdapter* Pointer to IoT event adapter
     */
    IoTEventAdapter* getIoTEventAdapter() { return iotAdapter_.get(); }
    
    /**
     * @brief Map IoT topic to parameter
     * 
     * @param topic IoT topic (may include wildcards)
     * @param parameter Parameter to update
     * @param sensorType Type of sensor data
     * @param minValue Minimum sensor value
     * @param maxValue Maximum sensor value
     */
    void mapIoTTopicToParameter(
        const std::string& topic, 
        Parameter* parameter,
        IoTParameterConverter::SensorType sensorType,
        float minValue = 0.0f,
        float maxValue = 1.0f
    );
    
    /**
     * @brief Set mapping mode for IoT parameter
     * 
     * @param topic IoT topic
     * @param mappingMode Mapping mode
     * @param threshold Threshold value for threshold mode
     * @param exponent Exponent for exponential mode
     */
    void setIoTMappingMode(
        const std::string& topic,
        IoTParameterMappings::MappingMode mappingMode,
        float threshold = 0.5f,
        float exponent = 2.0f
    );
    
    /**
     * @brief Update automation (call in audio thread)
     * 
     * @param deltaTime Time since last update in seconds
     */
    void updateAutomation(float deltaTime);
    
private:
    EnhancedParameterManager();
    ~EnhancedParameterManager();
    
    // Prevent copying
    EnhancedParameterManager(const EnhancedParameterManager&) = delete;
    EnhancedParameterManager& operator=(const EnhancedParameterManager&) = delete;
    
    ParameterGroup rootGroup_{"root", "Root"};
    std::map<Parameter::ParameterId, Parameter*> parameterRegistry_;
    std::map<std::pair<int, int>, Parameter*> midiCCMap_; // (controller, channel) -> parameter
    
    Synthesizer* synth_ = nullptr;
    IoTInterface* iotInterface_ = nullptr;
    std::unique_ptr<IoTEventAdapter> iotAdapter_;
    
    // Parameter smoothing tracking
    struct SmoothingInfo {
        FloatParameter* parameter;
        float updateInterval; // seconds
        float lastUpdate;     // seconds since start
    };
    
    std::vector<SmoothingInfo> smoothingParameters_;
    float totalTime_ = 0.0f;  // seconds since start
    
    // Helper methods
    void syncFromSynthesizer();
    void syncToSynthesizer();
};

} // namespace AIMusicHardware