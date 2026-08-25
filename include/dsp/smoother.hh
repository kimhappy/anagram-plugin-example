#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace dsp {
class LinearSmoother {
private:
    float inv_ramp_samples_;
    float current_;
    float target_;
    float step_ { 0.0f };

public:
    LinearSmoother(
        double const sample_rate,
        double const ramp_seconds,
        float const  value
    ) noexcept:
        inv_ramp_samples_ { static_cast< float >(1.0 / (ramp_seconds * sample_rate)) },
        current_ { value },
        target_ { value } {}

    [[nodiscard]]
    auto advance(uint32_t const samples) noexcept -> float {
        auto const remaining = target_ - current_;
        auto const travel    = std::min(step_ * static_cast< float >(samples), std::abs(remaining));
        current_ += std::copysign(travel, remaining);
        return current_;
    }

    [[nodiscard]]
    auto get_current() const noexcept -> float {
        return current_;
    }

    auto snap() noexcept -> void {
        current_ = target_;
    }

    auto set_target(float const target) noexcept -> void {
        if (target == target_) {
            return;
        }

        target_ = target;
        step_   = std::abs(target_ - current_) * inv_ramp_samples_;
    }
};
}  // namespace dsp
