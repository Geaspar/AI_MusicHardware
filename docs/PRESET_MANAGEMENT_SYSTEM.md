# Preset Management System Design

*May 9, 2025*

This document outlines the design and implementation plan for the preset management system in the AIMusicHardware project, inspired by professional synthesizers like Vital.

## 1. System Overview

The preset system allows users to save, load, and manage sound presets. It provides organization features, metadata support, and a visual browser interface.

### 1.1 Core Features

- **Preset Storage**: Save and load synth parameters as JSON files
- **Browser Interface**: Visual interface for browsing and selecting presets 
- **Categorization**: Organization by categories (bass, lead, pad, etc.)
- **Favorites**: System for marking and quickly accessing favorite presets
- **Metadata**: Support for author, creation date, and comments

### 1.2 Core Components

1. **PresetManager**: Backend for file operations and preset data management
2. **PresetBrowser**: UI component for browsing and selecting presets
3. **PresetSaveDialog**: UI for saving presets with metadata
4. **PresetListView**: Component showing a scrollable, filterable list of presets
5. **CategorySelector**: UI component for filtering presets by category
6. **ParameterManager**: Interface connecting preset data to synth parameters

## 2. Data Structure

### 2.1 Preset File Format (JSON)

```json
{
  "preset_name": "Example Preset",
  "metadata": {
    "author": "AI Music Hardware",
    "category": "Bass",
    "comments": "Deep bass with modulation",
    "created": "2025-05-09T12:00:00Z",
    "modified": "2025-05-09T12:00:00Z",
    "tags": ["bass", "analog", "deep"]
  },
  "parameters": {
    "osc1_waveform": 0,
    "osc1_level": 0.8,
    "filter_cutoff": 2000.0,
    "filter_resonance": 0.4,
    "env_attack": 0.01,
    "env_decay": 0.3,
    "env_sustain": 0.5,
    "env_release": 0.5
    // ...more parameters
  },
  "modulation": [
    {
      "source": "lfo1",
      "destination": "filter_cutoff",
      "amount": 0.3
    }
    // ...more modulation routings
  ]
}
```

### 2.2 Directory Structure

```
/presets
  /factory
    /bass
    /lead
    /pad
    /keys
    /fx
    /other
  /user
    /bass
    /lead
    /pad
    /keys
    /fx
    /other
  /favorites.json    // Stores references to favorite presets
  /settings.json     // Stores preset browser settings
```

## 3. Components Implementation

### 3.1 PresetManager Class

```cpp
class PresetManager {
public:
    // Initialization
    PresetManager();
    bool initialize(const std::string& presetsDirectory);
    
    // Preset operations
    bool loadPreset(const std::string& path);
    bool savePreset(const std::string& path, const std::string& name, 
                  const std::map<std::string, std::string>& metadata);
    bool deletePreset(const std::string& path);
    
    // Preset list operations
    std::vector<PresetInfo> getPresetList(const std::string& category = "");
    std::vector<std::string> getCategories();
    
    // Favorites management
    void addToFavorites(const std::string& path);
    void removeFromFavorites(const std::string& path);
    std::vector<PresetInfo> getFavorites();
    
    // Current preset operations
    PresetInfo getCurrentPreset() const;
    bool isCurrentPresetModified() const;
    void markCurrentPresetAsModified();
    void clearModifiedFlag();
    
private:
    std::string factoryPresetsPath_;
    std::string userPresetsPath_;
    std::vector<PresetInfo> cachedPresets_;
    std::set<std::string> favorites_;
    PresetInfo currentPreset_;
    bool currentPresetModified_;
    
    // Helpers
    void scanPresets();
    void loadFavorites();
    void saveFavorites();
    json serializeCurrentState();
    void deserializeToState(const json& data);
};
```

### 3.2 PresetBrowser UI Component

```cpp
class PresetBrowser : public UIComponent {
public:
    PresetBrowser(const std::string& id, PresetManager* presetManager);
    virtual ~PresetBrowser();
    
    // UI component interactions
    void setVisible(bool visible) override;
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Browser-specific functions
    void refreshPresetList();
    void setCategory(const std::string& category);
    void setSearchText(const std::string& text);
    void setSortMode(PresetSortMode mode);
    
    // Callbacks
    using PresetSelectedCallback = std::function<void(const PresetInfo&)>;
    void setPresetSelectedCallback(PresetSelectedCallback callback);
    
private:
    PresetManager* presetManager_;
    PresetListView* listView_;
    CategorySelector* categorySelector_;
    SearchBox* searchBox_;
    SortSelector* sortSelector_;
    PresetInfoPanel* infoPanel_;
    
    std::string currentCategory_;
    std::string searchText_;
    PresetSortMode sortMode_;
    std::vector<PresetInfo> filteredPresets_;
    
    PresetSelectedCallback presetSelectedCallback_;
    
    // Helper methods
    void filterPresets();
    void handlePresetSelection(const PresetInfo& preset);
    void createComponents();
};
```

### 3.3 PresetListView Component

```cpp
class PresetListView : public UIComponent {
public:
    PresetListView(const std::string& id);
    virtual ~PresetListView();
    
    // UI component overrides
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // List operations
    void setPresets(const std::vector<PresetInfo>& presets);
    void setSelectedIndex(int index);
    int getSelectedIndex() const;
    PresetInfo getSelectedPreset() const;
    
    // Callbacks
    using ItemSelectedCallback = std::function<void(const PresetInfo&)>;
    void setItemSelectedCallback(ItemSelectedCallback callback);
    
private:
    std::vector<PresetInfo> presets_;
    int selectedIndex_;
    int scrollOffset_;
    int visibleItems_;
    ItemSelectedCallback itemSelectedCallback_;
    
    // Helper methods
    void renderPresetItem(DisplayManager* display, const PresetInfo& preset, 
                         int index, bool selected);
    void ensureSelectedVisible();
};
```

### 3.4 PresetSaveDialog Component

```cpp
class PresetSaveDialog : public UIComponent {
public:
    PresetSaveDialog(const std::string& id, PresetManager* presetManager);
    virtual ~PresetSaveDialog();
    
    // UI component overrides
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Dialog operations
    void show(const std::string& initialName = "", 
             const std::string& initialCategory = "");
    void hide();
    
    // Callbacks
    using SaveCompletedCallback = std::function<void(bool success)>;
    void setSaveCompletedCallback(SaveCompletedCallback callback);
    
private:
    PresetManager* presetManager_;
    TextInput* nameInput_;
    CategorySelector* categorySelector_;
    TextInput* authorInput_;
    TextArea* commentsInput_;
    Button* saveButton_;
    Button* cancelButton_;
    
    std::string presetName_;
    std::string category_;
    std::string author_;
    std::string comments_;
    
    SaveCompletedCallback saveCompletedCallback_;
    
    // Helper methods
    void initializeComponents();
    void handleSave();
    void handleCancel();
    bool validateInputs();
};
```

## 4. Integration with Synthesizer

### 4.1 ParameterManager Interface

```cpp
class ParameterManager {
public:
    // Get/set parameter values
    float getParameterValue(const std::string& parameterId) const;
    void setParameterValue(const std::string& parameterId, float value);
    
    // Get all parameters
    std::map<std::string, float> getAllParameters() const;
    void setAllParameters(const std::map<std::string, float>& parameters);
    
    // Modulation routing
    void addModulation(const std::string& source, const std::string& destination, float amount);
    void removeModulation(const std::string& source, const std::string& destination);
    std::vector<ModulationRouting> getAllModulations() const;
    void setAllModulations(const std::vector<ModulationRouting>& modulations);
    
    // Parameter change notification
    using ParameterChangedCallback = std::function<void(const std::string&)>;
    void setParameterChangedCallback(ParameterChangedCallback callback);
    
private:
    std::map<std::string, float> parameters_;
    std::vector<ModulationRouting> modulations_;
    ParameterChangedCallback parameterChangedCallback_;
};
```

### 4.2 Integration with UI

```cpp
// Main UI integration
class PresetSelector : public UIComponent {
public:
    PresetSelector(const std::string& id, PresetManager* presetManager);
    
    // UI component overrides
    void update(float deltaTime) override;
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    
    // Preset operations
    void showBrowser();
    void hideBrowser();
    void saveCurrent();
    void nextPreset();
    void previousPreset();
    
private:
    PresetManager* presetManager_;
    Button* currentPresetButton_;
    Button* browsePrevButton_;
    Button* browseNextButton_;
    Button* saveButton_;
    
    PresetBrowser* browser_;
    PresetSaveDialog* saveDialog_;
    
    // Helpers
    void updateCurrentPresetDisplay();
    void handlePresetSelected(const PresetInfo& preset);
    void handleSaveCompleted(bool success);
};
```

## 5. Implementation Plan

### 5.1 Phase 1: Core Backend (Week 1)

- Implement PresetManager class
- Define JSON structure for presets
- Create directory structure
- Implement file loading/saving
- Build parameter serialization/deserialization

### 5.2 Phase 2: Basic UI Components (Week 2)

- Create PresetListView component
- Implement CategorySelector
- Build simple PresetBrowser
- Create PresetSaveDialog
- Implement basic navigation

### 5.3 Phase 3: Integration (Week 3)

- Connect PresetManager to Synthesizer
- Implement ParameterManager interface
- Add PresetSelector to main UI
- Connect UI components to backend
- Add keyboard shortcuts

### 5.4 Phase 4: Advanced Features (Week 4)

- Implement favorites system
- Add search functionality
- Create sorting options
- Add preview capability
- Implement preset info panel
- Add import/export functionality

### 5.5 Phase 5: Testing and Refinement (Week 5)

- Test with large preset libraries
- Optimize performance
- Add visual improvements
- Fix UI navigation issues
- Add factory presets

## 6. User Interface Design

### 6.1 Preset Browser Layout

```
+-----------------------------------------------+
| Category Tabs                                 |
+-------+---------------------------------------+
|       |                                       |
|       |                                       |
| List  |           Preset Info                 |
| View  |             Panel                     |
|       |                                       |
|       |                                       |
+-------+---------------------------------------+
| Search:  [________________] [Sort ▾] [Favs ♥] |
+-----------------------------------------------+
| [Cancel]                           [Load]     |
+-----------------------------------------------+
```

### 6.2 Save Dialog Layout

```
+-----------------------------------------------+
|             Save Preset                       |
+-----------------------------------------------+
| Name:     [___________________________]       |
|                                               |
| Category: [_________▾]                        |
|                                               |
| Author:   [___________________________]       |
|                                               |
| Comments:                                     |
| [_______________________________________]     |
| [_______________________________________]     |
|                                               |
+-----------------------------------------------+
| [Cancel]                           [Save]     |
+-----------------------------------------------+
```

### 6.3 Preset Selector (Main UI Header)

```
+-----------------------------------------------+
| [◀] [Awesome Bass] [▶] [💾] [Browse...]      |
+-----------------------------------------------+
```

## 7. Future Enhancements

- Cloud sync for presets
- Online preset sharing platform
- AI-assisted preset generation
- Preset morphing between different presets
- A/B comparison of presets
- Automatic categorization using ML
- Version control for presets
- Collaborative preset editing

## 8. Resources

- JSON library: nlohmann/json for C++
- File system operations: std::filesystem (C++17)
- UI rendering: Our existing DisplayManager and UIComponent system
- Reference implementation: Vital's preset browser