#include "SynthEngine.h"

#include <algorithm>

namespace wavesketch
{

namespace
{
    // Chaque voix est atténuée pour laisser de la marge quand plusieurs notes s'additionnent.
    constexpr float kVoiceGain = 0.25f;
}

SynthEngine::SynthEngine (TablePublisher& p)
    : publisher (p)
{
    prepare (sampleRate);
}

SynthEngine::~SynthEngine()
{
    // Le thread audio est arrêté à ce stade : on peut libérer directement.
    delete current;
    delete previous;
    delete waitingToRetire;
}

void SynthEngine::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    crossfadeStep = static_cast<float> (1.0 / std::max (1.0, kCrossfadeSeconds * sampleRate));

    for (auto& voice : voices)
        voice.prepare (sampleRate);

    envelopeParams = EnvelopeParams::compute (sampleRate, attack, decay, sustain, release);
    sustainPedalDown = false;
}

void SynthEngine::setEnvelope (float a, float d, float s, float r) noexcept
{
    if (a == attack && d == decay && s == sustain && r == release)
        return;

    attack = a;
    decay = d;
    sustain = s;
    release = r;
    envelopeParams = EnvelopeParams::compute (sampleRate, attack, decay, sustain, release);
}

void SynthEngine::handleMidi (const std::uint8_t* data, int numBytes) noexcept
{
    if (numBytes < 1)
        return;

    const int status = data[0];
    if (status < 0x80 || status >= 0xf0) // ignore les SysEx et messages système
        return;

    const int type = status & 0xf0;
    const int d1 = numBytes > 1 ? data[1] & 0x7f : 0;
    const int d2 = numBytes > 2 ? data[2] & 0x7f : 0;

    switch (type)
    {
        case 0x90:
            if (d2 > 0)
                noteOn (d1, static_cast<float> (d2) / 127.0f);
            else
                noteOff (d1); // note on avec vélocité 0 = note off
            break;

        case 0x80:
            noteOff (d1);
            break;

        case 0xb0:
            if (d1 == 64)       setSustainPedal (d2 >= 64);
            else if (d1 == 120) allSoundOff();
            else if (d1 == 121) { pitchWheel = 0.0f; setSustainPedal (false); }
            else if (d1 == 123) allNotesOff();
            break;

        case 0xe0:
        {
            const int value = (d2 << 7) | d1; // 0..16383, centre à 8192
            pitchWheel = static_cast<float> (value - 8192) / 8192.0f;
            break;
        }

        default:
            break;
    }
}

Voice* SynthEngine::findVoiceToStart() noexcept
{
    for (auto& voice : voices)
        if (! voice.isActive())
            return &voice;

    // Vol de voix : on sacrifie d'abord la note relâchée la plus discrète...
    Voice* best = nullptr;
    for (auto& voice : voices)
        if (voice.isReleasing() && (best == nullptr || voice.getLevel() < best->getLevel()))
            best = &voice;

    if (best != nullptr)
        return best;

    // ...sinon la note la plus ancienne.
    best = &voices.front();
    for (auto& voice : voices)
        if (voice.getOrder() < best->getOrder())
            best = &voice;

    return best;
}

void SynthEngine::noteOn (int note, float velocity) noexcept
{
    findVoiceToStart()->start (note, velocity, ++noteCounter);
}

void SynthEngine::noteOff (int note) noexcept
{
    for (auto& voice : voices)
    {
        if (voice.isActive() && voice.isHeld() && voice.getNote() == note)
        {
            if (sustainPedalDown)
                voice.setSustainedByPedal();
            else
                voice.release();
        }
    }
}

void SynthEngine::setSustainPedal (bool isDown) noexcept
{
    sustainPedalDown = isDown;

    if (! isDown)
        for (auto& voice : voices)
            if (voice.isSustainedByPedal())
                voice.release();
}

void SynthEngine::allNotesOff() noexcept
{
    sustainPedalDown = false;

    for (auto& voice : voices)
        if (voice.isActive())
            voice.release();
}

void SynthEngine::allSoundOff() noexcept
{
    sustainPedalDown = false;

    for (auto& voice : voices)
        voice.stopQuickly();
}

void SynthEngine::acquireNewTables() noexcept
{
    if (waitingToRetire != nullptr && publisher.retire (waitingToRetire))
        waitingToRetire = nullptr;

    // On n'accepte une nouvelle waveform qu'une fois le fondu précédent terminé :
    // les modifications rapides (dessin continu) sont regroupées naturellement.
    if (previous != nullptr || waitingToRetire != nullptr)
        return;

    if (auto* next = publisher.takePending())
    {
        if (current == nullptr || getNumActiveVoices() == 0)
        {
            // Rien ne sonne : inutile de faire un fondu.
            if (current != nullptr && ! publisher.retire (current))
                waitingToRetire = current;

            current = next;
        }
        else
        {
            previous = current;
            current = next;
            crossfadePosition = 0.0f;
        }
    }
}

void SynthEngine::render (float* out, int numSamples) noexcept
{
    std::fill (out, out + numSamples, 0.0f);

    while (numSamples > 0)
    {
        const int n = std::min (numSamples, kMaxChunk);
        acquireNewTables();

        float crossfade[kMaxChunk];
        VoiceRenderContext context;
        context.current = current;
        context.sampleRate = sampleRate;
        context.pitchBendSemitones = pitchWheel * pitchBendRange;
        context.envelope = envelopeParams;

        if (previous != nullptr)
        {
            for (int i = 0; i < n; ++i)
                crossfade[i] = std::min (1.0f, crossfadePosition + crossfadeStep * static_cast<float> (i + 1));

            crossfadePosition = crossfade[n - 1];
            context.previous = previous;
            context.crossfade = crossfade;
        }

        if (current != nullptr)
            for (auto& voice : voices)
                if (voice.isActive())
                    voice.render (out, n, context);

        for (int i = 0; i < n; ++i)
            out[i] *= kVoiceGain;

        if (previous != nullptr && crossfadePosition >= 1.0f)
        {
            if (! publisher.retire (previous))
                waitingToRetire = previous;

            previous = nullptr;
        }

        out += n;
        numSamples -= n;
    }
}

int SynthEngine::getNumActiveVoices() const noexcept
{
    return static_cast<int> (std::count_if (voices.begin(), voices.end(),
                                            [] (const Voice& v) { return v.isActive(); }));
}

} // namespace wavesketch
