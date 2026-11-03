#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"
#include "dsp/WaveShapes.h"

using namespace wavesketch;

WaveSketchProcessor::WaveSketchProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", params::createLayout())
{
    attackParam    = parameters.getRawParameterValue (params::attack);
    decayParam     = parameters.getRawParameterValue (params::decay);
    sustainParam   = parameters.getRawParameterValue (params::sustain);
    releaseParam   = parameters.getRawParameterValue (params::release);
    gainParam      = parameters.getRawParameterValue (params::gain);
    bendRangeParam = parameters.getRawParameterValue (params::bendRange);

    // Première table construite tout de suite, pour que le son soit prêt dès le premier bloc.
    waveform = makeBasicShape (BasicShape::sine);
    tablePublisher.publish (WavetableSet::create (waveform));

    keyboardState.addListener (this);
    startTimerHz (10); // ramasse-miettes des anciennes tables
}

WaveSketchProcessor::~WaveSketchProcessor()
{
    stopTimer();
    keyboardState.removeListener (this);
}

void WaveSketchProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);

    masterGain.reset (sampleRate, 0.05); // 50 ms pour suivre un changement de volume
    masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (gainParam->load()));
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

void WaveSketchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (buffer.getNumChannels() == 0 || numSamples == 0)
        return;

    engine.setEnvelope (attackParam->load(), decayParam->load(), sustainParam->load(), releaseParam->load());
    engine.setPitchBendRange (bendRangeParam->load());
    masterGain.setTargetValue (juce::Decibels::decibelsToGain (gainParam->load()));

    // Notes jouées à la souris sur le clavier virtuel
    {
        const auto scope = keyboardFifo.read (keyboardFifo.getNumReady());
        scope.forEach ([this] (int index)
        {
            engine.handleMidi (keyboardEvents[static_cast<size_t> (index)].bytes, 3);
        });
    }

    // Le bloc est découpé aux positions des événements MIDI : chaque note démarre
    // exactement à l'échantillon prévu par l'hôte, quelle que soit la taille du buffer.
    float* mono = buffer.getWritePointer (0);
    int position = 0;

    for (const auto metadata : midi)
    {
        const int eventPosition = juce::jlimit (0, numSamples, metadata.samplePosition);

        if (eventPosition > position)
        {
            engine.render (mono + position, eventPosition - position);
            position = eventPosition;
        }

        engine.handleMidi (metadata.data, metadata.numBytes);
    }

    if (position < numSamples)
        engine.render (mono + position, numSamples - position);

    masterGain.applyGain (mono, numSamples);

    for (int channel = 1; channel < buffer.getNumChannels(); ++channel)
        buffer.copyFrom (channel, 0, mono, numSamples);
}

juce::AudioProcessorEditor* WaveSketchProcessor::createEditor()
{
    return new WaveSketchEditor (*this);
}

void WaveSketchProcessor::setWaveform (const WaveformData& newWaveform)
{
    waveform = newWaveform;
    tableBuilder.requestBuild (waveform); // calcul en arrière-plan, puis publication vers l'audio
    sendChangeMessage();
}

void WaveSketchProcessor::timerCallback()
{
    tablePublisher.collectGarbage();
}

void WaveSketchProcessor::handleNoteOn (juce::MidiKeyboardState*, int, int note, float velocity)
{
    pushKeyboardEvent (0x90, note, velocity);
}

void WaveSketchProcessor::handleNoteOff (juce::MidiKeyboardState*, int, int note, float)
{
    pushKeyboardEvent (0x80, note, 0.0f);
}

void WaveSketchProcessor::pushKeyboardEvent (std::uint8_t status, int note, float velocity)
{
    const KeyboardEvent event { { status,
                                  static_cast<std::uint8_t> (juce::jlimit (0, 127, note)),
                                  static_cast<std::uint8_t> (juce::jlimit (status == 0x90 ? 1 : 0, 127, juce::roundToInt (velocity * 127.0f))) } };

    const auto scope = keyboardFifo.write (1);
    scope.forEach ([this, &event] (int index) { keyboardEvents[static_cast<size_t> (index)] = event; });
}

void WaveSketchProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = parameters.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void WaveSketchProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

// Point d'entrée appelé par le wrapper JUCE (VST3 ou Standalone).
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveSketchProcessor();
}
