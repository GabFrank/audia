#pragma once

#include <atomic>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>

#include "KWeighting.h"

namespace auma::metrics {

/**
 * Momentary loudness meter per ITU-R BS.1770 / EBU R128.
 *
 * Holds a lock-free FIFO that the audio thread writes interleaved
 * samples into. The worker thread calls `update()` to drain the FIFO,
 * apply K-weighting, and slide the 400 ms mean-square window forward.
 * Read the latest value from a non-realtime thread with `currentLufs()`.
 *
 * The class itself is not realtime — `update()` allocates only at
 * `prepare()` time; the audio-thread call site is `push()` which uses
 * `juce::AbstractFifo` and never allocates.
 */
class LufsMeter {
public:
    static constexpr float kSilenceFloorLufs = -100.0f;

    LufsMeter();

    void prepare(double sampleRate, int numChannels);
    void reset();

    /** Realtime-safe. Called from the audio thread. */
    void push(const float* const* channelData, int numChannels, int numSamples) noexcept;

    /** Worker-thread side. Drain the FIFO and update the sliding window. */
    void update();

    /** Atomic read; safe from any thread. */
    [[nodiscard]] float currentLufs() const noexcept;

private:
    double sampleRate_ = 48000.0;
    int numChannels_ = 0;
    int windowSamples_ = 0;

    juce::AbstractFifo fifo_{1};
    std::vector<float> fifoStorage_;   // interleaved
    int storageChannels_ = 0;

    std::vector<KWeighting> filters_;
    std::vector<float> meanSquareRing_;
    int ringWrite_ = 0;
    double runningSum_ = 0.0;
    int ringFilled_ = 0;

    std::atomic<float> latestLufs_{kSilenceFloorLufs};
};

}  // namespace auma::metrics
