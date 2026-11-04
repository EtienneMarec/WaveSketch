#pragma once

#include "PluginProcessor.h"
#include "dsp/WaveShapes.h"
#include "ui/Theme.h"
#include "ui/WaveformEditor.h"

class WaveSketchEditor final : public juce::AudioProcessorEditor,
                               private juce::ChangeListener
{
public:
    explicit WaveSketchEditor (WaveSketchProcessor&);
    ~WaveSketchEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    static constexpr int kMinWidth = 820, kMinHeight = 560;
    static constexpr int kMaxWidth = 2400, kMaxHeight = 1600;
    static constexpr int kDefaultWidth = 980, kDefaultHeight = 660;

private:
    // Un potentiomètre relié à un paramètre automatisable.
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override { updateButtonStates(); }
    void updateButtonStates();

    void addKnob (Knob&, const juce::String& paramId, const juce::String& name);
    void addButton (juce::Button&, const juce::String& name, const juce::String& tooltip, std::function<void()> onClick);
    void applyToWaveform (const juce::String& actionName,
                          const std::function<void (wavesketch::WaveformState&)>& operation);
    void setMode (wavesketch::EditMode);
    void openWaveformFile();
    void saveWaveformFile();
    void showError (const juce::String& message);
    void layoutKnobGroup (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs);

    WaveSketchProcessor& audioProcessor;
    wavesketch::theme::LookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 600 };

    juce::TextButton freehandModeButton, pointsModeButton;
    juce::ComboBox presetBox;
    juce::TextButton openButton, saveButton, undoButton, redoButton;
    juce::TextButton sineButton, triangleButton, sawButton, squareButton;
    juce::TextButton smoothButton, invertButton, resetButton;
    WaveformEditor waveformEditor { audioProcessor };
    Knob attackKnob, decayKnob, sustainKnob, releaseKnob, gainKnob, bendKnob;
    juce::MidiKeyboardComponent keyboard;

    // Zones calculées dans resized() et décorées dans paint()
    juce::Rectangle<int> headerArea, envelopePanel, outputPanel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSketchEditor)
};
