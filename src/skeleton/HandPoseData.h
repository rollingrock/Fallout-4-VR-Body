#pragma once

#include <array>
#include <cstdint>

#include "RE/NetImmerse/NiPoint.h"

namespace frik
{
    // Pose values for a single finger.
    // Flex: 0.0 = bent/closed, 1.0 = straight/open.
    // Splay: positive/negative Y-axis rotation on the proximal joint (side-to-side).
    struct FingerPose
    {
        float prox = 0.0f; // proximal (MCP / knuckle) joint flex
        float mid = 0.0f; // middle (PIP) joint flex
        float dist = 0.0f; // distal (DIP / fingertip) joint flex
        float splay = 0.0f; // lateral abduction/adduction of the proximal joint

        constexpr FingerPose() noexcept = default;

        constexpr FingerPose(const float proxValue, const float midValue, const float distValue, const float splayValue = 0.0f) noexcept
            : prox(proxValue),
              mid(midValue),
              dist(distValue),
              splay(splayValue)
        {}
    };
}

namespace frik::skeleton::data
{
    enum class HandPoseKind : std::uint8_t
    {
        Unset,
        Custom,
        Open,
        Pointing,
        HoldingWeapon,
        OffhandGrip,
        Attaboy,
        ThumbsUp,
        Fist,
        HoldingGun,
        HoldingMelee,
    };
}

namespace frik
{
    // Full hand pose: all 5 fingers, each with flex + splay, plus optional palm motion.
    struct HandFingersPose
    {
        FingerPose thumb;
        FingerPose index;
        FingerPose middle;
        FingerPose ring;
        FingerPose pinky;
        float palmPitch = 0.0f; // wrist flexion (+) / extension (-), degrees
        float palmYaw = 0.0f; // radial (+) / ulnar (-) deviation, degrees
        skeleton::data::HandPoseKind kind = skeleton::data::HandPoseKind::Custom;

        constexpr HandFingersPose() noexcept = default;

        constexpr HandFingersPose(const FingerPose thumbPose, const FingerPose indexPose, const FingerPose middlePose, const FingerPose ringPose, const FingerPose pinkyPose,
            const float palmPitch = 0.0f, const float palmYaw = 0.0f, const skeleton::data::HandPoseKind kindValue = skeleton::data::HandPoseKind::Custom) noexcept
            : thumb(thumbPose),
              index(indexPose),
              middle(middlePose),
              ring(ringPose),
              pinky(pinkyPose),
              palmPitch(palmPitch),
              palmYaw(palmYaw),
              kind(kindValue)
        {}

        /** Return a mutable finger pose by zero-based finger index. */
        FingerPose& getFingerAt(int fingerIndex) noexcept;
        /** Return a read-only finger pose by zero-based finger index. */
        const FingerPose& getFingerAt(int fingerIndex) const noexcept;
        /** Return the flat flex value for one of the 15 finger bones. */
        float getFlexAt(int boneIndex) const noexcept;
    };
}

namespace frik::skeleton::data
{
    using RotationData = std::array<float, 12>;

    enum class HandPoseOverrideTagState : std::uint8_t
    {
        None,
        Active,
        Overridden,
    };

    struct HandBonePoseData
    {
        const char* boneName;
        RotationData closedRotation;
        RotationData openRotation;
        RE::NiPoint3 openTranslation;
        RE::NiPoint3 openTranslationInPowerArmor;
    };

    const HandFingersPose& getOpenPose() noexcept;
    const HandFingersPose& getGunGripPose() noexcept;
    const HandFingersPose& getMeleeGripPose() noexcept;
    const HandFingersPose& getPointingPose() noexcept;
    const HandFingersPose& getAttaboyPose() noexcept;
    const HandFingersPose& getOffhandWeaponGripPose() noexcept;
    const HandFingersPose& getThumbsUpPose() noexcept;
    const HandFingersPose& getFistPose() noexcept;

    /**
     * Return the authored pose backing a predefined pose kind, so a new kind only
     * has to be wired up here rather than at every place that resolves one.
     *
     * @return null for kinds that carry no authored pose (Unset / Custom).
     */
    const HandFingersPose* getPoseForKind(HandPoseKind kind);

    const std::array<HandBonePoseData, 30>& getHandBoneData() noexcept;
}
