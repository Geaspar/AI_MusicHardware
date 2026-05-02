#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>

class ElkSynthProcessor;

// Simple JUCE editor that exposes a playable MIDI keyboard for the existing engine.
class ElkSynthEditor : public juce::AudioProcessorEditor,
                       private juce::MidiKeyboardStateListener,
                       private juce::Timer
{
public:
    explicit ElkSynthEditor(ElkSynthProcessor& p);
    ~ElkSynthEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // MidiKeyboardStateListener callbacks
    void handleNoteOn(juce::MidiKeyboardState* source,
                      int midiChannel,
                      int midiNoteNumber,
                      float velocity) override;

    void handleNoteOff(juce::MidiKeyboardState* source,
                       int midiChannel,
                       int midiNoteNumber,
                       float velocity) override;

    void timerCallback() override;
    void refreshSequencerState();

    ElkSynthProcessor& processor_;
    juce::MidiKeyboardState keyboardState_;
    juce::MidiKeyboardComponent keyboard_;
    juce::Label transportLabel_;
    juce::Label debugLabel_;
    juce::Label   waveformLabel_;
    juce::ComboBox waveformBox_;
    juce::Label   volumeLabel_;
    juce::Slider   volumeSlider_;
    juce::Label   attackLabel_;
    juce::Slider   attackSlider_;
    juce::Label   decayLabel_;
    juce::Slider   decaySlider_;
    juce::Label   sustainLabel_;
    juce::Slider   sustainSlider_;
    juce::Label   releaseLabel_;
    juce::Slider   releaseSlider_;
    juce::Label   filterCutoffLabel_;
    juce::Slider  filterCutoffSlider_;
    juce::Label   filterResLabel_;
    juce::Slider  filterResSlider_;
    juce::Label   delayLabel_;
    juce::Slider  delaySlider_;
    juce::Label   reverbLabel_;
    juce::Slider  reverbSlider_;
    juce::Label   lfoRateLabel_;
    juce::Slider  lfoRateSlider_;
    juce::Label   lfoCutoffLabel_;
    juce::Slider  lfoCutoffSlider_;
    juce::Label   lightLabel_;
    juce::Slider  lightSlider_;
    juce::Label   distanceLabel_;
    juce::Slider  distanceSlider_;
    juce::Label   sensorSourceLabel_;
    juce::ComboBox sensorSourceBox_;
    juce::Label   lightTargetLabel_;
    juce::ComboBox lightTargetBox_;
    juce::Label   distanceTargetLabel_;
    juce::ComboBox distanceTargetBox_;
    juce::TextButton playButton_{"Play"};
    juce::TextButton stopButton_{"Stop"};
    juce::TextButton patternsTestButton_{"Load Test Patterns"};
    juce::ToggleButton loopButton_{"Loop"};
    juce::Label bpmLabel_;
    juce::Slider bpmSlider_;
    juce::Label testPatternLabel_;
    juce::ComboBox testPatternBox_;
    juce::Label testPatternHelpLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ElkSynthEditor)
};
