#pragma once

#include "Wavetable.h"

#include <juce_core/juce_core.h>

#include <atomic>

namespace wavesketch
{

// Fait passer des WavetableSet du thread de l'interface vers le thread audio, sans verrou
// et sans que le thread audio ait jamais à allouer ou libérer de mémoire.
//
//  - publish()        : thread non-audio. Dépose un nouvel ensemble dans une "boîte aux lettres"
//                       atomique. Si l'audio n'avait pas encore pris le précédent, celui-ci
//                       n'a jamais été vu par l'audio : on peut le supprimer tout de suite.
//  - takePending()    : thread audio. Récupère l'ensemble en attente (ou nullptr).
//  - retire()         : thread audio. Rend un ensemble devenu inutile via une file sans verrou.
//  - collectGarbage() : thread de l'interface (message thread) uniquement. Supprime les
//                       ensembles rendus par l'audio.
class TablePublisher
{
public:
    TablePublisher() = default;

    // À ne détruire qu'une fois le thread audio arrêté (c'est le cas à la destruction du plugin).
    ~TablePublisher()
    {
        delete pending.exchange (nullptr);
        collectGarbage();
    }

    void publish (std::unique_ptr<WavetableSet> set)
    {
        delete pending.exchange (set.release(), std::memory_order_acq_rel);
    }

    WavetableSet* takePending() noexcept
    {
        return pending.exchange (nullptr, std::memory_order_acq_rel);
    }

    // Renvoie false si la file est pleine : l'appelant garde alors l'ensemble et réessaie plus tard.
    bool retire (WavetableSet* set) noexcept
    {
        const auto scope = fifo.write (1);

        if (scope.blockSize1 > 0)
        {
            retired[static_cast<size_t> (scope.startIndex1)] = set;
            return true;
        }

        if (scope.blockSize2 > 0)
        {
            retired[static_cast<size_t> (scope.startIndex2)] = set;
            return true;
        }

        return false;
    }

    void collectGarbage()
    {
        const auto scope = fifo.read (fifo.getNumReady());
        scope.forEach ([this] (int index)
        {
            auto& slot = retired[static_cast<size_t> (index)];
            delete slot;
            slot = nullptr;
        });
    }

private:
    static constexpr int kRetireCapacity = 64;

    std::atomic<WavetableSet*> pending { nullptr };
    juce::AbstractFifo fifo { kRetireCapacity };
    std::array<WavetableSet*, kRetireCapacity> retired {};

    static_assert (std::atomic<WavetableSet*>::is_always_lock_free);

    JUCE_DECLARE_NON_COPYABLE (TablePublisher)
};

} // namespace wavesketch
