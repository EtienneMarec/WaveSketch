#include "Presets.h"
#include "dsp/WaveShapes.h"

#include <cmath>
#include <functional>

namespace wavesketch
{

namespace
{
    constexpr double twoPi = 6.283185307179586;

    WaveformData generate (const std::function<double (double)>& f)
    {
        WaveformData data {};
        double peak = 0.0;
        std::array<double, kTableSize> raw {};

        for (int i = 0; i < kTableSize; ++i)
        {
            raw[static_cast<size_t> (i)] = f (static_cast<double> (i) / kTableSize);
            peak = std::max (peak, std::abs (raw[static_cast<size_t> (i)]));
        }

        // Le dessin occupe toute la hauteur de l'éditeur.
        for (size_t i = 0; i < raw.size(); ++i)
            data[i] = static_cast<float> (peak > 0.0 ? raw[i] / peak : 0.0);

        return data;
    }

    // Somme d'harmoniques : amplitudes[k] = amplitude de l'harmonique k + 1.
    WaveformData additive (std::initializer_list<double> amplitudes)
    {
        return generate ([amplitudes] (double t)
        {
            double v = 0.0;
            int k = 1;
            for (double a : amplitudes)
                v += a * std::sin (twoPi * k++ * t);
            return v;
        });
    }
}

const std::vector<WaveformPreset>& getBuiltInPresets()
{
    static const std::vector<WaveformPreset> presets = []
    {
        std::vector<WaveformPreset> list;
        const auto add = [&list] (const char* name, WaveformState state)
        {
            list.push_back ({ juce::String::fromUTF8 (name), std::move (state) });
        };

        add ("Sinus",          WaveformState::fromSamples (makeBasicShape (BasicShape::sine)));
        add ("Triangle",       WaveformState::fromSamples (makeBasicShape (BasicShape::triangle)));
        add ("Dents de scie",  WaveformState::fromSamples (makeBasicShape (BasicShape::saw)));
        add ("Carré",          WaveformState::fromSamples (makeBasicShape (BasicShape::square)));
        add ("Pulse 25 %",     WaveformState::fromSamples (generate ([] (double t) { return t < 0.25 ? 1.0 : -1.0; })));
        add ("Carré doux",     WaveformState::fromSamples (generate ([] (double t) { return std::tanh (4.0 * std::sin (twoPi * t)); })));
        add ("Demi-sinus",     WaveformState::fromSamples (generate ([] (double t) { return std::max (0.0, std::sin (twoPi * t)); })));
        add ("Sinus + octave", WaveformState::fromSamples (additive ({ 1.0, 0.6 })));
        add ("Orgue",          WaveformState::fromSamples (additive ({ 1.0, 0.8, 0.5, 0.6, 0.0, 0.35, 0.0, 0.3 })));
        add ("Voix douce",     WaveformState::fromSamples (additive ({ 0.5, 0.7, 1.0, 0.8, 0.4, 0.2, 0.1, 0.05 })));
        add ("Cuivre",         WaveformState::fromSamples (additive ({ 1.0, 0.7, 0.55, 0.45, 0.38, 0.32, 0.27, 0.22, 0.18, 0.15, 0.12, 0.1 })));

        add ("Bosses (points)",  WaveformState::fromPoints ({ { 0.0f, 0.0f }, { 0.12f, 0.9f }, { 0.25f, 0.2f }, { 0.38f, 0.7f },
                                                               { 0.5f, 0.0f }, { 0.7f, -0.85f }, { 0.85f, -0.3f } }));
        add ("Rampe (points)",   WaveformState::fromPoints ({ { 0.0f, -0.9f }, { 0.6f, 0.4f }, { 0.85f, 0.95f }, { 0.95f, 0.1f } }));
        add ("Vague (points)",   WaveformState::fromPoints ({ { 0.0f, 0.0f }, { 0.2f, 1.0f }, { 0.45f, -0.4f }, { 0.6f, 0.3f },
                                                               { 0.8f, -1.0f } }));
        return list;
    }();

    return presets;
}

} // namespace wavesketch
