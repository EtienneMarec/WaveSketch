#include <juce_core/juce_core.h>

#include "dsp/WaveShapes.h"
#include "dsp/WaveformOps.h"
#include "dsp/WavetableBuilder.h"

#include <cmath>

using namespace wavesketch;

class WaveformOpsTests final : public juce::UnitTest
{
public:
    WaveformOpsTests() : juce::UnitTest ("Operations de dessin", "WaveSketch") {}

    void runTest() override
    {
        beginTest ("Trait main levee : aucun trou entre deux positions eloignees");
        {
            WaveformData data {};
            data.fill (0.9f);
            ops::drawLine (data, 100.0f, 0.5f, 300.0f, -0.5f);
            for (int i = 100; i <= 300; ++i)
                expectWithinAbsoluteError (data[(size_t) i], 0.5f - (float) (i - 100) / 200.0f, 1.0e-5f);
            expectEquals (data[99], 0.9f);
            expectEquals (data[301], 0.9f);
        }

        beginTest ("Trait de droite a gauche et valeurs bornees");
        {
            WaveformData data {};
            ops::drawLine (data, 50.0f, 3.0f, 10.0f, -3.0f);
            expectEquals (data[10], -1.0f);
            expectEquals (data[50], 1.0f);
            for (int i = 10; i <= 50; ++i)
                expect (data[(size_t) i] >= -1.0f && data[(size_t) i] <= 1.0f);
        }

        beginTest ("Trait plus court qu'un echantillon");
        {
            WaveformData data {};
            ops::drawLine (data, 20.2f, 0.7f, 20.4f, 0.7f);
            expectEquals (data[20], 0.7f);
        }

        beginTest ("Indices hors table ignores sans plantage");
        {
            WaveformData data {};
            ops::drawLine (data, -500.0f, 0.2f, 99999.0f, 0.2f);
            expectEquals (data[0], 0.2f);
            expectEquals (data[kTableSize - 1], 0.2f);
        }

        beginTest ("Inversion de phase");
        {
            auto data = makeBasicShape (BasicShape::saw);
            const auto original = data;
            ops::invert (data);
            for (size_t i = 0; i < data.size(); ++i)
                expectEquals (data[i], -original[i]);
        }

        beginTest ("Lissage : adoucit les sauts, garde la moyenne, presque neutre sur un sinus");
        {
            auto square = makeBasicShape (BasicShape::square);
            double meanBefore = 0.0, meanAfter = 0.0;
            for (float v : square) meanBefore += v;
            ops::smooth (square);
            float maxJump = 0.0f;
            for (int i = 0; i < kTableSize; ++i)
            {
                meanAfter += square[(size_t) i];
                maxJump = std::max (maxJump, std::abs (square[(size_t) i] - square[(size_t) ((i + 1) & kTableMask)]));
            }
            expectLessThan (maxJump, 0.2f);
            expectWithinAbsoluteError (meanAfter / kTableSize, meanBefore / kTableSize, 1.0e-4);

            auto sine = makeBasicShape (BasicShape::sine);
            const auto original = sine;
            ops::smooth (sine);
            for (size_t i = 0; i < sine.size(); ++i)
                expectWithinAbsoluteError (sine[i], original[i], 0.01f);
        }

        beginTest ("Construction en arriere-plan puis publication");
        {
            TablePublisher publisher;
            {
                WavetableBuilder builder (publisher);
                for (int i = 0; i < 50; ++i)
                    builder.requestBuild (makeBasicShape (i % 2 == 0 ? BasicShape::sine : BasicShape::square));

                WavetableSet* received = nullptr;
                for (int attempt = 0; attempt < 200 && received == nullptr; ++attempt)
                {
                    received = publisher.takePending();
                    if (received == nullptr)
                        juce::Thread::sleep (5);
                }

                expect (received != nullptr);
                delete received;
            }
        }
    }
};

static WaveformOpsTests waveformOpsTests;
