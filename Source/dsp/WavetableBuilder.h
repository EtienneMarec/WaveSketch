#pragma once

#include "TablePublisher.h"

#include <juce_core/juce_core.h>

#include <mutex>

namespace wavesketch
{

// Thread d'arrière-plan qui transforme une waveform dessinée en WavetableSet puis la publie.
// L'interface reste fluide pendant le dessin, et le thread audio ne fait aucun calcul lourd.
// Si plusieurs demandes arrivent pendant un calcul, seule la plus récente est traitée.
class WavetableBuilder final : private juce::Thread
{
public:
    explicit WavetableBuilder (TablePublisher& publisherToUse)
        : juce::Thread ("WaveSketch table builder"), publisher (publisherToUse)
    {
        startThread (juce::Thread::Priority::low);
    }

    ~WavetableBuilder() override
    {
        signalThreadShouldExit();
        wakeUp.signal();
        stopThread (4000);
    }

    // Appelée depuis l'interface. Ne bloque que le temps de copier 2048 valeurs.
    void requestBuild (const WaveformData& waveform)
    {
        {
            const std::scoped_lock lock (mutex);
            pendingWaveform = waveform;
            hasPendingRequest = true;
        }

        wakeUp.signal();
    }

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            wakeUp.wait (-1);

            for (;;)
            {
                WaveformData waveform;
                {
                    const std::scoped_lock lock (mutex);
                    if (! hasPendingRequest)
                        break;

                    waveform = pendingWaveform;
                    hasPendingRequest = false;
                }

                if (threadShouldExit())
                    return;

                publisher.publish (WavetableSet::create (waveform));
            }
        }
    }

    TablePublisher& publisher;
    juce::WaitableEvent wakeUp;
    std::mutex mutex; // partagé uniquement entre l'interface et ce thread, jamais avec l'audio
    WaveformData pendingWaveform {};
    bool hasPendingRequest = false;

    JUCE_DECLARE_NON_COPYABLE (WavetableBuilder)
};

} // namespace wavesketch
