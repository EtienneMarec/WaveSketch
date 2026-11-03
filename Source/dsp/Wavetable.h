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
// Il est construit hors du thread audio puis n'est plus jamais modifié : le thread audio
// peut donc le lire sans verrou. Pour changer de son, on publie un nouvel ensemble
// (voir TablePublisher) au lieu de modifier celui-ci.
class WavetableSet
{
public:
    static std::unique_ptr<WavetableSet> create (const WaveformData& waveform);

    // Renvoie la table à utiliser pour une note dont la phase avance de
    // `phaseIncrement` (fréquence / fréquence d'échantillonnage) à chaque échantillon.
    const float* getTableFor (double phaseIncrement) const noexcept;

private:
    WavetableSet() = default;

    std::vector<std::vector<float>> levels;
};

} // namespace wavesketch
