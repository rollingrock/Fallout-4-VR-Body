#pragma once

#include <cmath>

#include "f4vr/MiscStructs.h"
#include "vrcf/VRControllersManager.h"

namespace frik
{
    /**
     * Return whether every element of a rotation matrix is a finite number.
     * Only the 3x3 rotation is checked; each row is an NiPoint4 whose "w" lane
     * is padding the engine does not keep meaningful.
     */
    inline bool isFiniteRotation(const RE::NiMatrix3& rotation)
    {
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                if (!std::isfinite(rotation.entry[row][column])) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Return whether a transform is safe to feed into the scene graph: every
     * component finite, and a scale far enough from zero to stay invertible.
     */
    inline bool isFiniteTransform(const RE::NiTransform& transform)
    {
        return isFiniteRotation(transform.rotate) && std::isfinite(transform.translate.x) && std::isfinite(transform.translate.y) && std::isfinite(transform.translate.z) &&
               std::isfinite(transform.scale) && std::abs(transform.scale) > 0.0001f;
    }

    void turnPlayerRadioOn(bool isActive);

    bool isAnyPipboyOpen();

    bool isCameraLookingAtObject(const RE::NiAVObject* cameraNode, const RE::NiAVObject* objectNode, float detectThresh);
    bool isArmorHasHeadLamp();

    bool isBetterScopesVRModLoaded();
    bool isFalloutLondonVRModLoaded();

    f4vr::MuzzleFlash* getMuzzleFlashNodes();

    float correctAdjustmentValue(float value, float sensitivityFactor);
}
