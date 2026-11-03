#pragma once

#include "PluginProcessor.h"

// Grande zone de dessin : affiche une période de la waveform et permet de la redessiner à la souris.
// Chaque mouvement envoie la nouvelle waveform au processeur, qui la fait calculer en arrière-plan
// puis la publie vers le moteur audio (avec un fondu, donc sans clic).
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

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }

    juce::Rectangle<float> getPlotArea() const;
    float xToIndex (float x) const;
    float yToValue (float y) const;
    void drawTo (juce::Point<float> position);

    WaveSketchProcessor& audioProcessor;

    bool isDrawing = false;
    wavesketch::WaveformData strokeData {};
    float lastIndex = 0.0f;
    float lastValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformEditor)
};
