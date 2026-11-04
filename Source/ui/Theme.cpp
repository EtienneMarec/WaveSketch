#include "Theme.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace wavesketch::theme
{

LookAndFeel::LookAndFeel()
    : juce::LookAndFeel_V4 (juce::LookAndFeel_V4::ColourScheme {
          background, panel, panelRaised, outline, text, accent, text, panelRaised, text })
{
    setColour (juce::ResizableWindow::backgroundColourId, background);

    setColour (juce::TextButton::buttonColourId, panelRaised);
    setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.22f));
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, accent);

    setColour (juce::Slider::rotarySliderFillColourId, accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, outline);
    setColour (juce::Slider::thumbColourId, text);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.35f));

    setColour (juce::Label::textColourId, text);
    setColour (juce::TextEditor::backgroundColourId, panelRaised);
    setColour (juce::TextEditor::outlineColourId, outline);
    setColour (juce::TextEditor::focusedOutlineColourId, accent);

    setColour (juce::ComboBox::backgroundColourId, panelRaised);
    setColour (juce::ComboBox::outlineColourId, outline);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::arrowColourId, textDim);
    setColour (juce::PopupMenu::backgroundColourId, panelRaised);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, text);

    setColour (juce::TooltipWindow::backgroundColourId, panelRaised);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::TooltipWindow::outlineColourId, outline);

    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffd9dce3));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1f2229));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff8a90a0));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, accent.withAlpha (0.25f));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, accent.withAlpha (0.7f));
    setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, panelRaised);
    setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, textDim);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                    float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float trackWidth = juce::jmax (3.0f, radius * 0.14f);
    const float arcRadius = radius - trackWidth * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const bool hover = slider.isMouseOverOrDragging() && slider.isEnabled();

    // Piste de fond puis arc de valeur
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (outline);
    g.strokePath (track, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);
    g.setColour (hover ? accent.brighter (0.2f) : accent);
    g.strokePath (value, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Bouton central et repère
    const float knobRadius = arcRadius - trackWidth * 1.4f;
    g.setColour (hover ? panelRaised.brighter (0.08f) : panelRaised);
    g.fillEllipse (juce::Rectangle<float> (knobRadius * 2.0f, knobRadius * 2.0f).withCentre (centre));

    const auto tip = centre.getPointOnCircumference (knobRadius * 0.8f, angle);
    const auto inner = centre.getPointOnCircumference (knobRadius * 0.25f, angle);
    g.setColour (text);
    g.drawLine ({ inner, tip }, juce::jmax (2.0f, trackWidth * 0.7f));
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                        bool isMouseOverButton, bool isButtonDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const float corner = juce::jmin (6.0f, bounds.getHeight() * 0.3f);
    const bool on = button.getToggleState();

    auto fill = on ? accent.withAlpha (0.18f) : panelRaised;
    if (isButtonDown)          fill = fill.brighter (0.12f);
    else if (isMouseOverButton) fill = fill.brighter (0.06f);
    if (! button.isEnabled())  fill = fill.withMultipliedAlpha (0.5f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (on ? accent.withAlpha (0.7f) : outline);
    g.drawRoundedRectangle (bounds, corner, 1.0f);
}

juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::FontOptions (juce::jmin (15.0f, static_cast<float> (buttonHeight) * 0.5f));
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (width, height).toFloat().reduced (0.5f);
    const float corner = juce::jmin (6.0f, bounds.getHeight() * 0.3f);

    g.setColour (box.isMouseOver (true) ? panelRaised.brighter (0.06f) : panelRaised);
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (box.hasKeyboardFocus (true) ? accent.withAlpha (0.7f) : outline);
    g.drawRoundedRectangle (bounds, corner, 1.0f);

    // Petite flèche vers le bas à droite
    const float arrowSize = juce::jmin (10.0f, bounds.getHeight() * 0.3f);
    const auto arrowCentre = juce::Point<float> (bounds.getRight() - arrowSize - 6.0f, bounds.getCentreY());
    juce::Path arrow;
    arrow.addTriangle (arrowCentre.translated (-arrowSize * 0.5f, -arrowSize * 0.25f),
                       arrowCentre.translated (arrowSize * 0.5f, -arrowSize * 0.25f),
                       arrowCentre.translated (0.0f, arrowSize * 0.3f));
    g.setColour (textDim);
    g.fillPath (arrow);
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return juce::FontOptions (juce::jmin (15.0f, static_cast<float> (box.getHeight()) * 0.5f));
}

juce::Label* LookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (juce::FontOptions (13.0f));
    label->setColour (juce::Label::textColourId, textDim);
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return label;
}

} // namespace wavesketch::theme
