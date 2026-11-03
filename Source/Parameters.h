#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Identifiants des paramètres automatisables. Ils sont enregistrés dans les projets Ableton :
// ne jamais les renommer une fois le plugin distribué.
namespace wavesketch::params
{
    inline constexpr const char* attack    = "attack";
    inline constexpr const char* decay     = "decay";
    inline constexpr const char* sustain   = "sustain";
    inline constexpr const char* release   = "release";
    inline constexpr const char* gain      = "gain";
    inline constexpr const char* bendRange = "bendRange";

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
