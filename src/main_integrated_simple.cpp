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
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"

// IoT support
// #include "../include/iot/DummyIoTInterface.h" // Disabled to avoid crash

using namespace AIMusicHardware;

// Custom SDL DisplayManager for rendering
class SDLDisplayManager : public DisplayManager {
public:
    SDLDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(1280), height_(800) {}
    
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
        if (!renderer_) return;
        // Simplified text rendering - in production, use SDL_ttf
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        int charWidth = 8;
        int charHeight = 14;
        
        for (size_t i = 0; i < text.length(); i++) {
            int charX = x + i * charWidth;
            
            if (text[i] != ' ') {
                // Simple character representation
                if (isalpha(text[i])) {
                    SDL_Rect charRect = { charX, y, charWidth - 1, charHeight };
                    SDL_RenderDrawRect(renderer_, &charRect);
                    
                    // Add distinguishing features for lowercase
                    if (islower(text[i])) {
                        SDL_RenderDrawLine(renderer_, charX, y + charHeight/2, 
                                         charX + charWidth - 2, y + charHeight/2);
                    }
                } else if (isdigit(text[i])) {
                    SDL_Rect digitRect = { charX + 1, y + 2, charWidth - 3, charHeight - 4 };
                    SDL_RenderDrawRect(renderer_, &digitRect);
                } else {
                    // Other characters - draw a simple line
                    SDL_RenderDrawLine(renderer_, charX, y + charHeight/2, 
                                     charX + charWidth - 2, y + charHeight/2);
                }
            }
        }
    }
    
    int getWidth() const override { return width_; }
    int getHeight() const override { return height_; }
    
private:
    SDL_Renderer* renderer_;
    int width_;
    int height_;
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
    
    // Create main synthesizer screen
    auto mainScreen = std::make_unique<Screen>("main");
    mainScreen->setBackgroundColor(Color(40, 40, 50)); // Lighter background
    mainScreen->setPosition(0, 0);
    mainScreen->setSize(1280, 800);
    std::cout << "Created main screen" << std::endl;
    
    // Title
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware - Professional Synthesizer");
    titleLabel->setPosition(400, 10);
    titleLabel->setSize(400, 30); // Set explicit size
    titleLabel->setTextColor(Color(200, 220, 255));
    mainScreen->addChild(std::move(titleLabel));
    
    // Create oscillator section
    auto oscSection = std::make_unique<Label>("osc_section", "OSCILLATOR");
    oscSection->setPosition(50, 50);
    oscSection->setSize(100, 20);
    oscSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(oscSection));
    
    // Add a bright test button to ensure rendering works
    auto testButton = std::make_unique<Button>("test", "TEST BUTTON");
    testButton->setPosition(10, 10);
    testButton->setSize(120, 40);
    testButton->setBackgroundColor(Color(100, 100, 200)); // Bright blue
    testButton->setHighlightColor(Color(150, 150, 255)); // Lighter blue when pressed
    testButton->setTextColor(Color(255, 255, 255));
    mainScreen->addChild(std::move(testButton));
    
    // Create simple knobs for now (without parameter binding until we have parameters)
    auto freqKnob = std::make_unique<SynthKnob>("Frequency", 50, 80, 80, 20.0f, 2000.0f, 440.0f);
    freqKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " Hz";
        return ss.str();
    });
    mainScreen->addChild(std::move(freqKnob));
    
    auto detuneKnob = std::make_unique<SynthKnob>("Detune", 180, 80, 80, -50.0f, 50.0f, 0.0f);
    detuneKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " cents";
        return ss.str();
    });
    mainScreen->addChild(std::move(detuneKnob));
    
    // Create filter section
    auto filterSection = std::make_unique<Label>("filter_section", "FILTER");
    filterSection->setPosition(350, 50);
    filterSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(filterSection));
    
    auto cutoffKnob = std::make_unique<SynthKnob>("Cutoff", 350, 80, 80, 20.0f, 20000.0f, 1000.0f);
    cutoffKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        if (value >= 1000.0f) {
            ss << std::fixed << std::setprecision(1) << value / 1000.0f << " kHz";
        } else {
            ss << std::fixed << std::setprecision(0) << value << " Hz";
        }
        return ss.str();
    });
    mainScreen->addChild(std::move(cutoffKnob));
    
    auto resKnob = std::make_unique<SynthKnob>("Resonance", 480, 80, 80, 0.0f, 1.0f, 0.5f);
    resKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << value * 100.0f << "%";
        return ss.str();
    });
    mainScreen->addChild(std::move(resKnob));
    
    // Create envelope section
    auto envSection = std::make_unique<Label>("env_section", "ENVELOPE");
    envSection->setPosition(650, 50);
    envSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(envSection));
    
    auto attackKnob = std::make_unique<SynthKnob>("Attack", 650, 80, 60, 0.001f, 2.0f, 0.01f);
    auto decayKnob = std::make_unique<SynthKnob>("Decay", 730, 80, 60, 0.001f, 2.0f, 0.1f);
    auto sustainKnob = std::make_unique<SynthKnob>("Sustain", 810, 80, 60, 0.0f, 1.0f, 0.7f);
    auto releaseKnob = std::make_unique<SynthKnob>("Release", 890, 80, 60, 0.001f, 4.0f, 0.5f);
    
    mainScreen->addChild(std::move(attackKnob));
    mainScreen->addChild(std::move(decayKnob));
    mainScreen->addChild(std::move(sustainKnob));
    mainScreen->addChild(std::move(releaseKnob));
    
    // Create visualization section
    auto vizSection = std::make_unique<Label>("viz_section", "VISUALIZATION");
    vizSection->setPosition(50, 200);
    vizSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(vizSection));
    
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setPosition(50, 230);
    waveform->setSize(300, 150);
    waveform->setWaveformColor(Color(0, 255, 128));
    mainScreen->addChild(std::move(waveform));
    
    auto spectrum = std::make_unique<SpectrumAnalyzer>("spectrum", 32);
    spectrum->setPosition(370, 230);
    spectrum->setSize(300, 150);
    mainScreen->addChild(std::move(spectrum));
    
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setPosition(690, 230);
    envelope->setSize(250, 150);
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    mainScreen->addChild(std::move(envelope));
    
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    levelMeter->setPosition(960, 230);
    levelMeter->setSize(30, 150);
    mainScreen->addChild(std::move(levelMeter));
    
    // Create preset browser section
    auto presetSection = std::make_unique<Label>("preset_section", "PRESET BROWSER");
    presetSection->setPosition(50, 410);
    presetSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(presetSection));
    
    // Initialize preset system
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    auto presetDatabase = std::make_unique<PresetDatabase>();
    
    // Add some demo presets
    PresetInfo preset1;
    preset1.name = "Init Patch";
    preset1.category = "Basic";
    preset1.author = "System";
    preset1.description = "Default initialization patch";
    presetDatabase->addPreset(preset1);
    
    PresetInfo preset2;
    preset2.name = "Warm Pad";
    preset2.category = "Pad";
    preset2.author = "Demo";
    preset2.description = "Warm analog-style pad sound";
    presetDatabase->addPreset(preset2);
    
    PresetInfo preset3;
    preset3.name = "Bass Growl";
    preset3.category = "Bass";
    preset3.author = "Demo";
    preset3.description = "Aggressive bass sound with filter modulation";
    presetDatabase->addPreset(preset3);
    
    auto presetBrowser = std::make_unique<PresetBrowserUI>("preset_browser");
    presetBrowser->setPosition(50, 440);
    presetBrowser->setSize(500, 320);
    presetBrowser->initialize(presetManager.get(), presetDatabase.get());
    presetBrowser->setParameterManager(&paramManager);
    
    // Set up preset loading callback
    presetBrowser->setPresetLoadCallback([&](const PresetInfo& preset) {
        std::cout << "Loading preset: " << preset.name << std::endl;
        // In a real implementation, this would load preset parameters
    });
    
    mainScreen->addChild(std::move(presetBrowser));
    
    // Create transport controls
    auto transportSection = std::make_unique<Label>("transport_section", "TRANSPORT");
    transportSection->setPosition(600, 410);
    transportSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(transportSection));
    
    auto playButton = std::make_unique<Button>("play", "PLAY");
    playButton->setPosition(600, 440);
    playButton->setSize(80, 40);
    playButton->setToggleMode(true);
    playButton->setClickCallback([&sequencer]() {
        if (sequencer->isPlaying()) {
            sequencer->stop();
        } else {
            sequencer->start();
        }
    });
    mainScreen->addChild(std::move(playButton));
    
    auto stopButton = std::make_unique<Button>("stop", "STOP");
    stopButton->setPosition(690, 440);
    stopButton->setSize(80, 40);
    stopButton->setClickCallback([&sequencer]() {
        sequencer->stop();
    });
    mainScreen->addChild(std::move(stopButton));
    
    // Create performance info
    auto perfSection = std::make_unique<Label>("perf_section", "PERFORMANCE");
    perfSection->setPosition(600, 520);
    perfSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(perfSection));
    
    auto perfInfo = std::make_unique<Label>("perf_info", "CPU: 0.0% | FPS: 60 | Audio: OK");
    perfInfo->setPosition(600, 550);
    mainScreen->addChild(std::move(perfInfo));
    
    // Get pointers to visualization components for audio thread
    WaveformVisualizer* waveformPtr = 
        static_cast<WaveformVisualizer*>(mainScreen->getChild("waveform"));
    LevelMeter* levelPtr = 
        static_cast<LevelMeter*>(mainScreen->getChild("level"));
    Label* perfInfoPtr = 
        static_cast<Label*>(mainScreen->getChild("perf_info"));
    
    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    std::cout << "Added screen to UI context" << std::endl;
    
    // Set up MIDI handling
    midiInput->setCallback(midiHandler.get());
    
    midiHandler->setNoteOnCallback([&](int channel, int note, int velocity) {
        synthesizer->noteOn(note, velocity / 127.0f);
    });
    
    midiHandler->setNoteOffCallback([&](int channel, int note) {
        synthesizer->noteOff(note);
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
    auto lastPerfUpdate = std::chrono::high_resolution_clock::now();
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
        
        // Update performance info every second
        frameCount++;
        if (std::chrono::duration<float>(currentTime - lastPerfUpdate).count() > 1.0f) {
            float fps = frameCount / std::chrono::duration<float>(currentTime - lastPerfUpdate).count();
            
            std::stringstream perfText;
            perfText << "CPU: " << std::fixed << std::setprecision(1) 
                    << cpuUsage << "% | FPS: " 
                    << static_cast<int>(fps) << " | Audio: OK";
            
            if (perfInfoPtr) {
                perfInfoPtr->setText(perfText.str());
            }
            
            frameCount = 0;
            lastPerfUpdate = currentTime;
        }
        
        // Let UIContext render, but don't let it swap buffers
        // First, save the current render state
        Screen* activeScreen = uiContext->getScreen("main");
        if (activeScreen) {
            sdlDisplayManager->clear(activeScreen->getBackgroundColor());
            activeScreen->render(sdlDisplayManager.get());
            
            // Debug: Draw a small marker to show we're rendering
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_Rect marker = {10, 580, 20, 10};
            SDL_RenderFillRect(renderer, &marker);
            
            // Debug: Draw where the button should be
            SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // Magenta
            SDL_Rect buttonOutline = {10, 10, 120, 40};
            SDL_RenderDrawRect(renderer, &buttonOutline);
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