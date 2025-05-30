#include "../../include/midi/MidiCCLearning.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

namespace AIMusicHardware {

MidiCCLearning::MidiCCLearning() {
    std::cout << "MIDI CC Learning system initialized" << std::endl;
}

MidiCCLearning::~MidiCCLearning() {
    // Auto-save mappings on destruction
    try {
        saveMappings(getDefaultMappingsPath());
    } catch (...) {
        // Don't throw in destructor
    }
}

bool MidiCCLearning::startLearning(const std::string& parameterId, 
                                  std::chrono::milliseconds timeout) {
    if (!enabled_.load()) {
        return false;
    }
    
    // Check if parameter already has a mapping
    {
        std::lock_guard<std::mutex> lock(mappingsMutex_);
        if (parameterToCC_.find(parameterId) != parameterToCC_.end()) {
            std::cout << "Parameter " << parameterId << " already has a mapping" << std::endl;
            return false;
        }
    }
    
    currentParameterId_ = parameterId;
    learningStartTime_ = std::chrono::system_clock::now();
    learningTimeout_ = timeout;
    
    updateLearningState(LearningState::WaitingForCC, 
                       "Move a controller to map to " + parameterId);
    
    std::cout << "Started learning for parameter: " << parameterId << std::endl;
    return true;
}

bool MidiCCLearning::startAutoLearning(std::chrono::milliseconds duration) {
    if (!enabled_.load()) {
        return false;
    }
    
    // Clear previous activity tracking
    ccActivity_.clear();
    learningStartTime_ = std::chrono::system_clock::now();
    learningTimeout_ = duration;
    
    updateLearningState(LearningState::Learning, 
                       "Auto-learning active - use your controllers");
    
    std::cout << "Started auto-learning for " << duration.count() << "ms" << std::endl;
    return true;
}

void MidiCCLearning::stopLearning() {
    currentParameterId_.clear();
    updateLearningState(LearningState::Idle, "Learning stopped");
    std::cout << "Learning stopped" << std::endl;
}

void MidiCCLearning::processMidiCC(int channel, int ccNumber, int value, const std::string& deviceName) {
    if (!enabled_.load()) {
        return;
    }
    
    // Update statistics
    updateStatistics(channel, ccNumber);
    
    LearningState state = learningState_.load();
    
    switch (state) {
        case LearningState::WaitingForCC:
            processLearningCC(channel, ccNumber, value, deviceName);
            break;
            
        case LearningState::Learning:
            if (learningMode_ == LearningMode::Auto) {
                updateCCActivity(channel, ccNumber, value, deviceName);
                if (isLearningTimedOut()) {
                    processAutoLearning();
                }
            }
            break;
            
        case LearningState::Idle:
            processNormalCC(channel, ccNumber, value);
            break;
            
        default:
            break;
    }
}

void MidiCCLearning::processLearningCC(int channel, int ccNumber, int value, const std::string& deviceName) {
    if (isLearningTimedOut()) {
        updateLearningState(LearningState::Idle, "Learning timed out");
        return;
    }
    
    // Check if CC movement is significant enough
    auto activityKey = std::make_pair(channel, ccNumber);
    auto it = ccActivity_.find(activityKey);
    
    if (it != ccActivity_.end()) {
        int change = std::abs(value - it->second.lastValue);
        if (change < learningSensitivity_) {
            return; // Not enough movement
        }
    }
    
    // Create the mapping
    completeMapping(channel, ccNumber, deviceName);
}

void MidiCCLearning::processNormalCC(int channel, int ccNumber, int value) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    auto mappingKey = std::make_pair(channel, ccNumber);
    auto it = mappings_.find(mappingKey);
    
    // Also check for "any channel" mappings
    if (it == mappings_.end()) {
        mappingKey = std::make_pair(-1, ccNumber);
        it = mappings_.find(mappingKey);
    }
    
    if (it != mappings_.end() && it->second.isActive) {
        const CCMapping& mapping = it->second;
        float paramValue = convertCCValue(value, mapping);
        
        // Apply smoothing if configured
        if (mapping.smoothing > 0.0f) {
            // In a real implementation, we'd track previous values for smoothing
            // For now, apply simple smoothing concept
        }
        
        notifyParameterChange(mapping.parameterId, paramValue);
    }
}

float MidiCCLearning::convertCCValue(int ccValue, const CCMapping& mapping) const {
    // Normalize CC value to 0-1
    float normalized = static_cast<float>(ccValue) / 127.0f;
    
    // Invert if needed
    if (mapping.inverted) {
        normalized = 1.0f - normalized;
    }
    
    // Apply curve
    normalized = applyCurve(normalized, mapping.curveType);
    
    // Scale to parameter range
    return mapping.minValue + normalized * (mapping.maxValue - mapping.minValue);
}

float MidiCCLearning::applyCurve(float normalizedValue, CCMapping::CurveType curveType) const {
    switch (curveType) {
        case CCMapping::CurveType::Linear:
            return normalizedValue;
            
        case CCMapping::CurveType::Exponential:
            return normalizedValue * normalizedValue;
            
        case CCMapping::CurveType::Logarithmic:
            return std::sqrt(normalizedValue);
            
        case CCMapping::CurveType::SShape:
            // S-curve using smoothstep function
            return normalizedValue * normalizedValue * (3.0f - 2.0f * normalizedValue);
            
        default:
            return normalizedValue;
    }
}

bool MidiCCLearning::createMapping(const CCMapping& mapping) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    auto mappingKey = std::make_pair(mapping.channel, mapping.ccNumber);
    
    // Check for conflicts
    if (mappings_.find(mappingKey) != mappings_.end()) {
        std::cout << "Warning: Overwriting existing mapping for CC " 
                  << mapping.ccNumber << " on channel " << mapping.channel << std::endl;
    }
    
    // Remove any existing mapping for this parameter
    auto paramIt = parameterToCC_.find(mapping.parameterId);
    if (paramIt != parameterToCC_.end()) {
        mappings_.erase(paramIt->second);
        parameterToCC_.erase(paramIt);
    }
    
    // Add new mapping
    mappings_[mappingKey] = mapping;
    parameterToCC_[mapping.parameterId] = mappingKey;
    
    // Update statistics
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.totalMappings++;
        stats_.activeMappings++;
    }
    
    // Notify callback
    if (mappingCreatedCallback_) {
        mappingCreatedCallback_(mapping);
    }
    
    std::cout << "Created mapping: CC" << mapping.ccNumber 
              << " (ch " << mapping.channel << ") -> " << mapping.parameterId << std::endl;
    
    return true;
}

bool MidiCCLearning::removeMapping(int channel, int ccNumber) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    auto mappingKey = std::make_pair(channel, ccNumber);
    auto it = mappings_.find(mappingKey);
    
    if (it != mappings_.end()) {
        // Remove from parameter lookup
        parameterToCC_.erase(it->second.parameterId);
        mappings_.erase(it);
        
        // Update statistics
        {
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            stats_.activeMappings--;
        }
        
        std::cout << "Removed mapping for CC" << ccNumber 
                  << " on channel " << channel << std::endl;
        return true;
    }
    
    return false;
}

bool MidiCCLearning::removeMapping(const std::string& parameterId) {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    auto it = parameterToCC_.find(parameterId);
    if (it != parameterToCC_.end()) {
        mappings_.erase(it->second);
        parameterToCC_.erase(it);
        
        // Update statistics
        {
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            stats_.activeMappings--;
        }
        
        std::cout << "Removed mapping for parameter " << parameterId << std::endl;
        return true;
    }
    
    return false;
}

const MidiCCLearning::CCMapping* MidiCCLearning::getMapping(int channel, int ccNumber) const {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    auto mappingKey = std::make_pair(channel, ccNumber);
    auto it = mappings_.find(mappingKey);
    
    if (it != mappings_.end()) {
        return &it->second;
    }
    
    // Check for "any channel" mapping
    mappingKey = std::make_pair(-1, ccNumber);
    it = mappings_.find(mappingKey);
    
    if (it != mappings_.end()) {
        return &it->second;
    }
    
    return nullptr;
}

const MidiCCLearning::CCMapping* MidiCCLearning::getMapping(const std::string& parameterId) const {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    auto it = parameterToCC_.find(parameterId);
    if (it != parameterToCC_.end()) {
        auto mappingIt = mappings_.find(it->second);
        if (mappingIt != mappings_.end()) {
            return &mappingIt->second;
        }
    }
    
    return nullptr;
}

std::vector<MidiCCLearning::CCMapping> MidiCCLearning::getAllMappings() const {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    std::vector<CCMapping> result;
    result.reserve(mappings_.size());
    
    for (const auto& pair : mappings_) {
        result.push_back(pair.second);
    }
    
    return result;
}

void MidiCCLearning::clearAllMappings() {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    mappings_.clear();
    parameterToCC_.clear();
    
    // Update statistics
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.activeMappings = 0;
    }
    
    std::cout << "Cleared all MIDI CC mappings" << std::endl;
}

void MidiCCLearning::updateLearningState(LearningState newState, const std::string& message) {
    learningState_.store(newState);
    
    if (learningStateCallback_) {
        learningStateCallback_(newState, message);
    }
    
    std::cout << "Learning state: " << message << std::endl;
}

void MidiCCLearning::notifyParameterChange(const std::string& parameterId, float value) {
    if (parameterChangeCallback_) {
        parameterChangeCallback_(parameterId, value);
    }
}

void MidiCCLearning::updateStatistics(int channel, int ccNumber) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    stats_.messagesProcessed++;
    stats_.lastActivity = std::chrono::system_clock::now();
    stats_.ccUsageCount[ccNumber]++;
}

bool MidiCCLearning::isLearningTimedOut() const {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - learningStartTime_);
    return elapsed >= learningTimeout_;
}

void MidiCCLearning::completeMapping(int channel, int ccNumber, const std::string& deviceName) {
    CCMapping mapping;
    mapping.channel = channel;
    mapping.ccNumber = ccNumber;
    mapping.parameterId = currentParameterId_;
    mapping.deviceName = deviceName;
    mapping.learnTime = std::chrono::system_clock::now();
    
    // Auto-detect optimal curve if enabled
    if (autoCurveDetection_) {
        mapping.curveType = detectOptimalCurve(currentParameterId_);
    }
    
    // Create the mapping
    createMapping(mapping);
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.learningSessionsCompleted++;
    }
    
    updateLearningState(LearningState::Idle, 
                       "Mapped CC" + std::to_string(ccNumber) + " to " + currentParameterId_);
    
    currentParameterId_.clear();
}

MidiCCLearning::CCMapping::CurveType MidiCCLearning::detectOptimalCurve(const std::string& parameterId) const {
    // Intelligent curve detection based on parameter type
    std::string paramLower = parameterId;
    std::transform(paramLower.begin(), paramLower.end(), paramLower.begin(), ::tolower);
    
    if (paramLower.find("frequency") != std::string::npos ||
        paramLower.find("cutoff") != std::string::npos ||
        paramLower.find("pitch") != std::string::npos) {
        return CCMapping::CurveType::Exponential; // Musical frequency response
    }
    
    if (paramLower.find("volume") != std::string::npos ||
        paramLower.find("gain") != std::string::npos ||
        paramLower.find("level") != std::string::npos) {
        return CCMapping::CurveType::Logarithmic; // Perceptual volume response
    }
    
    if (paramLower.find("resonance") != std::string::npos ||
        paramLower.find("filter") != std::string::npos) {
        return CCMapping::CurveType::SShape; // Musical filter response
    }
    
    return CCMapping::CurveType::Linear; // Default
}

void MidiCCLearning::updateCCActivity(int channel, int ccNumber, int value, const std::string& deviceName) {
    auto activityKey = std::make_pair(channel, ccNumber);
    auto& activity = ccActivity_[activityKey];
    
    if (activity.lastValue != -1) {
        int change = std::abs(value - activity.lastValue);
        if (change >= learningSensitivity_) {
            activity.changeCount++;
        }
    }
    
    activity.lastValue = value;
    activity.lastActivity = std::chrono::system_clock::now();
    activity.deviceName = deviceName;
}

void MidiCCLearning::processAutoLearning() {
    auto activeCCs = getActiveCCs();
    
    updateLearningState(LearningState::Idle, 
                       "Auto-learning completed. Found " + std::to_string(activeCCs.size()) + " active CCs");
    
    // In a real implementation, we would present these CCs to the user
    // for parameter assignment or create intelligent auto-mappings
    std::cout << "Auto-learning found " << activeCCs.size() << " active CCs:" << std::endl;
    for (const auto& cc : activeCCs) {
        std::cout << "  CC" << cc.second << " on channel " << cc.first << std::endl;
    }
}

std::vector<std::pair<int, int>> MidiCCLearning::getActiveCCs() const {
    std::vector<std::pair<int, int>> result;
    
    for (const auto& activity : ccActivity_) {
        if (activity.second.changeCount >= 3) { // Require some movement
            result.push_back(activity.first);
        }
    }
    
    return result;
}

bool MidiCCLearning::saveMappings(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(mappingsMutex_);
    
    try {
        nlohmann::json json;
        json["version"] = "1.0";
        json["mappings"] = nlohmann::json::array();
        
        for (const auto& pair : mappings_) {
            const CCMapping& mapping = pair.second;
            nlohmann::json mappingJson;
            
            mappingJson["channel"] = mapping.channel;
            mappingJson["ccNumber"] = mapping.ccNumber;
            mappingJson["parameterId"] = mapping.parameterId;
            mappingJson["minValue"] = mapping.minValue;
            mappingJson["maxValue"] = mapping.maxValue;
            mappingJson["inverted"] = mapping.inverted;
            mappingJson["smoothing"] = mapping.smoothing;
            mappingJson["curveType"] = static_cast<int>(mapping.curveType);
            mappingJson["deviceName"] = mapping.deviceName;
            mappingJson["isActive"] = mapping.isActive;
            
            json["mappings"].push_back(mappingJson);
        }
        
        std::ofstream file(filePath);
        file << json.dump(2);
        
        std::cout << "Saved " << mappings_.size() << " mappings to " << filePath << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to save mappings: " << e.what() << std::endl;
        return false;
    }
}

bool MidiCCLearning::loadMappings(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cout << "No existing mappings file found at " << filePath << std::endl;
            return false;
        }
        
        nlohmann::json json;
        file >> json;
        
        std::lock_guard<std::mutex> lock(mappingsMutex_);
        
        // Clear existing mappings
        mappings_.clear();
        parameterToCC_.clear();
        
        for (const auto& mappingJson : json["mappings"]) {
            CCMapping mapping;
            mapping.channel = mappingJson["channel"];
            mapping.ccNumber = mappingJson["ccNumber"];
            mapping.parameterId = mappingJson["parameterId"];
            mapping.minValue = mappingJson["minValue"];
            mapping.maxValue = mappingJson["maxValue"];
            mapping.inverted = mappingJson["inverted"];
            mapping.smoothing = mappingJson["smoothing"];
            mapping.curveType = static_cast<CCMapping::CurveType>(mappingJson["curveType"]);
            mapping.deviceName = mappingJson["deviceName"];
            mapping.isActive = mappingJson["isActive"];
            
            auto mappingKey = std::make_pair(mapping.channel, mapping.ccNumber);
            mappings_[mappingKey] = mapping;
            parameterToCC_[mapping.parameterId] = mappingKey;
        }
        
        // Update statistics
        {
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            stats_.activeMappings = static_cast<int>(mappings_.size());
        }
        
        std::cout << "Loaded " << mappings_.size() << " mappings from " << filePath << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load mappings: " << e.what() << std::endl;
        return false;
    }
}

std::string MidiCCLearning::getDefaultMappingsPath() {
    return "midi_cc_mappings.json";
}

MidiCCLearning::LearningStats MidiCCLearning::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

void MidiCCLearning::resetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex_);
    stats_ = LearningStats{};
    
    // Preserve active mapping count
    std::lock_guard<std::mutex> mappingsLock(mappingsMutex_);
    stats_.activeMappings = static_cast<int>(mappings_.size());
}

// Singleton manager implementation
MidiCCLearningManager& MidiCCLearningManager::getInstance() {
    static MidiCCLearningManager instance;
    return instance;
}

void MidiCCLearningManager::initialize() {
    if (initialized_) {
        return;
    }
    
    // Load existing mappings
    learning_.loadMappings(MidiCCLearning::getDefaultMappingsPath());
    
    // Set up callbacks for integration
    learning_.setParameterChangeCallback([](const std::string& parameterId, float value) {
        std::cout << "CC -> Parameter: " << parameterId << " = " << value << std::endl;
        // In real implementation, this would update the actual parameter
    });
    
    learning_.setMappingCreatedCallback([](const MidiCCLearning::CCMapping& mapping) {
        std::cout << "New mapping created: CC" << mapping.ccNumber 
                  << " -> " << mapping.parameterId << std::endl;
    });
    
    initialized_ = true;
    std::cout << "MIDI CC Learning Manager initialized" << std::endl;
}

void MidiCCLearningManager::shutdown() {
    if (initialized_) {
        learning_.saveMappings(MidiCCLearning::getDefaultMappingsPath());
        initialized_ = false;
        std::cout << "MIDI CC Learning Manager shut down" << std::endl;
    }
}

} // namespace AIMusicHardware