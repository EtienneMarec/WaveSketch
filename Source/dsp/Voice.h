#pragma once

#include "Envelope.h"
#include "Wavetable.h"

#include <cstdint>

namespace wavesketch
{

// Tout ce dont une voix a besoin pour produire un morceau de signal.
struct VoiceRenderContext
{
    const WavetableSet* current = nullptr;
    const WavetableSet* previous = nullptr; // non nul pendant un fondu entre deux waveforms
    const float* crossfade = nullptr;       // poids de `current` (0 → 1) pour chaque échantillon du fondu
    double sampleRate = 44100.0;
    float pitchBendSemitones = 0.0f;
    EnvelopeParams envelope;
};

// Une voix = une note en train de sonner : un oscillateur (lecture de la table en boucle)
// multiplié par son enveloppe ADSR et par la vélocité.
class Voice
{
public:
    void prepare (double newSampleRate) noexcept;

    // Démarre une note. Si la voix sonnait encore (vol de voix), elle fait d'abord un
    // fondu de quelques millisecondes vers le silence pour éviter un clic.
    void start (int midiNote, float velocity, std::uint64_t order) noexcept;
    void release() noexcept;      // note off : passe en phase de release
    void stopQuickly() noexcept;  // coupe la voix avec un fondu très court (All Sound Off)

    // Ajoute le signal de la voix dans `out`.
    void render (float* out, int numSamples, const VoiceRenderContext& context) noexcept;

    bool isActive() const noexcept           { return envelope.isActive() || fading; }
    bool isReleasing() const noexcept        { return ! held && ! sustainedByPedal; }
    int getNote() const noexcept             { return note; }
    bool isHeld() const noexcept             { return held; }
    std::uint64_t getOrder() const noexcept  { return order; }
    float getLevel() const noexcept          { return envelope.getLevel() * (fading ? fadeGain : 1.0f); }

    // Pédale de sustain : la note est relâchée mais continue de sonner tant que la pédale est enfoncée.
    void setSustainedByPedal() noexcept      { held = false; sustainedByPedal = true; }
    bool isSustainedByPedal() const noexcept { return sustainedByPedal; }

private:
    void begin (int midiNote, float velocity) noexcept;
    void updatePitch (const VoiceRenderContext& context) noexcept;

    Envelope envelope;
    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    const float* tableCurrent = nullptr;
    const float* tablePrevious = nullptr;

    int note = -1;          // note "logique" (celle qu'un note off doit relâcher)
    int soundingNote = -1;  // note réellement jouée (diffère de `note` pendant un vol de voix)
    float velocityGain = 0.0f;
    std::uint64_t order = 0;
    bool held = false;
    bool sustainedByPedal = false;

    bool fading = false;
    float fadeGain = 1.0f;
    float fadeStep = 0.0f;

    bool hasPendingNote = false;
    bool pendingReleased = false;
    float pendingVelocity = 0.0f;
};

} // namespace wavesketch
