#include <juce_core/juce_core.h>

#include "dsp/Envelope.h"
#include "dsp/SynthEngine.h"
#include "dsp/WaveShapes.h"
#include "dsp/Wavetable.h"

#include <cmath>
#include <vector>

using namespace wavesketch;

namespace
{
    constexpr double twoPi = 6.283185307179586;

    std::vector<float> renderEngine (SynthEngine& engine, int numSamples, int blockSize)
    {
        std::vector<float> out (static_cast<size_t> (numSamples));
        for (int pos = 0; pos < numSamples; pos += blockSize)
            engine.render (out.data() + pos, std::min (blockSize, numSamples - pos));
        return out;
    }

    int countRisingZeroCrossings (const std::vector<float>& signal)
    {
        int count = 0;
        for (size_t i = 1; i < signal.size(); ++i)
            if (signal[i - 1] < 0.0f && signal[i] >= 0.0f)
                ++count;
        return count;
    }

    float peak (const std::vector<float>& signal, size_t from = 0)
    {
        float p = 0.0f;
        for (size_t i = from; i < signal.size(); ++i)
            p = std::max (p, std::abs (signal[i]));
        return p;
    }
}

//==============================================================================
class WavetableTests final : public juce::UnitTest
{
public:
    WavetableTests() : juce::UnitTest ("Wavetable et interpolation", "WaveSketch") {}

    void runTest() override
    {
        const auto sine = makeBasicShape (BasicShape::sine);

        beginTest ("Lecture exacte sur les echantillons");
        for (int i = 0; i < kTableSize; i += 97)
            expectWithinAbsoluteError (readTable (sine.data(), (double) i / kTableSize), sine[(size_t) i], 1.0e-6f);

        beginTest ("Interpolation entre deux echantillons (Hermite)");
        {
            double maxError = 0.0;
            for (int i = 0; i < 10000; ++i)
            {
                const double phase = (i + 0.37) / 10000.0;
                maxError = std::max (maxError, std::abs (readTable (sine.data(), phase) - std::sin (twoPi * phase)));
            }
            expectLessThan (maxError, 1.0e-6);
        }

        beginTest ("Bouclage continu en fin de periode");
        expectWithinAbsoluteError (readTable (sine.data(), 1.0 - 0.25 / kTableSize),
                                   (float) std::sin (twoPi * (1.0 - 0.25 / kTableSize)), 1.0e-6f);

        beginTest ("Formes de base dans [-1, 1]");
        for (auto shape : { BasicShape::sine, BasicShape::triangle, BasicShape::saw, BasicShape::square })
        {
            const auto data = makeBasicShape (shape);
            for (float v : data)
                expect (v >= -1.0f && v <= 1.0f);
        }
        expectWithinAbsoluteError (sine[kTableSize / 4], 1.0f, 1.0e-6f);
        expectWithinAbsoluteError (makeBasicShape (BasicShape::triangle)[kTableSize / 4], 1.0f, 1.0e-6f);
    }
};

//==============================================================================
class EnvelopeTests final : public juce::UnitTest
{
public:
    EnvelopeTests() : juce::UnitTest ("Enveloppe ADSR", "WaveSketch") {}

    void runTest() override
    {
        constexpr double sr = 48000.0;
        const auto p = EnvelopeParams::compute (sr, 0.01f, 0.1f, 0.5f, 0.2f);

        beginTest ("Attaque, decay puis sustain");
        Envelope env;
        env.noteOn();
        int samples = 0;
        while (env.getStage() == Envelope::Stage::attack)
        {
            env.next (p);
            ++samples;
        }
        expectWithinAbsoluteError (samples, 480, 2);
        for (int i = 0; i < (int) sr; ++i)
            env.next (p);
        expect (env.getStage() == Envelope::Stage::sustain);
        expectWithinAbsoluteError (env.getLevel(), 0.5f, 1.0e-3f);

        beginTest ("Release jusqu'au silence");
        env.noteOff();
        samples = 0;
        while (env.isActive() && samples < (int) sr)
        {
            env.next (p);
            ++samples;
        }
        expect (! env.isActive());
        expectLessThan (samples, (int) (0.2 * sr));

        beginTest ("Changement de sustain sans saut brusque");
        Envelope env2;
        env2.noteOn();
        for (int i = 0; i < (int) sr; ++i)
            env2.next (p);
        const auto p2 = EnvelopeParams::compute (sr, 0.01f, 0.1f, 1.0f, 0.2f);
        float previous = env2.getLevel();
        float maxStep = 0.0f;
        for (int i = 0; i < 2000; ++i)
        {
            const float level = env2.next (p2);
            maxStep = std::max (maxStep, std::abs (level - previous));
            previous = level;
        }
        expectLessThan (maxStep, 0.01f);
    }
};

//==============================================================================
class SynthEngineTests final : public juce::UnitTest
{
public:
    SynthEngineTests() : juce::UnitTest ("Moteur de synthese", "WaveSketch") {}

    void runTest() override
    {
        beginTest ("Hauteur juste a toutes les frequences d'echantillonnage");
        for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
        {
            TablePublisher publisher;
            SynthEngine engine (publisher);
            engine.prepare (sr);
            publisher.publish (WavetableSet::create (makeBasicShape (BasicShape::sine)));
            engine.noteOn (69, 1.0f); // La 440 Hz
            expectWithinAbsoluteError (countRisingZeroCrossings (renderEngine (engine, (int) sr, 512)), 440, 1);
        }

        beginTest ("Pitch bend");
        {
            TablePublisher publisher;
            SynthEngine engine (publisher);
            engine.prepare (48000.0);
            engine.setPitchBendRange (12.0f);
            publisher.publish (WavetableSet::create (makeBasicShape (BasicShape::sine)));
            const std::uint8_t bendUp[] = { 0xe0, 0x7f, 0x7f };
            engine.handleMidi (bendUp, 3);
            engine.noteOn (69, 1.0f);
            expectWithinAbsoluteError (countRisingZeroCrossings (renderEngine (engine, 48000, 512)), 880, 2);
        }

        beginTest ("Velocite");
        {
            float peaks[2] {};
            for (int k = 0; k < 2; ++k)
            {
                TablePublisher publisher;
                SynthEngine engine (publisher);
                engine.prepare (48000.0);
                publisher.publish (WavetableSet::create (makeBasicShape (BasicShape::sine)));
                engine.noteOn (60, k == 0 ? 1.0f : 0.5f);
                peaks[k] = peak (renderEngine (engine, 4800, 256));
            }
            expectWithinAbsoluteError (peaks[1] / peaks[0], 0.5f, 0.01f);
        }

        beginTest ("16 voix maximum avec vol de voix");
        {
            TablePublisher publisher;
            SynthEngine engine (publisher);
            engine.prepare (48000.0);
            publisher.publish (WavetableSet::create (makeBasicShape (BasicShape::saw)));
            for (int n = 0; n < 20; ++n)
            {
                engine.noteOn (40 + n, 0.8f);
                renderEngine (engine, 64, 64);
            }
            expectEquals (engine.getNumActiveVoices(), SynthEngine::kNumVoices);
            const auto out = renderEngine (engine, 4800, 256);
            for (float v : out)
                expect (std::isfinite (v));
        }

        beginTest ("Note off puis silence complet");
        {
            TablePublisher publisher;
            SynthEngine engine (publisher);
            engine.prepare (48000.0);
            engine.setEnvelope (0.001f, 0.1f, 0.8f, 0.05f);
            publisher.publish (WavetableSet::create (makeBasicShape (BasicShape::square)));
            engine.noteOn (60, 1.0f);
            expectGreaterThan (peak (renderEngine (engine, 4800, 256)), 0.1f);
            engine.noteOff (60);
            renderEngine (engine, 4800, 256);
            expectEquals (engine.getNumActiveVoices(), 0);
            expectEquals (peak (renderEngine (engine, 1000, 256)), 0.0f);
        }

        beginTest ("Pedale de sustain");
        {
            TablePublisher publisher;
            SynthEngine engine (publisher);
            engine.prepare (48000.0);
            engine.setEnvelope (0.001f, 0.1f, 0.8f, 0.05f);
            publisher.publish (WavetableSet::create (makeBasicShape (BasicShape::sine)));
            engine.setSustainPedal (true);
            engine.noteOn (60, 1.0f);
            renderEngine (engine, 480, 256);
            engine.noteOff (60);
            renderEngine (engine, 9600, 256);
            expectEquals (engine.getNumActiveVoices(), 1);
            engine.setSustainPedal (false);
            renderEngine (engine, 9600, 256);
            expectEquals (engine.getNumActiveVoices(), 0);
        }

        beginTest ("Resultat identique quelle que soit la taille des blocs");
        {
            std::vector<float> results[3];
            const int blockSizes[] = { 1, 37, 2048 };
            for (int k = 0; k < 3; ++k)
            {
                TablePublisher publisher;
                SynthEngine engine (publisher);
                engine.prepare (44100.0);
                publisher.publish (WavetableSet::create (makeBasicShape (BasicShape::triangle)));
                engine.noteOn (64, 0.9f);
                engine.noteOn (67, 0.7f);
                results[k] = renderEngine (engine, 10000, blockSizes[k]);
            }
            expect (results[0] == results[1]);
            expect (results[0] == results[2]);
        }

        beginTest ("Changement de waveform pendant une note : pas de clic");
        {
            TablePublisher publisher;
            SynthEngine engine (publisher);
            engine.prepare (48000.0);
            engine.setEnvelope (0.001f, 0.01f, 1.0f, 0.1f);
            const auto sine = makeBasicShape (BasicShape::sine);
            publisher.publish (WavetableSet::create (sine));
            engine.noteOn (69, 1.0f);
            auto before = renderEngine (engine, 4800, 256);

            // Une waveform en opposition de phase : sans fondu, le signal sauterait d'un coup.
            WaveformData inverted {};
            for (size_t i = 0; i < inverted.size(); ++i)
                inverted[i] = -sine[i];
            publisher.publish (WavetableSet::create (inverted));

            auto after = renderEngine (engine, 4800, 256);
            expect (! engine.isCrossfading());

            float maxStep = std::abs (after.front() - before.back());
            for (size_t i = 1; i < after.size(); ++i)
                maxStep = std::max (maxStep, std::abs (after[i] - after[i - 1]));

            // Pente maximale d'un sinus à 440 Hz d'amplitude 0,25 : ~0,0144 par échantillon.
            expectLessThan (maxStep, 0.02f);
            publisher.collectGarbage();
        }

        beginTest ("Publications rapides en rafale");
        {
            TablePublisher publisher;
            SynthEngine engine (publisher);
            engine.prepare (48000.0);
            engine.noteOn (60, 1.0f);
            for (int i = 0; i < 500; ++i)
            {
                publisher.publish (WavetableSet::create (makeBasicShape (i % 2 == 0 ? BasicShape::saw : BasicShape::square)));
                const auto out = renderEngine (engine, 128, 128);
                for (float v : out)
                    expect (std::isfinite (v) && std::abs (v) <= 1.0f);
                publisher.collectGarbage();
            }
        }
    }
};

static WavetableTests wavetableTests;
static EnvelopeTests envelopeTests;
static SynthEngineTests synthEngineTests;
