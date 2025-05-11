#include "synthesis/voice/stacked_voice_manager.h"
#include "synthesis/voice/stacked_voice.h"
#include "synthesis/wavetable/oscillator_stack.h"

namespace AIMusicHardware {

StackedVoiceManager::StackedVoiceManager(int sampleRate, int maxVoices, int oscillatorsPerVoice)
    : VoiceManager(sampleRate, maxVoices)
    , oscillatorsPerVoice_(oscillatorsPerVoice)
    , detuneSpread_(10.0f)
    , stereoWidth_(0.5f)
    , convergence_(0.0f) {
}

StackedVoiceManager::~StackedVoiceManager() {
    // Smart pointers handle cleanup
}

void StackedVoiceManager::setOscillatorsPerVoice(int count) {
    if (count < 1) count = 1;
    if (count > 8) count = 8; // Maximum 8 oscillators per voice

    oscillatorsPerVoice_ = count;

    // Update existing voices
    for (auto& voice : voices_) {
        auto* stackedVoice = dynamic_cast<StackedVoice*>(voice.get());
        if (stackedVoice) {
            stackedVoice->setOscillatorCount(count);
        }
    }
}

void StackedVoiceManager::configureUnison(int count, float detune, float width, float convergence) {
    oscillatorsPerVoice_ = count;
    detuneSpread_ = detune;
    stereoWidth_ = width;
    convergence_ = convergence;

    // Update existing voices
    for (auto& voice : voices_) {
        auto* stackedVoice = dynamic_cast<StackedVoice*>(voice.get());
        if (stackedVoice) {
            stackedVoice->configureUnison(count, detune, width, convergence);
        }
    }
}

void StackedVoiceManager::setDetuneSpread(float cents) {
    detuneSpread_ = cents;

    // Update existing voices
    for (auto& voice : voices_) {
        auto* stackedVoice = dynamic_cast<StackedVoice*>(voice.get());
        if (stackedVoice) {
            stackedVoice->setDetuneSpread(cents);
        }
    }
}

void StackedVoiceManager::setStereoWidth(float width) {
    stereoWidth_ = width;

    // Update existing voices
    for (auto& voice : voices_) {
        auto* stackedVoice = dynamic_cast<StackedVoice*>(voice.get());
        if (stackedVoice) {
            stackedVoice->setStereoWidth(width);
        }
    }
}

void StackedVoiceManager::setOscillatorFramePosition(float position) {
    // Update all voices with the new frame position
    for (auto& voice : voices_) {
        auto* stackedVoice = dynamic_cast<StackedVoice*>(voice.get());
        if (stackedVoice) {
            stackedVoice->setFramePosition(position);
        }
    }
}

void StackedVoiceManager::applyDetunePreset(int presetType, float cents) {
    detuneSpread_ = cents;

    // Update all voices with the detune preset
    for (auto& voice : voices_) {
        auto* stackedVoice = dynamic_cast<StackedVoice*>(voice.get());
        if (stackedVoice) {
            stackedVoice->applyDetunePreset(presetType, cents);
        }
    }
}

void StackedVoiceManager::setDetuneType(DetuneType type) {
    // Update all voices with the detune type
    for (auto& voice : voices_) {
        auto* stackedVoice = dynamic_cast<StackedVoice*>(voice.get());
        if (stackedVoice) {
            stackedVoice->setDetuneType(type);
        }
    }
}

StackedVoice* StackedVoiceManager::getStackedVoice(int index) {
    if (index >= 0 && index < static_cast<int>(voices_.size())) {
        return dynamic_cast<StackedVoice*>(voices_[index].get());
    }
    return nullptr;
}

std::unique_ptr<Voice> StackedVoiceManager::createVoice() {
    auto voice = std::make_unique<StackedVoice>(getSampleRate(), oscillatorsPerVoice_);
    auto* stackedVoice = dynamic_cast<StackedVoice*>(voice.get());

    // Configure the new voice with current settings
    if (stackedVoice) {
        if (oscillatorsPerVoice_ > 1) {
            stackedVoice->configureUnison(oscillatorsPerVoice_, detuneSpread_, stereoWidth_, convergence_);
        }
    }

    return voice;
}

} // namespace AIMusicHardware