#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "dsp/SynthEngine.h"
#include "dsp/TablePublisher.h"
#include "dsp/WavetableBuilder.h"
#include "model/WaveformState.h"

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

    // Sauvegarde dans le projet : paramètres + waveform (mode, courbe, points) + taille de la fenêtre.
    // Ces deux méthodes peuvent être appelées par l'hôte depuis n'importe quel thread (sauf l'audio).
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getParameters() noexcept { return parameters; }
    juce::MidiKeyboardState& getKeyboardState() noexcept         { return keyboardState; }
    juce::UndoManager& getUndoManager() noexcept                 { return undoManager; }

    // Waveform éditée. Lecture et écriture protégées par un verrou partagé entre l'interface
    // et l'hôte, jamais avec le thread audio (qui ne voit que les tables publiées).
    wavesketch::WaveformState getWaveformState() const;

    // Remplace la waveform sans l'ajouter à l'historique (utilisé pendant un geste à la souris).
    void setWaveformState (const wavesketch::WaveformState& newState);

    // Remplace la waveform en une étape annulable (Ctrl+Z).
    void applyWaveformChange (const wavesketch::WaveformState& newState, const juce::String& actionName);

    // Termine un geste (trait, déplacement de point...) : l'ensemble devient une seule étape annulable.
    void commitWaveformGesture (const wavesketch::WaveformState& stateBeforeGesture, const juce::String& actionName);

    // Taille de la fenêtre, mémorisée dans le projet (0 = taille par défaut).
    juce::Point<int> getSavedEditorSize() const noexcept { return { editorWidth.load(), editorHeight.load() }; }
    void setSavedEditorSize (juce::Point<int> size) noexcept { editorWidth = size.x; editorHeight = size.y; }

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

    mutable juce::CriticalSection waveformLock;
    wavesketch::WaveformState waveformState;

    juce::UndoManager undoManager { 200, 30 }; // au moins 30 étapes, 200 au plus
    std::atomic<bool> undoHistoryNeedsClearing { false };

    std::atomic<int> editorWidth { 0 }, editorHeight { 0 };

    // Clavier virtuel de l'interface : les notes passent vers l'audio par une file sans verrou.
    struct KeyboardEvent { std::uint8_t bytes[3]; };
    juce::MidiKeyboardState keyboardState;
    juce::AbstractFifo keyboardFifo { 256 };
    std::array<KeyboardEvent, 256> keyboardEvents {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSketchProcessor)
};
