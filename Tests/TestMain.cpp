#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>

int renderSnapshots (const juce::File& folder);

namespace
{
    // Par défaut, JUCE envoie les messages de test au débogueur Windows : on les affiche dans la console.
    class ConsoleTestRunner final : public juce::UnitTestRunner
    {
        void logMessage (const juce::String& message) override
        {
            std::cout << message.toStdString() << std::endl;
        }
    };
}

// Lance tous les tests de la catégorie "WaveSketch" et renvoie 1 en cas d'échec (pour CTest).
// Avec "--snapshots <dossier>", produit à la place des captures de l'interface.
int main (int argc, char* argv[])
{
    // Le processeur et l'éditeur ont besoin du "message thread" de JUCE : ce thread principal en tient lieu.
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    if (argc >= 3 && juce::String (argv[1]) == "--snapshots")
        return renderSnapshots (juce::File::getCurrentWorkingDirectory().getChildFile (juce::String::fromUTF8 (argv[2])));

    ConsoleTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("WaveSketch");

    int passes = 0, failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        passes += runner.getResult (i)->passes;
        failures += runner.getResult (i)->failures;
    }

    std::cout << "\n" << runner.getNumResults() << " tests, "
              << passes << " verifications reussies, " << failures << " echec(s).\n";
    return failures == 0 && passes > 0 ? 0 : 1;
}
