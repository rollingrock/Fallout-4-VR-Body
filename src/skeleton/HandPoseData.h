#pragma once

#include <array>

#include "HandPose.h"

namespace frik::skeleton::data
{
    using RotationData = std::array<float, 12>;

    struct HandBonePoseData
    {
        const char* boneName;
        RotationData closedRotation;
        RotationData openRotation;
        RE::NiPoint3 openTranslation;
        RE::NiPoint3 openTranslationInPowerArmor;
    };

    const HandFingersPose& getGunGripPose() noexcept;
    const HandFingersPose& getMeleeGripPose() noexcept;
    const HandFingersPose& getPointingPose() noexcept;
    const HandFingersPose& getAttaboyPose() noexcept;
    const HandFingersPose& getOffhandWeaponGripPose() noexcept;
    const HandFingersPose& getThumbsUpPose() noexcept;
    const HandFingersPose& getFistPose() noexcept;

    const std::array<HandBonePoseData, 30>& getHandBoneData() noexcept;
}
