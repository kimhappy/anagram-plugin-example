#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "DistrhoPlugin.hpp"
#include "dsp/band.hh"
#include "dsp/equalizer.hh"
#include "dsp/smoother.hh"

START_NAMESPACE_DISTRHO

namespace {
static_assert(DISTRHO_PLUGIN_NUM_INPUTS == 1 || DISTRHO_PLUGIN_NUM_INPUTS == 2);
static_assert(DISTRHO_PLUGIN_NUM_INPUTS == DISTRHO_PLUGIN_NUM_OUTPUTS);

inline constexpr auto NUM_CHANNELS = uint32_t { DISTRHO_PLUGIN_NUM_INPUTS };

inline constexpr auto BYPASS_PARAMETER_INDEX     = uint32_t { 0 };
inline constexpr auto RESET_PARAMETER_INDEX      = uint32_t { 1 };
inline constexpr auto FIRST_GAIN_PARAMETER_INDEX = uint32_t { 2 };

[[nodiscard]]
auto channel_name(uint32_t const channel) noexcept -> char const* {
    return NUM_CHANNELS == 1 ? "Center" : channel == 0 ? "Left" : "Right";
}

[[nodiscard]]
auto channel_symbol(uint32_t const channel) noexcept -> char const* {
    return NUM_CHANNELS == 1 ? "center" : channel == 0 ? "left" : "right";
}

[[nodiscard]]
auto band_symbol(dsp::BandSpec const& spec) noexcept -> String {
    return String(static_cast< int >(spec.freq)) + "hz";
}

[[nodiscard]]
auto band_name(dsp::BandSpec const& spec) noexcept -> String {
    return String(spec.freq) + " Hz";
}
}  // namespace

class ExamplePlugin: public Plugin {
private:
    size_t                             num_bands_;
    float                              gain_db_abs_max_;
    std::vector< dsp::Equalizer >      equalizers_;
    std::vector< float >               gain_dbs_;
    std::vector< dsp::LinearSmoother > gain_smoothers_;
    std::vector< dsp::LinearSmoother > bypass_smoothers_;
    bool                               bypass_flag_ { false };
    bool                               reset_flag_ { false };

    [[nodiscard]]
    static auto _gain_index(uint32_t const index) noexcept -> size_t {
        return index - FIRST_GAIN_PARAMETER_INDEX;
    }

    auto _reset_state() noexcept -> void {
        for (auto& equalizer: equalizers_) {
            equalizer.reset();
        }

        for (auto& smoother: gain_smoothers_) {
            smoother.snap();
        }

        for (auto& smoother: bypass_smoothers_) {
            smoother.snap();
        }
    }

public:
    ExamplePlugin(
        double const                   smoothing_seconds,
        float const                    gain_db_abs_max,
        std::vector< dsp::BandSpec >&& specs
    ) noexcept:
        Plugin(
            FIRST_GAIN_PARAMETER_INDEX + NUM_CHANNELS * specs.size(),
            0,
            0
        ),
        num_bands_ { specs.size() },
        gain_db_abs_max_ { gain_db_abs_max },
        equalizers_(
            NUM_CHANNELS,
            dsp::Equalizer(
                getSampleRate(),
                std::move(specs)
            )
        ),
        gain_dbs_(
            NUM_CHANNELS * num_bands_,
            0.0f
        ),
        gain_smoothers_(
            NUM_CHANNELS * num_bands_,
            dsp::LinearSmoother(
                getSampleRate(),
                smoothing_seconds,
                0.0f
            )
        ),
        bypass_smoothers_(
            NUM_CHANNELS,
            dsp::LinearSmoother(
                getSampleRate(),
                smoothing_seconds,
                1.0f
            )
        ) {}

    ExamplePlugin(ExamplePlugin const&)                    = delete;
    auto operator=(ExamplePlugin const&) -> ExamplePlugin& = delete;

protected:
    [[nodiscard]]
    auto getVersion() const noexcept -> uint32_t override {
        return d_version(
            DISTRHO_PLUGIN_VERSION_MAJOR, DISTRHO_PLUGIN_VERSION_MINOR, DISTRHO_PLUGIN_VERSION_PATCH
        );
    }

    auto initParameter(
        uint32_t const index,
        Parameter&     parameter
    ) noexcept -> void override {
        if (index == BYPASS_PARAMETER_INDEX) {
            parameter.initDesignation(kParameterDesignationBypass);
            return;
        }

        if (index == RESET_PARAMETER_INDEX) {
            parameter.initDesignation(kParameterDesignationReset);
            return;
        }

        auto const gain_index = _gain_index(index);
        auto const channel    = static_cast< uint32_t >(gain_index / num_bands_);
        auto const spec       = equalizers_[ channel ].get_spec(gain_index % num_bands_);

        parameter.hints  = kParameterIsAutomatable;
        parameter.name   = String(channel_name(channel)) + " " + band_name(spec);
        parameter.symbol = String(channel_symbol(channel)) + "_" + band_symbol(spec);
        parameter.unit   = "dB";
        parameter.ranges = ParameterRanges(0.0f, -gain_db_abs_max_, gain_db_abs_max_);
    }

    [[nodiscard]]
    auto getParameterValue(uint32_t const index) const noexcept -> float override {
        if (index == BYPASS_PARAMETER_INDEX) {
            return bypass_flag_ ? 1.0f : 0.0f;
        }

        if (index == RESET_PARAMETER_INDEX) {
            return 0.0f;
        }

        return gain_dbs_[ _gain_index(index) ];
    }

    auto setParameterValue(
        uint32_t const index,
        float const    value
    ) noexcept -> void override {
        if (index == BYPASS_PARAMETER_INDEX) {
            bypass_flag_ = value > 0.5f;

            for (auto& smoother: bypass_smoothers_) {
                smoother.set_target(bypass_flag_ ? 0.0f : 1.0f);
            }

            return;
        }

        if (index == RESET_PARAMETER_INDEX) {
            reset_flag_ = reset_flag_ || value > 0.5f;
            return;
        }

        auto const gain_index = _gain_index(index);
        auto const gain_db    = std::clamp(value, -gain_db_abs_max_, gain_db_abs_max_);

        gain_dbs_[ gain_index ] = gain_db;
        gain_smoothers_[ gain_index ].set_target(gain_db);
    }

    auto activate() noexcept -> void override {
        _reset_state();
        reset_flag_ = true;
    }

    auto run(
        float const** const inputs,
        float** const       outputs,
        uint32_t const      frames
    ) noexcept -> void override {
        if (reset_flag_) {
            _reset_state();
            reset_flag_ = false;
        }

        for (auto channel = uint32_t { 0 }; channel < NUM_CHANNELS; ++channel) {
            auto& equalizer       = equalizers_[ channel ];
            auto& bypass_smoother = bypass_smoothers_[ channel ];

            for (auto band = size_t { 0 }; band < num_bands_; ++band) {
                equalizer.set_gain(
                    band, gain_smoothers_[ channel * num_bands_ + band ].advance(frames)
                );
            }

            auto const* const input  = inputs[ channel ];
            auto* const       output = outputs[ channel ];

            for (auto index = uint32_t { 0 }; index < frames; ++index) {
                auto const dry  = input[ index ];
                auto const mix  = bypass_smoother.advance(1);
                output[ index ] = dry + (equalizer.process(dry) - dry) * mix;
            }
        }
    }
};

auto createPlugin() -> Plugin* {
    return new ExamplePlugin(
        0.05,
        12.0f,
        std::vector< dsp::BandSpec > {
            { { 62.5f, 0.71f, dsp::BandShape::LOW_SHELF },
             { 125.0f, 1.00f, dsp::BandShape::PEAK },
             { 250.0f, 1.00f, dsp::BandShape::PEAK },
             { 500.0f, 1.00f, dsp::BandShape::PEAK },
             { 1000.0f, 1.00f, dsp::BandShape::PEAK },
             { 2000.0f, 0.71f, dsp::BandShape::HIGH_SHELF } }
    }
    );
}

END_NAMESPACE_DISTRHO
