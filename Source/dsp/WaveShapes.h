#pragma once

#include "Wavetable.h"

namespace wavesketch
{

enum class BasicShape
{
    sine,
    triangle,
    saw,
    square
};

// Génère une période de forme de base. Toutes commencent à 0 en montant,
// sauf le carré qui démarre directement en haut.
WaveformData makeBasicShape (BasicShape shape);

} // namespace wavesketch
