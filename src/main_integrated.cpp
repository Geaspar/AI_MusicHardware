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
#include "../include/ai/LLMInterface.h"

// Enhanced UI system
#include "../include/ui/UIContext.h"
#include "../include/ui/SynthKnob.h"
#include "../include/ui/PresetBrowserUIComponent.h"
#include "../include/ui/VisualizationComponents.h"
#include "../include/ui/ParameterUpdateQueue.h"
#include "../include/ui/parameters/ParameterManager.h"
#include "../include/ui/presets/PresetManager.h"
#include "../include/ui/presets/PresetDatabase.h"
#include "../include/ui/presets/PresetErrorHandler.h"
#include "../include/ui/presets/PresetPerformanceMonitor.h"

// IoT support
#include "../include/iot/DummyIoTInterface.h"

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
        // Simplified text rendering - in production, use SDL_ttf
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        // Draw placeholder rectangles for text
        for (size_t i = 0; i < text.length(); i++) {
            if (text[i] != ' ') {
                SDL_Rect charRect = { x + static_cast<int>(i * 8), y, 6, 12 };
                SDL_RenderFillRect(renderer_, &charRect);
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

// Audio processing thread function
void audioProcessingThread(AudioEngine* audioEngine, Synthesizer* synthesizer, 
                          EffectProcessor* effectProcessor, Sequencer* sequencer,
                          WaveformVisualizer* waveform, LevelMeter* levelMeter,
                          std::atomic<bool>& running) {
    
    const int bufferSize = 256;
    std::vector<float> audioBuffer(bufferSize * 2); // Stereo
    
    while (running) {
        // Process sequencer
        sequencer->process(static_cast<float>(bufferSize) / audioEngine->getSampleRate());
        
        // Process synthesizer
        synthesizer->process(audioBuffer.data(), bufferSize);
        
        // Process effects
        effectProcessor->process(audioBuffer.data(), bufferSize);
        
        // Update visualizers (thread-safe)
        if (waveform) {
            waveform->pushSamples(audioBuffer.data(), bufferSize, 2);
        }
        
        // Calculate RMS for level meter
        float rms = 0.0f;
        for (int i = 0; i < bufferSize * 2; i += 2) {
            float sample = (audioBuffer[i] + audioBuffer[i + 1]) * 0.5f;
            rms += sample * sample;
        }
        rms = std::sqrt(rms / bufferSize);
        
        if (levelMeter) {
            levelMeter->setLevel(rms * 2.0f);
        }
        
        // Send to audio output
        // Note: In a real implementation, this would be callback-based
        std::this_thread::sleep_for(std::chrono::microseconds(
            bufferSize * 1000000 / audioEngine->getSampleRate()));
    }
}

void sendAllNotesOff(MidiOutput* midiOutput) {
    for (int ch = 0; ch < 16; ++ch) {
        for (int note = 0; note < 128; ++note) {
            midiOutput->sendNoteOff(ch, note);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "AI Music Hardware - Integrated UI Version" << std::endl;
    std::cout << "Starting up with production-ready UI system..." << std::endl;
    
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
    
    // Initialize error handling and performance monitoring
    PresetErrorHandler& errorHandler = PresetErrorHandler::getInstance();
    errorHandler.initialize();
    
    PresetPerformanceMonitor& perfMonitor = PresetPerformanceMonitor::getInstance();
    perfMonitor.initialize();
    
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
        errorHandler.handleError(
            PresetErrorCode::INITIALIZATION_FAILED,
            "Failed to initialize synthesizer",
            ErrorSeverity::CRITICAL
        );
        return 1;
    }
    
    if (!effectProcessor->initialize()) {
        errorHandler.handleError(
            PresetErrorCode::INITIALIZATION_FAILED,
            "Failed to initialize effect processor",
            ErrorSeverity::CRITICAL
        );
        return 1;
    }
    
    if (!sequencer->initialize()) {
        errorHandler.handleError(
            PresetErrorCode::INITIALIZATION_FAILED,
            "Failed to initialize sequencer",
            ErrorSeverity::CRITICAL
        );
        return 1;
    }
    
    if (!audioEngine->initialize()) {
        errorHandler.handleError(
            PresetErrorCode::INITIALIZATION_FAILED,
            "Failed to initialize audio engine",
            ErrorSeverity::CRITICAL
        );
        return 1;
    }
    
    // Initialize hardware (non-critical)
    if (!hardwareInterface->initialize()) {
        errorHandler.handleError(
            PresetErrorCode::INITIALIZATION_FAILED,
            "Hardware interface unavailable",
            ErrorSeverity::WARNING
        );
    }
    
    // Initialize LLM (non-critical)
    std::string llmModelPath = (argc > 1) ? argv[1] : "./models/llm_model.bin";
    auto llmInterface = std::make_unique<LLMInterface>();
    if (!llmInterface->initialize(llmModelPath)) {
        errorHandler.handleError(
            PresetErrorCode::INITIALIZATION_FAILED,
            "LLM interface unavailable",
            ErrorSeverity::WARNING
        );
    }
    
    // Create UI context with SDL display manager
    auto uiContext = std::make_unique<UIContext>();
    auto sdlDisplayManager = std::make_shared<SDLDisplayManager>(renderer);
    uiContext->setDisplayManager(sdlDisplayManager);
    uiContext->initialize(1280, 800);
    
    // Create dummy IoT interface for parameter management
    auto dummyIoT = std::make_unique<DummyIoTInterface>();
    
    // Initialize parameter manager
    auto& paramManager = EnhancedParameterManager::getInstance();
    paramManager.connectIoTInterface(dummyIoT.get());
    paramManager.connectSynthesizer(synthesizer.get());
    
    // Create parameter groups
    auto* rootGroup = paramManager.getRootGroup();
    auto* synthGroup = rootGroup->getGroup("synth");
    auto* effectsGroup = rootGroup->getGroup("effects");
    
    // Create synthesizer parameters
    auto oscFreq = std::make_unique<FloatParameter>("osc_freq", "Frequency", 440.0f);
    oscFreq->setRange(20.0f, 2000.0f);
    paramManager.registerParameter(oscFreq.get());
    synthGroup->addParameter(std::move(oscFreq));
    
    auto oscDetune = std::make_unique<FloatParameter>("osc_detune", "Detune", 0.0f);
    oscDetune->setRange(-50.0f, 50.0f);
    paramManager.registerParameter(oscDetune.get());
    synthGroup->addParameter(std::move(oscDetune));
    
    auto filterCutoff = std::make_unique<FloatParameter>("filter_cutoff", "Cutoff", 1000.0f);
    filterCutoff->setRange(20.0f, 20000.0f);
    paramManager.registerParameter(filterCutoff.get());
    synthGroup->addParameter(std::move(filterCutoff));
    
    auto filterRes = std::make_unique<FloatParameter>("filter_res", "Resonance", 0.5f);
    filterRes->setRange(0.0f, 1.0f);
    paramManager.registerParameter(filterRes.get());
    synthGroup->addParameter(std::move(filterRes));
    
    // Initialize preset system
    auto presetManager = std::make_unique<PresetManager>(synthesizer.get());
    auto presetDatabase = std::make_unique<PresetDatabase>();
    
    // Create main synthesizer screen
    auto mainScreen = std::make_unique<Screen>("main");
    mainScreen->setBackgroundColor(Color(20, 20, 25));
    
    // Title
    auto titleLabel = std::make_unique<Label>("title", "AI Music Hardware Synthesizer");
    titleLabel->setPosition(500, 10);
    titleLabel->setTextColor(Color(200, 220, 255));
    mainScreen->addChild(std::move(titleLabel));
    
    // Create oscillator section with better spacing
    auto oscSection = std::make_unique<Label>("osc_section", "OSCILLATOR");
    oscSection->setPosition(50, 50);
    oscSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(oscSection));
    
    // Increase spacing between knobs (was 130 pixels apart, now 150)
    auto freqKnob = SynthKnobFactory::createFrequencyKnob("Frequency", 50, 100);
    freqKnob->bindToParameter(paramManager.findParameter("osc_freq"),
                             ParameterBridge::ScaleType::Exponential);
    mainScreen->addChild(std::move(freqKnob));
    
    auto detuneKnob = std::make_unique<SynthKnob>("Detune", 200, 100, 80, -50.0f, 50.0f, 0.0f);
    detuneKnob->bindToParameter(paramManager.findParameter("osc_detune"));
    detuneKnob->setValueFormatter([](float value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << value << " cents";
        return ss.str();
    });
    mainScreen->addChild(std::move(detuneKnob));
    
    // Create filter section with better spacing
    auto filterSection = std::make_unique<Label>("filter_section", "FILTER");
    filterSection->setPosition(400, 50);
    filterSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(filterSection));
    
    auto cutoffKnob = SynthKnobFactory::createFrequencyKnob("Cutoff", 400, 100);
    cutoffKnob->bindToParameter(paramManager.findParameter("filter_cutoff"),
                               ParameterBridge::ScaleType::Exponential);
    mainScreen->addChild(std::move(cutoffKnob));
    
    auto resKnob = SynthKnobFactory::createResonanceKnob("Resonance", 550, 100);
    resKnob->bindToParameter(paramManager.findParameter("filter_res"),
                            ParameterBridge::ScaleType::Quadratic);
    mainScreen->addChild(std::move(resKnob));
    
    // Create envelope parameters for synth connection
    auto envAttack = std::make_unique<FloatParameter>("env_attack", "Attack", 0.01f);
    envAttack->setRange(0.001f, 2.0f);
    paramManager.registerParameter(envAttack.get());
    synthGroup->addParameter(std::move(envAttack));
    
    auto envDecay = std::make_unique<FloatParameter>("env_decay", "Decay", 0.1f);
    envDecay->setRange(0.001f, 2.0f);
    paramManager.registerParameter(envDecay.get());
    synthGroup->addParameter(std::move(envDecay));
    
    auto envSustain = std::make_unique<FloatParameter>("env_sustain", "Sustain", 0.7f);
    envSustain->setRange(0.0f, 1.0f);
    paramManager.registerParameter(envSustain.get());
    synthGroup->addParameter(std::move(envSustain));
    
    auto envRelease = std::make_unique<FloatParameter>("env_release", "Release", 0.5f);
    envRelease->setRange(0.001f, 5.0f);
    paramManager.registerParameter(envRelease.get());
    synthGroup->addParameter(std::move(envRelease));
    
    // Create visualization section with better spacing
    auto vizSection = std::make_unique<Label>("viz_section", "VISUALIZATION");
    vizSection->setPosition(50, 230);
    vizSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(vizSection));
    
    auto waveform = std::make_unique<WaveformVisualizer>("waveform", 512);
    waveform->setPosition(50, 260);
    waveform->setSize(280, 140);
    waveform->setWaveformColor(Color(0, 255, 128));
    mainScreen->addChild(std::move(waveform));
    
    auto spectrum = std::make_unique<SpectrumAnalyzer>("spectrum", 32);
    spectrum->setPosition(350, 260);
    spectrum->setSize(280, 140);
    mainScreen->addChild(std::move(spectrum));
    
    auto envelope = std::make_unique<EnvelopeVisualizer>("envelope");
    envelope->setPosition(650, 260);
    envelope->setSize(240, 140);
    envelope->setADSR(0.01f, 0.1f, 0.7f, 0.5f);
    envelope->setEditable(true);
    
    // Connect envelope editor to synthesizer parameters
    envelope->setParameterChangeCallback([&paramManager, &synthesizer](float attack, float decay, float sustain, float release) {
        // Update parameter manager
        paramManager.setParameterValue("env_attack", attack);
        paramManager.setParameterValue("env_decay", decay);
        paramManager.setParameterValue("env_sustain", sustain);
        paramManager.setParameterValue("env_release", release);
        
        // Update synthesizer directly
        synthesizer->setParameter("env_attack", attack);
        synthesizer->setParameter("env_decay", decay);
        synthesizer->setParameter("env_sustain", sustain);
        synthesizer->setParameter("env_release", release);
    });
    
    mainScreen->addChild(std::move(envelope));
    
    auto levelMeter = std::make_unique<LevelMeter>("level", LevelMeter::Orientation::Vertical);
    levelMeter->setPosition(910, 260);
    levelMeter->setSize(30, 140);
    mainScreen->addChild(std::move(levelMeter));
    
    // Create preset browser section with better spacing
    auto presetSection = std::make_unique<Label>("preset_section", "PRESET BROWSER");
    presetSection->setPosition(50, 430);
    presetSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(presetSection));
    
    auto presetBrowser = std::make_unique<PresetBrowserUI>("preset_browser");
    presetBrowser->setPosition(50, 460);
    presetBrowser->setSize(480, 280);
    presetBrowser->initialize(presetManager.get(), presetDatabase.get());
    presetBrowser->setParameterManager(&paramManager);
    
    // Set up preset loading callback
    presetBrowser->setPresetLoadCallback([&](const PresetInfo& preset) {
        std::cout << "Loading preset: " << preset.name << std::endl;
        // Load preset parameters into synthesizer
        presetManager->loadPreset(preset.name);
    });
    
    mainScreen->addChild(std::move(presetBrowser));
    
    // Create performance info panel with better spacing
    auto perfSection = std::make_unique<Label>("perf_section", "PERFORMANCE");
    perfSection->setPosition(560, 430);
    perfSection->setTextColor(Color(150, 150, 180));
    mainScreen->addChild(std::move(perfSection));
    
    auto perfInfo = std::make_unique<Label>("perf_info", "CPU: 0.0% | FPS: 60");
    perfInfo->setPosition(560, 460);
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
    
    // Set up MIDI handling
    midiInput->setCallback([&](const MidiMessage& msg) {
        midiHandler->processMessage(msg);
    });
    
    midiHandler->setNoteOnCallback([&](int channel, int note, int velocity) {
        synthesizer->noteOn(note, velocity / 127.0f);
    });
    
    midiHandler->setNoteOffCallback([&](int channel, int note) {
        synthesizer->noteOff(note);
    });
    
    // Set up sequencer callbacks
    sequencer->setNoteCallbacks(
        [&](int pitch, float velocity, int channel) {
            synthesizer->noteOn(pitch, velocity);
            midiOutput->sendNoteOn(channel, pitch, static_cast<int>(velocity * 127.0f));
        },
        [&](int pitch, int channel) {
            synthesizer->noteOff(pitch);
            midiOutput->sendNoteOff(channel, pitch);
        }
    );
    
    // Start audio processing thread
    std::atomic<bool> audioRunning(true);
    std::thread audioThread(audioProcessingThread, audioEngine.get(), synthesizer.get(),
                           effectProcessor.get(), sequencer.get(),
                           waveformPtr, levelPtr, std::ref(audioRunning));
    
    // Main loop
    bool running = true;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto lastPerfUpdate = std::chrono::high_resolution_clock::now();
    
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
        
        // Update performance info every second
        if (std::chrono::duration<float>(currentTime - lastPerfUpdate).count() > 1.0f) {
            auto stats = perfMonitor.getStatistics();
            std::stringstream perfText;
            perfText << "CPU: " << std::fixed << std::setprecision(1) 
                    << (stats.totalTimeMs / 10.0f) << "% | FPS: " 
                    << static_cast<int>(1.0f / deltaTime);
            
            if (perfInfoPtr) {
                perfInfoPtr->setText(perfText.str());
            }
            
            lastPerfUpdate = currentTime;
        }
        
        // Render
        sdlDisplayManager->clear(Color(20, 20, 25));
        uiContext->render();
        
        // Frame rate limiting
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        if (frameDuration < 16.67f) { // Target 60 FPS
            SDL_Delay(static_cast<Uint32>(16.67f - frameDuration));
        }
    }
    
    // Cleanup
    std::cout << "AI Music Hardware - Shutting down..." << std::endl;
    
    // Stop audio thread first
    std::cout << "Stopping audio thread..." << std::endl;
    audioRunning = false;
    if (audioThread.joinable()) {
        audioThread.join();
    }
    
    // Send all notes off
    std::cout << "Sending all notes off..." << std::endl;
    sendAllNotesOff(midiOutput.get());
    
    // Stop audio engine before destroying UI
    std::cout << "Stopping audio engine..." << std::endl;
    audioEngine->shutdown();
    
    // Stop hardware interface
    std::cout << "Stopping hardware interface..." << std::endl;
    hardwareInterface->shutdown();
    
    // Clear pointers in UI context before shutdown
    uiContext->connectSynthesizer(nullptr);
    uiContext->connectEffectProcessor(nullptr);
    uiContext->connectSequencer(nullptr);
    uiContext->connectHardwareInterface(nullptr);
    uiContext->connectAdaptiveSequencer(nullptr);
    uiContext->connectLLMInterface(nullptr);
    
    // Shutdown UI
    std::cout << "Shutting down UI..." << std::endl;
    uiContext->shutdown();
    
    // Reset unique_ptrs explicitly before SDL cleanup
    uiContext.reset();
    sdlDisplayManager.reset();
    
    // Cleanup SDL
    std::cout << "Cleaning up SDL..." << std::endl;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    // Log final statistics
    auto errorStats = errorHandler.getStatistics();
    std::cout << "Error statistics - Total: " << errorStats.totalErrors 
              << ", Recovered: " << errorStats.recoveredErrors << std::endl;
    
    auto perfStats = perfMonitor.getStatistics();
    std::cout << "Performance statistics - Total operations: " << perfStats.totalOperations
              << ", Average time: " << perfStats.averageTimeUs << " us" << std::endl;
    
    return 0;
}