#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <iomanip>
#include "../include/synthesis/modulators/modulation_matrix.h"
#include <sstream>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <SDL2/SDL.h>
#ifdef HAVE_SDL_TTF
#include <SDL_ttf.h>
#endif

// Core systems
#include "../include/audio/AudioEngine.h"
#include "../include/audio/Synthesizer.h"
#include "../include/effects/EffectProcessor.h"
#include "../include/sequencer/Sequencer.h"
#include "../include/midi/MidiInterface.h"
#include "../include/hardware/HardwareInterface.h"

// Real-time wavetable engine support
#include "../include/synthesis/RealtimeWavetableVoice.h"
#include "../include/synthesis/FrequencyDomainWavetable.h"

// Enhanced UI system
#include "../include/ui/UIContext.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/PresetBrowserUIComponent.h"
#include "../include/ui/VisualizationComponents.h"
#include "../include/ui/ParameterUpdateQueue.h"
#include "../include/ui/MidiKeyboard.h"
#include "../include/ui/DropdownMenu.h"
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"
#include "../include/midi/MidiCCLearning.h"

// Effects
#include "../include/effects/Filter.h"
#include "../include/effects/AllEffects.h"

// IoT support
// #include "../include/iot/DummyIoTInterface.h" // Disabled to avoid crash

using namespace AIMusicHardware;

// Custom SDL DisplayManager for rendering
class SDLDisplayManager : public DisplayManager {
public:
    SDLDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(1280), height_(800), 
                                             font_(nullptr), fontLarge_(nullptr), fontSmall_(nullptr) {
#ifdef HAVE_SDL_TTF
        // Initialize SDL_ttf
        if (TTF_Init() == -1) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        } else {
            // Load fonts in different sizes
            font_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 14);          // Normal text
            fontLarge_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 18);     // Section headers
            fontSmall_ = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 12);     // Small labels
            
            if (!font_ || !fontLarge_ || !fontSmall_) {
                std::cerr << "Failed to load some fonts: " << TTF_GetError() << std::endl;
            } else {
                std::cout << "SDL_ttf initialized with multiple font sizes" << std::endl;
            }
        }
#endif
    }
    
    ~SDLDisplayManager() {
#ifdef HAVE_SDL_TTF
        if (font_) TTF_CloseFont(font_);
        if (fontLarge_) TTF_CloseFont(fontLarge_);
        if (fontSmall_) TTF_CloseFont(fontSmall_);
        TTF_Quit();
#endif
    }
    
    bool initialize(int width, int height) override {
        width_ = width;
        height_ = height;
        return renderer_ != nullptr;
    }
    
    void shutdown() override {
        // Ensure we don't try to use the renderer after SDL cleanup
        renderer_ = nullptr;
    }
    
    void clear(const Color& color = Color(0, 0, 0)) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
    }
    
    void swapBuffers() override {
        if (!renderer_) return;
        SDL_RenderPresent(renderer_);
    }
    
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {
        if (!renderer_) return;
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }
    
    void drawRect(int x, int y, int width, int height, const Color& color) override {
        if (!renderer_) return;
        SDL_Rect rect = { x, y, width, height };
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawRect(renderer_, &rect);
    }
    
    void fillRect(int x, int y, int width, int height, const Color& color) override {
        if (!renderer_) return;
        SDL_Rect rect = { x, y, width, height };
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer_, &rect);
    }
    
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {
        // Check if this looks like a section header based on content
        bool isHeader = (text.find("OSCILLATOR") != std::string::npos ||
                        text.find("FILTER") != std::string::npos ||
                        text.find("ENVELOPE") != std::string::npos ||
                        text.find("MASTER") != std::string::npos ||
                        text.find("VISUALIZATION") != std::string::npos ||
                        text.find("KEYBOARD") != std::string::npos ||
                        text.find("PRESET") != std::string::npos ||
                        text.find("MIDI CC") != std::string::npos ||
                        text.find("TRANSPORT") != std::string::npos ||
                        text.find("PERFORMANCE") != std::string::npos);
        
        // Check if this looks like a small knob label (knobs render their own labels)
        bool isSmallLabel = false;
        
        if (isHeader) {
            drawTextWithSize(x, y, text, color, TextSize::Large);
        } else if (isSmallLabel) {
            drawTextWithSize(x, y, text, color, TextSize::Small);
        } else {
            drawTextWithSize(x, y, text, color, TextSize::Normal);
        }
    }
    
    enum class TextSize {
        Small,
        Normal, 
        Large
    };
    
    void drawTextWithSize(int x, int y, const std::string& text, const Color& color, TextSize size) {
        if (!renderer_) return;
        
#ifdef HAVE_SDL_TTF
        TTF_Font* selectedFont = font_;
        switch (size) {
            case TextSize::Small: selectedFont = fontSmall_; break;
            case TextSize::Large: selectedFont = fontLarge_; break;
            default: selectedFont = font_; break;
        }
        
        if (selectedFont) {
            // Use SDL_ttf for real text rendering
            SDL_Color textColor = { color.r, color.g, color.b, 255 };
            SDL_Surface* textSurface = TTF_RenderText_Solid(selectedFont, text.c_str(), textColor);
            
            if (textSurface) {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer_, textSurface);
                if (textTexture) {
                    int textWidth = textSurface->w;
                    int textHeight = textSurface->h;
                    SDL_Rect destRect = { x, y, textWidth, textHeight };
                    
                    SDL_RenderCopy(renderer_, textTexture, nullptr, &destRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            return;
        }
#endif
    }
    
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    
private:
    SDL_Renderer* renderer_;
    int width_;
    int height_;
#ifdef HAVE_SDL_TTF
    TTF_Font* font_;
    TTF_Font* fontLarge_;
    TTF_Font* fontSmall_;
#endif
};

// Helper to translate SDL events to our InputEvent format
InputEvent translateSDLEvent(const SDL_Event& sdlEvent) {
    InputEvent event;
    
    switch (sdlEvent.type) {
        case SDL_MOUSEBUTTONDOWN:
            event.type = InputEventType::TouchPress;
            event.id = 0;
            event.value = static_cast<float>(sdlEvent.button.x);
            event.value2 = static_cast<float>(sdlEvent.button.y);
            break;
            
        case SDL_MOUSEBUTTONUP:
            event.type = InputEventType::TouchRelease;
            event.id = 0;
            event.value = static_cast<float>(sdlEvent.button.x);
            event.value2 = static_cast<float>(sdlEvent.button.y);
            break;
            
        case SDL_MOUSEMOTION:
            if (sdlEvent.motion.state & SDL_BUTTON_LMASK) {
                event.type = InputEventType::TouchMove;
                event.id = 0;
                event.value = static_cast<float>(sdlEvent.motion.x);
                event.value2 = static_cast<float>(sdlEvent.motion.y);
            }
            break;
            
        case SDL_KEYDOWN:
            event.type = InputEventType::ButtonPress;
            event.id = sdlEvent.key.keysym.sym;
            break;
            
        case SDL_KEYUP:
            event.type = InputEventType::ButtonRelease;
            event.id = sdlEvent.key.keysym.sym;
            break;
            
        case SDL_MOUSEWHEEL:
            event.type = InputEventType::EncoderRotate;
            event.id = 0;
            event.value = static_cast<float>(sdlEvent.wheel.y);
            break;
    }
    
    return event;
}

// Audio processing callback for real-time thread
void audioCallback(AudioEngine* audioEngine, Synthesizer* synthesizer, 
                  EffectProcessor* effectProcessor, Sequencer* sequencer,
                  WaveformVisualizer* waveform, LevelMeter* levelMeter,
                  float* outputBuffer, int numFrames) {
    
    // Process sequencer - TEMPORARILY DISABLED to debug duplicate notes
    // sequencer->process(static_cast<float>(numFrames) / audioEngine->getSampleRate());
    
    // Process synthesizer
    synthesizer->process(outputBuffer, numFrames);
    
    // Process effects
    effectProcessor->process(outputBuffer, numFrames);
    
    // Update visualizers (thread-safe)
    if (waveform) {
        waveform->pushSamples(outputBuffer, numFrames, 2);
    }
    
    // Calculate RMS for level meter
    if (levelMeter) {
        float rms = 0.0f;
        for (int i = 0; i < numFrames * 2; i += 2) {
            float sample = (outputBuffer[i] + outputBuffer[i + 1]) * 0.5f;
            rms += sample * sample;
        }
        rms = std::sqrt(rms / numFrames);
        levelMeter->setLevel(rms * 2.0f);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "AI Music Hardware - Integrated UI Version" << std::endl;
    std::cout << "Starting production-ready synthesizer..." << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "AI Music Hardware - Professional Synthesizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create SDL renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create core audio components
    auto audioEngine = std::make_unique<AudioEngine>();
    auto synthesizer = std::make_unique<Synthesizer>();
    
    // Use real-time wavetable voice manager and provide a frequency-domain wavetable
    synthesizer->setVoiceManagerType(AIMusicHardware::VoiceManagerType::RealTime);
    synthesizer->setOversamplingEnabled(false); // Start with no oversampling for best performance
    {
        auto wavetable = std::make_shared<FrequencyDomainWavetable>();
        bool loaded = wavetable->loadFromFile("generated_presets/sine.json");
        if (!loaded) {
            // Fallback when launched from build/bin
            loaded = wavetable->loadFromFile("../../generated_presets/sine.json");
        }
        if (loaded) {
            if (auto* rtVoiceManager = dynamic_cast<RealtimeWavetableVoiceManager*>(synthesizer->getVoiceManager())) {
                rtVoiceManager->setWavetable(wavetable);
                std::cout << "Realtime wavetable loaded successfully." << std::endl;
            } else {
                std::cout << "Realtime voice manager not active; cannot set frequency-domain wavetable." << std::endl;
            }
        } else {
            std::cerr << "Failed to load frequency-domain wavetable (sine.json). Real-time engine will be inactive." << std::endl;
        }
    }
    
    auto effectProcessor = std::make_unique<EffectProcessor>();
    // Shared mutex to synchronize audio thread with UI thread modifications
    std::mutex audioMutex;
    auto sequencer = std::make_unique<Sequencer>();
    auto midiInput = std::make_unique<MidiInput>();
    auto midiOutput = std::make_unique<MidiOutput>();
    auto midiHandler = std::make_unique<MidiHandler>();
    auto hardwareInterface = std::make_unique<HardwareInterface>();
    // MIDI activity indicator state
    std::atomic<bool> midiActivityPulse{false};
    float midiActivityTimer = 0.0f; // seconds to keep the light on after a message
    
    // Persistence config
    auto getUserConfigPath = []() -> std::string {
        try {
            std::string home;
            if (const char* h = std::getenv("HOME")) home = h;
#ifdef __APPLE__
            std::filesystem::path dir = std::filesystem::path(home) / "Library" / "Application Support" / "AIMusicHardware";
#else
            std::filesystem::path dir = std::filesystem::path(home) / ".aimusichardware";
#endif
            std::filesystem::create_directories(dir);
            auto path = (dir / "user_config.json").string();
            return path;
        } catch (...) {
            return std::string("user_config.json");
        }
    };
    const std::string userConfigPath = getUserConfigPath();
    nlohmann::json loadedConfig;
    // Try to load user config
    {
        std::ifstream in(userConfigPath);
        if (in.good()) {
            try {
                in >> loadedConfig;
                std::cout << "Loaded user config from: " << userConfigPath << std::endl;
            } catch (...) {
                loadedConfig = nlohmann::json{};
            }
        } else {
            std::cout << "No user config found at: " << userConfigPath << std::endl;
        }
    }

    std::string persistedMidiDeviceName;
    bool persistedHybridEnabled = false;
    if (loadedConfig.contains("midi") && loadedConfig["midi"].contains("deviceName")) {
        persistedMidiDeviceName = loadedConfig["midi"]["deviceName"].get<std::string>();
    }
    if (loadedConfig.contains("engine") && loadedConfig["engine"].contains("hybrid_enabled")) {
        try { persistedHybridEnabled = loadedConfig["engine"]["hybrid_enabled"].get<bool>(); } catch (...) {}
    }
    bool persistedMinPhase = false;
    if (loadedConfig.contains("engine") && loadedConfig["engine"].contains("timbre_min_phase")) {
        try { persistedMinPhase = loadedConfig["engine"]["timbre_min_phase"].get<bool>(); } catch (...) {}
    }

    // Initialize external MIDI input - but don't open any device yet
    std::cout << "\n=== Initializing External MIDI Controller Support ===" << std::endl;
    auto midiDevices = midiInput->getDevices();
    int currentMidiDevice = -1; // -1 means no device selected
    
    if (midiDevices.empty()) {
        std::cout << "No MIDI input devices found. External MIDI disabled." << std::endl;
    } else {
        std::cout << "Found " << midiDevices.size() << " MIDI input device(s):" << std::endl;
        for (size_t i = 0; i < midiDevices.size(); ++i) {
            std::cout << "  " << i << ": " << midiDevices[i] << std::endl;
        }
        
        // Check for Oxi One specifically
        int oxiIndex = -1;
        for (size_t i = 0; i < midiDevices.size(); ++i) {
            if (midiDevices[i].find("OXI ONE") != std::string::npos) {
                oxiIndex = static_cast<int>(i);
                std::cout << "Found OXI ONE at index " << oxiIndex << std::endl;
                break;
            }
        }
        
        // Auto-select device based on priority: command line arg > persisted name > Oxi One > first device
        int selectedDevice = -1;
        if (argc > 1) {
            selectedDevice = std::atoi(argv[1]);
            if (selectedDevice < 0 || selectedDevice >= static_cast<int>(midiDevices.size())) {
                std::cout << "Invalid device index. Will select later." << std::endl;
                selectedDevice = -1;
            }
        } else if (!persistedMidiDeviceName.empty()) {
            for (size_t i = 0; i < midiDevices.size(); ++i) {
                if (midiDevices[i] == persistedMidiDeviceName) {
                    selectedDevice = static_cast<int>(i);
                    std::cout << "Auto-selecting persisted MIDI device: " << persistedMidiDeviceName << std::endl;
                    break;
                }
            }
        } else if (oxiIndex >= 0) {
            selectedDevice = oxiIndex;
            std::cout << "Auto-selecting OXI ONE" << std::endl;
        } else if (!midiDevices.empty()) {
            selectedDevice = 0;
            std::cout << "Auto-selecting first available device" << std::endl;
        }
        
        if (selectedDevice >= 0) {
            std::cout << "\nOpening MIDI device " << selectedDevice << ": " << midiDevices[selectedDevice] << std::endl;
            if (midiInput->openDevice(selectedDevice)) {
                std::cout << "Successfully opened MIDI device!" << std::endl;
                std::cout << "External MIDI controller ready for input." << std::endl;
                currentMidiDevice = selectedDevice;
            } else {
                std::cerr << "Failed to open MIDI device. External MIDI disabled." << std::endl;
            }
        } else {
            std::cout << "No MIDI device selected. Use the dropdown in the UI to select one." << std::endl;
        }
    }
    std::cout << "====================================================\n" << std::endl;
    
    // Initialize core components
    if (!synthesizer->initialize()) {
        std::cerr << "Failed to initialize synthesizer!" << std::endl;
        return 1;
    }
    
    if (!effectProcessor->initialize()) {
        std::cerr << "Failed to initialize effect processor!" << std::endl;
        return 1;
    }
    
    if (!sequencer->initialize()) {
        std::cerr << "Failed to initialize sequencer!" << std::endl;
        return 1;
    }
    
    // Ensure sequencer is stopped to prevent unwanted note triggers
    sequencer->stop();
    std::cout << "Sequencer initialized and stopped. Playing: " << (sequencer->isPlaying() ? "YES" : "NO") << std::endl;
    
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    
    // Add a global low-pass filter to the external effect processor (will be kept at chain END)
    {
        auto filter = std::make_unique<Filter>(audioEngine->getSampleRate(), Filter::Type::LowPass);
        filter->setParameter("mix", 1.0f); // Full wet signal
        filter->setParameter("frequency", 20000.0f); // Start wide open
        filter->setParameter("resonance", 1.0f); // Low resonance
        effectProcessor->addEffect(std::move(filter));
        std::cout << "Added global low-pass filter to effect processor (initial)" << std::endl;
    }
    
    // Connect synthesizer to external effect processor so it can control the filter
    synthesizer->setExternalEffectProcessor(effectProcessor.get());
    
    // Initialize hardware (non-critical)
    if (!hardwareInterface->initialize()) {
        std::cerr << "Hardware interface unavailable, continuing without hardware..." << std::endl;
    }
    
    // Create UI context with SDL display manager
    auto uiContext = std::make_unique<UIContext>();
    auto sdlDisplayManager = std::make_shared<SDLDisplayManager>(renderer);
    uiContext->setDisplayManager(sdlDisplayManager);
    uiContext->initialize(1280, 800);
    
    // Initialize parameter manager
    auto& paramManager = EnhancedParameterManager::getInstance();
    // Skip IoT for now to avoid crash
    // paramManager.connectIoTInterface(dummyIoT.get());
    paramManager.connectSynthesizer(synthesizer.get());
    
    // Initialize MIDI CC Learning system
    auto& ccLearning = MidiCCLearningManager::getInstance();
    ccLearning.initialize();
    
    // Create parameter mapping storage for CC learning integration
    std::map<std::string, Slider*> parameterSliders; // Map parameter IDs to sliders
    
    // Set up CC learning to update synthesizer parameters and UI
    ccLearning.getLearning().setParameterChangeCallback([&synthesizer, &parameterSliders](const std::string& parameterId, float value) {
        // Update synthesizer
        synthesizer->setParameter(parameterId, value);
        
        // Update corresponding UI slider if it exists
        auto sliderIt = parameterSliders.find(parameterId);
        if (sliderIt != parameterSliders.end() && sliderIt->second) {
            sliderIt->second->setValue(value);
        }
        
        std::cout << "CC Learning -> " << parameterId << " = " << value << std::endl;
    });
    
    // Create main synthesizer screen first
    auto mainScreen = std::make_unique<Screen>("main");
    
    // Set up learning state callback for UI feedback
    ccLearning.getLearning().setLearningStateCallback([](MidiCCLearning::LearningState state, const std::string& message) {
        std::cout << "Learning State: " << message << std::endl;
        // Status could be displayed in a different tab later
    });
    
    auto connectSliderToParam = [&](Slider* slider, const std::string& paramId) {
        if (slider) {
            // Store slider reference for CC learning updates
            parameterSliders[paramId] = slider;
            
            // Set up value change callback to update synthesizer
            slider->setValueChangeCallback([&synthesizer, paramId](float value) {
                // For oscillator_type, pass the actual value (0-4)
                // For other parameters, they're already in the correct range
                synthesizer->setParameter(paramId, value);
                std::cout << "Updated " << paramId << " to " << value << std::endl;
            });
            
            // Initialize slider with current parameter value
            float currentValue = synthesizer->getParameter(paramId);
            slider->setValue(currentValue);
        }
    };
    
    // Helper to add parameter-specific learning functionality
    auto addParameterLearning = [&](Slider* slider, const std::string& paramId, int x, int y) {
        if (slider) {
            // Add a small learn button next to the knob
            auto learnButton = std::make_unique<Button>("learn_" + paramId, "L");
            learnButton->setPosition(x + 85, y + 30); // Position next to knob
            learnButton->setSize(20, 20);
            learnButton->setBackgroundColor(Color(80, 80, 120));
            learnButton->setTextColor(Color(255, 255, 255));
            learnButton->setClickCallback([&ccLearning, paramId]() {
                auto& learning = ccLearning.getLearning();
                if (learning.getLearningState() == MidiCCLearning::LearningState::Idle) {
                    learning.startLearning(paramId, std::chrono::milliseconds{5000});
                    std::cout << "Started learning for parameter: " << paramId << std::endl;
                } else {
                    learning.stopLearning();
                }
            });
            mainScreen->addChild(std::move(learnButton));
        }
    };
    mainScreen->setBackgroundColor(Color(40, 40, 50)); // Lighter background
    mainScreen->setPosition(0, 0);
    mainScreen->setSize(1280, 800);
    std::cout << "Created main screen" << std::endl;
    
    // Navigation buttons at the top
    // Main button
    auto mainNavButton = std::make_unique<Button>("main_nav_btn", "Main");
    mainNavButton->setPosition(50, 5);
    mainNavButton->setSize(80, 30);
    mainNavButton->setBackgroundColor(Color(100, 100, 140));
    mainNavButton->setTextColor(Color(255, 255, 255));
    mainNavButton->setClickCallback([&uiContext]() {
        uiContext->setActiveScreen("main");
    });
    mainScreen->addChild(std::move(mainNavButton));
    
    // Removed duplicate nav text label; button text is sufficient
    
    // Effects button
    auto effectsNavButton = std::make_unique<Button>("effects_nav_btn", "Effects");
    effectsNavButton->setPosition(140, 5);
    effectsNavButton->setSize(80, 30);
    effectsNavButton->setBackgroundColor(Color(80, 80, 120));
    effectsNavButton->setTextColor(Color(255, 255, 255));
    effectsNavButton->setClickCallback([&]() {
        uiContext->setActiveScreen("effects");
        // Deferred: advanced FX UI refresh (labels/pages) — removed due to undefined symbols in this commit
    });
    mainScreen->addChild(std::move(effectsNavButton));
    
    // Removed duplicate nav text label; button text is sufficient
    
    // Modulation button
    auto modNavButton = std::make_unique<Button>("mod_nav_btn", "Modulation");
    modNavButton->setPosition(230, 5);
    modNavButton->setSize(100, 30);
    modNavButton->setBackgroundColor(Color(80, 80, 120));
    modNavButton->setTextColor(Color(255, 255, 255));
    modNavButton->setClickCallback([&uiContext]() {
        uiContext->setActiveScreen("modulation");
    });
    mainScreen->addChild(std::move(modNavButton));
    
    // Removed duplicate nav text label; button text is sufficient
    
    // Presets button
    auto presetsNavButton = std::make_unique<Button>("presets_nav_btn", "Presets");
    presetsNavButton->setPosition(340, 5);
    presetsNavButton->setSize(80, 30);
    presetsNavButton->setBackgroundColor(Color(80, 80, 120));
    presetsNavButton->setTextColor(Color(255, 255, 255));
    presetsNavButton->setClickCallback([&uiContext]() {
        uiContext->setActiveScreen("presets");
    });
    mainScreen->addChild(std::move(presetsNavButton));
    
    // Removed duplicate nav text label; button text is sufficient
    
    // Settings button
    auto settingsNavButton = std::make_unique<Button>("settings_nav_btn", "Settings");
    settingsNavButton->setPosition(430, 5);
    settingsNavButton->setSize(80, 30);
    settingsNavButton->setBackgroundColor(Color(80, 80, 120));
    settingsNavButton->setTextColor(Color(255, 255, 255));
    settingsNavButton->setClickCallback([&uiContext]() {
        uiContext->setActiveScreen("settings");
    });
    mainScreen->addChild(std::move(settingsNavButton));
    
    // Removed duplicate nav text label; button text is sufficient
    
    // Sequencer button
    auto sequencerNavButton = std::make_unique<Button>("seq_nav_btn", "Sequencer");
    sequencerNavButton->setPosition(520, 5);
    sequencerNavButton->setSize(100, 30);
    sequencerNavButton->setBackgroundColor(Color(80, 80, 120));
    sequencerNavButton->setTextColor(Color(255, 255, 255));
    sequencerNavButton->setClickCallback([&uiContext]() {
        uiContext->setActiveScreen("sequencer");
    });
    mainScreen->addChild(std::move(sequencerNavButton));
    
    // Removed duplicate nav text label; button text is sufficient
    
    // Create oscillator section with bright colors
    auto oscSection = std::make_unique<Label>("osc_section", "OSCILLATOR");
    oscSection->setPosition(50, 40);
    oscSection->setSize(200, 25);
    oscSection->setTextColor(Color(255, 255, 100)); // Bright yellow for visibility
    mainScreen->addChild(std::move(oscSection));
    
    // Create oscillator sliders connected to synthesizer parameters
    auto freqSlider = std::make_unique<Slider>("freq_slider", "Frequency", 50, 85, 40, 100);
    freqSlider->setRange(20.0f, 20000.0f);
    freqSlider->setValue(440.0f);
    freqSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " Hz";
        return ss.str();
    });
    freqSlider->setColor(Color(255, 255, 100));
    freqSlider->setThumbColor(Color(255, 200, 100));
    Slider* freqSliderPtr = freqSlider.get();
    mainScreen->addChild(std::move(freqSlider));
    
    auto waveSlider = std::make_unique<Slider>("wave_slider", "Wave", 170, 85, 40, 100);
    waveSlider->setRange(0.0f, 4.0f);
    waveSlider->setValue(0.0f);
    waveSlider->setStep(1.0f);
    waveSlider->setValueFormatter([](float value) {
        const char* waveNames[] = {"Sine", "Square", "Saw", "Triangle", "Noise"};
        int index = static_cast<int>(value);
        if (index >= 0 && index < 5) {
            return std::string(waveNames[index]);
        }
        return std::string("Unknown");
    });
    waveSlider->setColor(Color(255, 255, 100));
    waveSlider->setThumbColor(Color(255, 200, 100));
    Slider* waveSliderPtr = waveSlider.get();
    mainScreen->addChild(std::move(waveSlider));
    
    // Create filter section
    auto filterSection = std::make_unique<Label>("filter_section", "FILTER");
    filterSection->setPosition(350, 40);
    filterSection->setSize(180, 25);
    filterSection->setTextColor(Color(100, 255, 100)); // Bright green for visibility
    mainScreen->addChild(std::move(filterSection));
    
    auto cutoffSlider = std::make_unique<Slider>("cutoff_slider", "Cutoff", 350, 85, 40, 100);
    // Use normalized 0-1 range for internal value, will convert to frequency
    cutoffSlider->setRange(0.0f, 1.0f);
    // Set to 1.0 which will map to 20kHz (filter wide open)
    cutoffSlider->setValue(1.0f);
    cutoffSlider->setValueFormatter([](float normalizedValue) {
        // Convert normalized value to frequency using logarithmic scale
        // 0.0 = 20 Hz, 0.5 = 500 Hz, 1.0 = 20000 Hz
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;
        float logMin = std::log10(minFreq);
        float logMax = std::log10(maxFreq);
        
        // Map so that 0.5 = 500 Hz
        // We need to solve for the curve that passes through (0,20), (0.5,500), (1,20000)
        // Using exponential mapping: freq = 20 * (1000)^normalizedValue
        float freq = minFreq * std::pow(1000.0f, normalizedValue);
        
        std::stringstream ss;
        if (freq >= 1000.0f) {
            ss << std::fixed << std::setprecision(1) << freq / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(0) << freq << " Hz";
        }
        return ss.str();
    });
    cutoffSlider->setColor(Color(100, 255, 100));
    cutoffSlider->setThumbColor(Color(100, 200, 255));
    Slider* cutoffSliderPtr = cutoffSlider.get();
    mainScreen->addChild(std::move(cutoffSlider));
    
    auto resSlider = std::make_unique<Slider>("res_slider", "Resonance", 460, 85, 40, 100);
    resSlider->setRange(0.0f, 1.0f);
    resSlider->setValue(0.1f); // Low resonance by default
    resSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    resSlider->setColor(Color(100, 255, 100));
    resSlider->setThumbColor(Color(100, 200, 255));
    Slider* resSliderPtr = resSlider.get();
    mainScreen->addChild(std::move(resSlider));
    
    // Create envelope section  
    auto envSection = std::make_unique<Label>("env_section", "ENVELOPE");
    envSection->setPosition(590, 40);
    envSection->setSize(200, 25);
    envSection->setTextColor(Color(255, 100, 255)); // Bright magenta for visibility
    mainScreen->addChild(std::move(envSection));
    
    auto attackSlider = std::make_unique<Slider>("attack_slider", "Attack", 590, 85, 40, 100);
    attackSlider->setRange(0.0f, 2.0f);
    attackSlider->setValue(0.02f);  // Changed from 0.01f to 0.02f for smoother attack
    attackSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    attackSlider->setColor(Color(255, 100, 255));
    attackSlider->setThumbColor(Color(255, 150, 255));
    Slider* attackSliderPtr = attackSlider.get();
    mainScreen->addChild(std::move(attackSlider));
    
    auto decaySlider = std::make_unique<Slider>("decay_slider", "Decay", 680, 85, 40, 100);
    decaySlider->setRange(0.0f, 2.0f);
    decaySlider->setValue(0.1f);
    decaySlider->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    decaySlider->setColor(Color(255, 100, 255));
    decaySlider->setThumbColor(Color(255, 150, 255));
    Slider* decaySliderPtr = decaySlider.get();
    mainScreen->addChild(std::move(decaySlider));
    
    auto sustainSlider = std::make_unique<Slider>("sustain_slider", "Sustain", 770, 85, 40, 100);
    sustainSlider->setRange(0.0f, 1.0f);
    sustainSlider->setValue(0.7f);
    sustainSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    sustainSlider->setColor(Color(255, 100, 255));
    sustainSlider->setThumbColor(Color(255, 150, 255));
    Slider* sustainSliderPtr = sustainSlider.get();
    mainScreen->addChild(std::move(sustainSlider));
    
    auto releaseSlider = std::make_unique<Slider>("release_slider", "Release", 860, 85, 40, 100);
    releaseSlider->setRange(0.0f, 4.0f);
    releaseSlider->setValue(0.5f);
    releaseSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value < 0.1f) {
            ss << std::fixed << std::setprecision(0) << value * 1000.0f << " ms";
        } else {
            ss << std::fixed << std::setprecision(2) << value << " s";
        }
        return ss.str();
    });
    releaseSlider->setColor(Color(255, 100, 255));
    releaseSlider->setThumbColor(Color(255, 150, 255));
    Slider* releaseSliderPtr = releaseSlider.get();
    mainScreen->addChild(std::move(releaseSlider));
    
    // Create master section
    auto masterSection = std::make_unique<Label>("master_section", "MASTER");
    masterSection->setPosition(980, 40);
    masterSection->setSize(130, 25);
    masterSection->setTextColor(Color(100, 200, 255)); // Bright cyan for visibility
    mainScreen->addChild(std::move(masterSection));
    
    auto volumeSlider = std::make_unique<Slider>("volume_slider", "Volume", 980, 85, 40, 100);
    volumeSlider->setRange(0.0f, 1.0f);
    volumeSlider->setValue(0.75f);
    volumeSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value <= 0.0f) {
            ss << "-∞ dB";
        } else {
            // Map 0-1 to -40dB to +6dB range (matching the synthesizer)
            float db = -40.0f + (value * 46.0f);
            ss << std::fixed << std::setprecision(1) << db << " dB";
        }
        return ss.str();
    });
    volumeSlider->setColor(Color(100, 200, 255));
    volumeSlider->setThumbColor(Color(150, 200, 255));
    Slider* volumeSliderPtr = volumeSlider.get();
    mainScreen->addChild(std::move(volumeSlider));

    // (Reset button will be added later, after MIDI indicator, for correct z-order)
    
    // Create LFO section with selector dropdown
    auto lfoSection = std::make_unique<Label>("lfo_section", "LFO");
    lfoSection->setPosition(900, 220);
    lfoSection->setSize(80, 25);
    lfoSection->setTextColor(Color(200, 255, 200)); // Light green for LFO
    mainScreen->addChild(std::move(lfoSection));
    
    // Create LFO selector dropdown (will be added later for z-order)
    std::unique_ptr<DropdownMenu> lfoSelectorDropdown;
    DropdownMenu* lfoSelectorDropdownPtr = nullptr;
    lfoSelectorDropdown = std::make_unique<DropdownMenu>("lfo_selector", "LFO 1");
    lfoSelectorDropdown->setPosition(980, 220);
    lfoSelectorDropdown->setSize(100, 25);
    lfoSelectorDropdown->addItem("LFO 1");
    lfoSelectorDropdown->addItem("LFO 2");
    lfoSelectorDropdown->selectItem(0); // Start with LFO 1
    lfoSelectorDropdownPtr = lfoSelectorDropdown.get();
    
    // Create shared LFO sliders (will display current selected LFO)
    auto lfoRateSlider = std::make_unique<Slider>("lfo_rate", "Rate", 900, 250, 40, 80);
    lfoRateSlider->setRange(0.1f, 20.0f);
    lfoRateSlider->setValue(1.0f);
    lfoRateSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " Hz";
        return ss.str();
    });
    lfoRateSlider->setColor(Color(180, 255, 180));
    lfoRateSlider->setThumbColor(Color(150, 255, 150));
    Slider* lfoRateSliderPtr = lfoRateSlider.get();
    mainScreen->addChild(std::move(lfoRateSlider));
    
    auto lfoDepthSlider = std::make_unique<Slider>("lfo_depth", "Depth", 990, 250, 40, 80);
    lfoDepthSlider->setRange(0.0f, 1.0f);
    lfoDepthSlider->setValue(1.0f);
    lfoDepthSlider->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    lfoDepthSlider->setColor(Color(180, 255, 180));
    lfoDepthSlider->setThumbColor(Color(150, 255, 150));
    Slider* lfoDepthSliderPtr = lfoDepthSlider.get();
    mainScreen->addChild(std::move(lfoDepthSlider));
    
    auto lfoShapeSlider = std::make_unique<Slider>("lfo_shape", "Shape", 1080, 250, 40, 80);
    lfoShapeSlider->setRange(0, 4); // 5 wave shapes
    lfoShapeSlider->setValue(0); // Sine
    lfoShapeSlider->setValueFormatter([](float value) {
        int shape = static_cast<int>(value);
        const char* shapes[] = {"Sine", "Triangle", "Saw", "Square", "Random"};
        return std::string(shapes[shape % 5]);
    });
    lfoShapeSlider->setColor(Color(180, 255, 180));
    lfoShapeSlider->setThumbColor(Color(150, 255, 150));
    Slider* lfoShapeSliderPtr = lfoShapeSlider.get();
    mainScreen->addChild(std::move(lfoShapeSlider));
    
    // Store LFO 1 and LFO 2 parameter values
    struct LFOState {
        float rate;
        float depth;
        float shape;
    };
    LFOState lfo1State = {1.0f, 1.0f, 0.0f};  // LFO 1 defaults
    LFOState lfo2State = {2.0f, 0.5f, 2.0f};  // LFO 2 defaults
    int currentLFO = 1;  // Currently selected LFO (1 or 2)
    
    // Create parameter name mappings for the current LFO
    auto getCurrentLFOParamName = [&currentLFO](const std::string& param) -> std::string {
        return "lfo" + std::to_string(currentLFO) + "_" + param;
    };
    
    // Set up LFO selector callback
    lfoSelectorDropdown->setSelectionCallback([&synthesizer, &currentLFO, &lfo1State, &lfo2State, 
                                               lfoRateSliderPtr, lfoDepthSliderPtr, lfoShapeSliderPtr,
                                               &getCurrentLFOParamName, &parameterSliders]
                                              (int index, const std::string& item) {
        // Save current LFO state
        if (currentLFO == 1) {
            lfo1State.rate = lfoRateSliderPtr->getValue();
            lfo1State.depth = lfoDepthSliderPtr->getValue();
            lfo1State.shape = lfoShapeSliderPtr->getValue();
        } else {
            lfo2State.rate = lfoRateSliderPtr->getValue();
            lfo2State.depth = lfoDepthSliderPtr->getValue();
            lfo2State.shape = lfoShapeSliderPtr->getValue();
        }
        
        // Switch to new LFO
        currentLFO = index + 1;  // index 0 = LFO 1, index 1 = LFO 2
        
        // Load new LFO state
        const LFOState& newState = (currentLFO == 1) ? lfo1State : lfo2State;
        lfoRateSliderPtr->setValue(newState.rate);
        lfoDepthSliderPtr->setValue(newState.depth);
        lfoShapeSliderPtr->setValue(newState.shape);
        
        // Update parameter mappings
        parameterSliders[getCurrentLFOParamName("rate")] = lfoRateSliderPtr;
        parameterSliders[getCurrentLFOParamName("depth")] = lfoDepthSliderPtr;
        parameterSliders[getCurrentLFOParamName("shape")] = lfoShapeSliderPtr;
        
        // Update colors based on selected LFO
        Color lfoColor = (currentLFO == 1) ? Color(180, 255, 180) : Color(180, 180, 255);
        Color thumbColor = (currentLFO == 1) ? Color(150, 255, 150) : Color(150, 150, 255);
        
        lfoRateSliderPtr->setColor(lfoColor);
        lfoRateSliderPtr->setThumbColor(thumbColor);
        lfoDepthSliderPtr->setColor(lfoColor);
        lfoDepthSliderPtr->setThumbColor(thumbColor);
        lfoShapeSliderPtr->setColor(lfoColor);
        lfoShapeSliderPtr->setThumbColor(thumbColor);
        
        std::cout << "Switched to " << item << std::endl;
    });
    
    // We'll need to store these pointers for later connection
    Slider* lfo1RateSliderPtr = lfoRateSliderPtr;   // These will be the same sliders
    Slider* lfo1DepthSliderPtr = lfoDepthSliderPtr;
    Slider* lfo1ShapeSliderPtr = lfoShapeSliderPtr;
    Slider* lfo2RateSliderPtr = lfoRateSliderPtr;   // But we'll handle them specially
    Slider* lfo2DepthSliderPtr = lfoDepthSliderPtr;
    Slider* lfo2ShapeSliderPtr = lfoShapeSliderPtr;
    
    // Create visualization section
    auto vizSection = std::make_unique<Label>("viz_section", "VISUALIZATION");
    vizSection->setPosition(50, 220);
    vizSection->setSize(200, 25);
    vizSection->setTextColor(Color(255, 200, 100)); // Bright orange for visibility
    mainScreen->addChild(std::move(vizSection));
    
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setPosition(50, 250);
    waveform->setSize(220, 150);
    waveform->setWaveformColor(Color(0, 255, 128));
    mainScreen->addChild(std::move(waveform));
    
    // Create filter visualizer (Vital-style) - between waveform and envelope
    auto filterViz = std::make_unique<FilterVisualizer>("filter_viz");
    filterViz->setPosition(280, 250);
    filterViz->setSize(300, 150);
    filterViz->setCurveColor(Color(100, 255, 100));
    filterViz->setFillColor(Color(100, 255, 100, 50));
    filterViz->setBackgroundColor(Color(30, 30, 35));
    filterViz->setGridColor(Color(50, 50, 55));
    filterViz->showGrid(true);
    filterViz->showFill(true);
    filterViz->setEditable(true);
    filterViz->setFilterType(FilterVisualizer::FilterType::LowPass);
    filterViz->setCutoffFrequency(1000.0f);
    filterViz->setResonance(0.7f);
    filterViz->setSampleRate(audioEngine->getSampleRate());
    FilterVisualizer* filterVizPtr = filterViz.get();
    mainScreen->addChild(std::move(filterViz));
    
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setPosition(590, 250);
    envelope->setSize(250, 150);
    // Match initial values with slider defaults
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    EnvelopeVisualizer* envelopePtr = envelope.get();
    mainScreen->addChild(std::move(envelope));
    
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    levelMeter->setPosition(850, 250);
    levelMeter->setSize(30, 150);
    mainScreen->addChild(std::move(levelMeter));
    
    // Create MIDI keyboard section
    auto keyboardSection = std::make_unique<Label>("keyboard_section", "MIDI KEYBOARD");
    keyboardSection->setPosition(50, 430);
    keyboardSection->setTextColor(Color(255, 150, 255)); // Bright pink for visibility
    mainScreen->addChild(std::move(keyboardSection));
    
    // Declare MIDI device dropdown (will be added last for proper z-order)
    std::unique_ptr<DropdownMenu> midiDeviceDropdownPtr;
    
    // Create MIDI device selector dropdown (but don't add to screen yet)
    midiDeviceDropdownPtr = std::make_unique<DropdownMenu>("midi_device_selector", "MIDI Device");
    midiDeviceDropdownPtr->setPosition(650, 430);
    midiDeviceDropdownPtr->setSize(200, 25);
    
    // Add "None" option first
    midiDeviceDropdownPtr->addItem("None");
    
    // Add all available MIDI devices
    for (const auto& device : midiDevices) {
        midiDeviceDropdownPtr->addItem(device);
    }
    
    // Set current selection
    if (currentMidiDevice >= 0 && currentMidiDevice < static_cast<int>(midiDevices.size())) {
        midiDeviceDropdownPtr->selectItem(currentMidiDevice + 1); // +1 because "None" is at index 0
    } else {
        midiDeviceDropdownPtr->selectItem(0); // "None"
    }
    
    // Add callback to handle device selection
    midiDeviceDropdownPtr->setSelectionCallback([&midiInput, &midiHandler, &currentMidiDevice, &midiDevices, &persistedMidiDeviceName](int index, const std::string& item) {
        std::cout << "MIDI Device Selection: " << item << " (index " << index << ")" << std::endl;
        
        // Close current device if any
        if (currentMidiDevice >= 0) {
            midiInput->closeDevice();
            currentMidiDevice = -1;
        }
        
        // Open new device if selected (index 0 is "None")
        if (index > 0 && index <= static_cast<int>(midiDevices.size())) {
            int deviceIndex = index - 1; // Adjust for "None" at index 0
            std::cout << "Opening MIDI device " << deviceIndex << ": " << midiDevices[deviceIndex] << std::endl;
            
            if (midiInput->openDevice(deviceIndex)) {
                currentMidiDevice = deviceIndex;
                // Re-establish callback connection
                midiInput->setCallback(midiHandler.get());
                std::cout << "Successfully opened MIDI device: " << midiDevices[deviceIndex] << std::endl;
                persistedMidiDeviceName = midiDevices[deviceIndex];
            } else {
                std::cerr << "Failed to open MIDI device: " << midiDevices[deviceIndex] << std::endl;
            }
        } else {
            // None selected
            persistedMidiDeviceName.clear();
        }
    });
    
    // Create the MIDI keyboard
    auto midiKeyboard = std::make_unique<MidiKeyboard>("midi_keyboard", 50, 460);
    
    // Configure keyboard for 3 octaves starting from C3
    MidiKeyboard::KeyboardConfig keyboardConfig;
    keyboardConfig.startOctave = 3;     // C3 - C6 range
    keyboardConfig.numOctaves = 3;
    keyboardConfig.whiteKeyWidth = 28;   // Slightly larger keys
    keyboardConfig.whiteKeyHeight = 140;
    keyboardConfig.blackKeyWidth = 20;
    keyboardConfig.blackKeyHeight = 90;
    keyboardConfig.whiteKeyColor = Color(250, 250, 250);
    keyboardConfig.blackKeyColor = Color(30, 30, 30);
    keyboardConfig.pressedWhiteColor = Color(100, 150, 255);
    keyboardConfig.pressedBlackColor = Color(80, 120, 200);
    keyboardConfig.keyBorderColor = Color(120, 120, 120);
    
    midiKeyboard->setConfig(keyboardConfig);
    midiKeyboard->setVelocityRange(30, 127);  // More expressive velocity range
    
    // Connect keyboard to synthesizer
    midiKeyboard->setNoteCallback([&synthesizer, &audioEngine](int note, int velocity, bool isNoteOn) {
        if (isNoteOn) {
            float normalizedVelocity = velocity / 127.0f;
            auto now = std::chrono::high_resolution_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            std::cout << "[" << timestamp << " ms] Keyboard Note On: " << MidiKeyboard::getNoteName(note) 
                      << " (note " << note << ") velocity " << velocity 
                      << " normalized: " << normalizedVelocity << std::endl;
            
            // Check audio engine status
            std::cout << "Audio Engine - Sample Rate: " << audioEngine->getSampleRate() 
                      << ", Buffer Size: " << audioEngine->getBufferSize() 
                      << ", Stream Time: " << audioEngine->getStreamTime() << std::endl;
            
            synthesizer->noteOn(note, normalizedVelocity);
            
            // Check synthesizer state
            std::cout << "Master Volume: " << synthesizer->getParameter("master_volume") << std::endl;
            std::cout << "Filter Cutoff: " << synthesizer->getParameter("filter_cutoff") << std::endl;
            std::cout << "Oscillator Type: " << synthesizer->getParameter("oscillator_type") << std::endl;
            
            // Removed auto-bumping low filter cutoff to preserve user setting
        } else {
            auto now = std::chrono::high_resolution_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            synthesizer->noteOff(note);
            std::cout << "[" << timestamp << " ms] Keyboard Note Off: " << MidiKeyboard::getNoteName(note) 
                      << " (note " << note << ")" << std::endl;
        }
    });
    
    // Store pointer for potential external MIDI input display
    MidiKeyboard* midiKeyboardPtr = midiKeyboard.get();
    // MIDI activity light UI on main screen (bottom left)
    auto midiActivityLabel = std::make_unique<Label>("midi_activity_label", "MIDI");
    midiActivityLabel->setPosition(50, 760);
    midiActivityLabel->setSize(50, 20);
    midiActivityLabel->setTextColor(Color(180, 180, 180));

    auto midiActivityLight = std::make_unique<Button>("midi_activity_light", "");
    midiActivityLight->setPosition(100, 755);
    midiActivityLight->setSize(20, 20);
    midiActivityLight->setBackgroundColor(Color(40, 60, 40)); // idle/dim state
    midiActivityLight->setHighlightColor(Color(0, 200, 80));  // active/bright state
    Button* midiActivityLightPtr = midiActivityLight.get();
    mainScreen->addChild(std::move(midiActivityLabel));
    mainScreen->addChild(std::move(midiActivityLight));
    // (Reset button will be added near the end for top-most z-order)
    mainScreen->addChild(std::move(midiKeyboard));
    
    // Add octave control buttons (positioned after keyboard ends)
    auto octaveDownButton = std::make_unique<Button>("octave_down", "OCT-");
    octaveDownButton->setPosition(50, 610);
    octaveDownButton->setSize(60, 30);
    octaveDownButton->setBackgroundColor(Color(80, 80, 100));
    octaveDownButton->setTextColor(Color(255, 255, 255));
    octaveDownButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(-1);
            std::cout << "Keyboard transposed down one octave" << std::endl;
        }
    });
    mainScreen->addChild(std::move(octaveDownButton));
    
    auto octaveUpButton = std::make_unique<Button>("octave_up", "OCT+");
    octaveUpButton->setPosition(120, 610);
    octaveUpButton->setSize(60, 30);
    octaveUpButton->setBackgroundColor(Color(80, 80, 100));
    octaveUpButton->setTextColor(Color(255, 255, 255));
    octaveUpButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(1);
            std::cout << "Keyboard transposed up one octave" << std::endl;
        }
    });
    mainScreen->addChild(std::move(octaveUpButton));
    
    // Add velocity mode button
    auto velocityModeButton = std::make_unique<Button>("velocity_mode", "VEL: VAR");
    velocityModeButton->setPosition(190, 610);
    velocityModeButton->setSize(130, 30);
    velocityModeButton->setBackgroundColor(Color(60, 100, 60));
    velocityModeButton->setTextColor(Color(255, 255, 255));
    velocityModeButton->setToggleMode(true);
    
    // Store raw pointer for callback
    Button* velocityModeButtonPtr = velocityModeButton.get();
    
    bool isFixedVelocity = false;
    velocityModeButton->setClickCallback([midiKeyboardPtr, &isFixedVelocity, velocityModeButtonPtr]() {
        if (midiKeyboardPtr) {
            isFixedVelocity = !isFixedVelocity;
            if (isFixedVelocity) {
                midiKeyboardPtr->setFixedVelocity(100);  // Fixed velocity
                velocityModeButtonPtr->setText("VEL: FIX");
                velocityModeButtonPtr->setBackgroundColor(Color(100, 60, 60));
                std::cout << "Keyboard set to fixed velocity mode" << std::endl;
            } else {
                midiKeyboardPtr->setFixedVelocity(0);    // Variable velocity
                velocityModeButtonPtr->setText("VEL: VAR");
                velocityModeButtonPtr->setBackgroundColor(Color(60, 100, 60));
                std::cout << "Keyboard set to variable velocity mode" << std::endl;
            }
        }
    });
    mainScreen->addChild(std::move(velocityModeButton));
    
    // Create preset selection section (moved to after performance info)
    // Will be added later at bottom right
    
    // Initialize preset system
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    auto presetDatabase = std::make_unique<PresetDatabase>();
    
    // Load real presets from test_presets directory
    std::string presetBaseDir = "test_presets";
    std::vector<std::string> categories = {"Bass", "Lead", "Pad"};
    
    for (const auto& category : categories) {
        std::string categoryDir = presetBaseDir + "/" + category;
        
        // Add presets from each category directory
        // In a real implementation, we'd scan the directory for .json files
        // For now, add the known presets manually
        if (category == "Bass") {
            PresetInfo preset;
            preset.name = "Deep Bass";
            preset.category = "Bass";
            preset.author = "System";
            preset.description = "Deep sub bass sound";
            preset.filePath = categoryDir + "/Deep Bass.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Pluck Bass";
            preset.description = "Percussive pluck bass";
            preset.filePath = categoryDir + "/Pluck Bass.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Sub Bass";
            preset.description = "Powerful sub-bass sound";
            preset.filePath = categoryDir + "/Sub Bass.json";
            presetDatabase->addPreset(preset);
        }
        else if (category == "Lead") {
            PresetInfo preset;
            preset.name = "Acid Lead";
            preset.category = "Lead";
            preset.author = "Alex Johnson";
            preset.description = "Classic acid lead synthesizer";
            preset.filePath = categoryDir + "/Acid Lead.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Bright Lead";
            preset.description = "Cutting lead synthesizer";
            preset.filePath = categoryDir + "/Bright Lead.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Warm Lead";
            preset.description = "Warm analog lead sound";
            preset.filePath = categoryDir + "/Warm Lead.json";
            presetDatabase->addPreset(preset);
        }
        else if (category == "Pad") {
            PresetInfo preset;
            preset.name = "Ambient Pad";
            preset.category = "Pad";
            preset.author = "System";
            preset.description = "Atmospheric pad sound";
            preset.filePath = categoryDir + "/Ambient Pad.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "Lush Pad";
            preset.description = "Rich, lush pad sound";
            preset.filePath = categoryDir + "/Lush Pad.json";
            presetDatabase->addPreset(preset);
            
            preset.name = "String Pad";
            preset.description = "String-like pad sound";
            preset.filePath = categoryDir + "/String Pad.json";
            presetDatabase->addPreset(preset);
        }
    }
    
    // Preset dropdown will be created later at bottom right
    
    // Create MODULATION section
    auto modSection = std::make_unique<Label>("mod_section", "MODULATION ROUTING");
    modSection->setPosition(850, 430);
    modSection->setTextColor(Color(200, 150, 255)); // Purple for modulation
    mainScreen->addChild(std::move(modSection));
    
    // Create modulation routing rows
    const int modRowStartY = 460;
    const int modRowHeight = 35;
    const int modRowCount = 3;
    
    // Modulation sources and destinations
    std::vector<std::string> modSources = {
        "None", "LFO 1", "LFO 2", "Envelope", "Velocity", "Aftertouch", "Mod Wheel"
    };
    
    // Build destinations from engine so all FX params are included
    std::vector<std::string> modDestinations;
    modDestinations.push_back("None");
    {
        auto dyn = synthesizer->getModDestinationNames();
        // Keep Pitch/Volume/Attack/Release/Filter names first if present
        auto prependIf = [&dyn, &modDestinations](const std::string& name){
            auto it = std::find(dyn.begin(), dyn.end(), name);
            if (it != dyn.end()) { modDestinations.push_back(name); dyn.erase(it); }
        };
        prependIf("Pitch");
        prependIf("Filter Cutoff");
        prependIf("Filter Res");
        prependIf("Volume");
        prependIf("Attack");
        prependIf("Release");
        // Append per-slot generic targets (assumes 6 slots)
        for (int s = 1; s <= 6; ++s) {
            for (int p = 1; p <= 4; ++p) {
                std::stringstream ss; ss << "Slot " << s << " — Param " << p; modDestinations.push_back(ss.str());
            }
        }
        // Append the rest (FX params, etc.)
        modDestinations.insert(modDestinations.end(), dyn.begin(), dyn.end());
    }
    
    // Store dropdown references to add them last (for proper z-order)
    std::vector<std::unique_ptr<DropdownMenu>> modSourceDropdowns;
    std::vector<std::unique_ptr<DropdownMenu>> modDestDropdowns;
    std::vector<Slider*> modAmountSliders; // Store amount slider pointers
    
    // Structure to track modulation connections
    struct ModulationConnection {
        std::string source = "None";
        std::string destination = "None";
        float amount = 0.0f;
        int sourceIndex = -1;
        int destIndex = -1;
    };
    std::vector<ModulationConnection> modConnections(modRowCount);
    
    for (int i = 0; i < modRowCount; ++i) {
        int yPos = modRowStartY + (i * modRowHeight);
        
        // Create source dropdown but don't add yet
        auto sourceDropdown = std::make_unique<DropdownMenu>("mod_source_" + std::to_string(i), "Source");
        sourceDropdown->setPosition(850, yPos);
        sourceDropdown->setSize(120, 25);
        sourceDropdown->addItems(modSources);
        
        // Create amount slider
        auto amountSlider = std::make_unique<Slider>("mod_amount_" + std::to_string(i), "", 980, yPos, 80, 25);
        amountSlider->setOrientation(Slider::Orientation::Horizontal);
        amountSlider->setRange(-1.0f, 1.0f);
        amountSlider->setValue(0.0f);
        amountSlider->setValueFormatter([](float value) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(0) << (value * 100.0f) << "%";
            return ss.str();
        });
        amountSlider->setColor(Color(200, 150, 255));
        amountSlider->setThumbColor(Color(220, 170, 255));
        Slider* amountSliderPtr = amountSlider.get();
        // Update connection amount and re-connect if both ends set
        amountSliderPtr->setValueChangeCallback([i, &modConnections, &synthesizer](float v){
            modConnections[i].amount = v;
            if (modConnections[i].sourceIndex > 0 && modConnections[i].destIndex > 0) {
                std::string sourceName = (modConnections[i].source == "LFO 1") ? "LFO1" : (modConnections[i].source == "LFO 2") ? "LFO2" : "";
                if (!sourceName.empty() && modConnections[i].destination != "None") {
                    // Disconnect previous then connect with new amount
                    synthesizer->disconnectModulation(sourceName, modConnections[i].destination);
                    synthesizer->connectModulation(sourceName, modConnections[i].destination, modConnections[i].amount);
                    if (modConnections[i].destination == "Pitch") {
                        float semitones = modConnections[i].amount * 12.0f;
                        if (sourceName == "LFO1") synthesizer->setGlobalPitchModulationAmount("lfo1", semitones);
                        else if (sourceName == "LFO2") synthesizer->setGlobalPitchModulationAmount("lfo2", semitones);
                    }
                }
            }
        });
        modAmountSliders.push_back(amountSliderPtr);
        mainScreen->addChild(std::move(amountSlider));
        
        // Create destination dropdown but don't add yet
        auto destDropdown = std::make_unique<DropdownMenu>("mod_dest_" + std::to_string(i), "Destination");
        destDropdown->setPosition(1070, yPos);
        destDropdown->setSize(130, 25);
        destDropdown->addItems(modDestinations);
        
        // Set up callbacks with proper modulation matrix connection
        sourceDropdown->setSelectionCallback([i, &modConnections, &synthesizer, amountSliderPtr](int index, const std::string& item) {
            std::cout << "Mod " << i << " source: " << item << std::endl;
            
            // Disconnect previous connection if any
            if (modConnections[i].sourceIndex > 0 && modConnections[i].destIndex > 0) {
                std::string oldSourceName = "";
                if (modConnections[i].source == "LFO 1") oldSourceName = "LFO1";
                else if (modConnections[i].source == "LFO 2") oldSourceName = "LFO2";
                
                if (!oldSourceName.empty() && modConnections[i].destination != "None") {
                    synthesizer->disconnectModulation(oldSourceName, modConnections[i].destination);
                }
            }
            
            modConnections[i].source = item;
            modConnections[i].sourceIndex = index;
            
            // Update modulation matrix if both source and destination are set
            if (modConnections[i].sourceIndex > 0 && modConnections[i].destIndex > 0) {
                // Map UI names to internal names
                std::string sourceName = "";
                if (item == "LFO 1") sourceName = "LFO1";
                else if (item == "LFO 2") sourceName = "LFO2";
                else if (item == "Mod Wheel") sourceName = "ModWheel";
                else if (item == "Aftertouch") sourceName = "Aftertouch";
                else if (item == "Velocity") sourceName = "Velocity";
                
                if (!sourceName.empty() && modConnections[i].destination != "None") {
                    // Apply safety limits for certain destinations
                    float safeAmount = modConnections[i].amount;
                    if (modConnections[i].destination == "Filter Res") {
                        // Get current base resonance value (0-1 normalized)
                        float baseResonance = synthesizer->getParameter("filter_resonance");
                        
                        // Calculate what the actual Q values would be at extremes
                        float minQ = 0.7f + (baseResonance + safeAmount * -1.0f) * 9.3f;
                        float maxQ = 0.7f + (baseResonance + safeAmount * 1.0f) * 9.3f;
                        
                        // Ensure Q stays within safe range (0.7 to 5.0 for stability)
                        const float MIN_SAFE_Q = 0.7f;
                        const float MAX_SAFE_Q = 5.0f;
                        
                        if (minQ < MIN_SAFE_Q || maxQ > MAX_SAFE_Q) {
                            // Calculate the maximum safe modulation amount
                            float maxPositiveMod = ((MAX_SAFE_Q - 0.7f) / 9.3f) - baseResonance;
                            float maxNegativeMod = ((MIN_SAFE_Q - 0.7f) / 9.3f) - baseResonance;
                            
                            safeAmount = std::max(maxNegativeMod, std::min(maxPositiveMod, safeAmount));
                            
                            std::cout << "WARNING: Limiting Filter Res modulation to keep Q between " 
                                      << MIN_SAFE_Q << " and " << MAX_SAFE_Q 
                                      << " (amount: " << modConnections[i].amount << " -> " << safeAmount << ")" << std::endl;
                        }
                    }
                    
                    std::cout << "DEBUG: Connecting modulation - Source: " << sourceName 
                              << ", Destination: " << modConnections[i].destination 
                              << ", Amount: " << safeAmount << std::endl;
                    synthesizer->connectModulation(sourceName, modConnections[i].destination, safeAmount);
                    
                    // Special handling for pitch modulation - set the amount in semitones
                    if (modConnections[i].destination == "Pitch") {
                        // Map -1 to 1 modulation amount to -12 to +12 semitones (1 octave range)
                        float semitones = modConnections[i].amount * 12.0f;
                        
                        // Use the new global method with the correct source
                        if (sourceName == "LFO1") {
                            synthesizer->setGlobalPitchModulationAmount("lfo1", semitones);
                        } else if (sourceName == "LFO2") {
                            synthesizer->setGlobalPitchModulationAmount("lfo2", semitones);
                        }
                    }
                }
            }
        });
        
        destDropdown->setSelectionCallback([i, &modConnections, &synthesizer, amountSliderPtr](int index, const std::string& item) {
            std::cout << "Mod " << i << " destination: " << item << std::endl;
            
            // Update amount slider range/appearance based on destination
            if (item == "Filter Res") {
                // Visual feedback that this destination has limited range
                amountSliderPtr->setColor(Color(255, 150, 150)); // Reddish to indicate caution
                std::cout << "NOTE: Filter Res modulation is limited to ±30% for stability" << std::endl;
            } else {
                // Reset to normal color
                amountSliderPtr->setColor(Color(200, 150, 255));
            }
            
            // Disconnect previous connection if any
            if (modConnections[i].sourceIndex > 0 && modConnections[i].destIndex > 0) {
                std::string sourceName = "";
                if (modConnections[i].source == "LFO 1") sourceName = "LFO1";
                else if (modConnections[i].source == "LFO 2") sourceName = "LFO2";
                
                if (!sourceName.empty() && modConnections[i].destination != "None") {
                    synthesizer->disconnectModulation(sourceName, modConnections[i].destination);
                }
            }
            
            modConnections[i].destination = item;
            modConnections[i].destIndex = index;
            
            // Update modulation matrix if both source and destination are set
            if (modConnections[i].sourceIndex > 0 && modConnections[i].destIndex > 0) {
                // Map UI names to internal names
                std::string sourceName = "";
                if (modConnections[i].source == "LFO 1") sourceName = "LFO1";
                else if (modConnections[i].source == "LFO 2") sourceName = "LFO2";
                
                if (!sourceName.empty() && item != "None") {
                    // Apply safety limits for certain destinations
                    float safeAmount = modConnections[i].amount;
                    if (item == "Filter Res") {
                        // Get current base resonance value (0-1 normalized)
                        float baseResonance = synthesizer->getParameter("filter_resonance");
                        
                        // Calculate what the actual Q values would be at extremes
                        float minQ = 0.7f + (baseResonance + safeAmount * -1.0f) * 9.3f;
                        float maxQ = 0.7f + (baseResonance + safeAmount * 1.0f) * 9.3f;
                        
                        // Ensure Q stays within safe range (0.7 to 5.0 for stability)
                        const float MIN_SAFE_Q = 0.7f;
                        const float MAX_SAFE_Q = 5.0f;
                        
                        if (minQ < MIN_SAFE_Q || maxQ > MAX_SAFE_Q) {
                            // Calculate the maximum safe modulation amount
                            float maxPositiveMod = ((MAX_SAFE_Q - 0.7f) / 9.3f) - baseResonance;
                            float maxNegativeMod = ((MIN_SAFE_Q - 0.7f) / 9.3f) - baseResonance;
                            
                            safeAmount = std::max(maxNegativeMod, std::min(maxPositiveMod, safeAmount));
                            
                            std::cout << "WARNING: Limiting Filter Res modulation to keep Q between " 
                                      << MIN_SAFE_Q << " and " << MAX_SAFE_Q 
                                      << " (amount: " << modConnections[i].amount << " -> " << safeAmount << ")" << std::endl;
                        }
                    }
                    synthesizer->connectModulation(sourceName, item, safeAmount);
                    
                    // Special handling for pitch modulation - set the amount in semitones
                    if (item == "Pitch") {
                        // Map -1 to 1 modulation amount to -12 to +12 semitones (1 octave range)
                        float semitones = modConnections[i].amount * 12.0f;
                        
                        // Use the new global method with the correct source
                        if (sourceName == "LFO1") {
                            synthesizer->setGlobalPitchModulationAmount("lfo1", semitones);
                        } else if (sourceName == "LFO2") {
                            synthesizer->setGlobalPitchModulationAmount("lfo2", semitones);
                        }
                    }
                }
            }
        });
        
        // Set up amount slider callback
        amountSliderPtr->setValueChangeCallback([i, &modConnections, &synthesizer](float value) {
            modConnections[i].amount = value;
            std::cout << "Mod " << i << " amount: " << (value * 100.0f) << "%" << std::endl;
            
            // Update modulation amount if connection exists
            if (modConnections[i].sourceIndex > 0 && modConnections[i].destIndex > 0) {
                // Map UI names to internal names
                std::string sourceName = "";
                if (modConnections[i].source == "LFO 1") sourceName = "LFO1";
                else if (modConnections[i].source == "LFO 2") sourceName = "LFO2";
                
                if (!sourceName.empty() && modConnections[i].destination != "None") {
                    // Apply safety limits for certain destinations
                    float safeAmount = value;
                    if (modConnections[i].destination == "Filter Res") {
                        // Get current base resonance value (0-1 normalized)
                        float baseResonance = synthesizer->getParameter("filter_resonance");
                        
                        // Calculate what the actual Q values would be at extremes
                        float minQ = 0.7f + (baseResonance + safeAmount * -1.0f) * 9.3f;
                        float maxQ = 0.7f + (baseResonance + safeAmount * 1.0f) * 9.3f;
                        
                        // Ensure Q stays within safe range (0.7 to 5.0 for stability)
                        const float MIN_SAFE_Q = 0.7f;
                        const float MAX_SAFE_Q = 5.0f;
                        
                        if (minQ < MIN_SAFE_Q || maxQ > MAX_SAFE_Q) {
                            // Calculate the maximum safe modulation amount
                            float maxPositiveMod = ((MAX_SAFE_Q - 0.7f) / 9.3f) - baseResonance;
                            float maxNegativeMod = ((MIN_SAFE_Q - 0.7f) / 9.3f) - baseResonance;
                            
                            safeAmount = std::max(maxNegativeMod, std::min(maxPositiveMod, value));
                            
                            if (safeAmount != value) {
                                std::cout << "WARNING: Limiting Filter Res modulation to keep Q between " 
                                          << MIN_SAFE_Q << " and " << MAX_SAFE_Q 
                                          << " (amount: " << value << " -> " << safeAmount << ")" << std::endl;
                            }
                        }
                    }
                    
                    // Disconnect and reconnect with new amount
                    synthesizer->disconnectModulation(sourceName, modConnections[i].destination);
                    synthesizer->connectModulation(sourceName, modConnections[i].destination, safeAmount);
                    
                    // Special handling for pitch modulation - set the amount in semitones
                    if (modConnections[i].destination == "Pitch") {
                        // Map -1 to 1 modulation amount to -12 to +12 semitones (1 octave range)
                        float semitones = value * 12.0f;
                        
                        // Use the new global method with the correct source
                        if (sourceName == "LFO1") {
                            synthesizer->setGlobalPitchModulationAmount("lfo1", semitones);
                        } else if (sourceName == "LFO2") {
                            synthesizer->setGlobalPitchModulationAmount("lfo2", semitones);
                        }
                    }
                }
            }
        });
        
        modSourceDropdowns.push_back(std::move(sourceDropdown));
        modDestDropdowns.push_back(std::move(destDropdown));
    }
    
    // Quick Effects access on Main screen (synced with Effects tab slots 1-3)
    auto quickFxLabel = std::make_unique<Label>("quick_fx_label", "FX (Quick)");
    quickFxLabel->setPosition(850, 570);
    quickFxLabel->setSize(120, 20);
    quickFxLabel->setTextColor(Color(100, 200, 200));
    mainScreen->addChild(std::move(quickFxLabel));

    const int quickFxStartY = 600;
    const int quickFxSlotHeight = 35;
    const int quickFxCount = 3;

    // Create three main-page effect type dropdowns that mirror fx_type_0..2 on Effects screen
    std::vector<std::unique_ptr<DropdownMenu>> mainEffectDropdowns;
    mainEffectDropdowns.reserve(quickFxCount);
    // Keep raw pointers for cross-screen syncing
    std::vector<DropdownMenu*> quickFxDd(quickFxCount, nullptr);
    // Re-entrancy guards to avoid recursive callback loops
    std::vector<bool> suppressMainToEffects(quickFxCount, false);
    std::vector<bool> suppressEffectsToMain(quickFxCount, false);

    // Helper to find index of an effect name in the shared list ("None" + getAvailableEffects())
    auto findEffectIndexByName = [] (const std::string& name) -> int {
        if (name == "None") return 0;
        const auto effects = AIMusicHardware::getAvailableEffects();
        for (size_t i = 0; i < effects.size(); ++i) {
            if (effects[i] == name) return static_cast<int>(i + 1);
        }
        return -1;
    };

    for (int i = 0; i < quickFxCount; ++i) {
        int yPos = quickFxStartY + i * quickFxSlotHeight;
        auto dd = std::make_unique<DropdownMenu>("effect_type_" + std::to_string(i), "Effect " + std::to_string(i + 1));
        dd->setPosition(850, yPos);
        dd->setSize(180, 25);
        dd->addItem("None");
        for (const auto& t : AIMusicHardware::getAvailableEffects()) dd->addItem(t);
        dd->selectItem(0);
        quickFxDd[i] = dd.get();
        dd->setSelectionCallback([i, &uiContext, &suppressMainToEffects, &suppressEffectsToMain](int index, const std::string& item){
            // Proxy selection to Effects tab slot dropdown to keep a single source of truth
            if (suppressEffectsToMain[i]) return; // skip if this change originated from Effects tab
            if (auto* effectsScreen = uiContext->getScreen("effects")) {
                if (auto* fxDd = dynamic_cast<DropdownMenu*>(effectsScreen->getChild("fx_type_" + std::to_string(i)))) {
                    suppressMainToEffects[i] = true;
                    // Use normal selection so Effects tab callback runs to rebuild chain and configure parameters
                    fxDd->selectItem(index);
                    suppressMainToEffects[i] = false;
                }
            }
        });
        mainEffectDropdowns.push_back(std::move(dd));
    }
    
    // Get pointers to visualization components for audio thread
    WaveformVisualizer* waveformPtr = 
        static_cast<WaveformVisualizer*>(mainScreen->getChild("waveform"));
    LevelMeter* levelPtr = 
        static_cast<LevelMeter*>(mainScreen->getChild("level"));
    
    // Connect UI controls to synthesizer parameters
    std::cout << "Connecting UI controls to synthesizer parameters..." << std::endl;
    
    // Note: For oscillator frame parameter, we need special handling since it uses normalized values
    connectSliderToParam(waveSliderPtr, "oscillator_type");
    
    // Add flags to prevent feedback loops
    std::atomic<bool> updatingFromSlider{false};
    std::atomic<bool> updatingFromVisualizer{false};
    
    // Special handling for filter cutoff - connect to synthesizer parameter system
    if (cutoffSliderPtr) {
        parameterSliders["filter_cutoff"] = cutoffSliderPtr;
        cutoffSliderPtr->setValueChangeCallback([&synthesizer, filterVizPtr, &updatingFromSlider, &updatingFromVisualizer](float normalizedValue) {
            if (updatingFromVisualizer) {
                std::cout << "CUTOFF SLIDER: Ignoring update from visualizer" << std::endl;
                return; // Prevent feedback loop
            }
            
            updatingFromSlider = true;
            
            // Update synthesizer parameter (normalized 0-1)
            synthesizer->setParameter("filter_cutoff", normalizedValue);
            
            // Convert normalized value to frequency for visualizer
            float frequencyHz = 20.0f * std::pow(1000.0f, normalizedValue);
            
            std::cout << "CUTOFF SLIDER: Normalized " << normalizedValue << " -> " << frequencyHz << " Hz" << std::endl;
            
            // Update visualizer
            if (filterVizPtr) {
                filterVizPtr->setCutoffFrequency(frequencyHz);
                std::cout << "CUTOFF SLIDER: Updated visualizer cutoff to " << frequencyHz << " Hz" << std::endl;
            }
            
            updatingFromSlider = false;
        });
        // Initialize with 20kHz (normalized 1.0) - filter wide open
        cutoffSliderPtr->setValue(1.0f);
    }
    
    // Special handling for filter resonance - connect to synthesizer parameter system
    if (resSliderPtr) {
        parameterSliders["filter_resonance"] = resSliderPtr;
        resSliderPtr->setValueChangeCallback([&synthesizer, filterVizPtr, &updatingFromSlider, &updatingFromVisualizer](float resonanceValue) {
            if (updatingFromVisualizer) {
                std::cout << "RESONANCE SLIDER: Ignoring update from visualizer" << std::endl;
                return; // Prevent feedback loop
            }
            
            updatingFromSlider = true;
            
            std::cout << "RESONANCE SLIDER: Value changed to " << resonanceValue << " (normalized)" << std::endl;
            
            // Update synthesizer parameter (normalized 0-1)
            synthesizer->setParameter("filter_resonance", resonanceValue);
            
            // Map 0-1 to reasonable resonance range for visualizer
            float resonance = 0.7f + resonanceValue * 9.3f;
            
            // Update visualizer with actual resonance value
            if (filterVizPtr) {
                filterVizPtr->setResonance(resonance);
                std::cout << "RESONANCE SLIDER: Updated visualizer resonance to " << resonance << std::endl;
            }
            
            updatingFromSlider = false;
        });
        // Initialize with low resonance
        resSliderPtr->setValue(0.1f);
    }
    
    // Connect filter visualizer to update both effect processor and sliders
    if (filterVizPtr) {
        filterVizPtr->setParameterChangeCallback([&synthesizer, cutoffSliderPtr, resSliderPtr, &updatingFromSlider, &updatingFromVisualizer](float cutoff, float resonance) {
            if (updatingFromSlider) {
                std::cout << "FILTER VIZ: Ignoring callback - update came from slider" << std::endl;
                return; // Prevent feedback loop
            }
            
            updatingFromVisualizer = true;
            
            std::cout << "FILTER VIZ: Dragged to cutoff=" << cutoff << " Hz, resonance=" << resonance << std::endl;
            
            // Update sliders
            if (cutoffSliderPtr) {
                // Convert frequency back to normalized value
                // freq = 20 * (1000)^normalizedValue
                // normalizedValue = log(freq/20) / log(1000)
                float normalizedValue = std::log(cutoff / 20.0f) / std::log(1000.0f);
                normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
                cutoffSliderPtr->setValue(normalizedValue);
                
                // Update synthesizer parameter
                synthesizer->setParameter("filter_cutoff", normalizedValue);
                std::cout << "FILTER VIZ: Updated cutoff to " << normalizedValue << " (normalized)" << std::endl;
            }
            if (resSliderPtr) {
                // Convert resonance back to 0-1 range for slider
                float sliderValue = (resonance - 0.7f) / 9.3f;
                resSliderPtr->setValue(sliderValue);
                
                // Update synthesizer parameter
                synthesizer->setParameter("filter_resonance", sliderValue);
                std::cout << "FILTER VIZ: Updated resonance to " << sliderValue << " (normalized)" << std::endl;
            }
            
            updatingFromVisualizer = false;
        });
    }
    
    connectSliderToParam(volumeSliderPtr, "master_volume");
    
    // Add CC learning buttons for each parameter
    addParameterLearning(waveSliderPtr, "oscillator_type", 170, 85);
    addParameterLearning(cutoffSliderPtr, "filter_cutoff", 350, 85);
    addParameterLearning(resSliderPtr, "filter_resonance", 460, 85);
    addParameterLearning(volumeSliderPtr, "master_volume", 980, 85);
    
    // Connect envelope visualizer to update synthesizer and sliders
    if (envelopePtr) {
        envelopePtr->setParameterChangeCallback([&synthesizer, attackSliderPtr, decaySliderPtr, 
                                                 sustainSliderPtr, releaseSliderPtr]
                                                (float attack, float decay, float sustain, float release) {
            // Update synthesizer parameters
            synthesizer->setParameter("envelope_attack", attack);
            synthesizer->setParameter("envelope_decay", decay);
            synthesizer->setParameter("envelope_sustain", sustain);
            synthesizer->setParameter("envelope_release", release);
            
            // Update sliders to reflect the new values
            if (attackSliderPtr) attackSliderPtr->setValue(attack);
            if (decaySliderPtr) decaySliderPtr->setValue(decay);
            if (sustainSliderPtr) sustainSliderPtr->setValue(sustain);
            if (releaseSliderPtr) releaseSliderPtr->setValue(release);
            
            std::cout << "Envelope updated from visualizer - A:" << attack 
                      << " D:" << decay << " S:" << sustain << " R:" << release << std::endl;
        });
    }
    
    // Also update visualizer when sliders change
    // We need to modify the connectSliderToParam lambda to also update the visualizer
    auto connectSliderToEnvelope = [&](Slider* slider, const std::string& paramId, 
                                      EnvelopeVisualizer* envViz,
                                      Slider* attackSlider, Slider* decaySlider, 
                                      Slider* sustainSlider, Slider* releaseSlider) {
        if (slider) {
            // Store slider reference for CC learning updates
            parameterSliders[paramId] = slider;
            
            // Set up value change callback to update both synthesizer and visualizer
            slider->setValueChangeCallback([&synthesizer, paramId, envViz, 
                                          attackSlider, decaySlider, sustainSlider, releaseSlider](float value) {
                // Update synthesizer
                synthesizer->setParameter(paramId, value);
                std::cout << "Updated " << paramId << " to " << value << std::endl;
                
                // Update visualizer with all current values
                if (envViz && attackSlider && decaySlider && sustainSlider && releaseSlider) {
                    envViz->setADSR(attackSlider->getValue(), decaySlider->getValue(),
                                   sustainSlider->getValue(), releaseSlider->getValue());
                }
            });
            
            // Initialize slider with current parameter value
            float currentValue = synthesizer->getParameter(paramId);
            slider->setValue(currentValue);
        }
    };
    
    // Re-connect envelope sliders with visualizer updates
    connectSliderToEnvelope(attackSliderPtr, "envelope_attack", envelopePtr,
                           attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr);
    connectSliderToEnvelope(decaySliderPtr, "envelope_decay", envelopePtr,
                           attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr);
    connectSliderToEnvelope(sustainSliderPtr, "envelope_sustain", envelopePtr,
                           attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr);
    connectSliderToEnvelope(releaseSliderPtr, "envelope_release", envelopePtr,
                           attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr);
    
    // Initialize envelope parameters in synthesizer
    synthesizer->setParameter("envelope_attack", 0.02f);  // Changed from 0.01f to 0.02f for smoother attack
    synthesizer->setParameter("envelope_decay", 0.1f);
    synthesizer->setParameter("envelope_sustain", 0.7f);
    synthesizer->setParameter("envelope_release", 0.5f);
    
    // Connect LFO parameters with special handling for shared sliders
    // The sliders will update the currently selected LFO
    lfoRateSliderPtr->setValueChangeCallback([&synthesizer, &getCurrentLFOParamName, &currentLFO, &lfo1State, &lfo2State](float value) {
        std::string paramName = getCurrentLFOParamName("rate");
        synthesizer->setParameter(paramName, value);
        
        // Update the stored state
        if (currentLFO == 1) {
            lfo1State.rate = value;
        } else {
            lfo2State.rate = value;
        }
        
        std::cout << "Updated " << paramName << " to " << value << std::endl;
    });
    
    lfoDepthSliderPtr->setValueChangeCallback([&synthesizer, &getCurrentLFOParamName, &currentLFO, &lfo1State, &lfo2State](float value) {
        std::string paramName = getCurrentLFOParamName("depth");
        synthesizer->setParameter(paramName, value);
        
        // Update the stored state
        if (currentLFO == 1) {
            lfo1State.depth = value;
        } else {
            lfo2State.depth = value;
        }
        
        std::cout << "Updated " << paramName << " to " << value << std::endl;
    });
    
    lfoShapeSliderPtr->setValueChangeCallback([&synthesizer, &getCurrentLFOParamName, &currentLFO, &lfo1State, &lfo2State](float value) {
        std::string paramName = getCurrentLFOParamName("shape");
        synthesizer->setParameter(paramName, value);
        
        // Update the stored state
        if (currentLFO == 1) {
            lfo1State.shape = value;
        } else {
            lfo2State.shape = value;
        }
        
        std::cout << "Updated " << paramName << " to " << value << std::endl;
    });
    
    // Initialize both LFOs with their default values
    std::cout << "Initializing LFO 1 - Rate: " << lfo1State.rate << ", Depth: " << lfo1State.depth << ", Shape: " << lfo1State.shape << std::endl;
    synthesizer->setParameter("lfo1_rate", lfo1State.rate);
    synthesizer->setParameter("lfo1_depth", lfo1State.depth);
    synthesizer->setParameter("lfo1_shape", lfo1State.shape);
    
    std::cout << "Initializing LFO 2 - Rate: " << lfo2State.rate << ", Depth: " << lfo2State.depth << ", Shape: " << lfo2State.shape << std::endl;
    synthesizer->setParameter("lfo2_rate", lfo2State.rate);
    synthesizer->setParameter("lfo2_depth", lfo2State.depth);
    synthesizer->setParameter("lfo2_shape", lfo2State.shape);
    
    // Set up parameter mappings for CC learning (start with LFO 1)
    parameterSliders["lfo1_rate"] = lfoRateSliderPtr;
    parameterSliders["lfo1_depth"] = lfoDepthSliderPtr;
    parameterSliders["lfo1_shape"] = lfoShapeSliderPtr;
    
    std::cout << "Parameter connections and CC learning established" << std::endl;

    // Apply persisted synthesizer parameters if available
    if (loadedConfig.contains("synth") && loadedConfig["synth"].is_object()) {
        const auto& sc = loadedConfig["synth"];
        auto getf = [&](const char* key, float& out) {
            if (sc.contains(key)) {
                try { out = sc[key].get<float>(); } catch (...) {}
            }
        };
        float v;
        // Oscillator type (0-4)
        if (sc.contains("oscillator_type")) {
            v = 0.0f; getf("oscillator_type", v);
            if (waveSliderPtr) waveSliderPtr->setValue(v);
            synthesizer->setParameter("oscillator_type", v);
        }
        // Filter cutoff (normalized 0-1)
        if (sc.contains("filter_cutoff")) {
            v = 1.0f; getf("filter_cutoff", v);
            // Avoid double-callback recursion during initial apply
            if (cutoffSliderPtr) {
                updatingFromVisualizer = true; // reuse guard to suppress slider callback path temporarily
                cutoffSliderPtr->setValue(v);
                updatingFromVisualizer = false;
            }
            // Ensure synthesizer and visualizer reflect the value even if callback is suppressed
            synthesizer->setParameter("filter_cutoff", v);
            if (filterVizPtr) {
                float frequencyHz = 20.0f * std::pow(1000.0f, std::max(0.0f, std::min(1.0f, v)));
                filterVizPtr->setCutoffFrequency(frequencyHz);
            }
            std::cout << "Loaded filter_cutoff (norm): " << v << std::endl;
        }
        // Filter resonance (normalized 0-1)
        if (sc.contains("filter_resonance")) {
            v = 0.1f; getf("filter_resonance", v);
            if (resSliderPtr) {
                updatingFromVisualizer = true;
                resSliderPtr->setValue(v);
                updatingFromVisualizer = false;
            }
            synthesizer->setParameter("filter_resonance", v);
            if (filterVizPtr) {
                float resonanceQ = 0.7f + std::max(0.0f, std::min(1.0f, v)) * 9.3f;
                filterVizPtr->setResonance(resonanceQ);
            }
            std::cout << "Loaded filter_resonance (norm): " << v << std::endl;
        }
        // Master volume (0-1)
        if (sc.contains("master_volume")) {
            v = 0.7f; getf("master_volume", v);
            if (volumeSliderPtr) volumeSliderPtr->setValue(v);
        }
        // Envelope ADSR
        float a = 0.02f, d = 0.1f, s = 0.7f, r = 0.5f;
        getf("envelope_attack", a);
        getf("envelope_decay", d);
        getf("envelope_sustain", s);
        getf("envelope_release", r);
        if (attackSliderPtr) attackSliderPtr->setValue(a);
        if (decaySliderPtr) decaySliderPtr->setValue(d);
        if (sustainSliderPtr) sustainSliderPtr->setValue(s);
        if (releaseSliderPtr) releaseSliderPtr->setValue(r);
        if (envelopePtr) envelopePtr->setADSR(a, d, s, r);
        synthesizer->setParameter("envelope_attack", a);
        synthesizer->setParameter("envelope_decay", d);
        synthesizer->setParameter("envelope_sustain", s);
        synthesizer->setParameter("envelope_release", r);
        // LFOs
        float l1r = lfo1State.rate, l1d = lfo1State.depth, l1s = lfo1State.shape;
        float l2r = lfo2State.rate, l2d = lfo2State.depth, l2s = lfo2State.shape;
        getf("lfo1_rate", l1r);
        getf("lfo1_depth", l1d);
        getf("lfo1_shape", l1s);
        getf("lfo2_rate", l2r);
        getf("lfo2_depth", l2d);
        getf("lfo2_shape", l2s);
        // Apply to synthesizer and internal state
        lfo1State.rate = l1r; lfo1State.depth = l1d; lfo1State.shape = l1s;
        lfo2State.rate = l2r; lfo2State.depth = l2d; lfo2State.shape = l2s;
        synthesizer->setParameter("lfo1_rate", l1r);
        synthesizer->setParameter("lfo1_depth", l1d);
        synthesizer->setParameter("lfo1_shape", l1s);
        synthesizer->setParameter("lfo2_rate", l2r);
        synthesizer->setParameter("lfo2_depth", l2d);
        synthesizer->setParameter("lfo2_shape", l2s);
        // Update state + engine, then update sliders by toggling LFO selector silently
        if (lfoRateSliderPtr && lfoDepthSliderPtr && lfoShapeSliderPtr) {
            if (lfoSelectorDropdownPtr) {
                int prev = std::max(0, lfoSelectorDropdownPtr->getSelectedIndex());
                // Apply LFO1
                lfoSelectorDropdownPtr->selectItemSilently(0);
                lfo1State.rate = l1r; lfo1State.depth = l1d; lfo1State.shape = l1s;
                synthesizer->setParameter("lfo1_rate", l1r);
                synthesizer->setParameter("lfo1_depth", l1d);
                synthesizer->setParameter("lfo1_shape", l1s);
                std::cout << "[PresetLoad] Applied LFO1: rate=" << l1r << ", depth=" << l1d << ", shape=" << l1s << std::endl;
                lfoRateSliderPtr->setValueSilently(l1r);
                lfoDepthSliderPtr->setValueSilently(l1d);
                lfoShapeSliderPtr->setValueSilently(l1s);
                // Apply LFO2
                lfoSelectorDropdownPtr->selectItemSilently(1);
                lfo2State.rate = l2r; lfo2State.depth = l2d; lfo2State.shape = l2s;
                synthesizer->setParameter("lfo2_rate", l2r);
                synthesizer->setParameter("lfo2_depth", l2d);
                synthesizer->setParameter("lfo2_shape", l2s);
                std::cout << "[PresetLoad] Applied LFO2: rate=" << l2r << ", depth=" << l2d << ", shape=" << l2s << std::endl;
                lfoRateSliderPtr->setValueSilently(l2r);
                lfoDepthSliderPtr->setValueSilently(l2d);
                lfoShapeSliderPtr->setValueSilently(l2s);
                lfoSelectorDropdownPtr->selectItemSilently(prev); // Restore prior selection
                // Finally, refresh the visible set (current selection)
                if (prev == 0) {
                    lfoRateSliderPtr->setValue(l1r);
                    lfoDepthSliderPtr->setValue(l1d);
                    lfoShapeSliderPtr->setValue(l1s);
                } else {
                    lfoRateSliderPtr->setValue(l2r);
                    lfoDepthSliderPtr->setValue(l2d);
                    lfoShapeSliderPtr->setValue(l2s);
                }
            } else {
                // No selector found; just show LFO1 values
                lfoRateSliderPtr->setValue(l1r);
                lfoDepthSliderPtr->setValue(l1d);
                lfoShapeSliderPtr->setValue(l1s);
            }
        }
    }

    // Create preset section at bottom right
    auto presetSection = std::make_unique<Label>("preset_section", "PRESETS");
    presetSection->setPosition(850, 720);  // Near bottom of 800px window
    presetSection->setTextColor(Color(150, 255, 150)); // Bright light green
    mainScreen->addChild(std::move(presetSection));
    // Debug label for preset load resolution (temporary)
    auto presetDebug = std::make_unique<Label>("preset_debug", "");
    presetDebug->setPosition(850, 780);
    presetDebug->setSize(400, 16);
    presetDebug->setTextColor(Color(160, 160, 160));
    mainScreen->addChild(std::move(presetDebug));
    
    // Create preset dropdown menu
    auto presetDropdown = std::make_unique<PresetDropdown>("preset_dropdown");
    presetDropdown->setPosition(850, 750);  // Bottom right
    presetDropdown->setSize(250, 30);
    
    // Add all presets to dropdown
    auto allPresets = presetDatabase->getAllPresets();
    for (const auto& preset : allPresets) {
        presetDropdown->addPreset(preset.name, preset.category, preset.filePath);
    }
    // Add a simple Hybrid morph demo preset entry (virtual)
    presetDropdown->addPreset("Hybrid Morph Demo (LFO1+ModWheel)", "Demo", "__virtual__/hybrid_morph_demo");
    
    std::cout << "Added " << allPresets.size() << " presets to dropdown" << std::endl;
    
    // Store pointer to dropdown for button callback
    PresetDropdown* presetDropdownPtr = presetDropdown.get();
    
    // Create Load button
    auto loadPresetButton = std::make_unique<Button>("load_preset", "Load");
    loadPresetButton->setPosition(1110, 750);
    loadPresetButton->setSize(60, 30);
    loadPresetButton->setBackgroundColor(Color(60, 100, 60));
    loadPresetButton->setTextColor(Color(255, 255, 255));
    
    loadPresetButton->setClickCallback([&, presetDropdownPtr, waveSliderPtr, cutoffSliderPtr, resSliderPtr, volumeSliderPtr]() {
        auto selectedPreset = presetDropdownPtr->getSelectedPreset();
        if (!selectedPreset.fullPath.empty()) {
            std::cout << "Loading preset: " << selectedPreset.name << " from " << selectedPreset.fullPath << std::endl;
            if (selectedPreset.fullPath == "__virtual__/hybrid_morph_demo") {
                // Programmatically set up a simple demo: Saw, moderate env, LFO1->Wavetable Position, ModWheel->Wavetable Position
                std::lock_guard<std::mutex> lock(audioMutex);
                synthesizer->setParameter("oscillator_type", 1.0f); // Saw
                synthesizer->setParameter("envelope_attack", 0.01f);
                synthesizer->setParameter("envelope_decay", 0.25f);
                synthesizer->setParameter("envelope_sustain", 0.6f);
                synthesizer->setParameter("envelope_release", 0.3f);
                // Disconnect any existing routes, then connect
                if (auto* mm = synthesizer->getModulationMatrix()) {
                    mm->disconnect("LFO1", "Wavetable Position");
                    mm->disconnect("ModWheel", "Wavetable Position");
                    mm->connect("LFO1", "Wavetable Position", 0.35f);
                    mm->connect("ModWheel", "Wavetable Position", 0.5f);
                }
                std::cout << "Loaded virtual demo preset: Hybrid Morph Demo" << std::endl;
                return;
            }
            // Load the preset file and apply parameters to synthesizer
            if (presetManager->loadPreset(selectedPreset.fullPath)) {
                std::cout << "Successfully loaded preset: " << selectedPreset.name << std::endl;
                
                // Update UI controls to reflect loaded preset values
                if (waveSliderPtr) {
                    float oscType = synthesizer->getParameter("oscillator_type");
                    waveSliderPtr->setValue(oscType); // Already normalized 0-4
                }
                
                if (cutoffSliderPtr) {
                    float cutoffNorm = synthesizer->getParameter("filter_cutoff");
                    // Slider expects normalized value [0,1]
                    cutoffSliderPtr->setValue(cutoffNorm);
                }
                
                if (resSliderPtr) {
                    float resonance = synthesizer->getParameter("filter_resonance");
                    resSliderPtr->setValue(resonance); // Already normalized
                }
                
                if (volumeSliderPtr) {
                    float volume = synthesizer->getParameter("master_volume");
                    volumeSliderPtr->setValue(volume); // Already normalized
                }
                
                std::cout << "Preset loaded and UI updated: " << selectedPreset.name << std::endl;
            } else {
                std::cerr << "Failed to load preset: " << selectedPreset.name << std::endl;
            }
        }
    });
    
    // Create Save button (placeholder for now)
    auto savePresetButton = std::make_unique<Button>("save_preset", "Save");
    savePresetButton->setPosition(1180, 750);
    savePresetButton->setSize(60, 30);
    savePresetButton->setBackgroundColor(Color(60, 60, 100));
    savePresetButton->setTextColor(Color(255, 255, 255));
    // Defer binding the real save callback until after effects UI state is initialized
    Button* savePresetButtonPtr = savePresetButton.get();
    
    mainScreen->addChild(std::move(presetDropdown));
    mainScreen->addChild(std::move(loadPresetButton));
    mainScreen->addChild(std::move(savePresetButton));

    // Add all dropdowns last for proper z-order (they render on top)
    // Add LFO selector dropdown
    if (lfoSelectorDropdown) {
        mainScreen->addChild(std::move(lfoSelectorDropdown));
    }
    
    // Add MIDI device dropdown
    if (midiDeviceDropdownPtr) {
        mainScreen->addChild(std::move(midiDeviceDropdownPtr));
    }
    
    // Add modulation dropdowns
    for (auto& dropdown : modSourceDropdowns) {
        mainScreen->addChild(std::move(dropdown));
    }
    for (auto& dropdown : modDestDropdowns) {
        mainScreen->addChild(std::move(dropdown));
    }

    // Apply persisted modulation routing if available
    if (loadedConfig.contains("mod_routing") && loadedConfig["mod_routing"].is_array()) {
        if (auto* ms = uiContext->getScreen("main")) {
            int idxRow = 0;
            for (const auto& mr : loadedConfig["mod_routing"]) {
                if (idxRow >= modRowCount) break;
                // Prefer name-based mapping; fall back to indices for backward compatibility
                int srcIndex = 0;
                int dstIndex = 0;
                if (mr.contains("source") && mr["source"].is_string()) {
                    std::string sName = mr["source"].get<std::string>();
                    // Map to UI label
                    if (auto* srcDd = dynamic_cast<DropdownMenu*>(ms->getChild("mod_source_" + std::to_string(idxRow)))) {
                        // Try exact match first
                        srcDd->selectItemSilently(sName);
                        srcIndex = srcDd->getSelectedIndex();
                        // Fallback mapping for internal names to UI labels
                        if (srcIndex < 0) {
                            if (sName == "LFO1") sName = "LFO 1";
                            else if (sName == "LFO2") sName = "LFO 2";
                            else if (sName == "ModWheel") sName = "Mod Wheel";
                            else if (sName == "Aftertouch") sName = "Aftertouch";
                            else if (sName == "Velocity") sName = "Velocity";
                            else if (sName == "Envelope") sName = "Envelope";
                            srcDd->selectItemSilently(sName);
                            srcIndex = std::max(0, srcDd->getSelectedIndex());
                        }
                    }
                } else {
                    srcIndex = (mr.contains("sourceIndex") ? mr["sourceIndex"].get<int>() : 0);
                }
                if (mr.contains("destination") && mr["destination"].is_string()) {
                    std::string dName = mr["destination"].get<std::string>();
                    if (auto* dstDd = dynamic_cast<DropdownMenu*>(ms->getChild("mod_dest_" + std::to_string(idxRow)))) {
                        dstDd->selectItemSilently(dName);
                        dstIndex = std::max(0, dstDd->getSelectedIndex());
                    }
                } else {
                    dstIndex = (mr.contains("destIndex") ? mr["destIndex"].get<int>() : 0);
                }
                float amount = (mr.contains("amount") ? mr["amount"].get<float>() : 0.0f);
                if (auto* src = dynamic_cast<DropdownMenu*>(ms->getChild("mod_source_" + std::to_string(idxRow)))) {
                    src->selectItem(std::max(0, srcIndex));
                }
                if (auto* dst = dynamic_cast<DropdownMenu*>(ms->getChild("mod_dest_" + std::to_string(idxRow)))) {
                    dst->selectItem(std::max(0, dstIndex));
                }
                if (auto* amt = dynamic_cast<Slider*>(ms->getChild("mod_amount_" + std::to_string(idxRow)))) {
                    amt->setValue(amount);
                }
                // Track for save
                modConnections[idxRow].sourceIndex = srcIndex;
                modConnections[idxRow].destIndex = dstIndex;
                modConnections[idxRow].amount = amount;
                ++idxRow;
            }
        }
    }
    
    // Add main-screen quick FX dropdowns last for z-order
    for (auto& dropdown : mainEffectDropdowns) {
        mainScreen->addChild(std::move(dropdown));
    }

    // Finally add Reset button to the right of MIDI indicator, ensure top-most among buttons
    {
        auto resetButton = std::make_unique<Button>("reset_params_btn", "Reset parameters");
        // Position relative to MIDI indicator: (100,755) -> place at (130, 748)
        resetButton->setPosition(130, 748);
        resetButton->setSize(170, 30);
        resetButton->setBackgroundColor(Color(220, 40, 40)); // bright red for visibility
        resetButton->setTextColor(Color(255, 255, 255)); // force white label
        // One-time log to confirm creation and placement
        std::cout << "[UI] Adding reset button at (" << 130 << "," << 748
                  << ") size (" << 100 << "x" << 30 << ")" << std::endl;
        resetButton->setClickCallback([&, waveSliderPtr, cutoffSliderPtr, resSliderPtr,
                                       attackSliderPtr, decaySliderPtr, sustainSliderPtr, releaseSliderPtr,
                                       volumeSliderPtr, filterVizPtr, lfoRateSliderPtr, lfoDepthSliderPtr, lfoShapeSliderPtr,
                                       mainScreen = mainScreen.get(), uiCtx = uiContext.get()]() {
            if (waveSliderPtr) waveSliderPtr->setValue(0.0f);
            synthesizer->setParameter("oscillator_type", 0.0f);
            if (cutoffSliderPtr) cutoffSliderPtr->setValue(1.0f);
            if (resSliderPtr)    resSliderPtr->setValue(0.1f);
            if (filterVizPtr) {
                filterVizPtr->setCutoffFrequency(20000.0f);
                filterVizPtr->setResonance(0.7f + 0.1f * 9.3f);
            }
            if (attackSliderPtr)  attackSliderPtr->setValue(0.02f);
            if (decaySliderPtr)   decaySliderPtr->setValue(0.1f);
            if (sustainSliderPtr) sustainSliderPtr->setValue(0.7f);
            if (releaseSliderPtr) releaseSliderPtr->setValue(0.5f);
            synthesizer->setParameter("envelope_attack", 0.02f);
            synthesizer->setParameter("envelope_decay", 0.1f);
            synthesizer->setParameter("envelope_sustain", 0.7f);
            synthesizer->setParameter("envelope_release", 0.5f);
            if (volumeSliderPtr)  volumeSliderPtr->setValue(0.75f);
            synthesizer->setParameter("master_volume", 0.75f);
            int lfoSelIndex = 0;
            if (auto* dd = dynamic_cast<DropdownMenu*>(mainScreen->getChild("lfo_selector"))) {
                lfoSelIndex = dd->getSelectedIndex();
            }
            auto applyLFO = [&](int idx, float rate, float depth, float shape){
                if (auto* dd = dynamic_cast<DropdownMenu*>(mainScreen->getChild("lfo_selector"))) {
                    dd->selectItem(idx);
                }
                if (lfoRateSliderPtr)  lfoRateSliderPtr->setValue(rate);
                if (lfoDepthSliderPtr) lfoDepthSliderPtr->setValue(depth);
                if (lfoShapeSliderPtr) lfoShapeSliderPtr->setValue(shape);
                synthesizer->setParameter(idx == 0 ? "lfo1_rate"  : "lfo2_rate",  rate);
                synthesizer->setParameter(idx == 0 ? "lfo1_depth" : "lfo2_depth", depth);
                synthesizer->setParameter(idx == 0 ? "lfo1_shape" : "lfo2_shape", shape);
            };
            applyLFO(0, 1.0f, 1.0f, 0.0f);
            applyLFO(1, 2.0f, 0.5f, 2.0f);
            if (auto* dd = dynamic_cast<DropdownMenu*>(mainScreen->getChild("lfo_selector"))) {
                dd->selectItem(std::max(0, std::min(1, lfoSelIndex)));
            }
            // Reset Effects: set all slots to None, mix to defaults, enabled ON, clear param caches, rebuild chain
            {
                // Switch to Effects screen contextually to access its UI components
                Screen* effectsScreen = uiCtx->getScreen("effects");
                if (effectsScreen) {
                    // Reset effect type dropdowns to "None" (index 0) and mix sliders to 50%
                    for (int s = 0; s < 6; ++s) {
                        if (auto* dd = dynamic_cast<DropdownMenu*>(effectsScreen->getChild("fx_type_" + std::to_string(s)))) {
                            dd->selectItem(0);
                        }
                        if (auto* mix = dynamic_cast<Slider*>(effectsScreen->getChild("fx_mix_" + std::to_string(s)))) {
                            mix->setValue(0.5f);
                        }
                    }
                    // Also reset quick FX dropdowns on main screen (first 3 mirror slots) and zero their param sliders
                    if (auto* ms = uiCtx->getScreen("main")) {
                        for (int i = 0; i < 3; ++i) {
                            if (auto* qdd = dynamic_cast<DropdownMenu*>(ms->getChild("effect_type_" + std::to_string(i)))) {
                                qdd->selectItem(0);
                            }
                        }
                    }

                    // Zero vertical parameter sliders for all slots (even when None), as requested
                    for (int s = 0; s < 6; ++s) {
                        if (auto* v1 = dynamic_cast<Slider*>(effectsScreen->getChild("fx_v1_" + std::to_string(s)))) v1->setValue(0.0f);
                        if (auto* v2 = dynamic_cast<Slider*>(effectsScreen->getChild("fx_v2_" + std::to_string(s)))) v2->setValue(0.0f);
                        if (auto* v3 = dynamic_cast<Slider*>(effectsScreen->getChild("fx_v3_" + std::to_string(s)))) v3->setValue(0.0f);
                        if (auto* v4 = dynamic_cast<Slider*>(effectsScreen->getChild("fx_v4_" + std::to_string(s)))) v4->setValue(0.0f);
                    }
                }
            }
            // Reset Modulation routing: set all rows to None/None and amount 0, disconnect in synth
            {
                for (int i = 0; i < 3; ++i) {
                    // Disconnect existing connections by enumerating current destinations
                    std::vector<std::string> sources = {"LFO1", "LFO2", "ModWheel", "Aftertouch", "Velocity", "Envelope"};
                    auto allDests = synthesizer->getModDestinationNames();
                    for (const auto& sName : sources) {
                        for (const auto& dName : allDests) {
                            synthesizer->disconnectModulation(sName, dName);
                        }
                    }
                    if (auto* ms = uiCtx->getScreen("main")) {
                        if (auto* src = dynamic_cast<DropdownMenu*>(ms->getChild("mod_source_" + std::to_string(i)))) {
                            src->selectItemSilently(0);
                        }
                        if (auto* dst = dynamic_cast<DropdownMenu*>(ms->getChild("mod_dest_" + std::to_string(i)))) {
                            dst->selectItemSilently(0);
                        }
                        if (auto* amt = dynamic_cast<Slider*>(ms->getChild("mod_amount_" + std::to_string(i)))) {
                            amt->setValue(0.0f);
                        }
                    }
                }
            }
            std::cout << "Reset: Synth parameters and effects restored to defaults" << std::endl;
        });
        mainScreen->addChild(std::move(resetButton));
        if (auto* rb = mainScreen->getChild("reset_params_btn")) {
            std::cout << "[UI] Reset button added: visible="
                      << (rb->isVisible() ? "true" : "false")
                      << ", enabled=" << (rb->isEnabled() ? "true" : "false") << std::endl;
        } else {
            std::cout << "[UI] Reset button NOT found after addChild" << std::endl;
        }
    }


    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    std::cout << "Added main screen to UI context" << std::endl;
    
    // Create sequencer screen
    auto sequencerScreen = std::make_unique<Screen>("sequencer");
    sequencerScreen->setBackgroundColor(Color(30, 30, 40));
    sequencerScreen->setPosition(0, 0);
    sequencerScreen->setSize(1280, 800);
    
    // Create a function to add navigation buttons to any screen
    auto addNavigationButtons = [](Screen* screen, const std::string& currentScreen, UIContext* uiContext) {
        // Main button
        auto mainButton = std::make_unique<Button>("nav_main", "Main");
        mainButton->setPosition(50, 5);
        mainButton->setSize(80, 30);
        mainButton->setBackgroundColor(currentScreen == "main" ? Color(100, 100, 140) : Color(80, 80, 120));
        mainButton->setTextColor(Color(255, 255, 255));
        mainButton->setClickCallback([uiContext]() { uiContext->setActiveScreen("main"); });
        screen->addChild(std::move(mainButton));
        
        // Remove duplicate label; button caption suffices
        
        // Effects button
        auto effectsButton = std::make_unique<Button>("nav_effects", "Effects");
        effectsButton->setPosition(140, 5);
        effectsButton->setSize(80, 30);
        effectsButton->setBackgroundColor(currentScreen == "effects" ? Color(100, 100, 140) : Color(80, 80, 120));
        effectsButton->setTextColor(Color(255, 255, 255));
        effectsButton->setClickCallback([uiContext]() { uiContext->setActiveScreen("effects"); });
        screen->addChild(std::move(effectsButton));
        
        // Remove duplicate label; button caption suffices
        
        // Modulation button
        auto modButton = std::make_unique<Button>("nav_mod", "Modulation");
        modButton->setPosition(230, 5);
        modButton->setSize(100, 30);
        modButton->setBackgroundColor(currentScreen == "modulation" ? Color(100, 100, 140) : Color(80, 80, 120));
        modButton->setTextColor(Color(255, 255, 255));
        modButton->setClickCallback([uiContext]() { uiContext->setActiveScreen("modulation"); });
        screen->addChild(std::move(modButton));
        
        // Remove duplicate label; button caption suffices
        
        // Presets button
        auto presetsButton = std::make_unique<Button>("nav_presets", "Presets");
        presetsButton->setPosition(340, 5);
        presetsButton->setSize(80, 30);
        presetsButton->setBackgroundColor(currentScreen == "presets" ? Color(100, 100, 140) : Color(80, 80, 120));
        presetsButton->setTextColor(Color(255, 255, 255));
        presetsButton->setClickCallback([uiContext]() { uiContext->setActiveScreen("presets"); });
        screen->addChild(std::move(presetsButton));
        
        // Remove duplicate label; button caption suffices
        
        // Settings button
        auto settingsButton = std::make_unique<Button>("nav_settings", "Settings");
        settingsButton->setPosition(430, 5);
        settingsButton->setSize(80, 30);
        settingsButton->setBackgroundColor(currentScreen == "settings" ? Color(100, 100, 140) : Color(80, 80, 120));
        settingsButton->setTextColor(Color(255, 255, 255));
        settingsButton->setClickCallback([uiContext]() { uiContext->setActiveScreen("settings"); });
        screen->addChild(std::move(settingsButton));
        
        // Remove duplicate label; button caption suffices
        
        // Sequencer button
        auto seqButton = std::make_unique<Button>("nav_seq", "Sequencer");
        seqButton->setPosition(520, 5);
        seqButton->setSize(100, 30);
        seqButton->setBackgroundColor(currentScreen == "sequencer" ? Color(100, 100, 140) : Color(80, 80, 120));
        seqButton->setTextColor(Color(255, 255, 255));
        seqButton->setClickCallback([uiContext]() { uiContext->setActiveScreen("sequencer"); });
        screen->addChild(std::move(seqButton));
        
        // Remove duplicate label; button caption suffices
    };
    
    // Add navigation to sequencer screen
    addNavigationButtons(sequencerScreen.get(), "sequencer", uiContext.get());
    
    // Transport section
    auto transportSection = std::make_unique<Label>("transport_section", "TRANSPORT");
    transportSection->setPosition(50, 70);
    transportSection->setSize(200, 25);
    transportSection->setTextColor(Color(255, 255, 100));
    sequencerScreen->addChild(std::move(transportSection));
    
    // Play/Stop button
    auto playStopButton = std::make_unique<Button>("play_stop_btn", "Play");
    playStopButton->setPosition(50, 110);
    playStopButton->setSize(80, 40);
    playStopButton->setBackgroundColor(Color(50, 120, 50));
    playStopButton->setTextColor(Color(255, 255, 255));
    
    // Track playing state
    bool* isPlaying = new bool(false);
    
    auto playStopButtonPtr = playStopButton.get();
    
    // Create label for play/stop button first
    auto playStopLabel = std::make_unique<Label>("play_stop_label", "Play");
    playStopLabel->setPosition(65, 120);
    playStopLabel->setSize(50, 20);
    playStopLabel->setTextColor(Color(255, 255, 255));
    auto playStopLabelPtr = playStopLabel.get();
    
    // Set callback with both button and label pointers
    playStopButton->setClickCallback([&sequencer, playStopButtonPtr, playStopLabelPtr, isPlaying]() {
        if (*isPlaying) {
            sequencer->stop();
            playStopButtonPtr->setText("Play");
            playStopLabelPtr->setText("Play");
            playStopButtonPtr->setBackgroundColor(Color(50, 120, 50));
            *isPlaying = false;
            std::cout << "Sequencer stopped" << std::endl;
        } else {
            sequencer->start();
            playStopButtonPtr->setText("Stop");
            playStopLabelPtr->setText("Stop");
            playStopButtonPtr->setBackgroundColor(Color(120, 50, 50));
            *isPlaying = true;
            std::cout << "Sequencer started" << std::endl;
        }
    });
    sequencerScreen->addChild(std::move(playStopButton));
    sequencerScreen->addChild(std::move(playStopLabel));
    
    // Reset button
    auto resetButton = std::make_unique<Button>("reset_btn", "Reset");
    resetButton->setPosition(140, 110);
    resetButton->setSize(80, 40);
    resetButton->setBackgroundColor(Color(80, 80, 120));
    resetButton->setTextColor(Color(255, 255, 255));
    resetButton->setClickCallback([&sequencer]() {
        sequencer->reset();
        std::cout << "Sequencer reset" << std::endl;
    });
    sequencerScreen->addChild(std::move(resetButton));
    
    // Add label for reset button
    auto resetLabel = std::make_unique<Label>("reset_label", "Reset");
    resetLabel->setPosition(155, 120);
    resetLabel->setSize(50, 20);
    resetLabel->setTextColor(Color(255, 255, 255));
    sequencerScreen->addChild(std::move(resetLabel));
    
    // Tempo control
    auto tempoLabel = std::make_unique<Label>("tempo_label", "BPM");
    tempoLabel->setPosition(250, 70);
    tempoLabel->setSize(100, 25);
    tempoLabel->setTextColor(Color(200, 200, 200));
    sequencerScreen->addChild(std::move(tempoLabel));
    
    auto tempoSlider = std::make_unique<Slider>("tempo_slider", "Tempo", 250, 110, 40, 100);
    tempoSlider->setRange(60.0f, 200.0f);
    tempoSlider->setValue(120.0f);
    tempoSlider->setValueChangeCallback([&sequencer](float value) {
        sequencer->setTempo(value);
        std::cout << "Tempo set to: " << value << " BPM" << std::endl;
    });
    sequencerScreen->addChild(std::move(tempoSlider));
    
    // Loop toggle
    auto loopButton = std::make_unique<Button>("loop_btn", "Loop: ON");
    loopButton->setPosition(360, 110);
    loopButton->setSize(80, 40);
    loopButton->setBackgroundColor(Color(50, 100, 50));
    loopButton->setTextColor(Color(255, 255, 255));
    
    bool* isLooping = new bool(true);
    auto loopButtonPtr = loopButton.get();
    
    // Create label for loop button
    auto loopLabel = std::make_unique<Label>("loop_label", "Loop: ON");
    loopLabel->setPosition(368, 120);
    loopLabel->setSize(65, 20);
    loopLabel->setTextColor(Color(255, 255, 255));
    auto loopLabelPtr = loopLabel.get();
    
    loopButton->setClickCallback([&sequencer, loopButtonPtr, loopLabelPtr, isLooping]() {
        *isLooping = !*isLooping;
        sequencer->setLooping(*isLooping);
        if (*isLooping) {
            loopButtonPtr->setText("Loop: ON");
            loopLabelPtr->setText("Loop: ON");
            loopButtonPtr->setBackgroundColor(Color(50, 100, 50));
        } else {
            loopButtonPtr->setText("Loop: OFF");
            loopLabelPtr->setText("Loop: OFF");
            loopButtonPtr->setBackgroundColor(Color(100, 50, 50));
        }
        std::cout << "Looping: " << (*isLooping ? "ON" : "OFF") << std::endl;
    });
    sequencerScreen->addChild(std::move(loopButton));
    sequencerScreen->addChild(std::move(loopLabel));
    
    // Pattern info section
    auto patternSection = std::make_unique<Label>("pattern_section", "PATTERN");
    patternSection->setPosition(50, 200);
    patternSection->setSize(200, 25);
    patternSection->setTextColor(Color(255, 255, 100));
    sequencerScreen->addChild(std::move(patternSection));
    
    // Current position display
    auto positionLabel = std::make_unique<Label>("position_label", "Position: 1.1.1");
    positionLabel->setPosition(50, 240);
    positionLabel->setSize(200, 25);
    positionLabel->setTextColor(Color(200, 200, 200));
    sequencerScreen->addChild(std::move(positionLabel));
    
    // Pattern status
    auto patternStatusLabel = std::make_unique<Label>("pattern_status", "No patterns loaded");
    patternStatusLabel->setPosition(50, 270);
    patternStatusLabel->setSize(300, 25);
    patternStatusLabel->setTextColor(Color(255, 100, 100));
    sequencerScreen->addChild(std::move(patternStatusLabel));
    
    // Info about sequencer being disabled
    auto infoLabel = std::make_unique<Label>("info_label", "Note: Sequencer audio is temporarily disabled (debugging duplicate notes)");
    infoLabel->setPosition(50, 350);
    infoLabel->setSize(600, 25);
    infoLabel->setTextColor(Color(255, 200, 100));
    sequencerScreen->addChild(std::move(infoLabel));
    
    // Add test pattern button
    auto addTestPatternButton = std::make_unique<Button>("add_pattern_btn", "Add Test Pattern");
    addTestPatternButton->setPosition(50, 400);
    addTestPatternButton->setSize(150, 40);
    addTestPatternButton->setBackgroundColor(Color(80, 80, 120));
    addTestPatternButton->setTextColor(Color(255, 255, 255));
    auto sequencerScreenPtr = sequencerScreen.get();
    addTestPatternButton->setClickCallback([&sequencer, sequencerScreenPtr]() {
        // Create a simple test pattern
        auto pattern = std::make_unique<Pattern>("Test Pattern");
        pattern->setLength(4.0); // 4 beats
        
        // Add some notes (C major scale)
        pattern->addNote(Note(60, 0.8f, 0.0, 0.5));   // C
        pattern->addNote(Note(62, 0.8f, 0.5, 0.5));   // D
        pattern->addNote(Note(64, 0.8f, 1.0, 0.5));   // E
        pattern->addNote(Note(65, 0.8f, 1.5, 0.5));   // F
        pattern->addNote(Note(67, 0.8f, 2.0, 0.5));   // G
        pattern->addNote(Note(69, 0.8f, 2.5, 0.5));   // A
        pattern->addNote(Note(71, 0.8f, 3.0, 0.5));   // B
        pattern->addNote(Note(72, 0.8f, 3.5, 0.5));   // C
        
        sequencer->addPattern(std::move(pattern));
        sequencer->setCurrentPattern(0);
        
        // Find the pattern status label by ID
        if (auto* statusLabel = dynamic_cast<Label*>(sequencerScreenPtr->getChild("pattern_status"))) {
            statusLabel->setText("Test pattern loaded (C major scale)");
            statusLabel->setTextColor(Color(100, 255, 100));
        }
        std::cout << "Added test pattern to sequencer" << std::endl;
    });
    sequencerScreen->addChild(std::move(addTestPatternButton));
    
    // Add label for test pattern button
    auto testPatternLabel = std::make_unique<Label>("test_pattern_label", "Add Test Pattern");
    testPatternLabel->setPosition(65, 410);
    testPatternLabel->setSize(120, 20);
    testPatternLabel->setTextColor(Color(255, 255, 255));
    sequencerScreen->addChild(std::move(testPatternLabel));
    
    // Add sequencer screen to context
    uiContext->addScreen(std::move(sequencerScreen));
    std::cout << "Added sequencer screen to UI context" << std::endl;
    
    // Create Effects screen
    auto effectsScreen = std::make_unique<Screen>("effects");
    effectsScreen->setBackgroundColor(Color(35, 30, 40));
    effectsScreen->setPosition(0, 0);
    effectsScreen->setSize(1280, 800);
    addNavigationButtons(effectsScreen.get(), "effects", uiContext.get());

    auto effectsTitle = std::make_unique<Label>("effects_title", "EFFECTS RACK");
    effectsTitle->setPosition(50, 50);
    effectsTitle->setSize(300, 30);
    effectsTitle->setTextColor(Color(255, 255, 100));
    effectsScreen->addChild(std::move(effectsTitle));

    // Multi-slot Effects UI (per-slot Type, Bypass, Mix + 4 vertical sliders)
    const int fxSlotCount = 6;
    const int rowStartY = 150;   // shifted up by 10px for tighter placement
    const int rowHeight = 100;   // adjusted to fit within 800px height

    // Per-slot state
    std::vector<std::string> slotSelectedType(fxSlotCount, "None");
    std::vector<float> slotMix(fxSlotCount, 0.5f);
    std::vector<bool> slotEnabled(fxSlotCount, true);
    std::vector<int> slotPage(fxSlotCount, 0); // Effects Page per slot (0=Page 1, 1=Page 2)

    // Per-slot, per-type caches (persistence across type switches)
    // slotParamCache[slot][type][paramName] = value
    std::vector<std::unordered_map<std::string, std::unordered_map<std::string, float>>> slotParamCache(fxSlotCount);
    // Mix and enabled caches per slot/type
    std::vector<std::unordered_map<std::string, float>> slotMixCache(fxSlotCount);
    std::vector<std::unordered_map<std::string, bool>>  slotEnabledCache(fxSlotCount);

    // Per-slot UI pointers
    std::vector<DropdownMenu*> slotTypeDd(fxSlotCount, nullptr);
    std::vector<Button*>       slotBypassBtn(fxSlotCount, nullptr);
    std::vector<Slider*>       slotMixSlider(fxSlotCount, nullptr);
    std::vector<Slider*>       slotV1Slider(fxSlotCount, nullptr);
    std::vector<Slider*>       slotV2Slider(fxSlotCount, nullptr);
    std::vector<Slider*>       slotV3Slider(fxSlotCount, nullptr);
    std::vector<Slider*>       slotV4Slider(fxSlotCount, nullptr);
    std::vector<Label*>        slotV1Label(fxSlotCount, nullptr);
    std::vector<Label*>        slotV2Label(fxSlotCount, nullptr);
    std::vector<Label*>        slotV3Label(fxSlotCount, nullptr);
    std::vector<Label*>        slotV4Label(fxSlotCount, nullptr);

    auto setEffectMix = [&](Effect* fx, const std::string& type, float mixVal) {
        if (!fx) return;
        if (type == "Reverb") {
            fx->setParameter("wetLevel", mixVal);
            fx->setParameter("dryLevel", 1.0f - mixVal);
        } else {
            fx->setParameter("mix", mixVal);
        }
    };

    auto createEffectWithDefaults = [&](const std::string& type) -> std::unique_ptr<Effect> {
        auto fx = createEffectComplete(type, audioEngine->getSampleRate());
        if (!fx) return nullptr;
        if (type == "Reverb") {
            fx->setParameter("roomSize", 0.7f);
            fx->setParameter("damping", 0.3f);
            fx->setParameter("wetLevel", 0.3f);
            fx->setParameter("dryLevel", 0.7f);
            fx->setParameter("width", 1.0f);
        } else if (type == "Delay") {
            fx->setParameter("delayTime", 0.35f);
            fx->setParameter("feedback", 0.35f);
            fx->setParameter("mix", 0.35f);
        } else if (type == "Distortion") {
            fx->setParameter("drive", 5.0f);
            fx->setParameter("level", 0.5f);
            fx->setParameter("tone", 0.5f);
            fx->setParameter("mix", 0.6f);
        } else if (type == "Saturation") {
            fx->setParameter("drive", 2.0f);
            fx->setParameter("tone", 0.5f);
            fx->setParameter("mix", 0.5f);
        } else if (type == "FDNReverb (Hall)") {
            fx->setParameter("mix", 0.25f);
            fx->setParameter("size", 1.0f);
            fx->setParameter("decay_rt60_s", 1.5f);
            fx->setParameter("high_damping", 0.3f);
            fx->setParameter("bass_mult", 1.0f);
            fx->setParameter("stereo_width", 1.0f);
            fx->setParameter("predelay_ms", 0.0f);
            fx->setParameter("diffusion", 0.5f);
            fx->setParameter("mod_rate", 0.15f);
            fx->setParameter("mod_depth", 0.2f);
        } else if (type == "PlateReverb") {
            fx->setParameter("mix", 0.20f);
        } else if (type == "BitCrusher") {
            fx->setParameter("bitDepth", 8.0f);
            fx->setParameter("sampleRateReduction", 0.5f);
            fx->setParameter("drive", 1.5f);
            fx->setParameter("mix", 0.5f);
        } else if (type == "Phaser") {
            fx->setParameter("rate", 0.5f);
            fx->setParameter("depth", 0.5f);
            fx->setParameter("feedback", 0.2f);
            fx->setParameter("mix", 0.5f);
        } else if (type == "EQ") {
            fx->setParameter("lowGain", 0.0f);
            fx->setParameter("midGain", 0.0f);
            fx->setParameter("highGain", 0.0f);
        } else if (type == "LowPassFilter") {
            fx->setParameter("frequency", 8000.0f);
            fx->setParameter("resonance", 1.0f);
            fx->setParameter("mix", 1.0f);
        } else if (type == "Chorus") {
            // Chorus implemented via Modulation effect
            fx->setParameter("rate", 0.8f);
            fx->setParameter("depth", 0.4f);
            fx->setParameter("spread", 0.3f);
            fx->setParameter("feedback", 0.0f);
        }
        return fx;
    };

    // Load persisted effects configuration if available
    struct LoadedSlot {
        std::string type = "None";
        float mix = 0.5f;
        bool enabled = true;
        int page = 0;
        std::unordered_map<std::string, float> params; // per current type
    };
    std::vector<LoadedSlot> loadedSlots;
    if (loadedConfig.contains("effects") && loadedConfig["effects"].contains("slots")) {
        try {
            for (const auto& s : loadedConfig["effects"]["slots"]) {
                LoadedSlot ls;
                if (s.contains("type")) ls.type = s["type"].get<std::string>();
                if (s.contains("mix")) ls.mix = s["mix"].get<float>();
                if (s.contains("enabled")) ls.enabled = s["enabled"].get<bool>();
                if (s.contains("page")) ls.page = std::max(0, std::min(1, s["page"].get<int>()));
                if (s.contains("params") && s["params"].is_object()) {
                    for (auto it = s["params"].begin(); it != s["params"].end(); ++it) {
                        ls.params[it.key()] = it.value().get<float>();
                    }
                }
                loadedSlots.push_back(std::move(ls));
            }
        } catch (...) {
            loadedSlots.clear();
        }
    }

    // Rebuild effects chain from slots, keeping the GLOBAL FILTER at the END of the chain
    auto rebuildEffectsChain = [&]() {
        std::lock_guard<std::mutex> lock(audioMutex);

        // Clear entire chain
        while (effectProcessor->getNumEffects() > 0) {
            effectProcessor->removeEffect(effectProcessor->getNumEffects() - 1);
        }

        // Re-add all user-selected effects in slot order
        for (int s = 0; s < fxSlotCount; ++s) {
            const std::string& type = slotSelectedType[s];
            if (type == "None") continue;
            auto fx = createEffectWithDefaults(type);
            if (!fx) continue;
            effectProcessor->addEffect(std::move(fx));
            // Apply parameters/mix/bypass
            auto* added = effectProcessor->getEffect(effectProcessor->getNumEffects() - 1);
            if (slotParamCache[s].count(type)) {
                for (const auto& kv : slotParamCache[s][type]) {
                    added->setParameter(kv.first, kv.second);
                }
            }
            bool enabledForType = slotEnabled[s];
            if (slotEnabledCache[s].count(type)) {
                enabledForType = slotEnabledCache[s][type];
            }
            float mixForType = slotMix[s];
            if (slotMixCache[s].count(type)) {
                mixForType = slotMixCache[s][type];
            }
            setEffectMix(added, type, enabledForType ? mixForType : 0.0f);
        }

        // Append GLOBAL FILTER last
        {
            auto globalFilter = std::make_unique<Filter>(audioEngine->getSampleRate(), Filter::Type::LowPass);
            globalFilter->setParameter("mix", 1.0f);
            // Apply current synth base parameters for cutoff/resonance
            float cutoffNorm = synthesizer->getParameter("filter_cutoff");
            cutoffNorm = std::max(0.0f, std::min(1.0f, cutoffNorm));
            float frequencyHz = 20.0f * std::pow(1000.0f, cutoffNorm);
            globalFilter->setParameter("frequency", frequencyHz);

            float resNorm = synthesizer->getParameter("filter_resonance");
            resNorm = std::max(0.0f, std::min(1.0f, resNorm));
            float resonanceQ = 0.7f + resNorm * 9.3f;
            globalFilter->setParameter("resonance", resonanceQ);

            effectProcessor->addEffect(std::move(globalFilter));
        }
    };

    // Helper: map slot index to effect instance in processor (filter reserved at END)
    auto getFxForSlot = [&]() -> std::function<Effect*(int)> {
        return [&](int slotIndex) -> Effect* {
            int countBefore = 0;
            for (int i = 0; i < slotIndex; ++i) {
                if (slotSelectedType[i] != "None") ++countBefore;
            }
            // User effects occupy indices [0 .. N-1]; global filter is at index N
            if (static_cast<size_t>(countBefore) < effectProcessor->getNumEffects()) {
                // Guard against returning the final global filter
                size_t lastIndex = effectProcessor->getNumEffects() - 1;
                size_t effectIndex = static_cast<size_t>(countBefore);
                if (effectIndex < lastIndex) {
                    return effectProcessor->getEffect(effectIndex);
                }
            }
            return nullptr;
        };
    }();

    // Helper to configure parameter sliders per slot and type
    auto configureSlotParams = [&](int s, const std::string& type){
        std::cout << "[FXUI] configure slot " << s << " type='" << type << "'" << std::endl;
        // Utility to disable a param slot
        auto disableParam = [&](Label* l, Slider* sl){
            if (l) l->setText("N/A");
            if (sl) { sl->setEnabled(false); sl->setValueChangeCallback(nullptr); sl->setRange(0.0f, 1.0f); }
        };

        // Handle None selection
        if (type == "None") {
            disableParam(slotV1Label[s], slotV1Slider[s]);
            disableParam(slotV2Label[s], slotV2Slider[s]);
            disableParam(slotV3Label[s], slotV3Slider[s]);
            disableParam(slotV4Label[s], slotV4Slider[s]);
            return;
        }

        // Get effect for this slot
        Effect* fx = getFxForSlot(s);
        if (!fx) {
            disableParam(slotV1Label[s], slotV1Slider[s]);
            disableParam(slotV2Label[s], slotV2Slider[s]);
            disableParam(slotV3Label[s], slotV3Slider[s]);
            disableParam(slotV4Label[s], slotV4Slider[s]);
            return;
        }

        // Enable all by default
        if (slotV1Slider[s]) slotV1Slider[s]->setEnabled(true);
        if (slotV2Slider[s]) slotV2Slider[s]->setEnabled(true);
        if (slotV3Slider[s]) slotV3Slider[s]->setEnabled(true);
        if (slotV4Slider[s]) slotV4Slider[s]->setEnabled(true);

        // Clear callbacks before reassigning
        if (slotV1Slider[s]) slotV1Slider[s]->setValueChangeCallback(nullptr);
        if (slotV2Slider[s]) slotV2Slider[s]->setValueChangeCallback(nullptr);
        if (slotV3Slider[s]) slotV3Slider[s]->setValueChangeCallback(nullptr);
        if (slotV4Slider[s]) slotV4Slider[s]->setValueChangeCallback(nullptr);

        // Mapping per type
        if (type == "Reverb") {
            std::cout << "[FXUI] slot " << s << " page=N/A (Reverb single page)" << std::endl;
            if (slotV1Label[s]) slotV1Label[s]->setText("Room Size");
            slotV1Slider[s]->setRange(0.0f, 1.0f);
            slotV1Slider[s]->setValue(fx->getParameter("roomSize"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Room "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("roomSize", v); slotParamCache[s][type]["roomSize"] = v; } });

            if (slotV2Label[s]) slotV2Label[s]->setText("Damping");
            slotV2Slider[s]->setRange(0.0f, 1.0f);
            slotV2Slider[s]->setValue(fx->getParameter("damping"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Damp "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("damping", v); slotParamCache[s][type]["damping"] = v; } });

            if (slotV3Label[s]) slotV3Label[s]->setText("Width");
            slotV3Slider[s]->setRange(0.0f, 1.0f);
            slotV3Slider[s]->setValue(fx->getParameter("width"));
            slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Width "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("width", v); slotParamCache[s][type]["width"] = v; } });

            disableParam(slotV4Label[s], slotV4Slider[s]);
        } else if (type == "Delay") {
            std::cout << "[FXUI] slot " << s << " page=N/A (Delay single page)" << std::endl;
            if (slotV1Label[s]) slotV1Label[s]->setText("Time (s)");
            slotV1Slider[s]->setRange(0.01f, 1.0f);
            slotV1Slider[s]->setValue(fx->getParameter("delayTime"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<v<<" s"; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("delayTime", v); slotParamCache[s][type]["delayTime"] = v; } });

            if (slotV2Label[s]) slotV2Label[s]->setText("Feedback");
            slotV2Slider[s]->setRange(0.0f, 0.95f);
            slotV2Slider[s]->setValue(fx->getParameter("feedback"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"FB "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("feedback", v); slotParamCache[s][type]["feedback"] = v; } });

            disableParam(slotV3Label[s], slotV3Slider[s]);
            disableParam(slotV4Label[s], slotV4Slider[s]);
        } else if (type == "Distortion") {
            std::cout << "[FXUI] slot " << s << " page=N/A (Distortion single page)" << std::endl;
            if (slotV1Label[s]) slotV1Label[s]->setText("Drive");
            slotV1Slider[s]->setRange(0.0f, 10.0f);
            slotV1Slider[s]->setValue(fx->getParameter("drive"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Drive "<<std::fixed<<std::setprecision(2)<<v; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("drive", v); slotParamCache[s][type]["drive"] = v; } });

            if (slotV2Label[s]) slotV2Label[s]->setText("Tone");
            slotV2Slider[s]->setRange(0.0f, 1.0f);
            slotV2Slider[s]->setValue(fx->getParameter("tone"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Tone "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("tone", v); slotParamCache[s][type]["tone"] = v; } });

            if (slotV3Label[s]) slotV3Label[s]->setText("Level");
            slotV3Slider[s]->setRange(0.0f, 1.0f);
            slotV3Slider[s]->setValue(fx->getParameter("level"));
            slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Level "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("level", v); slotParamCache[s][type]["level"] = v; } });

            disableParam(slotV4Label[s], slotV4Slider[s]);
        } else if (type == "BitCrusher") {
            std::cout << "[FXUI] slot " << s << " page=N/A (BitCrusher single page)" << std::endl;
            // BitCrusher: Bit Depth, Sample Rate Reduction, Drive, Output Trim
            if (slotV1Label[s]) slotV1Label[s]->setText("Bit Depth");
            slotV1Slider[s]->setRange(1.0f, 16.0f);
            slotV1Slider[s]->setValue(fx->getParameter("bitDepth"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(0)<<v; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("bitDepth", v); slotParamCache[s][type]["bitDepth"] = v; }
            });

            if (slotV2Label[s]) slotV2Label[s]->setText("SRR");
            slotV2Slider[s]->setRange(0.01f, 1.0f);
            slotV2Slider[s]->setValue(fx->getParameter("sampleRateReduction"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("sampleRateReduction", v); slotParamCache[s][type]["sampleRateReduction"] = v; }
            });

            if (slotV3Label[s]) slotV3Label[s]->setText("Drive");
            slotV3Slider[s]->setRange(1.0f, 10.0f);
            slotV3Slider[s]->setValue(fx->getParameter("drive"));
            slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<v; return ss.str();});
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("drive", v); slotParamCache[s][type]["drive"] = v; }
            });

            if (slotV4Label[s]) slotV4Label[s]->setText("Output Trim (dB)");
            slotV4Slider[s]->setRange(-12.0f, 6.0f);
            slotV4Slider[s]->setValue(fx->getParameter("output_trim_db"));
            slotV4Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<v<<" dB"; return ss.str();});
            slotV4Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("output_trim_db", v); slotParamCache[s][type]["output_trim_db"] = v; }
            });
        } else if (type == "Phaser") {
            std::cout << "[FXUI] slot " << s << " page=N/A (Phaser single page)" << std::endl;
            if (slotV1Label[s]) slotV1Label[s]->setText("Rate (Hz)");
            slotV1Slider[s]->setRange(0.05f, 5.0f);
            slotV1Slider[s]->setValue(fx->getParameter("rate"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<v<<" Hz"; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("rate", v); slotParamCache[s][type]["rate"] = v; } });

            if (slotV2Label[s]) slotV2Label[s]->setText("Depth");
            slotV2Slider[s]->setRange(0.0f, 1.0f);
            slotV2Slider[s]->setValue(fx->getParameter("depth"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Depth "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("depth", v); slotParamCache[s][type]["depth"] = v; } });

            if (slotV3Label[s]) slotV3Label[s]->setText("Feedback");
            slotV3Slider[s]->setRange(0.0f, 0.9f);
            slotV3Slider[s]->setValue(fx->getParameter("feedback"));
            slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"FB "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("feedback", v); slotParamCache[s][type]["feedback"] = v; } });

            disableParam(slotV4Label[s], slotV4Slider[s]);
        } else if (type == "EQ") {
            std::cout << "[FXUI] slot " << s << " page=N/A (EQ single page)" << std::endl;
            if (slotV1Label[s]) slotV1Label[s]->setText("Low (dB)");
            slotV1Slider[s]->setRange(-12.0f, 12.0f);
            slotV1Slider[s]->setValue(fx->getParameter("lowGain"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<v<<" dB"; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("lowGain", v); slotParamCache[s][type]["lowGain"] = v; } });

            if (slotV2Label[s]) slotV2Label[s]->setText("Mid (dB)");
            slotV2Slider[s]->setRange(-12.0f, 12.0f);
            slotV2Slider[s]->setValue(fx->getParameter("midGain"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<v<<" dB"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("midGain", v); slotParamCache[s][type]["midGain"] = v; } });

            if (slotV3Label[s]) slotV3Label[s]->setText("High (dB)");
            slotV3Slider[s]->setRange(-12.0f, 12.0f);
            slotV3Slider[s]->setValue(fx->getParameter("highGain"));
            slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<v<<" dB"; return ss.str();});
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("highGain", v); slotParamCache[s][type]["highGain"] = v; } });

            disableParam(slotV4Label[s], slotV4Slider[s]);
        } else if (type == "LowPassFilter") {
            std::cout << "[FXUI] slot " << s << " page=N/A (LPF single page)" << std::endl;
            if (slotV1Label[s]) slotV1Label[s]->setText("Cutoff (Hz)");
            slotV1Slider[s]->setRange(20.0f, 20000.0f);
            slotV1Slider[s]->setValue(fx->getParameter("frequency"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; if (v >= 1000.0f) { ss<<std::fixed<<std::setprecision(2)<<(v/1000.0f)<<" kHz"; } else { ss<<std::fixed<<std::setprecision(0)<<v<<" Hz"; } return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("frequency", v); slotParamCache[s][type]["frequency"] = v; } });

            if (slotV2Label[s]) slotV2Label[s]->setText("Resonance");
            slotV2Slider[s]->setRange(0.7f, 5.0f);
            slotV2Slider[s]->setValue(fx->getParameter("resonance"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Q "<<std::fixed<<std::setprecision(2)<<v; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("resonance", v); slotParamCache[s][type]["resonance"] = v; } });

            disableParam(slotV3Label[s], slotV3Slider[s]);
            disableParam(slotV4Label[s], slotV4Slider[s]);
        } else if (type == "Chorus") {
            std::cout << "[FXUI] slot " << s << " page=N/A (Chorus single page)" << std::endl;
            // Chorus (Modulation) parameter mapping
            if (slotV1Label[s]) slotV1Label[s]->setText("Rate (Hz)");
            slotV1Slider[s]->setRange(0.1f, 5.0f);
            slotV1Slider[s]->setValue(fx->getParameter("rate"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<v<<" Hz"; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("rate", v); slotParamCache[s][type]["rate"] = v; } });

            if (slotV2Label[s]) slotV2Label[s]->setText("Depth");
            slotV2Slider[s]->setRange(0.0f, 1.0f);
            slotV2Slider[s]->setValue(fx->getParameter("depth"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Depth "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("depth", v); slotParamCache[s][type]["depth"] = v; } });

            if (slotV3Label[s]) slotV3Label[s]->setText("Spread");
            slotV3Slider[s]->setRange(0.0f, 1.0f);
            slotV3Slider[s]->setValue(fx->getParameter("spread"));
            slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Spread "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("spread", v); slotParamCache[s][type]["spread"] = v; } });

            if (slotV4Label[s]) slotV4Label[s]->setText("Output Trim (dB)");
            slotV4Slider[s]->setRange(-12.0f, 6.0f);
            slotV4Slider[s]->setValue(fx->getParameter("output_trim_db"));
            slotV4Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<v<<" dB"; return ss.str();});
            slotV4Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("output_trim_db", v); slotParamCache[s][type]["output_trim_db"] = v; }
            });
        } else if (type == "Saturation") {
            // Saturation: Drive, Tone, Mix, Output Trim
            if (slotV1Label[s]) slotV1Label[s]->setText("Drive");
            slotV1Slider[s]->setRange(1.0f, 20.0f);
            slotV1Slider[s]->setValue(fx->getParameter("drive"));
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("drive", v); slotParamCache[s][type]["drive"] = v; } });

            if (slotV2Label[s]) slotV2Label[s]->setText("Tone");
            slotV2Slider[s]->setRange(0.0f, 1.0f);
            slotV2Slider[s]->setValue(fx->getParameter("tone"));
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("tone", v); slotParamCache[s][type]["tone"] = v; } });

            if (slotV3Label[s]) slotV3Label[s]->setText("Mix");
            slotV3Slider[s]->setRange(0.0f, 1.0f);
            slotV3Slider[s]->setValue(fx->getParameter("mix"));
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("mix", v); slotParamCache[s][type]["mix"] = v; } });

            if (slotV4Label[s]) slotV4Label[s]->setText("Output Trim (dB)");
            slotV4Slider[s]->setRange(-12.0f, 6.0f);
            slotV4Slider[s]->setValue(fx->getParameter("output_trim_db"));
            slotV4Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<v<<" dB"; return ss.str();});
            slotV4Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("output_trim_db", v); slotParamCache[s][type]["output_trim_db"] = v; }
            });
        } else if (type == "FDNReverb (Hall)") {
            // FDN Hall: switchable pages via fx_page_[s] dropdown
            int page = 0;
            if (auto* es = uiContext->getScreen("effects")) {
                if (auto* p = dynamic_cast<DropdownMenu*>(es->getChild("fx_page_" + std::to_string(s)))) {
                    page = p->getSelectedIndex();
                }
            }
            std::cout << "[FXUI] slot " << s << " page=" << page << " (FDN)" << std::endl;
            if (page == 0) {
                if (slotV1Label[s]) slotV1Label[s]->setText("Predelay (ms)");
                slotV1Slider[s]->setRange(0.0f, 100.0f);
                slotV1Slider[s]->setValue(fx->getParameter("predelay_ms"));
                slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(0)<<v<<" ms"; return ss.str();});
                slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("predelay_ms", v); slotParamCache[s][type]["predelay_ms"] = v; } });

                if (slotV2Label[s]) slotV2Label[s]->setText("Size");
                slotV2Slider[s]->setRange(0.5f, 2.0f);
                slotV2Slider[s]->setValue(fx->getParameter("size"));
                slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Size "<<std::fixed<<std::setprecision(2)<<v<<"x"; return ss.str();});
                slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("size", v); slotParamCache[s][type]["size"] = v; } });

                if (slotV3Label[s]) slotV3Label[s]->setText("Diffusion");
                slotV3Slider[s]->setRange(0.0f, 1.0f);
                slotV3Slider[s]->setValue(fx->getParameter("diffusion"));
                slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Diff "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
                slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("diffusion", v); slotParamCache[s][type]["diffusion"] = v; } });

                if (slotV4Label[s]) slotV4Label[s]->setText("Mod Rate (Hz)");
                slotV4Slider[s]->setRange(0.05f, 1.0f);
                slotV4Slider[s]->setValue(fx->getParameter("mod_rate"));
                slotV4Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<v<<" Hz"; return ss.str();});
                slotV4Slider[s]->setValueChangeCallback([&, s, type](float v){ std::lock_guard<std::mutex> lock(audioMutex); if (auto* f = getFxForSlot(s)) { f->setParameter("mod_rate", v); slotParamCache[s][type]["mod_rate"] = v; } });
                return;
            }
            // Page 2: Expose Decay, High Damping, Bass Mult, Stereo Width
            if (slotV1Label[s]) slotV1Label[s]->setText("Decay (s)");
            slotV1Slider[s]->setRange(0.2f, 20.0f);
            slotV1Slider[s]->setValue(fx->getParameter("decay_rt60_s"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<v<<" s"; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("decay_rt60_s", v); slotParamCache[s][type]["decay_rt60_s"] = v; }
            });

            if (slotV2Label[s]) slotV2Label[s]->setText("High Damp");
            slotV2Slider[s]->setRange(0.0f, 1.0f);
            slotV2Slider[s]->setValue(fx->getParameter("high_damping"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"HiDamp "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("high_damping", v); slotParamCache[s][type]["high_damping"] = v; }
            });

            if (slotV3Label[s]) slotV3Label[s]->setText("Bass Mult");
            slotV3Slider[s]->setRange(0.5f, 2.0f);
            slotV3Slider[s]->setValue(fx->getParameter("bass_mult"));
            slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Bass "<<std::fixed<<std::setprecision(2)<<v<<"x"; return ss.str();});
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("bass_mult", v); slotParamCache[s][type]["bass_mult"] = v; }
            });

            if (slotV4Label[s]) slotV4Label[s]->setText("Output Trim (dB)");
            slotV4Slider[s]->setRange(-12.0f, 6.0f);
            slotV4Slider[s]->setValue(fx->getParameter("output_trim_db"));
            slotV4Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<v<<" dB"; return ss.str();});
            slotV4Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("output_trim_db", v); slotParamCache[s][type]["output_trim_db"] = v; }
            });
        } else if (type == "PlateReverb") {
            int page = 0;
            if (auto* es = uiContext->getScreen("effects")) {
                if (auto* p = dynamic_cast<DropdownMenu*>(es->getChild("fx_page_" + std::to_string(s)))) {
                    page = p->getSelectedIndex();
                }
            }
            std::cout << "[FXUI] slot " << s << " page=" << page << " (Plate)" << std::endl;
            if (page == 0) {
                // Page 1: Predelay, Diffusion, Mod Rate, Mod Depth
                if (slotV1Label[s]) slotV1Label[s]->setText("Predelay (ms)");
                slotV1Slider[s]->setRange(0.0f, 100.0f);
                slotV1Slider[s]->setValue(fx->getParameter("predelay_ms"));
                slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(0)<<v<<" ms"; return ss.str();});
                slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){
                    std::lock_guard<std::mutex> lock(audioMutex);
                    if (auto* f = getFxForSlot(s)) { f->setParameter("predelay_ms", v); slotParamCache[s][type]["predelay_ms"] = v; }
                });

                if (slotV2Label[s]) slotV2Label[s]->setText("Diffusion");
                slotV2Slider[s]->setRange(0.0f, 1.0f);
                slotV2Slider[s]->setValue(fx->getParameter("diffusion"));
                slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Diff "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
                slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){
                    std::lock_guard<std::mutex> lock(audioMutex);
                    if (auto* f = getFxForSlot(s)) { f->setParameter("diffusion", v); slotParamCache[s][type]["diffusion"] = v; }
                });

                if (slotV3Label[s]) slotV3Label[s]->setText("Mod Rate (Hz)");
                slotV3Slider[s]->setRange(0.05f, 1.0f);
                slotV3Slider[s]->setValue(fx->getParameter("mod_rate"));
                slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<v<<" Hz"; return ss.str();});
                slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){
                    std::lock_guard<std::mutex> lock(audioMutex);
                    if (auto* f = getFxForSlot(s)) { f->setParameter("mod_rate", v); slotParamCache[s][type]["mod_rate"] = v; }
                });

                if (slotV4Label[s]) slotV4Label[s]->setText("Mod Depth (%)");
                slotV4Slider[s]->setRange(0.0f, 0.25f);
                slotV4Slider[s]->setValue(fx->getParameter("mod_depth"));
                slotV4Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<(v*100.0f)<<"%"; return ss.str();});
                slotV4Slider[s]->setValueChangeCallback([&, s, type](float v){
                    std::lock_guard<std::mutex> lock(audioMutex);
                    if (auto* f = getFxForSlot(s)) { f->setParameter("mod_depth", v); slotParamCache[s][type]["mod_depth"] = v; }
                });
                return;
            }
            // Page 2: Decay, High Damping (tone), Size, Output Trim
            if (slotV1Label[s]) slotV1Label[s]->setText("Decay (s)");
            slotV1Slider[s]->setRange(0.2f, 20.0f);
            slotV1Slider[s]->setValue(fx->getParameter("decay_rt60_s"));
            slotV1Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(2)<<v<<" s"; return ss.str();});
            slotV1Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("decay_rt60_s", v); slotParamCache[s][type]["decay_rt60_s"] = v; }
            });

            if (slotV2Label[s]) slotV2Label[s]->setText("High Damp");
            slotV2Slider[s]->setRange(0.0f, 1.0f);
            slotV2Slider[s]->setValue(fx->getParameter("high_damping"));
            slotV2Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"HiDamp "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
            slotV2Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("high_damping", v); slotParamCache[s][type]["high_damping"] = v; }
            });

            if (slotV3Label[s]) slotV3Label[s]->setText("Size");
            slotV3Slider[s]->setRange(0.5f, 2.0f);
            slotV3Slider[s]->setValue(fx->getParameter("size"));
            slotV3Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<"Size "<<std::fixed<<std::setprecision(2)<<v<<"x"; return ss.str();});
            slotV3Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("size", v); slotParamCache[s][type]["size"] = v; }
            });

            if (slotV4Label[s]) slotV4Label[s]->setText("Output Trim (dB)");
            slotV4Slider[s]->setRange(-12.0f, 6.0f);
            slotV4Slider[s]->setValue(fx->getParameter("output_trim_db"));
            slotV4Slider[s]->setValueFormatter([](float v){ std::stringstream ss; ss<<std::fixed<<std::setprecision(1)<<v<<" dB"; return ss.str();});
            slotV4Slider[s]->setValueChangeCallback([&, s, type](float v){
                std::lock_guard<std::mutex> lock(audioMutex);
                if (auto* f = getFxForSlot(s)) { f->setParameter("output_trim_db", v); slotParamCache[s][type]["output_trim_db"] = v; }
            });
            // Consider adding Size on Plate: mapped to Mix knob row if needed later
        } else {
            // Unknown: disable all
            disableParam(slotV1Label[s], slotV1Slider[s]);
            disableParam(slotV2Label[s], slotV2Slider[s]);
            disableParam(slotV3Label[s], slotV3Slider[s]);
            disableParam(slotV4Label[s], slotV4Slider[s]);
        }
    };

    // Helper: explicitly set the four vertical parameter labels for a slot based on type/page
    auto updateFxSlotLabels = [&](int s){
        int page = 0;
        if (auto* es = uiContext->getScreen("effects")) {
            if (auto* p = dynamic_cast<DropdownMenu*>(es->getChild("fx_page_" + std::to_string(s)))) {
                page = std::max(0, p->getSelectedIndex());
            }
        }
        const std::string type = slotSelectedType[s];
        auto setLbl = [&](Label* l, const char* txt){ if (l) l->setText(txt); };
        if (type == "FDNReverb (Hall)") {
            if (page == 0) {
                setLbl(slotV1Label[s], "Predelay (ms)");
                setLbl(slotV2Label[s], "Size");
                setLbl(slotV3Label[s], "Diffusion");
                setLbl(slotV4Label[s], "Mod Rate (Hz)");
            } else {
                setLbl(slotV1Label[s], "Decay (s)");
                setLbl(slotV2Label[s], "High Damp");
                setLbl(slotV3Label[s], "Bass Mult");
                setLbl(slotV4Label[s], "Output Trim (dB)");
            }
        } else if (type == "PlateReverb") {
            if (page == 0) {
                setLbl(slotV1Label[s], "Predelay (ms)");
                setLbl(slotV2Label[s], "Diffusion");
                setLbl(slotV3Label[s], "Mod Rate (Hz)");
                setLbl(slotV4Label[s], "Mod Depth (%)");
            } else {
                setLbl(slotV1Label[s], "Decay (s)");
                setLbl(slotV2Label[s], "High Damp");
                setLbl(slotV3Label[s], "Size");
                setLbl(slotV4Label[s], "Output Trim (dB)");
            }
        } else if (type == "Reverb") {
            setLbl(slotV1Label[s], "Room Size");
            setLbl(slotV2Label[s], "Damping");
            setLbl(slotV3Label[s], "Width");
            setLbl(slotV4Label[s], "N/A");
        } else if (type == "Delay") {
            setLbl(slotV1Label[s], "Time (s)");
            setLbl(slotV2Label[s], "Feedback");
            setLbl(slotV3Label[s], "N/A");
            setLbl(slotV4Label[s], "N/A");
        } else if (type == "BitCrusher") {
            setLbl(slotV1Label[s], "Bit Depth");
            setLbl(slotV2Label[s], "SRR");
            setLbl(slotV3Label[s], "Drive");
            setLbl(slotV4Label[s], "Output Trim (dB)");
        } else if (type == "Phaser") {
            setLbl(slotV1Label[s], "Rate (Hz)");
            setLbl(slotV2Label[s], "Depth");
            setLbl(slotV3Label[s], "Feedback");
            setLbl(slotV4Label[s], "N/A");
        } else if (type == "EQ") {
            setLbl(slotV1Label[s], "Low (dB)");
            setLbl(slotV2Label[s], "Mid (dB)");
            setLbl(slotV3Label[s], "High (dB)");
            setLbl(slotV4Label[s], "N/A");
        } else if (type == "LowPassFilter") {
            setLbl(slotV1Label[s], "Cutoff (Hz)");
            setLbl(slotV2Label[s], "Resonance");
            setLbl(slotV3Label[s], "N/A");
            setLbl(slotV4Label[s], "N/A");
        } else if (type == "Chorus") {
            setLbl(slotV1Label[s], "Rate (Hz)");
            setLbl(slotV2Label[s], "Depth");
            setLbl(slotV3Label[s], "Spread");
            setLbl(slotV4Label[s], "Output Trim (dB)");
        } else if (type == "Saturation") {
            setLbl(slotV1Label[s], "Drive");
            setLbl(slotV2Label[s], "Tone");
            setLbl(slotV3Label[s], "Mix");
            setLbl(slotV4Label[s], "Output Trim (dB)");
        } else {
            setLbl(slotV1Label[s], "N/A");
            setLbl(slotV2Label[s], "N/A");
            setLbl(slotV3Label[s], "N/A");
            setLbl(slotV4Label[s], "N/A");
        }
        // Diagnostics for verification
        auto lbl = [&](Label* l){ return l ? l->getText() : std::string("<null>"); };
        std::cout << "[FXUI] slot " << s << " page " << page << " labels=['"
                  << lbl(slotV1Label[s]) << "','" << lbl(slotV2Label[s]) << "','"
                  << lbl(slotV3Label[s]) << "','" << lbl(slotV4Label[s]) << "']" << std::endl;
    };

    // Build per-slot rows
    for (int s = 0; s < fxSlotCount; ++s) {
        int y = rowStartY + s * rowHeight;

        auto slotLabel = std::make_unique<Label>("fx_slot_label_" + std::to_string(s), "Slot " + std::to_string(s+1));
        slotLabel->setPosition(50, y);
        slotLabel->setSize(60, 20);
        slotLabel->setTextColor(Color(180, 180, 180));
        effectsScreen->addChild(std::move(slotLabel));

        auto typeDd = std::make_unique<DropdownMenu>("fx_type_" + std::to_string(s), "None");
        typeDd->setPosition(120, y-4);
        typeDd->setSize(200, 28); // wider to show text fully
        typeDd->addItem("None");
        for (const auto& t : AIMusicHardware::getAvailableEffects()) typeDd->addItem(t);
        slotTypeDd[s] = typeDd.get();
        // Default to "None" selected so text is visible
        typeDd->selectItem(0);

        auto bypassBtn = std::make_unique<Button>("fx_bypass_" + std::to_string(s), "ON");
        // Position bypass to the right of the dropdown to avoid overlapping the arrow
        bypassBtn->setPosition(330, y-4);
        bypassBtn->setSize(50, 28);
        bypassBtn->setToggleMode(true);
        bypassBtn->setBackgroundColor(Color(50,100,50));
        bypassBtn->setTextColor(Color(255,255,255));
        slotBypassBtn[s] = bypassBtn.get();

        auto mix = std::make_unique<Slider>("fx_mix_" + std::to_string(s), "Mix", 0,0,40,40);
        mix->setOrientation(Slider::Orientation::Horizontal);
        // Shift mix slider 200px right from baseline, keep 5px down
        mix->setPosition(400, y - 3);
        // Reduce horizontal slider size by an additional ~30%
        mix->setSize(156, 19);
        mix->setRange(0.0f, 1.0f);
        mix->setValue(slotMix[s]);
        mix->setValueFormatter([](float v){ std::stringstream ss; ss<<"Mix "<<std::fixed<<std::setprecision(0)<<(v*100.0f)<<"%"; return ss.str();});
        slotMixSlider[s] = mix.get();

        // Add four vertical sliders to the right of the mix slider (placeholders for per-slot params)
        // Shift vertical sliders 100px to the right from previous position
        const int vBaseX = 610;
        const int vSpacing = 90;  // keep spacing; only scale slider size
        const int vWidth = 15;    // ~30% smaller than previous 21
        const int vHeight = 62;   // ~30% smaller than previous 88
        // Shift vertical sliders an additional 10px down for alignment
        const int vY = y - 30;    // keep vertical alignment

        auto v1Label = std::make_unique<Label>("fx_v1_label_" + std::to_string(s), "Param 1");
        v1Label->setPosition(vBaseX - 12, vY - 16);
        v1Label->setSize(70, 16);
        v1Label->setTextColor(Color(200, 200, 200));
        slotV1Label[s] = v1Label.get();
        effectsScreen->addChild(std::move(v1Label));

        auto v1 = std::make_unique<Slider>("fx_v1_" + std::to_string(s), "", 0, 0, vWidth, vHeight);
        v1->setOrientation(Slider::Orientation::Vertical);
        v1->setPosition(vBaseX, vY);
        v1->setSize(vWidth, vHeight);
        v1->setRange(0.0f, 1.0f);
        v1->setValue(0.0f);
        v1->setShowValue(false);
        slotV1Slider[s] = v1.get();

        auto v2Label = std::make_unique<Label>("fx_v2_label_" + std::to_string(s), "Param 2");
        v2Label->setPosition(vBaseX + vSpacing - 12, vY - 16);
        v2Label->setSize(70, 16);
        v2Label->setTextColor(Color(200, 200, 200));
        slotV2Label[s] = v2Label.get();
        effectsScreen->addChild(std::move(v2Label));

        auto v2 = std::make_unique<Slider>("fx_v2_" + std::to_string(s), "", 0, 0, vWidth, vHeight);
        v2->setOrientation(Slider::Orientation::Vertical);
        v2->setPosition(vBaseX + vSpacing, vY);
        v2->setSize(vWidth, vHeight);
        v2->setRange(0.0f, 1.0f);
        v2->setValue(0.0f);
        v2->setShowValue(false);
        slotV2Slider[s] = v2.get();

        auto v3Label = std::make_unique<Label>("fx_v3_label_" + std::to_string(s), "Param 3");
        v3Label->setPosition(vBaseX + 2*vSpacing - 12, vY - 16);
        v3Label->setSize(70, 16);
        v3Label->setTextColor(Color(200, 200, 200));
        slotV3Label[s] = v3Label.get();
        effectsScreen->addChild(std::move(v3Label));

        auto v3 = std::make_unique<Slider>("fx_v3_" + std::to_string(s), "", 0, 0, vWidth, vHeight);
        v3->setOrientation(Slider::Orientation::Vertical);
        v3->setPosition(vBaseX + 2*vSpacing, vY);
        v3->setSize(vWidth, vHeight);
        v3->setRange(0.0f, 1.0f);
        v3->setValue(0.0f);
        v3->setShowValue(false);
        slotV3Slider[s] = v3.get();

        auto v4Label = std::make_unique<Label>("fx_v4_label_" + std::to_string(s), "Param 4");
        v4Label->setPosition(vBaseX + 3*vSpacing - 12, vY - 16);
        v4Label->setSize(70, 16);
        v4Label->setTextColor(Color(200, 200, 200));
        slotV4Label[s] = v4Label.get();
        effectsScreen->addChild(std::move(v4Label));

        auto v4 = std::make_unique<Slider>("fx_v4_" + std::to_string(s), "", 0, 0, vWidth, vHeight);
        v4->setOrientation(Slider::Orientation::Vertical);
        v4->setPosition(vBaseX + 3*vSpacing, vY);
        v4->setSize(vWidth, vHeight);
        v4->setRange(0.0f, 1.0f);
        v4->setValue(0.0f);
        v4->setShowValue(false);
        slotV4Slider[s] = v4.get();
        
        // Page dropdown to switch parameter pages for effects (e.g., Hall Page1/Page2)
        auto pageDd = std::make_unique<DropdownMenu>("fx_page_" + std::to_string(s), "Page 1");
        pageDd->setPosition(vBaseX + 3*vSpacing + 40, vY - 4);
        pageDd->setSize(90, 22);
        pageDd->addItem("Page 1");
        pageDd->addItem("Page 2");
        DropdownMenu* pageDdPtr = pageDd.get();
        // Ensure default selection is Page 1 for consistent behavior
        pageDd->selectItemSilently(0);
        effectsScreen->addChild(std::move(pageDd));
        
        // Callbacks

        typeDd->setSelectionCallback([&, s](int index, const std::string& item){
            slotSelectedType[s] = item;
            rebuildEffectsChain();
            // Enable/disable controls if None is selected
            bool enableControls = (item != std::string("None"));
            slotMixSlider[s]->setEnabled(enableControls);
            slotBypassBtn[s]->setEnabled(enableControls);
            if (slotV1Slider[s]) slotV1Slider[s]->setEnabled(enableControls);
            if (slotV2Slider[s]) slotV2Slider[s]->setEnabled(enableControls);
            if (slotV3Slider[s]) slotV3Slider[s]->setEnabled(enableControls);
            if (slotV4Slider[s]) slotV4Slider[s]->setEnabled(enableControls);
            // Restore per-type cached mix/enabled UI if available
            float uiMix = slotMix[s];
            if (slotMixCache[s].count(item)) uiMix = slotMixCache[s][item];
            slotMixSlider[s]->setValue(uiMix);
            bool uiEnabled = slotEnabled[s];
            if (slotEnabledCache[s].count(item)) uiEnabled = slotEnabledCache[s][item];
            slotBypassBtn[s]->setText(uiEnabled ? "ON" : "OFF");
            slotBypassBtn[s]->setBackgroundColor(uiEnabled ? Color(50,100,50) : Color(100,50,50));
            // Configure parameter mappings for this slot/type
            configureSlotParams(s, item);
            updateFxSlotLabels(s);
            // Reset to Page 1 on type change
            if (auto* es = uiContext->getScreen("effects")) {
                if (auto* p = dynamic_cast<DropdownMenu*>(es->getChild("fx_page_" + std::to_string(s)))) {
                    // Use non-silent selection so UI shows the change and callback reconfigures params
                    p->selectItem(0);
                    slotPage[s] = 0;
                }
            }
            // Mirror selection to main-screen quick FX dropdowns (first 3 slots)
            if (s < 3) {
                // If this selection originated from the main screen, do not mirror back
                if (!suppressMainToEffects[s]) {
                    if (auto* mainScreenPtr = uiContext->getScreen("main")) {
                        if (auto* mainDd = dynamic_cast<DropdownMenu*>(mainScreenPtr->getChild("effect_type_" + std::to_string(s)))) {
                            suppressEffectsToMain[s] = true;
                            mainDd->selectItemSilently(index);
                            suppressEffectsToMain[s] = false;
                        }
                    }
                }
            }
        });

        mix->setValueChangeCallback([&, s](float v){
            std::lock_guard<std::mutex> lock(audioMutex);
            slotMix[s] = v;
            const std::string type = slotSelectedType[s];
            slotMixCache[s][type] = v; // cache per-type mix
            if (auto* fx = getFxForSlot(s)) setEffectMix(fx, type, slotEnabled[s] ? v : 0.0f);
        });

        bypassBtn->setClickCallback([&, s](){
            std::lock_guard<std::mutex> lock(audioMutex);
            bool enabled = slotBypassBtn[s]->getText() == std::string("OFF");
            slotEnabled[s] = enabled;
            slotBypassBtn[s]->setText(enabled ? "ON" : "OFF");
            slotBypassBtn[s]->setBackgroundColor(enabled ? Color(50,100,50) : Color(100,50,50));
            const std::string type = slotSelectedType[s];
            slotEnabledCache[s][type] = enabled; // cache per-type enabled
            if (auto* fx = getFxForSlot(s)) setEffectMix(fx, type, enabled ? slotMix[s] : 0.0f);
        });

        // Page dropdown callback
        pageDdPtr->setSelectionCallback([&, s](int idx, const std::string& item){
            slotPage[s] = idx;
            // Reconfigure with same type to redraw param set
            configureSlotParams(s, slotSelectedType[s]);
            updateFxSlotLabels(s);
            // Force UI to reflect page by updating label placeholder
            if (auto* es = uiContext->getScreen("effects")) {
                if (auto* p = dynamic_cast<DropdownMenu*>(es->getChild("fx_page_" + std::to_string(s)))) {
                    p->selectItemSilently(idx);
                }
            }
        });

        // Add to screen
        effectsScreen->addChild(std::move(typeDd));
        effectsScreen->addChild(std::move(bypassBtn));
        effectsScreen->addChild(std::move(mix));
        effectsScreen->addChild(std::move(v1));
        effectsScreen->addChild(std::move(v2));
        effectsScreen->addChild(std::move(v3));
        effectsScreen->addChild(std::move(v4));
        // Initialize controls; apply loaded state if present
        if (s < static_cast<int>(loadedSlots.size())) {
            const auto& ls = loadedSlots[s];
            slotSelectedType[s] = ls.type;
            slotMix[s] = ls.mix;
            slotEnabled[s] = ls.enabled;
            slotPage[s] = std::max(0, std::min(1, ls.page));
            if (!ls.type.empty() && ls.type != "None" && !ls.params.empty()) {
                slotParamCache[s][ls.type] = ls.params;
            }
            // Select type in UI (this will trigger rebuild and configure)
            int idx = 0;
            if (ls.type == "None") idx = 0; else {
                // find in list
                int found = -1;
                const auto effects = AIMusicHardware::getAvailableEffects();
                for (size_t ii = 0; ii < effects.size(); ++ii) {
                    if (effects[ii] == ls.type) { found = static_cast<int>(ii + 1); break; }
                }
                idx = (found >= 0) ? found : 0;
            }
            slotTypeDd[s]->selectItem(idx);
            // Apply persisted page selection and reconfigure
            if (auto* p = dynamic_cast<DropdownMenu*>(effectsScreen->getChild("fx_page_" + std::to_string(s)))) {
                p->selectItemSilently(slotPage[s]);
                configureSlotParams(s, slotSelectedType[s]);
                updateFxSlotLabels(s);
            }
            // Restore mix and bypass UI
            slotMixSlider[s]->setValue(slotMix[s]);
            slotBypassBtn[s]->setText(slotEnabled[s] ? "ON" : "OFF");
            slotBypassBtn[s]->setBackgroundColor(slotEnabled[s] ? Color(50,100,50) : Color(100,50,50));
        } else {
            // Initialize controls disabled for None, with N/A labels
            configureSlotParams(s, slotSelectedType[s]);
            updateFxSlotLabels(s);
            bool enableControlsInit = (slotSelectedType[s] != std::string("None"));
            slotMixSlider[s]->setEnabled(enableControlsInit);
            slotBypassBtn[s]->setEnabled(enableControlsInit);
            if (slotV1Slider[s]) slotV1Slider[s]->setEnabled(enableControlsInit);
            if (slotV2Slider[s]) slotV2Slider[s]->setEnabled(enableControlsInit);
            if (slotV3Slider[s]) slotV3Slider[s]->setEnabled(enableControlsInit);
            if (slotV4Slider[s]) slotV4Slider[s]->setEnabled(enableControlsInit);
        }
    }

    uiContext->addScreen(std::move(effectsScreen));
    
    // Create Modulation screen
    auto modulationScreen = std::make_unique<Screen>("modulation");
    modulationScreen->setBackgroundColor(Color(30, 35, 40));
    modulationScreen->setPosition(0, 0);
    modulationScreen->setSize(1280, 800);
    addNavigationButtons(modulationScreen.get(), "modulation", uiContext.get());
    
    auto modTitle = std::make_unique<Label>("mod_title", "MODULATION MATRIX");
    modTitle->setPosition(50, 50);
    modTitle->setSize(250, 30);
    modTitle->setTextColor(Color(255, 255, 100));
    modulationScreen->addChild(std::move(modTitle));
    
    auto modInfo = std::make_unique<Label>("mod_info", "Visual modulation routing coming soon...");
    modInfo->setPosition(50, 100);
    modInfo->setSize(400, 25);
    modInfo->setTextColor(Color(200, 200, 200));
    modulationScreen->addChild(std::move(modInfo));
    
    uiContext->addScreen(std::move(modulationScreen));
    
    // Create Presets screen
    auto presetsScreen = std::make_unique<Screen>("presets");
    presetsScreen->setBackgroundColor(Color(35, 35, 40));
    presetsScreen->setPosition(0, 0);
    presetsScreen->setSize(1280, 800);
    addNavigationButtons(presetsScreen.get(), "presets", uiContext.get());
    
    auto presetsTitle = std::make_unique<Label>("presets_title", "PRESET BROWSER");
    presetsTitle->setPosition(50, 50);
    presetsTitle->setSize(200, 30);
    presetsTitle->setTextColor(Color(255, 255, 100));
    presetsScreen->addChild(std::move(presetsTitle));
    
    auto presetsInfo = std::make_unique<Label>("presets_info", "Enhanced preset management coming soon...");
    presetsInfo->setPosition(50, 100);
    presetsInfo->setSize(400, 25);
    presetsInfo->setTextColor(Color(200, 200, 200));
    presetsScreen->addChild(std::move(presetsInfo));
    
    uiContext->addScreen(std::move(presetsScreen));
    
    // Create Settings screen
    auto settingsScreen = std::make_unique<Screen>("settings");
    settingsScreen->setBackgroundColor(Color(30, 30, 35));
    settingsScreen->setPosition(0, 0);
    settingsScreen->setSize(1280, 800);
    addNavigationButtons(settingsScreen.get(), "settings", uiContext.get());
    
    auto settingsTitle = std::make_unique<Label>("settings_title", "SETTINGS");
    settingsTitle->setPosition(50, 50);
    settingsTitle->setSize(200, 30);
    settingsTitle->setTextColor(Color(255, 255, 100));
    settingsScreen->addChild(std::move(settingsTitle));
    
    auto audioSection = std::make_unique<Label>("audio_section", "Audio Configuration");
    audioSection->setPosition(50, 100);
    audioSection->setSize(200, 25);
    audioSection->setTextColor(Color(200, 200, 255));
    settingsScreen->addChild(std::move(audioSection));
    
    auto sampleRateLabel = std::make_unique<Label>("sample_rate", "Sample Rate: 44100 Hz");
    sampleRateLabel->setPosition(70, 130);
    sampleRateLabel->setSize(200, 20);
    sampleRateLabel->setTextColor(Color(180, 180, 180));
    settingsScreen->addChild(std::move(sampleRateLabel));
    
    auto bufferSizeLabel = std::make_unique<Label>("buffer_size", "Buffer Size: 512 samples");
    bufferSizeLabel->setPosition(70, 155);
    bufferSizeLabel->setSize(200, 20);
    bufferSizeLabel->setTextColor(Color(180, 180, 180));
    settingsScreen->addChild(std::move(bufferSizeLabel));
    
    auto midiSection = std::make_unique<Label>("midi_section", "MIDI Configuration");
    midiSection->setPosition(50, 200);
    midiSection->setSize(200, 25);
    midiSection->setTextColor(Color(200, 200, 255));
    settingsScreen->addChild(std::move(midiSection));
    
    auto midiInfo = std::make_unique<Label>("midi_info", "MIDI device selection and mapping options coming soon...");
    midiInfo->setPosition(70, 230);
    midiInfo->setSize(400, 20);
    midiInfo->setTextColor(Color(180, 180, 180));
    settingsScreen->addChild(std::move(midiInfo));
    
    // Add Audio Quality section
    auto qualitySection = std::make_unique<Label>("quality_section", "Audio Quality");
    qualitySection->setPosition(50, 280);
    qualitySection->setSize(200, 25);
    qualitySection->setTextColor(Color(200, 200, 255));
    settingsScreen->addChild(std::move(qualitySection));
    
    auto qualityLabel = std::make_unique<Label>("quality_label", "Oversampling:");
    qualityLabel->setPosition(70, 310);
    qualityLabel->setSize(100, 20);
    qualityLabel->setTextColor(Color(180, 180, 180));
    settingsScreen->addChild(std::move(qualityLabel));
    
    auto qualityDropdown = std::make_unique<DropdownMenu>("quality", "1x (Draft)");
    qualityDropdown->setPosition(170, 305);
    qualityDropdown->setSize(120, 30);
    qualityDropdown->addItem("1x (Draft)");
    qualityDropdown->addItem("2x (Good)");
    qualityDropdown->addItem("4x (Better)");
    qualityDropdown->addItem("8x (Best)");
    qualityDropdown->setSelectionCallback([&synthesizer](int index, const std::string& item) {
        OversamplingProcessor::Factor factors[] = {
            OversamplingProcessor::Factor::x1,
            OversamplingProcessor::Factor::x2,
            OversamplingProcessor::Factor::x4,
            OversamplingProcessor::Factor::x8
        };
        
        synthesizer->setOversamplingFactor(factors[index]);
        std::cout << "Audio quality set to " << item << std::endl;
    });
    settingsScreen->addChild(std::move(qualityDropdown));
    
    auto aliasingInfo = std::make_unique<Label>("aliasing_info", "Band-limited oscillators enabled for zero aliasing");
    aliasingInfo->setPosition(70, 345);
    aliasingInfo->setSize(400, 20);
    aliasingInfo->setTextColor(Color(150, 255, 150));
    settingsScreen->addChild(std::move(aliasingInfo));

    // Engine status label (shows current engine mode)
    auto engineStatus = std::make_unique<Label>("engine_status", "Engine: Legacy/Realtime WT");
    engineStatus->setPosition(70, 370);
    engineStatus->setSize(400, 20);
    engineStatus->setTextColor(Color(180, 200, 255));
    settingsScreen->addChild(std::move(engineStatus));

    // Hybrid engine toggle (persistent)
    auto hybridToggle = std::make_unique<Button>("hybrid_toggle", "Engine: Legacy (click to switch)");
    hybridToggle->setPosition(70, 400);
    hybridToggle->setSize(260, 30);
    hybridToggle->setBackgroundColor(Color(60, 80, 110));
    auto hybridTogglePtr = hybridToggle.get();
    hybridToggle->setClickCallback([&](void){
        bool target = !synthesizer->isHybridWavetableEnabled();
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            synthesizer->setHybridWavetableEnabled(target);
        }
        // Read back actual state after change
        bool actual = synthesizer->isHybridWavetableEnabled();
        if (auto* settings = uiContext->getScreen("settings")) {
            if (auto* lbl = dynamic_cast<Label*>(settings->getChild("engine_status"))) {
                lbl->setText(actual ? "Engine: Hybrid (Spectral)" : "Engine: Legacy/Realtime WT");
            }
        }
        hybridTogglePtr->setText(actual ? "Engine: Hybrid (click to switch)" : "Engine: Legacy (click to switch)");
    });
    settingsScreen->addChild(std::move(hybridToggle));

    // Timbre mode toggle (Linear / Min-Phase) — persisted
    auto timbreToggle = std::make_unique<Button>("timbre_mode_toggle", "Timbre: Linear");
    timbreToggle->setPosition(340, 400);
    timbreToggle->setSize(180, 30);
    timbreToggle->setBackgroundColor(Color(60, 80, 110));
    auto timbreTogglePtr = timbreToggle.get();
    timbreToggle->setClickCallback([&](void){
        bool next = !synthesizer->isHybridTimbreMinPhase();
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            synthesizer->setHybridTimbreMinPhase(next);
            // Propagate to voice manager so new voices pick it up immediately
            if (auto* vm = synthesizer->getVoiceManager()) {
                vm->setHybridMinPhase(next);
            }
        }
        timbreTogglePtr->setText(next ? "Timbre: Min-Phase" : "Timbre: Linear");
    });
    settingsScreen->addChild(std::move(timbreToggle));

    // Hybrid worker/cache stats label
    auto hybridStats = std::make_unique<Label>("hybrid_stats", "Hybrid: cache 0/0, Q 0, in-flight 0");
    hybridStats->setPosition(70, 440);
    hybridStats->setSize(500, 20);
    hybridStats->setTextColor(Color(160, 200, 200));
    settingsScreen->addChild(std::move(hybridStats));

    // Hybrid cache capacity slider
    auto cacheLabel = std::make_unique<Label>("hybrid_cache_label", "Cache Capacity: 256");
    cacheLabel->setPosition(70, 470);
    cacheLabel->setSize(260, 20);
    cacheLabel->setTextColor(Color(180, 180, 180));
    settingsScreen->addChild(std::move(cacheLabel));

    auto cacheSlider = std::make_unique<Slider>("hybrid_cache_capacity");
    cacheSlider->setPosition(70, 500);
    cacheSlider->setSize(320, 24);
    cacheSlider->setRange(64.0f, 1024.0f);
    cacheSlider->setOrientation(Slider::Orientation::Horizontal);
    // Hide inline value to avoid duplication/overlap with the label
    cacheSlider->setShowValue(false);
    auto cacheSliderPtr = cacheSlider.get();
    cacheSlider->setValueChangeCallback([&](float v){
        int cap = std::max(64, static_cast<int>(v));
        if (synthesizer) {
            if (auto cache = synthesizer->getSpectralCache()) {
                cache->setCapacity(static_cast<size_t>(cap));
            }
        }
        if (auto* settings = uiContext->getScreen("settings")) {
            if (auto* lbl = dynamic_cast<Label*>(settings->getChild("hybrid_cache_label"))) {
                lbl->setText(std::string("Cache Capacity: ") + std::to_string(cap));
            }
        }
    });
    settingsScreen->addChild(std::move(cacheSlider));

    // Prewarm breadth sliders
    auto prewarmMorphLabel = std::make_unique<Label>("prewarm_morph_label", "Prewarm Morph Frames (±)");
    prewarmMorphLabel->setPosition(70, 540);
    prewarmMorphLabel->setSize(320, 20);
    prewarmMorphLabel->setTextColor(Color(180, 180, 180));
    settingsScreen->addChild(std::move(prewarmMorphLabel));

    auto prewarmMorph = std::make_unique<Slider>("prewarm_morph");
    prewarmMorph->setPosition(70, 570);
    prewarmMorph->setSize(180, 24);
    prewarmMorph->setRange(0.0f, 4.0f);
    prewarmMorph->setOrientation(Slider::Orientation::Horizontal);
    prewarmMorph->setValueFormatter([](float v){ std::stringstream ss; ss<<"M ±"<<static_cast<int>(v); return ss.str();});
    auto prewarmPitchLabel = std::make_unique<Label>("prewarm_pitch_label", "Prewarm Pitch Bands (±)");
    prewarmPitchLabel->setPosition(70, 630);
    prewarmPitchLabel->setSize(320, 20);
    prewarmPitchLabel->setTextColor(Color(180, 180, 180));
    settingsScreen->addChild(std::move(prewarmPitchLabel));

    auto prewarmPitch = std::make_unique<Slider>("prewarm_pitch");
    prewarmPitch->setPosition(70, 660);
    prewarmPitch->setSize(180, 24);
    prewarmPitch->setRange(0.0f, 4.0f);
    prewarmPitch->setOrientation(Slider::Orientation::Horizontal);
    prewarmPitch->setValueFormatter([](float v){ std::stringstream ss; ss<<"P ±"<<static_cast<int>(v); return ss.str();});
    auto prewarmMorphPtr = prewarmMorph.get();
    auto prewarmPitchPtr = prewarmPitch.get();
    auto applyPrewarm = [&](){
        int m = static_cast<int>(prewarmMorphPtr->getValue());
        int p = static_cast<int>(prewarmPitchPtr->getValue());
        if (synthesizer && synthesizer->getSpectralWorker()) {
            synthesizer->getSpectralWorker()->setPrewarmBreadth(m, p);
        }
        if (auto* settings = uiContext->getScreen("settings")) {
            if (auto* mlbl = dynamic_cast<Label*>(settings->getChild("prewarm_morph_label"))) {
                std::stringstream ss; ss<<"Prewarm Morph Frames (±"<<m<<")"; mlbl->setText(ss.str());
            }
            if (auto* plbl = dynamic_cast<Label*>(settings->getChild("prewarm_pitch_label"))) {
                std::stringstream ss; ss<<"Prewarm Pitch Bands (±"<<p<<")"; plbl->setText(ss.str());
            }
        }
    };
    prewarmMorph->setValueChangeCallback([&](float){ applyPrewarm(); });
    prewarmPitch->setValueChangeCallback([&](float){ applyPrewarm(); });
    settingsScreen->addChild(std::move(prewarmMorph));
    settingsScreen->addChild(std::move(prewarmPitch));

    // Reset stats button
    auto resetStatsBtn = std::make_unique<Button>("reset_hybrid_stats", "Reset Stats");
    resetStatsBtn->setPosition(70, 740);
    resetStatsBtn->setSize(140, 28);
    resetStatsBtn->setBackgroundColor(Color(80, 60, 60));
    resetStatsBtn->setClickCallback([&](){
        if (synthesizer && synthesizer->getSpectralCache()) {
            synthesizer->getSpectralCache()->resetStats();
        }
    });
    settingsScreen->addChild(std::move(resetStatsBtn));

    // Explanations to the right of controls
    auto cacheExplain = std::make_unique<Label>("hybrid_cache_explain", "Caches rendered wave cycles; higher = more memory, fewer misses.");
    cacheExplain->setPosition(410, 500);
    cacheExplain->setSize(520, 20);
    cacheExplain->setTextColor(Color(150, 180, 180));
    settingsScreen->addChild(std::move(cacheExplain));

    // Preset Output Trim (dB)
    auto trimLabel = std::make_unique<Label>("preset_trim_label", "Preset Output Trim (dB)");
    trimLabel->setPosition(50, 560);
    trimLabel->setSize(200, 20);
    trimLabel->setTextColor(Color(200, 200, 200));
    settingsScreen->addChild(std::move(trimLabel));

    auto trimSlider = std::make_unique<Slider>("preset_output_trim_db");
    trimSlider->setPosition(50, 590);
    trimSlider->setSize(300, 20);
    trimSlider->setOrientation(Slider::Orientation::Horizontal);
    trimSlider->setRange(-12.0f, 6.0f);
    trimSlider->setValue(synthesizer->getParameter("preset_output_trim_db"));
    trimSlider->setValueFormatter([](float v){ char buf[32]; std::snprintf(buf, sizeof(buf), "%+.1f dB", v); return std::string(buf); });
    trimSlider->setValueChangeCallback([&](float v){ synthesizer->setParameter("preset_output_trim_db", v); });
    settingsScreen->addChild(std::move(trimSlider));

    auto morphExplain = std::make_unique<Label>("prewarm_morph_explain", "Prepares neighboring morph frames to reduce stalls during morph modulation.");
    morphExplain->setPosition(260, 570);
    morphExplain->setSize(600, 20);
    morphExplain->setTextColor(Color(150, 180, 180));
    settingsScreen->addChild(std::move(morphExplain));

    auto pitchExplain = std::make_unique<Label>("prewarm_pitch_explain", "Prepares adjacent pitch bands for fast pitch bends/LFO; smooths Hybrid under rapid changes.");
    pitchExplain->setPosition(260, 690);
    pitchExplain->setSize(600, 20);
    pitchExplain->setTextColor(Color(150, 180, 180));
    settingsScreen->addChild(std::move(pitchExplain));
    
    uiContext->addScreen(std::move(settingsScreen));
    
    // Bind Save preset callback now that effects UI state is available
    if (auto* btn = dynamic_cast<Button*>(uiContext->getScreen("main")->getChild("save_preset"))) {
        btn->setClickCallback([&, presetDropdownPtr]() {
            try {
                using clock = std::chrono::system_clock;
                auto now = clock::now();
                auto t = clock::to_time_t(now);
                std::tm tm{};
#ifdef _WIN32
                localtime_s(&tm, &t);
#else
                localtime_r(&t, &tm);
#endif
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d_%02d-%02d-%02d",
                            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                            tm.tm_hour, tm.tm_min, tm.tm_sec);
                std::string name = std::string("Quick Save ") + buf;
                std::string author = "";
                std::string category = "User";
                std::string description = "Quick-saved from UI";
                std::string dir = PresetManager::getUserPresetsDirectory();
                std::string path = dir + "/" + name + ".preset";

                nlohmann::json presetJson;
                // Metadata with schema, uuid, and timestamps
                presetJson["schema_version"] = "1.0.0";
                // Generate a simple UUID v4-like string
                auto genHex = [](int n){
                    static const char* kHex = "0123456789abcdef";
                    std::string s; s.reserve(n);
                    std::random_device rd; std::mt19937 rng(rd()); std::uniform_int_distribution<int> d(0,15);
                    for (int i=0;i<n;++i) s.push_back(kHex[d(rng)]);
                    return s;
                };
                std::string uuid = genHex(8) + std::string("-") + genHex(4) + std::string("-") + genHex(4) + std::string("-") + genHex(4) + std::string("-") + genHex(12);
                presetJson["metadata"] = {{"name",name},{"author",author},{"category",category},{"description",description},{"version","1.0.0"},{"uuid",uuid}};
                presetJson["metadata"]["created_at"] = static_cast<int64_t>(t);
                presetJson["metadata"]["modified_at"] = static_cast<int64_t>(t);
                // Synth params
                {
                    nlohmann::json params;
                    auto allParams = synthesizer->getAllParameters();
                    for (const auto& kv : allParams) params[kv.first] = kv.second;
                    // Ensure preset-level trim exists
                    if (!params.contains("preset_output_trim_db")) {
                        params["preset_output_trim_db"] = synthesizer->getParameter("preset_output_trim_db");
                    }
                    presetJson["parameters"] = params;
                }
                // Effects
                {
                    nlohmann::json slots = nlohmann::json::array();
                    for (int s = 0; s < fxSlotCount; ++s) {
                        nlohmann::json sj;
                        sj["type"] = slotSelectedType[s];
                        sj["mix"] = slotMix[s];
                        sj["enabled"] = slotEnabled[s];
                        sj["page"] = std::max(0, std::min(1, slotPage[s]));
                        if (slotParamCache[s].count(slotSelectedType[s])) {
                            nlohmann::json pj;
                            for (const auto& kv : slotParamCache[s][slotSelectedType[s]]) pj[kv.first] = kv.second;
                            sj["params"] = pj;
                        }
                        slots.push_back(sj);
                    }
                    presetJson["effects"]["slots"] = slots;
                }
                // Mod routing (include forward-compatible stable IDs)
                {
                    nlohmann::json mods = nlohmann::json::array();
                    auto normalizeId = [](std::string s){
                        std::string out; out.reserve(s.size());
                        for (char c : s) {
                            char lc = (char)std::tolower((unsigned char)c);
                            out.push_back(std::isalnum((unsigned char)lc) ? lc : '_');
                        }
                        // collapse consecutive underscores
                        std::string out2; out2.reserve(out.size());
                        bool prevUnderscore = false;
                        for (char c : out) {
                            if (c=='_') { if (!prevUnderscore) { out2.push_back(c); prevUnderscore = true; } }
                            else { out2.push_back(c); prevUnderscore=false; }
                        }
                        return out2;
                    };
                    for (size_t i = 0; i < modConnections.size(); ++i) {
                        if (modConnections[i].source.empty() || modConnections[i].destination.empty()) continue;
                        nlohmann::json mj;
                        std::string src = modConnections[i].source;
                        if (src == "LFO 1") src = "LFO1"; else if (src == "LFO 2") src = "LFO2"; else if (src == "Mod Wheel") src = "ModWheel";
                        mj["source"] = src;                                 // canonical source name
                        mj["source_id"] = normalizeId(src);                // normalized source id
                        const std::string& dstDisplay = modConnections[i].destination;
                        mj["destination"] = dstDisplay;                    // display name
                        // Build a structured dest_id for FX: fxs{slot}_{effectKey}_{paramKey}; synth: synth_{paramKey}
                        auto effectKeyFromType = [&](const std::string& fxType){
                            if (fxType == "FDNReverb (Hall)") return std::string("fdnreverbhall");
                            if (fxType == "PlateReverb") return std::string("plateverb");
                            if (fxType == "LowPassFilter") return std::string("lowpassfilter");
                            if (fxType == "Modulation") return std::string("modulation");
                            return normalizeId(fxType);
                        };
                        auto paramKeyFromDisplay = [&](std::string disp){
                            // drop effect prefix word(s)
                            auto pos = disp.find(' ');
                            if (pos != std::string::npos) disp = disp.substr(pos + 1);
                            // special handling for prefixes with slash
                            if (disp.rfind("LPF ", 0) == 0) disp = disp.substr(4);
                            // normalize
                            auto key = normalizeId(disp);
                            // common remaps
                            if (key == "mod_rate") key = "mod_rate";
                            if (key == "output_trim_db" || key == "output_trim__db") key = "output_trim_db";
                            if (key == "predelay" || key == "predelay_ms") key = "predelay";
                            if (key == "high_damp" || key == "high_damping") key = "high_damping";
                            return key;
                        };
                        std::string destId = normalizeId(dstDisplay);
                        mj["amount"] = modConnections[i].amount;
                        // Optional: include per-slot effect qualifier when determinable
                        auto inferEffect = [&](const std::string& disp)->std::string{
                            if (disp.rfind("Hall ", 0) == 0) return "FDNReverb (Hall)";
                            if (disp.rfind("Plate ", 0) == 0) return "PlateReverb";
                            if (disp.rfind("Delay ", 0) == 0) return "Delay";
                            if (disp.rfind("Reverb ", 0) == 0) return "Reverb";
                            if (disp.rfind("FX LPF", 0) == 0) return "LowPassFilter";
                            if (disp.rfind("Saturation ", 0) == 0) return "Saturation";
                            if (disp.rfind("Distortion ", 0) == 0) return "Distortion";
                            if (disp.rfind("Phaser ", 0) == 0) return "Phaser";
                            if (disp.rfind("EQ ", 0) == 0) return "EQ";
                            if (disp.rfind("BitCrusher ", 0) == 0) return "BitCrusher";
                            if (disp.rfind("Compressor ", 0) == 0) return "Compressor";
                            if (disp.rfind("BassBoost ", 0) == 0) return "BassBoost";
                            if (disp.rfind("Chorus/Mod ", 0) == 0) return "Modulation";
                            return std::string();
                        };
                        std::string fxType = inferEffect(dstDisplay);
                        if (!fxType.empty()) {
                            int foundSlot = -1;
                            for (int s = 0; s < fxSlotCount; ++s) {
                                if (slotSelectedType[s] == fxType) { foundSlot = s; break; }
                            }
                            if (foundSlot >= 0) {
                                mj["fx_slot"] = foundSlot + 1; // 1-based for readability
                                mj["effect"] = fxType;
                                std::string effectKey = effectKeyFromType(fxType);
                                std::string paramKey = paramKeyFromDisplay(dstDisplay);
                                destId = std::string("fxs") + std::to_string(foundSlot + 1) + "_" + effectKey + "_" + paramKey;
                            }
                        }
                        mj["dest_id"] = destId;
                        mods.push_back(mj);
                    }
                    presetJson["mod_routing"] = mods;
                }
                std::ofstream out(path);
                if (out.is_open()) { out << presetJson.dump(2); out.close(); std::cout << "Saved preset to: " << path << std::endl; }
                if (auto* dropdown = dynamic_cast<PresetDropdown*>(uiContext->getScreen("main")->getChild("preset_dropdown"))) {
                    dropdown->addPreset(name, category, path);
                }
            } catch (...) { std::cerr << "Exception while saving preset" << std::endl; }
        });
    }

    // Re-bind Load preset to support .preset (synth + effects + modulation)
    if (auto* loadBtn = dynamic_cast<Button*>(uiContext->getScreen("main")->getChild("load_preset"))) {
        loadBtn->setClickCallback([&, presetDropdownPtr, waveSliderPtr, cutoffSliderPtr, resSliderPtr, volumeSliderPtr,
                                   lfoRateSliderPtr, lfoDepthSliderPtr, lfoShapeSliderPtr,
                                   quickFxDd]() mutable {
            auto* mainScreenPtr = uiContext->getScreen("main");
            auto* dropdown = dynamic_cast<PresetDropdown*>(mainScreenPtr->getChild("preset_dropdown"));
            if (!dropdown) return;
            auto selectedPreset = dropdown->getSelectedPreset();
            if (selectedPreset.fullPath.empty()) return;
            std::cout << "Loading preset: " << selectedPreset.name << " from " << selectedPreset.fullPath << std::endl;
            // Case-insensitive .preset extension check
            auto pathLower = selectedPreset.fullPath;
            std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(), [](unsigned char c){ return std::tolower(c); });
            if (pathLower.size() >= 7 && pathLower.rfind(".preset") == pathLower.size() - 7) {
                // Load custom preset JSON
                try {
                    nlohmann::json j;
                    std::ifstream in(selectedPreset.fullPath);
                    if (!in.is_open()) { std::cerr << "Failed to open preset file" << std::endl; return; }
                    in >> j; in.close();
                        // Parameters
                    if (j.contains("parameters") && j["parameters"].is_object()) {
                        std::map<std::string,float> pm;
                        for (auto it = j["parameters"].begin(); it != j["parameters"].end(); ++it) {
                            pm[it.key()] = it.value().get<float>();
                        }
                            // Apply non-LFO first in batch
                            std::map<std::string,float> nonLfo;
                            for (const auto& kv : pm) {
                                if (kv.first.rfind("lfo", 0) != 0) nonLfo[kv.first] = kv.second;
                            }
                            synthesizer->setAllParameters(nonLfo);
                        // Handle preset-level output trim explicitly (optional)
                        if (pm.count("preset_output_trim_db")) {
                            synthesizer->setParameter("preset_output_trim_db", pm["preset_output_trim_db"]);
                        }
                        // Engine toggles (if present)
                        if (pm.count("engine.hybrid_enabled")) {
                            synthesizer->setParameter("engine.hybrid_enabled", pm["engine.hybrid_enabled"]);
                        }
                        if (pm.count("engine.timbre_min_phase")) {
                            synthesizer->setParameter("engine.timbre_min_phase", pm["engine.timbre_min_phase"]);
                        }
                            // Extract LFOs from preset (fallback to current engine only if not present)
                            bool hasL1r = pm.count("lfo1_rate"), hasL1d = pm.count("lfo1_depth"), hasL1s = pm.count("lfo1_shape");
                            bool hasL2r = pm.count("lfo2_rate"), hasL2d = pm.count("lfo2_depth"), hasL2s = pm.count("lfo2_shape");
                            float l1r = hasL1r ? pm["lfo1_rate"]  : synthesizer->getParameter("lfo1_rate");
                            float l1d = hasL1d ? pm["lfo1_depth"] : synthesizer->getParameter("lfo1_depth");
                            float l1s = hasL1s ? pm["lfo1_shape"] : synthesizer->getParameter("lfo1_shape");
                            float l2r = hasL2r ? pm["lfo2_rate"]  : synthesizer->getParameter("lfo2_rate");
                            float l2d = hasL2d ? pm["lfo2_depth"] : synthesizer->getParameter("lfo2_depth");
                            float l2s = hasL2s ? pm["lfo2_shape"] : synthesizer->getParameter("lfo2_shape");
                            // Apply LFOs explicitly to the engine
                            synthesizer->setParameter("lfo1_rate", l1r);
                            synthesizer->setParameter("lfo1_depth", l1d);
                            synthesizer->setParameter("lfo1_shape", l1s);
                            synthesizer->setParameter("lfo2_rate", l2r);
                            synthesizer->setParameter("lfo2_depth", l2d);
                            synthesizer->setParameter("lfo2_shape", l2s);
                        // Update states and engine, then shared sliders by temporarily switching the selector silently
                        if (lfoRateSliderPtr && lfoDepthSliderPtr && lfoShapeSliderPtr) {
                            if (lfoSelectorDropdownPtr) {
                                int prev = std::max(0, lfoSelectorDropdownPtr->getSelectedIndex());
                                // Apply LFO1
                                lfoSelectorDropdownPtr->selectItemSilently(0);
                                lfo1State.rate = l1r; lfo1State.depth = l1d; lfo1State.shape = l1s;
                                synthesizer->setParameter("lfo1_rate", l1r);
                                synthesizer->setParameter("lfo1_depth", l1d);
                                synthesizer->setParameter("lfo1_shape", l1s);
                                std::cout << "[PresetLoad] Applied LFO1: rate=" << l1r << ", depth=" << l1d << ", shape=" << l1s << std::endl;
                                lfoRateSliderPtr->setValueSilently(l1r);
                                lfoDepthSliderPtr->setValueSilently(l1d);
                                lfoShapeSliderPtr->setValueSilently(l1s);
                                // Apply LFO2
                                lfoSelectorDropdownPtr->selectItemSilently(1);
                                lfo2State.rate = l2r; lfo2State.depth = l2d; lfo2State.shape = l2s;
                                synthesizer->setParameter("lfo2_rate", l2r);
                                synthesizer->setParameter("lfo2_depth", l2d);
                                synthesizer->setParameter("lfo2_shape", l2s);
                                std::cout << "[PresetLoad] Applied LFO2: rate=" << l2r << ", depth=" << l2d << ", shape=" << l2s << std::endl;
                                lfoRateSliderPtr->setValueSilently(l2r);
                                lfoDepthSliderPtr->setValueSilently(l2d);
                                lfoShapeSliderPtr->setValueSilently(l2s);
                                lfoSelectorDropdownPtr->selectItemSilently(prev); // Restore prior selection
                                if (prev == 0) {
                                    lfoRateSliderPtr->setValue(l1r);
                                    lfoDepthSliderPtr->setValue(l1d);
                                    lfoShapeSliderPtr->setValue(l1s);
                                } else {
                                    lfoRateSliderPtr->setValue(l2r);
                                    lfoDepthSliderPtr->setValue(l2d);
                                    lfoShapeSliderPtr->setValue(l2s);
                                }
                            } else {
                                // No selector found; just show LFO1 values
                                lfoRateSliderPtr->setValue(l1r);
                                lfoDepthSliderPtr->setValue(l1d);
                                lfoShapeSliderPtr->setValue(l1s);
                            }
                        }
                    } else if (j.contains("synth") && j["synth"].is_object()) {
                        // Backward-compat: load from "synth" object
                        std::map<std::string,float> pm;
                        for (auto it = j["synth"].begin(); it != j["synth"].end(); ++it) {
                            if (it.value().is_number()) pm[it.key()] = it.value().get<float>();
                        }
                        synthesizer->setAllParameters(pm);
                    }
                    // Update top-level UI sliders for core synth params + envelope
                    if (waveSliderPtr) waveSliderPtr->setValue(synthesizer->getParameter("oscillator_type"));
                    if (cutoffSliderPtr) cutoffSliderPtr->setValue(synthesizer->getParameter("filter_cutoff"));
                    if (resSliderPtr) resSliderPtr->setValue(synthesizer->getParameter("filter_resonance"));
                    if (volumeSliderPtr) volumeSliderPtr->setValue(synthesizer->getParameter("master_volume"));
                    if (attackSliderPtr) attackSliderPtr->setValue(synthesizer->getParameter("envelope_attack"));
                    if (decaySliderPtr) decaySliderPtr->setValue(synthesizer->getParameter("envelope_decay"));
                    if (sustainSliderPtr) sustainSliderPtr->setValue(synthesizer->getParameter("envelope_sustain"));
                    if (releaseSliderPtr) releaseSliderPtr->setValue(synthesizer->getParameter("envelope_release"));
                    if (envelopePtr) envelopePtr->setADSR(
                        synthesizer->getParameter("envelope_attack"),
                        synthesizer->getParameter("envelope_decay"),
                        synthesizer->getParameter("envelope_sustain"),
                        synthesizer->getParameter("envelope_release")
                    );
                    // Start a brief ramp to avoid clicks as parameters settle
                    synthesizer->startPresetApplyRamp(0.02f);
                    std::cout << "Loaded synth params: cutoff=" << synthesizer->getParameter("filter_cutoff")
                              << ", res=" << synthesizer->getParameter("filter_resonance")
                              << ", vol=" << synthesizer->getParameter("master_volume") << std::endl;
                    // Effects slots
                    if (j.contains("effects") && j["effects"].contains("slots") && j["effects"]["slots"].is_array()) {
                        int idx = 0;
                        for (const auto& s : j["effects"]["slots"]) {
                            if (idx >= fxSlotCount) break;
                            if (s.contains("type")) slotSelectedType[idx] = s["type"].get<std::string>();
                            if (s.contains("mix")) slotMix[idx] = s["mix"].get<float>();
                            if (s.contains("enabled")) slotEnabled[idx] = s["enabled"].get<bool>();
                            if (s.contains("page")) slotPage[idx] = std::max(0, std::min(1, s["page"].get<int>()));
                            slotParamCache[idx][slotSelectedType[idx]].clear();
                            if (s.contains("params") && s["params"].is_object()) {
                                for (auto it = s["params"].begin(); it != s["params"].end(); ++it) {
                                    slotParamCache[idx][slotSelectedType[idx]][it.key()] = it.value().get<float>();
                                }
                            }
                            ++idx;
                        }
                        // Update UI widgets for slots
                        for (int s = 0; s < fxSlotCount; ++s) {
                            if (slotTypeDd[s]) slotTypeDd[s]->selectItemSilently(slotSelectedType[s]);
                            if (slotMixSlider[s]) slotMixSlider[s]->setValue(slotMix[s]);
                            if (slotBypassBtn[s]) {
                                slotBypassBtn[s]->setText(slotEnabled[s] ? "ON" : "OFF");
                                slotBypassBtn[s]->setBackgroundColor(slotEnabled[s] ? Color(50,100,50) : Color(100,50,50));
                            }
                            // Apply saved page and force callback to refresh labels
                            if (auto* p = dynamic_cast<DropdownMenu*>(uiContext->getScreen("effects")->getChild("fx_page_" + std::to_string(s)))) {
                                int pageIdx = std::max(0, std::min(1, slotPage[s]));
                                p->selectItemSilently(pageIdx);
                                // Fire selection callback to update parameter labels immediately
                                p->selectItem(pageIdx);
                                // Force a no-op toggle to ensure label text/values bind even if index stayed the same
                                int other = pageIdx == 0 ? 1 : 0;
                                std::cout << "[FXUI] slot " << s << " force-toggle page " << pageIdx << "->" << other << "->" << pageIdx << std::endl;
                                p->selectItem(other);
                                p->selectItem(pageIdx);
                            }
                            // Reconfigure parameter sliders for this slot based on page
                            configureSlotParams(s, slotSelectedType[s]);
                            // Mirror to main quick FX dropdowns for first three slots
                            if (s < static_cast<int>(quickFxDd.size()) && quickFxDd[s]) {
                                quickFxDd[s]->selectItemSilently(slotSelectedType[s]);
                            }
                        }
                        // Rebuild chain
                        rebuildEffectsChain();
                        std::cout << "Rebuilt FX chain after preset load; global filter cutoff now="
                                  << synthesizer->getParameter("filter_cutoff") << std::endl;
                    }
                    // Modulation routing
                    if (j.contains("mod_routing") && j["mod_routing"].is_array()) {
                        // Build destination ID map from current engine destinations
                        auto normalizeId = [](std::string s){
                            std::string out; out.reserve(s.size());
                            for (char c : s) { char lc = (char)std::tolower((unsigned char)c); out.push_back(std::isalnum((unsigned char)lc) ? lc : '_'); }
                            // collapse consecutive underscores
                            std::string out2; out2.reserve(out.size());
                            bool prevUnderscore = false; for (char c : out) { if (c=='_') { if (!prevUnderscore) { out2.push_back(c); prevUnderscore = true; } } else { out2.push_back(c); prevUnderscore=false; } }
                            return out2;
                        };
                        std::unordered_map<std::string,std::string> idToName;
                        {
                            auto names = synthesizer->getModDestinationNames();
                            for (const auto& n : names) { idToName[normalizeId(n)] = n; }
                        }
                        // Build a slot→effect map from current UI slot selection to help disambiguate
                        auto remapFxDisplayToType = [&](const std::string& disp)->std::string{
                            if (disp.rfind("Hall ", 0) == 0) return "FDNReverb (Hall)";
                            if (disp.rfind("Plate ", 0) == 0) return "PlateReverb";
                            if (disp.rfind("Delay ", 0) == 0) return "Delay";
                            if (disp.rfind("Reverb ", 0) == 0) return "Reverb";
                            if (disp.rfind("FX LPF", 0) == 0) return "LowPassFilter";
                            if (disp.rfind("Saturation ", 0) == 0) return "Saturation";
                            if (disp.rfind("Distortion ", 0) == 0) return "Distortion";
                            if (disp.rfind("Phaser ", 0) == 0) return "Phaser";
                            if (disp.rfind("EQ ", 0) == 0) return "EQ";
                            if (disp.rfind("BitCrusher ", 0) == 0) return "BitCrusher";
                            if (disp.rfind("Compressor ", 0) == 0) return "Compressor";
                            if (disp.rfind("BassBoost ", 0) == 0) return "BassBoost";
                            if (disp.rfind("Chorus/Mod ", 0) == 0) return "Modulation";
                            return std::string();
                        };
                        // Update UI model and engine
                        for (size_t i = 0; i < modConnections.size(); ++i) {
                            if (i >= j["mod_routing"].size()) break;
                            const auto& mr = j["mod_routing"][i];
                            std::string src = mr.value("source_id", mr.value("source", ""));
                            std::string dstDisplay = mr.value("destination", "");
                            std::string dstId = mr.contains("dest_id") ? mr["dest_id"].get<std::string>() : dstDisplay;
                            int fxSlot1Based = mr.value("fx_slot", 0);
                            std::string fxType = mr.value("effect", remapFxDisplayToType(dstDisplay));
                            float amt = mr.value("amount", 0.0f);
                            // Resolve destination by ID first; if structured fxs{slot}_{effect}_{param}, try param-only match first
                            std::string resolvedDest = dstDisplay;
                            std::string normId = normalizeId(dstId);
                            auto it = idToName.find(normId);
                            if (it != idToName.end()) {
                                resolvedDest = it->second;
                            } else {
                                // Try to extract trailing param key from structured id
                                auto lastUnderscore = normId.rfind('_');
                                if (lastUnderscore != std::string::npos && lastUnderscore + 1 < normId.size()) {
                                    std::string paramKey = normId.substr(lastUnderscore + 1);
                                    auto pit = idToName.find(paramKey);
                                    if (pit != idToName.end()) resolvedDest = pit->second;
                                }
                            }
                            // If an fx_slot qualifier exists and there are multiple similar dests, prefer the one matching the slot's effect type
                            if (fxSlot1Based > 0 && fxSlot1Based <= fxSlotCount && !fxType.empty()) {
                                int slotIdx = fxSlot1Based - 1;
                                if (slotSelectedType[slotIdx] == fxType) {
                                    // Keep resolvedDest as-is (we don't have per-slot destination variants yet), but we could prioritize later
                                }
                            }
                            // Helper to canonicalize source for display
                            auto toDisplaySource = [](std::string s)->std::string{
                                std::string lower; lower.reserve(s.size());
                                for (char c : s) lower.push_back(std::tolower(static_cast<unsigned char>(c)));
                                if (lower == "lfo1" || lower == "lfo 1") return "LFO 1";
                                if (lower == "lfo2" || lower == "lfo 2") return "LFO 2";
                                if (lower == "modwheel" || lower == "mod wheel") return "Mod Wheel";
                                if (lower == "aftertouch") return "Aftertouch";
                                if (lower == "velocity") return "Velocity";
                                if (lower == "envelope" || lower == "env") return "Envelope";
                                if (lower == "none" || lower.empty() || lower == "noen") return "None";
                                return s;
                            };
                            // Update model (display names)
                            modConnections[i].source = toDisplaySource(src);
                            modConnections[i].destination = resolvedDest;
                            modConnections[i].amount = amt;
                            // Map source to engine-internal source name (case/format)
                            auto toEngineSource = [](std::string s)->std::string{
                                std::string lower; lower.reserve(s.size());
                                for (char c : s) lower.push_back(std::tolower(static_cast<unsigned char>(c)));
                                if (lower == "lfo1" || lower == "lfo 1") return "LFO1";
                                if (lower == "lfo2" || lower == "lfo 2") return "LFO2";
                                if (lower == "modwheel" || lower == "mod wheel") return "ModWheel";
                                if (lower == "aftertouch") return "Aftertouch";
                                if (lower == "velocity") return "Velocity";
                                if (lower == "envelope" || lower == "env") return "Envelope";
                                if (lower == "none" || lower.empty() || lower == "noen") return "None";
                                return s;
                            };
                            std::string engineSrc = toEngineSource(src);
                            // Apply to engine (skip when None)
                            if (engineSrc != "None") {
                                synthesizer->disconnectModulation(engineSrc, resolvedDest);
                                synthesizer->connectModulation(engineSrc, resolvedDest, amt);
                            }
                            // Debug: show resolution mapping briefly
                            if (auto* screen = uiContext->getScreen("main")) {
                                if (auto* dbg = dynamic_cast<Label*>(screen->getChild("preset_debug"))) {
                                    std::stringstream ss;
                                    ss << "Mod " << (i+1) << ": " << toDisplaySource(src) << " -> " << resolvedDest << " (" << dstId << ")";
                                    dbg->setText(ss.str());
                                }
                            }
                            // Reflect in UI controls + debug to verify indices/text
                            if (i < modSourceDropdowns.size() && modSourceDropdowns[i]) {
                                // Canonicalize source name then select deterministically
                                std::string sName = modConnections[i].source;
                                if (sName == "LFO1" || sName == "lfo1" || sName == "LFO 1" || sName == "lfo 1") sName = "LFO 1";
                                else if (sName == "LFO2" || sName == "lfo2" || sName == "LFO 2" || sName == "lfo 2") sName = "LFO 2";
                                else if (sName == "ModWheel" || sName == "modwheel" || sName == "mod wheel") sName = "Mod Wheel";
                                else if (sName == "Envelope" || sName == "envelope" || sName == "env") sName = "Envelope";
                                else if (sName == "Velocity" || sName == "velocity") sName = "Velocity";
                                else if (sName == "Aftertouch" || sName == "aftertouch") sName = "Aftertouch";
                                else if (sName == "None" || sName == "none") sName = "None";
                                modSourceDropdowns[i]->selectItemSilently(sName);
                                int sidx = modSourceDropdowns[i]->getSelectedIndex();
                                // Final safety: if non-None and still unknown, set to 1 to enable gating
                                modConnections[i].sourceIndex = (sidx >= 0) ? sidx : ((sName == "None") ? 0 : 1);
                                std::cout << "[PresetLoad] Row " << i << " set source '" << modConnections[i].source
                                          << "' (index=" << modConnections[i].sourceIndex << ")" << std::endl;
                            } else if (auto* srcDd = dynamic_cast<DropdownMenu*>(uiContext->getScreen("main")->getChild("mod_source_" + std::to_string(i)))) {
                                std::string sName = modConnections[i].source;
                                if (sName == "LFO1" || sName == "lfo1" || sName == "LFO 1" || sName == "lfo 1") sName = "LFO 1";
                                else if (sName == "LFO2" || sName == "lfo2" || sName == "LFO 2" || sName == "lfo 2") sName = "LFO 2";
                                else if (sName == "ModWheel" || sName == "modwheel" || sName == "mod wheel") sName = "Mod Wheel";
                                else if (sName == "Envelope" || sName == "envelope" || sName == "env") sName = "Envelope";
                                else if (sName == "Velocity" || sName == "velocity") sName = "Velocity";
                                else if (sName == "Aftertouch" || sName == "aftertouch") sName = "Aftertouch";
                                else if (sName == "None" || sName == "none") sName = "None";
                                srcDd->selectItemSilently(sName);
                                int sidx = srcDd->getSelectedIndex();
                                modConnections[i].sourceIndex = (sidx >= 0) ? sidx : ((sName == "None") ? 0 : 1);
                                std::cout << "[PresetLoad] Row " << i << " fallback src set to '" << modConnections[i].source
                                          << "' (index=" << modConnections[i].sourceIndex << ")" << std::endl;
                            }
                            if (i < modDestDropdowns.size() && modDestDropdowns[i]) {
                                modDestDropdowns[i]->selectItemSilently(modConnections[i].destination);
                                int didx = modDestDropdowns[i]->getSelectedIndex();
                                if (didx < 0 && modConnections[i].destination != "None") {
                                    // Try resolved, human-readable destination as a fallback
                                    modDestDropdowns[i]->selectItemSilently(resolvedDest);
                                    didx = modDestDropdowns[i]->getSelectedIndex();
                                }
                                // Final safety: ensure non-zero when destination is not "None" to enable gating
                                modConnections[i].destIndex = (didx >= 0) ? didx : (modConnections[i].destination == "None" ? 0 : 1);
                                std::cout << "[PresetLoad] Row " << i << " set dest '" << modConnections[i].destination
                                          << "' (index=" << modConnections[i].destIndex << ")" << std::endl;
                            } else if (auto* dstDd = dynamic_cast<DropdownMenu*>(uiContext->getScreen("main")->getChild("mod_dest_" + std::to_string(i)))) {
                                dstDd->selectItemSilently(modConnections[i].destination);
                                int didx = dstDd->getSelectedIndex();
                                if (didx < 0 && modConnections[i].destination != "None") {
                                    dstDd->selectItemSilently(resolvedDest);
                                    didx = dstDd->getSelectedIndex();
                                }
                                modConnections[i].destIndex = (didx >= 0) ? didx : (modConnections[i].destination == "None" ? 0 : 1);
                                std::cout << "[PresetLoad] Row " << i << " fallback dest set to '" << modConnections[i].destination
                                          << "' (index=" << modConnections[i].destIndex << ")" << std::endl;
                            }
                            if (i < modAmountSliders.size() && modAmountSliders[i]) {
                                modAmountSliders[i]->setValue(modConnections[i].amount);
                                std::cout << "[PresetLoad] Row " << i << " set amount=" << modConnections[i].amount << std::endl;
                            }
                        }
                    }
                    std::cout << "Preset loaded (.preset): " << selectedPreset.name << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error loading .preset: " << e.what() << std::endl;
                }
                return; // handled
            }
            // Fallback: older preset format
            if (presetManager->loadPreset(selectedPreset.fullPath)) {
                std::cout << "Successfully loaded preset: " << selectedPreset.name << std::endl;
                // Update top-level UI sliders for core synth params + envelope
                if (waveSliderPtr) waveSliderPtr->setValue(synthesizer->getParameter("oscillator_type"));
                if (cutoffSliderPtr) cutoffSliderPtr->setValue(synthesizer->getParameter("filter_cutoff"));
                if (resSliderPtr) resSliderPtr->setValue(synthesizer->getParameter("filter_resonance"));
                if (volumeSliderPtr) volumeSliderPtr->setValue(synthesizer->getParameter("master_volume"));
                if (attackSliderPtr) attackSliderPtr->setValue(synthesizer->getParameter("envelope_attack"));
                if (decaySliderPtr) decaySliderPtr->setValue(synthesizer->getParameter("envelope_decay"));
                if (sustainSliderPtr) sustainSliderPtr->setValue(synthesizer->getParameter("envelope_sustain"));
                if (releaseSliderPtr) releaseSliderPtr->setValue(synthesizer->getParameter("envelope_release"));
                if (envelopePtr) envelopePtr->setADSR(
                    synthesizer->getParameter("envelope_attack"),
                    synthesizer->getParameter("envelope_decay"),
                    synthesizer->getParameter("envelope_sustain"),
                    synthesizer->getParameter("envelope_release")
                );
                // Start a brief ramp on legacy format too
                synthesizer->startPresetApplyRamp(0.02f);
            } else {
                std::cerr << "Failed to load preset: " << selectedPreset.name << std::endl;
            }
        });
    }

    // Set up MIDI handling
    midiInput->setCallback(midiHandler.get());
    
    midiHandler->setNoteOnCallback([&, midiKeyboardPtr](int channel, int note, int velocity) {
        float normalizedVelocity = velocity / 127.0f;
        synthesizer->noteOn(note, normalizedVelocity);
        
        // Display external MIDI input on keyboard
        if (midiKeyboardPtr) {
            midiKeyboardPtr->setNotePressed(note, true, velocity);
        }
        
        std::cout << "MIDI Note On: " << MidiKeyboard::getNoteName(note) 
                  << " (note " << note << ") velocity " << velocity << std::endl;
    });
    
    midiHandler->setNoteOffCallback([&, midiKeyboardPtr](int channel, int note) {
        synthesizer->noteOff(note);
        
        // Update keyboard display
        if (midiKeyboardPtr) {
            midiKeyboardPtr->setNotePressed(note, false, 0);
        }
        
        std::cout << "MIDI Note Off: " << MidiKeyboard::getNoteName(note) 
                  << " (note " << note << ")" << std::endl;
    });
    
    // Set up MIDI CC processing for CC learning
    midiHandler->setControlChangeCallback([&](int channel, int ccNumber, int value) {
        ccLearning.getLearning().processMidiCC(channel, ccNumber, value, "MIDI Input");
    });
    // Blink activity light on any MIDI message
    midiHandler->setGenericCallback([&](const MidiMessage& /*message*/) {
        midiActivityPulse.store(true, std::memory_order_relaxed);
    });
    
    // Set up sequencer callbacks
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel, const Envelope& env) {
            synthesizer->noteOn(pitch, velocity);
            midiOutput->sendNoteOn(channel, pitch, static_cast<int>(velocity * 127.0f));
        },
        [&](int pitch, int channel) {
            synthesizer->noteOff(pitch);
            midiOutput->sendNoteOff(channel, pitch);
        }
    );
    
    // Set up audio callback (mutex declared earlier, reuse here)
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        std::lock_guard<std::mutex> lock(audioMutex);
        audioCallback(audioEngine.get(), synthesizer.get(), effectProcessor.get(),
                     sequencer.get(), waveformPtr, levelPtr, outputBuffer, numFrames);
    });
    
    // Deferred apply: if synth settings were loaded, re-apply once after UI has fully initialized
    bool needsDeferredSynthApply = loadedConfig.contains("synth") && loadedConfig["synth"].is_object();
    nlohmann::json deferredSynthConfig = needsDeferredSynthApply ? loadedConfig["synth"] : nlohmann::json{};

    // Apply persisted Hybrid toggle after UI creation
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        synthesizer->setHybridWavetableEnabled(persistedHybridEnabled);
    }
    if (auto* settings = uiContext->getScreen("settings")) {
        if (auto* lbl = dynamic_cast<Label*>(settings->getChild("engine_status"))) {
            lbl->setText(persistedHybridEnabled ? "Engine: Hybrid (Spectral)" : "Engine: Legacy/Realtime WT");
        }
        if (auto* btn = dynamic_cast<Button*>(settings->getChild("hybrid_toggle"))) {
            btn->setText(persistedHybridEnabled ? "Engine: Hybrid (click to switch)" : "Engine: Legacy (click to switch)");
        }
        if (auto* tbtn = dynamic_cast<Button*>(settings->getChild("timbre_mode_toggle"))) {
            synthesizer->setHybridTimbreMinPhase(persistedMinPhase);
            tbtn->setText(persistedMinPhase ? "Timbre: Min-Phase" : "Timbre: Linear");
        }
    }

    // Main loop
    bool running = true;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto lastPerfUpdate = std::chrono::high_resolution_clock::now();
    auto lastAudioCheck = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    float cpuUsage = 0.0f;
    
    std::cout << "Starting main loop..." << std::endl;
    
    while (running) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Process SDL events
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            if (sdlEvent.type == SDL_QUIT) {
                std::cout << "Got SDL_QUIT event" << std::endl;
                running = false;
            } else if (sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                std::cout << "Got ESC key" << std::endl;
                running = false;
            } else if (sdlEvent.type == SDL_KEYDOWN && (sdlEvent.key.keysym.mod & KMOD_SHIFT) && sdlEvent.key.keysym.sym == SDLK_h) {
                // Hidden toggle: Shift+H enables/disables hybrid wavetable engine
                static bool hybridEnabled = false;
                hybridEnabled = !hybridEnabled;
                {
                    // Protect audio graph while rebuilding voices to avoid race with audio callback
                    std::lock_guard<std::mutex> lock(audioMutex);
                    synthesizer->setHybridWavetableEnabled(hybridEnabled);
                }
                std::cout << (hybridEnabled ? "Hybrid Wavetable: ON" : "Hybrid Wavetable: OFF") << std::endl;
                // Update settings screen label if visible (read back from synth to reflect actual state)
                if (auto* settings = uiContext->getScreen("settings")) {
                    if (auto* lbl = dynamic_cast<Label*>(settings->getChild("engine_status"))) {
                        bool actual = synthesizer->isHybridWavetableEnabled();
                        lbl->setText(actual ? "Engine: Hybrid (Spectral)" : "Engine: Legacy/Realtime WT");
                    }
                }
            } else {
                InputEvent inputEvent = translateSDLEvent(sdlEvent);
                
                // Check if any dropdown is open and handle it first
                bool dropdownHandled = false;
                Screen* activeScreen = uiContext ? uiContext->getScreen(uiContext->getActiveScreenId()) : nullptr;
                if (activeScreen) {
                    // Check all dropdowns in reverse order (last added = top-most)
                    // LFO selector dropdown
                    if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("lfo_selector"))) {
                        if (dropdown->isDropdownOpen()) {
                            dropdownHandled = dropdown->handleInput(inputEvent);
                            if (inputEvent.type == InputEventType::TouchPress && !dropdownHandled) {
                                // Click outside dropdown - close it but consume the event
                                dropdownHandled = true;
                            }
                        }
                    }
                    
                    // MIDI device dropdown
                    if (!dropdownHandled) {
                        if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("midi_device_selector"))) {
                            if (dropdown->isDropdownOpen()) {
                                dropdownHandled = dropdown->handleInput(inputEvent);
                                if (inputEvent.type == InputEventType::TouchPress && !dropdownHandled) {
                                    // Click outside dropdown - close it but consume the event
                                    dropdownHandled = true;
                                }
                            }
                        }
                    }
                    
                    // Preset dropdown
                    if (!dropdownHandled) {
                        if (auto* dropdown = dynamic_cast<PresetDropdown*>(activeScreen->getChild("preset_dropdown"))) {
                            if (dropdown->isDropdownOpen()) {
                                dropdownHandled = dropdown->handleInput(inputEvent);
                                if (inputEvent.type == InputEventType::TouchPress && !dropdownHandled) {
                                    dropdownHandled = true;
                                }
                            }
                        }
                    }
                    
                    // Effects tab: multi-slot effect type dropdowns (handle in reverse order)
                    if (!dropdownHandled) {
                        for (int i = fxSlotCount - 1; i >= 0; --i) {
                            if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("fx_type_" + std::to_string(i)))) {
                                if (dropdown->isDropdownOpen()) {
                                    dropdownHandled = dropdown->handleInput(inputEvent);
                                    if (inputEvent.type == InputEventType::TouchPress && !dropdownHandled) {
                                        dropdownHandled = true; // consume to avoid underlying toggles
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    // Effects tab: per-slot page dropdowns (handle in reverse order)
                    if (!dropdownHandled) {
                        for (int i = fxSlotCount - 1; i >= 0; --i) {
                            if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("fx_page_" + std::to_string(i)))) {
                                if (dropdown->isDropdownOpen()) {
                                    dropdownHandled = dropdown->handleInput(inputEvent);
                                    if (inputEvent.type == InputEventType::TouchPress && !dropdownHandled) {
                                        dropdownHandled = true;
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    // Main-screen quick FX dropdowns: handle open lists before other inputs
                    if (!dropdownHandled) {
                        for (int i = 2; i >= 0; --i) {
                            if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("effect_type_" + std::to_string(i)))) {
                                if (dropdown->isDropdownOpen()) {
                                    dropdownHandled = dropdown->handleInput(inputEvent);
                                    if (inputEvent.type == InputEventType::TouchPress && !dropdownHandled) {
                                        dropdownHandled = true;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    
                    // Modulation dropdowns
                    if (!dropdownHandled) {
                        for (int i = 2; i >= 0; --i) {  // Check in reverse order
                            bool found = false;
                            if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("mod_dest_" + std::to_string(i)))) {
                                if (dropdown->isDropdownOpen()) {
                                    dropdownHandled = dropdown->handleInput(inputEvent);
                                    if (inputEvent.type == InputEventType::TouchPress && !dropdownHandled) {
                                        dropdownHandled = true;
                                    }
                                    found = true;
                                }
                            }
                            if (!found && !dropdownHandled) {
                                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("mod_source_" + std::to_string(i)))) {
                                    if (dropdown->isDropdownOpen()) {
                                        dropdownHandled = dropdown->handleInput(inputEvent);
                                        if (inputEvent.type == InputEventType::TouchPress && !dropdownHandled) {
                                            dropdownHandled = true;
                                        }
                                        found = true;
                                    }
                                }
                            }
                            if (found) break;
                        }
                    }
                }
                
                // Only pass to UI context if no dropdown handled it
                if (!dropdownHandled) {
                    uiContext->handleInput(inputEvent);
                }
            }
        }
        
        // Update UI
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        
        uiContext->update(deltaTime);

        // Run deferred synth apply exactly once a short time after startup to ensure callbacks are connected
        if (needsDeferredSynthApply) {
            // Re-apply cutoff and resonance to ensure persistence
            try {
                if (deferredSynthConfig.contains("filter_cutoff")) {
                    float v = deferredSynthConfig["filter_cutoff"].get<float>();
                    if (cutoffSliderPtr) cutoffSliderPtr->setValue(v);
                    synthesizer->setParameter("filter_cutoff", v);
                    if (filterVizPtr) {
                        float frequencyHz = 20.0f * std::pow(1000.0f, std::max(0.0f, std::min(1.0f, v)));
                        filterVizPtr->setCutoffFrequency(frequencyHz);
                    }
                }
                if (deferredSynthConfig.contains("filter_resonance")) {
                    float v = deferredSynthConfig["filter_resonance"].get<float>();
                    if (resSliderPtr) resSliderPtr->setValue(v);
                    synthesizer->setParameter("filter_resonance", v);
                    if (filterVizPtr) {
                        float resonanceQ = 0.7f + std::max(0.0f, std::min(1.0f, v)) * 9.3f;
                        filterVizPtr->setResonance(resonanceQ);
                    }
                }
            } catch (...) {}
            needsDeferredSynthApply = false;
        }

        // Update MIDI activity indicator
        if (midiActivityPulse.exchange(false, std::memory_order_relaxed)) {
            midiActivityTimer = 0.20f; // 200 ms blink on activity
        }
        if (midiActivityTimer > 0.0f) {
            midiActivityTimer -= deltaTime;
            if (uiContext) if (auto* activeScreen = uiContext->getScreen("main")) {
                if (auto* light = dynamic_cast<Button*>(activeScreen->getChild("midi_activity_light"))) {
                    light->setBackgroundColor(Color(0, 200, 80));
                }
            }
        } else {
            if (uiContext) if (auto* activeScreen = uiContext->getScreen("main")) {
                if (auto* light = dynamic_cast<Button*>(activeScreen->getChild("midi_activity_light"))) {
                    light->setBackgroundColor(Color(40, 60, 40));
                }
            }
        }
        
        // Update performance info every second
        frameCount++;
        if (std::chrono::duration<float>(currentTime - lastPerfUpdate).count() > 1.0f) {
            float fps = frameCount / std::chrono::duration<float>(currentTime - lastPerfUpdate).count();
            
            // Performance tracking is now just for internal use
            // Could be displayed in a different tab later
            std::stringstream perfText;
            perfText << "CPU: " << std::fixed << std::setprecision(1) 
                    << cpuUsage << "% | FPS: " 
                    << static_cast<int>(fps) << " | Audio: OK";

            // Update hybrid stats label on Settings
            if (uiContext) if (auto* settings = uiContext->getScreen("settings")) {
                if (auto* statLbl = dynamic_cast<Label*>(settings->getChild("hybrid_stats"))) {
                    uint64_t h = synthesizer ? synthesizer->hybridCacheHits() : 0;
                    uint64_t m = synthesizer ? synthesizer->hybridCacheMisses() : 0;
                    size_t q = synthesizer ? synthesizer->hybridQueueSize() : 0;
                    size_t f = synthesizer ? synthesizer->hybridInFlight() : 0;
                    std::stringstream s;
                    s << "Hybrid: cache " << h << "/" << (h + m) << ", Q " << q << ", in-flight " << f;
                    statLbl->setText(s.str());
                }
            }
            
            frameCount = 0;
            lastPerfUpdate = currentTime;
        }
        
        // Check audio stream status every second
        if (std::chrono::duration<float>(currentTime - lastAudioCheck).count() > 1.0f) {
            bool streamRunning = audioEngine->isStreamRunning();
            bool engineHealthy = audioEngine->isHealthy();
            
            static bool lastStreamState = true;
            static int retryCount = 0;
            
            // Detect when stream stops
            if (!streamRunning && lastStreamState) {
                std::cout << "\n!!! Audio stream stopped (device disconnected?) - attempting recovery..." << std::endl;
                lastStreamState = false;
                retryCount = 0;
            }
            
            // Try to recover if stream is not running
            if (!streamRunning && retryCount < 10) {
                std::cout << "Recovery attempt " << (retryCount + 1) << "/10..." << std::endl;
                
                // Stop the current audio engine completely
                std::cout << "  - Shutting down audio engine..." << std::endl;
                audioEngine->shutdown();
                
                // Wait for device to settle
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                
                // Create a new audio engine instance
                std::cout << "  - Creating new audio engine..." << std::endl;
                audioEngine = std::make_unique<AudioEngine>();
                
                // Try to initialize with default device
                if (audioEngine->initialize()) {
                    std::cout << "  - Audio engine initialized!" << std::endl;
                    
                    // Re-set the audio callback
                    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
                        std::lock_guard<std::mutex> lock(audioMutex);
                        audioCallback(audioEngine.get(), synthesizer.get(), effectProcessor.get(),
                                     sequencer.get(), waveformPtr, levelPtr, outputBuffer, numFrames);
                    });
                    
                    if (audioEngine->isStreamRunning()) {
                        std::cout << "✓ Audio recovery successful! Stream is running." << std::endl;
                        lastStreamState = true;
                        retryCount = 0;
                        
                        // Update any components that need the new sample rate
                        if (synthesizer) {
                            // Re-initialize synthesizer sample rate if needed
                            std::cout << "  - Audio sample rate: " << audioEngine->getSampleRate() << " Hz" << std::endl;
                        }
                    } else {
                        std::cout << "✗ Stream failed to start" << std::endl;
                        retryCount++;
                    }
                } else {
                    std::cerr << "✗ Failed to initialize audio engine" << std::endl;
                    retryCount++;
                    
                    // Wait longer before next retry
                    if (retryCount < 10) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                }
            } else if (streamRunning && !lastStreamState) {
                // Stream recovered
                std::cout << "✓ Audio stream is running again!" << std::endl;
                lastStreamState = true;
                retryCount = 0;
            }
            
            lastAudioCheck = currentTime;
        }
        
        // Let UIContext render, but don't let it swap buffers
        // First, save the current render state
        Screen* activeScreen = nullptr;
        if (uiContext) {
            activeScreen = uiContext->getScreen(uiContext->getActiveScreenId());
        }
        if (activeScreen) {
            sdlDisplayManager->clear(activeScreen->getBackgroundColor());
            activeScreen->render(sdlDisplayManager.get());
            
            // Render dropdown lists on top of everything else
            // This ensures dropdowns appear above all other components
            
            // Check modulation source dropdowns
            for (int i = 0; i < 3; ++i) {
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("mod_source_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("mod_dest_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
            }
            
            // Check effect dropdowns
            for (int i = 0; i < 3; ++i) {
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("effect_type_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
            }
            
            // Check preset dropdown
            if (auto* dropdown = dynamic_cast<PresetDropdown*>(activeScreen->getChild("preset_dropdown"))) {
                if (dropdown->isDropdownOpen()) {
                    dropdown->renderDropdownList(sdlDisplayManager.get());
                }
            }
            
            // Check LFO selector dropdown
            if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("lfo_selector"))) {
                if (dropdown->isDropdownOpen()) {
                    dropdown->renderDropdownList(sdlDisplayManager.get());
                }
            }
            
            // Effects tab: effect type dropdowns (multi-slot)
            for (int i = 0; i < fxSlotCount; ++i) {
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("fx_type_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
            }
            // Effects tab: page dropdowns (multi-slot)
            for (int i = 0; i < fxSlotCount; ++i) {
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("fx_page_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
            }
            // Main-screen quick FX dropdowns
            for (int i = 0; i < 3; ++i) {
                if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("effect_type_" + std::to_string(i)))) {
                    if (dropdown->isDropdownOpen()) {
                        dropdown->renderDropdownList(sdlDisplayManager.get());
                    }
                }
            }

            // Check MIDI device dropdown
            if (auto* dropdown = dynamic_cast<DropdownMenu*>(activeScreen->getChild("midi_device_selector"))) {
                if (dropdown->isDropdownOpen()) {
                    dropdown->renderDropdownList(sdlDisplayManager.get());
                }
            }
        }
        
        // Present the frame
        SDL_RenderPresent(renderer);
        
        // Frame rate limiting and CPU usage calculation
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        
        cpuUsage = (frameDuration / 16.67f) * 100.0f; // Percentage of 60 FPS frame time
        
        if (frameDuration < 16.67f) { // Target 60 FPS
            SDL_Delay(static_cast<Uint32>(16.67f - frameDuration));
        }
    }
    
    // Cleanup - Order is critical to prevent crashes
    std::cout << "AI Music Hardware - Shutting down..." << std::endl;
    
    // 1. First stop audio engine to prevent callbacks during shutdown
    std::cout << "Stopping audio engine..." << std::endl;
    if (audioEngine) {
        audioEngine->shutdown();
    }
    
    // 2. Send all notes off (only if MIDI output is open)
    if (midiOutput && midiOutput->isDeviceOpen()) {
        std::cout << "Sending all notes off..." << std::endl;
        try {
            for (int ch = 0; ch < 16; ++ch) {
                for (int note = 0; note < 128; ++note) {
                    midiOutput->sendNoteOff(ch, note);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error sending notes off: " << e.what() << std::endl;
        }
    }
    
    // 3. Clear all UI connections before shutting down UI
    std::cout << "Clearing UI connections..." << std::endl;
    if (uiContext) {
        uiContext->connectSynthesizer(nullptr);
        uiContext->connectEffectProcessor(nullptr);
        uiContext->connectSequencer(nullptr);
        uiContext->connectHardwareInterface(nullptr);
        uiContext->connectAdaptiveSequencer(nullptr);
        uiContext->connectLLMInterface(nullptr);
    }
    
    // 4. Shutdown UI context first (before destroying display manager)
    std::cout << "Shutting down UI..." << std::endl;
    if (uiContext) {
        uiContext->shutdown();
        uiContext.reset();
    }
    
    // 5. Reset display manager before destroying SDL renderer
    std::cout << "Resetting display manager..." << std::endl;
    sdlDisplayManager.reset();
    
    // 6. Stop hardware interface
            std::cout << "Stopping hardware interface..." << std::endl;
    if (hardwareInterface) {
        hardwareInterface->shutdown();
    }
    
        // Save user configuration (MIDI + Effects + Synth)
    try {
        nlohmann::json outCfg;
        outCfg["midi"]["deviceName"] = persistedMidiDeviceName;
        // Effects
        nlohmann::json slots = nlohmann::json::array();
        for (int s = 0; s < fxSlotCount; ++s) {
            nlohmann::json sj;
            sj["type"] = slotSelectedType[s];
            sj["mix"] = slotMix[s];
            sj["enabled"] = slotEnabled[s];
            sj["page"] = std::max(0, std::min(1, slotPage[s]));
            // Save params for current type if available
            if (slotParamCache[s].count(slotSelectedType[s])) {
                nlohmann::json pj;
                for (const auto& kv : slotParamCache[s][slotSelectedType[s]]) {
                    pj[kv.first] = kv.second;
                }
                sj["params"] = pj;
            }
            slots.push_back(sj);
        }
        outCfg["effects"]["slots"] = slots;
        // Modulation routing
        {
            nlohmann::json mods = nlohmann::json::array();
            for (int i = 0; i < 3; ++i) {
                nlohmann::json mj;
                // Persist using cached connection data (avoid touching UI during shutdown)
                std::string sourceText = modConnections[i].source.empty() ? "None" : modConnections[i].source;
                std::string destText = modConnections[i].destination.empty() ? "None" : modConnections[i].destination;
                std::string sourceInternal = sourceText;
                if (sourceInternal == "LFO 1") sourceInternal = "LFO1";
                else if (sourceInternal == "LFO 2") sourceInternal = "LFO2";
                else if (sourceInternal == "Mod Wheel") sourceInternal = "ModWheel";
                mj["source"] = sourceInternal;
                mj["destination"] = destText;
                mj["amount"] = modConnections[i].amount;
                mj["sourceIndex"] = std::max(0, modConnections[i].sourceIndex);
                mj["destIndex"] = std::max(0, modConnections[i].destIndex);
                mods.push_back(mj);
            }
            outCfg["mod_routing"] = mods;
        }
        // Engine settings
        outCfg["engine"]["hybrid_enabled"] = synthesizer && synthesizer->isHybridWavetableEnabled();
        outCfg["engine"]["timbre_min_phase"] = synthesizer && synthesizer->isHybridTimbreMinPhase();

        // Synth params
        nlohmann::json sj;
        if (synthesizer) sj["oscillator_type"] = synthesizer->getParameter("oscillator_type");
        // Always read from synthesizer (UI may be shut down here)
        if (synthesizer) {
            sj["filter_cutoff"] = synthesizer->getParameter("filter_cutoff");
            sj["filter_resonance"] = synthesizer->getParameter("filter_resonance");
            sj["master_volume"] = synthesizer->getParameter("master_volume");
            sj["envelope_attack"] = synthesizer->getParameter("envelope_attack");
            sj["envelope_decay"] = synthesizer->getParameter("envelope_decay");
            sj["envelope_sustain"] = synthesizer->getParameter("envelope_sustain");
            sj["envelope_release"] = synthesizer->getParameter("envelope_release");
            sj["lfo1_rate"] = synthesizer->getParameter("lfo1_rate");
            sj["lfo1_depth"] = synthesizer->getParameter("lfo1_depth");
            sj["lfo1_shape"] = synthesizer->getParameter("lfo1_shape");
            sj["lfo2_rate"] = synthesizer->getParameter("lfo2_rate");
            sj["lfo2_depth"] = synthesizer->getParameter("lfo2_depth");
            sj["lfo2_shape"] = synthesizer->getParameter("lfo2_shape");
        }
        outCfg["synth"] = sj;
        std::ofstream out(userConfigPath);
        out << outCfg.dump(2);
        out.flush();
        std::cout << "Saved user config to: " << userConfigPath << std::endl;
        if (outCfg.contains("synth")) {
            try {
                std::cout << "Saved filter_cutoff: " << outCfg["synth"]["filter_cutoff"].get<float>()
                          << ", filter_resonance: " << outCfg["synth"]["filter_resonance"].get<float>() << std::endl;
            } catch (...) {}
        }
    } catch (...) {
        // ignore save errors
    }

    // 7. Cleanup SDL in correct order: renderer first, then window, then SDL
    std::cout << "Cleaning up SDL..." << std::endl;
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
    
    // 8. Force destruction of remaining components in safe order
    std::cout << "Destroying audio components..." << std::endl;
    midiHandler.reset();
    midiOutput.reset();
    midiInput.reset();
    sequencer.reset();
    effectProcessor.reset();
    synthesizer.reset();
    audioEngine.reset();
    hardwareInterface.reset();
    
    std::cout << "Shutdown complete." << std::endl;
    
    return 0;
}
