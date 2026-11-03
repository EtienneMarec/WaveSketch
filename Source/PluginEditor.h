#pragma once

#include "PluginProcessor.h"

class WaveSketchEditor final : public juce::AudioProcessorEditor
{
public:
    explicit WaveSketchEditor (WaveSketchProcessor&);
    ~WaveSketchEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WaveSketchProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveSketchEditor)
};
