#pragma once

#include "f4vr/MiscStructs.h"
#include "vrcf/VRControllersManager.h"

namespace frik
{
    void turnPlayerRadioOn(bool isActive);

    void triggerStrongHaptic(vrcf::Hand hand = vrcf::Hand::Primary);
    void triggerShortHaptic(vrcf::Hand hand = vrcf::Hand::Primary);

    bool isAnyPipboyOpen();

    bool isCameraLookingAtObject(const RE::NiAVObject* cameraNode, const RE::NiAVObject* objectNode, float detectThresh);
    bool isArmorHasHeadLamp();

    bool isBetterScopesVRModLoaded();
    bool isFalloutLondonVRModLoaded();

    f4vr::MuzzleFlash* getMuzzleFlashNodes();

    float correctAdjustmentValue(float value, float sensitivityFactor);
}
