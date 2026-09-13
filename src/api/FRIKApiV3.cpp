#define FRIK_API_EXPORTS
#include "FRIKApiV3.h"

#include "ApiCore.h"
#include "RecoilControllerRuntime.h"
#include "ScopeAuthority.h"

#include <cstddef>
#include <intrin.h>

/**
 * FRIK API v3: the append-only function table.
 *
 * Like the v1-v4 and v2 tables, this file holds no logic and no state - entries either
 * point straight at a shared core function or convert this version's enums and structs
 * and call core. See ApiCore.h for the shared implementation. New entries go at the end
 * of the table and bump FRIK_API_V3_VERSION.
 */
namespace
{
    using namespace frik;
    using namespace frik::api;
    using namespace frik::skeleton::data;

    namespace core = frik::api::core;

    // Every v3 enum is value-identical to core's, so the shims cast instead of switching.
    static_assert(static_cast<int>(FRIKApiV3::Hand::Primary) == static_cast<int>(core::Hand::Primary));
    static_assert(static_cast<int>(FRIKApiV3::Hand::Offhand) == static_cast<int>(core::Hand::Offhand));
    static_assert(static_cast<int>(FRIKApiV3::Hand::Right) == static_cast<int>(core::Hand::Right));
    static_assert(static_cast<int>(FRIKApiV3::Hand::Left) == static_cast<int>(core::Hand::Left));
    static_assert(static_cast<int>(FRIKApiV3::Feature::Flashlight) == static_cast<int>(core::Feature::Flashlight));
    static_assert(static_cast<int>(FRIKApiV3::Feature::WeaponPositioning) == static_cast<int>(core::Feature::WeaponPositioning));
    static_assert(static_cast<int>(FRIKApiV3::Feature::Pipboy) == static_cast<int>(core::Feature::Pipboy));
    static_assert(static_cast<int>(FRIKApiV3::Feature::SmoothMovement) == static_cast<int>(core::Feature::SmoothMovement));

    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::Unset) == static_cast<int>(HandPoseKind::Unset));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::Custom) == static_cast<int>(HandPoseKind::Custom));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::Open) == static_cast<int>(HandPoseKind::Open));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::Pointing) == static_cast<int>(HandPoseKind::Pointing));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::HoldingWeapon) == static_cast<int>(HandPoseKind::HoldingWeapon));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::OffhandGrip) == static_cast<int>(HandPoseKind::OffhandGrip));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::Attaboy) == static_cast<int>(HandPoseKind::Attaboy));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::ThumbsUp) == static_cast<int>(HandPoseKind::ThumbsUp));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::Fist) == static_cast<int>(HandPoseKind::Fist));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::HoldingGun) == static_cast<int>(HandPoseKind::HoldingGun));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseKind::HoldingMelee) == static_cast<int>(HandPoseKind::HoldingMelee));

    static_assert(static_cast<int>(FRIKApiV3::HandPoseTagState::None) == static_cast<int>(HandPoseOverrideTagState::None));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseTagState::Active) == static_cast<int>(HandPoseOverrideTagState::Active));
    static_assert(static_cast<int>(FRIKApiV3::HandPoseTagState::Overridden) == static_cast<int>(HandPoseOverrideTagState::Overridden));

    static_assert(static_cast<int>(FRIKApiV3::ScopeCapability::KeepsBodyVisible) == static_cast<int>(ScopeCapability::KeepsBodyVisible));
    static_assert(static_cast<int>(FRIKApiV3::ScopeCapability::OwnsScopeCamera) == static_cast<int>(ScopeCapability::OwnsScopeCamera));
    static_assert(static_cast<int>(FRIKApiV3::ScopeCapability::PublishesLookingThrough) == static_cast<int>(ScopeCapability::PublishesLookingThrough));
    static_assert(static_cast<int>(FRIKApiV3::ScopeCapability::OwnsDamping) == static_cast<int>(ScopeCapability::OwnsDamping));

    // The published priority scale must stay in step with the internal one.
    static_assert(FRIKApiV3::HAND_POSE_PRIORITY_DEFAULT == core::HAND_POSE_PRIORITY_DEFAULT);
    static_assert(FRIKApiV3::HAND_POSE_PRIORITY_FRIK_INTERNAL == core::HAND_POSE_PRIORITY_FRIK_INTERNAL);

    // The recoil ABI is byte-identical to core's, so a v3 controller pointer is passed straight through.
    static_assert(sizeof(FRIKApiV3::RecoilSample) == sizeof(core::RecoilSample));
    static_assert(sizeof(FRIKApiV3::RecoilResponse) == sizeof(core::RecoilResponse));
    static_assert(offsetof(FRIKApiV3::RecoilSample, nativeKickLocal) == offsetof(core::RecoilSample, nativeKickLocal));
    static_assert(offsetof(FRIKApiV3::RecoilResponse, handMask) == offsetof(core::RecoilResponse, handMask));
    static_assert(offsetof(FRIKApiV3::RecoilResponse, delivery) == offsetof(core::RecoilResponse, delivery));
    static_assert(offsetof(FRIKApiV3::RecoilResponse, controlledKickLocal) == offsetof(core::RecoilResponse, controlledKickLocal));
    static_assert(static_cast<int>(FRIKApiV3::RecoilDelivery::Damped) == static_cast<int>(core::RecoilDelivery::Damped));
    static_assert(static_cast<int>(FRIKApiV3::RecoilDelivery::Direct) == static_cast<int>(core::RecoilDelivery::Direct));
    static_assert(static_cast<int>(FRIKApiV3::RecoilHandMask::None) == static_cast<int>(core::RecoilHandMask::None));
    static_assert(static_cast<int>(FRIKApiV3::RecoilHandMask::Primary) == static_cast<int>(core::RecoilHandMask::Primary));
    static_assert(static_cast<int>(FRIKApiV3::RecoilHandMask::Offhand) == static_cast<int>(core::RecoilHandMask::Offhand));

    static_assert(sizeof(FRIKApiV3::SkeletonLifecycleData) == sizeof(core::SkeletonLifecycleData));
    static_assert(offsetof(FRIKApiV3::SkeletonLifecycleData, generation) == offsetof(core::SkeletonLifecycleData, generation));
    static_assert(offsetof(FRIKApiV3::SkeletonLifecycleData, rootNode) == offsetof(core::SkeletonLifecycleData, rootNode));
    static_assert(offsetof(FRIKApiV3::SkeletonLifecycleData, inPowerArmor) == offsetof(core::SkeletonLifecycleData, inPowerArmor));

    void copyLocalTransformsToApiData(const std::array<RE::NiTransform, FINGER_BONE_COUNT>& localTransforms, const std::uint16_t enabledMask,
        FRIKApiV3::FingerLocalTransformOverride& outTransforms)
    {
        outTransforms = {};
        outTransforms.enabledMask = enabledMask;
        for (std::size_t i = 0; i < localTransforms.size(); ++i) {
            outTransforms.localTransforms[i] = localTransforms[i];
        }
    }

    std::uint32_t FRIK_CALL getVersion()
    {
        return FRIK_API_V3_VERSION;
    }

    RE::NiPoint3 FRIK_CALL getIndexFingerTipPosition(const FRIKApiV3::Hand hand)
    {
        return core::getIndexFingerTipPosition(static_cast<core::Hand>(hand));
    }

    FRIKApiV3::HandPoseTagState FRIK_CALL getHandPoseSetTagState(const char* tag, const FRIKApiV3::Hand hand)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return FRIKApiV3::HandPoseTagState::None;
        }

        return static_cast<FRIKApiV3::HandPoseTagState>(core::getHandPoseSetTagState(*normalizedTag, core::isLeftForHand(hand)));
    }

    FRIKApiV3::HandPoseKind FRIK_CALL getCurrentHandPose(const FRIKApiV3::Hand hand)
    {
        return static_cast<FRIKApiV3::HandPoseKind>(core::getCurrentHandPoseKind(core::isLeftForHand(hand)));
    }

    bool FRIK_CALL setHandPose(const char* tag, const FRIKApiV3::Hand hand, const FRIKApiV3::HandPoseKind handPose, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag || priority < 0) {
            return false;
        }

        const bool isLeft = core::isLeftForHand(hand);
        if (handPose == FRIKApiV3::HandPoseKind::Unset) {
            core::clearHandPose(*normalizedTag, isLeft);
            return true;
        }

        const auto* pose = getPoseForKind(static_cast<HandPoseKind>(handPose));
        if (!pose) {
            return false;
        }

        logger::sample("setHandPose tag:'{}' hand={} pose={} priority={}", *normalizedTag, FRIKApiV3::handName(hand), static_cast<int>(handPose), priority);
        core::setHandPose(*normalizedTag, isLeft, *pose, priority);
        return true;
    }

    bool FRIK_CALL setHandPoseCustom(const char* tag, const FRIKApiV3::Hand hand, const FRIKApiV3::HandPoseData& handPose, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag || priority < 0) {
            return false;
        }

        logger::sample("setHandPoseCustom tag:'{}' hand={} priority={}", *normalizedTag, FRIKApiV3::handName(hand), priority);
        core::setHandPose(*normalizedTag, core::isLeftForHand(hand), core::makeHandPoseFromApiData(handPose), priority);
        return true;
    }

    bool FRIK_CALL setHandPoseCustomLocalTransforms(const char* tag, const FRIKApiV3::Hand hand, const FRIKApiV3::FingerLocalTransformOverride* overrideData, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag || !overrideData || priority < 0) {
            return false;
        }

        std::array<RE::NiTransform, FINGER_BONE_COUNT> localTransforms{};
        for (std::size_t i = 0; i < localTransforms.size(); ++i) {
            localTransforms[i] = overrideData->localTransforms[i];
        }

        return core::setHandPoseLocalTransforms(*normalizedTag, core::isLeftForHand(hand), localTransforms, overrideData->enabledMask, priority);
    }

    bool FRIK_CALL getHandPoseLocalTransformsForPose(const FRIKApiV3::Hand hand, const FRIKApiV3::HandPoseData& handPose, FRIKApiV3::FingerLocalTransformOverride* outTransforms)
    {
        if (!outTransforms) {
            return false;
        }

        std::array<RE::NiTransform, FINGER_BONE_COUNT> localTransforms{};
        std::uint16_t enabledMask = 0;
        if (!core::getHandPoseLocalTransformsForPose(core::isLeftForHand(hand), core::makeHandPoseFromApiData(handPose), localTransforms, enabledMask)) {
            *outTransforms = {};
            return false;
        }

        copyLocalTransformsToApiData(localTransforms, enabledMask, *outTransforms);
        return true;
    }

    bool FRIK_CALL mirrorFingerLocalTransforms(const FRIKApiV3::Hand sourceHand, const FRIKApiV3::FingerLocalTransformOverride* sourceTransforms,
        FRIKApiV3::FingerLocalTransformOverride* outTargetTransforms)
    {
        if ((sourceHand != FRIKApiV3::Hand::Left && sourceHand != FRIKApiV3::Hand::Right) || !sourceTransforms || !outTargetTransforms) {
            return false;
        }
        *outTargetTransforms = {};

        std::array<RE::NiTransform, FINGER_BONE_COUNT> source{};
        for (std::size_t index = 0; index < source.size(); ++index) {
            source[index] = sourceTransforms->localTransforms[index];
        }

        std::array<RE::NiTransform, FINGER_BONE_COUNT> target{};
        std::uint16_t targetMask = 0;
        if (!core::mirrorFingerLocalTransforms(sourceHand == FRIKApiV3::Hand::Left, source, sourceTransforms->enabledMask, target, targetMask)) {
            return false;
        }

        copyLocalTransformsToApiData(target, targetMask, *outTargetTransforms);
        return true;
    }

    bool FRIK_CALL clearHandPose(const char* tag, const FRIKApiV3::Hand hand)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        logger::sample("clearHandPose tag:'{}' hand={}", *normalizedTag, FRIKApiV3::handName(hand));
        core::clearHandPose(*normalizedTag, core::isLeftForHand(hand));
        return true;
    }

    bool FRIK_CALL setHandWorldTransform(const char* tag, const FRIKApiV3::Hand hand, const RE::NiTransform& worldTransform, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag || priority < 0) {
            return false;
        }

        return core::setHandWorldTransform(*normalizedTag, core::isLeftForHand(hand), worldTransform, priority);
    }

    bool FRIK_CALL clearHandWorldTransform(const char* tag, const FRIKApiV3::Hand hand)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        return core::clearHandWorldTransform(*normalizedTag, core::isLeftForHand(hand));
    }

    bool FRIK_CALL registerOpenModSettingButtonToMainConfig(const FRIKApiV3::OpenExternalModConfigData& data)
    {
        return core::registerOpenModSettingButtonToMainConfig(data.buttonIconNifPath, data.callbackReceiverName, data.callbackMessageType);
    }

    bool FRIK_CALL blockFeature(const char* tag, const FRIKApiV3::Feature feature, const bool block)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        return core::blockFeature(*normalizedTag, static_cast<core::Feature>(feature), block);
    }

    bool FRIK_CALL isFeatureBlocked(const FRIKApiV3::Feature feature)
    {
        return core::isFeatureBlocked(static_cast<core::Feature>(feature));
    }

    bool FRIK_CALL registerRecoilController(const char* tag, const FRIKApiV3::WeaponHandRecoilController controller, void* userData, const int priority)
    {
        return frik::api::registerWeaponHandRecoilController(tag, reinterpret_cast<core::WeaponHandRecoilController>(controller), userData, priority);
    }

    constexpr FRIKApiV3 FRIK_API_V3_FUNCTIONS_TABLE{ .getVersion = &getVersion,
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
        .registerWeaponHandRecoilController = &registerRecoilController,
        .unregisterWeaponHandRecoilController = &frik::api::unregisterWeaponHandRecoilController,
        .getSkeletonGeneration = &core::getSkeletonGeneration,
        .isInPowerArmor = &core::isInPowerArmor,
        .setScopeProvider = &core::setScopeProvider,
        .clearScopeProvider = &core::clearScopeProvider,
        .setLookingThroughScope = &core::setLookingThroughScope,
        .isLookingThroughScope = &core::isLookingThroughScope };
}

namespace frik::api
{
    FRIK_API const FRIKApiV3* FRIK_CALL FRIKAPI_V3_GetApi()
    {
        core::logApiAcquired("FRIK API v3.*", FRIK_API_V3_VERSION, sizeof(FRIKApiV3), _ReturnAddress());
        return &FRIK_API_V3_FUNCTIONS_TABLE;
    }

    FRIK_API std::uint32_t FRIK_CALL FRIKAPI_V3_GetApiStructSize()
    {
        return sizeof(FRIKApiV3);
    }
}
