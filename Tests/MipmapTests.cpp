#include <juce_dsp/juce_dsp.h>

#include "dsp/SynthEngine.h"
#include "dsp/WaveShapes.h"
#include "dsp/Wavetable.h"

#include <cmath>
#include <vector>

using namespace wavesketch;

namespace
{
    constexpr double twoPi = 6.283185307179586;

    // Magnitude de chaque harmonique (0 .. kTableSize/2) d'une table.
    std::vector<float> tableSpectrum (const float* table)
    {
        juce::dsp::FFT fft (11);
        std::vector<float> data (2 * kTableSize, 0.0f);
        std::copy (table, table + kTableSize, data.begin());
        fft.performFrequencyOnlyForwardTransform (data.data(), true);
        data.resize (kTableSize / 2 + 1);
        return data;
    }

    struct SpectrumReport
    {
        double worstNonHarmonicDb = 0.0; // pic le plus fort hors harmoniques, relatif à la fondamentale
        double thirdHarmonicDb = 0.0;
    };

    // Analyse spectrale d'un signal : tout ce qui ne tombe pas sur un multiple de la fondamentale
    // est de l'aliasing (ou du bruit). Fenêtre de Blackman-Harris pour des lobes secondaires < -90 dB.
    SpectrumReport analyse (const std::vector<float>& signal, double sampleRate, double fundamental)
    {
        constexpr int order = 15;
        constexpr int size = 1 << order;
        jassert ((int) signal.size() >= size);

        std::vector<float> data (2 * size, 0.0f);
        for (int n = 0; n < size; ++n)
        {
            const double x = twoPi * n / size;
            const double w = 0.35875 - 0.48829 * std::cos (x) + 0.14128 * std::cos (2 * x) - 0.01168 * std::cos (3 * x);
            data[(size_t) n] = (float) (signal[(size_t) n] * w);
        }

        juce::dsp::FFT fft (order);
        fft.performFrequencyOnlyForwardTransform (data.data(), true);

        const double binHz = sampleRate / size;
        const auto magnitudeNear = [&] (double freq)
        {
            const int centre = (int) std::lround (freq / binHz);
            float m = 0.0f;
            for (int b = std::max (1, centre - 4); b <= std::min (size / 2, centre + 4); ++b)
                m = std::max (m, data[(size_t) b]);
            return m;
        };

        const float fundamentalMag = magnitudeNear (fundamental);
        float worst = 0.0f;

        for (int b = 1; b < size / 2; ++b)
        {
            const double freq = b * binHz;
            if (freq < 20.0)
                continue;

            const double k = std::round (freq / fundamental);
            if (k >= 1.0 && std::abs (freq - k * fundamental) < 8.0 * binHz)
                continue;

            worst = std::max (worst, data[(size_t) b]);
        }

        SpectrumReport r;
        r.worstNonHarmonicDb = 20.0 * std::log10 (std::max (1.0e-12f, worst) / fundamentalMag);
        r.thirdHarmonicDb = 20.0 * std::log10 (std::max (1.0e-12f, magnitudeNear (3.0 * fundamental)) / fundamentalMag);
        return r;
    }

    std::vector<float> renderNote (BasicShape shape, int note, double sampleRate, int numSamples)
    {
        TablePublisher publisher;
        SynthEngine engine (publisher);
        engine.prepare (sampleRate);
        engine.setEnvelope (0.001f, 0.01f, 1.0f, 0.1f);
        publisher.publish (WavetableSet::create (makeBasicShape (shape)));
        engine.noteOn (note, 1.0f);

        std::vector<float> warmup (4096);
        engine.render (warmup.data(), (int) warmup.size()); // on ignore l'attaque

        std::vector<float> out ((size_t) numSamples);
        engine.render (out.data(), numSamples);
        return out;
    }
}

class MipmapTests final : public juce::UnitTest
{
public:
    MipmapTests() : juce::UnitTest ("Anti-aliasing, DC et normalisation", "WaveSketch") {}

    void runTest() override
    {
        beginTest ("Chaque octave ne contient aucune harmonique au-dessus de sa limite");
        {
            const auto set = WavetableSet::create (makeBasicShape (BasicShape::saw));
            for (int level = 0; level < WavetableSet::kNumLevels; ++level)
            {
                const auto spectrum = tableSpectrum (set->getLevel (level));
                const int maxHarmonic = WavetableSet::getMaxHarmonic (level);
                float inBand = 0.0f, outOfBand = 0.0f;
                for (int h = 1; h < (int) spectrum.size(); ++h)
                    (h <= maxHarmonic ? inBand : outOfBand) = std::max (h <= maxHarmonic ? inBand : outOfBand, spectrum[(size_t) h]);

                if (maxHarmonic > 0)
                    expectLessThan (outOfBand / inBand, 1.0e-4f);
                else
                    expectLessThan (outOfBand, 1.0e-6f); // niveau silencieux
            }
        }

        beginTest ("Les harmoniques conservees sont intactes");
        {
            const auto saw = makeBasicShape (BasicShape::saw);
            const auto set = WavetableSet::create (saw);
            const auto original = tableSpectrum (saw.data());
            const auto level0 = tableSpectrum (set->getLevel (0));
            for (int h = 1; h <= WavetableSet::kMaxHarmonics; ++h)
                expectWithinAbsoluteError (level0[(size_t) h] / set->getNormalisationGain(), original[(size_t) h],
                                           original[1] * 1.0e-4f);
        }

        beginTest ("Retrait du DC sur un dessin decale vers le haut");
        {
            WaveformData offset {};
            for (int i = 0; i < kTableSize; ++i)
                offset[(size_t) i] = 0.6f + 0.3f * (float) std::sin (twoPi * i / kTableSize);

            const auto copy = offset;
            const auto set = WavetableSet::create (offset);
            expect (offset == copy); // le dessin n'est pas modifié

            for (int level = 0; level < WavetableSet::kNumLevels; ++level)
            {
                double mean = 0.0;
                for (int i = 0; i < kTableSize; ++i)
                    mean += set->getLevel (level)[i];
                expectWithinAbsoluteError (mean / kTableSize, 0.0, 1.0e-5);
            }
        }

        beginTest ("Normalisation : un petit dessin atteint le niveau plein");
        {
            WaveformData quiet {};
            for (int i = 0; i < kTableSize; ++i)
                quiet[(size_t) i] = 0.1f * (float) std::sin (twoPi * i / kTableSize);

            const auto set = WavetableSet::create (quiet);
            float peak = 0.0f;
            for (int i = 0; i < kTableSize; ++i)
                peak = std::max (peak, std::abs (set->getLevel (0)[i]));
            expectWithinAbsoluteError (peak, 1.0f, 1.0e-4f);
            expectWithinAbsoluteError (set->getNormalisationGain(), 10.0f, 1.0e-2f);
        }

        beginTest ("Dessin plat ou constant : silence, sans NaN");
        for (float constant : { 0.0f, 0.7f, -1.0f })
        {
            WaveformData flat {};
            flat.fill (constant);
            const auto set = WavetableSet::create (flat);
            for (int level = 0; level < WavetableSet::kNumLevels; ++level)
                for (int i = 0; i < kTableSize; ++i)
                    expect (std::abs (set->getLevel (level)[i]) < 1.0e-6f);
        }

        beginTest ("Choix de l'octave : jamais au-dessus de Nyquist, et le plus riche possible");
        for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
        {
            for (double note = -24.0; note <= 151.0; note += 0.25)
            {
                const double freq = 440.0 * std::exp2 ((note - 69.0) / 12.0);
                const int level = WavetableSet::getLevelFor (freq / sr);
                expectLessOrEqual (WavetableSet::getMaxHarmonic (level) * freq, sr / 2.0);
                if (level > 0)
                    expectGreaterThan (WavetableSet::getMaxHarmonic (level - 1) * freq, sr / 2.0);
            }
        }

        beginTest ("Carre joue dans l'aigu : aucun aliasing mesurable (sortie du moteur)");
        for (auto [note, sr] : { std::pair { 84, 48000.0 }, std::pair { 96, 48000.0 }, std::pair { 108, 48000.0 },
                                 std::pair { 100, 44100.0 }, std::pair { 112, 96000.0 } })
        {
            const double freq = 440.0 * std::exp2 ((note - 69.0) / 12.0);
            const auto report = analyse (renderNote (BasicShape::square, note, sr, 1 << 15), sr, freq);
            logMessage ("  note " + juce::String (note) + " @ " + juce::String (sr) + " Hz : pire composante hors harmonique "
                        + juce::String (report.worstNonHarmonicDb, 1) + " dB");
            expectLessThan (report.worstNonHarmonicDb, -70.0);

            if (3.0 * freq < sr / 2.0)
                expectWithinAbsoluteError (report.thirdHarmonicDb, 20.0 * std::log10 (1.0 / 3.0), 1.0); // carré : h3 = 1/3
        }

        beginTest ("Controle : un carre naif (sans anti-aliasing) est bien detecte");
        {
            constexpr double sr = 48000.0;
            const double freq = 440.0 * std::exp2 ((96 - 69.0) / 12.0);
            std::vector<float> naive (1 << 15);
            double phase = 0.0;
            for (auto& v : naive)
            {
                v = phase < 0.5 ? 0.25f : -0.25f;
                phase += freq / sr;
                phase -= std::floor (phase);
            }
            const auto report = analyse (naive, sr, freq);
            logMessage ("  carre naif : " + juce::String (report.worstNonHarmonicDb, 1) + " dB");
            expectGreaterThan (report.worstNonHarmonicDb, -40.0);
        }
    }
};

static MipmapTests mipmapTests;
