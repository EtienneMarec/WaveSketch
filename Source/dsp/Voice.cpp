#include "Voice.h"

#include <cmath>

namespace wavesketch
{

namespace
{
    constexpr double kFadeSeconds = 0.003; // durée du fondu lors d'un vol de voix

    double noteToFrequency (double note) noexcept
    {
        return 440.0 * std::exp2 ((note - 69.0) / 12.0);
    }
}

void Voice::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    fadeStep = static_cast<float> (1.0 / std::max (1.0, kFadeSeconds * sampleRate));
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

    tableCurrent = context.current != nullptr ? context.current->getTableFor (phaseIncrement) : nullptr;
    tablePrevious = context.previous != nullptr ? context.previous->getTableFor (phaseIncrement) : nullptr;
}

void Voice::render (float* out, int numSamples, const VoiceRenderContext& context) noexcept
{
    updatePitch (context);

    if (tableCurrent == nullptr)
        return;

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

        float sample = readTable (tableCurrent, phase);

        if (tablePrevious != nullptr)
        {
            const float old = readTable (tablePrevious, phase);
            sample = old + (sample - old) * context.crossfade[i];
        }

        out[i] += sample * gain;

        phase += phaseIncrement;
        phase -= std::floor (phase);
    }
}

} // namespace wavesketch
