#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "dsp/SynthEngine.h"
#include "dsp/TablePublisher.h"
#include "dsp/WavetableBuilder.h"

// Le "processeur" est le cœur du plugin : c'est lui que l'hôte (Ableton) appelle
// pour produire le son (processBlock) et pour sauvegarder/restaurer l'état.
// L'interface graphique (l'éditeur) est un objet séparé, créé et détruit à la
// demande quand l'utilisateur ouvre ou ferme la fenêtre du plugin.
class WaveSketchProcessor final : public juce::AudioProcessor,
                                  public juce::ChangeBroadcaster, // prévient l'interface quand la waveform change
                                  private juce::MidiKeyboardState::Listener,
                                  private juce::Timer
{
public:
    WaveSketchProcessor();
    ~WaveSketchProcessor() override;

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

    //==============================================================================
    // Accès depuis l'interface (message thread uniquement)
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }
    juce::MidiKeyboardState& getKeyboardState() noexcept         { return keyboardState; }

    const wavesketch::WaveformData& getWaveform() const noexcept { return waveform; }
    void setWaveform (const wavesketch::WaveformData& newWaveform);

private:
    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void pushKeyboardEvent (std::uint8_t status, int note, float velocity);
    void timerCallback() override;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* bendRangeParam = nullptr;

    // Ordre important : le moteur et le thread de calcul utilisent tablePublisher,
    // ils doivent donc être construits après lui et détruits avant lui.
    wavesketch::TablePublisher tablePublisher;
    wavesketch::SynthEngine engine { tablePublisher };
    wavesketch::WavetableBuilder tableBuilder { tablePublisher };
    juce::SmoothedValue<float> masterGain;

    wavesketch::WaveformData waveform {};

    // Clavier virtuel de l'interface : les notes passent vers l'audio par une file sans verrou.
    struct KeyboardEvent { std::uint8_t bytes[3]; };
    juce::MidiKeyboardState keyboardState;
    juce::AbstractFifo keyboardFifo { 256 };
    std::array<KeyboardEvent, 256> keyboardEvents {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSketchProcessor)
};
