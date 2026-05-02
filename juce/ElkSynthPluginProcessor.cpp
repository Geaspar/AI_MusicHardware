#include "ElkSynthPluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <utility>

#include "../include/audio/Synthesizer.h"
#include "../include/sequencer/Sequencer.h"
#include "../include/sequencer/HostSync.h"
#include "../include/sequencer/ClockSource.h"
#include "../include/effects/EffectProcessor.h"
#include "../include/effects/Filter.h"
#include "../include/effects/EffectUtils.h"
#include "ElkSynthPluginEditor.h"

using namespace AIMusicHardware;

ElkSynthProcessor::ElkSynthProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initial sample rate will be adjusted in prepareToPlay.
    synth_      = std::make_unique<Synthesizer>(44100);
    seq_        = std::make_unique<Sequencer>(120.0, 4);
    hostSync_   = std::make_unique<HostSync>();
    clockSource_ = std::make_unique<ClockSource>();

    // Create a minimal external FX processor with a single global
    // low-pass Filter so the JUCE UI Cutoff/Resonance controls affect audio.
    fxProcessor_ = std::make_unique<EffectProcessor>(44100);
    {
        // Global low-pass filter (always-on)
        auto globalFilter = std::make_unique<Filter>(44100, Filter::Type::LowPass);
        globalFilter->setParameter("mix", 1.0f);
        fxProcessor_->addEffect(std::move(globalFilter));

        // Simple delay for JUCE path (time/feedback fixed, mix controlled from UI)
        auto delay = std::make_unique<Delay>(44100);
        delay->setParameter("delayTime", 0.35f);   // medium delay
        delay->setParameter("feedback",  0.35f);   // gentle repeats
        delay->setParameter("mix",       0.0f);    // start dry, UI controls mix
        delayEffectIndex_ = static_cast<int>(fxProcessor_->getNumEffects());
        fxProcessor_->addEffect(std::move(delay));

        // Classic reverb (room size/damping fixed, wet level from UI)
        auto reverb = std::make_unique<Reverb>(44100);
        reverb->setParameter("roomSize", 0.6f);
        reverb->setParameter("damping",  0.4f);
        reverb->setParameter("wetLevel", 0.0f);    // start dry
        reverb->setParameter("dryLevel", 1.0f);
        reverb->setParameter("width",    1.0f);
        reverbEffectIndex_ = static_cast<int>(fxProcessor_->getNumEffects());
        fxProcessor_->addEffect(std::move(reverb));

        fxProcessor_->initialize();
    }

    if (synth_) {
        synth_->setExternalEffectProcessor(fxProcessor_.get());
    }

    seq_->attachHostSync(hostSync_.get());
    seq_->attachClockSource(clockSource_.get());

    seq_->setNoteCallbacks(
        [this](int pitch, float velocity, int channel, const Envelope& env) {
            if (synth_) {
                synth_->noteOn(pitch, velocity, env, channel);
            }

            if (seq_) {
                const int bpb = std::max(1, seq_->getBeatsPerBar());
                const double stepBeats = static_cast<double>(bpb) / 16.0;
                if (stepBeats > 0.0) {
                    double pos = std::fmod(seq_->getPrecisePositionInBeats(), static_cast<double>(bpb));
                    if (pos < 0.0) pos += static_cast<double>(bpb);
                    int col = static_cast<int>(std::floor(pos / stepBeats + 1e-6));
                    if (col < 0) col = 0;
                    if (col > 15) col %= 16;
                    debugFiredPerColumn_[static_cast<size_t>(col)] += 1;
                    if (col == debugActiveColumn_.load(std::memory_order_relaxed)) {
                        debugFiredCount_.store(debugFiredPerColumn_[static_cast<size_t>(col)],
                                               std::memory_order_relaxed);
                    }
                }
            }
        },
        [this](int pitch, int channel) {
            if (synth_) {
                synth_->noteOff(pitch, channel);
            }
        }
    );

    // Optional: enable simple sensor sources via environment for quick testing.
    // - AIMH_LIGHT_FILE=/tmp/light.txt (0..1)
    // - AIMH_DISTANCE_FILE=/tmp/distance.txt (0..1)
    // - AIMH_SENSOR_FILE=/tmp/sensor.txt (legacy alias for light)
    // - AIMH_FAKE_SENSOR=1 (legacy alias for fake light ramp)
    // - AIMH_FAKE_DISTANCE=1 (fake distance ramp)
    // When any external source is enabled, we default Sensor Mode to External
    // unless AIMH_SENSOR_MODE=manual is explicitly set.
    bool modeExplicit = false;
    if (const char* mode = std::getenv("AIMH_SENSOR_MODE")) {
        modeExplicit = true;
        if (std::strcmp(mode, "external") == 0) {
            sensorMode_.store(static_cast<int>(SensorMode::External), std::memory_order_relaxed);
        } else if (std::strcmp(mode, "manual") == 0) {
            sensorMode_.store(static_cast<int>(SensorMode::Manual), std::memory_order_relaxed);
        }
    }

    auto parseTarget = [](const char* s) -> std::optional<SensorTarget> {
        if (!s) return std::nullopt;
        if (std::strcmp(s, "cutoff") == 0) return SensorTarget::FilterCutoff;
        if (std::strcmp(s, "volume") == 0) return SensorTarget::MasterVolume;
        if (std::strcmp(s, "pitchbend") == 0) return SensorTarget::PitchBend;
        return std::nullopt;
    };

    if (auto t = parseTarget(std::getenv("AIMH_LIGHT_TARGET"))) {
        lightTarget_.store(static_cast<int>(*t), std::memory_order_relaxed);
    }
    if (auto t = parseTarget(std::getenv("AIMH_DISTANCE_TARGET"))) {
        distanceTarget_.store(static_cast<int>(*t), std::memory_order_relaxed);
    }

    const char* lightFile = std::getenv("AIMH_LIGHT_FILE");
    if (!lightFile) lightFile = std::getenv("AIMH_SENSOR_FILE"); // legacy alias
    const char* distanceFile = std::getenv("AIMH_DISTANCE_FILE");

    const bool fakeLight = (std::getenv("AIMH_FAKE_SENSOR") != nullptr) || (std::getenv("AIMH_FAKE_LIGHT") != nullptr);
    const bool fakeDistance = (std::getenv("AIMH_FAKE_DISTANCE") != nullptr);

    std::function<float()> lightReader;
    std::function<float()> distanceReader;

    if (fakeLight) {
        lightReader = []() {
            static float v = 0.0f;
            v += 0.02f;
            if (v > 1.0f) v = 0.0f;
            return v;
        };
        std::cout << "[JUCE] Sensor poller: fake light ramp enabled\n";
    } else if (lightFile) {
        std::string filePath = lightFile;
        lightReader = [filePath]() {
            std::ifstream f(filePath);
            float v = 0.0f;
            if (f >> v) return std::clamp(v, 0.0f, 1.0f);
            return 0.0f;
        };
        std::cout << "[JUCE] Sensor poller: light file source " << filePath << "\n";
    }

    if (fakeDistance) {
        distanceReader = []() {
            static float v = 0.0f;
            v += 0.02f;
            if (v > 1.0f) v = 0.0f;
            return v;
        };
        std::cout << "[JUCE] Sensor poller: fake distance ramp enabled\n";
    } else if (distanceFile) {
        std::string filePath = distanceFile;
        distanceReader = [filePath]() {
            std::ifstream f(filePath);
            float v = 0.0f;
            if (f >> v) return std::clamp(v, 0.0f, 1.0f);
            return 0.5f;
        };
        std::cout << "[JUCE] Sensor poller: distance file source " << filePath << "\n";
    }

    if (lightReader || distanceReader) {
        startSensorPoller(std::move(lightReader), std::move(distanceReader), 20);
        if (!modeExplicit) {
            sensorMode_.store(static_cast<int>(SensorMode::External), std::memory_order_relaxed);
        }
    }
}

ElkSynthProcessor::~ElkSynthProcessor() {
    stopSensorPoller();
}
void ElkSynthProcessor::stopSensorPoller() {
    if (sensorPoller_) {
        sensorPoller_->signalThreadShouldExit();
        sensorPoller_->waitForThreadToExit(500);
        sensorPoller_.reset();
    }
}

void ElkSynthProcessor::startSensorPoller(std::function<float()> reader, int intervalMs) {
    startSensorPoller(std::move(reader), std::function<float()>(), intervalMs);
}

void ElkSynthProcessor::startSensorPoller(std::function<float()> lightReader,
                                         std::function<float()> distanceReader,
                                         int intervalMs) {
    stopSensorPoller();
    sensorPoller_ = std::make_unique<SensorPoller>(std::move(lightReader), std::move(distanceReader), *this, intervalMs);
    sensorPoller_->startThread();
}

juce::AudioProcessorEditor* ElkSynthProcessor::createEditor()
{
    return new ElkSynthEditor(*this);
}

void ElkSynthProcessor::prepareToPlay(double newSampleRate, int /*samplesPerBlock*/) {
    sampleRate_ = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    if (synth_) {
        std::cout << "[JUCE] prepareToPlay sampleRate=" << sampleRate_ << std::endl;
        synth_->setSampleRate(static_cast<int>(sampleRate_));
        if (fxProcessor_) {
            fxProcessor_->setSampleRate(static_cast<int>(sampleRate_));
        }
        // Ensure we have a standard VoiceManager and fully wired engine.
        synth_->setVoiceManagerType(AIMusicHardware::VoiceManagerType::Standard);
        if (!synth_->initialize()) {
            std::cout << "[JUCE] Synthesizer::initialize() returned false\n";
        }
        std::cout << "[JUCE] VoiceManager after init: "
                  << (synth_->getVoiceManager() ? "ok" : "null") << std::endl;
    }
    if (seq_) {
        if (!seq_->initialize()) {
            std::cout << "[JUCE] Sequencer::initialize() returned false\n";
        }
        seq_->synchronizeWithAudioEngine(0.0, sampleRate_);
    }
    resetSequencerDebugState();
}

void ElkSynthProcessor::releaseResources() {
    if (synth_) {
        synth_->allNotesOff(-1);
    }
}

bool ElkSynthProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Stereo in/out only for now.
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::stereo() ||
           mainOut == juce::AudioChannelSet::mono();
}

void ElkSynthProcessor::handleExternalNoteOn(int midiNote, float velocity)
{
    if (synth_) {
        // Note: oscillator type, volume, and envelope are controlled via the editor.
        if (!synth_->getVoiceManager()) {
            std::cout << "[JUCE] Handle noteOn but voiceManager is null\n";
            return;
        }
        synth_->noteOn(midiNote, velocity, 0);
    }
}

void ElkSynthProcessor::handleExternalNoteOff(int midiNote)
{
    if (synth_) {
        synth_->noteOff(midiNote, 0);
    }
}

void ElkSynthProcessor::setOscillatorTypeFromUI(int index)
{
    if (!synth_) return;
    using OT = AIMusicHardware::OscillatorType;
    OT type = OT::Sine;
    switch (index) {
        case 1: type = OT::Square;   break;
        case 2: type = OT::Saw;      break;
        case 3: type = OT::Triangle; break;
        case 4: type = OT::Noise;    break;
        default: break; // 0 -> Sine
    }
    synth_->setOscillatorType(type);
}

void ElkSynthProcessor::setMasterVolumeFromUI(float value)
{
    if (!synth_) return;
    synth_->setParameter("master_volume", std::clamp(value, 0.0f, 1.0f));
}

void ElkSynthProcessor::setEnvelopeAttackFromUI(float value)
{
    if (!synth_) return;
    synth_->setParameter("envelope_attack", std::max(0.001f, value));
}

void ElkSynthProcessor::setEnvelopeDecayFromUI(float value)
{
    if (!synth_) return;
    synth_->setParameter("envelope_decay", std::max(0.01f, value));
}

void ElkSynthProcessor::setEnvelopeSustainFromUI(float value)
{
    if (!synth_) return;
    synth_->setParameter("envelope_sustain", std::clamp(value, 0.0f, 1.0f));
}

void ElkSynthProcessor::setEnvelopeReleaseFromUI(float value)
{
    if (!synth_) return;
    synth_->setParameter("envelope_release", std::max(0.01f, value));
}

void ElkSynthProcessor::setFilterCutoffFromUI(float value)
{
    if (!synth_) return;
    // Normalized 0..1, mapped inside Synthesizer to ~20 Hz .. 20 kHz
    float norm = std::clamp(value, 0.0f, 1.0f);
    synth_->setParameter("filter_cutoff", norm);
}

void ElkSynthProcessor::setFilterResonanceFromUI(float value)
{
    if (!synth_) return;
    // Normalized 0..1, mapped inside Synthesizer to a reasonable resonance range
    float norm = std::clamp(value, 0.0f, 1.0f);
    synth_->setParameter("filter_resonance", norm);
}

void ElkSynthProcessor::setDelayMixFromUI(float value)
{
    if (!fxProcessor_ || delayEffectIndex_ < 0) return;
    float mix = std::clamp(value, 0.0f, 1.0f);
    if (auto* e = fxProcessor_->getEffect(static_cast<size_t>(delayEffectIndex_))) {
        e->setParameter("mix", mix);
    }
}

void ElkSynthProcessor::setReverbMixFromUI(float value)
{
    if (!fxProcessor_ || reverbEffectIndex_ < 0) return;
    float wet = std::clamp(value, 0.0f, 1.0f);
    if (auto* e = fxProcessor_->getEffect(static_cast<size_t>(reverbEffectIndex_))) {
        e->setParameter("wetLevel", wet);
    }
}

void ElkSynthProcessor::setLfo1RateFromUI(float hz)
{
    if (!synth_) return;
    // Clamp to a musically useful range
    float clamped = std::clamp(hz, 0.1f, 10.0f);
    synth_->setLFORate(0, clamped); // LFO index 0 -> "LFO1"
}

void ElkSynthProcessor::setLfo1FilterDepthFromUI(float amount)
{
    if (!synth_) return;
    // 0..1 depth controlling LFO1 -> Filter Cutoff modulation amount
    float depth = std::clamp(amount, 0.0f, 1.0f);
    synth_->connectModulation("LFO1", "Filter Cutoff", depth);
}

void ElkSynthProcessor::setExternalLightNormalized(float value)
{
    externalLightValueNorm_.store(std::clamp(value, 0.0f, 1.0f), std::memory_order_relaxed);
}

void ElkSynthProcessor::setExternalDistanceNormalized(float value)
{
    externalDistanceValueNorm_.store(std::clamp(value, 0.0f, 1.0f), std::memory_order_relaxed);
}

void ElkSynthProcessor::setManualLightNormalized(float value)
{
    manualLightValueNorm_.store(std::clamp(value, 0.0f, 1.0f), std::memory_order_relaxed);
}

void ElkSynthProcessor::setManualDistanceNormalized(float value)
{
    manualDistanceValueNorm_.store(std::clamp(value, 0.0f, 1.0f), std::memory_order_relaxed);
}

void ElkSynthProcessor::setLightTargetFromUI(int targetIndex)
{
    int clamped = std::clamp(targetIndex, 0, 2);
    lightTarget_.store(clamped, std::memory_order_relaxed);
}

void ElkSynthProcessor::setDistanceTargetFromUI(int targetIndex)
{
    int clamped = std::clamp(targetIndex, 0, 2);
    distanceTarget_.store(clamped, std::memory_order_relaxed);
}

void ElkSynthProcessor::setSensorModeFromUI(int modeIndex)
{
    // 0: Manual, 1: External
    int clamped = std::clamp(modeIndex, 0, 1);
    sensorMode_.store(clamped, std::memory_order_relaxed);
}

void ElkSynthProcessor::startSequencerFromUI()
{
    if (!seq_) return;
    seq_->start();
    resetSequencerDebugState();
}

void ElkSynthProcessor::stopSequencerFromUI()
{
    if (!seq_) return;
    seq_->stop();
    resetSequencerDebugState();
}

void ElkSynthProcessor::setSequencerTempoFromUI(double bpm)
{
    if (!seq_) return;
    seq_->setTempo(std::clamp(bpm, 30.0, 240.0));
}

void ElkSynthProcessor::setSequencerLoopingFromUI(bool enabled)
{
    if (!seq_) return;
    seq_->setLooping(enabled);
}

void ElkSynthProcessor::ensureSequencerTestPatternsLoaded()
{
    if (!seq_) return;

    while (seq_->getNumPatterns() <= 1) {
        const int nextIndex = static_cast<int>(seq_->getNumPatterns()) + 1;
        seq_->createPattern("Pattern " + std::to_string(nextIndex), 16);
    }

    const int bpb = std::max(1, seq_->getBeatsPerBar());
    const double step = static_cast<double>(bpb) / 16.0;

    seq_->clearPattern(0);
    seq_->renamePattern(0, "Test Spaced");
    for (int col : {1, 6, 10, 12}) {
        seq_->addNoteToPattern(0, Note(60, 1.0f, static_cast<double>(col) * step, step, 0));
    }

    seq_->clearPattern(1);
    seq_->renamePattern(1, "Test Retriggers");
    for (int col : {3, 4, 5, 6}) {
        seq_->addNoteToPattern(1, Note(62, 1.0f, static_cast<double>(col) * step, step, 0));
    }
}

void ElkSynthProcessor::loadPatternsTestModeFromUI()
{
    if (!seq_) return;

    ensureSequencerTestPatternsLoaded();
    patternsTestModeActive_.store(true, std::memory_order_relaxed);
    seq_->setPlaybackMode(PlaybackMode::SinglePattern);
    seq_->setPerLoopDedupeEnabled(true);
    seq_->setLooping(true);
    seq_->setCurrentPattern(0);
    seq_->setPositionInBeats(0.0);
    seq_->start();
    resetSequencerDebugState();
}

void ElkSynthProcessor::selectSequencerPatternFromUI(int patternIndex)
{
    if (!seq_) return;
    if (patternIndex < 0) return;

    if (patternsTestModeActive_.load(std::memory_order_relaxed)) {
        ensureSequencerTestPatternsLoaded();
    }

    seq_->setCurrentPattern(static_cast<size_t>(patternIndex));
    seq_->setPositionInBeats(0.0);
    resetSequencerDebugState();
}

ElkSynthProcessor::SequencerUIState ElkSynthProcessor::getSequencerUIState() const
{
    SequencerUIState state;
    if (!seq_) return state;

    state.playing = seq_->isPlaying();
    state.looping = seq_->isLooping();
    state.patternsTestMode = patternsTestModeActive_.load(std::memory_order_relaxed);
    state.tempoBpm = seq_->getTempo();
    state.positionInBeats = seq_->getPrecisePositionInBeats();
    state.bar = seq_->getCurrentBar();
    state.beat = seq_->getCurrentBeat();
    state.currentPatternIndex = static_cast<int>(seq_->getCurrentPatternIndex());
    state.currentSectionName = seq_->getCurrentSectionName();
    state.activeColumn = debugActiveColumn_.load(std::memory_order_relaxed);
    state.firedCount = debugFiredCount_.load(std::memory_order_relaxed);

    if (const Pattern* pattern = seq_->getPattern(static_cast<size_t>(state.currentPatternIndex))) {
        state.currentPatternName = pattern->getName();
    }

    return state;
}

void ElkSynthProcessor::resetSequencerDebugState()
{
    debugFiredPerColumn_.fill(0);
    debugActiveColumn_.store(-1, std::memory_order_relaxed);
    debugFiredCount_.store(0, std::memory_order_relaxed);
    debugLastBar_ = -1;
    debugLastPosBeats_ = 0.0;
}

void ElkSynthProcessor::updateSequencerDebugState(double positionInBeats)
{
    if (!seq_) return;

    const int bpb = std::max(1, seq_->getBeatsPerBar());
    const int barIndex = static_cast<int>(std::floor(positionInBeats / std::max(1, bpb)));
    const bool looped = (positionInBeats + 1e-6 < debugLastPosBeats_);

    if (looped || barIndex != debugLastBar_) {
        debugLastBar_ = barIndex;
        debugFiredPerColumn_.fill(0);
    }

    const double stepBeats = static_cast<double>(bpb) / 16.0;
    double posInBar = std::fmod(positionInBeats, static_cast<double>(bpb));
    if (posInBar < 0.0) posInBar += static_cast<double>(bpb);

    int col = -1;
    if (stepBeats > 0.0) {
        col = static_cast<int>(std::floor(posInBar / stepBeats + 1e-6));
        if (col < 0) col = 0;
        if (col > 15) col %= 16;
    }

    debugActiveColumn_.store(col, std::memory_order_relaxed);
    if (col >= 0) {
        debugFiredCount_.store(debugFiredPerColumn_[static_cast<size_t>(col)], std::memory_order_relaxed);
    } else {
        debugFiredCount_.store(0, std::memory_order_relaxed);
    }

    debugLastPosBeats_ = positionInBeats;
}

// Factory function required by JUCE's plugin client code.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ElkSynthProcessor();
}

void ElkSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    buffer.clear();

    // 1) Handle MIDI input
    for (const auto metadata : midi) {
        const auto msg = metadata.getMessage();
        const int ch   = msg.getChannel() - 1;

        if (msg.isNoteOn()) {
            const int   note = msg.getNoteNumber();
            const float vel  = msg.getVelocity();
            if (synth_) synth_->noteOn(note, vel, ch);
        } else if (msg.isNoteOff()) {
            const int note = msg.getNoteNumber();
            if (synth_) synth_->noteOff(note, ch);
        } else if (msg.isPitchWheel()) {
            if (synth_) {
                const int wheel = msg.getPitchWheelValue(); // 0..16383
                float bend = static_cast<float>(wheel - 8192) / 8192.0f; // ~[-1, 1]
                bend = std::clamp(bend, -1.0f, 1.0f);
                synth_->setPitchBend(bend, ch);
            }
        } else if (msg.isController()) {
            if (synth_) {
                const int cc = msg.getControllerNumber();
                const int v = msg.getControllerValue(); // 0..127
                const float norm = std::clamp(static_cast<float>(v) / 127.0f, 0.0f, 1.0f);
                // Common defaults for synth control (also useful for sensor->MIDI bridges):
                // - CC74: brightness -> filter cutoff
                // - CC71: resonance  -> filter resonance
                // - CC7:  volume     -> master volume
                switch (cc) {
                    case 74: synth_->setParameter("filter_cutoff", norm); break;
                    case 71: synth_->setParameter("filter_resonance", norm); break;
                    case 7:  synth_->setParameter("master_volume", norm); break;
                    default: break;
                }
            }
        }
    }

    // 2) Host sync to our HostSync / Sequencer (if playhead is available)
    if (auto* playHead = getPlayHead()) {
        juce::AudioPlayHead::CurrentPositionInfo cpi;
        if (playHead->getCurrentPosition(cpi)) {
            const double bpm = (cpi.bpm > 1.0 ? cpi.bpm : seq_->getTempo());
            const double ppq = cpi.ppqPosition;
            const double sr  = sampleRate_;

            if (hostSync_) {
                hostSync_->updateFromHost(bpm, ppq, sr);
            }
            if (seq_) {
                seq_->synchronizeWithAudioEngine(cpi.timeInSeconds, sr);
            }
        }
    }

    // 3) Advance Sequencer in beats
    if (seq_) {
        updateSequencerDebugState(seq_->getPrecisePositionInBeats());
        const double dtSeconds   = static_cast<double>(numSamples) / sampleRate_;
        const double secPerBeat  = seq_->getPreciseBeatTime();
        const double beatsPerSec = secPerBeat > 0.0 ? 1.0 / secPerBeat : 0.0;
        const double deltaBeats  = dtSeconds * beatsPerSec;
        seq_->process(deltaBeats);
        const int activeCol = debugActiveColumn_.load(std::memory_order_relaxed);
        if (activeCol >= 0 && activeCol < static_cast<int>(debugFiredPerColumn_.size())) {
            debugFiredCount_.store(debugFiredPerColumn_[static_cast<size_t>(activeCol)],
                                   std::memory_order_relaxed);
        }
    }

    // 4) Render audio from Synthesizer into JUCE buffer
    if (synth_) {
        const bool external = sensorMode_.load(std::memory_order_relaxed) == static_cast<int>(SensorMode::External);

        const float lightNorm = external
            ? externalLightValueNorm_.load(std::memory_order_relaxed)
            : manualLightValueNorm_.load(std::memory_order_relaxed);
        const float distanceNorm = external
            ? externalDistanceValueNorm_.load(std::memory_order_relaxed)
            : manualDistanceValueNorm_.load(std::memory_order_relaxed);

        const int lightTarget = lightTarget_.load(std::memory_order_relaxed);
        const int distanceTarget = distanceTarget_.load(std::memory_order_relaxed);

        auto applyTarget = [this](float norm, SensorTarget t, bool invertPitch) {
            switch (t) {
                case SensorTarget::FilterCutoff:
                    synth_->setParameter("filter_cutoff", norm);
                    break;
                case SensorTarget::MasterVolume:
                    synth_->setParameter("master_volume", norm);
                    break;
                case SensorTarget::PitchBend: {
                    float bend = (norm * 2.0f) - 1.0f; // [-1, 1]
                    if (invertPitch) bend = -bend;
                    bend = std::clamp(bend, -1.0f, 1.0f);
                    synth_->setPitchBend(bend, 0);
                    break;
                }
            }
        };

        // Light sensor (default: cutoff). Apply on target change or value delta.
        const bool lightTargetChanged = (lightTarget != lastLightTargetApplied_);
        if (lightTargetChanged || std::abs(lightNorm - lastLightApplied_) > 1e-4f) {
            lastLightApplied_ = lightNorm;
            lastLightTargetApplied_ = lightTarget;
            applyTarget(lightNorm, static_cast<SensorTarget>(lightTarget), /*invertPitch=*/false);
        }

        // Distance sensor (default: pitch bend). Invert pitch by default for a Doppler-like feel:
        // closer (smaller distance -> lower norm) => higher pitch.
        const bool distanceTargetChanged = (distanceTarget != lastDistanceTargetApplied_);
        if (distanceTargetChanged || std::abs(distanceNorm - lastDistanceApplied_) > 1e-4f) {
            lastDistanceApplied_ = distanceNorm;
            lastDistanceTargetApplied_ = distanceTarget;
            applyTarget(distanceNorm, static_cast<SensorTarget>(distanceTarget), /*invertPitch=*/true);
        }

        std::vector<float> interleaved(static_cast<size_t>(numSamples) * 2, 0.0f);
        synth_->process(interleaved.data(), numSamples);

        // Apply external FX chain (global low-pass filter driven by Cutoff/Res)
        if (fxProcessor_) {
            fxProcessor_->process(interleaved.data(), numSamples);
        }

        auto* left  = buffer.getWritePointer(0);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        for (int i = 0; i < numSamples; ++i) {
            const float l = interleaved[2 * i + 0];
            const float r = interleaved[2 * i + 1];
            left[i] += l;
            if (right) right[i] += r;
        }
    }
}
