#include "PluginEditor.h"

#include "PluginProcessor.h"

namespace auma {

AumaEditor::AumaEditor(AumaProcessor& p)
    : juce::AudioProcessorEditor(&p), processor_(p) {
    setSize(360, 160);

    lufsLabel_.setJustificationType(juce::Justification::centred);
    lufsLabel_.setFont(juce::Font(28.0f, juce::Font::bold));
    lufsLabel_.setColour(juce::Label::textColourId, juce::Colour::fromRGB(0xe6, 0xe6, 0xe6));
    lufsLabel_.setText("--.- LUFS", juce::dontSendNotification);
    addAndMakeVisible(lufsLabel_);

    startTimerHz(15);
}

void AumaEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(0x10, 0x12, 0x18));
    g.setColour(juce::Colour::fromRGB(0xe6, 0xe6, 0xe6));
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("AuMA", getLocalBounds().reduced(12), juce::Justification::topLeft, false);

    g.setFont(juce::Font(11.0f));
    g.setColour(juce::Colour::fromRGB(0x7a, 0x82, 0x95));
    g.drawText("Momentary loudness",
               juce::Rectangle<int>(0, 36, getWidth(), 16),
               juce::Justification::centred, false);
}

void AumaEditor::resized() {
    lufsLabel_.setBounds(0, 60, getWidth(), 60);
}

void AumaEditor::timerCallback() {
    const float lufs = processor_.lufsMeter().currentLufs();
    if (lufs <= metrics::LufsMeter::kSilenceFloorLufs + 0.1f) {
        lufsLabel_.setText("--.- LUFS", juce::dontSendNotification);
    } else {
        lufsLabel_.setText(juce::String(lufs, 1) + " LUFS", juce::dontSendNotification);
    }
}

}  // namespace auma
