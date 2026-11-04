#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "model/Presets.h"

// Rendu de l'interface dans des fichiers PNG, à plusieurs tailles et échelles d'affichage
// (100 %, 125 %, 150 %), pour vérifier la mise en page sans ouvrir de DAW.
// Utilisation : WaveSketchTests.exe --snapshots <dossier>
int renderSnapshots (const juce::File& folder)
{
    folder.createDirectory();

    struct Case { const char* name; int width, height; float scale; bool pointsMode; };
    const Case cases[] = {
        { "defaut_100",   980,  660, 1.0f,  false },
        { "defaut_125",   980,  660, 1.25f, false },
        { "defaut_150",   980,  660, 1.5f,  true  },
        { "minimum_100",  820,  560, 1.0f,  true  },
        { "minimum_150",  820,  560, 1.5f,  false },
        { "grand_100",   1600, 1000, 1.0f,  true  },
    };

    int written = 0;

    for (const auto& c : cases)
    {
        WaveSketchProcessor processor;

        for (const auto& preset : wavesketch::getBuiltInPresets())
            if (preset.state.mode == (c.pointsMode ? wavesketch::EditMode::points : wavesketch::EditMode::freehand)
                && preset.name.startsWith (c.pointsMode ? "Vague" : "Orgue"))
                processor.setWaveformState (preset.state);

        std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditorAndMakeActive());
        editor->setSize (c.width, c.height);

        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, c.scale);
        const auto file = folder.getChildFile (juce::String (c.name) + ".png");
        file.deleteFile();

        juce::FileOutputStream stream (file);
        if (stream.openedOk() && juce::PNGImageFormat().writeImageToStream (image, stream))
            ++written;

        editor.reset();
    }

    std::cout << written << " captures ecrites dans " << folder.getFullPathName() << "\n";
    return written == (int) std::size (cases) ? 0 : 1;
}
