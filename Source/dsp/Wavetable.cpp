#include "Wavetable.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>

namespace wavesketch
{

namespace
{
    constexpr int kFftOrder = 11;
    static_assert ((1 << kFftOrder) == kTableSize);
}

std::unique_ptr<WavetableSet> WavetableSet::create (const WaveformData& waveform)
{
    std::unique_ptr<WavetableSet> set (new WavetableSet());
    set->tables.assign (static_cast<size_t> (kNumLevels) * kTableSize, 0.0f);

    const juce::dsp::FFT fft (kFftOrder);

    // 1. Spectre de la waveform : spectrum[2k] et spectrum[2k + 1] = partie réelle et imaginaire
    //    de l'harmonique k (k = 0 .. 1024).
    std::vector<float> spectrum (2 * kTableSize, 0.0f);
    std::copy (waveform.begin(), waveform.end(), spectrum.begin());
    fft.performRealOnlyForwardTransform (spectrum.data(), true);

    // 2. Retrait du DC : l'harmonique 0 est la valeur moyenne de la courbe.
    //    Un décalage permanent n'est pas audible mais gaspille de la marge et peut provoquer des clics.
    spectrum[0] = 0.0f;
    spectrum[1] = 0.0f;

    // 3. Une table par octave : on garde les harmoniques 1..maxHarmonic, on efface les autres,
    //    puis FFT inverse pour revenir à une forme d'onde.
    std::vector<float> work (2 * kTableSize);

    for (int level = 0; level < kNumBandLimitedLevels; ++level)
    {
        const int maxHarmonic = getMaxHarmonic (level);

        std::fill (work.begin(), work.end(), 0.0f);
        std::copy (spectrum.begin(), spectrum.begin() + 2 * (maxHarmonic + 1), work.begin());
        fft.performRealOnlyInverseTransform (work.data());

        std::copy (work.begin(), work.begin() + kTableSize,
                   set->tables.begin() + static_cast<std::ptrdiff_t> (level) * kTableSize);
    }

    // 4. Normalisation : le pic de la version la plus riche est ramené à 1. Le même gain est
    //    appliqué à toutes les octaves pour que le volume reste constant d'une note à l'autre.
    const auto level0 = set->tables.begin();
    float peak = 0.0f;
    std::for_each (level0, level0 + kTableSize, [&peak] (float v) { peak = std::max (peak, std::abs (v)); });

    // Gain plafonné à +40 dB : un dessin quasiment plat reste discret au lieu de faire
    // ressortir à plein volume des irrégularités invisibles.
    constexpr float maxGain = 100.0f;
    set->normalisationGain = peak > 1.0e-6f ? std::min (1.0f / peak, maxGain) : 0.0f;

    for (auto& v : set->tables)
        v *= set->normalisationGain;

    return set;
}

} // namespace wavesketch
