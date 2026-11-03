#include "Wavetable.h"

namespace wavesketch
{

std::unique_ptr<WavetableSet> WavetableSet::create (const WaveformData& waveform)
{
    std::unique_ptr<WavetableSet> set (new WavetableSet());

    // Étape 2 : une seule table, copie brute du dessin (pas encore d'anti-aliasing).
    set->levels.emplace_back (waveform.begin(), waveform.end());
    return set;
}

const float* WavetableSet::getTableFor (double /*phaseIncrement*/) const noexcept
{
    return levels.front().data();
}

} // namespace wavesketch
