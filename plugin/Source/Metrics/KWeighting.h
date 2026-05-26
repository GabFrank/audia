#pragma once

#include <array>

namespace auma::metrics {

/**
 * ITU-R BS.1770 K-weighting: a high-shelf pre-filter cascaded with a
 * high-pass. Coefficients are derived per sample rate. The filter is
 * single-channel; the caller instantiates one per channel.
 */
class KWeighting {
public:
    KWeighting() = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    /** Process one sample in place. */
    [[nodiscard]] float process(float x) noexcept;

private:
    struct Biquad {
        std::array<double, 3> b{1.0, 0.0, 0.0};
        std::array<double, 2> a{0.0, 0.0};  // a0 is normalised to 1
        double z1 = 0.0;
        double z2 = 0.0;

        void reset() noexcept { z1 = z2 = 0.0; }

        double processSample(double x) noexcept {
            // Direct Form II transposed.
            const double y = b[0] * x + z1;
            z1 = b[1] * x - a[0] * y + z2;
            z2 = b[2] * x - a[1] * y;
            return y;
        }
    };

    Biquad shelf_;
    Biquad highpass_;
};

}  // namespace auma::metrics
