#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Palette et apparence de l'interface. Tout est dessiné en vectoriel (aucune image bitmap),
// donc l'affichage reste net quelle que soit la mise à l'échelle de Windows (125 %, 150 %...).
namespace wavesketch::theme
{
    inline const juce::Colour background    { 0xff121418 };
    inline const juce::Colour panel         { 0xff1b1e25 };
    inline const juce::Colour panelRaised   { 0xff232731 };
    inline const juce::Colour outline       { 0xff2c313c };
    inline const juce::Colour text          { 0xffe6e8ee };
    inline const juce::Colour textDim       { 0xff8a90a0 };
    inline const juce::Colour accent        { 0xff4fc3f7 }; // courbe, potentiomètres
    inline const juce::Colour points        { 0xffffb74d }; // points de contrôle

    class LookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();

        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                                   bool isMouseOverButton, bool isButtonDown) override;

        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

        void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                           int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;

        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Label* createSliderTextBox (juce::Slider&) override;
    };
}
