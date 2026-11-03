#include <juce_audio_processors/juce_audio_processors.h>

#include "Parameters.h"
#include "PluginProcessor.h"
#include "dsp/WaveShapes.h"
#include "model/Presets.h"
#include "model/WaveformState.h"

#include <cmath>
#include <limits>

using namespace wavesketch;

namespace
{
    WaveformState examplePointsState()
    {
        return WaveformState::fromPoints ({ { 0.0f, 0.1f }, { 256.0f / kTableSize, 0.9f }, { 0.5f, -0.3f },
                                            { 1536.0f / kTableSize, -1.0f }, { 1800.0f / kTableSize, 0.4f } });
    }
}

//==============================================================================
class PointCurveTests final : public juce::UnitTest
{
public:
    PointCurveTests() : juce::UnitTest ("Mode points", "WaveSketch") {}

    void runTest() override
    {
        beginTest ("La courbe passe exactement par les points");
        {
            const auto state = examplePointsState();
            for (const auto& p : state.points)
                expectWithinAbsoluteError (state.samples[(size_t) std::lround (p.x * kTableSize)], p.y, 1.0e-6f);
        }

        beginTest ("Aucun depassement entre les points (courbe monotone)");
        {
            const auto state = examplePointsState();
            for (float v : state.samples)
                expect (v >= -1.0f && v <= 0.9f + 1.0e-6f);
        }

        beginTest ("Courbe periodique : la fin rejoint le debut");
        {
            const auto state = examplePointsState();
            expectLessThan (std::abs (state.samples[kTableSize - 1] - state.samples[0]), 0.01f);
            float maxStep = 0.0f;
            for (int i = 0; i < kTableSize; ++i)
                maxStep = std::max (maxStep, std::abs (state.samples[(size_t) i] - state.samples[(size_t) ((i + 1) & kTableMask)]));
            expectLessThan (maxStep, 0.02f);
        }

        beginTest ("Deux points suffisent, un seul ou aucun ne plante pas");
        {
            const auto two = WaveformState::fromPoints ({ { 0.1f, 1.0f }, { 0.6f, -1.0f } });
            expectEquals ((int) two.points.size(), 2);
            for (float v : two.samples)
                expect (std::isfinite (v));

            const auto one = renderPoints ({ { 0.3f, 0.5f } });
            expectEquals (one[100], 0.5f);
            const auto none = renderPoints ({});
            expectEquals (none[100], 0.0f);
        }

        beginTest ("Nettoyage des points : tri, doublons, valeurs invalides");
        {
            const float nan = std::numeric_limits<float>::quiet_NaN();
            const auto pts = sanitisePoints ({ { 0.7f, 0.2f }, { 0.1f, 5.0f }, { 0.7f, 0.9f }, { nan, 0.0f }, { -3.0f, -0.5f } });
            expectEquals ((int) pts.size(), 3);
            expectEquals (pts[0].x, 0.0f);
            expectEquals (pts[1].y, 1.0f);
            expect (pts[0].x < pts[1].x && pts[1].x < pts[2].x);
        }

        beginTest ("Passage en mode points depuis un dessin");
        {
            const auto sine = WaveformState::fromSamples (makeBasicShape (BasicShape::sine));
            const auto asPoints = sine.withMode (EditMode::points);
            expect (asPoints.mode == EditMode::points);
            expectEquals ((int) asPoints.points.size(), WaveformState::kPointsWhenConverting);
            for (size_t i = 0; i < sine.samples.size(); ++i)
                expectWithinAbsoluteError (asPoints.samples[i], sine.samples[i], 0.03f);

            // Aller-retour sans modification : les points sont conservés tels quels.
            const auto back = asPoints.withMode (EditMode::freehand).withMode (EditMode::points);
            expect (back == asPoints);
        }

        beginTest ("Presets integres valides");
        for (const auto& preset : getBuiltInPresets())
        {
            float peak = 0.0f;
            for (float v : preset.state.samples)
            {
                expect (v >= -1.0f && v <= 1.0f);
                peak = std::max (peak, std::abs (v));
            }
            expectGreaterThan (peak, 0.5f, preset.name);
            if (preset.state.mode == EditMode::points)
                expectGreaterOrEqual ((int) preset.state.points.size(), WaveformState::kMinPoints);
        }
    }
};

//==============================================================================
class StateTests final : public juce::UnitTest
{
public:
    StateTests() : juce::UnitTest ("Sauvegarde et restauration", "WaveSketch") {}

    void runTest() override
    {
        beginTest ("Serialisation de la waveform : restauration au bit pres");
        {
            auto state = examplePointsState();
            state.samples[17] = 0.123456789f;
            const auto xmlText = state.toValueTree().toXmlString();
            const auto restored = WaveformState::fromValueTree (juce::ValueTree::fromXml (xmlText));
            expect (restored.has_value());
            expect (*restored == state);
        }

        beginTest ("Donnees de waveform invalides refusees");
        {
            auto tree = examplePointsState().toValueTree();
            tree.setProperty ("samples", "n'importe quoi", nullptr);
            expect (! WaveformState::fromValueTree (tree).has_value());

            std::vector<float> wrongSize (100, 0.0f);
            tree.setProperty ("samples", juce::MemoryBlock (wrongSize.data(), wrongSize.size() * sizeof (float)).toBase64Encoding(), nullptr);
            expect (! WaveformState::fromValueTree (tree).has_value());

            WaveformData withNan {};
            withNan[5] = std::numeric_limits<float>::quiet_NaN();
            tree.setProperty ("samples", juce::MemoryBlock (withNan.data(), sizeof (withNan)).toBase64Encoding(), nullptr);
            expect (! WaveformState::fromValueTree (tree).has_value());

            expect (! WaveformState::fromValueTree (juce::ValueTree ("AUTRE")).has_value());
        }

        beginTest ("Fichier .wavesketch : ecriture puis relecture");
        {
            const auto file = juce::File::createTempFile (kWaveformFileExtension);
            const auto state = examplePointsState();
            expect (saveWaveformFile (state, file).wasOk());

            juce::String error;
            const auto loaded = loadWaveformFile (file, error);
            expect (loaded.has_value() && *loaded == state);

            file.replaceWithText ("ceci n'est pas du XML");
            expect (! loadWaveformFile (file, error).has_value());
            expect (error.isNotEmpty());
            file.deleteFile();
        }

        beginTest ("Projet : parametres + waveform + taille de fenetre restaures a l'identique");
        {
            WaveSketchProcessor original;
            const float values[] = { 0.37f, 0.61f, 0.25f, 0.83f, 0.42f, 0.5f };
            const char* ids[] = { params::attack, params::decay, params::sustain, params::release, params::gain, params::bendRange };
            for (int i = 0; i < 6; ++i)
                original.getParameters().getParameter (ids[i])->setValueNotifyingHost (values[i]);

            const auto state = examplePointsState();
            original.setWaveformState (state);
            original.setSavedEditorSize ({ 1111, 777 });

            juce::MemoryBlock saved;
            original.getStateInformation (saved);

            WaveSketchProcessor restored;
            restored.setStateInformation (saved.getData(), (int) saved.getSize());

            expect (restored.getWaveformState() == state);
            for (int i = 0; i < 6; ++i)
                expectEquals (restored.getParameters().getParameter (ids[i])->getValue(),
                              original.getParameters().getParameter (ids[i])->getValue(), ids[i]);
            expect (restored.getSavedEditorSize() == juce::Point<int> (1111, 777));

            juce::MemoryBlock savedAgain;
            restored.getStateInformation (savedAgain);
            expect (saved == savedAgain, "sauvegarder l'etat restaure doit redonner exactement les memes octets");

            // À l'ouverture de l'interface : la fenêtre reprend sa taille et affiche la waveform restaurée.
            std::unique_ptr<juce::AudioProcessorEditor> editor (restored.createEditorAndMakeActive());
            expect (editor != nullptr);
            expectEquals (editor->getWidth(), 1111);
            expectEquals (editor->getHeight(), 777);
            editor.reset();
            expect (restored.getWaveformState() == state);
        }

        beginTest ("Donnees de projet corrompues : aucun plantage, etat conserve");
        {
            WaveSketchProcessor processor;
            const auto before = processor.getWaveformState();
            juce::Random random (1234);

            for (int trial = 0; trial < 50; ++trial)
            {
                juce::MemoryBlock junk ((size_t) random.nextInt ({ 1, 2000 }));
                random.fillBitsRandomly (junk.getData(), junk.getSize());
                processor.setStateInformation (junk.getData(), (int) junk.getSize());
            }

            juce::MemoryBlock valid;
            processor.getStateInformation (valid);
            processor.setStateInformation (valid.getData(), (int) valid.getSize() / 2); // tronqué
            processor.setStateInformation (nullptr, 0);

            expect (processor.getWaveformState() == before);
        }

        beginTest ("Ancien format (parametres seuls) toujours lisible");
        {
            WaveSketchProcessor processor;
            auto tree = processor.getParameters().copyState();
            tree.getChildWithProperty ("id", params::sustain).setProperty ("value", 0.1f, nullptr);

            juce::MemoryBlock data;
            juce::AudioProcessor::copyXmlToBinary (*tree.createXml(), data);
            processor.setStateInformation (data.getData(), (int) data.getSize());

            expectWithinAbsoluteError (processor.getParameters().getRawParameterValue (params::sustain)->load(), 0.1f, 1.0e-6f);
        }
    }
};

//==============================================================================
class UndoTests final : public juce::UnitTest
{
public:
    UndoTests() : juce::UnitTest ("Annuler / retablir", "WaveSketch") {}

    void runTest() override
    {
        WaveSketchProcessor processor;
        auto& undo = processor.getUndoManager();
        const auto initial = processor.getWaveformState();
        const auto square = WaveformState::fromSamples (makeBasicShape (BasicShape::square));
        const auto saw = WaveformState::fromSamples (makeBasicShape (BasicShape::saw));

        beginTest ("Chaque modification est une etape");
        processor.applyWaveformChange (square, "carre");
        processor.applyWaveformChange (saw, "scie");
        undo.undo();
        expect (processor.getWaveformState() == square);
        undo.undo();
        expect (processor.getWaveformState() == initial);
        undo.redo();
        expect (processor.getWaveformState() == square);

        beginTest ("Un geste complet (plusieurs mouvements) = une seule etape");
        {
            const auto before = processor.getWaveformState();
            auto live = before;
            for (int i = 0; i < 20; ++i)
            {
                live.samples[(size_t) (i * 10)] = 0.5f;
                processor.setWaveformState (live);
            }
            processor.commitWaveformGesture (before, "trait");
            expect (processor.getWaveformState() == live);
            undo.undo();
            expect (processor.getWaveformState() == before);
            undo.redo();
            expect (processor.getWaveformState() == live);
        }

        beginTest ("Une modification sans effet n'ajoute pas d'etape");
        {
            undo.beginNewTransaction();
            processor.applyWaveformChange (processor.getWaveformState(), "rien");
            processor.commitWaveformGesture (processor.getWaveformState(), "rien");
            expectEquals (undo.getNumActionsInCurrentTransaction(), 0);
        }

        beginTest ("Charger un projet vide l'historique");
        {
            juce::MemoryBlock data;
            processor.getStateInformation (data);
            expect (undo.canUndo());
            processor.setStateInformation (data.getData(), (int) data.getSize());
            expect (! undo.canUndo());
        }
    }
};

static PointCurveTests pointCurveTests;
static StateTests stateTests;
static UndoTests undoTests;
