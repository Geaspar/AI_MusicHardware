#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace AIMusicHardware {

// Forward declarations
class Synthesizer;

/**
 * @class PresetManager
 * @brief Manages preset saving, loading, and browsing for the synthesizer
 * 
 * The PresetManager handles all functionality related to presets, including
 * serialization, storage, categorization, and browsing/search capabilities.
 */
class PresetManager {
public:
    /**
     * @brief Construct a new Preset Manager
     * @param synth Pointer to the synthesizer to manage presets for
     */
    PresetManager(Synthesizer* synth);
    
    /**
     * @brief Destructor
     */
    ~PresetManager();
    
    /**
     * @brief Load a preset from a file
     * @param filePath Path to the preset file
     * @return true if successfully loaded
     */
    bool loadPreset(const std::string& filePath);
    
    /**
     * @brief Save current synthesizer state as a preset
     * @param filePath Path to save the preset file
     * @param name Name of the preset
     * @param author Author of the preset (optional)
     * @param category Category of the preset (optional)
     * @param description Description of the preset (optional)
     * @return true if successfully saved
     */
    bool savePreset(const std::string& filePath, const std::string& name, 
                   const std::string& author = "", const std::string& category = "",
                   const std::string& description = "");
    
    /**
     * @brief Get a list of all available presets
     * @return Vector of preset file paths
     */
    std::vector<std::string> getAllPresets() const;
    
    /**
     * @brief Get presets filtered by category
     * @param category Category to filter by
     * @return Vector of matching preset file paths
     */
    std::vector<std::string> getPresetsByCategory(const std::string& category) const;
    
    /**
     * @brief Search presets by name, author, or description
     * @param query Search query string
     * @return Vector of matching preset file paths
     */
    std::vector<std::string> searchPresets(const std::string& query) const;
    
    /**
     * @brief Load the next preset in the list
     * @return true if a preset was loaded
     */
    bool loadNextPreset();
    
    /**
     * @brief Load the previous preset in the list
     * @return true if a preset was loaded
     */
    bool loadPreviousPreset();
    
    /**
     * @brief Get the name of the currently loaded preset
     * @return Current preset name
     */
    std::string getCurrentPresetName() const;
    
    /**
     * @brief Get the file path of the currently loaded preset
     * @return Current preset path
     */
    std::string getCurrentPresetPath() const;
    
    /**
     * @brief Get the author of the currently loaded preset
     * @return Current preset author
     */
    std::string getCurrentPresetAuthor() const;
    
    /**
     * @brief Get the category of the currently loaded preset
     * @return Current preset category
     */
    std::string getCurrentPresetCategory() const;
    
    /**
     * @brief Get the description of the currently loaded preset
     * @return Current preset description
     */
    std::string getCurrentPresetDescription() const;
    
    /**
     * @brief Get the default factory presets directory
     * @return Path to factory presets
     */
    static std::string getFactoryPresetsDirectory();
    
    /**
     * @brief Get the user presets directory
     * @return Path to user presets
     */
    static std::string getUserPresetsDirectory();
    
    /**
     * @brief Get all preset directories (factory and user)
     * @return Vector of preset directory paths
     */
    static std::vector<std::string> getAllPresetDirectories();
    
    /**
     * @brief Get all available preset categories
     * @return Vector of category names
     */
    static std::vector<std::string> getAvailableCategories();
    
    /**
     * @brief Add a callback to be notified when a preset is loaded
     * @param callback Function to call when a preset is loaded
     */
    void addPresetLoadedCallback(std::function<void(const std::string&)> callback);
    
private:
    // The synthesizer to control
    Synthesizer* synth_;
    
    // Current preset information
    std::string currentPresetPath_;
    std::string currentPresetName_;
    std::string currentPresetAuthor_;
    std::string currentPresetCategory_;
    std::string currentPresetDescription_;
    
    // Callbacks for preset loaded events
    std::vector<std::function<void(const std::string&)>> presetLoadedCallbacks_;
    
    // Serialization methods
    nlohmann::json serializeParameters() const;
    bool deserializeParameters(const nlohmann::json& json);
    
    // Filesystem helpers
    bool ensureDirectoryExists(const std::string& path) const;
    std::vector<std::string> getPresetsInDirectory(const std::string& directory) const;
    
    // Notification helper
    void notifyPresetLoaded(const std::string& presetPath);
};

} // namespace AIMusicHardware