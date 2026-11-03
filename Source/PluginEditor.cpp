#include "PluginEditor.h"

WaveSketchEditor::WaveSketchEditor (WaveSketchProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizable (true, true);
    setResizeLimits (640, 420, 1600, 1100);
    setSize (820, 540);
}

void WaveSketchEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff16181d));

    g.setColour (juce::Colour (0xffe6e8ee));
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText ("WaveSketch", getLocalBounds().removeFromTop (getHeight() / 2), juce::Justification::centredBottom);

    g.setColour (juce::Colour (0xff8a90a0));
    g.setFont (juce::FontOptions (15.0f));
    g.drawText ("Etape 1 : plugin vide (aucun son)", getLocalBounds().removeFromBottom (getHeight() / 2).removeFromTop (40),
                juce::Justification::centred);
}

void WaveSketchEditor::resized()
{
}
