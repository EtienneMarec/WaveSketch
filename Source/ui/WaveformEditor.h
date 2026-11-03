#pragma once

#include "PluginProcessor.h"

// Grande zone de dessin : affiche une période de la waveform et permet de la modifier à la souris.
//  - Mode main levée : on dessine directement la courbe.
//  - Mode points : clic = ajouter un point (et le déplacer), glisser = déplacer,
//    clic droit ou double-clic sur un point = le supprimer.
// Chaque mouvement envoie la nouvelle waveform au processeur (calcul en arrière-plan, puis fondu
// côté audio, donc sans clic). Un geste complet (de l'appui au relâchement) = une étape d'annulation.
class WaveformEditor final : public juce::Component,
                             private juce::ChangeListener
{
public:
    explicit WaveformEditor (WaveSketchProcessor&);
    ~WaveformEditor() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    juce::Rectangle<float> getPlotArea() const;
    float xToIndex (float x) const;
    float xToPhase (float x) const;
    float yToValue (float y) const;
    juce::Point<float> pointToScreen (const wavesketch::ControlPoint&) const;
    int findPointAt (juce::Point<float> position) const;

    void beginGesture();
    void pushLiveState();
    void freehandDrawTo (juce::Point<float> position);
    void movePoint (int index, juce::Point<float> position);
    void deletePoint (int index);

    void paintGrid (juce::Graphics&, juce::Rectangle<float> area) const;
    void paintCurve (juce::Graphics&, juce::Rectangle<float> area) const;
    void paintPoints (juce::Graphics&) const;

    WaveSketchProcessor& audioProcessor;
    wavesketch::WaveformState state; // copie affichée (fait foi pendant un geste)

    bool gestureActive = false;
    wavesketch::WaveformState stateBeforeGesture;
    juce::String gestureName;

    float lastIndex = 0.0f;
    float lastValue = 0.0f;
    int draggedPoint = -1;
    int hoveredPoint = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformEditor)
};
