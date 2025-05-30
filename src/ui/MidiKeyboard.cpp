#include "../../include/ui/MidiKeyboard.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace AIMusicHardware {

// Black key pattern for each note in an octave (C, C#, D, D#, E, F, F#, G, G#, A, A#, B)
const bool MidiKeyboard::BLACK_KEY_PATTERN[12] = {
    false, true, false, true, false,    // C, C#, D, D#, E
    false, true, false, true, false, true, false  // F, F#, G, G#, A, A#, B
};

// Positions of white keys within an octave (0-6 for C,D,E,F,G,A,B)
const int MidiKeyboard::WHITE_KEY_POSITIONS[7] = { 0, 2, 4, 5, 7, 9, 11 };

// Positions of black keys within an octave (1,3,6,8,10 for C#,D#,F#,G#,A#)
const int MidiKeyboard::BLACK_KEY_POSITIONS[5] = { 1, 3, 6, 8, 10 };

MidiKeyboard::MidiKeyboard(const std::string& id, int x, int y) 
    : UIComponent(id) {
    x_ = x;
    y_ = y;
    generateKeyLayout();
}

void MidiKeyboard::setConfig(const KeyboardConfig& config) {
    config_ = config;
    generateKeyLayout();
}

void MidiKeyboard::setOctaveRange(int startOctave, int numOctaves) {
    config_.startOctave = std::max(0, std::min(9, startOctave));
    config_.numOctaves = std::max(1, std::min(6, numOctaves));
    generateKeyLayout();
}

void MidiKeyboard::transposeOctave(int deltaOctaves) {
    int newStart = config_.startOctave + deltaOctaves;
    setOctaveRange(newStart, config_.numOctaves);
}

void MidiKeyboard::setVelocityRange(int minVelocity, int maxVelocity) {
    minVelocity_ = std::max(1, std::min(127, minVelocity));
    maxVelocity_ = std::max(minVelocity_, std::min(127, maxVelocity));
}

void MidiKeyboard::setFixedVelocity(int velocity) {
    fixedVelocity_ = std::max(0, std::min(127, velocity));
}

void MidiKeyboard::generateKeyLayout() {
    keys_.clear();
    noteToKey_.clear();
    
    int currentX = x_;
    int keyboardY = y_;
    
    // Calculate total keyboard width
    int totalWhiteKeys = config_.numOctaves * 7;
    int totalWidth = totalWhiteKeys * config_.whiteKeyWidth;
    width_ = totalWidth;
    height_ = config_.whiteKeyHeight;
    
    // Generate white keys first
    int whiteKeyIndex = 0;
    
    for (int octave = 0; octave < config_.numOctaves; octave++) {
        for (int i = 0; i < 7; i++) {
            int noteInOctave = WHITE_KEY_POSITIONS[i];
            int midiNote = (config_.startOctave + octave) * 12 + noteInOctave + 12; // +12 for MIDI offset
            
            KeyInfo key;
            key.note = midiNote;
            key.isBlackKey = false;
            key.x = currentX + whiteKeyIndex * config_.whiteKeyWidth;
            key.y = keyboardY;
            key.width = config_.whiteKeyWidth;
            key.height = config_.whiteKeyHeight;
            key.isPressed = false;
            key.color = config_.whiteKeyColor;
            
            keys_.push_back(key);
            noteToKey_[midiNote] = &keys_.back();
            whiteKeyIndex++;
        }
    }
    
    // Generate black keys (render on top of white keys)
    for (int octave = 0; octave < config_.numOctaves; octave++) {
        for (int i = 0; i < 5; i++) {
            int noteInOctave = BLACK_KEY_POSITIONS[i];
            int midiNote = (config_.startOctave + octave) * 12 + noteInOctave + 12;
            
            // Calculate black key position (between white keys)
            int whiteKeysBefore = 0;
            for (int w = 0; w < 7; w++) {
                if (WHITE_KEY_POSITIONS[w] < noteInOctave) {
                    whiteKeysBefore++;
                }
            }
            
            int blackKeyX = currentX + (octave * 7 + whiteKeysBefore) * config_.whiteKeyWidth - config_.blackKeyWidth / 2;
            
            KeyInfo key;
            key.note = midiNote;
            key.isBlackKey = true;
            key.x = blackKeyX;
            key.y = keyboardY;
            key.width = config_.blackKeyWidth;
            key.height = config_.blackKeyHeight;
            key.isPressed = false;
            key.color = config_.blackKeyColor;
            
            keys_.push_back(key);
            noteToKey_[midiNote] = &keys_.back();
        }
    }
    
    // Sort keys so black keys are rendered last (on top)
    std::sort(keys_.begin(), keys_.end(), [](const KeyInfo& a, const KeyInfo& b) {
        if (a.isBlackKey != b.isBlackKey) {
            return !a.isBlackKey; // White keys first
        }
        return a.note < b.note;
    });
}

void MidiKeyboard::setNotePressed(int note, bool pressed, int velocity) {
    auto it = noteToKey_.find(note);
    if (it != noteToKey_.end()) {
        KeyInfo* key = it->second;
        key->isPressed = pressed;
        
        if (pressed) {
            pressedNotes_.insert(note);
            key->color = key->isBlackKey ? config_.pressedBlackColor : config_.pressedWhiteColor;
            keyAnimations_[note] = 1.0f;
        } else {
            pressedNotes_.erase(note);
            key->color = key->isBlackKey ? config_.blackKeyColor : config_.whiteKeyColor;
            keyAnimations_[note] = 0.0f;
        }
        
        // Notify callback
        if (noteCallback_) {
            noteCallback_(note, velocity, pressed);
        }
    }
}

void MidiKeyboard::releaseAllNotes() {
    for (int note : pressedNotes_) {
        setNotePressed(note, false, 0);
    }
    pressedNotes_.clear();
}

MidiKeyboard::KeyInfo* MidiKeyboard::findKeyAtPosition(int x, int y) {
    // Check black keys first (they're on top)
    for (auto& key : keys_) {
        if (key.isBlackKey) {
            if (x >= key.x && x < key.x + key.width &&
                y >= key.y && y < key.y + key.height) {
                return &key;
            }
        }
    }
    
    // Then check white keys
    for (auto& key : keys_) {
        if (!key.isBlackKey) {
            if (x >= key.x && x < key.x + key.width &&
                y >= key.y && y < key.y + key.height) {
                return &key;
            }
        }
    }
    
    return nullptr;
}

int MidiKeyboard::calculateVelocity(const KeyInfo& key, int clickY) {
    if (fixedVelocity_ > 0) {
        return fixedVelocity_;
    }
    
    // Calculate velocity based on click position (bottom = higher velocity)
    float relativeY = static_cast<float>(clickY - key.y) / key.height;
    relativeY = std::max(0.0f, std::min(1.0f, relativeY));
    
    // Invert so bottom of key = high velocity
    float velocityNorm = 1.0f - relativeY;
    
    // Apply curve to make it feel more natural
    velocityNorm = std::pow(velocityNorm, 0.7f);
    
    int velocity = static_cast<int>(minVelocity_ + velocityNorm * (maxVelocity_ - minVelocity_));
    return std::max(minVelocity_, std::min(maxVelocity_, velocity));
}

void MidiKeyboard::notifyNoteEvent(int note, int velocity, bool isNoteOn) {
    if (noteCallback_) {
        noteCallback_(note, velocity, isNoteOn);
    }
}

bool MidiKeyboard::handleInput(const InputEvent& event) {
    if (!isVisible() || !isEnabled()) {
        return false;
    }
    
    switch (event.type) {
        case InputEventType::TouchPress: {
            int localX = static_cast<int>(event.value) - x_;
            int localY = static_cast<int>(event.value2) - y_;
            
            KeyInfo* key = findKeyAtPosition(x_ + localX, y_ + localY);
            if (key && !key->isPressed) {
                int velocity = calculateVelocity(*key, y_ + localY);
                setNotePressed(key->note, true, velocity);
                
                isDragging_ = true;
                dragStartX_ = localX;
                dragStartY_ = localY;
                lastHoveredKey_ = key;
                return true;
            }
            break;
        }
        
        case InputEventType::TouchMove: {
            if (isDragging_) {
                int localX = static_cast<int>(event.value) - x_;
                int localY = static_cast<int>(event.value2) - y_;
                
                KeyInfo* key = findKeyAtPosition(x_ + localX, y_ + localY);
                
                // Handle key changes during drag
                if (key != lastHoveredKey_) {
                    if (lastHoveredKey_ && lastHoveredKey_->isPressed) {
                        setNotePressed(lastHoveredKey_->note, false, 0);
                    }
                    
                    if (key && !key->isPressed) {
                        int velocity = calculateVelocity(*key, y_ + localY);
                        setNotePressed(key->note, true, velocity);
                    }
                    
                    lastHoveredKey_ = key;
                }
                return true;
            }
            break;
        }
        
        case InputEventType::TouchRelease: {
            if (isDragging_) {
                if (lastHoveredKey_ && lastHoveredKey_->isPressed) {
                    setNotePressed(lastHoveredKey_->note, false, 0);
                }
                
                isDragging_ = false;
                lastHoveredKey_ = nullptr;
                return true;
            }
            break;
        }
        
        default:
            break;
    }
    
    return false;
}

void MidiKeyboard::update(float deltaTime) {
    // Update key animations
    for (auto& anim : keyAnimations_) {
        int note = anim.first;
        float& animValue = anim.second;
        
        bool isPressed = (pressedNotes_.find(note) != pressedNotes_.end());
        float target = isPressed ? 1.0f : 0.0f;
        
        // Smooth animation
        float animSpeed = 8.0f; // Animation speed
        animValue += (target - animValue) * animSpeed * deltaTime;
        
        // Update key color based on animation
        auto keyIt = noteToKey_.find(note);
        if (keyIt != noteToKey_.end()) {
            KeyInfo* key = keyIt->second;
            
            if (isPressed) {
                // Blend to pressed color
                Color baseColor = key->isBlackKey ? config_.blackKeyColor : config_.whiteKeyColor;
                Color pressedColor = key->isBlackKey ? config_.pressedBlackColor : config_.pressedWhiteColor;
                
                key->color.r = static_cast<uint8_t>(baseColor.r + animValue * (pressedColor.r - baseColor.r));
                key->color.g = static_cast<uint8_t>(baseColor.g + animValue * (pressedColor.g - baseColor.g));
                key->color.b = static_cast<uint8_t>(baseColor.b + animValue * (pressedColor.b - baseColor.b));
            } else {
                // Blend back to normal color
                Color normalColor = key->isBlackKey ? config_.blackKeyColor : config_.whiteKeyColor;
                key->color = normalColor;
            }
        }
    }
}

void MidiKeyboard::render(DisplayManager* display) {
    if (!isVisible()) {
        return;
    }
    
    // Render white keys first
    for (const auto& key : keys_) {
        if (!key.isBlackKey) {
            // Fill key
            display->fillRect(key.x, key.y, key.width, key.height, key.color);
            
            // Draw border
            display->drawRect(key.x, key.y, key.width, key.height, config_.keyBorderColor);
            
            // Add subtle 3D effect for white keys
            if (!key.isPressed) {
                Color highlight(255, 255, 255);
                display->drawLine(key.x + 1, key.y + 1, key.x + key.width - 2, key.y + 1, highlight);
                display->drawLine(key.x + 1, key.y + 1, key.x + 1, key.y + key.height - 2, highlight);
            }
        }
    }
    
    // Render black keys on top
    for (const auto& key : keys_) {
        if (key.isBlackKey) {
            // Fill key
            display->fillRect(key.x, key.y, key.width, key.height, key.color);
            
            // Draw border
            display->drawRect(key.x, key.y, key.width, key.height, config_.keyBorderColor);
            
            // Add subtle highlight for black keys when not pressed
            if (!key.isPressed) {
                Color highlight(80, 80, 80);
                display->drawLine(key.x + 1, key.y + 1, key.x + key.width - 2, key.y + 1, highlight);
                display->drawLine(key.x + 1, key.y + 1, key.x + 1, key.y + key.height - 2, highlight);
            }
        }
    }
    
    // Draw octave markers
    for (int octave = 0; octave <= config_.numOctaves; octave++) {
        int x = x_ + octave * 7 * config_.whiteKeyWidth;
        int y = y_ + config_.whiteKeyHeight - 15;
        
        if (octave < config_.numOctaves) {
            std::string octaveLabel = "C" + std::to_string(config_.startOctave + octave);
            display->drawText(x + 2, y, octaveLabel, nullptr, Color(100, 100, 100));
        }
    }
}

std::string MidiKeyboard::getNoteName(int note) {
    const char* noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = noteToOctave(note);
    int noteInOctave = note % 12;
    
    std::stringstream ss;
    ss << noteNames[noteInOctave] << octave;
    return ss.str();
}

} // namespace AIMusicHardware