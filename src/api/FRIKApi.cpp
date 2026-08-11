#define FRIK_API_EXPORTS
#include "FRIKApi.h"

#include "ApiCore.h"

#include <optional>
#include <string>

/**
 * FRIK API v1-v4: the published, stable function table.
 *
 * This file holds no logic and no state - every entry either points straight at
 * a shared core function (when the signature mentions no version-specific type)
 * or is a thin shim that converts this version's enums and structs and calls
 * core. See ApiCore.h for the shared implementation.
 */
namespace
{
    using namespace frik;
    using namespace frik::api;
    using namespace frik::skeleton::data;

    namespace core = frik::api::core;

    // The hand and feature selectors are value-identical to core's, so the shims
    // cast instead of switching. These keep that assumption honest.
    static_assert(static_cast<int>(FRIKApi::Hand::Primary) == static_cast<int>(core::Hand::Primary));
    static_assert(static_cast<int>(FRIKApi::Hand::Offhand) == static_cast<int>(core::Hand::Offhand));
    static_assert(static_cast<int>(FRIKApi::Hand::Right) == static_cast<int>(core::Hand::Right));
    static_assert(static_cast<int>(FRIKApi::Hand::Left) == static_cast<int>(core::Hand::Left));
    static_assert(static_cast<int>(FRIKApi::Feature::Flashlight) == static_cast<int>(core::Feature::Flashlight));
    static_assert(static_cast<int>(FRIKApi::Feature::WeaponPositioning) == static_cast<int>(core::Feature::WeaponPositioning));
    static_assert(static_cast<int>(FRIKApi::Feature::Pipboy) == static_cast<int>(core::Feature::Pipboy));
    static_assert(static_cast<int>(FRIKApi::Feature::SmoothMovement) == static_cast<int>(core::Feature::SmoothMovement));

    /**
     * Translate this version's pose kind into the internal one.
     * v4 has no Fist/HoldingGun/HoldingMelee, so those never arrive here.
     */
    HandPoseKind toCoreHandPoseKind(const FRIKApi::HandPoseKind kind)
    {
        switch (kind) {
        case FRIKApi::HandPoseKind::Unset:
            return HandPoseKind::Unset;
        case FRIKApi::HandPoseKind::Custom:
            return HandPoseKind::Custom;
        case FRIKApi::HandPoseKind::Open:
            return HandPoseKind::Open;
        case FRIKApi::HandPoseKind::Pointing:
            return HandPoseKind::Pointing;
        case FRIKApi::HandPoseKind::HoldingWeapon:
            return HandPoseKind::HoldingWeapon;
        case FRIKApi::HandPoseKind::OffhandGrip:
            return HandPoseKind::OffhandGrip;
        case FRIKApi::HandPoseKind::Attaboy:
            return HandPoseKind::Attaboy;
        case FRIKApi::HandPoseKind::ThumbsUp:
            return HandPoseKind::ThumbsUp;
        }
        return HandPoseKind::Unset;
    }

    /**
     * Translate the internal pose kind into the eight kinds a v4 client knows.
     *
     * Kinds introduced after v4 (Fist, HoldingGun, HoldingMelee) are folded down
     * to their nearest v4 equivalent, so a client compiled against this header
     * can never receive an enumerator that does not exist in it.
     */
    FRIKApi::HandPoseKind toApiHandPoseKind(const HandPoseKind kind)
    {
        switch (kind) {
        case HandPoseKind::Unset:
            return FRIKApi::HandPoseKind::Unset;
        case HandPoseKind::Custom:
            return FRIKApi::HandPoseKind::Custom;
        case HandPoseKind::Open:
            return FRIKApi::HandPoseKind::Open;
        case HandPoseKind::Pointing:
            return FRIKApi::HandPoseKind::Pointing;
        case HandPoseKind::HoldingWeapon:
        case HandPoseKind::HoldingGun:
        case HandPoseKind::HoldingMelee:
            return FRIKApi::HandPoseKind::HoldingWeapon;
        case HandPoseKind::OffhandGrip:
            return FRIKApi::HandPoseKind::OffhandGrip;
        case HandPoseKind::Attaboy:
            return FRIKApi::HandPoseKind::Attaboy;
        case HandPoseKind::ThumbsUp:
            return FRIKApi::HandPoseKind::ThumbsUp;
        case HandPoseKind::Fist:
            return FRIKApi::HandPoseKind::Unset;
        }
        return FRIKApi::HandPoseKind::Unset;
    }

    FRIKApi::HandPoseTagState toApiHandPoseTagState(const HandPoseOverrideTagState state)
    {
        switch (state) {
        case HandPoseOverrideTagState::None:
            return FRIKApi::HandPoseTagState::None;
        case HandPoseOverrideTagState::Active:
            return FRIKApi::HandPoseTagState::Active;
        case HandPoseOverrideTagState::Overridden:
            return FRIKApi::HandPoseTagState::Overriden;
        }
        return FRIKApi::HandPoseTagState::None;
    }

    std::uint32_t FRIK_CALL getVersion()
    {
        return FRIK_API_VERSION;
    }

    RE::NiPoint3 FRIK_CALL getIndexFingerTipPosition(const FRIKApi::Hand hand)
    {
        return core::getIndexFingerTipPosition(static_cast<core::Hand>(hand));
    }

    FRIKApi::HandPoseTagState FRIK_CALL getHandPoseSetTagState(const char* tag, const FRIKApi::Hand hand)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return FRIKApi::HandPoseTagState::None;
        }

        return toApiHandPoseTagState(core::getHandPoseSetTagState(*normalizedTag, core::isLeftForHand(hand)));
    }

    FRIKApi::HandPoseKind FRIK_CALL getCurrentHandPose(const FRIKApi::Hand hand)
    {
        return toApiHandPoseKind(core::getCurrentHandPoseKind(core::isLeftForHand(hand)));
    }

    bool FRIK_CALL setHandPose(const char* tag, const FRIKApi::Hand hand, const FRIKApi::HandPoseKind handPose)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        const bool isLeft = core::isLeftForHand(hand);
        if (handPose == FRIKApi::HandPoseKind::Unset) {
            core::clearHandPose(*normalizedTag, isLeft);
            return true;
        }

        const auto* pose = getPoseForKind(toCoreHandPoseKind(handPose));
        if (!pose) {
            return false;
        }

        logger::sample("API setHandPose tag:'{}' hand={} pose={}", *normalizedTag, FRIKApi::handName(hand), static_cast<int>(handPose));
        core::setHandPose(*normalizedTag, isLeft, *pose, core::HAND_POSE_PRIORITY_DEFAULT);
        return true;
    }

    bool FRIK_CALL setHandPoseCustomFingerPositions(const char* tag, const FRIKApi::Hand hand, const float thumb, const float index, const float middle, const float ring,
        const float pinky)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        logger::sample("API setHandPoseCustomFingerPositions tag:'{}' hand={}", *normalizedTag, FRIKApi::handName(hand));
        core::setHandPose(*normalizedTag,
            core::isLeftForHand(hand),
            HandFingersPose{ FingerPose{ thumb, thumb, thumb },
                FingerPose{ index, index, index },
                FingerPose{ middle, middle, middle },
                FingerPose{ ring, ring, ring },
                FingerPose{ pinky, pinky, pinky } },
            core::HAND_POSE_PRIORITY_DEFAULT);
        return true;
    }

    bool FRIK_CALL setHandPoseCustom(const char* tag, const FRIKApi::Hand hand, const FRIKApi::HandPoseData& handPose, const bool forceTop)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        logger::sample("API setHandPoseCustom tag:'{}' hand={} forceTop={}", *normalizedTag, FRIKApi::handName(hand), forceTop);
        core::setHandPose(*normalizedTag, core::isLeftForHand(hand), core::makeHandPoseFromApiData(handPose), core::priorityFromForceTop(forceTop));
        return true;
    }

    bool FRIK_CALL clearHandPose(const char* tag, const FRIKApi::Hand hand)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        logger::sample("API clearHandPose tag:'{}' hand={}", *normalizedTag, FRIKApi::handName(hand));
        core::clearHandPose(*normalizedTag, core::isLeftForHand(hand));
        return true;
    }

    void FRIK_CALL setHandPoseFingerPositions(const FRIKApi::Hand hand, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        logger::sample("API [DEPRECATED] setHandPoseFingerPositions hand={}", FRIKApi::handName(hand));
        core::setHandPose(core::LEGACY_API_HAND_POSE_TAG,
            core::isLeftForHand(hand),
            HandFingersPose{ FingerPose{ thumb, thumb, thumb },
                FingerPose{ index, index, index },
                FingerPose{ middle, middle, middle },
                FingerPose{ ring, ring, ring },
                FingerPose{ pinky, pinky, pinky } },
            core::HAND_POSE_PRIORITY_DEFAULT);
    }

    void FRIK_CALL clearHandPoseFingerPositions(const FRIKApi::Hand hand)
    {
        logger::sample("API [DEPRECATED] clearHandPoseFingerPositions hand={}", FRIKApi::handName(hand));
        core::clearHandPose(core::LEGACY_API_HAND_POSE_TAG, core::isLeftForHand(hand));
    }

    bool FRIK_CALL registerOpenModSettingButtonToMainConfig(const FRIKApi::OpenExternalModConfigData& data)
    {
        return core::registerOpenModSettingButtonToMainConfig(data.buttonIconNifPath, data.callbackReceiverName, data.callbackMessageType);
    }

    bool FRIK_CALL blockFeature(const char* tag, const FRIKApi::Feature feature, const bool block)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        return core::blockFeature(*normalizedTag, static_cast<core::Feature>(feature), block);
    }

    bool FRIK_CALL isFeatureBlocked(const FRIKApi::Feature feature)
    {
        return core::isFeatureBlocked(static_cast<core::Feature>(feature));
    }

    constexpr FRIKApi FRIK_API_FUNCTIONS_TABLE{ .getVersion = &getVersion,
        .getModVersion = &core::getModVersion,
        .isSkeletonReady = &core::isSkeletonReady,
        .isConfigOpen = &core::isConfigOpen,
        .isSelfieModeOn = &core::isSelfieModeOn,
        .setSelfieModeOn = &core::setSelfieModeOn,
        .isOffHandGrippingWeapon = &core::isOffHandGrippingWeapon,
        .isWristPipboyOpen = &core::isWristPipboyOpen,
        .getIndexFingerTipPosition = &getIndexFingerTipPosition,
        .getHandPoseSetTagState = &getHandPoseSetTagState,
        .getCurrentHandPose = &getCurrentHandPose,
        .setHandPose = &setHandPose,
        .setHandPoseCustomFingerPositions = &setHandPoseCustomFingerPositions,
        .clearHandPose = &clearHandPose,
        .setHandPoseFingerPositions = &setHandPoseFingerPositions,
        .clearHandPoseFingerPositions = &clearHandPoseFingerPositions,
        .registerOpenModSettingButtonToMainConfig = &registerOpenModSettingButtonToMainConfig,
        .blockOffHandWeaponGripping = &core::blockOffHandWeaponGripping,
        .setHandPoseCustom = &setHandPoseCustom,
        .blockFeature = &blockFeature,
        .isFeatureBlocked = &isFeatureBlocked,
        .getConfigValue = &core::getConfigValue,
        .hasConfigValueOverride = &core::hasConfigValueOverride,
        .setConfigValueOverride = &core::setConfigValueOverride,
        .clearConfigValueOverride = &core::clearConfigValueOverride };
}

namespace frik::api
{
    FRIK_API const FRIKApi* FRIK_CALL FRIKAPI_GetApi()
    {
        return &FRIK_API_FUNCTIONS_TABLE;
    }
}
