#include "Voice.h"

#include <cmath>

namespace wavesketch
{

namespace
{
    constexpr double kFadeSeconds = 0.003;      // durée du fondu lors d'un vol de voix
    constexpr double kLevelFadeSeconds = 0.005; // durée du fondu lors d'un changement d'octave de table

    double noteToFrequency (double note) noexcept
    {
        return 440.0 * std::exp2 ((note - 69.0) / 12.0);
    }
}

void Voice::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    fadeStep = static_cast<float> (1.0 / std::max (1.0, kFadeSeconds * sampleRate));
    levelFadeStep = static_cast<float> (1.0 / std::max (1.0, kLevelFadeSeconds * sampleRate));
    level = previousLevel = -1;
    levelFade = 1.0f;
    envelope.reset();
    fading = false;
    fadeGain = 1.0f;
    hasPendingNote = false;
    held = false;
    sustainedByPedal = false;
    note = soundingNote = -1;
}

void Voice::start (int midiNote, float velocity, std::uint64_t newOrder) noexcept
{
    order = newOrder;
    note = midiNote;
    held = true;
    sustainedByPedal = false;

    if (isActive())
    {
        // La voix sonne encore : on la fait taire en douceur, puis la nouvelle note démarre.
        hasPendingNote = true;
        pendingReleased = false;
        pendingVelocity = velocity;
        fading = true;
    }
    else
    {
        begin (midiNote, velocity);
    }
}

void Voice::begin (int midiNote, float velocity) noexcept
{
    soundingNote = midiNote;
    velocityGain = velocity;
    phase = 0.0;
    level = -1; // choisi au prochain updatePitch, sans fondu
    levelFade = 1.0f;
    envelope.reset();
    envelope.noteOn();
}

void Voice::release() noexcept
{
    held = false;
    sustainedByPedal = false;

    if (hasPendingNote)
        pendingReleased = true;
    else
        envelope.noteOff();
}

void Voice::stopQuickly() noexcept
{
    held = false;
    sustainedByPedal = false;
    hasPendingNote = false;

    if (isActive())
        fading = true;
}

void Voice::updatePitch (const VoiceRenderContext& context) noexcept
{
    const double frequency = noteToFrequency (soundingNote + static_cast<double> (context.pitchBendSemitones));
    phaseIncrement = frequency / context.sampleRate;

    const int newLevel = WavetableSet::getLevelFor (phaseIncrement);

    if (level < 0)
    {
        level = newLevel;
        levelFade = 1.0f;
    }
    else if (newLevel != level)
    {
        previousLevel = level;
        level = newLevel;
        levelFade = 0.0f;
    }
}

float Voice::readSet (const WavetableSet& set) const noexcept
{
    const float sample = readTable (set.getLevel (level), phase);

    if (levelFade >= 1.0f)
        return sample;

    const float old = readTable (set.getLevel (previousLevel), phase);
    return old + (sample - old) * levelFade;
}

void Voice::render (float* out, int numSamples, const VoiceRenderContext& context) noexcept
{
    if (context.current == nullptr)
        return;

    updatePitch (context);

    for (int i = 0; i < numSamples && isActive(); ++i)
    {
        float gain = envelope.next (context.envelope) * velocityGain;

        if (fading)
        {
            gain *= fadeGain;
            fadeGain -= fadeStep;

            if (fadeGain <= 0.0f)
            {
                fading = false;
                fadeGain = 1.0f;
                envelope.reset();

                if (hasPendingNote)
                {
                    hasPendingNote = false;
                    begin (note, pendingVelocity);
                    if (pendingReleased)
                        envelope.noteOff();
                    updatePitch (context);
                }
            }
        }

        float sample = readSet (*context.current);

        if (context.previous != nullptr)
        {
            const float old = readSet (*context.previous);
            sample = old + (sample - old) * context.crossfade[i];
        }

        if (levelFade < 1.0f)
            levelFade = std::min (1.0f, levelFade + levelFadeStep);

        out[i] += sample * gain;

        phase += phaseIncrement;
        phase -= std::floor (phase);
    }
}

} // namespace wavesketch
