#include "ElkSynthPluginEditor.h"
#include "ElkSynthPluginProcessor.h"

ElkSynthEditor::ElkSynthEditor(ElkSynthProcessor& p)
    : juce::AudioProcessorEditor(&p),
      processor_(p),
      keyboard_(keyboardState_, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize(980, 560);

    keyboardState_.addListener(this);
    addAndMakeVisible(keyboard_);

    transportLabel_.setJustificationType(juce::Justification::centredLeft);
    transportLabel_.setText("Seq: stopped", juce::dontSendNotification);
    transportLabel_.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(transportLabel_);

    debugLabel_.setJustificationType(juce::Justification::centredLeft);
    debugLabel_.setText("[DBG] col - fired 0", juce::dontSendNotification);
    debugLabel_.setColour(juce::Label::textColourId, juce::Colours::orange.brighter());
    addAndMakeVisible(debugLabel_);

    playButton_.onClick = [this]() { processor_.startSequencerFromUI(); };
    addAndMakeVisible(playButton_);

    stopButton_.onClick = [this]() { processor_.stopSequencerFromUI(); };
    addAndMakeVisible(stopButton_);

    patternsTestButton_.onClick = [this]() {
        processor_.loadPatternsTestModeFromUI();
        testPatternBox_.setSelectedId(1, juce::dontSendNotification);
        refreshSequencerState();
    };
    addAndMakeVisible(patternsTestButton_);

    loopButton_.setToggleState(true, juce::dontSendNotification);
    loopButton_.onClick = [this]() {
        processor_.setSequencerLoopingFromUI(loopButton_.getToggleState());
    };
    addAndMakeVisible(loopButton_);

    bpmLabel_.setText("BPM", juce::dontSendNotification);
    bpmLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(bpmLabel_);

    bpmSlider_.setRange(30.0, 240.0, 0.1);
    bpmSlider_.setSkewFactorFromMidPoint(120.0);
    bpmSlider_.setValue(120.0, juce::dontSendNotification);
    bpmSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    bpmSlider_.onValueChange = [this]() {
        processor_.setSequencerTempoFromUI(bpmSlider_.getValue());
    };
    addAndMakeVisible(bpmSlider_);

    testPatternLabel_.setText("Pattern", juce::dontSendNotification);
    testPatternLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(testPatternLabel_);

    testPatternBox_.addItem("Test Spaced", 1);
    testPatternBox_.addItem("Test Retriggers", 2);
    testPatternBox_.setSelectedId(1, juce::dontSendNotification);
    testPatternBox_.onChange = [this]() {
        processor_.selectSequencerPatternFromUI(testPatternBox_.getSelectedId() - 1);
    };
    addAndMakeVisible(testPatternBox_);

    testPatternHelpLabel_.setText("Patterns Test uses [1,6,10,12] and [3,4,5,6] 16ths.",
                                  juce::dontSendNotification);
    testPatternHelpLabel_.setJustificationType(juce::Justification::topLeft);
    testPatternHelpLabel_.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(testPatternHelpLabel_);

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

    refreshSequencerState();
    startTimerHz(10);
}

ElkSynthEditor::~ElkSynthEditor()
{
    stopTimer();
    keyboardState_.removeListener(this);
}

void ElkSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1d2126));

    auto bounds = getLocalBounds().reduced(12);
    auto headerArea = bounds.removeFromTop(34);
    auto keyboardArea = bounds.removeFromBottom(118);
    auto contentArea = bounds;
    auto leftPanel = contentArea.removeFromLeft(static_cast<int>(contentArea.getWidth() * 0.58f));
    contentArea.removeFromLeft(12);
    auto rightPanel = contentArea;

    g.setColour(juce::Colour(0xff14181c));
    g.fillRoundedRectangle(leftPanel.toFloat(), 10.0f);
    g.fillRoundedRectangle(rightPanel.toFloat(), 10.0f);
    g.fillRoundedRectangle(keyboardArea.toFloat(), 10.0f);

    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("AIMH JUCE Synth / Sequencer Host",
               headerArea,
               juce::Justification::centred);

    g.setFont(13.0f);
    g.setColour(juce::Colours::lightgrey);
    g.drawText("Sound + modulation",
               leftPanel.removeFromTop(24).reduced(12, 4),
               juce::Justification::centredLeft);
    g.drawText("Sequencer diagnostics",
               rightPanel.removeFromTop(24).reduced(12, 4),
               juce::Justification::centredLeft);
}

void ElkSynthEditor::resized()
{
    auto area = getLocalBounds().reduced(12);
    area.removeFromTop(34);

    auto keyboardArea = area.removeFromBottom(118).reduced(10, 10);
    keyboard_.setBounds(keyboardArea);

    auto leftPanel = area.removeFromLeft(static_cast<int>(area.getWidth() * 0.58f)).reduced(12, 12);
    area.removeFromLeft(12);
    auto rightPanel = area.reduced(12, 12);
    leftPanel.removeFromTop(24);
    rightPanel.removeFromTop(24);

    auto makeRow = [](juce::Rectangle<int>& bounds, int height, int gap = 6) {
        auto row = bounds.removeFromTop(height);
        bounds.removeFromTop(gap);
        return row;
    };

    auto layoutLabeledControl = [](juce::Rectangle<int> row,
                                   juce::Label& label,
                                   juce::Component& control,
                                   int labelWidth) {
        label.setBounds(row.removeFromLeft(labelWidth));
        control.setBounds(row);
    };

    auto topRow = makeRow(leftPanel, 28);
    auto waveArea = topRow.removeFromLeft(topRow.getWidth() / 2);
    waveArea.removeFromRight(6);
    layoutLabeledControl(waveArea, waveformLabel_, waveformBox_, 50);
    topRow.removeFromLeft(6);
    layoutLabeledControl(topRow, volumeLabel_, volumeSlider_, 42);

    layoutLabeledControl(makeRow(leftPanel, 22), filterCutoffLabel_, filterCutoffSlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), filterResLabel_, filterResSlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), delayLabel_, delaySlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), reverbLabel_, reverbSlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), attackLabel_, attackSlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), decayLabel_, decaySlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), sustainLabel_, sustainSlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), releaseLabel_, releaseSlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), sensorSourceLabel_, sensorSourceBox_, 80);
    layoutLabeledControl(makeRow(leftPanel, 22), lightTargetLabel_, lightTargetBox_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), distanceTargetLabel_, distanceTargetBox_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), lightLabel_, lightSlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), distanceLabel_, distanceSlider_, 70);
    layoutLabeledControl(makeRow(leftPanel, 22), lfoRateLabel_, lfoRateSlider_, 80);
    layoutLabeledControl(makeRow(leftPanel, 22), lfoCutoffLabel_, lfoCutoffSlider_, 80);

    transportLabel_.setBounds(makeRow(rightPanel, 40));
    debugLabel_.setBounds(makeRow(rightPanel, 40));

    auto buttonRow = makeRow(rightPanel, 28);
    playButton_.setBounds(buttonRow.removeFromLeft(72));
    buttonRow.removeFromLeft(8);
    stopButton_.setBounds(buttonRow.removeFromLeft(72));
    buttonRow.removeFromLeft(12);
    loopButton_.setBounds(buttonRow.removeFromLeft(90));

    auto bpmRow = makeRow(rightPanel, 24);
    layoutLabeledControl(bpmRow, bpmLabel_, bpmSlider_, 44);

    patternsTestButton_.setBounds(makeRow(rightPanel, 28));

    auto testPatternRow = makeRow(rightPanel, 24);
    layoutLabeledControl(testPatternRow, testPatternLabel_, testPatternBox_, 56);

    testPatternHelpLabel_.setBounds(makeRow(rightPanel, 40));
}

void ElkSynthEditor::timerCallback()
{
    refreshSequencerState();
}

void ElkSynthEditor::refreshSequencerState()
{
    const auto state = processor_.getSequencerUIState();

    juce::String patternName = state.currentPatternName.empty()
        ? (juce::String("Pattern ") + juce::String(state.currentPatternIndex + 1))
        : juce::String(state.currentPatternName);

    juce::String transportText = "Seq: ";
    transportText << (state.playing ? "playing" : "stopped")
                  << " | " << juce::String(state.tempoBpm, 1) << " BPM"
                  << " | bar " << juce::String(state.bar) << " beat " << juce::String(state.beat)
                  << " | " << patternName;
    if (state.patternsTestMode) {
        transportText << " | test mode";
    }

    juce::String debugText;
    debugText << "[DBG] col " << juce::String(state.activeColumn)
              << " fired " << juce::String(state.firedCount)
              << " | pos " << juce::String(state.positionInBeats, 2);

    transportLabel_.setColour(juce::Label::textColourId,
                              state.playing ? juce::Colours::lightgreen : juce::Colours::lightgrey);
    debugLabel_.setColour(juce::Label::textColourId, juce::Colours::orange.brighter());
    transportLabel_.setText(transportText, juce::dontSendNotification);
    debugLabel_.setText(debugText, juce::dontSendNotification);

    if (!bpmSlider_.isMouseButtonDown()) {
        bpmSlider_.setValue(state.tempoBpm, juce::dontSendNotification);
    }

    if (loopButton_.getToggleState() != state.looping) {
        loopButton_.setToggleState(state.looping, juce::dontSendNotification);
    }

    if (state.patternsTestMode && state.currentPatternIndex >= 0 && state.currentPatternIndex < 2) {
        const int selectedId = state.currentPatternIndex + 1;
        if (testPatternBox_.getSelectedId() != selectedId) {
            testPatternBox_.setSelectedId(selectedId, juce::dontSendNotification);
        }
    }
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
