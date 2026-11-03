#pragma once

#include <algorithm>
#include <cmath>

namespace wavesketch
{

// Coefficients de l'enveloppe, calculés une fois par bloc et partagés par toutes les voix.
struct EnvelopeParams
{
    float attackIncrement = 1.0f; // montée linéaire par échantillon
    float decayCoef = 0.0f;       // facteur de rapprochement exponentiel vers le sustain
    float sustain = 1.0f;         // niveau de maintien (0..1)
    float sustainSmoothing = 1.0f; // vitesse à laquelle le niveau suit un changement de sustain
    float releaseCoef = 0.0f;     // facteur de décroissance exponentielle

    static EnvelopeParams compute (double sampleRate, float attackSec, float decaySec,
                                   float sustainLevel, float releaseSec) noexcept
    {
        const auto samples = [sampleRate] (float seconds)
        {
            return std::max (1.0, static_cast<double> (seconds) * sampleRate);
        };

        EnvelopeParams p;
        p.attackIncrement = static_cast<float> (1.0 / samples (attackSec));
        // Decay : l'écart au sustain est divisé par 1000 (-60 dB) en `decaySec` secondes.
        p.decayCoef = static_cast<float> (std::exp (std::log (1.0e-3) / samples (decaySec)));
        p.sustain = std::clamp (sustainLevel, 0.0f, 1.0f);
        // ~10 ms pour suivre un changement de sustain : évite un clic quand on l'automatise.
        p.sustainSmoothing = static_cast<float> (1.0 - std::exp (-1.0 / samples (0.01f)));
        // Release : le niveau tombe à 1/10000 (-80 dB) en `releaseSec` secondes.
        p.releaseCoef = static_cast<float> (std::exp (std::log (1.0e-4) / samples (releaseSec)));
        return p;
    }
};

// Enveloppe ADSR (Attack, Decay, Sustain, Release) : fait varier le volume d'une note
// dans le temps. Attaque linéaire, decay et release exponentiels (plus naturels à l'oreille).
// Chaque étape part du niveau actuel : relancer une note ou changer un réglage ne crée
// jamais de saut brusque du volume, donc jamais de clic.
class Envelope
{
public:
    enum class Stage { idle, attack, decay, sustain, release };

    void noteOn() noexcept   { stage = Stage::attack; }
    void noteOff() noexcept  { if (stage != Stage::idle) stage = Stage::release; }
    void reset() noexcept    { stage = Stage::idle; level = 0.0f; }

    bool isActive() const noexcept   { return stage != Stage::idle; }
    bool isReleasing() const noexcept { return stage == Stage::release; }
    float getLevel() const noexcept  { return level; }
    Stage getStage() const noexcept  { return stage; }

    float next (const EnvelopeParams& p) noexcept
    {
        switch (stage)
        {
            case Stage::idle:
                break;

            case Stage::attack:
                level += p.attackIncrement;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::decay;
                }
                break;

            case Stage::decay:
                level = p.sustain + (level - p.sustain) * p.decayCoef;
                if (std::abs (level - p.sustain) < 1.0e-4f)
                    stage = Stage::sustain;
                break;

            case Stage::sustain:
                level += (p.sustain - level) * p.sustainSmoothing;
                break;

            case Stage::release:
                level *= p.releaseCoef;
                if (level < 1.0e-4f)
                    reset();
                break;
        }

        return level;
    }

private:
    Stage stage = Stage::idle;
    float level = 0.0f;
};

} // namespace wavesketch
