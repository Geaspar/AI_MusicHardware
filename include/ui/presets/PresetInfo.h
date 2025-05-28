#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <nlohmann/json.hpp>

namespace AIMusicHardware {

/**
 * @brief Comprehensive preset information structure
 * 
 * Based on analysis of Vital synth and industry best practices,
 * this structure contains all metadata needed for professional
 * preset management including performance optimization hints.
 */
struct PresetInfo {
    // Core identification
    std::string name;
    std::string filePath;
    std::string category;  // Bass, Lead, Pad, Keys, FX, etc.
    
    // Authorship and attribution
    std::string author;
    std::string license;
    std::string description;
    std::vector<std::string> tags;
    
    // Temporal information
    std::chrono::time_point<std::chrono::system_clock> created;
    std::chrono::time_point<std::chrono::system_clock> modified;
    std::size_t fileSize;
    
    // User preferences
    bool isFavorite = false;
    int userRating = 0;  // 0-5 stars
    int playCount = 0;
    std::chrono::time_point<std::chrono::system_clock> lastAccessed;
    
    // Content analysis (for smart features)
    struct AudioCharacteristics {
        float bassContent = 0.0f;      // 0-1, amount of low frequency content
        float midContent = 0.0f;       // 0-1, amount of mid frequency content
        float trebleContent = 0.0f;    // 0-1, amount of high frequency content
        float brightness = 0.0f;       // 0-1, overall spectral brightness
        float warmth = 0.0f;          // 0-1, perceived warmth
        float complexity = 0.0f;       // 0-1, parameter complexity measure
        bool hasArpeggiator = false;
        bool hasSequencer = false;
        int modulationCount = 0;       // Number of active modulations
    } audioCharacteristics;

    // Raw parameter data for ML analysis
    nlohmann::json parameterData;
    
    // Performance hints (for UI optimization)
    bool isMetadataCached = false;
    bool needsParameterAnalysis = true;
    
    // Serialization support
    nlohmann::json toJson() const;
    static PresetInfo fromJson(const nlohmann::json& json);
    static PresetInfo fromFile(const std::string& filePath);

    // Audio analysis helper
    static void analyzeAudioCharacteristics(PresetInfo& info, const nlohmann::json& parameters);
    static AudioCharacteristics analyzeAudioCharacteristics(const nlohmann::json& parameters);
    
    // Comparison operators for sorting
    bool operator<(const PresetInfo& other) const {
        return name < other.name;
    }
    
    bool operator==(const PresetInfo& other) const {
        return filePath == other.filePath;
    }
};

/**
 * @brief Preset category definitions following industry standards
 */
enum class PresetCategory {
    Bass,
    Lead,
    Pad,
    Keys,
    Arp,
    Pluck,
    Percussion,
    SFX,
    Experimental,
    Template,
    Custom
};

/**
 * @brief Convert category enum to string
 */
std::string categoryToString(PresetCategory category);

/**
 * @brief Convert string to category enum
 */
PresetCategory stringToCategory(const std::string& categoryStr);

/**
 * @brief Get all available category strings
 */
std::vector<std::string> getAllCategoryStrings();

/**
 * @brief Sorting criteria for preset lists
 */
enum class PresetSortCriteria {
    Name,
    Author,
    Category,
    DateCreated,
    DateModified,
    Favorites,
    Rating,
    PlayCount,
    FileSize
};

/**
 * @brief Sort direction
 */
enum class SortDirection {
    Ascending,
    Descending
};

/**
 * @brief Filter criteria for preset searches
 */
struct PresetFilterCriteria {
    std::string searchText;
    std::vector<std::string> categories;
    std::vector<std::string> authors;
    std::vector<std::string> tags;
    bool favoritesOnly = false;
    int minRating = 0;
    
    // Date range filtering
    bool hasDateRange = false;
    std::chrono::time_point<std::chrono::system_clock> dateFrom;
    std::chrono::time_point<std::chrono::system_clock> dateTo;
    
    // Audio characteristics filtering
    bool hasAudioFilter = false;
    float minBassContent = 0.0f;
    float maxBassContent = 1.0f;
    float minBrightness = 0.0f;
    float maxBrightness = 1.0f;
    
    // Clear all filters
    void clear();
    
    // Check if any filters are active
    bool hasActiveFilters() const;
};

} // namespace AIMusicHardware