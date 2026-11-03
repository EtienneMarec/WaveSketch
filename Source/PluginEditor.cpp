#include "PluginEditor.h"
#include "Parameters.h"
#include "dsp/WaveformOps.h"
#include "model/Presets.h"

using namespace wavesketch;

namespace
{
    const juce::Colour backgroundColour (0xff16181d);
    const juce::Colour textColour (0xffe6e8ee);

    constexpr int kModeGroup = 1001;
    constexpr int kDefaultWidth = 960, kDefaultHeight = 640;

    juce::String utf8 (const char* text) { return juce::String::fromUTF8 (text); }

    // Dossier proposé par les boîtes de dialogue : le dernier utilisé, sinon Documents\WaveSketch.
    juce::File& lastWaveformFolder()
    {
        static juce::File folder;
        return folder;
    }

    juce::File getWaveformFolder()
    {
        auto& folder = lastWaveformFolder();
        if (! folder.isDirectory())
            folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("WaveSketch");
        return folder;
    }

    void rememberWaveformFolder (const juce::File& file)
    {
        lastWaveformFolder() = file.getParentDirectory();
    }
}

WaveSketchEditor::WaveSketchEditor (WaveSketchProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);

    // --- Mode d'édition (boutons exclusifs) ---
    for (auto* b : { &freehandModeButton, &pointsModeButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (kModeGroup);
    }
    addButton (freehandModeButton, utf8 ("Main levée"), "Dessiner librement la courbe", [this] { setMode (EditMode::freehand); });
    addButton (pointsModeButton, "Points",
               utf8 ("Clic : ajouter un point. Glisser : déplacer. Clic droit ou double-clic : supprimer."),
               [this] { setMode (EditMode::points); });

    // --- Presets et fichiers ---
    presetBox.setTextWhenNothingSelected ("Presets");
    const auto& presets = getBuiltInPresets();
    for (int i = 0; i < static_cast<int> (presets.size()); ++i)
        presetBox.addItem (presets[static_cast<size_t> (i)].name, i + 1);
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        const auto& list = getBuiltInPresets();
        if (index >= 0 && index < static_cast<int> (list.size()))
            audioProcessor.applyWaveformChange (list[static_cast<size_t> (index)].state, "Preset " + list[static_cast<size_t> (index)].name);
        presetBox.setSelectedId (0, juce::dontSendNotification);
    };
    addAndMakeVisible (presetBox);

    addButton (openButton, utf8 ("Ouvrir…"), "Charger une waveform depuis un fichier .wavesketch", [this] { openWaveformFile(); });
    addButton (saveButton, utf8 ("Enregistrer…"), "Enregistrer la waveform dans un fichier .wavesketch", [this] { saveWaveformFile(); });
    addButton (undoButton, "Annuler", "Annuler (Ctrl+Z)", [this] { audioProcessor.getUndoManager().undo(); });
    addButton (redoButton, utf8 ("Rétablir"), utf8 ("Rétablir (Ctrl+Y)"), [this] { audioProcessor.getUndoManager().redo(); });

    // --- Formes de base et outils ---
    const auto addShape = [this] (juce::TextButton& button, const char* name, BasicShape shape)
    {
        addButton (button, utf8 (name), utf8 ("Remplace le dessin par cette forme"), [this, shape, name]
        {
            audioProcessor.applyWaveformChange (WaveformState::fromSamples (makeBasicShape (shape)), utf8 ("Forme ") + utf8 (name));
        });
    };
    addShape (sineButton,     "Sinus",         BasicShape::sine);
    addShape (triangleButton, "Triangle",      BasicShape::triangle);
    addShape (sawButton,      "Dents de scie", BasicShape::saw);
    addShape (squareButton,   "Carré",         BasicShape::square);

    addButton (smoothButton, "Lisser", utf8 ("Adoucit la courbe (cliquer plusieurs fois pour lisser davantage)"),
               [this] { applyToWaveform ("Lisser", [] (WaveformState& s) { ops::smooth (s.samples); }); });

    addButton (invertButton, "Inverser", utf8 ("Inversion de phase : retourne la courbe verticalement"), [this]
    {
        applyToWaveform ("Inverser", [] (WaveformState& s)
        {
            if (s.mode == EditMode::points)
            {
                auto points = s.points;
                for (auto& pt : points)
                    pt.y = -pt.y;
                s = WaveformState::fromPoints (std::move (points));
            }
            else
            {
                ops::invert (s.samples);
            }
        });
    });

    addButton (resetButton, utf8 ("Réinitialiser"), utf8 ("Revient au sinus de départ"), [this]
    {
        audioProcessor.applyWaveformChange (WaveformState::fromSamples (makeBasicShape (BasicShape::sine)), utf8 ("Réinitialiser"));
    });

    addAndMakeVisible (waveformEditor);

    // --- Réglages ---
    addKnob (attackKnob,  params::attack,    "Attack");
    addKnob (decayKnob,   params::decay,     "Decay");
    addKnob (sustainKnob, params::sustain,   "Sustain");
    addKnob (releaseKnob, params::release,   "Release");
    addKnob (gainKnob,    params::gain,      "Volume");
    addKnob (bendKnob,    params::bendRange, "Bend");

    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (48);
    addAndMakeVisible (keyboard);

    audioProcessor.addChangeListener (this);
    audioProcessor.getUndoManager().addChangeListener (this);
    updateButtonStates();

    // À lire avant setResizeLimits(), qui redimensionne déjà la fenêtre (et donc écrase la taille mémorisée).
    const auto saved = audioProcessor.getSavedEditorSize();

    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setResizeLimits (820, 540, 2000, 1400);

    if (saved.x >= 820 && saved.y >= 540)
        setSize (juce::jmin (saved.x, 2000), juce::jmin (saved.y, 1400));
    else
        setSize (kDefaultWidth, kDefaultHeight);
}

WaveSketchEditor::~WaveSketchEditor()
{
    audioProcessor.getUndoManager().removeChangeListener (this);
    audioProcessor.removeChangeListener (this);

    // Évite une note bloquée si la fenêtre se ferme pendant qu'une touche est enfoncée.
    audioProcessor.getKeyboardState().allNotesOff (1);
    setLookAndFeel (nullptr);
}

//==============================================================================
void WaveSketchEditor::addKnob (Knob& knob, const juce::String& paramId, const juce::String& name)
{
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, textColour);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getParameters(), paramId, knob.slider);
}

void WaveSketchEditor::addButton (juce::Button& button, const juce::String& name, const juce::String& tooltip,
                                  std::function<void()> onClick)
{
    button.setButtonText (name);
    button.setTooltip (tooltip);
    button.onClick = std::move (onClick);
    addAndMakeVisible (button);
}

void WaveSketchEditor::applyToWaveform (const juce::String& actionName,
                                        const std::function<void (WaveformState&)>& operation)
{
    auto state = audioProcessor.getWaveformState();
    operation (state);
    audioProcessor.applyWaveformChange (state, actionName);
}

void WaveSketchEditor::setMode (EditMode mode)
{
    const auto state = audioProcessor.getWaveformState();
    audioProcessor.applyWaveformChange (state.withMode (mode),
                                        mode == EditMode::points ? "Mode points" : utf8 ("Mode main levée"));
}

void WaveSketchEditor::updateButtonStates()
{
    const bool pointsMode = audioProcessor.getWaveformState().mode == EditMode::points;
    freehandModeButton.setToggleState (! pointsMode, juce::dontSendNotification);
    pointsModeButton.setToggleState (pointsMode, juce::dontSendNotification);
    smoothButton.setEnabled (! pointsMode); // en mode points, la courbe est déjà lisse

    auto& undo = audioProcessor.getUndoManager();
    undoButton.setEnabled (undo.canUndo());
    redoButton.setEnabled (undo.canRedo());
    undoButton.setTooltip (undo.canUndo() ? "Annuler : " + undo.getUndoDescription() + " (Ctrl+Z)" : "Annuler (Ctrl+Z)");
    redoButton.setTooltip (undo.canRedo() ? utf8 ("Rétablir : ") + undo.getRedoDescription() + " (Ctrl+Y)" : utf8 ("Rétablir (Ctrl+Y)"));
}

bool WaveSketchEditor::keyPressed (const juce::KeyPress& key)
{
    auto& undo = audioProcessor.getUndoManager();
    const auto mods = key.getModifiers();

    if (mods.isCommandDown() && key.getKeyCode() == 'Z' && ! mods.isShiftDown())
    {
        undo.undo();
        return true;
    }

    if (mods.isCommandDown() && (key.getKeyCode() == 'Y' || (key.getKeyCode() == 'Z' && mods.isShiftDown())))
    {
        undo.redo();
        return true;
    }

    return false;
}

//==============================================================================
void WaveSketchEditor::openWaveformFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Ouvrir une waveform", getWaveformFolder(),
                                                       juce::String ("*") + kWaveformFileExtension);

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file == juce::File())
            return;

        rememberWaveformFolder (file);
        juce::String error;

        if (const auto state = loadWaveformFile (file, error))
            audioProcessor.applyWaveformChange (*state, "Ouvrir " + file.getFileNameWithoutExtension());
        else
            showError (error);
    });
}

void WaveSketchEditor::saveWaveformFile()
{
    const auto folder = getWaveformFolder();
    folder.createDirectory();

    fileChooser = std::make_unique<juce::FileChooser> ("Enregistrer la waveform", folder.getChildFile (juce::String ("waveform") + kWaveformFileExtension),
                                                       juce::String ("*") + kWaveformFileExtension);

    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file == juce::File())
            return;

        if (! file.hasFileExtension (kWaveformFileExtension))
            file = file.withFileExtension (kWaveformFileExtension);

        rememberWaveformFolder (file);

        if (const auto result = wavesketch::saveWaveformFile (audioProcessor.getWaveformState(), file); result.failed())
            showError (result.getErrorMessage());
    });
}

void WaveSketchEditor::showError (const juce::String& message)
{
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::WarningIcon)
                                      .withTitle ("WaveSketch")
                                      .withMessage (message)
                                      .withButton ("OK", 0)
                                      .withAssociatedComponent (this),
                                  nullptr);
}

//==============================================================================
void WaveSketchEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);

    g.setColour (textColour);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("WaveSketch", getLocalBounds().reduced (16, 0).removeFromTop (48), juce::Justification::centredLeft);
}

void WaveSketchEditor::resized()
{
    audioProcessor.setSavedEditorSize ({ getWidth(), getHeight() });

    auto area = getLocalBounds().reduced (12);

    // Ligne 1 : titre, mode, presets, fichiers, annuler/rétablir
    auto row1 = area.removeFromTop (36);
    row1.removeFromLeft (130);
    freehandModeButton.setBounds (row1.removeFromLeft (100).reduced (2, 3));
    pointsModeButton.setBounds (row1.removeFromLeft (80).reduced (2, 3));

    for (auto* b : { &redoButton, &undoButton })
        b->setBounds (row1.removeFromRight (80).reduced (2, 3));
    row1.removeFromRight (8);
    for (auto* b : { &saveButton, &openButton })
        b->setBounds (row1.removeFromRight (100).reduced (2, 3));
    presetBox.setBounds (row1.removeFromRight (juce::jmin (180, row1.getWidth() - 8)).reduced (4, 4));

    // Ligne 2 : formes de base et outils
    area.removeFromTop (4);
    auto row2 = area.removeFromTop (32);
    const int buttonWidth = juce::jmin (110, row2.getWidth() / 7);
    for (auto* b : { &sineButton, &triangleButton, &sawButton, &squareButton })
        b->setBounds (row2.removeFromLeft (buttonWidth).reduced (2, 2));
    for (auto* b : { &resetButton, &invertButton, &smoothButton })
        b->setBounds (row2.removeFromRight (buttonWidth).reduced (2, 2));

    keyboard.setBounds (area.removeFromBottom (70));
    area.removeFromBottom (8);

    auto knobs = area.removeFromBottom (120);
    const int knobWidth = knobs.getWidth() / 6;
    for (auto* k : { &attackKnob, &decayKnob, &sustainKnob, &releaseKnob, &gainKnob, &bendKnob })
    {
        auto cell = knobs.removeFromLeft (knobWidth);
        k->label.setBounds (cell.removeFromTop (20));
        k->slider.setBounds (cell);
    }

    area.removeFromBottom (8);
    waveformEditor.setBounds (area.withTrimmedTop (8));
}
