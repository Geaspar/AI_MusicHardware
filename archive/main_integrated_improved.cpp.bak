#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <SDL2/SDL.h>

// Core systems
#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/effects/EffectProcessor.h"
#include "../include/sequencer/Sequencer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/hardware/HardwareInterface.h"

// Enhanced UI system
#include "../include/ui/UIContext.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/PresetBrowserUIComponent.h"
#include "../include/ui/VisualizationComponents.h"
#include "../include/ui/ParameterUpdateQueue.h"
#include "../include/ui/MidiKeyboard.h"
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"
#include "../include/midi/MidiCCLearning.h"

using namespace AIMusicHardware;

// Improved layout constants
namespace Layout {
    constexpr int WINDOW_WIDTH = 1400;
    constexpr int WINDOW_HEIGHT = 900;
    constexpr int KNOB_SIZE = 80;
    constexpr int SMALL_KNOB_SIZE = 70;
    constexpr int KNOB_SPACING = 120;
    constexpr int SECTION_HEIGHT = 180;
    constexpr int SECTION_SPACING = 200;
    constexpr int MARGIN = 50;
    constexpr int LABEL_HEIGHT = 25;
}

// Custom SDL DisplayManager for rendering
class SDLDisplayManager : public DisplayManager {
public:
    SDLDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(Layout::WINDOW_WIDTH), height_(Layout::WINDOW_HEIGHT) {}
    
    bool initialize(int width, int height) override {
        width_ = width;
        height_ = height;
        return renderer_ != nullptr;
    }
    
    void shutdown() override {
        renderer_ = nullptr;
    }
    
    void clear(const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
    }
    
    void present() override {
        if (!renderer_) return;
        SDL_RenderPresent(renderer_);
    }
    
    void drawRect(int x, int y, int width, int height, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_Rect rect = {x, y, width, height};
        SDL_RenderDrawRect(renderer_, &rect);
    }
    
    void fillRect(int x, int y, int width, int height, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_Rect rect = {x, y, width, height};
        SDL_RenderFillRect(renderer_, &rect);
    }
    
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }
    
    void drawCircle(int centerX, int centerY, int radius, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        for (int w = 0; w < radius * 2; w++) {
            for (int h = 0; h < radius * 2; h++) {
                int dx = radius - w;
                int dy = radius - h;
                if ((dx*dx + dy*dy) <= (radius * radius)) {
                    SDL_RenderDrawPoint(renderer_, centerX + dx, centerY + dy);
                }
            }
        }
    }
    
    void fillCircle(int centerX, int centerY, int radius, const Color& color) override {
        drawCircle(centerX, centerY, radius, color);
    }
    
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {
        // For now, we'll use simple rectangle placeholder for text
        if (!renderer_) return;
        
        // Draw text background
        SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 200);
        SDL_Rect bgRect = {x-2, y-2, static_cast<int>(text.length() * 8 + 4), 16};
        SDL_RenderFillRect(renderer_, &bgRect);
        
        // Draw text as colored rectangles (placeholder)
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        for (size_t i = 0; i < text.length() && i < 50; ++i) {
            SDL_Rect charRect = {x + static_cast<int>(i * 8), y, 6, 12};
            SDL_RenderFillRect(renderer_, &charRect);
        }
    }
    
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }

private:
    SDL_Renderer* renderer_;
    int width_;
    int height_;
};

// Helper class for better UI section organization
class UISection {
public:
    UISection(const std::string& title, int x, int y, int width) 
        : title_(title), x_(x), y_(y), width_(width) {}
    
    void addToScreen(Screen* screen) {
        // Section background
        auto bg = std::make_unique<Label>("bg_" + title_, "");
        bg->setPosition(x_ - 10, y_ - 10);
        bg->setSize(width_ + 20, Layout::SECTION_HEIGHT + 20);
        bg->setBackgroundColor(Color(50, 50, 60, 100));
        screen->addChild(std::move(bg));
        
        // Section title
        auto titleLabel = std::make_unique<Label>("title_" + title_, title_);
        titleLabel->setPosition(x_, y_);
        titleLabel->setSize(width_, Layout::LABEL_HEIGHT);
        titleLabel->setTextColor(Color(220, 220, 255));
        titleLabel->setBackgroundColor(Color(70, 70, 90));
        screen->addChild(std::move(titleLabel));
    }
    
    int getKnobX(int index) const {
        return x_ + (index * Layout::KNOB_SPACING) + (Layout::KNOB_SPACING - Layout::KNOB_SIZE) / 2;
    }
    
    int getKnobY() const {
        return y_ + 40;
    }
    
    int getLabelY() const {
        return getKnobY() + Layout::KNOB_SIZE + 10;
    }

private:
    std::string title_;
    int x_, y_, width_;
};

std::unique_ptr<SynthKnob> createLabeledKnob(
    const std::string& name, 
    const std::string& label,
    int x, int y, 
    float min = 0.0f, float max = 1.0f, float def = 0.5f,
    Screen* screen = nullptr) {
    
    auto knob = std::make_unique<SynthKnob>(name, x, y, Layout::KNOB_SIZE, min, max, def);
    
    // Add label underneath knob
    if (screen) {
        auto labelWidget = std::make_unique<Label>("label_" + name, label);
        labelWidget->setPosition(x - 10, y + Layout::KNOB_SIZE + 5);
        labelWidget->setSize(Layout::KNOB_SIZE + 20, 20);
        labelWidget->setTextColor(Color(180, 180, 200));
        labelWidget->setBackgroundColor(Color(40, 40, 50));
        screen->addChild(std::move(labelWidget));
    }
    
    return knob;
}

int main() {
    std::cout << "Starting AIMusicHardware Integrated Application with Improved UI..." << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "AIMusicHardware - Professional Synthesizer (Improved UI)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        Layout::WINDOW_WIDTH, Layout::WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create display manager
    auto displayManager = std::make_unique<SDLDisplayManager>(renderer);
    displayManager->initialize(Layout::WINDOW_WIDTH, Layout::WINDOW_HEIGHT);
    
    // Initialize core systems
    auto audioEngine = std::make_unique<AudioEngine>();
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine" << std::endl;
        return 1;
    }
    
    auto synthesizer = std::make_unique<Synthesizer>();
    if (!synthesizer->initialize()) {
        std::cerr << "Failed to initialize synthesizer" << std::endl;
        return 1;
    }
    
    // Initialize MIDI and CC Learning
    auto midiInterface = std::make_unique<MidiInterface>();
    midiInterface->initialize();
    
    auto ccLearning = std::make_unique<MidiCCLearning>(midiInterface.get());
    
    // Create UI context
    auto uiContext = std::make_unique<UIContext>(displayManager.get());
    uiContext->initialize();
    
    // Create main screen with improved layout
    auto mainScreen = std::make_unique<Screen>("main");
    mainScreen->setBackgroundColor(Color(35, 35, 45));
    mainScreen->setPosition(0, 0);
    mainScreen->setSize(Layout::WINDOW_WIDTH, Layout::WINDOW_HEIGHT);
    
    // Title bar
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware - Professional Synthesizer");
    titleLabel->setPosition(Layout::MARGIN, 10);
    titleLabel->setSize(600, 35);
    titleLabel->setTextColor(Color(220, 230, 255));
    titleLabel->setBackgroundColor(Color(60, 60, 80));
    mainScreen->addChild(std::move(titleLabel));
    
    // Status bar
    auto statusLabel = std::make_unique<Label>("status", "Ready - Use mouse to control parameters");
    statusLabel->setPosition(Layout::MARGIN, Layout::WINDOW_HEIGHT - 40);
    statusLabel->setSize(800, 25);
    statusLabel->setTextColor(Color(150, 200, 150));
    statusLabel->setBackgroundColor(Color(40, 40, 50));
    mainScreen->addChild(std::move(statusLabel));
    
    // Create organized sections
    UISection oscSection("OSCILLATOR", Layout::MARGIN, 70, 350);
    oscSection.addToScreen(mainScreen.get());
    
    UISection filterSection("FILTER", Layout::MARGIN + 380, 70, 250);
    filterSection.addToScreen(mainScreen.get());
    
    UISection envSection("ENVELOPE", Layout::MARGIN + 660, 70, 400);
    envSection.addToScreen(mainScreen.get());
    
    UISection masterSection("MASTER", Layout::MARGIN + 1090, 70, 200);
    masterSection.addToScreen(mainScreen.get());
    
    // Oscillator knobs with proper spacing and labels
    auto freqKnob = createLabeledKnob("frequency", "FREQUENCY", 
        oscSection.getKnobX(0), oscSection.getKnobY(), 
        20.0f, 20000.0f, 440.0f, mainScreen.get());
    freqKnob->setValueFormatter([](float value) {
        if (value >= 1000.0f) {
            return std::to_string(static_cast<int>(value / 1000.0f)) + " kHz";
        }
        return std::to_string(static_cast<int>(value)) + " Hz";
    });
    
    auto waveKnob = createLabeledKnob("waveform", "WAVEFORM",
        oscSection.getKnobX(1), oscSection.getKnobY(),
        0.0f, 4.0f, 0.0f, mainScreen.get());
    waveKnob->setValueFormatter([](float value) {
        const char* waveNames[] = {"Sine", "Saw", "Square", "Triangle", "Noise"};
        int index = static_cast<int>(value);
        return std::string(waveNames[std::clamp(index, 0, 4)]);
    });
    
    auto detuneKnob = createLabeledKnob("detune", "DETUNE",
        oscSection.getKnobX(2), oscSection.getKnobY(),
        -50.0f, 50.0f, 0.0f, mainScreen.get());
    detuneKnob->setValueFormatter([](float value) {
        return std::to_string(static_cast<int>(value)) + " cents";
    });
    
    // Filter knobs
    auto cutoffKnob = createLabeledKnob("cutoff", "CUTOFF",
        filterSection.getKnobX(0), filterSection.getKnobY(),
        20.0f, 20000.0f, 1000.0f, mainScreen.get());
    cutoffKnob->setValueFormatter([](float value) {
        if (value >= 1000.0f) {
            return std::to_string(static_cast<int>(value / 100.0f) / 10.0f) + " kHz";
        }
        return std::to_string(static_cast<int>(value)) + " Hz";
    });
    
    auto resKnob = createLabeledKnob("resonance", "RESONANCE",
        filterSection.getKnobX(1), filterSection.getKnobY(),
        0.0f, 1.0f, 0.1f, mainScreen.get());
    resKnob->setValueFormatter([](float value) {
        return std::to_string(static_cast<int>(value * 100)) + "%";
    });
    
    // Envelope knobs  
    auto attackKnob = createLabeledKnob("attack", "ATTACK",
        envSection.getKnobX(0), envSection.getKnobY(),
        0.001f, 3.0f, 0.01f, mainScreen.get());
    attackKnob->setValueFormatter([](float value) {
        if (value < 0.1f) {
            return std::to_string(static_cast<int>(value * 1000)) + " ms";
        }
        return std::to_string(static_cast<int>(value * 100) / 100.0f) + " s";
    });
    
    auto decayKnob = createLabeledKnob("decay", "DECAY",
        envSection.getKnobX(1), envSection.getKnobY(),
        0.001f, 3.0f, 0.1f, mainScreen.get());
    decayKnob->setValueFormatter([](float value) {
        if (value < 0.1f) {
            return std::to_string(static_cast<int>(value * 1000)) + " ms";
        }
        return std::to_string(static_cast<int>(value * 100) / 100.0f) + " s";
    });
    
    auto sustainKnob = createLabeledKnob("sustain", "SUSTAIN",
        envSection.getKnobX(2), envSection.getKnobY(),
        0.0f, 1.0f, 0.7f, mainScreen.get());
    sustainKnob->setValueFormatter([](float value) {
        return std::to_string(static_cast<int>(value * 100)) + "%";
    });
    
    auto releaseKnob = createLabeledKnob("release", "RELEASE",
        envSection.getKnobX(3), envSection.getKnobY(),
        0.001f, 5.0f, 0.3f, mainScreen.get());
    releaseKnob->setValueFormatter([](float value) {
        if (value < 0.1f) {
            return std::to_string(static_cast<int>(value * 1000)) + " ms";
        }
        return std::to_string(static_cast<int>(value * 100) / 100.0f) + " s";
    });
    
    // Master knobs
    auto volumeKnob = createLabeledKnob("volume", "VOLUME",
        masterSection.getKnobX(0), masterSection.getKnobY(),
        0.0f, 1.0f, 0.7f, mainScreen.get());
    volumeKnob->setValueFormatter([](float value) {
        if (value <= 0.0f) return "-∞ dB";
        float db = 20.0f * std::log10(value);
        return std::to_string(static_cast<int>(db)) + " dB";
    });
    
    // Add all knobs to screen
    mainScreen->addChild(std::move(freqKnob));
    mainScreen->addChild(std::move(waveKnob));
    mainScreen->addChild(std::move(detuneKnob));
    mainScreen->addChild(std::move(cutoffKnob));
    mainScreen->addChild(std::move(resKnob));
    mainScreen->addChild(std::move(attackKnob));
    mainScreen->addChild(std::move(decayKnob));
    mainScreen->addChild(std::move(sustainKnob));
    mainScreen->addChild(std::move(releaseKnob));
    mainScreen->addChild(std::move(volumeKnob));
    
    // Add preset section
    UISection presetSection("PRESETS", Layout::MARGIN, 300, 600);
    presetSection.addToScreen(mainScreen.get());
    
    // Preset buttons
    std::vector<std::string> presetNames = {
        "Deep Bass", "Bright Lead", "Lush Pad", "Acid Lead", "Warm Lead"
    };
    
    for (size_t i = 0; i < presetNames.size(); ++i) {
        auto presetBtn = std::make_unique<Button>("preset_" + std::to_string(i), presetNames[i]);
        presetBtn->setPosition(Layout::MARGIN + 20 + (i * 110), 350);
        presetBtn->setSize(100, 30);
        presetBtn->setBackgroundColor(Color(70, 100, 70));
        presetBtn->setTextColor(Color(220, 255, 220));
        presetBtn->setClickCallback([i, &presetNames]() {
            std::cout << "Loading preset: " << presetNames[i] << std::endl;
        });
        mainScreen->addChild(std::move(presetBtn));
    }
    
    // Help text
    auto helpLabel = std::make_unique<Label>("help", 
        "Controls: Mouse drag knobs • Double-click to reset • Shift+drag for fine control");
    helpLabel->setPosition(Layout::MARGIN, 420);
    helpLabel->setSize(800, 20);
    helpLabel->setTextColor(Color(150, 150, 170));
    mainScreen->addChild(std::move(helpLabel));
    
    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    
    std::cout << "UI Layout improved with proper spacing and labels" << std::endl;
    
    // Main loop
    bool running = true;
    SDL_Event event;
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            
            // Convert SDL events to UI events
            InputEvent uiEvent;
            switch (event.type) {
                case SDL_MOUSEBUTTONDOWN:
                    uiEvent.type = InputEventType::TouchPress;
                    uiEvent.value = event.button.x;
                    uiEvent.value2 = event.button.y;
                    uiEvent.id = event.button.button;
                    uiContext->handleInput(uiEvent);
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    uiEvent.type = InputEventType::TouchRelease;
                    uiEvent.value = event.button.x;
                    uiEvent.value2 = event.button.y;
                    uiEvent.id = event.button.button;
                    uiContext->handleInput(uiEvent);
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (event.motion.state & SDL_BUTTON_LMASK) {
                        uiEvent.type = InputEventType::TouchMove;
                        uiEvent.value = event.motion.x;
                        uiEvent.value2 = event.motion.y;
                        uiContext->handleInput(uiEvent);
                    }
                    break;
                    
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
            }
        }
        
        // Update systems
        uiContext->update(deltaTime);
        
        // Render
        displayManager->clear(Color(35, 35, 45));
        uiContext->render();
        displayManager->present();
        
        // Small delay to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Cleanup
    std::cout << "Shutting down..." << std::endl;
    
    uiContext.reset();
    displayManager.reset();
    audioEngine.reset();
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Application terminated successfully." << std::endl;
    return 0;
}