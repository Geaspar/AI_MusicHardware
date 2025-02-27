#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <future>
#include <map>

namespace AIMusicHardware {

struct SynthParameter {
    std::string name;
    float value;
    std::string description;
};

struct MusicSuggestion {
    enum class Type {
        Note,
        Chord,
        Rhythm,
        Pattern,
        Effect,
        Sound,
        Parameter
    };
    
    Type type;
    std::string description;
    std::vector<int> midiNotes;
    std::vector<float> parameters;
    std::map<std::string, float> parameterMap;
    
    MusicSuggestion() : type(Type::Note) {}
};

struct SequencerPattern {
    std::string name;
    std::string description;
    std::vector<std::pair<int, float>> notes; // note number, velocity
    float bpm;
};

struct UserPreference {
    enum class Category {
        Sound,
        Rhythm,
        Harmony,
        Effect,
        Interface,
        Workflow
    };
    
    Category category;
    std::string name;
    std::string value;
    float strength; // 0.0-1.0 indicating how strong this preference is
    
    UserPreference() : category(Category::Sound), strength(0.5f) {}
};

class LLMInterface {
public:
    using ResponseCallback = std::function<void(const std::string& response)>;
    using SuggestionCallback = std::function<void(const std::vector<MusicSuggestion>& suggestions)>;
    using PatternCallback = std::function<void(const std::vector<SequencerPattern>& patterns)>;
    using SynthConfigCallback = std::function<void(const std::map<std::string, float>& parameters)>;
    
    LLMInterface();
    ~LLMInterface();
    
    bool initialize(const std::string& modelPath);
    
    // Text-based interface
    void prompt(const std::string& userPrompt, ResponseCallback callback);
    
    // Music-specific interface
    void suggestNextNotes(const std::vector<int>& currentNotes, SuggestionCallback callback);
    void suggestPatternCompletion(const std::vector<int>& currentPattern, PatternCallback callback);
    void suggestChords(const std::vector<int>& melody, SuggestionCallback callback);
    void suggestEffects(const std::string& description, SuggestionCallback callback);
    
    // Sequencer pattern autocomplete
    void autocompletePattern(const std::vector<int>& patternStart, int measures, PatternCallback callback);
    void generatePatternVariations(const std::vector<int>& basePattern, int numVariations, PatternCallback callback);
    void generateComplementaryPattern(const std::vector<int>& mainPattern, const std::string& relationship, PatternCallback callback);
    
    // Synth parameter configuration from voice/text input
    void configureSynthFromDescription(const std::string& description, SynthConfigCallback callback);
    void suggestSoundPreset(const std::string& description, SynthConfigCallback callback);
    void translateEmotionToSound(const std::string& emotion, SynthConfigCallback callback);
    
    // Higher-level music assistance
    void suggestMusicalIdea(const std::string& context, SuggestionCallback callback);
    void analyzeCurrentComposition(const std::vector<SequencerPattern>& patterns, ResponseCallback callback);
    
    // User preference learning and adaptation
    void recordUserFeedback(const MusicSuggestion& suggestion, bool accepted, float rating = 0.0f);
    void recordParameterAdjustment(const std::string& paramName, float oldValue, float newValue);
    void recordPatternEdits(const SequencerPattern& originalPattern, const SequencerPattern& editedPattern);
    void explicitlySetPreference(const std::string& category, const std::string& name, const std::string& value);
    
    // Get learned preferences
    std::vector<UserPreference> getUserPreferences() const;
    
    // Apply learned preferences to suggestions
    void adjustSuggestionsToPreferences(std::vector<MusicSuggestion>& suggestions) const;
    void adjustPatternToPreferences(SequencerPattern& pattern) const;
    
    // Save/load user profile
    bool saveUserProfile(const std::string& filename) const;
    bool loadUserProfile(const std::string& filename);
    
    // Voice input processing
    void processVoiceInput(const std::string& transcribedText, ResponseCallback callback);
    
    // Set system prompt/context
    void setSystemPrompt(const std::string& systemPrompt);
    void setMusicalContext(const std::string& key, const std::string& scale, int bpm);
    
    // Asynchronous versions
    std::future<std::string> promptAsync(const std::string& userPrompt);
    std::future<std::vector<MusicSuggestion>> suggestNextAsync(const std::string& context);
    std::future<std::map<std::string, float>> configureSynthFromDescriptionAsync(const std::string& description);
    
    // Cancel any ongoing operations
    void cancel();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    std::string systemPrompt_;
    std::string musicalKey_;
    std::string musicalScale_;
    int bpm_;
    
    std::vector<UserPreference> userPreferences_;
    
    // Internal preference learning methods
    void updatePreferenceModel();
    void analyzeUserBehaviorPatterns();
    UserPreference::Category categoryFromString(const std::string& category) const;
};

} // namespace AIMusicHardware