#include "WaveShapes.h"

#include <cmath>

namespace wavesketch
{

WaveformData makeBasicShape (BasicShape shape)
{
    constexpr double twoPi = 6.283185307179586;
    WaveformData data {};

    for (int i = 0; i < kTableSize; ++i)
    {
        const double t = static_cast<double> (i) / kTableSize;
        double v = 0.0;

        switch (shape)
        {
            case BasicShape::sine:     v = std::sin (twoPi * t); break;
            case BasicShape::triangle: v = t < 0.25 ? 4.0 * t : (t < 0.75 ? 2.0 - 4.0 * t : 4.0 * t - 4.0); break;
            case BasicShape::saw:      v = t < 0.5 ? 2.0 * t : 2.0 * t - 2.0; break;
            case BasicShape::square:   v = t < 0.5 ? 1.0 : -1.0; break;
        }

        data[static_cast<size_t> (i)] = static_cast<float> (v);
    }

    return data;
}

} // namespace wavesketch
