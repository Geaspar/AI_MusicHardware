#include <SDL2/SDL.h>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "../include/ui/UIContext.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/PresetBrowserUIComponent.h"
#include "../include/ui/VisualizationComponents.h"
#include "../include/ui/ParameterUpdateQueue.h"
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"
#include "../include/iot/DummyIoTInterface.h"

using namespace AIMusicHardware;

// Custom SDL DisplayManager (same as before)
class SDLDisplayManager : public DisplayManager {
public:
    SDLDisplayManager(SDL_Renderer* renderer) : renderer_(renderer), width_(1280), height_(800) {}
    
    bool initialize(int width, int height) override {
        width_ = width;
        height_ = height;
        return renderer_ != nullptr;
    }
    
    void shutdown() override {}
    
    void clear(const Color& color = Color(0, 0, 0)) override {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
    }
    
    void swapBuffers() override {
        SDL_RenderPresent(renderer_);
    }
    
    void drawLine(int x1, int y1, int x2, int y2, const Color& color) override {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }
    
    void drawRect(int x, int y, int width, int height, const Color& color) override {
        SDL_Rect rect = { x, y, width, height };
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawRect(renderer_, &rect);
    }
    
    void fillRect(int x, int y, int width, int height, const Color& color) override {
        SDL_Rect rect = { x, y, width, height };
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer_, &rect);
    }
    
    void drawText(int x, int y, const std::string& text, Font* font, const Color& color) override {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        
        int charWidth = 8;
        int charHeight = 16;
        
        for (size_t i = 0; i < text.length(); i++) {
            int charX = x + i * charWidth;
            
            if (text[i] != ' ') {
                if (isalpha(text[i])) {
                    SDL_Rect charRect = { charX, y, charWidth - 1, charHeight };
                    SDL_RenderDrawRect(renderer_, &charRect);
                    
                    if (islower(text[i])) {
                        SDL_RenderDrawLine(renderer_, charX, y + charHeight/2, 
                                         charX + charWidth - 2, y + charHeight/2);
                    }
                } else if (isdigit(text[i])) {
                    SDL_Rect digitRect = { charX + 1, y + 2, charWidth - 3, charHeight - 4 };
                    SDL_RenderDrawRect(renderer_, &digitRect);
                } else {
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

// Helper to translate SDL events
InputEvent translateSDLEvent(const SDL_Event& sdlEvent) {
    InputEvent event;
    
    switch (sdlEvent.type) {
        case SDL_MOUSEBUTTONDOWN:
            event.type = InputEventType::TouchPress;
            event.id = 0;
            event.value = sdlEvent.button.x;
            event.value2 = sdlEvent.button.y;
            break;
            
        case SDL_MOUSEBUTTONUP:
            event.type = InputEventType::TouchRelease;
            event.id = 0;
            event.value = sdlEvent.button.x;
            event.value2 = sdlEvent.button.y;
            break;
            
        case SDL_MOUSEMOTION:
            if (sdlEvent.motion.state & SDL_BUTTON_LMASK) {
                event.type = InputEventType::TouchMove;
                event.id = 0;
                event.value = sdlEvent.motion.x;
                event.value2 = sdlEvent.motion.y;
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
            event.value = sdlEvent.wheel.y;
            break;
    }
    
    return event;
}

// Audio generation thread simulation
void audioThreadSimulation(bool& running, WaveformVisualizer* waveform, 
                          LevelMeter* levelMeter, PhaseMeter* phaseMeter) {
    const int sampleRate = 44100;
    const int bufferSize = 256;
    float phase = 0.0f;
    float frequency = 440.0f;
    
    std::vector<float> monoBuffer(bufferSize);
    std::vector<float> stereoBuffer(bufferSize * 2);
    
    while (running) {
        // Generate test audio (sine wave with varying frequency)
        float modulation = std::sin(phase * 0.001f) * 0.3f;
        
        for (int i = 0; i < bufferSize; ++i) {
            float sample = std::sin(phase) * 0.5f;
            monoBuffer[i] = sample;
            
            // Stereo with slight phase shift for interesting patterns
            stereoBuffer[i * 2] = sample;
            stereoBuffer[i * 2 + 1] = sample * std::cos(phase * 0.1f);
            
            phase += 2.0f * M_PI * frequency * (1.0f + modulation) / sampleRate;
            if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
        
        // Push to visualizers
        if (waveform) {
            waveform->pushSamples(stereoBuffer.data(), bufferSize, 2);
        }
        
        // Calculate RMS level
        float rms = 0.0f;
        for (float sample : monoBuffer) {
            rms += sample * sample;
        }
        rms = std::sqrt(rms / bufferSize);
        
        if (levelMeter) {
            levelMeter->setLevel(rms * 2.0f); // Scale up for visibility
        }
        
        if (phaseMeter) {
            phaseMeter->pushSamples(monoBuffer.data(), &stereoBuffer[1], bufferSize);
        }
        
        // Simulate audio processing time
        std::this_thread::sleep_for(std::chrono::microseconds(
            bufferSize * 1000000 / sampleRate));
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Comprehensive UI Test - Demonstrating all UI components" << std::endl;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Comprehensive UI Test",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1280, 800,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    // Create UI context
    auto uiContext = std::make_unique<UIContext>();
    auto sdlDisplayManager = std::make_shared<SDLDisplayManager>(renderer);
    uiContext->setDisplayManager(sdlDisplayManager);
    uiContext->initialize(1280, 800);
    
    // Create main screen
    auto mainScreen = std::make_unique<Screen>("main");
    mainScreen->setBackgroundColor(Color(20, 20, 25));
    
    // Title
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware - Comprehensive UI Demo");
    titleLabel->setPosition(450, 10);
    titleLabel->setTextColor(Color(200, 220, 255));
    mainScreen->addChild(std::move(titleLabel));
    
    // Create dummy IoT interface
    auto dummyIoT = std::make_unique<DummyIoTInterface>();
    
    // Create parameter manager
    auto& paramManager = EnhancedParameterManager::getInstance();
    paramManager.connectIoTInterface(dummyIoT.get());
    auto* rootGroup = paramManager.getRootGroup();
    
    // Create synth parameters
    auto synthGroup = std::make_unique<ParameterGroup>("synth", "Synthesizer");
    
    // Oscillator parameters
    auto oscFreq = std::make_unique<FloatParameter>("osc_freq", "Frequency", 440.0f);
    oscFreq->setRange(20.0f, 2000.0f);
    
    auto oscDetune = std::make_unique<FloatParameter>("osc_detune", "Detune", 0.0f);
    oscDetune->setRange(-50.0f, 50.0f);
    
    // Filter parameters
    auto filterCutoff = std::make_unique<FloatParameter>("filter_cutoff", "Cutoff", 1000.0f);
    filterCutoff->setRange(20.0f, 20000.0f);
    
    auto filterRes = std::make_unique<FloatParameter>("filter_res", "Resonance", 0.5f);
    filterRes->setRange(0.0f, 1.0f);
    
    // Envelope parameters
    auto envAttack = std::make_unique<FloatParameter>("env_attack", "Attack", 0.01f);
    envAttack->setRange(0.001f, 2.0f);
    
    auto envDecay = std::make_unique<FloatParameter>("env_decay", "Decay", 0.1f);
    envDecay->setRange(0.001f, 2.0f);
    
    auto envSustain = std::make_unique<FloatParameter>("env_sustain", "Sustain", 0.7f);
    envSustain->setRange(0.0f, 1.0f);
    
    auto envRelease = std::make_unique<FloatParameter>("env_release", "Release", 0.5f);
    envRelease->setRange(0.001f, 4.0f);
    
    // Register parameters
    paramManager.registerParameter(oscFreq.get());
    paramManager.registerParameter(oscDetune.get());
    paramManager.registerParameter(filterCutoff.get());
    paramManager.registerParameter(filterRes.get());
    paramManager.registerParameter(envAttack.get());
    paramManager.registerParameter(envDecay.get());
    paramManager.registerParameter(envSustain.get());
    paramManager.registerParameter(envRelease.get());
    
    synthGroup->addParameter(std::move(oscFreq));
    synthGroup->addParameter(std::move(oscDetune));
    synthGroup->addParameter(std::move(filterCutoff));
    synthGroup->addParameter(std::move(filterRes));
    synthGroup->addParameter(std::move(envAttack));
    synthGroup->addParameter(std::move(envDecay));
    synthGroup->addParameter(std::move(envSustain));
    synthGroup->addParameter(std::move(envRelease));
    
    rootGroup->addGroup(std::move(synthGroup));
    
    // Create synthesizer knobs
    auto freqKnob = SynthKnobFactory::createFrequencyKnob("Frequency", 50, 80);
    freqKnob->bindToParameter(paramManager.findParameter("osc_freq"),
                             ParameterBridge::ScaleType::Exponential);
    
    auto detuneKnob = std::make_unique<SynthKnob>("Detune", 180, 80, 80, -50.0f, 50.0f, 0.0f);
    detuneKnob->bindToParameter(paramManager.findParameter("osc_detune"));
    detuneKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " cents";
        return ss.str();
    });
    
    auto cutoffKnob = SynthKnobFactory::createFrequencyKnob("Cutoff", 310, 80);
    cutoffKnob->bindToParameter(paramManager.findParameter("filter_cutoff"),
                               ParameterBridge::ScaleType::Exponential);
    
    auto resKnob = SynthKnobFactory::createResonanceKnob("Resonance", 440, 80);
    resKnob->bindToParameter(paramManager.findParameter("filter_res"),
                            ParameterBridge::ScaleType::Quadratic);
    
    // Add section labels
    auto oscLabel = std::make_unique<Label>("osc_label", "OSCILLATOR");
    oscLabel->setPosition(100, 50);
    oscLabel->setTextColor(Color(150, 150, 180));
    
    auto filterLabel = std::make_unique<Label>("filter_label", "FILTER");
    filterLabel->setPosition(350, 50);
    filterLabel->setTextColor(Color(150, 150, 180));
    
    // Create visualization components
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setPosition(50, 200);
    waveform->setSize(300, 150);
    waveform->setWaveformColor(Color(0, 255, 128));
    
    auto spectrum = std::make_unique<SpectrumAnalyzer>("spectrum", 32);
    spectrum->setPosition(370, 200);
    spectrum->setSize(300, 150);
    
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setPosition(690, 200);
    envelope->setSize(250, 150);
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    envelope->setParameterChangeCallback([&paramManager](float a, float d, float s, float r) {
        if (auto* param = paramManager.findParameter("env_attack")) {
            ((FloatParameter*)param)->setValue(a);
        }
        if (auto* param = paramManager.findParameter("env_decay")) {
            ((FloatParameter*)param)->setValue(d);
        }
        if (auto* param = paramManager.findParameter("env_sustain")) {
            ((FloatParameter*)param)->setValue(s);
        }
        if (auto* param = paramManager.findParameter("env_release")) {
            ((FloatParameter*)param)->setValue(r);
        }
    });
    
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    levelMeter->setPosition(960, 200);
    levelMeter->setSize(30, 150);
    
    auto phaseMeter = std::make_unique<PhaseMeter>("phase");
    phaseMeter->setPosition(1010, 200);
    phaseMeter->setSize(150, 150);
    
    // Visualization labels
    auto waveLabel = std::make_unique<Label>("wave_label", "Waveform");
    waveLabel->setPosition(150, 175);
    
    auto specLabel = std::make_unique<Label>("spec_label", "Spectrum");
    specLabel->setPosition(470, 175);
    
    auto envLabel = std::make_unique<Label>("env_label", "Envelope (drag to edit)");
    envLabel->setPosition(730, 175);
    
    auto levelLabel = std::make_unique<Label>("level_label", "Level");
    levelLabel->setPosition(955, 175);
    
    auto phaseLabel = std::make_unique<Label>("phase_label", "Phase");
    phaseLabel->setPosition(1065, 175);
    
    // Create preset browser
    auto presetBrowser = std::make_unique<PresetBrowserUI>("preset_browser");
    presetBrowser->setPosition(50, 380);
    presetBrowser->setSize(400, 380);
    
    // Initialize preset system
    // Note: PresetManager requires a Synthesizer pointer, using nullptr for UI demo
    auto presetManager = std::make_unique<PresetManager>(nullptr);
    auto presetDatabase = std::make_unique<PresetDatabase>();
    
    // Add some test presets
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
    preset2.description = "Warm analog-style pad";
    presetDatabase->addPreset(preset2);
    
    PresetInfo preset3;
    preset3.name = "Bass Growl";
    preset3.category = "Bass";
    preset3.author = "Demo";
    preset3.description = "Aggressive bass sound";
    presetDatabase->addPreset(preset3);
    
    presetBrowser->initialize(presetManager.get(), presetDatabase.get());
    presetBrowser->setParameterManager(&paramManager);
    
    // Mode selection buttons
    auto waveformBtn = std::make_unique<Button>("wave_btn", "Waveform");
    waveformBtn->setPosition(500, 380);
    waveformBtn->setSize(100, 30);
    waveformBtn->setToggleMode(true);
    waveformBtn->setToggled(true);
    waveformBtn->setClickCallback([&waveform]() {
        waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Waveform);
    });
    
    auto spectrumBtn = std::make_unique<Button>("spec_btn", "Spectrum");
    spectrumBtn->setPosition(610, 380);
    spectrumBtn->setSize(100, 30);
    spectrumBtn->setToggleMode(true);
    spectrumBtn->setClickCallback([&waveform]() {
        waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Spectrum);
    });
    
    auto waterfallBtn = std::make_unique<Button>("water_btn", "Waterfall");
    waterfallBtn->setPosition(720, 380);
    waterfallBtn->setSize(100, 30);
    waterfallBtn->setToggleMode(true);
    waterfallBtn->setClickCallback([&waveform]() {
        waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Waterfall);
    });
    
    auto lissajousBtn = std::make_unique<Button>("liss_btn", "Lissajous");
    lissajousBtn->setPosition(830, 380);
    lissajousBtn->setSize(100, 30);
    lissajousBtn->setToggleMode(true);
    lissajousBtn->setClickCallback([&waveform]() {
        waveform->setDisplayMode(WaveformVisualizer::DisplayMode::Lissajous);
    });
    
    // Info text
    auto infoLabel = std::make_unique<Label>("info", 
        "Use mouse wheel to zoom waveform | Click mode buttons to change visualization");
    infoLabel->setPosition(500, 420);
    
    // Add all components to screen
    mainScreen->addChild(std::move(oscLabel));
    mainScreen->addChild(std::move(filterLabel));
    mainScreen->addChild(std::move(freqKnob));
    mainScreen->addChild(std::move(detuneKnob));
    mainScreen->addChild(std::move(cutoffKnob));
    mainScreen->addChild(std::move(resKnob));
    mainScreen->addChild(std::move(waveform));
    mainScreen->addChild(std::move(spectrum));
    mainScreen->addChild(std::move(envelope));
    mainScreen->addChild(std::move(levelMeter));
    mainScreen->addChild(std::move(phaseMeter));
    mainScreen->addChild(std::move(waveLabel));
    mainScreen->addChild(std::move(specLabel));
    mainScreen->addChild(std::move(envLabel));
    mainScreen->addChild(std::move(levelLabel));
    mainScreen->addChild(std::move(phaseLabel));
    mainScreen->addChild(std::move(presetBrowser));
    mainScreen->addChild(std::move(waveformBtn));
    mainScreen->addChild(std::move(spectrumBtn));
    mainScreen->addChild(std::move(waterfallBtn));
    mainScreen->addChild(std::move(lissajousBtn));
    mainScreen->addChild(std::move(infoLabel));
    
    // Get pointers to visualization components for audio thread
    WaveformVisualizer* waveformPtr = 
        static_cast<WaveformVisualizer*>(mainScreen->getChild("waveform"));
    LevelMeter* levelPtr = 
        static_cast<LevelMeter*>(mainScreen->getChild("level"));
    PhaseMeter* phasePtr = 
        static_cast<PhaseMeter*>(mainScreen->getChild("phase"));
    
    // Add screen to context
    uiContext->addScreen(std::move(mainScreen));
    uiContext->setActiveScreen("main");
    
    // Start audio simulation thread
    bool audioRunning = true;
    std::thread audioThread(audioThreadSimulation, std::ref(audioRunning),
                           waveformPtr, levelPtr, phasePtr);
    
    // Main loop
    bool quit = false;
    auto lastTime = std::chrono::steady_clock::now();
    
    while (!quit) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Process events
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            if (sdlEvent.type == SDL_QUIT) {
                quit = true;
            } else if (sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            } else {
                InputEvent inputEvent = translateSDLEvent(sdlEvent);
                uiContext->handleInput(inputEvent);
            }
        }
        
        // Update UI
        uiContext->update(deltaTime);
        
        // Render
        sdlDisplayManager->clear(Color(20, 20, 25));
        uiContext->render();
        
        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    audioRunning = false;
    audioThread.join();
    
    uiContext->shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}