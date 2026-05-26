#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace auma {

AumaProcessor::AumaProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

void AumaProcessor::prepareToPlay(double, int) {}

void AumaProcessor::releaseResources() {}

bool AumaProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == mainOut;
}

void AumaProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    // Transparent pass-through. No allocation, no locks, no IO.
    // Commit 8 pushes samples into a lock-free FIFO here.
    juce::ignoreUnused(buffer);
}

juce::AudioProcessorEditor* AumaProcessor::createEditor() {
    return new AumaEditor(*this);
}

void AumaProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::ignoreUnused(destData);
}

void AumaProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::ignoreUnused(data, sizeInBytes);
}

}  // namespace auma

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new auma::AumaProcessor();
}
