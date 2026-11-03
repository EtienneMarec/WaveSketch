#include "PluginEditor.h"
#include "Parameters.h"
#include "dsp/WaveformOps.h"

using namespace wavesketch;

namespace
{
    const juce::Colour backgroundColour (0xff16181d);
    const juce::Colour textColour (0xffe6e8ee);
}

WaveSketchEditor::WaveSketchEditor (WaveSketchProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    addShapeButton (sineButton,     "Sinus",                          BasicShape::sine);
    addShapeButton (triangleButton, "Triangle",                       BasicShape::triangle);
    addShapeButton (sawButton,      "Dents de scie",                  BasicShape::saw);
    addShapeButton (squareButton,   juce::String::fromUTF8 ("Carré"), BasicShape::square);

    addToolButton (smoothButton, "Lisser", juce::String::fromUTF8 ("Adoucit la courbe (cliquer plusieurs fois pour lisser davantage)"),
                   [] (WaveformData& d) { ops::smooth (d); });
    addToolButton (invertButton, "Inverser", juce::String::fromUTF8 ("Inversion de phase : retourne la courbe verticalement"),
                   [] (WaveformData& d) { ops::invert (d); });
    addToolButton (resetButton, juce::String::fromUTF8 ("Réinitialiser"), juce::String::fromUTF8 ("Revient au sinus de départ"),
                   [] (WaveformData& d) { d = makeBasicShape (BasicShape::sine); });

    addAndMakeVisible (waveformEditor);

    addKnob (attackKnob,  params::attack,    "Attack");
    addKnob (decayKnob,   params::decay,     "Decay");
    addKnob (sustainKnob, params::sustain,   "Sustain");
    addKnob (releaseKnob, params::release,   "Release");
    addKnob (gainKnob,    params::gain,      "Volume");
    addKnob (bendKnob,    params::bendRange, "Bend");

    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (48);
    addAndMakeVisible (keyboard);

    setResizable (true, true);
    setResizeLimits (720, 480, 1800, 1200);
    setSize (900, 600);
}

WaveSketchEditor::~WaveSketchEditor()
{
    // Évite une note bloquée si la fenêtre se ferme pendant qu'une touche est enfoncée.
    audioProcessor.getKeyboardState().allNotesOff (1);
    setLookAndFeel (nullptr);
}

void WaveSketchEditor::addKnob (Knob& knob, const juce::String& paramId, const juce::String& name)
{
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, textColour);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getParameters(), paramId, knob.slider);
}

void WaveSketchEditor::addShapeButton (juce::TextButton& button, const juce::String& name, BasicShape shape)
{
    button.setButtonText (name);
    button.onClick = [this, shape] { audioProcessor.setWaveform (makeBasicShape (shape)); };
    addAndMakeVisible (button);
}

void WaveSketchEditor::addToolButton (juce::TextButton& button, const juce::String& name, const juce::String& tooltip,
                                      std::function<void (WaveformData&)> operation)
{
    button.setButtonText (name);
    button.setTooltip (tooltip);
    button.onClick = [this, op = std::move (operation)]
    {
        auto data = audioProcessor.getWaveform();
        op (data);
        audioProcessor.setWaveform (data);
    };
    addAndMakeVisible (button);
}

void WaveSketchEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);

    g.setColour (textColour);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("WaveSketch", getLocalBounds().reduced (16, 0).removeFromTop (48), juce::Justification::centredLeft);
}

void WaveSketchEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto top = area.removeFromTop (36);
    top.removeFromLeft (130); // titre
    const int buttonWidth = juce::jmin (100, top.getWidth() / 7);
    for (auto* b : { &sineButton, &triangleButton, &sawButton, &squareButton })
        b->setBounds (top.removeFromLeft (buttonWidth).reduced (3, 2));

    for (auto* b : { &resetButton, &invertButton, &smoothButton })
        b->setBounds (top.removeFromRight (buttonWidth).reduced (3, 2));

    keyboard.setBounds (area.removeFromBottom (70));
    area.removeFromBottom (8);

    auto knobs = area.removeFromBottom (120);
    const int knobWidth = knobs.getWidth() / 6;
    for (auto* k : { &attackKnob, &decayKnob, &sustainKnob, &releaseKnob, &gainKnob, &bendKnob })
    {
        auto cell = knobs.removeFromLeft (knobWidth);
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell);
    }

    area.removeFromBottom (8);
    waveformEditor.setBounds (area.withTrimmedTop (8));
}
