#include "WaveformEditor.h"
#include "dsp/WaveformOps.h"

using namespace wavesketch;

namespace
{
    const juce::Colour panelColour (0xff1b1e25);
    const juce::Colour gridColour (0xff272b34);
    const juce::Colour gridStrongColour (0xff323743);
    const juce::Colour zeroLineColour (0xff5a6070);
    const juce::Colour waveColour (0xff4fc3f7);

    constexpr float kPadding = 14.0f;
}

WaveformEditor::WaveformEditor (WaveSketchProcessor& p)
    : audioProcessor (p)
{
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    setOpaque (false);
    audioProcessor.addChangeListener (this);
}

WaveformEditor::~WaveformEditor()
{
    audioProcessor.removeChangeListener (this);
}

juce::Rectangle<float> WaveformEditor::getPlotArea() const
{
    return getLocalBounds().toFloat().reduced (kPadding);
}

float WaveformEditor::xToIndex (float x) const
{
    const auto area = getPlotArea();
    const float index = (x - area.getX()) / area.getWidth() * static_cast<float> (kTableSize);
    return juce::jlimit (0.0f, static_cast<float> (kTableSize - 1), index);
}

float WaveformEditor::yToValue (float y) const
{
    const auto area = getPlotArea();
    return juce::jlimit (-1.0f, 1.0f, (area.getCentreY() - y) / (area.getHeight() * 0.5f));
}

void WaveformEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isLeftButtonDown())
        return;

    isDrawing = true;
    strokeData = audioProcessor.getWaveform();

    // Un simple clic modifie la largeur d'un pixel autour du curseur.
    const float index = xToIndex (e.position.x);
    const float value = yToValue (e.position.y);
    const float halfPixel = 0.5f * static_cast<float> (kTableSize) / juce::jmax (1.0f, getPlotArea().getWidth());
    ops::drawLine (strokeData, index - halfPixel, value, index + halfPixel, value);

    lastIndex = index;
    lastValue = value;
    audioProcessor.setWaveform (strokeData);
    repaint();
}

void WaveformEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (isDrawing)
        drawTo (e.position);
}

void WaveformEditor::mouseUp (const juce::MouseEvent&)
{
    isDrawing = false;
}

void WaveformEditor::drawTo (juce::Point<float> position)
{
    // Interpolation entre la position précédente et la nouvelle : pas de trou si la souris va vite.
    const float index = xToIndex (position.x);
    const float value = yToValue (position.y);
    ops::drawLine (strokeData, lastIndex, lastValue, index, value);

    lastIndex = index;
    lastValue = value;
    audioProcessor.setWaveform (strokeData);
    repaint();
}

void WaveformEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (panelColour);
    g.fillRoundedRectangle (bounds, 8.0f);

    const auto area = getPlotArea();
    const float midY = area.getCentreY();
    const float halfHeight = area.getHeight() * 0.5f;

    // Grille : 16 divisions horizontales (traits plus marqués aux quarts), repères à ±0,5 et ±1.
    for (int i = 0; i <= 16; ++i)
    {
        g.setColour (i % 4 == 0 ? gridStrongColour : gridColour);
        g.drawVerticalLine (juce::roundToInt (area.getX() + area.getWidth() * static_cast<float> (i) / 16.0f),
                            area.getY(), area.getBottom());
    }

    for (float level : { -1.0f, -0.5f, 0.5f, 1.0f })
    {
        g.setColour (std::abs (level) == 1.0f ? gridStrongColour : gridColour);
        g.drawHorizontalLine (juce::roundToInt (midY - level * halfHeight), area.getX(), area.getRight());
    }

    g.setColour (zeroLineColour);
    g.drawHorizontalLine (juce::roundToInt (midY), area.getX(), area.getRight());

    // Courbe : un point par échantillon, plus le premier répété à droite pour montrer le bouclage.
    const auto& wave = audioProcessor.getWaveform();
    juce::Path curve;

    for (int i = 0; i <= kTableSize; ++i)
    {
        const float x = area.getX() + area.getWidth() * static_cast<float> (i) / static_cast<float> (kTableSize);
        const float y = midY - wave[static_cast<size_t> (i & kTableMask)] * halfHeight;

        if (i == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }

    juce::Path fill (curve);
    fill.lineTo (area.getRight(), midY);
    fill.lineTo (area.getX(), midY);
    fill.closeSubPath();

    g.setColour (waveColour.withAlpha (0.12f));
    g.fillPath (fill);

    g.setColour (waveColour);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
