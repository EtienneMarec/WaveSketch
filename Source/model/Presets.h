#pragma once

#include "WaveformState.h"

namespace wavesketch
{

struct WaveformPreset
{
    juce::String name;
    WaveformState state;
};

// Waveforms intégrées, proposées dans le menu "Presets".
const std::vector<WaveformPreset>& getBuiltInPresets();

} // namespace wavesketch
