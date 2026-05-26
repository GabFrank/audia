#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Metrics/LufsMeter.h"
#include "Net/Protocol.h"
#include "Net/WebSocketClient.h"

namespace auma {

/**
 * Skeleton processor for the AuMA plugin.
 *
 * `processBlock` is realtime-locked: no allocation, no locks, no IO,
 * no network. Audio samples are pushed into a lock-free FIFO; the
 * worker thread drains it, runs the LUFS meter, and dispatches the
 * telemetry frame over WebSocket.
 */
class AumaProcessor : public juce::AudioProcessor {
public:
    AumaProcessor();
    ~AumaProcessor() override;

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
    bool isConnected() const noexcept { return wsClient_.isConnected(); }

private:
    void rebuildRegisterPayload();

    metrics::LufsMeter lufsMeter_;
    net::WebSocketClient wsClient_;
    juce::String instanceId_;

    double currentSampleRate_ = 0.0;
    int currentChannels_ = 0;
    int metricsCounter_ = 0;

    class WorkerTimer : public juce::Timer {
    public:
        explicit WorkerTimer(AumaProcessor& owner) : owner_(owner) {}
        void timerCallback() override { owner_.onWorkerTick(); }

    private:
        AumaProcessor& owner_;
    };

    void onWorkerTick();

    WorkerTimer workerTimer_{*this};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AumaProcessor)
};

}  // namespace auma
