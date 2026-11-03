#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Le "processeur" est le cœur du plugin : c'est lui que l'hôte (Ableton) appelle
// pour produire le son (processBlock) et pour sauvegarder/restaurer l'état.
// L'interface graphique (l'éditeur) est un objet séparé, créé et détruit à la
// demande quand l'utilisateur ouvre ou ferme la fenêtre du plugin.
class WaveSketchProcessor final : public juce::AudioProcessor
{
public:
    WaveSketchProcessor();
    ~WaveSketchProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override                             { return 1; }
    int getCurrentProgram() override                          { return 0; }
    void setCurrentProgram (int) override                     {}
    const juce::String getProgramName (int) override          { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSketchProcessor)
};
