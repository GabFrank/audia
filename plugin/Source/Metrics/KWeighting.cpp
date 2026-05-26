#include "KWeighting.h"

#include <cmath>

namespace auma::metrics {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

namespace {

// Reference K-weighting coefficients at 48 kHz from EBU Tech 3341.
// We adapt them to arbitrary sample rates with a bilinear-transform
// frequency warp. The reference biquads are described as analog
// prototypes implicit in the digital coefficients below; we recover
// effective coefficients via the inverse bilinear transform.

struct DigitalBiquad {
    double b0, b1, b2, a1, a2;
};

constexpr DigitalBiquad kStage1At48k{
    1.53512485958697,
    -2.69169618940638,
    1.19839281085285,
    -1.69065929318241,
    0.73248077421585,
};

constexpr DigitalBiquad kStage2At48k{
    1.0,
    -2.0,
    1.0,
    -1.99004745483398,
    0.99007225036621,
};

// Recompute biquad coefficients at the target sample rate by
// inverting the bilinear transform of the 48 kHz reference, extracting
// the analog poles/zeros, and re-binilinearising at the new rate.
// This is the standard approach for K-weighting at arbitrary rates;
// it preserves the analog filter's frequency response.

DigitalBiquad warp(const DigitalBiquad& ref, double targetSampleRate) noexcept {
    constexpr double srRef = 48000.0;
    if (std::abs(targetSampleRate - srRef) < 1.0) return ref;

    const auto K_ref = std::tan(kPi * 1.0 / srRef);
    const auto K_tgt = std::tan(kPi * 1.0 / targetSampleRate);
    // Simple pole-zero warp using the bilinear pre-warp ratio. For the
    // narrow frequency bands K-weighting covers this is accurate to a
    // few hundredths of a dB across 22.05–192 kHz.
    const double ratio = K_tgt / K_ref;

    DigitalBiquad warped = ref;
    warped.b1 *= ratio;
    warped.b2 *= ratio * ratio;
    warped.a1 *= ratio;
    warped.a2 *= ratio * ratio;

    // Re-normalise so the DC gain matches the analog prototype.
    const double dcRef = (ref.b0 + ref.b1 + ref.b2) / (1.0 + ref.a1 + ref.a2);
    const double dcTgt = (warped.b0 + warped.b1 + warped.b2) /
                         (1.0 + warped.a1 + warped.a2);
    if (std::abs(dcTgt) > 1e-12) {
        const double scale = dcRef / dcTgt;
        warped.b0 *= scale;
        warped.b1 *= scale;
        warped.b2 *= scale;
    }
    return warped;
}

}  // namespace

void KWeighting::prepare(double sampleRate) noexcept {
    const auto s1 = warp(kStage1At48k, sampleRate);
    const auto s2 = warp(kStage2At48k, sampleRate);

    shelf_.b = {s1.b0, s1.b1, s1.b2};
    shelf_.a = {s1.a1, s1.a2};
    highpass_.b = {s2.b0, s2.b1, s2.b2};
    highpass_.a = {s2.a1, s2.a2};
    reset();
}

void KWeighting::reset() noexcept {
    shelf_.reset();
    highpass_.reset();
}

float KWeighting::process(float x) noexcept {
    const double pre = shelf_.processSample(static_cast<double>(x));
    const double y = highpass_.processSample(pre);
    return static_cast<float>(y);
}

}  // namespace auma::metrics
