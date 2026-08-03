#pragma once

#include "f4vr/MiscStructs.h"
#include "vrcf/VRControllersManager.h"

namespace frik
{
    void turnPlayerRadioOn(bool isActive);

    bool isAnyPipboyOpen();

    bool isCameraLookingAtObject(const RE::NiAVObject* cameraNode, const RE::NiAVObject* objectNode, float detectThresh);
    bool isArmorHasHeadLamp();

    bool isBetterScopesVRModLoaded();
    bool isFalloutLondonVRModLoaded();

    f4vr::MuzzleFlash* getMuzzleFlashNodes();

    float correctAdjustmentValue(float value, float sensitivityFactor);
}
