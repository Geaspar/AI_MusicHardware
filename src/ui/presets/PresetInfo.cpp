#include "../../../include/ui/presets/PresetInfo.h"
#include <fstream>
#include <filesystem>
#include <sstream>

namespace AIMusicHardware {

nlohmann::json PresetInfo::toJson() const {
    nlohmann::json json;
    
    // Core identification
    json["name"] = name;
    json["filePath"] = filePath;
    json["category"] = category;
    
    // Authorship
    json["author"] = author;
    json["license"] = license;
    json["description"] = description;
    json["tags"] = tags;
    
    // Temporal information
    json["created"] = std::chrono::duration_cast<std::chrono::seconds>(created.time_since_epoch()).count();
    json["modified"] = std::chrono::duration_cast<std::chrono::seconds>(modified.time_since_epoch()).count();
    json["fileSize"] = fileSize;
    
    // User preferences
    json["isFavorite"] = isFavorite;
    json["userRating"] = userRating;
    json["playCount"] = playCount;
    json["lastAccessed"] = std::chrono::duration_cast<std::chrono::seconds>(lastAccessed.time_since_epoch()).count();
    
    // Audio characteristics
    json["audioCharacteristics"] = {
        {"bassContent", audioCharacteristics.bassContent},
        {"midContent", audioCharacteristics.midContent},
        {"trebleContent", audioCharacteristics.trebleContent},
        {"brightness", audioCharacteristics.brightness},
        {"warmth", audioCharacteristics.warmth},
        {"complexity", audioCharacteristics.complexity},
        {"hasArpeggiator", audioCharacteristics.hasArpeggiator},
        {"hasSequencer", audioCharacteristics.hasSequencer},
        {"modulationCount", audioCharacteristics.modulationCount}
    };
    
    return json;
}

PresetInfo PresetInfo::fromJson(const nlohmann::json& json) {
    PresetInfo info;
    
    // Core identification
    info.name = json.value("name", "");
    info.filePath = json.value("filePath", "");
    info.category = json.value("category", "");
    
    // Authorship
    info.author = json.value("author", "");
    info.license = json.value("license", "");
    info.description = json.value("description", "");
    if (json.contains("tags") && json["tags"].is_array()) {
        info.tags = json["tags"];
    }
    
    // Temporal information
    if (json.contains("created")) {
        auto timestamp = std::chrono::seconds(json["created"].get<long long>());
        info.created = std::chrono::time_point<std::chrono::system_clock>(timestamp);
    }
    if (json.contains("modified")) {
        auto timestamp = std::chrono::seconds(json["modified"].get<long long>());
        info.modified = std::chrono::time_point<std::chrono::system_clock>(timestamp);
    }
    info.fileSize = json.value("fileSize", 0);
    
    // User preferences
    info.isFavorite = json.value("isFavorite", false);
    info.userRating = json.value("userRating", 0);
    info.playCount = json.value("playCount", 0);
    if (json.contains("lastAccessed")) {
        auto timestamp = std::chrono::seconds(json["lastAccessed"].get<long long>());
        info.lastAccessed = std::chrono::time_point<std::chrono::system_clock>(timestamp);
    }
    
    // Audio characteristics
    if (json.contains("audioCharacteristics")) {
        auto& ac = json["audioCharacteristics"];
        info.audioCharacteristics.bassContent = ac.value("bassContent", 0.0f);
        info.audioCharacteristics.midContent = ac.value("midContent", 0.0f);
        info.audioCharacteristics.trebleContent = ac.value("trebleContent", 0.0f);
        info.audioCharacteristics.brightness = ac.value("brightness", 0.0f);
        info.audioCharacteristics.warmth = ac.value("warmth", 0.0f);
        info.audioCharacteristics.complexity = ac.value("complexity", 0.0f);
        info.audioCharacteristics.hasArpeggiator = ac.value("hasArpeggiator", false);
        info.audioCharacteristics.hasSequencer = ac.value("hasSequencer", false);
        info.audioCharacteristics.modulationCount = ac.value("modulationCount", 0);
    }
    
    info.isMetadataCached = true;
    info.needsParameterAnalysis = false;
    
    return info;
}

PresetInfo PresetInfo::fromFile(const std::string& filePath) {
    PresetInfo info;
    info.filePath = filePath;
    
    try {
        // Get basic file information
        if (std::filesystem::exists(filePath)) {
            auto fileTime = std::filesystem::last_write_time(filePath);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
            info.modified = sctp;
            info.created = sctp; // Fallback, will be updated if found in JSON
            info.fileSize = std::filesystem::file_size(filePath);
        }
        
        // Extract name from filename
        std::filesystem::path path(filePath);
        info.name = path.stem().string();
        
        // Try to load metadata from JSON preset file
        std::ifstream file(filePath);
        if (file.is_open()) {
            nlohmann::json presetJson;
            file >> presetJson;
            
            // Extract metadata if present
            if (presetJson.contains("metadata")) {
                auto& metadata = presetJson["metadata"];
                info.author = metadata.value("author", "");
                info.category = metadata.value("category", "");
                info.description = metadata.value("comments", "");
                info.license = metadata.value("license", "");
                
                if (metadata.contains("tags") && metadata["tags"].is_array()) {
                    info.tags = metadata["tags"];
                }
                
                if (metadata.contains("created")) {
                    auto timestamp = std::chrono::seconds(metadata["created"].get<long long>());
                    info.created = std::chrono::time_point<std::chrono::system_clock>(timestamp);
                }
            }
            
            // Analyze preset parameters for audio characteristics
            if (presetJson.contains("parameters")) {
                analyzeAudioCharacteristics(info, presetJson["parameters"]);
            }
            
            // Check for modulation data
            if (presetJson.contains("modulations") && presetJson["modulations"].is_array()) {
                info.audioCharacteristics.modulationCount = presetJson["modulations"].size();
            }
        }
        
        info.isMetadataCached = true;
        info.needsParameterAnalysis = false;
        
    } catch (const std::exception& e) {
        // If parsing fails, return basic info with flags set for retry
        info.isMetadataCached = false;
        info.needsParameterAnalysis = true;
    }
    
    return info;
}

void PresetInfo::analyzeAudioCharacteristics(PresetInfo& info, const nlohmann::json& parameters) {
    // Simple heuristic analysis based on common synthesizer parameters
    // This would be expanded with more sophisticated analysis
    
    auto& ac = info.audioCharacteristics;
    
    // Analyze filter cutoff for brightness
    if (parameters.contains("filter_cutoff")) {
        float cutoff = parameters["filter_cutoff"];
        ac.brightness = std::min(1.0f, cutoff / 10000.0f); // Normalize to 0-1
    }
    
    // Analyze oscillator types for warmth
    if (parameters.contains("osc1_waveform")) {
        int waveform = parameters["osc1_waveform"];
        // Saw = 0, Square = 1, Triangle = 2, Sine = 3 (example mapping)
        ac.warmth = (waveform == 2 || waveform == 3) ? 0.8f : 0.4f;
    }
    
    // Analyze envelope settings for complexity
    float complexity = 0.0f;
    if (parameters.contains("env_attack")) complexity += parameters["env_attack"].get<float>();
    if (parameters.contains("env_decay")) complexity += parameters["env_decay"].get<float>();
    if (parameters.contains("env_sustain")) complexity += parameters["env_sustain"].get<float>();
    if (parameters.contains("env_release")) complexity += parameters["env_release"].get<float>();
    ac.complexity = std::min(1.0f, complexity / 4.0f);
    
    // Check for arpeggiator
    if (parameters.contains("arp_enabled")) {
        ac.hasArpeggiator = parameters["arp_enabled"];
    }
    
    // Simple frequency analysis based on filter and oscillator settings
    if (parameters.contains("filter_cutoff")) {
        float cutoff = parameters["filter_cutoff"];
        if (cutoff < 500) {
            ac.bassContent = 0.8f;
            ac.midContent = 0.3f;
            ac.trebleContent = 0.1f;
        } else if (cutoff < 2000) {
            ac.bassContent = 0.5f;
            ac.midContent = 0.7f;
            ac.trebleContent = 0.4f;
        } else {
            ac.bassContent = 0.2f;
            ac.midContent = 0.6f;
            ac.trebleContent = 0.9f;
        }
    }
}

std::string categoryToString(PresetCategory category) {
    switch (category) {
        case PresetCategory::Bass: return "Bass";
        case PresetCategory::Lead: return "Lead";
        case PresetCategory::Pad: return "Pad";
        case PresetCategory::Keys: return "Keys";
        case PresetCategory::Arp: return "Arp";
        case PresetCategory::Pluck: return "Pluck";
        case PresetCategory::Percussion: return "Percussion";
        case PresetCategory::SFX: return "SFX";
        case PresetCategory::Experimental: return "Experimental";
        case PresetCategory::Template: return "Template";
        case PresetCategory::Custom: return "Custom";
        default: return "Unknown";
    }
}

PresetCategory stringToCategory(const std::string& categoryStr) {
    if (categoryStr == "Bass") return PresetCategory::Bass;
    if (categoryStr == "Lead") return PresetCategory::Lead;
    if (categoryStr == "Pad") return PresetCategory::Pad;
    if (categoryStr == "Keys") return PresetCategory::Keys;
    if (categoryStr == "Arp") return PresetCategory::Arp;
    if (categoryStr == "Pluck") return PresetCategory::Pluck;
    if (categoryStr == "Percussion") return PresetCategory::Percussion;
    if (categoryStr == "SFX") return PresetCategory::SFX;
    if (categoryStr == "Experimental") return PresetCategory::Experimental;
    if (categoryStr == "Template") return PresetCategory::Template;
    if (categoryStr == "Custom") return PresetCategory::Custom;
    return PresetCategory::Custom;
}

std::vector<std::string> getAllCategoryStrings() {
    return {
        "Bass", "Lead", "Pad", "Keys", "Arp", "Pluck",
        "Percussion", "SFX", "Experimental", "Template", "Custom"
    };
}

void PresetFilterCriteria::clear() {
    searchText.clear();
    categories.clear();
    authors.clear();
    tags.clear();
    favoritesOnly = false;
    minRating = 0;
    hasDateRange = false;
    hasAudioFilter = false;
    minBassContent = 0.0f;
    maxBassContent = 1.0f;
    minBrightness = 0.0f;
    maxBrightness = 1.0f;
}

bool PresetFilterCriteria::hasActiveFilters() const {
    return !searchText.empty() ||
           !categories.empty() ||
           !authors.empty() ||
           !tags.empty() ||
           favoritesOnly ||
           minRating > 0 ||
           hasDateRange ||
           hasAudioFilter;
}

PresetInfo::AudioCharacteristics PresetInfo::analyzeAudioCharacteristics(const nlohmann::json& parameters) {
    AudioCharacteristics ac;

    // Analyze filter cutoff for brightness
    if (parameters.contains("filter_cutoff")) {
        float cutoff = parameters["filter_cutoff"];
        ac.brightness = cutoff; // Simple mapping: higher cutoff = brighter
    }

    // Analyze oscillator waveform for warmth
    if (parameters.contains("osc1_waveform")) {
        int waveform = parameters["osc1_waveform"];
        // Saw = 0, Square = 1, Triangle = 2, Sine = 3 (example mapping)
        ac.warmth = (waveform == 2 || waveform == 3) ? 0.8f : 0.4f;
    }

    // Analyze envelope settings for complexity
    float complexity = 0.0f;
    if (parameters.contains("env_attack")) complexity += parameters["env_attack"].get<float>();
    if (parameters.contains("env_decay")) complexity += parameters["env_decay"].get<float>();
    if (parameters.contains("env_sustain")) complexity += parameters["env_sustain"].get<float>();
    if (parameters.contains("env_release")) complexity += parameters["env_release"].get<float>();
    ac.complexity = std::min(1.0f, complexity / 4.0f);

    // Check for arpeggiator
    if (parameters.contains("arp_enabled")) {
        ac.hasArpeggiator = parameters["arp_enabled"].get<bool>();
    }

    // Check for sequencer
    if (parameters.contains("seq_enabled")) {
        ac.hasSequencer = parameters["seq_enabled"].get<bool>();
    }

    // Estimate frequency content based on filter and oscillator settings
    ac.bassContent = 1.0f - ac.brightness; // Inverse relationship for demo
    ac.midContent = (ac.bassContent + ac.brightness) / 2.0f;
    ac.trebleContent = ac.brightness;

    return ac;
}

} // namespace AIMusicHardware