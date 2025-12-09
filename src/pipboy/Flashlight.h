#pragma once

#include "PipboyPhysicalHandler.h"

namespace frik
{
    class Skeleton;

    class Flashlight
    {
    public:
        explicit Flashlight(Skeleton* skelly);

        void onFrameUpdate();

    private:
        void checkSwitchingFlashlightHeadHand();
        void adjustFlashlightTransformToHandOrHead() const;
        static void setLightValues();

        Skeleton* _skelly;

        // to stop continuous flashlight haptic feedback
        bool _flashlightHapticActivated = false;
    };
}
