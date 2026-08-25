#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include <range/v3/all.hpp>

#include "dsp/band.hh"
#include "dsp/biquad.hh"
#include "dsp/equalizer.hh"
#include "dsp/smoother.hh"
#include "util/const.hh"

namespace {
auto failures = 0;

auto report(
    bool const        passed,
    char const* const expression,
    int const         line
) noexcept -> void {
    if (!passed) {
        ++failures;
        std::printf("FAIL dsp_test.cc:%d  %s\n", line, expression);
    }
}

auto report_near(
    double const      actual,
    double const      expected,
    double const      tolerance,
    char const* const expression,
    int const         line
) noexcept -> void {
    if (!(std::abs(actual - expected) <= tolerance)) {
        ++failures;
        std::printf(
            "FAIL dsp_test.cc:%d  %s\n     expected %g +- %g, got %g\n",
            line,
            expression,
            expected,
            tolerance,
            actual
        );
    }
}

#define CHECK(expression) report((expression), #expression, __LINE__)
#define CHECK_NEAR(actual, expected, tolerance)                       \
    report_near((actual), (expected), (tolerance), #actual, __LINE__)

inline constexpr auto SAMPLE_RATE            = 48000.0;
inline constexpr auto RESPONSE_LENGTH        = size_t { 1 << 16 };
inline constexpr auto MAGNITUDE_DB_TOLERANCE = 0.02;

[[nodiscard]]
auto make_specs() -> std::vector< dsp::BandSpec > {
    return {
        {   62.5f, 0.71f,  dsp::BandShape::LOW_SHELF },
        {  125.0f, 1.00f,       dsp::BandShape::PEAK },
        {  250.0f, 1.00f,       dsp::BandShape::PEAK },
        {  500.0f, 1.00f,       dsp::BandShape::PEAK },
        { 1000.0f, 1.00f,       dsp::BandShape::PEAK },
        { 2000.0f, 0.71f, dsp::BandShape::HIGH_SHELF }
    };
}

[[nodiscard]]
auto impulse_response(
    dsp::Equalizer& equalizer,
    size_t const    length
) -> std::vector< float > {
    return ranges::views::iota(size_t { 0 }, length) |
           ranges::views::transform([ & ](auto const index) noexcept -> float {
               return equalizer.process(index == 0 ? 1.0f : 0.0f);
           }) |
           ranges::to< std::vector< float > >();
}

[[nodiscard]]
auto magnitude_db(
    std::vector< float > const& response,
    double const                freq,
    double const                sample_rate
) noexcept -> double {
    auto const omega = 2.0 * util::PI * freq / sample_rate;

    auto const phasors =
        ranges::views::enumerate(response) |
        ranges::views::transform([ & ](auto const& indexed) noexcept -> std::complex< double > {
            auto const [ index, sample ] = indexed;
            return std::polar(static_cast< double >(sample), -omega * static_cast< double >(index));
        });

    return 20.0 * std::log10(std::abs(ranges::accumulate(phasors, std::complex< double > {})));
}

[[nodiscard]]
auto is_finite(std::vector< float > const& samples) noexcept -> bool {
    return ranges::all_of(samples, [](auto const sample) noexcept -> bool {
        return std::isfinite(sample);
    });
}

[[nodiscard]]
auto peak(std::vector< float > const& samples) noexcept -> float {
    auto const magnitudes =
        samples | ranges::views::transform([](auto const sample) noexcept -> float {
            return std::abs(sample);
        });

    return ranges::max(magnitudes);
}

auto test_biquad_passthrough() noexcept -> void {
    auto filter = dsp::Biquad({ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f });

    CHECK(filter.process(0.5f) == 0.5f);
    CHECK(filter.process(-1.0f) == -1.0f);
}

auto test_biquad_reset_clears_state() noexcept -> void {
    auto filter = dsp::Biquad({ 0.5f, 0.5f, 0.0f, 0.0f, 0.0f });

    CHECK(filter.process(1.0f) == 0.5f);
    filter.reset();
    CHECK(filter.process(0.0f) == 0.0f);
}

auto test_equalizer_is_transparent_at_unity() -> void {
    auto equalizer = dsp::Equalizer(SAMPLE_RATE, make_specs());

    for (auto index = 0; index < 512; ++index) {
        auto const input = std::sin(0.05 * index);
        CHECK_NEAR(equalizer.process(static_cast< float >(input)), input, 1.0e-4);
    }
}

auto test_equalizer_reports_its_configuration() -> void {
    auto const specs     = make_specs();
    auto       equalizer = dsp::Equalizer(SAMPLE_RATE, specs);

    CHECK(equalizer.get_num_bands() == specs.size());
    CHECK(equalizer.get_spec(0).freq == specs[ 0 ].freq);
    CHECK(equalizer.get_spec(5).shape == dsp::BandShape::HIGH_SHELF);
    CHECK(equalizer.get_gain(3) == 0.0f);

    equalizer.set_gain(3, -4.5f);
    CHECK(equalizer.get_gain(3) == -4.5f);
}

auto test_peak_band_hits_its_target_gain() -> void {
    for (auto const gain_db: { -12.0f, -6.0f, 6.0f, 12.0f }) {
        auto equalizer = dsp::Equalizer(SAMPLE_RATE, make_specs());
        equalizer.set_gain(4, gain_db);

        auto const response = impulse_response(equalizer, RESPONSE_LENGTH);

        CHECK_NEAR(magnitude_db(response, 1000.0, SAMPLE_RATE), gain_db, MAGNITUDE_DB_TOLERANCE);
        CHECK_NEAR(magnitude_db(response, 40.0, SAMPLE_RATE), 0.0, 0.2);
    }
}

auto test_low_shelf_lifts_dc_only() -> void {
    auto equalizer = dsp::Equalizer(SAMPLE_RATE, make_specs());
    equalizer.set_gain(0, 9.0f);

    auto const response = impulse_response(equalizer, RESPONSE_LENGTH);

    CHECK_NEAR(magnitude_db(response, 0.0, SAMPLE_RATE), 9.0, MAGNITUDE_DB_TOLERANCE);
    CHECK_NEAR(magnitude_db(response, SAMPLE_RATE * 0.5, SAMPLE_RATE), 0.0, MAGNITUDE_DB_TOLERANCE);
}

auto test_high_shelf_lifts_nyquist_only() -> void {
    auto equalizer = dsp::Equalizer(SAMPLE_RATE, make_specs());
    equalizer.set_gain(5, -9.0f);

    auto const response = impulse_response(equalizer, RESPONSE_LENGTH);

    CHECK_NEAR(
        magnitude_db(response, SAMPLE_RATE * 0.5, SAMPLE_RATE), -9.0, MAGNITUDE_DB_TOLERANCE
    );
    CHECK_NEAR(magnitude_db(response, 0.0, SAMPLE_RATE), 0.0, MAGNITUDE_DB_TOLERANCE);
}

auto test_bands_cascade_multiply() -> void {
    auto low  = dsp::Equalizer(SAMPLE_RATE, make_specs());
    auto high = dsp::Equalizer(SAMPLE_RATE, make_specs());
    auto both = dsp::Equalizer(SAMPLE_RATE, make_specs());

    low.set_gain(1, 6.0f);
    high.set_gain(4, -9.0f);
    both.set_gain(1, 6.0f);
    both.set_gain(4, -9.0f);

    auto const low_response  = impulse_response(low, RESPONSE_LENGTH);
    auto const high_response = impulse_response(high, RESPONSE_LENGTH);
    auto const both_response = impulse_response(both, RESPONSE_LENGTH);

    for (auto const freq: { 80.0, 125.0, 400.0, 1000.0, 4000.0 }) {
        CHECK_NEAR(
            magnitude_db(both_response, freq, SAMPLE_RATE),
            magnitude_db(low_response, freq, SAMPLE_RATE) +
                magnitude_db(high_response, freq, SAMPLE_RATE),
            MAGNITUDE_DB_TOLERANCE
        );
    }
}

auto test_extreme_settings_stay_stable() -> void {
    auto equalizer = dsp::Equalizer(SAMPLE_RATE, make_specs());

    for (auto band = size_t { 0 }; band < equalizer.get_num_bands(); ++band) {
        equalizer.set_gain(band, 12.0f);
    }

    auto const response = impulse_response(equalizer, RESPONSE_LENGTH);
    auto const tail     = std::vector< float >(response.end() - 1024, response.end());

    CHECK(is_finite(response));
    CHECK(peak(tail) < 1.0e-6f);
}

auto test_bands_above_nyquist_stay_finite() -> void {
    auto const low_rate  = 8000.0;
    auto       equalizer = dsp::Equalizer(low_rate, make_specs());

    for (auto band = size_t { 0 }; band < equalizer.get_num_bands(); ++band) {
        equalizer.set_gain(band, -12.0f);
    }

    CHECK(is_finite(impulse_response(equalizer, 4096)));
}

auto test_reset_makes_processing_repeatable() -> void {
    auto equalizer = dsp::Equalizer(SAMPLE_RATE, make_specs());
    equalizer.set_gain(2, 8.0f);

    auto const first = impulse_response(equalizer, 2048);
    equalizer.reset();
    auto const second = impulse_response(equalizer, 2048);

    CHECK(first == second);
    CHECK(peak(first) > 0.0f);
}

auto test_equalizer_holds_at_another_sample_rate() -> void {
    auto const other_rate = 96000.0;
    auto       equalizer  = dsp::Equalizer(other_rate, make_specs());

    equalizer.set_gain(4, 6.0f);

    auto const response = impulse_response(equalizer, RESPONSE_LENGTH);

    CHECK_NEAR(magnitude_db(response, 1000.0, other_rate), 6.0, MAGNITUDE_DB_TOLERANCE);
}

auto test_smoother_reaches_its_target_in_one_ramp() noexcept -> void {
    auto const ramp_seconds = 0.05;
    auto const ramp_samples = static_cast< uint32_t >(ramp_seconds * SAMPLE_RATE);
    auto       smoother     = dsp::LinearSmoother(SAMPLE_RATE, ramp_seconds, 0.0f);

    smoother.set_target(1.0f);

    CHECK_NEAR(smoother.advance(ramp_samples / 4), 0.25f, 1.0e-4);
    CHECK_NEAR(smoother.advance(ramp_samples / 4), 0.50f, 1.0e-4);
    CHECK(smoother.advance(ramp_samples / 4 - 1) < 1.0f);
    CHECK_NEAR(smoother.advance(ramp_samples), 1.0f, 1.0e-6);
}

auto test_smoother_is_monotonic_across_blocks() noexcept -> void {
    auto const block    = uint32_t { 16 };
    auto       smoother = dsp::LinearSmoother(SAMPLE_RATE, 0.05, 0.0f);

    smoother.set_target(1.0f);

    auto previous = 0.0f;

    for (auto index = uint32_t { 0 }; index < 100; ++index) {
        auto const current = smoother.advance(block);
        CHECK(current > previous);
        CHECK(current < 1.0f);
        previous = current;
    }
}

auto test_smoother_per_sample_matches_per_block() noexcept -> void {
    auto per_sample = dsp::LinearSmoother(SAMPLE_RATE, 0.05, 0.0f);
    auto per_block  = dsp::LinearSmoother(SAMPLE_RATE, 0.05, 0.0f);

    per_sample.set_target(1.0f);
    per_block.set_target(1.0f);

    auto stepped = 0.0f;

    for (auto index = uint32_t { 0 }; index < 16; ++index) {
        stepped = per_sample.advance(1);
    }

    CHECK_NEAR(stepped, per_block.advance(16), 1.0e-6);

    for (auto index = uint32_t { 0 }; index < 4096; ++index) {
        stepped = per_sample.advance(1);
    }

    CHECK_NEAR(stepped, 1.0f, 1.0e-6);
}

auto test_smoother_ramps_downwards() noexcept -> void {
    auto smoother = dsp::LinearSmoother(SAMPLE_RATE, 0.05, 1.0f);

    smoother.set_target(-1.0f);

    auto const halfway = smoother.advance(1200);

    CHECK(halfway < 1.0f);
    CHECK(halfway > -1.0f);
    CHECK_NEAR(smoother.advance(4800), -1.0f, 1.0e-6);
}

auto test_smoother_never_overshoots() noexcept -> void {
    auto smoother = dsp::LinearSmoother(SAMPLE_RATE, 0.05, 0.0f);

    smoother.set_target(0.25f);

    CHECK_NEAR(smoother.advance(1u << 20), 0.25f, 1.0e-6);
}

auto test_smoother_holds_without_a_target() noexcept -> void {
    auto smoother = dsp::LinearSmoother(SAMPLE_RATE, 0.05, 0.75f);

    CHECK(smoother.advance(4096) == 0.75f);
    smoother.set_target(0.75f);
    CHECK(smoother.advance(4096) == 0.75f);
}

auto test_smoother_advance_of_zero_frames_holds() noexcept -> void {
    auto smoother = dsp::LinearSmoother(SAMPLE_RATE, 0.05, 0.0f);

    smoother.set_target(1.0f);

    CHECK(smoother.advance(0) == 0.0f);
    CHECK(smoother.get_current() == 0.0f);
}

auto test_smoother_snaps() noexcept -> void {
    auto smoother = dsp::LinearSmoother(SAMPLE_RATE, 0.05, 0.0f);

    smoother.set_target(-3.0f);
    smoother.snap();

    CHECK(smoother.get_current() == -3.0f);
    CHECK(smoother.advance(64) == -3.0f);
}
}  // namespace

auto main() -> int {
    test_biquad_passthrough();
    test_biquad_reset_clears_state();
    test_equalizer_is_transparent_at_unity();
    test_equalizer_reports_its_configuration();
    test_peak_band_hits_its_target_gain();
    test_low_shelf_lifts_dc_only();
    test_high_shelf_lifts_nyquist_only();
    test_bands_cascade_multiply();
    test_extreme_settings_stay_stable();
    test_bands_above_nyquist_stay_finite();
    test_reset_makes_processing_repeatable();
    test_equalizer_holds_at_another_sample_rate();
    test_smoother_reaches_its_target_in_one_ramp();
    test_smoother_is_monotonic_across_blocks();
    test_smoother_per_sample_matches_per_block();
    test_smoother_ramps_downwards();
    test_smoother_never_overshoots();
    test_smoother_holds_without_a_target();
    test_smoother_advance_of_zero_frames_holds();
    test_smoother_snaps();

    if (failures == 0) {
        std::printf("all dsp tests passed\n");
        return 0;
    }

    std::printf("%d dsp check(s) failed\n", failures);
    return 1;
}
