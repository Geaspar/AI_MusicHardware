#include "../../../include/ui/presets/PresetManager.h"
#include "../../../include/audio/Synthesizer.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <random>

namespace AIMusicHardware {

namespace fs = std::filesystem;

PresetManager::PresetManager(Synthesizer* synth)
    : synth_(synth),
      currentPresetPath_(""),
      currentPresetName_("Init"),
      currentPresetAuthor_(""),
      currentPresetCategory_(""),
      currentPresetDescription_("") {
    
    // Ensure preset directories exist
    ensureDirectoryExists(getUserPresetsDirectory());
}

PresetManager::~PresetManager() {
    // Nothing specific to clean up
}

bool PresetManager::loadPreset(const std::string& filePath) {
    try {
        // Read the file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to open preset file: " << filePath << std::endl;
            return false;
        }
        
        // Parse JSON
        nlohmann::json presetData;
        file >> presetData;
        file.close();
        
        // Extract metadata
        if (presetData.contains("metadata")) {
            auto& metadata = presetData["metadata"];
            currentPresetName_ = metadata.value("name", "Unnamed");
            currentPresetAuthor_ = metadata.value("author", "");
            currentPresetCategory_ = metadata.value("category", "");
            currentPresetDescription_ = metadata.value("description", "");
        }
        
        // Apply parameters to synthesizer (with schema/migration awareness)
        if (presetData.contains("schema_version")) {
            // Future: handle migrations based on version string
        }
        if (presetData.contains("parameters")) {
            if (!deserializeParameters(presetData["parameters"])) {
                return false;
            }
        }
        
        // Update current preset path
        currentPresetPath_ = filePath;
        
        // Notify listeners
        notifyPresetLoaded(filePath);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading preset: " << e.what() << std::endl;
        return false;
    }
}

bool PresetManager::savePreset(const std::string& filePath, const std::string& name,
                              const std::string& author, const std::string& category,
                              const std::string& description) {
    try {
        if (!synth_) {
            std::cerr << "Cannot save preset: no synthesizer attached" << std::endl;
            return false;
        }
        
        // Create the preset JSON structure
        nlohmann::json presetData;
        
        // Add schema + metadata (with uuid and timestamps)
        presetData["schema_version"] = "1.0.0";
        // Generate a simple UUID-like id
        auto genHex = [] (int n) {
            static const char* kHex = "0123456789abcdef";
            std::string s; s.reserve(n);
            std::random_device rd; std::mt19937 rng(rd()); std::uniform_int_distribution<int> d(0,15);
            for (int i=0;i<n;++i) s.push_back(kHex[d(rng)]);
            return s;
        };
        std::string uuid = genHex(8) + std::string("-") + genHex(4) + std::string("-") + genHex(4) + std::string("-") + genHex(4) + std::string("-") + genHex(12);
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        presetData["metadata"] = {
            {"name", name},
            {"author", author},
            {"category", category},
            {"description", description},
            {"version", "1.0.0"},
            {"uuid", uuid},
            {"created_at", static_cast<int64_t>(t)},
            {"modified_at", static_cast<int64_t>(t)}
        };
        
        // Add parameters
        presetData["parameters"] = serializeParameters();
        
        // Ensure directory exists
        fs::path path(filePath);
        ensureDirectoryExists(path.parent_path().string());
        
        // Write to file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to create preset file: " << filePath << std::endl;
            return false;
        }
        
        file << presetData.dump(4); // Pretty print with 4-space indent
        file.close();
        
        // Update current preset info
        currentPresetPath_ = filePath;
        currentPresetName_ = name;
        currentPresetAuthor_ = author;
        currentPresetCategory_ = category;
        currentPresetDescription_ = description;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving preset: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> PresetManager::getAllPresets() const {
    std::vector<std::string> allPresets;
    
    // Gather presets from all directories
    for (const auto& dir : getAllPresetDirectories()) {
        auto dirPresets = getPresetsInDirectory(dir);
        allPresets.insert(allPresets.end(), dirPresets.begin(), dirPresets.end());
    }
    
    // Sort alphabetically by filename
    std::sort(allPresets.begin(), allPresets.end(), [](const std::string& a, const std::string& b) {
        return fs::path(a).filename().string() < fs::path(b).filename().string();
    });
    
    return allPresets;
}

std::vector<std::string> PresetManager::getPresetsByCategory(const std::string& category) const {
    std::vector<std::string> matchingPresets;
    
    // Get all presets
    auto allPresets = getAllPresets();
    
    // Filter by category
    for (const auto& presetPath : allPresets) {
        try {
            // Read the file
            std::ifstream file(presetPath);
            if (!file.is_open()) continue;
            
            // Parse JSON
            nlohmann::json presetData;
            file >> presetData;
            file.close();
            
            // Check category
            if (presetData.contains("metadata") && 
                presetData["metadata"].contains("category") &&
                presetData["metadata"]["category"] == category) {
                matchingPresets.push_back(presetPath);
            }
            
        } catch (const std::exception& e) {
            // Skip problematic presets
            continue;
        }
    }
    
    return matchingPresets;
}

std::vector<std::string> PresetManager::searchPresets(const std::string& query) const {
    std::vector<std::string> matchingPresets;
    if (query.empty()) {
        return getAllPresets(); // Return all presets if query is empty
    }
    
    // Convert query to lowercase for case-insensitive search
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), 
                  [](unsigned char c){ return std::tolower(c); });
    
    // Get all presets
    auto allPresets = getAllPresets();
    
    // Search in metadata
    for (const auto& presetPath : allPresets) {
        try {
            // Read the file
            std::ifstream file(presetPath);
            if (!file.is_open()) continue;
            
            // Parse JSON
            nlohmann::json presetData;
            file >> presetData;
            file.close();
            
            // Check name, author, description
            bool matches = false;
            
            if (presetData.contains("metadata")) {
                auto& metadata = presetData["metadata"];
                
                // Helper lambda for case-insensitive search
                auto containsIgnoreCase = [&lowerQuery](const std::string& text) {
                    std::string lower = text;
                    std::transform(lower.begin(), lower.end(), lower.begin(),
                                  [](unsigned char c){ return std::tolower(c); });
                    return lower.find(lowerQuery) != std::string::npos;
                };
                
                // Check name
                if (metadata.contains("name") && containsIgnoreCase(metadata["name"])) {
                    matches = true;
                }
                
                // Check author
                if (!matches && metadata.contains("author") && 
                    containsIgnoreCase(metadata["author"])) {
                    matches = true;
                }
                
                // Check category
                if (!matches && metadata.contains("category") && 
                    containsIgnoreCase(metadata["category"])) {
                    matches = true;
                }
                
                // Check description
                if (!matches && metadata.contains("description") && 
                    containsIgnoreCase(metadata["description"])) {
                    matches = true;
                }
            }
            
            // Also check filename
            if (!matches) {
                std::string filename = fs::path(presetPath).filename().string();
                std::transform(filename.begin(), filename.end(), filename.begin(),
                              [](unsigned char c){ return std::tolower(c); });
                if (filename.find(lowerQuery) != std::string::npos) {
                    matches = true;
                }
            }
            
            if (matches) {
                matchingPresets.push_back(presetPath);
            }
            
        } catch (const std::exception& e) {
            // Skip problematic presets
            continue;
        }
    }
    
    return matchingPresets;
}

bool PresetManager::loadNextPreset() {
    // Get all presets
    auto allPresets = getAllPresets();
    if (allPresets.empty()) return false;
    
    // Find current preset in the list
    auto it = std::find(allPresets.begin(), allPresets.end(), currentPresetPath_);
    
    // If found and not the last preset, load the next one
    if (it != allPresets.end() && it + 1 != allPresets.end()) {
        return loadPreset(*(it + 1));
    } 
    // If not found or at the end, load the first preset
    else {
        return loadPreset(allPresets.front());
    }
}

bool PresetManager::loadPreviousPreset() {
    // Get all presets
    auto allPresets = getAllPresets();
    if (allPresets.empty()) return false;
    
    // Find current preset in the list
    auto it = std::find(allPresets.begin(), allPresets.end(), currentPresetPath_);
    
    // If found and not the first preset, load the previous one
    if (it != allPresets.end() && it != allPresets.begin()) {
        return loadPreset(*(it - 1));
    } 
    // If not found or at the beginning, load the last preset
    else {
        return loadPreset(allPresets.back());
    }
}

std::string PresetManager::getCurrentPresetName() const {
    return currentPresetName_;
}

std::string PresetManager::getCurrentPresetPath() const {
    return currentPresetPath_;
}

std::string PresetManager::getCurrentPresetAuthor() const {
    return currentPresetAuthor_;
}

std::string PresetManager::getCurrentPresetCategory() const {
    return currentPresetCategory_;
}

std::string PresetManager::getCurrentPresetDescription() const {
    return currentPresetDescription_;
}

std::string PresetManager::getFactoryPresetsDirectory() {
    return (fs::current_path() / "presets" / "factory").string();
}

std::string PresetManager::getUserPresetsDirectory() {
    return (fs::current_path() / "presets" / "user").string();
}

std::vector<std::string> PresetManager::getAllPresetDirectories() {
    return {getFactoryPresetsDirectory(), getUserPresetsDirectory()};
}

std::vector<std::string> PresetManager::getAvailableCategories() {
    return {
        "Bass", 
        "Lead", 
        "Pad", 
        "Keys", 
        "Pluck", 
        "Percussion", 
        "FX", 
        "Other"
    };
}

void PresetManager::addPresetLoadedCallback(std::function<void(const std::string&)> callback) {
    presetLoadedCallbacks_.push_back(callback);
}

nlohmann::json PresetManager::serializeParameters() const {
    nlohmann::json parameters;
    
    // Get all parameters from synthesizer
    if (synth_) {
        auto allParams = synth_->getAllParameters();
        for (const auto& [name, value] : allParams) {
            parameters[name] = value;
        }
    }
    
    return parameters;
}

bool PresetManager::deserializeParameters(const nlohmann::json& json) {
    if (!synth_) return false;
    
    // Create a map of parameter name to value
    std::map<std::string, float> paramMap;
    
    for (auto it = json.begin(); it != json.end(); ++it) {
        std::string key = it.key();
        float value = it.value();
        
        // Map legacy envelope keys to current identifiers
        if (key == "env_attack") key = "envelope_attack";
        else if (key == "env_decay") key = "envelope_decay";
        else if (key == "env_sustain") key = "envelope_sustain";
        else if (key == "env_release") key = "envelope_release";
        else if (key == "attack") key = "envelope_attack";
        else if (key == "decay") key = "envelope_decay";
        else if (key == "sustain") key = "envelope_sustain";
        else if (key == "release") key = "envelope_release";

        // Optional: map legacy filter keys that were absolute Hz to normalized if needed
        if (key == "filter_cutoff" && value > 1.0f) {
            // Convert Hz (20-20000) to normalized 0-1 approximately (log scale 20Hz to 20kHz)
            float hz = std::clamp(value, 20.0f, 20000.0f);
            float norm = std::log10(hz / 20.0f) / 3.0f; // since 20*10^3 = 20000
            value = std::clamp(norm, 0.0f, 1.0f);
        }

        paramMap[key] = value;
    }
    
    // Apply all parameters at once
    synth_->setAllParameters(paramMap);
    
    return true;
}

bool PresetManager::ensureDirectoryExists(const std::string& path) const {
    try {
        if (!fs::exists(path)) {
            return fs::create_directories(path);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating directory " << path << ": " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> PresetManager::getPresetsInDirectory(const std::string& directory) const {
    std::vector<std::string> presets;
    
    try {
        if (!fs::exists(directory)) return presets;
        
        for (const auto& entry : fs::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".preset") {
                presets.push_back(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory " << directory << ": " << e.what() << std::endl;
    }
    
    return presets;
}

void PresetManager::notifyPresetLoaded(const std::string& presetPath) {
    for (const auto& callback : presetLoadedCallbacks_) {
        if (callback) {
            callback(presetPath);
        }
    }
}

} // namespace AIMusicHardware