#include "ElkSynthPluginEditor.h"
#include "ElkSynthPluginProcessor.h"

ElkSynthEditor::ElkSynthEditor(ElkSynthProcessor& p)
    : juce::AudioProcessorEditor(&p),
      processor_(p),
      keyboard_(keyboardState_, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize(820, 380);

    keyboardState_.addListener(this);
    addAndMakeVisible(keyboard_);

    // Waveform selector
    waveformLabel_.setText("Wave", juce::dontSendNotification);
    waveformLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(waveformLabel_);

    waveformBox_.addItem("Sine",     1);
    waveformBox_.addItem("Square",   2);
    waveformBox_.addItem("Saw",      3);
    waveformBox_.addItem("Triangle", 4);
    waveformBox_.addItem("Noise",    5);
    waveformBox_.setSelectedId(1, juce::dontSendNotification);
    waveformBox_.onChange = [this]() {
        int idx = waveformBox_.getSelectedId() - 1; // map 1..5 -> 0..4
        processor_.setOscillatorTypeFromUI(idx);
    };
    addAndMakeVisible(waveformBox_);

    // Master volume slider (0..1)
    volumeLabel_.setText("Vol", juce::dontSendNotification);
    volumeLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(volumeLabel_);

    volumeSlider_.setRange(0.0, 1.0, 0.001);
    volumeSlider_.setValue(0.7);
    volumeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    volumeSlider_.onValueChange = [this]() {
        processor_.setMasterVolumeFromUI((float) volumeSlider_.getValue());
    };
    addAndMakeVisible(volumeSlider_);

    // Filter cutoff (0..1, mapped to 20 Hz - 20 kHz in engine)
    filterCutoffLabel_.setText("Cutoff", juce::dontSendNotification);
    filterCutoffLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(filterCutoffLabel_);

    filterCutoffSlider_.setRange(0.0, 1.0, 0.001);
    filterCutoffSlider_.setValue(1.0); // matches engine default (wide open)
    filterCutoffSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    filterCutoffSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    filterCutoffSlider_.onValueChange = [this]() {
        processor_.setFilterCutoffFromUI((float) filterCutoffSlider_.getValue());
    };
    addAndMakeVisible(filterCutoffSlider_);

    // Filter resonance (0..1, mapped inside engine)
    filterResLabel_.setText("Res", juce::dontSendNotification);
    filterResLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(filterResLabel_);

    filterResSlider_.setRange(0.0, 1.0, 0.001);
    filterResSlider_.setValue(0.1); // matches engine default
    filterResSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    filterResSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    filterResSlider_.onValueChange = [this]() {
        processor_.setFilterResonanceFromUI((float) filterResSlider_.getValue());
    };
    addAndMakeVisible(filterResSlider_);

    // Delay mix (0..1)
    delayLabel_.setText("Delay", juce::dontSendNotification);
    delayLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(delayLabel_);

    delaySlider_.setRange(0.0, 1.0, 0.001);
    delaySlider_.setValue(0.0);
    delaySlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    delaySlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    delaySlider_.onValueChange = [this]() {
        processor_.setDelayMixFromUI((float) delaySlider_.getValue());
    };
    addAndMakeVisible(delaySlider_);

    // Reverb mix (0..1)
    reverbLabel_.setText("Reverb", juce::dontSendNotification);
    reverbLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(reverbLabel_);

    reverbSlider_.setRange(0.0, 1.0, 0.001);
    reverbSlider_.setValue(0.0);
    reverbSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    reverbSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    reverbSlider_.onValueChange = [this]() {
        processor_.setReverbMixFromUI((float) reverbSlider_.getValue());
    };
    addAndMakeVisible(reverbSlider_);

    // Manual sensor mock sliders (0..1) for desktop testing
    lightLabel_.setText("Light", juce::dontSendNotification);
    lightLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(lightLabel_);

    lightSlider_.setRange(0.0, 1.0, 0.001);
    lightSlider_.setValue(1.0);
    lightSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lightSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    lightSlider_.onValueChange = [this]() {
        processor_.setManualLightNormalized((float) lightSlider_.getValue());
    };
    addAndMakeVisible(lightSlider_);

    distanceLabel_.setText("Distance", juce::dontSendNotification);
    distanceLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(distanceLabel_);

    distanceSlider_.setRange(0.0, 1.0, 0.001);
    distanceSlider_.setValue(0.5);
    distanceSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    distanceSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    distanceSlider_.onValueChange = [this]() {
        processor_.setManualDistanceNormalized((float) distanceSlider_.getValue());
    };
    addAndMakeVisible(distanceSlider_);

    // Sensor source (Manual vs External)
    sensorSourceLabel_.setText("Sensor Src", juce::dontSendNotification);
    sensorSourceLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(sensorSourceLabel_);

    sensorSourceBox_.addItem("Manual",   1);
    sensorSourceBox_.addItem("External", 2);
    sensorSourceBox_.setSelectedId(1, juce::dontSendNotification);
    sensorSourceBox_.onChange = [this]() {
        int idx = sensorSourceBox_.getSelectedId() - 1; // 0-based
        processor_.setSensorModeFromUI(idx);
    };
    addAndMakeVisible(sensorSourceBox_);

    // Targets
    lightTargetLabel_.setText("Light->", juce::dontSendNotification);
    lightTargetLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(lightTargetLabel_);

    lightTargetBox_.addItem("Cutoff",     1);
    lightTargetBox_.addItem("Volume",     2);
    lightTargetBox_.addItem("Pitch Bend", 3);
    lightTargetBox_.setSelectedId(1, juce::dontSendNotification);
    lightTargetBox_.onChange = [this]() {
        int idx = lightTargetBox_.getSelectedId() - 1; // 0-based for processor
        processor_.setLightTargetFromUI(idx);
    };
    addAndMakeVisible(lightTargetBox_);

    distanceTargetLabel_.setText("Dist->", juce::dontSendNotification);
    distanceTargetLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(distanceTargetLabel_);

    distanceTargetBox_.addItem("Cutoff",     1);
    distanceTargetBox_.addItem("Volume",     2);
    distanceTargetBox_.addItem("Pitch Bend", 3);
    distanceTargetBox_.setSelectedId(3, juce::dontSendNotification);
    distanceTargetBox_.onChange = [this]() {
        int idx = distanceTargetBox_.getSelectedId() - 1; // 0-based for processor
        processor_.setDistanceTargetFromUI(idx);
    };
    addAndMakeVisible(distanceTargetBox_);

    // LFO1 rate (Hz)
    lfoRateLabel_.setText("LFO1 Rate", juce::dontSendNotification);
    lfoRateLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(lfoRateLabel_);

    lfoRateSlider_.setRange(0.1, 10.0, 0.01);
    lfoRateSlider_.setSkewFactorFromMidPoint(2.0); // more resolution at low rates
    lfoRateSlider_.setValue(1.0);
    lfoRateSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoRateSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    lfoRateSlider_.onValueChange = [this]() {
        processor_.setLfo1RateFromUI((float) lfoRateSlider_.getValue());
    };
    addAndMakeVisible(lfoRateSlider_);

    // LFO1 -> Filter Cutoff depth
    lfoCutoffLabel_.setText("LFO->Cutoff", juce::dontSendNotification);
    lfoCutoffLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(lfoCutoffLabel_);

    lfoCutoffSlider_.setRange(0.0, 1.0, 0.001);
    lfoCutoffSlider_.setValue(0.0);
    lfoCutoffSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    lfoCutoffSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    lfoCutoffSlider_.onValueChange = [this]() {
        processor_.setLfo1FilterDepthFromUI((float) lfoCutoffSlider_.getValue());
    };
    addAndMakeVisible(lfoCutoffSlider_);

    // ADSR sliders
    auto setupEnvSlider = [this](juce::Slider& s, float min, float max, float init) {
        s.setRange(min, max);
        s.setValue(init);
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        addAndMakeVisible(s);
    };

    setupEnvSlider(attackSlider_,  0.001f, 2.0f, 0.01f);
    setupEnvSlider(decaySlider_,   0.01f,  4.0f, 0.10f);
    setupEnvSlider(sustainSlider_, 0.0f,   1.0f, 0.70f);
    setupEnvSlider(releaseSlider_, 0.01f,  4.0f, 0.50f);

    attackSlider_.onValueChange  = [this]() { processor_.setEnvelopeAttackFromUI ((float) attackSlider_.getValue());  };
    decaySlider_.onValueChange   = [this]() { processor_.setEnvelopeDecayFromUI  ((float) decaySlider_.getValue());   };
    sustainSlider_.onValueChange = [this]() { processor_.setEnvelopeSustainFromUI((float) sustainSlider_.getValue()); };
    releaseSlider_.onValueChange = [this]() { processor_.setEnvelopeReleaseFromUI((float) releaseSlider_.getValue()); };

    attackLabel_.setText("Attack", juce::dontSendNotification);
    attackLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(attackLabel_);

    decayLabel_.setText("Decay", juce::dontSendNotification);
    decayLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(decayLabel_);

    sustainLabel_.setText("Sustain", juce::dontSendNotification);
    sustainLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(sustainLabel_);

    releaseLabel_.setText("Release", juce::dontSendNotification);
    releaseLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(releaseLabel_);
}

ElkSynthEditor::~ElkSynthEditor()
{
    keyboardState_.removeListener(this);
}

void ElkSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    auto headerArea = getLocalBounds().removeFromTop(30);
    g.drawText("AIMH JUCE Synth - On-Screen Keyboard",
               headerArea,
               juce::Justification::centred);
}

void ElkSynthEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Header
    auto headerArea = area.removeFromTop(30);
    juce::ignoreUnused(headerArea);

    // Controls area
    auto controls = area.removeFromTop(250);

    // Layout: waveform + volume on first row
    auto row = controls.removeFromTop(30);
    auto waveArea = row.removeFromLeft(200);
    waveformLabel_.setBounds(waveArea.removeFromLeft(50).reduced(0, 4));
    waveformBox_.setBounds(waveArea.reduced(0, 4));

    auto volArea = row;
    volumeLabel_.setBounds(volArea.removeFromLeft(40).reduced(0, 4));
    volumeSlider_.setBounds(volArea.reduced(0, 4));

    // Filter + ADSR rows
    auto makeRow = [&controls](int height) {
        return controls.removeFromTop(height);
    };

    auto rowF = makeRow(20);
    filterCutoffLabel_.setBounds (rowF.removeFromLeft(70).reduced(0, 2));
    filterCutoffSlider_.setBounds(rowF.reduced(0, 2));

    auto rowFr = makeRow(20);
    filterResLabel_.setBounds (rowFr.removeFromLeft(70).reduced(0, 2));
    filterResSlider_.setBounds(rowFr.reduced(0, 2));

    auto rowA = makeRow(20);
    attackLabel_.setBounds (rowA.removeFromLeft(70).reduced(0, 2));
    attackSlider_.setBounds(rowA.reduced(0, 2));

    auto rowD = makeRow(20);
    decayLabel_.setBounds  (rowD.removeFromLeft(70).reduced(0, 2));
    decaySlider_.setBounds (rowD.reduced(0, 2));

    auto rowS = makeRow(20);
    sustainLabel_.setBounds(rowS.removeFromLeft(70).reduced(0, 2));
    sustainSlider_.setBounds(rowS.reduced(0, 2));

    auto rowR = makeRow(20);
    releaseLabel_.setBounds(rowR.removeFromLeft(70).reduced(0, 2));
    releaseSlider_.setBounds(rowR.reduced(0, 2));

    auto rowDly = makeRow(20);
    delayLabel_.setBounds (rowDly.removeFromLeft(70).reduced(0, 2));
    delaySlider_.setBounds(rowDly.reduced(0, 2));

    auto rowRev = makeRow(20);
    reverbLabel_.setBounds (rowRev.removeFromLeft(70).reduced(0, 2));
    reverbSlider_.setBounds(rowRev.reduced(0, 2));

    auto rowLight = makeRow(20);
    lightLabel_.setBounds (rowLight.removeFromLeft(70).reduced(0, 2));
    lightSlider_.setBounds(rowLight.reduced(0, 2));

    auto rowDistance = makeRow(20);
    distanceLabel_.setBounds (rowDistance.removeFromLeft(70).reduced(0, 2));
    distanceSlider_.setBounds(rowDistance.reduced(0, 2));

    auto rowSensorSrc = makeRow(22);
    sensorSourceLabel_.setBounds(rowSensorSrc.removeFromLeft(80).reduced(0, 2));
    sensorSourceBox_.setBounds(rowSensorSrc.removeFromLeft(120).reduced(0, 2));

    auto rowLightTarget = makeRow(22);
    lightTargetLabel_.setBounds(rowLightTarget.removeFromLeft(70).reduced(0, 2));
    lightTargetBox_.setBounds(rowLightTarget.removeFromLeft(140).reduced(0, 2));

    auto rowDistanceTarget = makeRow(22);
    distanceTargetLabel_.setBounds(rowDistanceTarget.removeFromLeft(70).reduced(0, 2));
    distanceTargetBox_.setBounds(rowDistanceTarget.removeFromLeft(140).reduced(0, 2));

    auto rowLfoR = makeRow(20);
    lfoRateLabel_.setBounds (rowLfoR.removeFromLeft(80).reduced(0, 2));
    lfoRateSlider_.setBounds(rowLfoR.reduced(0, 2));

    auto rowLfoC = makeRow(20);
    lfoCutoffLabel_.setBounds (rowLfoC.removeFromLeft(80).reduced(0, 2));
    lfoCutoffSlider_.setBounds(rowLfoC.reduced(0, 2));

    // Keyboard fills remaining area
    keyboard_.setBounds(area);
}

void ElkSynthEditor::handleNoteOn(juce::MidiKeyboardState*,
                                  int midiChannel,
                                  int midiNoteNumber,
                                  float velocity)
{
    juce::ignoreUnused(midiChannel);
    processor_.handleExternalNoteOn(midiNoteNumber, velocity);
}

void ElkSynthEditor::handleNoteOff(juce::MidiKeyboardState*,
                                   int midiChannel,
                                   int midiNoteNumber,
                                   float /*velocity*/)
{
    juce::ignoreUnused(midiChannel);
    processor_.handleExternalNoteOff(midiNoteNumber);
}
