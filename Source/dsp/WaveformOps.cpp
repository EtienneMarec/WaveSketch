#include "WaveformOps.h"

#include <algorithm>
#include <cmath>

namespace wavesketch::ops
{

void drawLine (WaveformData& data, float index0, float value0, float index1, float value1) noexcept
{
    if (index1 < index0)
    {
        std::swap (index0, index1);
        std::swap (value0, value1);
    }

    index0 = std::clamp (index0, 0.0f, static_cast<float> (kTableSize - 1));
    index1 = std::clamp (index1, 0.0f, static_cast<float> (kTableSize - 1));
    value0 = std::clamp (value0, -1.0f, 1.0f);
    value1 = std::clamp (value1, -1.0f, 1.0f);

    const int first = static_cast<int> (std::ceil (index0));
    const int last = static_cast<int> (std::floor (index1));

    if (first > last)
    {
        // Les deux positions tombent entre deux échantillons : on modifie le plus proche.
        data[static_cast<size_t> (std::lround (index1))] = value1;
        return;
    }

    const float span = index1 - index0;

    for (int i = first; i <= last; ++i)
    {
        const float t = span > 0.0f ? (static_cast<float> (i) - index0) / span : 1.0f;
        data[static_cast<size_t> (i)] = value0 + t * (value1 - value0);
    }
}

void smooth (WaveformData& data, int radius) noexcept
{
    radius = std::clamp (radius, 1, kTableSize / 4);
    const float norm = 1.0f / static_cast<float> (2 * radius + 1);

    for (int pass = 0; pass < 2; ++pass)
    {
        const WaveformData source = data;
        float sum = 0.0f;

        for (int k = -radius; k <= radius; ++k)
            sum += source[static_cast<size_t> (k & kTableMask)];

        for (int i = 0; i < kTableSize; ++i)
        {
            data[static_cast<size_t> (i)] = sum * norm;
            sum += source[static_cast<size_t> ((i + radius + 1) & kTableMask)]
                 - source[static_cast<size_t> ((i - radius) & kTableMask)];
        }
    }
}

void invert (WaveformData& data) noexcept
{
    for (auto& v : data)
        v = -v;
}

} // namespace wavesketch::ops
