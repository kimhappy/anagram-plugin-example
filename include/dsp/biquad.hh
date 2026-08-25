#pragma once

namespace dsp {
struct BiquadCoeffs {
    float b0;
    float b1;
    float b2;
    float a1;
    float a2;
};

class Biquad {
private:
    BiquadCoeffs coeffs_;
    float        state1_ { 0.0f };
    float        state2_ { 0.0f };

public:
    constexpr Biquad(BiquadCoeffs const& coeffs) noexcept:
        coeffs_ { coeffs } {}

    [[nodiscard]]
    constexpr auto process(float const input) noexcept -> float {
        auto const output = coeffs_.b0 * input + state1_;
        state1_           = coeffs_.b1 * input - coeffs_.a1 * output + state2_;
        state2_           = coeffs_.b2 * input - coeffs_.a2 * output;
        return output;
    }

    constexpr auto reset() noexcept -> void {
        state1_ = 0.0f;
        state2_ = 0.0f;
    }

    constexpr auto set_coeffs(BiquadCoeffs const& coeffs) noexcept -> void {
        coeffs_ = coeffs;
    }
};
}  // namespace dsp
