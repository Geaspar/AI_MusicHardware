#pragma once

#include "UIComponents.h"
#include <functional>
#include <map>
#include <set>

namespace AIMusicHardware {

/**
 * @brief Professional MIDI keyboard UI component
 * 
 * Provides a visual piano keyboard interface with:
 * - Standard piano layout (white and black keys)
 * - Multiple octaves with configurable range
 * - Velocity-sensitive input based on click position
 * - Visual feedback for pressed keys
 * - MIDI note callbacks for audio engine integration
 */
class MidiKeyboard : public UIComponent {
public:
    // MIDI note callback: (note, velocity, isNoteOn)
    using NoteCallback = std::function<void(int note, int velocity, bool isNoteOn)>;
    
    /**
     * @brief Key information structure
     */
    struct KeyInfo {
        int note;           // MIDI note number (0-127)
        bool isBlackKey;    // True for black keys (sharps/flats)
        int x, y;           // Position in keyboard
        int width, height;  // Key dimensions
        bool isPressed;     // Current press state
        Color color;        // Current key color
    };
    
    /**
     * @brief Keyboard layout configuration
     */
    struct KeyboardConfig {
        int startOctave = 2;        // Starting octave (C2 = MIDI note 36)
        int numOctaves = 3;         // Number of octaves to display
        int whiteKeyWidth = 24;     // Width of white keys
        int whiteKeyHeight = 120;   // Height of white keys
        int blackKeyWidth = 16;     // Width of black keys
        int blackKeyHeight = 80;    // Height of black keys
        Color whiteKeyColor = Color(240, 240, 240);
        Color blackKeyColor = Color(40, 40, 40);
        Color pressedWhiteColor = Color(100, 150, 255);
        Color pressedBlackColor = Color(80, 120, 200);
        Color keyBorderColor = Color(128, 128, 128);
    };

public:
    MidiKeyboard(const std::string& id, int x = 0, int y = 0);
    virtual ~MidiKeyboard() = default;
    
    // Configuration
    void setConfig(const KeyboardConfig& config);
    const KeyboardConfig& getConfig() const { return config_; }
    
    // Callbacks
    void setNoteCallback(NoteCallback callback) { noteCallback_ = callback; }
    
    // Manual note control (for external MIDI input display)
    void setNotePressed(int note, bool pressed, int velocity = 64);
    void releaseAllNotes();
    
    // Octave control
    void setOctaveRange(int startOctave, int numOctaves);
    void transposeOctave(int deltaOctaves);
    
    // Velocity settings
    void setVelocityRange(int minVelocity, int maxVelocity);
    void setFixedVelocity(int velocity); // 0 = variable velocity based on click position
    
    // Visual state
    bool isNotePressed(int note) const;
    int getLowestNote() const;
    int getHighestNote() const;
    
    // UIComponent interface
    void render(DisplayManager* display) override;
    bool handleInput(const InputEvent& event) override;
    void update(float deltaTime) override;
    
    // Utility
    static bool isBlackKey(int note);
    static std::string getNoteName(int note);
    static int noteToOctave(int note);
    
protected:
    void generateKeyLayout();
    KeyInfo* findKeyAtPosition(int x, int y);
    int calculateVelocity(const KeyInfo& key, int clickY);
    void updateKeyVisuals();
    void notifyNoteEvent(int note, int velocity, bool isNoteOn);
    
private:
    KeyboardConfig config_;
    std::vector<KeyInfo> keys_;
    std::map<int, KeyInfo*> noteToKey_;  // Fast note lookup
    std::set<int> pressedNotes_;         // Currently pressed notes
    
    NoteCallback noteCallback_;
    
    // Velocity settings
    int minVelocity_ = 20;
    int maxVelocity_ = 127;
    int fixedVelocity_ = 0;  // 0 = variable
    
    // Mouse interaction state
    bool isDragging_ = false;
    int dragStartX_ = 0;
    int dragStartY_ = 0;
    KeyInfo* lastHoveredKey_ = nullptr;
    
    // Animation state
    float keyPressAnimation_ = 0.0f;
    std::map<int, float> keyAnimations_;  // Per-key animation states
    
    // Key layout constants
    static const int NOTES_PER_OCTAVE = 12;
    static const bool BLACK_KEY_PATTERN[12]; // Pattern for each note in octave
    static const int WHITE_KEY_POSITIONS[7]; // Positions of white keys (C,D,E,F,G,A,B)
    static const int BLACK_KEY_POSITIONS[5]; // Positions of black keys (C#,D#,F#,G#,A#)
};

// Inline implementations for performance
inline bool MidiKeyboard::isBlackKey(int note) {
    return BLACK_KEY_PATTERN[note % 12];
}

inline int MidiKeyboard::noteToOctave(int note) {
    return (note / 12) - 1;  // MIDI note 60 = C4 (4th octave)
}

inline bool MidiKeyboard::isNotePressed(int note) const {
    return pressedNotes_.find(note) != pressedNotes_.end();
}

inline int MidiKeyboard::getLowestNote() const {
    return config_.startOctave * 12 + 12;  // C of starting octave
}

inline int MidiKeyboard::getHighestNote() const {
    return getLowestNote() + (config_.numOctaves * 12) - 1;
}

} // namespace AIMusicHardware