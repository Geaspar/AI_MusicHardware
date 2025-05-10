#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>

namespace AIMusicHardware {

// Forward declarations
class Synthesizer;
class ParameterManager;

/**
 * @brief Stores information about a preset
 */
struct PresetInfo {
    std::string name;           // Preset name
    std::string path;           // Full path to preset file
    std::string category;       // Category (bass, lead, etc)
    std::string author;         // Author name
    std::string comments;       // Preset description/comments
    std::string created;        // Creation timestamp
    std::string modified;       // Last modified timestamp
    std::vector<std::string> tags; // Tags for additional categorization
    bool isFactory;             // True if this is a factory preset
    bool isFavorite;            // True if user marked as favorite
    
    PresetInfo() : isFactory(false), isFavorite(false) {}
};

/**
 * @brief Preset sort modes
 */
enum class PresetSortMode {
    ByName,
    ByDate,
    ByAuthor,
    ByCategory
};

/**
 * @brief Manages preset loading, saving, and browsing
 * 
 * This class handles all operations related to presets, including:
 * - Loading and saving presets
 * - Browsing preset directories
 * - Managing favorites
 * - Tracking the current preset state
 */
class PresetManager {
public:
    /**
     * @brief Constructor
     */
    PresetManager();
    
    /**
     * @brief Destructor
     */
    ~PresetManager();
    
    /**
     * @brief Initialize the preset manager
     * 
     * @param presetsDirectory Base directory for presets
     * @return true if initialization was successful
     */
    bool initialize(const std::string& presetsDirectory);
    
    /**
     * @brief Connect to the synthesizer for parameter access
     * 
     * @param synth Pointer to the synthesizer
     */
    void connectSynthesizer(Synthesizer* synth);
    
    /**
     * @brief Connect to parameter manager for parameter access
     * 
     * @param paramManager Pointer to parameter manager
     */
    void connectParameterManager(ParameterManager* paramManager);
    
    /**
     * @brief Load a preset from a file
     * 
     * @param path Path to preset file
     * @return true if the preset was loaded successfully
     */
    bool loadPreset(const std::string& path);
    
    /**
     * @brief Save current settings as a preset
     * 
     * @param path Path to save to (directory + filename)
     * @param name Preset name
     * @param metadata Map of metadata values (category, author, etc)
     * @return true if the preset was saved successfully
     */
    bool savePreset(const std::string& path, const std::string& name, 
                  const std::map<std::string, std::string>& metadata);
    
    /**
     * @brief Delete a preset file
     * 
     * @param path Path to preset file
     * @return true if the preset was deleted successfully
     */
    bool deletePreset(const std::string& path);
    
    /**
     * @brief Get a list of available presets
     * 
     * @param category Optional category filter
     * @return std::vector<PresetInfo> List of presets
     */
    std::vector<PresetInfo> getPresetList(const std::string& category = "");
    
    /**
     * @brief Get list of available preset categories
     * 
     * @return std::vector<std::string> List of category names
     */
    std::vector<std::string> getCategories();
    
    /**
     * @brief Add a preset to favorites
     * 
     * @param path Path to preset file
     */
    void addToFavorites(const std::string& path);
    
    /**
     * @brief Remove a preset from favorites
     * 
     * @param path Path to preset file
     */
    void removeFromFavorites(const std::string& path);
    
    /**
     * @brief Get list of favorite presets
     * 
     * @return std::vector<PresetInfo> List of favorite presets
     */
    std::vector<PresetInfo> getFavorites();
    
    /**
     * @brief Get the current preset
     * 
     * @return const PresetInfo& Current preset info
     */
    const PresetInfo& getCurrentPreset() const;
    
    /**
     * @brief Check if current preset has been modified
     * 
     * @return true if the current preset has been modified
     */
    bool isCurrentPresetModified() const;
    
    /**
     * @brief Mark the current preset as modified
     */
    void markCurrentPresetAsModified();
    
    /**
     * @brief Clear the modified flag
     */
    void clearModifiedFlag();
    
    /**
     * @brief Set callback for preset change notification
     * 
     * @param callback Function to call when preset changes
     */
    using PresetChangedCallback = std::function<void(const PresetInfo&)>;
    void setPresetChangedCallback(PresetChangedCallback callback);
    
    /**
     * @brief Create default directories if they don't exist
     */
    void createDefaultDirectories();
    
    /**
     * @brief Load the next preset in the current category
     * 
     * @return true if successful
     */
    bool loadNextPreset();
    
    /**
     * @brief Load the previous preset in the current category
     * 
     * @return true if successful
     */
    bool loadPreviousPreset();

private:
    std::string presetsDirectory_;
    std::string factoryPresetsPath_;
    std::string userPresetsPath_;
    std::vector<PresetInfo> cachedPresets_;
    std::set<std::string> favorites_;
    PresetInfo currentPreset_;
    bool currentPresetModified_;
    PresetChangedCallback presetChangedCallback_;
    
    // Connected components
    Synthesizer* synth_;
    ParameterManager* paramManager_;
    
    // Helpers
    void scanPresets();
    void loadFavorites();
    void saveFavorites();
    bool deserializePreset(const std::string& path, PresetInfo& outInfo);
    bool applyPresetToSynth(const PresetInfo& preset);
    std::string serializeCurrentState();
    bool isValidPresetFile(const std::string& path);
};

} // namespace AIMusicHardware