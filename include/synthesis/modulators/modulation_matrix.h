#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace AIMusicHardware {

/**
 * ModulationSource is a base class for all modulation sources.
 */
class ModulationSource {
public:
    ModulationSource(const std::string& name);
    virtual ~ModulationSource();
    
    // Get current value (-1.0 to 1.0 for bipolar, 0.0 to 1.0 for unipolar)
    virtual float getValue() const = 0;
    
    // Update the modulation source
    virtual void update() = 0;
    
    // Get name for this source
    const std::string& getName() const { return name_; }
    
    // Bipolar/Unipolar setting
    bool isBipolar() const { return bipolar_; }
    void setBipolar(bool bipolar) { bipolar_ = bipolar; }
    
private:
    std::string name_;
    bool bipolar_;
};

/**
 * ModulationDestination represents a target parameter that can be modulated.
 */
class ModulationDestination {
public:
    using SetterFunc = std::function<void(float)>;
    using GetterFunc = std::function<float()>;
    
    ModulationDestination(const std::string& name, 
                          SetterFunc setter, 
                          GetterFunc getter,
                          float minValue = 0.0f,
                          float maxValue = 1.0f);
    ~ModulationDestination();
    
    // Get/set base value without modulation
    float getBaseValue() const;
    void setBaseValue(float value);
    
    // Get/set min/max allowed values
    float getMinValue() const { return minValue_; }
    float getMaxValue() const { return maxValue_; }
    void setRange(float min, float max) { minValue_ = min; maxValue_ = max; }
    
    // Apply modulation and update the target parameter
    void update();
    
    // Get name for this destination
    const std::string& getName() const { return name_; }
    
private:
    std::string name_;
    SetterFunc setter_;
    GetterFunc getter_;
    float minValue_;
    float maxValue_;
};

/**
 * ModulationConnection connects a source to a destination with scaling.
 */
class ModulationConnection {
public:
    ModulationConnection(ModulationSource* source, 
                         ModulationDestination* destination,
                         float amount = 1.0f);
    ~ModulationConnection();
    
    // Calculate the modulation amount
    float calculateModulation() const;
    
    // Get/set the amount of modulation (-1.0 to 1.0)
    float getAmount() const { return amount_; }
    void setAmount(float amount) { amount_ = amount; }
    
    // Access source and destination
    ModulationSource* getSource() const { return source_; }
    ModulationDestination* getDestination() const { return destination_; }
    
private:
    ModulationSource* source_;
    ModulationDestination* destination_;
    float amount_;
};

/**
 * ModulationMatrix manages all connections between sources and destinations.
 */
class ModulationMatrix {
public:
    ModulationMatrix();
    ~ModulationMatrix();
    
    // Source management
    void addSource(std::unique_ptr<ModulationSource> source);
    ModulationSource* getSource(const std::string& name);
    
    // Destination management
    void addDestination(std::unique_ptr<ModulationDestination> destination);
    ModulationDestination* getDestination(const std::string& name);
    
    // Connection management
    void connect(const std::string& sourceName, 
                 const std::string& destinationName, 
                 float amount = 1.0f);
    void disconnect(const std::string& sourceName, 
                   const std::string& destinationName);
    
    // Update all modulation sources and connections
    void update();
    
private:
    std::vector<std::unique_ptr<ModulationSource>> sources_;
    std::unordered_map<std::string, ModulationSource*> sourceMap_;
    
    std::vector<std::unique_ptr<ModulationDestination>> destinations_;
    std::unordered_map<std::string, ModulationDestination*> destinationMap_;
    
    std::vector<std::unique_ptr<ModulationConnection>> connections_;
};

} // namespace AIMusicHardware