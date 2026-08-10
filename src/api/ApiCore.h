#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "RE/NetImmerse/NiPoint.h"
#include "RE/NetImmerse/NiTransform.h"
#include "skeleton/HandPose.h"
#include "skeleton/HandPoseData.h"

/**
 * Internal implementation shared by every published FRIK API major version.
 *
 * This header must never include a public API header (FRIKApi.h / FRIKApiV2.h).
 * It speaks FRIK's own vocabulary - `bool isLeft`, `HandFingersPose`,
 * `skeleton::data::HandPoseKind` - so each published major is a thin projection
 * onto it and no major can constrain another. All cross-mod state (blocking
 * tags, external hand authority) lives here exactly once, so a v1 client and a
 * v2 client arbitrate against each other instead of fighting invisibly.
 */
namespace frik::api::core
{
#define FRIK_CORE_CALL __cdecl

    /**
     * The player hand to act on, with left-handed support.
     * Enumerator values are identical in every published API major, so the
     * version shims static_assert and cast rather than switch.
     */
    enum class Hand : std::uint8_t
    {
        Primary,
        Offhand,
        Right,
        Left,
    };

    /**
     * FRIK subsystems that external mods can turn off when they replace them.
     */
    enum class Feature : std::uint8_t
    {
        Flashlight,
        WeaponPositioning,
        Pipboy,
        SmoothMovement,
    };

    inline constexpr std::size_t FEATURE_COUNT = 4;

    /**
     * The hand-pose priority scale. HandPose owns the ordering, so these alias
     * its constants rather than restating the values.
     */
    inline constexpr int HAND_POSE_PRIORITY_DEFAULT = HandPose::PRIORITY_EXTERNAL_DEFAULT;
    inline constexpr int HAND_POSE_PRIORITY_FRIK_INTERNAL = HandPose::PRIORITY_FRIK_INTERNAL;

    /**
     * Tag used by the deprecated tagless v1 hand-pose functions.
     */
    inline constexpr std::string_view LEGACY_API_HAND_POSE_TAG = "frik.api.legacy";

    /**
     * Map the v1-era forceTop flag onto the priority scale.
     */
    constexpr int priorityFromForceTop(const bool forceTop)
    {
        return forceTop ? HAND_POSE_PRIORITY_FRIK_INTERNAL : HAND_POSE_PRIORITY_DEFAULT;
    }

    // ------------------------------------------------------------------
    // Shared exported functions.
    // These mention no version-specific type, so every API major stores the
    // same function pointer in its table - there is no per-version shim.
    // ------------------------------------------------------------------

    const char* FRIK_CORE_CALL getModVersion();
    bool FRIK_CORE_CALL isSkeletonReady();
    bool FRIK_CORE_CALL isConfigOpen();
    bool FRIK_CORE_CALL isSelfieModeOn();
    void FRIK_CORE_CALL setSelfieModeOn(bool setOn);
    bool FRIK_CORE_CALL isOffHandGrippingWeapon();
    bool FRIK_CORE_CALL isWristPipboyOpen();
    bool FRIK_CORE_CALL blockOffHandWeaponGripping(const char* tag, bool block);
    bool FRIK_CORE_CALL blockPrimaryHandWeaponPose(const char* tag, bool block);
    bool FRIK_CORE_CALL blockPrimaryWeaponNodeOwnership(const char* tag, bool block);
    int FRIK_CORE_CALL getConfigValue(const char* caller, const char* section, const char* key, char* outBuf, int bufLen, const char* defaultValue);
    bool FRIK_CORE_CALL hasConfigValueOverride(const char* caller, const char* section, const char* key);
    bool FRIK_CORE_CALL setConfigValueOverride(const char* caller, const char* section, const char* key, const char* value);
    bool FRIK_CORE_CALL clearConfigValueOverride(const char* caller, const char* section, const char* key);

    // ------------------------------------------------------------------
    // Neutral-typed operations. Each API major converts its own enums and
    // structs at the boundary and calls straight through to these.
    // ------------------------------------------------------------------

    /**
     * Trim a client-supplied tag, rejecting null/blank ones.
     */
    std::optional<std::string> normalizeTag(const char* tag);

    /**
     * Resolve a hand selector against the current left-handed setting.
     */
    bool isLeftForHand(Hand hand);

    RE::NiPoint3 getIndexFingerTipPosition(Hand hand);

    skeleton::data::HandPoseOverrideTagState getHandPoseSetTagState(std::string_view tag, bool isLeft);
    skeleton::data::HandPoseKind getCurrentHandPoseKind(bool isLeft);

    /**
     * Build the authored pose backing a predefined pose kind.
     * Returns nullopt for kinds that carry no authored pose (Unset / Custom).
     */
    std::optional<HandFingersPose> makePredefinedHandPose(skeleton::data::HandPoseKind kind);

    /**
     * Build a pose whose joints all share one flex value per finger.
     */
    HandFingersPose makeUniformFingerPose(float thumb, float index, float middle, float ring, float pinky);

    void setHandPose(std::string_view tag, bool isLeft, const HandFingersPose& pose, int priority);
    void clearHandPose(std::string_view tag, bool isLeft);

    bool setHandTransform(std::string_view tag, bool isLeft, const RE::NiTransform& worldTransform, int priority);
    bool clearHandTransform(std::string_view tag, bool isLeft);

    bool setHandPoseLocalTransforms(std::string_view tag, bool isLeft, const std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& localTransforms, std::uint16_t enabledMask,
        int priority);

    bool getHandPoseLocalTransformsForPose(bool isLeft, const HandFingersPose& pose, std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& outTransforms,
        std::uint16_t& outEnabledMask);

    bool mirrorFingerLocalTransforms(bool sourceIsLeft, const std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& sourceTransforms, std::uint16_t sourceEnabledMask,
        std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask);

    bool blockFeature(std::string_view tag, Feature feature, bool block);
    bool isFeatureBlocked(Feature feature);

    bool registerOpenModSettingButtonToMainConfig(const char* buttonIconNifPath, const char* callbackReceiverName, std::uint32_t callbackMessageType);

    /**
     * Drop every external-authority registration when the skeleton is released.
     * Clients must republish after the next skeleton-ready event.
     */
    void clearExternalStateForSkeletonRelease();
}
