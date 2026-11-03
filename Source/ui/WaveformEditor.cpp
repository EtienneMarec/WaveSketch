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
    const juce::Colour pointColour (0xffffb74d);

    constexpr float kPadding = 14.0f;
    constexpr float kPointRadius = 5.5f;
    constexpr float kPointHitRadius = 10.0f;
    constexpr float kMinPointGap = 1.0f / kTableSize;
}

WaveformEditor::WaveformEditor (WaveSketchProcessor& p)
    : audioProcessor (p), state (p.getWaveformState())
{
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    audioProcessor.addChangeListener (this);
}

WaveformEditor::~WaveformEditor()
{
    audioProcessor.removeChangeListener (this);
}

void WaveformEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    // Pendant un geste, c'est la copie locale qui fait foi.
    if (gestureActive)
        return;

    state = audioProcessor.getWaveformState();
    hoveredPoint = -1;
    repaint();
}

//==============================================================================
juce::Rectangle<float> WaveformEditor::getPlotArea() const
{
    return getLocalBounds().toFloat().reduced (kPadding);
}

float WaveformEditor::xToIndex (float x) const
{
    return juce::jlimit (0.0f, static_cast<float> (kTableSize - 1), xToPhase (x) * static_cast<float> (kTableSize));
}

float WaveformEditor::xToPhase (float x) const
{
    const auto area = getPlotArea();
    return juce::jlimit (0.0f, 1.0f - kMinPointGap, (x - area.getX()) / area.getWidth());
}

float WaveformEditor::yToValue (float y) const
{
    const auto area = getPlotArea();
    return juce::jlimit (-1.0f, 1.0f, (area.getCentreY() - y) / (area.getHeight() * 0.5f));
}

juce::Point<float> WaveformEditor::pointToScreen (const ControlPoint& p) const
{
    const auto area = getPlotArea();
    return { area.getX() + p.x * area.getWidth(), area.getCentreY() - p.y * area.getHeight() * 0.5f };
}

int WaveformEditor::findPointAt (juce::Point<float> position) const
{
    if (state.mode != EditMode::points)
        return -1;

    int best = -1;
    float bestDistance = kPointHitRadius;

    for (size_t i = 0; i < state.points.size(); ++i)
    {
        const float d = pointToScreen (state.points[i]).getDistanceFrom (position);
        if (d <= bestDistance)
        {
            bestDistance = d;
            best = static_cast<int> (i);
        }
    }

    return best;
}

//==============================================================================
void WaveformEditor::beginGesture()
{
    gestureActive = true;
    stateBeforeGesture = state;
}

void WaveformEditor::pushLiveState()
{
    audioProcessor.setWaveformState (state);
    repaint();
}

void WaveformEditor::mouseDown (const juce::MouseEvent& e)
{
    if (state.mode == EditMode::points)
    {
        const int hit = findPointAt (e.position);

        if (e.mods.isPopupMenu())
        {
            if (hit >= 0)
                deletePoint (hit);
            return;
        }

        beginGesture();

        if (hit >= 0)
        {
            draggedPoint = hit;
            gestureName = juce::String::fromUTF8 ("Déplacer un point");
        }
        else if (state.points.size() < static_cast<size_t> (WaveformState::kMaxPoints))
        {
            // Nouveau point à la position du clic, inséré à sa place (les points restent triés).
            const ControlPoint newPoint { xToPhase (e.position.x), yToValue (e.position.y) };
            auto points = state.points;
            points.push_back (newPoint);
            state = WaveformState::fromPoints (std::move (points));

            draggedPoint = -1;
            for (size_t i = 0; i < state.points.size(); ++i)
                if (state.points[i] == newPoint)
                    draggedPoint = static_cast<int> (i);

            gestureName = "Ajouter un point";
            pushLiveState();
        }

        hoveredPoint = draggedPoint;
        return;
    }

    if (! e.mods.isLeftButtonDown())
        return;

    beginGesture();
    gestureName = "Dessin";

    // Un simple clic modifie la largeur d'un pixel autour du curseur.
    const float index = xToIndex (e.position.x);
    const float value = yToValue (e.position.y);
    const float halfPixel = 0.5f * static_cast<float> (kTableSize) / juce::jmax (1.0f, getPlotArea().getWidth());
    ops::drawLine (state.samples, index - halfPixel, value, index + halfPixel, value);

    lastIndex = index;
    lastValue = value;
    pushLiveState();
}

void WaveformEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (! gestureActive)
        return;

    if (state.mode == EditMode::points)
    {
        if (draggedPoint >= 0)
            movePoint (draggedPoint, e.position);
    }
    else
    {
        freehandDrawTo (e.position);
    }
}

void WaveformEditor::mouseUp (const juce::MouseEvent&)
{
    if (! gestureActive)
        return;

    gestureActive = false;
    draggedPoint = -1;
    audioProcessor.commitWaveformGesture (stateBeforeGesture, gestureName);
}

void WaveformEditor::mouseMove (const juce::MouseEvent& e)
{
    const int hit = findPointAt (e.position);
    if (hit != hoveredPoint)
    {
        hoveredPoint = hit;
        setMouseCursor (hit >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::CrosshairCursor);
        repaint();
    }
}

void WaveformEditor::mouseExit (const juce::MouseEvent&)
{
    if (hoveredPoint >= 0 && ! gestureActive)
    {
        hoveredPoint = -1;
        repaint();
    }
}

void WaveformEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (gestureActive)
        return;

    if (const int hit = findPointAt (e.position); hit >= 0)
        deletePoint (hit);
}

//==============================================================================
void WaveformEditor::freehandDrawTo (juce::Point<float> position)
{
    // Interpolation entre la position précédente et la nouvelle : pas de trou si la souris va vite.
    const float index = xToIndex (position.x);
    const float value = yToValue (position.y);
    ops::drawLine (state.samples, lastIndex, lastValue, index, value);

    lastIndex = index;
    lastValue = value;
    pushLiveState();
}

void WaveformEditor::movePoint (int index, juce::Point<float> position)
{
    auto points = state.points;
    const auto i = static_cast<size_t> (index);

    // Un point ne peut pas dépasser ses voisins : l'ordre reste le même pendant tout le déplacement.
    const float minX = i > 0 ? points[i - 1].x + kMinPointGap : 0.0f;
    const float maxX = i + 1 < points.size() ? points[i + 1].x - kMinPointGap : 1.0f - kMinPointGap;

    points[i].x = juce::jlimit (minX, juce::jmax (minX, maxX), xToPhase (position.x));
    points[i].y = yToValue (position.y);

    state.points = points;
    state.samples = renderPoints (points);
    pushLiveState();
}

void WaveformEditor::deletePoint (int index)
{
    if (state.points.size() <= static_cast<size_t> (WaveformState::kMinPoints))
        return;

    auto points = state.points;
    points.erase (points.begin() + index);
    hoveredPoint = -1;
    audioProcessor.applyWaveformChange (WaveformState::fromPoints (std::move (points)), "Supprimer un point");
}

//==============================================================================
void WaveformEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (panelColour);
    g.fillRoundedRectangle (bounds, 8.0f);

    const auto area = getPlotArea();
    paintGrid (g, area);
    paintCurve (g, area);

    if (state.mode == EditMode::points)
        paintPoints (g);
}

void WaveformEditor::paintGrid (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const float midY = area.getCentreY();
    const float halfHeight = area.getHeight() * 0.5f;

    // 16 divisions horizontales (traits plus marqués aux quarts), repères à ±0,5 et ±1.
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
}

void WaveformEditor::paintCurve (juce::Graphics& g, juce::Rectangle<float> area) const
{
    const float midY = area.getCentreY();
    const float halfHeight = area.getHeight() * 0.5f;

    // Un point par échantillon, plus le premier répété à droite pour montrer le bouclage.
    juce::Path curve;
    for (int i = 0; i <= kTableSize; ++i)
    {
        const float x = area.getX() + area.getWidth() * static_cast<float> (i) / static_cast<float> (kTableSize);
        const float y = midY - state.samples[static_cast<size_t> (i & kTableMask)] * halfHeight;

        if (i == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }

    juce::Path fill (curve);
    fill.lineTo (area.getRight(), midY);
    fill.lineTo (area.getX(), midY);
    fill.closeSubPath();

    g.setColour (waveColour.withAlpha (0.12f));
    g.fillPath (fill);

    g.setColour (state.mode == EditMode::points ? waveColour.withAlpha (0.85f) : waveColour);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void WaveformEditor::paintPoints (juce::Graphics& g) const
{
    for (size_t i = 0; i < state.points.size(); ++i)
    {
        const auto centre = pointToScreen (state.points[i]);
        const bool highlighted = static_cast<int> (i) == hoveredPoint || static_cast<int> (i) == draggedPoint;
        const float r = highlighted ? kPointRadius + 2.0f : kPointRadius;

        g.setColour (panelColour);
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f + 3.0f, r * 2.0f + 3.0f).withCentre (centre));
        g.setColour (highlighted ? pointColour.brighter (0.3f) : pointColour);
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre));
    }
}
