#define FRIK_API_EXPORTS
#include "FRIKApiV2.h"

#include "ApiCore.h"
#include "RecoilControllerRuntime.h"

#include <optional>
#include <string>

/**
 * FRIK API v2: the current function table.
 *
 * Like the v1-v4 table, this file holds no logic and no state - entries either
 * point straight at a shared core function or convert this version's enums and
 * structs and call core. See ApiCore.h for the shared implementation.
 */
namespace
{
    using namespace frik;
    using namespace frik::api;
    using namespace frik::skeleton::data;

    namespace core = frik::api::core;

    // The hand and feature selectors are value-identical to core's, so the shims
    // cast instead of switching. These keep that assumption honest.
    static_assert(static_cast<int>(FRIKApiV2::Hand::Primary) == static_cast<int>(core::Hand::Primary));
    static_assert(static_cast<int>(FRIKApiV2::Hand::Offhand) == static_cast<int>(core::Hand::Offhand));
    static_assert(static_cast<int>(FRIKApiV2::Hand::Right) == static_cast<int>(core::Hand::Right));
    static_assert(static_cast<int>(FRIKApiV2::Hand::Left) == static_cast<int>(core::Hand::Left));
    static_assert(static_cast<int>(FRIKApiV2::Feature::Flashlight) == static_cast<int>(core::Feature::Flashlight));
    static_assert(static_cast<int>(FRIKApiV2::Feature::WeaponPositioning) == static_cast<int>(core::Feature::WeaponPositioning));
    static_assert(static_cast<int>(FRIKApiV2::Feature::Pipboy) == static_cast<int>(core::Feature::Pipboy));
    static_assert(static_cast<int>(FRIKApiV2::Feature::SmoothMovement) == static_cast<int>(core::Feature::SmoothMovement));

    // The published priority scale must stay in step with the internal one.
    static_assert(FRIKApiV2::HAND_POSE_PRIORITY_DEFAULT == core::HAND_POSE_PRIORITY_DEFAULT);
    static_assert(FRIKApiV2::HAND_POSE_PRIORITY_FRIK_INTERNAL == core::HAND_POSE_PRIORITY_FRIK_INTERNAL);

    HandPoseKind toCoreHandPoseKind(const FRIKApiV2::HandPoseKind kind)
    {
        switch (kind) {
        case FRIKApiV2::HandPoseKind::Unset:
            return HandPoseKind::Unset;
        case FRIKApiV2::HandPoseKind::Custom:
            return HandPoseKind::Custom;
        case FRIKApiV2::HandPoseKind::Open:
            return HandPoseKind::Open;
        case FRIKApiV2::HandPoseKind::Pointing:
            return HandPoseKind::Pointing;
        case FRIKApiV2::HandPoseKind::HoldingWeapon:
            return HandPoseKind::HoldingWeapon;
        case FRIKApiV2::HandPoseKind::OffhandGrip:
            return HandPoseKind::OffhandGrip;
        case FRIKApiV2::HandPoseKind::Attaboy:
            return HandPoseKind::Attaboy;
        case FRIKApiV2::HandPoseKind::ThumbsUp:
            return HandPoseKind::ThumbsUp;
        case FRIKApiV2::HandPoseKind::HoldingGun:
            return HandPoseKind::HoldingGun;
        case FRIKApiV2::HandPoseKind::HoldingMelee:
            return HandPoseKind::HoldingMelee;
        }
        return HandPoseKind::Unset;
    }

    /**
     * Fist has no published equivalent, so it reports as Unset.
     */
    FRIKApiV2::HandPoseKind toApiHandPoseKind(const HandPoseKind kind)
    {
        switch (kind) {
        case HandPoseKind::Unset:
            return FRIKApiV2::HandPoseKind::Unset;
        case HandPoseKind::Custom:
            return FRIKApiV2::HandPoseKind::Custom;
        case HandPoseKind::Open:
            return FRIKApiV2::HandPoseKind::Open;
        case HandPoseKind::Pointing:
            return FRIKApiV2::HandPoseKind::Pointing;
        case HandPoseKind::HoldingWeapon:
            return FRIKApiV2::HandPoseKind::HoldingWeapon;
        case HandPoseKind::OffhandGrip:
            return FRIKApiV2::HandPoseKind::OffhandGrip;
        case HandPoseKind::Attaboy:
            return FRIKApiV2::HandPoseKind::Attaboy;
        case HandPoseKind::ThumbsUp:
            return FRIKApiV2::HandPoseKind::ThumbsUp;
        case HandPoseKind::HoldingGun:
            return FRIKApiV2::HandPoseKind::HoldingGun;
        case HandPoseKind::HoldingMelee:
            return FRIKApiV2::HandPoseKind::HoldingMelee;
        case HandPoseKind::Fist:
            return FRIKApiV2::HandPoseKind::Unset;
        }
        return FRIKApiV2::HandPoseKind::Unset;
    }

    FRIKApiV2::HandPoseTagState toApiHandPoseTagState(const HandPoseOverrideTagState state)
    {
        switch (state) {
        case HandPoseOverrideTagState::None:
            return FRIKApiV2::HandPoseTagState::None;
        case HandPoseOverrideTagState::Active:
            return FRIKApiV2::HandPoseTagState::Active;
        case HandPoseOverrideTagState::Overridden:
            return FRIKApiV2::HandPoseTagState::Overriden;
        }
        return FRIKApiV2::HandPoseTagState::None;
    }

    void copyLocalTransformsToApiData(const std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& localTransforms, const std::uint16_t enabledMask,
        FRIKApiV2::FingerLocalTransformOverride& outTransforms)
    {
        outTransforms = {};
        outTransforms.enabledMask = enabledMask;
        for (std::size_t i = 0; i < localTransforms.size(); ++i) {
            outTransforms.localTransforms[i] = localTransforms[i];
        }
    }

    std::uint32_t FRIK_CALL getVersion()
    {
        return FRIK_API_V2_VERSION;
    }

    RE::NiPoint3 FRIK_CALL getIndexFingerTipPosition(const FRIKApiV2::Hand hand)
    {
        return core::getIndexFingerTipPosition(static_cast<core::Hand>(hand));
    }

    FRIKApiV2::HandPoseTagState FRIK_CALL getHandPoseSetTagState(const char* tag, const FRIKApiV2::Hand hand)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return FRIKApiV2::HandPoseTagState::None;
        }

        return toApiHandPoseTagState(core::getHandPoseSetTagState(*normalizedTag, core::isLeftForHand(hand)));
    }

    FRIKApiV2::HandPoseKind FRIK_CALL getCurrentHandPose(const FRIKApiV2::Hand hand)
    {
        return toApiHandPoseKind(core::getCurrentHandPoseKind(core::isLeftForHand(hand)));
    }

    bool FRIK_CALL setHandPose(const char* tag, const FRIKApiV2::Hand hand, const FRIKApiV2::HandPoseKind handPose, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag || priority < 0) {
            return false;
        }

        const bool isLeft = core::isLeftForHand(hand);
        if (handPose == FRIKApiV2::HandPoseKind::Unset) {
            core::clearHandPose(*normalizedTag, isLeft);
            return true;
        }

        const auto* pose = getPoseForKind(toCoreHandPoseKind(handPose));
        if (!pose) {
            return false;
        }

        logger::sample("APIv2 setHandPose tag:'{}' hand={} pose={} priority={}", *normalizedTag, FRIKApiV2::handName(hand), static_cast<int>(handPose), priority);
        core::setHandPose(*normalizedTag, isLeft, *pose, priority);
        return true;
    }

    bool FRIK_CALL setHandPoseCustom(const char* tag, const FRIKApiV2::Hand hand, const FRIKApiV2::HandPoseData& handPose, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag || priority < 0) {
            return false;
        }

        logger::sample("APIv2 setHandPoseCustom tag:'{}' hand={} priority={}", *normalizedTag, FRIKApiV2::handName(hand), priority);
        core::setHandPose(*normalizedTag, core::isLeftForHand(hand), core::makeHandPoseFromApiData(handPose), priority);
        return true;
    }

    bool FRIK_CALL setHandPoseCustomLocalTransforms(const char* tag, const FRIKApiV2::Hand hand, const FRIKApiV2::FingerLocalTransformOverride* overrideData, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag || !overrideData || priority < 0) {
            return false;
        }

        std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT> localTransforms{};
        for (std::size_t i = 0; i < localTransforms.size(); ++i) {
            localTransforms[i] = overrideData->localTransforms[i];
        }

        return core::setHandPoseLocalTransforms(*normalizedTag, core::isLeftForHand(hand), localTransforms, overrideData->enabledMask, priority);
    }

    bool FRIK_CALL getHandPoseLocalTransformsForPose(const FRIKApiV2::Hand hand, const FRIKApiV2::HandPoseData& handPose, FRIKApiV2::FingerLocalTransformOverride* outTransforms)
    {
        if (!outTransforms) {
            return false;
        }

        std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT> localTransforms{};
        std::uint16_t enabledMask = 0;
        if (!core::getHandPoseLocalTransformsForPose(core::isLeftForHand(hand), core::makeHandPoseFromApiData(handPose), localTransforms, enabledMask)) {
            *outTransforms = {};
            return false;
        }

        copyLocalTransformsToApiData(localTransforms, enabledMask, *outTransforms);
        return true;
    }

    bool FRIK_CALL mirrorFingerLocalTransforms(const FRIKApiV2::Hand sourceHand, const FRIKApiV2::FingerLocalTransformOverride* sourceTransforms,
        FRIKApiV2::FingerLocalTransformOverride* outTargetTransforms)
    {
        if ((sourceHand != FRIKApiV2::Hand::Left && sourceHand != FRIKApiV2::Hand::Right) || !sourceTransforms || !outTargetTransforms) {
            return false;
        }
        *outTargetTransforms = {};

        std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT> source{};
        for (std::size_t index = 0; index < source.size(); ++index) {
            source[index] = sourceTransforms->localTransforms[index];
        }

        std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT> target{};
        std::uint16_t targetMask = 0;
        if (!core::mirrorFingerLocalTransforms(sourceHand == FRIKApiV2::Hand::Left, source, sourceTransforms->enabledMask, target, targetMask)) {
            return false;
        }

        copyLocalTransformsToApiData(target, targetMask, *outTargetTransforms);
        return true;
    }

    bool FRIK_CALL clearHandPose(const char* tag, const FRIKApiV2::Hand hand)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        logger::sample("APIv2 clearHandPose tag:'{}' hand={}", *normalizedTag, FRIKApiV2::handName(hand));
        core::clearHandPose(*normalizedTag, core::isLeftForHand(hand));
        return true;
    }

    bool FRIK_CALL setHandWorldTransform(const char* tag, const FRIKApiV2::Hand hand, const RE::NiTransform& worldTransform, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag || priority < 0) {
            return false;
        }

        return core::setHandWorldTransform(*normalizedTag, core::isLeftForHand(hand), worldTransform, priority);
    }

    bool FRIK_CALL clearHandWorldTransform(const char* tag, const FRIKApiV2::Hand hand)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        return core::clearHandWorldTransform(*normalizedTag, core::isLeftForHand(hand));
    }

    bool FRIK_CALL registerOpenModSettingButtonToMainConfig(const FRIKApiV2::OpenExternalModConfigData& data)
    {
        return core::registerOpenModSettingButtonToMainConfig(data.buttonIconNifPath, data.callbackReceiverName, data.callbackMessageType);
    }

    bool FRIK_CALL blockFeature(const char* tag, const FRIKApiV2::Feature feature, const bool block)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        return core::blockFeature(*normalizedTag, static_cast<core::Feature>(feature), block);
    }

    bool FRIK_CALL isFeatureBlocked(const FRIKApiV2::Feature feature)
    {
        return core::isFeatureBlocked(static_cast<core::Feature>(feature));
    }

    constexpr FRIKApiV2 FRIK_API_V2_FUNCTIONS_TABLE{ .getVersion = &getVersion,
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
        .setHandPoseCustom = &setHandPoseCustom,
        .setHandPoseCustomLocalTransforms = &setHandPoseCustomLocalTransforms,
        .getHandPoseLocalTransformsForPose = &getHandPoseLocalTransformsForPose,
        .mirrorFingerLocalTransforms = &mirrorFingerLocalTransforms,
        .clearHandPose = &clearHandPose,
        .setHandWorldTransform = &setHandWorldTransform,
        .clearHandWorldTransform = &clearHandWorldTransform,
        .registerOpenModSettingButtonToMainConfig = &registerOpenModSettingButtonToMainConfig,
        .blockOffHandWeaponGripping = &core::blockOffHandWeaponGripping,
        .blockFeature = &blockFeature,
        .isFeatureBlocked = &isFeatureBlocked,
        .blockPrimaryHandWeaponPose = &core::blockPrimaryHandWeaponPose,
        .blockPrimaryWeaponNodeOwnership = &core::blockPrimaryWeaponNodeOwnership,
        .getConfigValue = &core::getConfigValue,
        .hasConfigValueOverride = &core::hasConfigValueOverride,
        .setConfigValueOverride = &core::setConfigValueOverride,
        .clearConfigValueOverride = &core::clearConfigValueOverride,
        .registerWeaponHandRecoilController = &frik::api::registerWeaponHandRecoilController,
        .unregisterWeaponHandRecoilController = &frik::api::unregisterWeaponHandRecoilController };
}

namespace frik::api
{
    FRIK_API const FRIKApiV2* FRIK_CALL FRIKAPI_V2_GetApi()
    {
        return &FRIK_API_V2_FUNCTIONS_TABLE;
    }

    FRIK_API std::uint32_t FRIK_CALL FRIKAPI_V2_GetApiStructSize()
    {
        return sizeof(FRIKApiV2);
    }
}
