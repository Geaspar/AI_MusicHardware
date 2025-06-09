#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <iomanip>
#include <sstream>
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

// Enhanced UI system
#include "../include/ui/UIContext.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/PresetBrowserUIComponent.h"
#include "../include/ui/VisualizationComponents.h"
#include "../include/ui/ParameterUpdateQueue.h"
#include "../include/ui/MidiKeyboard.h"
#include "../include/ui/GridLayout.h"
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"
#include "../include/midi/MidiCCLearning.h"

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
        drawTextWithSize(x, y, text, color, TextSize::Normal);
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

// Audio processing callback
void audioCallback(AudioEngine* audioEngine, Synthesizer* synthesizer, 
                  EffectProcessor* effectProcessor, Sequencer* sequencer,
                  WaveformVisualizer* waveform, LevelMeter* levelMeter,
                  float* outputBuffer, int numFrames) {
    
    // Process sequencer
    sequencer->process(static_cast<float>(numFrames) / audioEngine->getSampleRate());
    
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
    std::cout << "AI Music Hardware - Grid Layout Version" << std::endl;
    std::cout << "Starting synthesizer with organized grid layout..." << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create SDL window
    SDL_Window* window = SDL_CreateWindow(
        "AI Music Hardware - Grid Layout",
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
    auto effectProcessor = std::make_unique<EffectProcessor>();
    auto sequencer = std::make_unique<Sequencer>();
    auto midiInput = std::make_unique<MidiInput>();
    auto midiOutput = std::make_unique<MidiOutput>();
    auto midiHandler = std::make_unique<MidiHandler>();
    auto hardwareInterface = std::make_unique<HardwareInterface>();
    
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
    
    if (!audioEngine->initialize()) {
        std::cerr << "Failed to initialize audio engine!" << std::endl;
        return 1;
    }
    
    // Create UI context with SDL display manager
    auto uiContext = std::make_unique<UIContext>();
    auto sdlDisplayManager = std::make_shared<SDLDisplayManager>(renderer);
    uiContext->setDisplayManager(sdlDisplayManager);
    uiContext->initialize(1280, 800);
    
    // Initialize parameter manager
    auto& paramManager = EnhancedParameterManager::getInstance();
    paramManager.connectSynthesizer(synthesizer.get());
    
    // Initialize MIDI CC Learning system
    auto& ccLearning = MidiCCLearningManager::getInstance();
    ccLearning.initialize();
    
    // Create parameter mapping storage for CC learning integration
    std::map<std::string, SynthKnob*> parameterKnobs;
    
    // Create main synthesizer screen with grid layout
    auto mainScreen = std::make_unique<Screen>("main");
    mainScreen->setBackgroundColor(Color(40, 40, 50));
    mainScreen->setPosition(0, 0);
    mainScreen->setSize(1280, 800);
    
    // Create the main grid layout (8x6 grid)
    auto mainGrid = std::make_unique<GridLayout>("main_grid", 6, 8);
    mainGrid->setPosition(0, 0);
    mainGrid->setSize(1280, 800);
    mainGrid->setPadding(20);
    mainGrid->setSpacing(10, 10);
    
    // Title - spans full width
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware - Grid Layout");
    titleLabel->setTextColor(Color(200, 220, 255));
    mainGrid->addComponent(std::move(titleLabel), 0, 0, 1, 8);
    
    // OSCILLATOR SECTION (row 1, cols 0-1)
    auto oscContainer = std::make_unique<GridLayout>("osc_grid", 2, 2);
    
    auto oscSection = std::make_unique<Label>("osc_section", "OSCILLATOR");
    oscSection->setTextColor(Color(255, 255, 100));
    oscContainer->addComponent(std::move(oscSection), 0, 0, 1, 2);
    
    auto freqKnob = SynthKnobFactory::createFrequencyKnob("Frequency", 0, 0, 80);
    freqKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " Hz";
        return ss.str();
    });
    SynthKnob* freqKnobPtr = freqKnob.get();
    oscContainer->addComponent(std::move(freqKnob), 1, 0);
    
    auto waveKnob = std::make_unique<SynthKnob>("Wave", 0, 0, 80, 0.0f, 4.0f, 0.0f);
    waveKnob->setValueFormatter([](float value) {
        const char* waveNames[] = {"Sine", "Saw", "Square", "Triangle", "Noise"};
        int index = static_cast<int>(value);
        if (index >= 0 && index < 5) {
            return std::string(waveNames[index]);
        }
        return std::string("Unknown");
    });
    SynthKnob* waveKnobPtr = waveKnob.get();
    oscContainer->addComponent(std::move(waveKnob), 1, 1);
    
    mainGrid->addComponent(std::move(oscContainer), 1, 0, 1, 2);
    
    // FILTER SECTION (row 1, cols 2-3)
    auto filterContainer = std::make_unique<GridLayout>("filter_grid", 2, 2);
    
    auto filterSection = std::make_unique<Label>("filter_section", "FILTER");
    filterSection->setTextColor(Color(100, 255, 100));
    filterContainer->addComponent(std::move(filterSection), 0, 0, 1, 2);
    
    auto cutoffKnob = SynthKnobFactory::createFrequencyKnob("Cutoff", 0, 0, 80);
    cutoffKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value >= 1000.0f) {
            ss << std::fixed << std::setprecision(1) << value / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(0) << value << " Hz";
        }
        return ss.str();
    });
    SynthKnob* cutoffKnobPtr = cutoffKnob.get();
    filterContainer->addComponent(std::move(cutoffKnob), 1, 0);
    
    auto resKnob = SynthKnobFactory::createResonanceKnob("Resonance", 0, 0, 80);
    resKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    SynthKnob* resKnobPtr = resKnob.get();
    filterContainer->addComponent(std::move(resKnob), 1, 1);
    
    mainGrid->addComponent(std::move(filterContainer), 1, 2, 1, 2);
    
    // ENVELOPE SECTION (row 1, cols 4-6)
    auto envContainer = std::make_unique<GridLayout>("env_grid", 2, 4);
    
    auto envSection = std::make_unique<Label>("env_section", "ENVELOPE");
    envSection->setTextColor(Color(255, 100, 255));
    envContainer->addComponent(std::move(envSection), 0, 0, 1, 4);
    
    auto attackKnob = SynthKnobFactory::createTimeKnob("Attack", 0, 0, 60, 2.0f);
    auto decayKnob = SynthKnobFactory::createTimeKnob("Decay", 0, 0, 60, 2.0f);
    auto sustainKnob = SynthKnobFactory::createVolumeKnob("Sustain", 0, 0, 60);
    auto releaseKnob = SynthKnobFactory::createTimeKnob("Release", 0, 0, 60, 4.0f);
    
    envContainer->addComponent(std::move(attackKnob), 1, 0);
    envContainer->addComponent(std::move(decayKnob), 1, 1);
    envContainer->addComponent(std::move(sustainKnob), 1, 2);
    envContainer->addComponent(std::move(releaseKnob), 1, 3);
    
    mainGrid->addComponent(std::move(envContainer), 1, 4, 1, 3);
    
    // MASTER SECTION (row 1, col 7)
    auto masterContainer = std::make_unique<GridLayout>("master_grid", 2, 1);
    
    auto masterSection = std::make_unique<Label>("master_section", "MASTER");
    masterSection->setTextColor(Color(100, 200, 255));
    masterContainer->addComponent(std::move(masterSection), 0, 0);
    
    auto volumeKnob = SynthKnobFactory::createVolumeKnob("Volume", 0, 0, 80);
    volumeKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value == 0.0f) {
            ss << "-∞ dB";
        } else {
            float db = 20.0f * std::log10(value);
            ss << std::fixed << std::setprecision(1) << db << " dB";
        }
        return ss.str();
    });
    SynthKnob* volumeKnobPtr = volumeKnob.get();
    masterContainer->addComponent(std::move(volumeKnob), 1, 0);
    
    mainGrid->addComponent(std::move(masterContainer), 1, 7, 1, 1);
    
    // VISUALIZATION SECTION (row 2, spans full width)
    auto vizSection = std::make_unique<Label>("viz_section", "VISUALIZATION");
    vizSection->setTextColor(Color(255, 200, 100));
    mainGrid->addComponent(std::move(vizSection), 2, 0, 1, 2);
    
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setWaveformColor(Color(0, 255, 128));
    mainGrid->addComponent(std::move(waveform), 2, 0, 1, 2);
    
    auto spectrum = std::make_unique<SpectrumAnalyzer>("spectrum", 32);
    mainGrid->addComponent(std::move(spectrum), 2, 2, 1, 2);
    
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    mainGrid->addComponent(std::move(envelope), 2, 4, 1, 2);
    
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    mainGrid->addComponent(std::move(levelMeter), 2, 6, 1, 1);
    
    // MIDI KEYBOARD (row 3-4, cols 0-5)
    auto keyboardSection = std::make_unique<Label>("keyboard_section", "MIDI KEYBOARD");
    keyboardSection->setTextColor(Color(255, 150, 255));
    mainGrid->addComponent(std::move(keyboardSection), 3, 0, 1, 2);
    
    auto midiKeyboard = std::make_unique<MidiKeyboard>("midi_keyboard", 0, 0);
    
    // Configure keyboard
    MidiKeyboard::KeyboardConfig keyboardConfig;
    keyboardConfig.startOctave = 3;
    keyboardConfig.numOctaves = 3;
    keyboardConfig.whiteKeyWidth = 28;
    keyboardConfig.whiteKeyHeight = 120;
    keyboardConfig.blackKeyWidth = 20;
    keyboardConfig.blackKeyHeight = 80;
    
    midiKeyboard->setConfig(keyboardConfig);
    midiKeyboard->setVelocityRange(30, 127);
    
    // Connect keyboard to synthesizer
    midiKeyboard->setNoteCallback([&synthesizer](int note, int velocity, bool isNoteOn) {
        if (isNoteOn) {
            float normalizedVelocity = velocity / 127.0f;
            synthesizer->noteOn(note, normalizedVelocity);
        } else {
            synthesizer->noteOff(note);
        }
    });
    
    MidiKeyboard* midiKeyboardPtr = midiKeyboard.get();
    mainGrid->addComponent(std::move(midiKeyboard), 4, 0, 1, 6);
    
    // Keyboard controls (row 5, cols 0-2)
    auto octaveDownButton = std::make_unique<Button>("octave_down", "OCT-");
    octaveDownButton->setBackgroundColor(Color(80, 80, 100));
    octaveDownButton->setTextColor(Color(255, 255, 255));
    octaveDownButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(-1);
        }
    });
    mainGrid->addComponent(std::move(octaveDownButton), 5, 0);
    
    auto octaveUpButton = std::make_unique<Button>("octave_up", "OCT+");
    octaveUpButton->setBackgroundColor(Color(80, 80, 100));
    octaveUpButton->setTextColor(Color(255, 255, 255));
    octaveUpButton->setClickCallback([midiKeyboardPtr]() {
        if (midiKeyboardPtr) {
            midiKeyboardPtr->transposeOctave(1);
        }
    });
    mainGrid->addComponent(std::move(octaveUpButton), 5, 1);
    
    // PRESET BROWSER (rows 3-5, cols 6-7)
    auto presetSection = std::make_unique<Label>("preset_section", "PRESET BROWSER");
    presetSection->setTextColor(Color(150, 255, 150));
    mainGrid->addComponent(std::move(presetSection), 3, 6, 1, 2);
    
    // Initialize preset system
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    auto presetDatabase = std::make_unique<PresetDatabase>();
    
    // Add some test presets
    PresetInfo preset1;
    preset1.name = "Deep Bass";
    preset1.category = "Bass";
    preset1.author = "System";
    presetDatabase->addPreset(preset1);
    
    PresetInfo preset2;
    preset2.name = "Acid Lead";
    preset2.category = "Lead";
    preset2.author = "System";
    presetDatabase->addPreset(preset2);
    
    auto presetBrowser = std::make_unique<PresetBrowserUI>("preset_browser");
    presetBrowser->initialize(presetManager.get(), presetDatabase.get());
    presetBrowser->setParameterManager(&paramManager);
    mainGrid->addComponent(std::move(presetBrowser), 4, 6, 2, 2);
    
    // Connect knobs to synthesizer parameters
    auto connectKnobToParam = [&](SynthKnob* knob, const std::string& paramId) {
        if (knob) {
            parameterKnobs[paramId] = knob;
            knob->setValueChangeCallback([&synthesizer, paramId](float normalizedValue) {
                synthesizer->setParameter(paramId, normalizedValue);
            });
            float currentValue = synthesizer->getParameter(paramId);
            knob->setValue(currentValue);
        }
    };
    
    connectKnobToParam(waveKnobPtr, "oscillator_type");
    connectKnobToParam(resKnobPtr, "filter_resonance");
    connectKnobToParam(volumeKnobPtr, "master_volume");
    
    // Special handling for filter cutoff
    if (cutoffKnobPtr) {
        parameterKnobs["filter_cutoff"] = cutoffKnobPtr;
        cutoffKnobPtr->setValueChangeCallback([&synthesizer](float frequencyHz) {
            float normalized = (std::log(frequencyHz / 20.0f) / std::log(20000.0f / 20.0f));
            normalized = std::max(0.0f, std::min(1.0f, normalized));
            synthesizer->setParameter("filter_cutoff", normalized);
        });
        cutoffKnobPtr->setValue(1000.0f);
    }
    
    // Add grid to screen
    mainScreen->addChild(std::move(mainGrid));
    
    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    
    // Get pointers to visualization components
    WaveformVisualizer* waveformPtr = 
        static_cast<WaveformVisualizer*>(
            static_cast<GridLayout*>(uiContext->getScreen("main")->getChild("main_grid"))
                ->getChild("waveform"));
    LevelMeter* levelPtr = 
        static_cast<LevelMeter*>(
            static_cast<GridLayout*>(uiContext->getScreen("main")->getChild("main_grid"))
                ->getChild("level"));
    
    // Set up audio callback
    std::mutex audioMutex;
    audioEngine->setAudioCallback([&](float* outputBuffer, int numFrames) {
        std::lock_guard<std::mutex> lock(audioMutex);
        audioCallback(audioEngine.get(), synthesizer.get(), effectProcessor.get(),
                     sequencer.get(), waveformPtr, levelPtr, outputBuffer, numFrames);
    });
    
    // Main loop
    bool running = true;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    std::cout << "Starting main loop..." << std::endl;
    
    while (running) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Process SDL events
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            if (sdlEvent.type == SDL_QUIT) {
                running = false;
            } else if (sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            } else {
                InputEvent inputEvent = translateSDLEvent(sdlEvent);
                uiContext->handleInput(inputEvent);
            }
        }
        
        // Update UI
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;
        
        uiContext->update(deltaTime);
        
        // Render
        Screen* activeScreen = uiContext->getScreen("main");
        if (activeScreen) {
            sdlDisplayManager->clear(activeScreen->getBackgroundColor());
            activeScreen->render(sdlDisplayManager.get());
        }
        
        // Present frame
        SDL_RenderPresent(renderer);
        
        // Frame rate limiting
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        
        if (frameDuration < 16.67f) { // Target 60 FPS
            SDL_Delay(static_cast<Uint32>(16.67f - frameDuration));
        }
    }
    
    // Cleanup
    std::cout << "Shutting down..." << std::endl;
    
    if (audioEngine) {
        audioEngine->shutdown();
    }
    
    if (uiContext) {
        uiContext->shutdown();
    }
    
    sdlDisplayManager.reset();
    
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    
    std::cout << "Shutdown complete." << std::endl;
    
    return 0;
}