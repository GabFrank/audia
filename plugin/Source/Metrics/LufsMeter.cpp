#include "LufsMeter.h"

#include <algorithm>
#include <cmath>

namespace auma::metrics {

namespace {

constexpr double kWindowSeconds = 0.4;
constexpr float kAbsoluteSilenceFloor = 1.0e-12f;  // ~-120 dBFS

}  // namespace

LufsMeter::LufsMeter() = default;

void LufsMeter::prepare(double sampleRate, int numChannels) {
    sampleRate_ = sampleRate;
    numChannels_ = std::max(1, numChannels);
    storageChannels_ = numChannels_;

    // Two windows of headroom is enough — the worker drains at ~30 Hz.
    const int fifoSamples = static_cast<int>(sampleRate_ * kWindowSeconds * 2.0);
    fifo_.setTotalSize(fifoSamples);
    fifo_.reset();
    fifoStorage_.assign(static_cast<size_t>(fifoSamples) * storageChannels_, 0.0f);

    filters_.assign(numChannels_, KWeighting{});
    for (auto& f : filters_) f.prepare(sampleRate);

    windowSamples_ = static_cast<int>(sampleRate_ * kWindowSeconds);
    meanSquareRing_.assign(windowSamples_, 0.0f);
    ringWrite_ = 0;
    ringFilled_ = 0;
    runningSum_ = 0.0;
    latestLufs_.store(kSilenceFloorLufs, std::memory_order_relaxed);
}

void LufsMeter::reset() {
    for (auto& f : filters_) f.reset();
    std::fill(meanSquareRing_.begin(), meanSquareRing_.end(), 0.0f);
    ringWrite_ = 0;
    ringFilled_ = 0;
    runningSum_ = 0.0;
    latestLufs_.store(kSilenceFloorLufs, std::memory_order_relaxed);
}

void LufsMeter::push(const float* const* channelData, int numChannels, int numSamples) noexcept {
    if (storageChannels_ == 0 || numSamples <= 0) return;
    const int chCount = std::min(numChannels, storageChannels_);

    int start1, size1, start2, size2;
    fifo_.prepareToWrite(numSamples, start1, size1, start2, size2);

    auto writeChunk = [&](int destStart, int chunkSize, int srcOffset) {
        for (int i = 0; i < chunkSize; ++i) {
            const int dstBase = (destStart + i) * storageChannels_;
            for (int ch = 0; ch < chCount; ++ch) {
                fifoStorage_[dstBase + ch] = channelData[ch][srcOffset + i];
            }
            // Zero unused channel slots.
            for (int ch = chCount; ch < storageChannels_; ++ch) {
                fifoStorage_[dstBase + ch] = 0.0f;
            }
        }
    };

    writeChunk(start1, size1, 0);
    writeChunk(start2, size2, size1);
    fifo_.finishedWrite(size1 + size2);
}

void LufsMeter::update() {
    if (storageChannels_ == 0) return;

    const int ready = fifo_.getNumReady();
    if (ready <= 0) return;

    int start1, size1, start2, size2;
    fifo_.prepareToRead(ready, start1, size1, start2, size2);

    auto consume = [&](int srcStart, int chunkSize) {
        for (int i = 0; i < chunkSize; ++i) {
            const int srcBase = (srcStart + i) * storageChannels_;
            float weightedSquareSum = 0.0f;
            // BS.1770 channel weights: L=R=C=1.0, surround=1.41. We only
            // support mono/stereo today, so the weight is 1.0 everywhere.
            for (int ch = 0; ch < numChannels_; ++ch) {
                const float w = filters_[ch].process(fifoStorage_[srcBase + ch]);
                weightedSquareSum += w * w;
            }

            // Slide the mean-square window forward by one sample.
            const float outgoing = meanSquareRing_[ringWrite_];
            meanSquareRing_[ringWrite_] = weightedSquareSum;
            runningSum_ += static_cast<double>(weightedSquareSum) -
                           static_cast<double>(outgoing);
            ringWrite_ = (ringWrite_ + 1) % windowSamples_;
            if (ringFilled_ < windowSamples_) ++ringFilled_;
        }
    };

    consume(start1, size1);
    consume(start2, size2);
    fifo_.finishedRead(size1 + size2);

    if (ringFilled_ < windowSamples_) {
        latestLufs_.store(kSilenceFloorLufs, std::memory_order_relaxed);
        return;
    }

    const double mean = std::max(runningSum_ / static_cast<double>(windowSamples_), 0.0);
    if (mean < kAbsoluteSilenceFloor) {
        latestLufs_.store(kSilenceFloorLufs, std::memory_order_relaxed);
        return;
    }
    const double lufs = -0.691 + 10.0 * std::log10(mean);
    latestLufs_.store(static_cast<float>(lufs), std::memory_order_relaxed);
}

float LufsMeter::currentLufs() const noexcept {
    return latestLufs_.load(std::memory_order_relaxed);
}

}  // namespace auma::metrics
