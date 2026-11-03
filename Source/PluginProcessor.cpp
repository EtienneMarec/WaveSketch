#include "PluginProcessor.h"
#include "PluginEditor.h"

WaveSketchProcessor::WaveSketchProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void WaveSketchProcessor::prepareToPlay (double, int)
{
}

void WaveSketchProcessor::releaseResources()
{
}

bool WaveSketchProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Un synthé n'a pas d'entrée audio ; on accepte une sortie mono ou stéréo.
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void WaveSketchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
}

juce::AudioProcessorEditor* WaveSketchProcessor::createEditor()
{
    return new WaveSketchEditor (*this);
}

void WaveSketchProcessor::getStateInformation (juce::MemoryBlock&)
{
}

void WaveSketchProcessor::setStateInformation (const void*, int)
{
}

// Point d'entrée appelé par le wrapper JUCE (VST3 ou Standalone).
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveSketchProcessor();
}
