#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>

namespace AIMusicHardware {

// Forward declarations
class Synthesizer;

/**
 * @brief Represents a modulation routing between parameters
 */
struct ModulationRouting {
    std::string source;      // Source parameter ID (LFO, envelope, etc)
    std::string destination; // Destination parameter ID
    float amount;            // Modulation amount (-1.0 to 1.0)
    
    ModulationRouting() : amount(0.0f) {}
    ModulationRouting(const std::string& src, const std::string& dest, float amt)
        : source(src), destination(dest), amount(amt) {}
    
    // Equality operators for container operations
    bool operator==(const ModulationRouting& other) const {
        return source == other.source && destination == other.destination;
    }
    
    bool operator!=(const ModulationRouting& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Manages synthesizer parameters for UI and preset system
 * 
 * This class provides a unified interface for getting and setting
 * synthesizer parameters, as well as managing modulation routings.
 * It serves as the connection between the UI, preset system, and
 * the core synthesis engine.
 */
class ParameterManager {
public:
    /**
     * @brief Constructor
     */
    ParameterManager();
    
    /**
     * @brief Destructor
     */
    ~ParameterManager();
    
    /**
     * @brief Initialize the parameter manager
     * 
     * @return true if initialization was successful
     */
    bool initialize();
    
    /**
     * @brief Connect to the synthesizer
     * 
     * @param synth Pointer to the synthesizer
     */
    void connectSynthesizer(Synthesizer* synth);
    
    /**
     * @brief Get the value of a parameter
     * 
     * @param parameterId The ID of the parameter
     * @return float The parameter value
     */
    float getParameterValue(const std::string& parameterId) const;
    
    /**
     * @brief Set the value of a parameter
     * 
     * @param parameterId The ID of the parameter
     * @param value The new value
     */
    void setParameterValue(const std::string& parameterId, float value);
    
    /**
     * @brief Get all parameters
     * 
     * @return std::map<std::string, float> Map of parameter IDs to values
     */
    std::map<std::string, float> getAllParameters() const;
    
    /**
     * @brief Set all parameters at once
     * 
     * @param parameters Map of parameter IDs to values
     */
    void setAllParameters(const std::map<std::string, float>& parameters);
    
    /**
     * @brief Add a modulation routing
     * 
     * @param source Source parameter ID
     * @param destination Destination parameter ID
     * @param amount Modulation amount (-1.0 to 1.0)
     */
    void addModulation(const std::string& source, const std::string& destination, float amount);
    
    /**
     * @brief Remove a modulation routing
     * 
     * @param source Source parameter ID
     * @param destination Destination parameter ID
     */
    void removeModulation(const std::string& source, const std::string& destination);
    
    /**
     * @brief Get all modulation routings
     * 
     * @return std::vector<ModulationRouting> List of all modulation routings
     */
    std::vector<ModulationRouting> getAllModulations() const;
    
    /**
     * @brief Set all modulation routings at once
     * 
     * @param modulations List of modulation routings
     */
    void setAllModulations(const std::vector<ModulationRouting>& modulations);
    
    /**
     * @brief Set callback for parameter change notification
     * 
     * @param callback Function to call when a parameter changes
     */
    using ParameterChangedCallback = std::function<void(const std::string&)>;
    void setParameterChangedCallback(ParameterChangedCallback callback);
    
    /**
     * @brief Get a list of all available parameter IDs
     * 
     * @return std::vector<std::string> List of parameter IDs
     */
    std::vector<std::string> getAllParameterIds() const;
    
    /**
     * @brief Get the display name for a parameter
     * 
     * @param parameterId The ID of the parameter
     * @return std::string The display name
     */
    std::string getParameterDisplayName(const std::string& parameterId) const;
    
    /**
     * @brief Format a parameter value for display
     * 
     * @param parameterId The ID of the parameter
     * @param value The parameter value
     * @return std::string The formatted value string
     */
    std::string formatParameterValue(const std::string& parameterId, float value) const;
    
    /**
     * @brief Get parameter range (min/max values)
     * 
     * @param parameterId The ID of the parameter
     * @param min Output: minimum value
     * @param max Output: maximum value
     */
    void getParameterRange(const std::string& parameterId, float& min, float& max) const;

private:
    Synthesizer* synth_;
    std::map<std::string, float> parameters_;
    std::vector<ModulationRouting> modulations_;
    ParameterChangedCallback parameterChangedCallback_;
    
    // Helper methods
    void initializeDefaultParameters();
    void updateSynthesizer(const std::string& parameterId);
    bool isValidParameter(const std::string& parameterId) const;
};

} // namespace AIMusicHardware