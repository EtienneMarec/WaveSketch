#pragma once

#include "PluginProcessor.h"
#include "dsp/WaveShapes.h"
#include "ui/WaveformEditor.h"

class WaveSketchEditor final : public juce::AudioProcessorEditor
{
public:
    explicit WaveSketchEditor (WaveSketchProcessor&);
    ~WaveSketchEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Un potentiomètre relié à un paramètre automatisable.
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (Knob&, const juce::String& paramId, const juce::String& name);
    void addShapeButton (juce::TextButton&, const juce::String& name, wavesketch::BasicShape);
    void addToolButton (juce::TextButton&, const juce::String& name, const juce::String& tooltip,
                        std::function<void (wavesketch::WaveformData&)> operation);

    WaveSketchProcessor& audioProcessor;
    juce::LookAndFeel_V4 lookAndFeel { juce::LookAndFeel_V4::getMidnightColourScheme() };
    juce::TooltipWindow tooltipWindow { this, 600 };

    juce::TextButton sineButton, triangleButton, sawButton, squareButton;
    juce::TextButton smoothButton, invertButton, resetButton;
    WaveformEditor waveformEditor { audioProcessor };
    Knob attackKnob, decayKnob, sustainKnob, releaseKnob, gainKnob, bendKnob;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSketchEditor)
};
