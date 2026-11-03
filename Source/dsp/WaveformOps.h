#pragma once

#include "Wavetable.h"

// Transformations appliquées au dessin de la waveform (côté interface, jamais dans le thread audio).
namespace wavesketch::ops
{

// Trace un segment entre deux positions (index fractionnaire dans la table, valeur dans [-1, 1])
// en remplissant TOUS les échantillons intermédiaires : même si la souris saute de
// plusieurs pixels entre deux événements, il ne reste aucun trou dans le dessin.
void drawLine (WaveformData& data, float index0, float value0, float index1, float value1) noexcept;

// Lissage circulaire (moyenne glissante appliquée deux fois, la fin de période rejoignant le début).
void smooth (WaveformData& data, int radius = 8) noexcept;

// Inversion de phase (polarité) : la waveform est retournée verticalement.
void invert (WaveformData& data) noexcept;

} // namespace wavesketch::ops
