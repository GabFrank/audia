#include "PluginEditor.h"

#include "PluginProcessor.h"

namespace auma {

AumaEditor::AumaEditor(AumaProcessor& p) : juce::AudioProcessorEditor(&p) {
    setSize(360, 160);
}

void AumaEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(0x10, 0x12, 0x18));
    g.setColour(juce::Colour::fromRGB(0xe6, 0xe6, 0xe6));
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("AuMA", getLocalBounds().reduced(12), juce::Justification::topLeft, false);
    g.setFont(juce::Font(12.0f));
    g.setColour(juce::Colour::fromRGB(0x9a, 0xa2, 0xb1));
    g.drawText("Mastering assistant — skeleton build",
               getLocalBounds().reduced(12).withTop(36),
               juce::Justification::topLeft, false);
}

void AumaEditor::resized() {}

}  // namespace auma
