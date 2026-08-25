#pragma once

namespace dsp {
enum class BandShape {
    LOW_SHELF,
    PEAK,
    HIGH_SHELF
};

struct BandSpec {
    float     freq;
    float     q;
    BandShape shape;
};
}  // namespace dsp
