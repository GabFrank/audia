#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace auma {

AumaProcessor::AumaProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

void AumaProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/) {
    lufsMeter_.prepare(sampleRate, getTotalNumInputChannels());
    workerTimer_.startTimerHz(30);
}

void AumaProcessor::releaseResources() {
    workerTimer_.stopTimer();
    lufsMeter_.reset();
}

bool AumaProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == mainOut;
}

void AumaProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    // Realtime-safe path: push a read-only view of the input into the
    // LUFS meter's lock-free FIFO. The worker thread drains it. Audio
    // is untouched — the plugin is a transparent tap.
    lufsMeter_.push(buffer.getArrayOfReadPointers(),
                    buffer.getNumChannels(),
                    buffer.getNumSamples());
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
