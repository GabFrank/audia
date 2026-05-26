#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Metrics/LufsMeter.h"

namespace auma {

/**
 * Skeleton processor for the AuMA plugin.
 *
 * `processBlock` is realtime-locked: no allocation, no locks, no IO,
 * no network. The current build is a pass-through; subsequent commits
 * add a lock-free FIFO into a worker thread that runs the LUFS meter
 * and a WebSocket client.
 */
class AumaProcessor : public juce::AudioProcessor {
public:
    AumaProcessor();
    ~AumaProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AuMA"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    metrics::LufsMeter& lufsMeter() noexcept { return lufsMeter_; }

private:
    metrics::LufsMeter lufsMeter_;

    class WorkerTimer : public juce::Timer {
    public:
        explicit WorkerTimer(AumaProcessor& owner) : owner_(owner) {}
        void timerCallback() override { owner_.lufsMeter_.update(); }

    private:
        AumaProcessor& owner_;
    };

    WorkerTimer workerTimer_{*this};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AumaProcessor)
};

}  // namespace auma
