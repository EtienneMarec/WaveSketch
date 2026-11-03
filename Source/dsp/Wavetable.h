#pragma once

#include <array>
#include <memory>
#include <vector>

namespace wavesketch
{

// Nombre d'échantillons dans une période de la waveform.
// Une puissance de 2 permet de "boucler" un index avec un simple masque binaire (i & kTableMask).
constexpr int kTableSize = 2048;
constexpr int kTableMask = kTableSize - 1;

// La waveform telle que dessinée par l'utilisateur, valeurs dans [-1, 1].
using WaveformData = std::array<float, kTableSize>;

// Lit la table à une position fractionnaire. phase ∈ [0, 1) représente une période complète.
// Interpolation d'Hermite cubique (4 points) : beaucoup plus propre qu'une interpolation linéaire
// pour un coût négligeable, ce qui réduit le bruit de "marches d'escalier" entre deux échantillons.
inline float readTable (const float* table, double phase) noexcept
{
    const double pos = phase * kTableSize;
    const int i = static_cast<int> (pos);
    const float f = static_cast<float> (pos - i);

    const float xm1 = table[(i - 1) & kTableMask];
    const float x0  = table[i & kTableMask];
    const float x1  = table[(i + 1) & kTableMask];
    const float x2  = table[(i + 2) & kTableMask];

    const float c1 = 0.5f * (x1 - xm1);
    const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

// Ensemble de tables prêtes à être jouées par le thread audio.
//
// Anti-aliasing par "mipmaps" : un son numérique ne peut pas contenir de fréquence au-dessus
// de la moitié de la fréquence d'échantillonnage (Nyquist). Une harmonique qui dépasse se
// "replie" en une fréquence parasite, non musicale : c'est l'aliasing. On prépare donc, par FFT,
// une version de la waveform par octave, dont on a retiré les harmoniques trop aiguës.
// Chaque voix choisit la version qui contient le plus d'harmoniques sans dépasser Nyquist.
//
// L'ensemble est construit hors du thread audio puis n'est plus jamais modifié : le thread audio
// peut le lire sans verrou. Pour changer de son, on publie un nouvel ensemble (TablePublisher).
class WavetableSet
{
public:
    // Niveau 0 : 512 harmoniques. Chaque niveau suivant en garde deux fois moins (une octave),
    // jusqu'à 1 seule (sinus pur). Le dernier niveau est silencieux : il sert quand même la
    // fondamentale dépasserait Nyquist (note extrêmement aiguë + pitch bend).
    // 512 et non 1024 : la table reste suréchantillonnée 2x, ce qui garde l'interpolation précise.
    static constexpr int kMaxHarmonics = 512;
    static constexpr int kNumBandLimitedLevels = 10;
    static constexpr int kNumLevels = kNumBandLimitedLevels + 1;

    // Construit les tables : retrait de la composante continue (DC), limitation de bande par octave,
    // puis normalisation du niveau. La waveform reçue (le dessin affiché) n'est pas modifiée.
    static std::unique_ptr<WavetableSet> create (const WaveformData& waveform);

    static constexpr int getMaxHarmonic (int level) noexcept
    {
        return level < kNumBandLimitedLevels ? (kMaxHarmonics >> level) : 0;
    }

    // Niveau à utiliser pour une note dont la phase avance de `phaseIncrement`
    // (fréquence / fréquence d'échantillonnage) à chaque échantillon.
    static int getLevelFor (double phaseIncrement) noexcept
    {
        // L'harmonique h a pour fréquence h × f : elle doit rester sous Nyquist, soit h × inc ≤ 0,5.
        for (int level = 0; level < kNumBandLimitedLevels; ++level)
            if (getMaxHarmonic (level) * phaseIncrement <= 0.5)
                return level;

        return kNumLevels - 1;
    }

    const float* getLevel (int level) const noexcept
    {
        return tables.data() + static_cast<size_t> (level) * kTableSize;
    }

    // Gain appliqué pour amener le pic du signal à 1 (0 si la waveform est silencieuse).
    float getNormalisationGain() const noexcept { return normalisationGain; }

private:
    WavetableSet() = default;

    std::vector<float> tables; // kNumLevels tables de kTableSize échantillons, à la suite
    float normalisationGain = 1.0f;
};

} // namespace wavesketch
