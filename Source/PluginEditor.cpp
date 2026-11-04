#include "PluginEditor.h"
#include "Parameters.h"
#include "dsp/WaveformOps.h"
#include "model/Presets.h"

using namespace wavesketch;

namespace
{
    constexpr int kModeGroup = 1001;

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

    // Place une rangée de boutons collés les uns aux autres (bords communs sans arrondi).
    void layoutButtonRow (juce::Rectangle<int>& row, std::initializer_list<juce::Button*> buttons, int width, bool fromLeft)
    {
        const int count = static_cast<int> (buttons.size());
        int index = 0;

        for (auto* b : buttons)
        {
            int edges = 0;
            if (index > 0)         edges |= juce::Button::ConnectedOnLeft;
            if (index < count - 1) edges |= juce::Button::ConnectedOnRight;
            b->setConnectedEdges (edges);
            ++index;
        }

        if (fromLeft)
        {
            for (auto* b : buttons)
                b->setBounds (row.removeFromLeft (width));
        }
        else
        {
            for (auto it = std::rbegin (buttons); it != std::rend (buttons); ++it)
                (*it)->setBounds (row.removeFromRight (width));
        }
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
    presetBox.setTooltip (utf8 ("Waveforms intégrées"));
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
    bendKnob.slider.setTooltip (utf8 ("Amplitude de la molette de pitch bend, en demi-tons"));

    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (48);
    keyboard.setScrollButtonWidth (20);
    addAndMakeVisible (keyboard);

    audioProcessor.addChangeListener (this);
    audioProcessor.getUndoManager().addChangeListener (this);
    updateButtonStates();

    // À lire avant setResizeLimits(), qui redimensionne déjà la fenêtre (et donc écrase la taille mémorisée).
    const auto saved = audioProcessor.getSavedEditorSize();

    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);

    if (saved.x >= kMinWidth && saved.y >= kMinHeight)
        setSize (juce::jmin (saved.x, kMaxWidth), juce::jmin (saved.y, kMaxHeight));
    else
        setSize (kDefaultWidth, kDefaultHeight);

    // Certains éléments (comme la zone de valeur des potentiomètres) sont créés avant d'être
    // ajoutés à la fenêtre : on leur réapplique le thème une fois tout en place.
    sendLookAndFeelChange();
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
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    knob.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    knob.slider.setDoubleClickReturnValue (true, audioProcessor.getParameters().getParameter (paramId)->convertFrom0to1 (
                                                     audioProcessor.getParameters().getParameter (paramId)->getDefaultValue()));
    addAndMakeVisible (knob.slider);

    knob.label.setText (name, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    knob.label.setColour (juce::Label::textColourId, theme::text);
    knob.label.setInterceptsMouseClicks (false, false);
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
    g.fillAll (theme::background);

    // Titre et version
    auto title = headerArea.withWidth (220);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("WaveSketch", title.removeFromTop (title.getHeight() * 3 / 5), juce::Justification::bottomLeft);
    g.setColour (theme::textDim);
    g.setFont (juce::FontOptions (11.5f));
    g.drawText (juce::String ("v") + JucePlugin_VersionString + juce::String::fromUTF8 (" · synthé à waveform dessinée"),
                title, juce::Justification::topLeft);

    // Panneaux des réglages, avec leur titre
    for (auto [panel, name] : { std::pair { envelopePanel, "ENVELOPPE" }, std::pair { outputPanel, "SORTIE" } })
    {
        if (panel.isEmpty())
            continue;

        g.setColour (theme::panel);
        g.fillRoundedRectangle (panel.toFloat(), 8.0f);
        g.setColour (theme::outline);
        g.drawRoundedRectangle (panel.toFloat().reduced (0.5f), 8.0f, 1.0f);

        g.setColour (theme::textDim);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (name, panel.reduced (12, 6).removeFromTop (14), juce::Justification::topLeft);
    }
}

void WaveSketchEditor::layoutKnobGroup (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs)
{
    area = area.reduced (8, 0).withTrimmedTop (18).withTrimmedBottom (6);
    const int width = area.getWidth() / static_cast<int> (knobs.size());

    for (auto* k : knobs)
    {
        auto cell = area.removeFromLeft (width);
        k->label.setBounds (cell.removeFromTop (18));
        k->slider.setBounds (cell.reduced (4, 0));
    }
}

void WaveSketchEditor::resized()
{
    audioProcessor.setSavedEditorSize ({ getWidth(), getHeight() });

    auto area = getLocalBounds().reduced (14, 10);

    // En-tête : titre à gauche ; presets, fichiers et annuler/rétablir à droite
    headerArea = area.removeFromTop (46);
    auto header = headerArea.reduced (0, 7);
    layoutButtonRow (header, { &undoButton, &redoButton }, 84, false);
    header.removeFromRight (12);
    layoutButtonRow (header, { &openButton, &saveButton }, 104, false);
    header.removeFromRight (8);
    presetBox.setBounds (header.removeFromRight (juce::jlimit (120, 200, header.getWidth() - 230)));

    // Barre d'outils : mode, formes de base, transformations
    area.removeFromTop (6);
    auto toolbar = area.removeFromTop (32);
    const int toolWidth = juce::jlimit (72, 112, (toolbar.getWidth() - 40) / 9);
    layoutButtonRow (toolbar, { &freehandModeButton, &pointsModeButton }, toolWidth, true);
    toolbar.removeFromLeft (20);
    layoutButtonRow (toolbar, { &sineButton, &triangleButton, &sawButton, &squareButton }, toolWidth, true);
    layoutButtonRow (toolbar, { &smoothButton, &invertButton, &resetButton }, toolWidth, false);

    // Bas : clavier, puis réglages
    keyboard.setBounds (area.removeFromBottom (juce::jlimit (52, 80, getHeight() / 10)));
    // 50 touches blanches entre C0 et C7 : le clavier occupe toute la largeur.
    keyboard.setKeyWidth (juce::jmax (12.0f, static_cast<float> (keyboard.getWidth()) / 50.0f));
    area.removeFromBottom (10);

    auto controls = area.removeFromBottom (juce::jlimit (130, 180, getHeight() / 4));
    envelopePanel = controls.removeFromLeft (controls.getWidth() * 4 / 6 - 5);
    controls.removeFromLeft (10);
    outputPanel = controls;
    layoutKnobGroup (envelopePanel, { &attackKnob, &decayKnob, &sustainKnob, &releaseKnob });
    layoutKnobGroup (outputPanel, { &gainKnob, &bendKnob });

    // Centre : la zone de dessin prend toute la place restante
    area.removeFromBottom (10);
    area.removeFromTop (8);
    waveformEditor.setBounds (area);
}
