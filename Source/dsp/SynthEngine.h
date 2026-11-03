#pragma once

#include "TablePublisher.h"
#include "Voice.h"

#include <array>
#include <cstdint>

namespace wavesketch
{

// Le moteur de synthèse, entièrement utilisé depuis le thread audio : il reçoit le MIDI,
// répartit les notes sur 16 voix et mélange leur signal.
// Aucune méthode n'alloue de mémoire, ne prend de verrou ni ne touche à l'interface.
class SynthEngine
{
public:
    static constexpr int kNumVoices = 16;
    static constexpr int kMaxChunk = 256;       // le rendu interne se fait par morceaux de 256 échantillons max
    static constexpr double kCrossfadeSeconds = 0.02; // fondu lors d'un changement de waveform

    explicit SynthEngine (TablePublisher& publisher);
    ~SynthEngine();

    void prepare (double sampleRate);

    // Réglages, à appeler au début de chaque bloc.
    void setEnvelope (float attackSec, float decaySec, float sustain, float releaseSec) noexcept;
    void setPitchBendRange (float semitones) noexcept { pitchBendRange = semitones; }

    // Message MIDI brut (octets), sans passer par juce::MidiMessage qui peut allouer pour les SysEx.
    void handleMidi (const std::uint8_t* data, int numBytes) noexcept;

    void noteOn (int note, float velocity) noexcept;
    void noteOff (int note) noexcept;
    void setSustainPedal (bool isDown) noexcept;
    void setPitchWheel (float normalised) noexcept { pitchWheel = normalised; } // -1..+1
    void allNotesOff() noexcept;
    void allSoundOff() noexcept;

    // Écrit (remplace) `numSamples` échantillons mono dans `out`.
    void render (float* out, int numSamples) noexcept;

    int getNumActiveVoices() const noexcept;
    bool isCrossfading() const noexcept { return previous != nullptr; }

private:
    void acquireNewTables() noexcept;
    Voice* findVoiceToStart() noexcept;

    TablePublisher& publisher;
    std::array<Voice, kNumVoices> voices;

    WavetableSet* current = nullptr;
    WavetableSet* previous = nullptr;
    WavetableSet* waitingToRetire = nullptr; // si la file de retour était pleine
    float crossfadePosition = 1.0f;
    float crossfadeStep = 1.0f;

    double sampleRate = 44100.0;
    float attack = 0.005f, decay = 0.3f, sustain = 0.8f, release = 0.3f;
    EnvelopeParams envelopeParams;
    float pitchBendRange = 2.0f;
    float pitchWheel = 0.0f;
    bool sustainPedalDown = false;
    std::uint64_t noteCounter = 0;
};

} // namespace wavesketch
