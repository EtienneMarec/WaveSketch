#include "Parameters.h"

namespace wavesketch::params
{

namespace
{
    juce::String formatSeconds (float seconds, int)
    {
        return seconds < 1.0f ? juce::String (juce::roundToInt (seconds * 1000.0f)) + " ms"
                              : juce::String (seconds, 2) + " s";
    }

    // Saisie au clavier : "250 ms", "0.25", "1.5 s"... (sans unité = secondes)
    float parseSeconds (const juce::String& text)
    {
        const auto value = text.retainCharacters ("0123456789.,-").replaceCharacter (',', '.').getFloatValue();
        return text.containsIgnoreCase ("ms") ? value / 1000.0f : value;
    }

    std::unique_ptr<juce::AudioParameterFloat> makeTime (const char* id, const char* name,
                                                         float minSec, float maxSec, float centreSec, float defaultSec)
    {
        juce::NormalisableRange<float> range (minSec, maxSec);
        range.setSkewForCentre (centreSec); // plus de précision sur les temps courts

        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, range, defaultSec,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction (formatSeconds)
                .withValueFromStringFunction (parseSeconds));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (makeTime (attack,  "Attack",  0.001f, 5.0f,  0.25f, 0.005f));
    layout.add (makeTime (decay,   "Decay",   0.001f, 5.0f,  0.5f,  0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { sustain, 1 }, "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; })));

    layout.add (makeTime (release, "Release", 0.005f, 10.0f, 0.6f,  0.3f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { gain, 1 }, "Volume", juce::NormalisableRange<float> (-48.0f, 6.0f, 0.0f, 2.0f), -6.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })));

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { bendRange, 1 }, "Pitch Bend Range", 0, 24, 2,
        juce::AudioParameterIntAttributes().withLabel ("st")));

    return layout;
}

} // namespace wavesketch::params
