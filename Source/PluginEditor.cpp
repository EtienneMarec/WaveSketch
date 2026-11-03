#include "PluginEditor.h"
#include "Parameters.h"
#include "dsp/WaveShapes.h"

using namespace wavesketch;

namespace
{
    const juce::Colour backgroundColour (0xff16181d);
    const juce::Colour panelColour (0xff1f222a);
    const juce::Colour gridColour (0xff2c303a);
    const juce::Colour waveColour (0xff4fc3f7);
    const juce::Colour textColour (0xffe6e8ee);
}

WaveSketchEditor::WaveSketchEditor (WaveSketchProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    addShapeButton (sineButton,     "Sinus",                              BasicShape::sine);
    addShapeButton (triangleButton, "Triangle",                           BasicShape::triangle);
    addShapeButton (sawButton,      "Dents de scie",                      BasicShape::saw);
    addShapeButton (squareButton,   juce::String::fromUTF8 ("Carré"),     BasicShape::square);

    addAndMakeVisible (preview);

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
    setResizeLimits (640, 460, 1600, 1100);
    setSize (860, 580);
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
    button.onClick = [this, shape]
    {
        audioProcessor.setWaveform (makeBasicShape (shape));
        preview.repaint();
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
    top.removeFromLeft (140); // titre
    for (auto* b : { &sineButton, &triangleButton, &sawButton, &squareButton })
    {
        b->setBounds (top.removeFromLeft (110).reduced (3, 2));
    }

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
    preview.setBounds (area.withTrimmedTop (8));
}

void WaveSketchEditor::WaveformPreview::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (panelColour);
    g.fillRoundedRectangle (bounds, 6.0f);

    const auto area = bounds.reduced (10.0f);
    const float midY = area.getCentreY();
    const float halfHeight = area.getHeight() * 0.5f;

    g.setColour (gridColour);
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine (juce::roundToInt (area.getX() + area.getWidth() * (float) i / 8.0f), area.getY(), area.getBottom());
    g.drawHorizontalLine (juce::roundToInt (midY), area.getX(), area.getRight());

    const auto& wave = audioProcessor.getWaveform();
    juce::Path path;
    const int numPoints = juce::jmax (2, juce::roundToInt (area.getWidth()));

    for (int x = 0; x < numPoints; ++x)
    {
        const int index = juce::jlimit (0, kTableSize - 1, x * kTableSize / numPoints);
        const float px = area.getX() + area.getWidth() * (float) x / (float) (numPoints - 1);
        const float py = midY - wave[(size_t) index] * halfHeight;

        if (x == 0) path.startNewSubPath (px, py);
        else        path.lineTo (px, py);
    }

    g.setColour (waveColour);
    g.strokePath (path, juce::PathStrokeType (2.0f));
}
