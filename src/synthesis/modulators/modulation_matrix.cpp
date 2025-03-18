#include "../../../include/synthesis/modulators/modulation_matrix.h"

namespace AIMusicHardware {

// ModulationSource implementation
ModulationSource::ModulationSource(const std::string& name)
    : name_(name), bipolar_(true) {
}

ModulationSource::~ModulationSource() {
}

// ModulationDestination implementation
ModulationDestination::ModulationDestination(const std::string& name,
                                             SetterFunc setter,
                                             GetterFunc getter,
                                             float minValue,
                                             float maxValue)
    : name_(name),
      setter_(setter),
      getter_(getter),
      minValue_(minValue),
      maxValue_(maxValue) {
}

ModulationDestination::~ModulationDestination() {
}

float ModulationDestination::getBaseValue() const {
    return getter_();
}

void ModulationDestination::setBaseValue(float value) {
    // Ensure the value is within range
    float clampedValue = std::clamp(value, minValue_, maxValue_);
    setter_(clampedValue);
}

void ModulationDestination::update() {
    // Base implementation - just set the parameter to its base value
    // The ModulationMatrix will handle applying modulation
    setter_(getter_());
}

// ModulationConnection implementation
ModulationConnection::ModulationConnection(ModulationSource* source,
                                           ModulationDestination* destination,
                                           float amount)
    : source_(source),
      destination_(destination),
      amount_(amount) {
}

ModulationConnection::~ModulationConnection() {
}

float ModulationConnection::calculateModulation() const {
    if (!source_ || !destination_) {
        return 0.0f;
    }
    
    // Get the current value from the source
    float sourceValue = source_->getValue();
    
    // For unipolar sources (0 to 1), convert to bipolar (-1 to 1) if needed
    if (!source_->isBipolar()) {
        sourceValue = sourceValue * 2.0f - 1.0f;
    }
    
    // Calculate modulation amount
    return sourceValue * amount_;
}

// ModulationMatrix implementation
ModulationMatrix::ModulationMatrix() {
}

ModulationMatrix::~ModulationMatrix() {
}

void ModulationMatrix::addSource(std::unique_ptr<ModulationSource> source) {
    if (source) {
        const std::string& name = source->getName();
        sourceMap_[name] = source.get();
        sources_.push_back(std::move(source));
    }
}

ModulationSource* ModulationMatrix::getSource(const std::string& name) {
    auto it = sourceMap_.find(name);
    return (it != sourceMap_.end()) ? it->second : nullptr;
}

void ModulationMatrix::addDestination(std::unique_ptr<ModulationDestination> destination) {
    if (destination) {
        const std::string& name = destination->getName();
        destinationMap_[name] = destination.get();
        destinations_.push_back(std::move(destination));
    }
}

ModulationDestination* ModulationMatrix::getDestination(const std::string& name) {
    auto it = destinationMap_.find(name);
    return (it != destinationMap_.end()) ? it->second : nullptr;
}

void ModulationMatrix::connect(const std::string& sourceName,
                               const std::string& destinationName,
                               float amount) {
    ModulationSource* source = getSource(sourceName);
    ModulationDestination* destination = getDestination(destinationName);
    
    if (source && destination) {
        // Check if the connection already exists
        for (auto& conn : connections_) {
            if (conn->getSource() == source && conn->getDestination() == destination) {
                // Update existing connection
                conn->setAmount(amount);
                return;
            }
        }
        
        // Create new connection
        connections_.push_back(
            std::make_unique<ModulationConnection>(source, destination, amount));
    }
}

void ModulationMatrix::disconnect(const std::string& sourceName,
                                 const std::string& destinationName) {
    ModulationSource* source = getSource(sourceName);
    ModulationDestination* destination = getDestination(destinationName);
    
    if (source && destination) {
        // Find and remove the connection
        for (auto it = connections_.begin(); it != connections_.end(); ++it) {
            if ((*it)->getSource() == source && (*it)->getDestination() == destination) {
                connections_.erase(it);
                return;
            }
        }
    }
}

void ModulationMatrix::update() {
    // First update all sources
    for (auto& source : sources_) {
        source->update();
    }
    
    // Create a map to hold modulation amounts by destination
    std::unordered_map<ModulationDestination*, float> modulations;
    
    // Calculate all modulations
    for (auto& connection : connections_) {
        ModulationDestination* dest = connection->getDestination();
        float mod = connection->calculateModulation();
        
        // Accumulate modulations for this destination
        modulations[dest] += mod;
    }
    
    // Apply modulations to all destinations
    for (auto& destPair : modulations) {
        ModulationDestination* dest = destPair.first;
        float mod = destPair.second;
        
        // Get the current base value
        float baseValue = dest->getBaseValue();
        
        // Calculate the range of the destination
        float range = dest->getMaxValue() - dest->getMinValue();
        
        // Apply modulation - modulates by a percentage of the range
        float modAmount = range * mod;
        float newValue = baseValue + modAmount;
        
        // Clamp to destination range
        newValue = std::clamp(newValue, dest->getMinValue(), dest->getMaxValue());
        
        // Set the new value
        dest->setBaseValue(newValue);
    }
    
    // Update all destinations
    for (auto& dest : destinations_) {
        dest->update();
    }
}

} // namespace AIMusicHardware