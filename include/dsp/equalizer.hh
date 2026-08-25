#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include <range/v3/all.hpp>

#include "dsp/band.hh"
#include "dsp/biquad.hh"
#include "util/const.hh"

namespace dsp {
class Equalizer {
private:
    static constexpr auto FREQ_RATIO_MAX = 0.45;

    struct BandGeometry {
        float cos_w0;
        float alpha;
    };

    std::vector< BandSpec >     specs_;
    std::vector< BandGeometry > geometries_;
    std::vector< float >        gain_dbs_;
    std::vector< Biquad >       filters_;

    [[nodiscard]]
    static auto _make_geometry(
        double const    sample_rate,
        BandSpec const& spec
    ) noexcept -> BandGeometry {
        auto const freq = std::min(static_cast< double >(spec.freq), sample_rate * FREQ_RATIO_MAX);
        auto const w0   = 2.0 * util::PI * freq / sample_rate;

        return { static_cast< float >(std::cos(w0)),
                 static_cast< float >(std::sin(w0)) / (2.0f * spec.q) };
    }

    [[nodiscard]]
    static auto _make_coeffs(
        BandShape const     shape,
        BandGeometry const& geometry,
        float const         gain_db
    ) noexcept -> BiquadCoeffs {
        auto const amplitude  = std::pow(10.0f, gain_db * 0.025f);
        auto const cos_w0     = geometry.cos_w0;
        auto const slope      = 2.0f * std::sqrt(amplitude) * geometry.alpha;
        auto const sum        = amplitude + 1.0f;
        auto const difference = amplitude - 1.0f;

        if (shape == BandShape::LOW_SHELF) {
            auto const scale = 1.0f / (sum + difference * cos_w0 + slope);

            return { amplitude * (sum - difference * cos_w0 + slope) * scale,
                     2.0f * amplitude * (difference - sum * cos_w0) * scale,
                     amplitude * (sum - difference * cos_w0 - slope) * scale,
                     -2.0f * (difference + sum * cos_w0) * scale,
                     (sum + difference * cos_w0 - slope) * scale };
        }

        if (shape == BandShape::HIGH_SHELF) {
            auto const scale = 1.0f / (sum - difference * cos_w0 + slope);

            return { amplitude * (sum + difference * cos_w0 + slope) * scale,
                     -2.0f * amplitude * (difference + sum * cos_w0) * scale,
                     amplitude * (sum + difference * cos_w0 - slope) * scale,
                     2.0f * (difference - sum * cos_w0) * scale,
                     (sum - difference * cos_w0 - slope) * scale };
        }

        auto const scale = 1.0f / (1.0f + geometry.alpha / amplitude);

        return { (1.0f + geometry.alpha * amplitude) * scale,
                 -2.0f * cos_w0 * scale,
                 (1.0f - geometry.alpha * amplitude) * scale,
                 -2.0f * cos_w0 * scale,
                 (1.0f - geometry.alpha / amplitude) * scale };
    }

    auto _refresh_coeffs(size_t const band) noexcept -> void {
        filters_[ band ].set_coeffs(
            _make_coeffs(specs_[ band ].shape, geometries_[ band ], gain_dbs_[ band ])
        );
    }

public:
    Equalizer(
        double const            sample_rate,
        std::vector< BandSpec > specs
    ) noexcept:
        specs_(std::move(specs)),
        geometries_(
            specs_ | ranges::views::transform([ & ](auto const& spec) noexcept -> BandGeometry {
                return _make_geometry(sample_rate, spec);
            }) |
            ranges::to< std::vector< BandGeometry > >()
        ),
        gain_dbs_(
            specs_.size(),
            0.0f
        ),
        filters_(
            ranges::views::zip_with(
                [](auto const& spec,
                   auto const& geometry) noexcept -> Biquad {
                    return Biquad(_make_coeffs(spec.shape, geometry, 0.0f));
                },
                specs_,
                geometries_
            ) |
            ranges::to< std::vector< Biquad > >()
        ) {}

    [[nodiscard]]
    auto process(float const input) noexcept -> float {
        auto output = input;

        for (auto& filter: filters_) {
            output = filter.process(output);
        }

        return output;
    }

    auto reset() noexcept -> void {
        for (auto& filter: filters_) {
            filter.reset();
        }
    }

    [[nodiscard]]
    auto get_num_bands() const noexcept -> size_t {
        return specs_.size();
    }

    [[nodiscard]]
    auto get_spec(size_t const band) const noexcept -> BandSpec {
        return specs_[ band ];
    }

    [[nodiscard]]
    auto get_gain(size_t const band) const noexcept -> float {
        return gain_dbs_[ band ];
    }

    auto set_gain(
        size_t const band,
        float const  gain_db
    ) noexcept -> void {
        if (gain_db == gain_dbs_[ band ]) {
            return;
        }

        gain_dbs_[ band ] = gain_db;
        _refresh_coeffs(band);
    }
};
}  // namespace dsp
