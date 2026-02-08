#include <juce_audio_utils/juce_audio_utils.h>

#include "ElkSynthPluginProcessor.h"

// Minimal standalone host that reuses ElkSynthProcessor as the engine core.
class ElkSynthStandaloneApp  : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "AIMH JUCE Standalone"; }
    const juce::String getApplicationVersion() override    { return "1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : juce::DocumentWindow(name,
                                   juce::Colours::black,
                                   juce::DocumentWindow::allButtons)
        {
            // Headless-style: we keep a simple AudioDeviceManager + AudioProcessorPlayer,
            // but do not create a heavy UI. This can be extended later if needed.
            setUsingNativeTitleBar(true);
            setVisible(true);

            audioDeviceManager.initialiseWithDefaultDevices(0, 2);

            processor = std::make_unique<ElkSynthProcessor>();
            processorPlayer.setProcessor(processor.get());
            audioDeviceManager.addAudioCallback(&processorPlayer);

            // Attach the processor's editor as the window content so we get
            // the MIDI keyboard UI for development.
            auto* editor = processor->createEditor();
            setContentOwned(editor, true);
            centreWithSize(editor->getWidth(), editor->getHeight());
        }

        ~MainWindow() override
        {
            audioDeviceManager.removeAudioCallback(&processorPlayer);
            processorPlayer.setProcessor(nullptr);
            processor.reset();
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        juce::AudioDeviceManager audioDeviceManager;
        juce::AudioProcessorPlayer processorPlayer;
        std::unique_ptr<ElkSynthProcessor> processor;
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (ElkSynthStandaloneApp)
