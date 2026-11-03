#include "WaveformState.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace wavesketch
{

namespace
{
    constexpr float kMinPointGap = 1.0f / kTableSize;
    constexpr float kMaxPointX = 1.0f - kMinPointGap;

    juce::String encodeFloats (const float* data, size_t count)
    {
        return juce::MemoryBlock (data, count * sizeof (float)).toBase64Encoding();
    }

    std::optional<std::vector<float>> decodeFloats (const juce::String& text)
    {
        juce::MemoryBlock block;
        if (! block.fromBase64Encoding (text) || block.getSize() % sizeof (float) != 0)
            return std::nullopt;

        std::vector<float> values (block.getSize() / sizeof (float));
        std::memcpy (values.data(), block.getData(), block.getSize());

        if (! std::all_of (values.begin(), values.end(), [] (float v) { return std::isfinite (v); }))
            return std::nullopt;

        return values;
    }
}

//==============================================================================
std::vector<ControlPoint> sanitisePoints (std::vector<ControlPoint> points)
{
    points.erase (std::remove_if (points.begin(), points.end(),
                                  [] (const ControlPoint& p) { return ! std::isfinite (p.x) || ! std::isfinite (p.y); }),
                  points.end());

    for (auto& p : points)
    {
        p.x = std::clamp (p.x, 0.0f, kMaxPointX);
        p.y = std::clamp (p.y, -1.0f, 1.0f);
    }

    std::stable_sort (points.begin(), points.end(), [] (const auto& a, const auto& b) { return a.x < b.x; });

    std::vector<ControlPoint> result;
    for (const auto& p : points)
        if (result.empty() || p.x - result.back().x >= kMinPointGap)
            result.push_back (p);

    if (result.size() > static_cast<size_t> (WaveformState::kMaxPoints))
        result.resize (static_cast<size_t> (WaveformState::kMaxPoints));

    return result;
}

WaveformData renderPoints (const std::vector<ControlPoint>& input)
{
    WaveformData out {};
    const auto pts = sanitisePoints (input);
    const int n = static_cast<int> (pts.size());

    if (n == 0)
        return out;

    if (n == 1)
    {
        out.fill (pts.front().y);
        return out;
    }

    // La courbe est périodique : avant le premier point vient le dernier (décalé d'une période),
    // après le dernier revient le premier.
    const auto X = [&] (int i) -> double
    {
        if (i < 0)  return pts[static_cast<size_t> (n - 1)].x - 1.0;
        if (i >= n) return pts[static_cast<size_t> (i - n)].x + 1.0;
        return pts[static_cast<size_t> (i)].x;
    };
    const auto Y = [&] (int i) -> double { return pts[static_cast<size_t> (((i % n) + n) % n)].y; };

    // Pente de la tangente en chaque point (Fritsch-Carlson) : nulle aux sommets et creux,
    // ce qui empêche la courbe de dépasser les points.
    std::vector<double> tangent (static_cast<size_t> (n + 1));
    for (int i = 0; i < n; ++i)
    {
        const double h0 = X (i) - X (i - 1), h1 = X (i + 1) - X (i);
        const double d0 = (Y (i) - Y (i - 1)) / h0, d1 = (Y (i + 1) - Y (i)) / h1;

        tangent[static_cast<size_t> (i)] = d0 * d1 <= 0.0 ? 0.0
                                                          : 3.0 * (h0 + h1) / ((2.0 * h1 + h0) / d0 + (h1 + 2.0 * h0) / d1);
    }
    tangent[static_cast<size_t> (n)] = tangent[0];

    for (int s = 0; s < kTableSize; ++s)
    {
        double t = static_cast<double> (s) / kTableSize;
        if (t < X (0))
            t += 1.0; // portion avant le premier point = fin du dernier segment

        int i = 0;
        while (i < n - 1 && t >= X (i + 1))
            ++i;

        const double h = X (i + 1) - X (i);
        const double u = (t - X (i)) / h;
        const double u2 = u * u, u3 = u2 * u;

        const double value = (2.0 * u3 - 3.0 * u2 + 1.0) * Y (i)
                           + (u3 - 2.0 * u2 + u) * h * tangent[static_cast<size_t> (i)]
                           + (-2.0 * u3 + 3.0 * u2) * Y (i + 1)
                           + (u3 - u2) * h * tangent[static_cast<size_t> (i + 1)];

        out[static_cast<size_t> (s)] = static_cast<float> (std::clamp (value, -1.0, 1.0));
    }

    return out;
}

//==============================================================================
WaveformState WaveformState::fromSamples (const WaveformData& samples)
{
    WaveformState state;
    state.mode = EditMode::freehand;
    state.samples = samples;
    return state;
}

WaveformState WaveformState::fromPoints (std::vector<ControlPoint> newPoints)
{
    WaveformState state;
    state.mode = EditMode::points;
    state.points = sanitisePoints (std::move (newPoints));
    state.samples = renderPoints (state.points);
    return state;
}

WaveformState WaveformState::withMode (EditMode newMode) const
{
    if (newMode == mode)
        return *this;

    if (newMode == EditMode::freehand)
    {
        auto copy = *this;
        copy.mode = EditMode::freehand;
        return copy;
    }

    if (points.size() >= static_cast<size_t> (kMinPoints) && renderPoints (points) == samples)
    {
        auto copy = *this;
        copy.mode = EditMode::points;
        return copy;
    }

    std::vector<ControlPoint> sampled;
    for (int k = 0; k < kPointsWhenConverting; ++k)
    {
        const int index = k * kTableSize / kPointsWhenConverting;
        sampled.push_back ({ static_cast<float> (index) / kTableSize, samples[static_cast<size_t> (index)] });
    }

    return fromPoints (std::move (sampled));
}

//==============================================================================
juce::ValueTree WaveformState::toValueTree() const
{
    juce::ValueTree tree (treeType);
    tree.setProperty ("version", 1, nullptr);
    tree.setProperty ("mode", mode == EditMode::points ? "points" : "freehand", nullptr);
    tree.setProperty ("samples", encodeFloats (samples.data(), samples.size()), nullptr);

    std::vector<float> flat;
    for (const auto& p : points)
    {
        flat.push_back (p.x);
        flat.push_back (p.y);
    }
    tree.setProperty ("points", encodeFloats (flat.data(), flat.size()), nullptr);
    return tree;
}

std::optional<WaveformState> WaveformState::fromValueTree (const juce::ValueTree& tree)
{
    if (! tree.hasType (treeType))
        return std::nullopt;

    const auto samples = decodeFloats (tree.getProperty ("samples").toString());
    if (! samples || samples->size() != static_cast<size_t> (kTableSize))
        return std::nullopt;

    WaveformState state;
    for (size_t i = 0; i < state.samples.size(); ++i)
        state.samples[i] = std::clamp ((*samples)[i], -1.0f, 1.0f);

    if (const auto flat = decodeFloats (tree.getProperty ("points").toString()); flat && flat->size() % 2 == 0)
    {
        std::vector<ControlPoint> pts;
        for (size_t i = 0; i < flat->size(); i += 2)
            pts.push_back ({ (*flat)[i], (*flat)[i + 1] });
        state.points = sanitisePoints (std::move (pts));
    }

    const bool wantsPoints = tree.getProperty ("mode").toString() == "points";
    state.mode = wantsPoints && state.points.size() >= static_cast<size_t> (kMinPoints) ? EditMode::points
                                                                                        : EditMode::freehand;
    return state;
}

//==============================================================================
juce::Result saveWaveformFile (const WaveformState& state, const juce::File& file)
{
    const auto xml = state.toValueTree().createXml();

    if (xml == nullptr || ! xml->writeTo (file))
        return juce::Result::fail (juce::String::fromUTF8 ("Impossible d'écrire le fichier ") + file.getFullPathName());

    return juce::Result::ok();
}

std::optional<WaveformState> loadWaveformFile (const juce::File& file, juce::String& errorMessage)
{
    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr)
    {
        errorMessage = "Ce fichier n'est pas un fichier XML lisible.";
        return std::nullopt;
    }

    auto state = WaveformState::fromValueTree (juce::ValueTree::fromXml (*xml));

    if (! state)
        errorMessage = "Ce fichier ne contient pas de waveform WaveSketch valide.";

    return state;
}

} // namespace wavesketch
