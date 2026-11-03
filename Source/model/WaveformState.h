#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "dsp/Wavetable.h"

#include <optional>
#include <vector>

namespace wavesketch
{

enum class EditMode
{
    freehand, // dessin libre échantillon par échantillon
    points    // points de contrôle reliés par une courbe lisse
};

// Un point de contrôle : x = position dans la période [0, 1), y = valeur [-1, 1].
struct ControlPoint
{
    float x = 0.0f;
    float y = 0.0f;

    bool operator== (const ControlPoint& other) const noexcept { return x == other.x && y == other.y; }
};

// Tout ce qui décrit la waveform éditée : c'est ce qui est sauvegardé dans le projet
// et dans les fichiers, et ce que l'annulation (Ctrl+Z) restaure.
struct WaveformState
{
    static constexpr int kMinPoints = 2;
    static constexpr int kMaxPoints = 64;
    static constexpr int kPointsWhenConverting = 16;

    EditMode mode = EditMode::freehand;
    WaveformData samples {};          // la courbe affichée et jouée (en mode points : calculée depuis les points)
    std::vector<ControlPoint> points; // triés par x

    bool operator== (const WaveformState& other) const noexcept
    {
        return mode == other.mode && samples == other.samples && points == other.points;
    }

    bool operator!= (const WaveformState& other) const noexcept { return ! operator== (other); }

    static WaveformState fromSamples (const WaveformData& samples);
    static WaveformState fromPoints (std::vector<ControlPoint> points);

    // Change de mode. Vers le mode points : réutilise les points existants s'ils correspondent
    // encore au dessin, sinon en crée de nouveaux en échantillonnant la courbe actuelle.
    WaveformState withMode (EditMode newMode) const;

    // Sérialisation (projet Ableton et fichiers .wavesketch). Les nombres sont stockés en binaire
    // (base64) pour une restauration exacte, au bit près.
    juce::ValueTree toValueTree() const;
    static std::optional<WaveformState> fromValueTree (const juce::ValueTree&);

    static inline const juce::Identifier treeType { "WAVEFORM" };
};

// Courbe lisse et périodique passant par les points (Hermite cubique monotone, méthode de
// Fritsch-Carlson) : la courbe ne dépasse jamais les points, donc pas de "bosses" parasites.
WaveformData renderPoints (const std::vector<ControlPoint>& points);

// Trie, borne et dédoublonne une liste de points.
std::vector<ControlPoint> sanitisePoints (std::vector<ControlPoint> points);

// Fichiers de waveform (.wavesketch, format XML).
inline constexpr const char* kWaveformFileExtension = ".wavesketch";
juce::Result saveWaveformFile (const WaveformState&, const juce::File&);
std::optional<WaveformState> loadWaveformFile (const juce::File&, juce::String& errorMessage);

} // namespace wavesketch
