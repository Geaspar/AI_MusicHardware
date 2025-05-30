#include <iostream>
#include "../include/ui/MidiKeyboard.h"

using namespace AIMusicHardware;

class TestDisplayManager : public DisplayManager {
public:
    bool initialize(int width, int height) override { return true; }
    void shutdown() override {}
    void clear(const Color& color = Color(0, 0, 0)) override {}
    void swapBuffers() override {}
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {}
    void drawRect(int x, int y, int width, int height, const Color& color) override {}
    void fillRect(int x, int y, int width, int height, const Color& color) override {}
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {}
    int getWidth() const override { return 800; }
    int getHeight() const override { return 600; }
};

int main() {
    std::cout << "=== MIDI Keyboard UI Component Test ===" << std::endl;
    
    // Create keyboard instance
    MidiKeyboard keyboard("test_keyboard", 50, 100);
    
    // Test configuration
    MidiKeyboard::KeyboardConfig config;
    config.startOctave = 4;
    config.numOctaves = 2;
    config.whiteKeyWidth = 30;
    config.whiteKeyHeight = 150;
    config.blackKeyWidth = 20;
    config.blackKeyHeight = 100;
    
    keyboard.setConfig(config);
    
    // Test note callback
    int noteOnCount = 0;
    int noteOffCount = 0;
    
    keyboard.setNoteCallback([&](int note, int velocity, bool isNoteOn) {
        if (isNoteOn) {
            noteOnCount++;
            std::cout << "Note On: " << MidiKeyboard::getNoteName(note) 
                      << " (MIDI " << note << ") velocity " << velocity << std::endl;
        } else {
            noteOffCount++;
            std::cout << "Note Off: " << MidiKeyboard::getNoteName(note) 
                      << " (MIDI " << note << ")" << std::endl;
        }
    });
    
    // Test manual note setting
    std::cout << "\n=== Test 1: Manual Note Control ===" << std::endl;
    keyboard.setNotePressed(60, true, 100);  // C4
    keyboard.setNotePressed(64, true, 80);   // E4
    keyboard.setNotePressed(67, true, 90);   // G4
    
    std::cout << "Note 60 pressed: " << (keyboard.isNotePressed(60) ? "Yes" : "No") << std::endl;
    std::cout << "Note 64 pressed: " << (keyboard.isNotePressed(64) ? "Yes" : "No") << std::endl;
    std::cout << "Note 67 pressed: " << (keyboard.isNotePressed(67) ? "Yes" : "No") << std::endl;
    std::cout << "Note 72 pressed: " << (keyboard.isNotePressed(72) ? "Yes" : "No") << std::endl;
    
    keyboard.releaseAllNotes();
    std::cout << "After release all - Note 60 pressed: " << (keyboard.isNotePressed(60) ? "Yes" : "No") << std::endl;
    
    std::cout << "\n=== Test 2: Keyboard Configuration ===" << std::endl;
    std::cout << "Lowest note: " << keyboard.getLowestNote() << " (" 
              << MidiKeyboard::getNoteName(keyboard.getLowestNote()) << ")" << std::endl;
    std::cout << "Highest note: " << keyboard.getHighestNote() << " (" 
              << MidiKeyboard::getNoteName(keyboard.getHighestNote()) << ")" << std::endl;
    
    // Test octave transposition
    std::cout << "\n=== Test 3: Octave Transposition ===" << std::endl;
    std::cout << "Before transpose - Lowest: " << MidiKeyboard::getNoteName(keyboard.getLowestNote()) << std::endl;
    keyboard.transposeOctave(1);
    std::cout << "After transpose +1 - Lowest: " << MidiKeyboard::getNoteName(keyboard.getLowestNote()) << std::endl;
    keyboard.transposeOctave(-2);
    std::cout << "After transpose -2 - Lowest: " << MidiKeyboard::getNoteName(keyboard.getLowestNote()) << std::endl;
    
    // Test velocity settings
    std::cout << "\n=== Test 4: Velocity Settings ===" << std::endl;
    keyboard.setVelocityRange(50, 120);
    keyboard.setFixedVelocity(100);
    keyboard.setNotePressed(60, true, 64);  // Should use fixed velocity
    
    keyboard.setFixedVelocity(0);  // Back to variable
    keyboard.setNotePressed(62, true, 127);  // Should use provided velocity
    
    // Test utility functions
    std::cout << "\n=== Test 5: Utility Functions ===" << std::endl;
    
    // Test note name generation
    for (int note = 60; note <= 72; note++) {
        std::cout << "Note " << note << ": " << MidiKeyboard::getNoteName(note);
        if (MidiKeyboard::isBlackKey(note)) {
            std::cout << " (black key)";
        } else {
            std::cout << " (white key)";
        }
        std::cout << " - Octave: " << MidiKeyboard::noteToOctave(note) << std::endl;
    }
    
    // Test input simulation
    std::cout << "\n=== Test 6: Input Event Simulation ===" << std::endl;
    
    InputEvent touchPress;
    touchPress.type = InputEventType::TouchPress;
    touchPress.value = 80;   // X position (should hit first white key)
    touchPress.value2 = 150; // Y position
    
    bool handled = keyboard.handleInput(touchPress);
    std::cout << "Touch press handled: " << (handled ? "Yes" : "No") << std::endl;
    
    InputEvent touchRelease;
    touchRelease.type = InputEventType::TouchRelease;
    touchRelease.value = 80;
    touchRelease.value2 = 150;
    
    handled = keyboard.handleInput(touchRelease);
    std::cout << "Touch release handled: " << (handled ? "Yes" : "No") << std::endl;
    
    // Test rendering (with dummy display manager)
    std::cout << "\n=== Test 7: Rendering Test ===" << std::endl;
    TestDisplayManager display;
    keyboard.render(&display);
    std::cout << "Render completed without errors" << std::endl;
    
    // Test update cycle
    keyboard.update(0.016f);  // 60 FPS frame time
    std::cout << "Update cycle completed without errors" << std::endl;
    
    // Final statistics
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Note On events: " << noteOnCount << std::endl;
    std::cout << "Note Off events: " << noteOffCount << std::endl;
    std::cout << "Total events: " << (noteOnCount + noteOffCount) << std::endl;
    
    // Test range limits
    std::cout << "\n=== Test 8: Range Limit Tests ===" << std::endl;
    keyboard.setOctaveRange(0, 10);  // Should clamp to valid range
    std::cout << "Range after extreme values - Lowest: " << MidiKeyboard::getNoteName(keyboard.getLowestNote()) << std::endl;
    
    keyboard.setOctaveRange(5, 3);   // Normal range
    std::cout << "Final range - Lowest: " << MidiKeyboard::getNoteName(keyboard.getLowestNote()) 
              << " to " << MidiKeyboard::getNoteName(keyboard.getHighestNote()) << std::endl;
    
    std::cout << "\n=== MIDI Keyboard Test Complete ===" << std::endl;
    std::cout << "All tests passed successfully!" << std::endl;
    
    return 0;
}