#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "FramePhaseRegistry.h"
#include "RE/NetImmerse/NiPoint.h"
#include "RE/NetImmerse/NiTransform.h"
#include "skeleton/HandPose.h"
#include "skeleton/HandPoseData.h"
#include "skeleton/HandPoseMath.h"

namespace RE
{
    class NiNode;
}

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

    // ------------------------------------------------------------------
    // Weapon hand recoil C ABI. Every published major mirrors these
    // byte-for-byte and static_asserts it, so the recoil registry never
    // depends on a version header.
    // ------------------------------------------------------------------

    enum class RecoilDelivery : std::uint8_t
    {
        Damped = 0,
        Direct = 1,
    };

    enum class RecoilHandMask : std::uint8_t
    {
        None = 0,
        Primary = 1u << 0,
        Offhand = 1u << 1,
    };

    struct RecoilSample
    {
        std::uint32_t structSize = 0;
        std::uint32_t reserved0[3] = {};
        RE::NiTransform nativeKickLocal{};
        std::uint32_t reserved[8] = {};
    };

    struct RecoilResponse
    {
        std::uint32_t structSize = 0;
        std::uint32_t handMask = static_cast<std::uint32_t>(RecoilHandMask::Primary);
        RecoilDelivery delivery = RecoilDelivery::Direct;
        std::uint32_t reserved0 = 0;
        RE::NiTransform controlledKickLocal{};
        std::uint32_t reserved[8] = {};
    };

    using WeaponHandRecoilController = bool(FRIK_CORE_CALL*)(const RecoilSample* sample, RecoilResponse* outResponse, void* userData) noexcept;

    static_assert(sizeof(RecoilSample) == 112, "RecoilSample ABI changed");
    static_assert(sizeof(RecoilResponse) == 112, "RecoilResponse ABI changed");

    /**
     * Payload of the kSkeletonReady / kSkeletonDestroying lifecycle messages.
     * generation counts skeleton builds this session, so a client can tell a rebuild from the body it measured.
     */
    struct SkeletonLifecycleData
    {
        std::uint32_t structSize = 0;
        std::uint32_t generation = 0;
        RE::NiNode* rootNode = nullptr;
        bool inPowerArmor = false;
        std::uint8_t reserved0[7] = {};
        std::uint32_t reserved[4] = {};
    };

    static_assert(sizeof(SkeletonLifecycleData) == 40, "SkeletonLifecycleData ABI changed");

    /**
     * Which tracked transform of a hand to read; the values FRIK itself uses this frame.
     */
    enum class TrackedHandKind : std::uint8_t
    {
        Wand = 0,
        WeaponOffset = 1,
        FirstPersonHand = 2,
    };

    /**
     * World transforms of one arm chain. validMask bit i is set when bone i exists (forearm 2/3 do not in power armor).
     */
    struct ArmChainTransforms
    {
        std::uint32_t structSize = 0;
        std::uint32_t validMask = 0;
        RE::NiTransform shoulder{};
        RE::NiTransform upperArm{};
        RE::NiTransform upperArmTwist{};
        RE::NiTransform forearm1{};
        RE::NiTransform forearm2{};
        RE::NiTransform forearm3{};
        RE::NiTransform hand{};
        std::uint32_t reserved[4] = {};
    };

    static_assert(sizeof(ArmChainTransforms) == 480, "ArmChainTransforms ABI changed");

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
    std::uint32_t FRIK_CORE_CALL getSkeletonGeneration();
    bool FRIK_CORE_CALL isInPowerArmor();
    bool FRIK_CORE_CALL setScopeProvider(const char* tag, std::uint32_t capabilities);
    bool FRIK_CORE_CALL clearScopeProvider(const char* tag);
    bool FRIK_CORE_CALL setLookingThroughScope(const char* tag, bool lookingThrough);
    bool FRIK_CORE_CALL isLookingThroughScope();
    bool FRIK_CORE_CALL registerFrameCallback(const char* tag, std::uint32_t phase, FrameCallback callback, void* userData, int priority);
    bool FRIK_CORE_CALL unregisterFrameCallback(const char* tag);
    bool FRIK_CORE_CALL getBoneWorldTransform(const char* boneName, RE::NiTransform* outTransform);
    bool FRIK_CORE_CALL isConfigOpen();
    bool FRIK_CORE_CALL isSelfieModeOn();
    void FRIK_CORE_CALL setSelfieModeOn(bool setOn);
    bool FRIK_CORE_CALL isOffHandGrippingWeapon();
    bool FRIK_CORE_CALL isWristPipboyOpen();
    bool FRIK_CORE_CALL blockOffHandWeaponGripping(const char* tag, bool block);
    bool FRIK_CORE_CALL blockPrimaryHandWeaponPose(const char* tag, bool block);
    bool FRIK_CORE_CALL blockPrimaryWeaponNodeOwnership(const char* tag, bool block);
    int FRIK_CORE_CALL getConfigValue(const char* section, const char* key, char* outBuf, int bufLen, const char* defaultValue);
    bool FRIK_CORE_CALL hasConfigValueOverride(const char* section, const char* key);
    bool FRIK_CORE_CALL setConfigValueOverride(const char* caller, const char* section, const char* key, const char* value);
    bool FRIK_CORE_CALL clearConfigValueOverride(const char* caller, const char* section, const char* key);

    /**
     * Record which client module acquired an exported API table, once per module per table.
     */
    void logApiAcquired(std::string_view apiName, std::uint32_t apiVersion, std::size_t tableSize, const void* returnAddress);

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

    /**
     * Same, for a published version's own hand selector. Every major declares its
     * enumerators with the same values as core's and static_asserts it, so this
     * casts rather than switching and no version needs its own shim.
     */
    template <typename ApiHand>
    bool isLeftForHand(const ApiHand hand)
    {
        return isLeftForHand(static_cast<Hand>(hand));
    }

    /**
     * Convert a published version's hand-pose payload into the internal pose.
     *
     * Templated on the version's struct rather than named outright, because this
     * header must not include a public API header. Every major's HandPoseData has
     * carried the same members since v1, so they all bind here; a major that
     * changes the payload simply stops matching and needs its own conversion.
     */
    template <typename ApiHandPoseData>
    HandFingersPose makeHandPoseFromApiData(const ApiHandPoseData& handPose)
    {
        return HandFingersPose{ FingerPose{ handPose.thumb.prox, handPose.thumb.mid, handPose.thumb.dist, handPose.thumb.splay },
            FingerPose{ handPose.index.prox, handPose.index.mid, handPose.index.dist, handPose.index.splay },
            FingerPose{ handPose.middle.prox, handPose.middle.mid, handPose.middle.dist, handPose.middle.splay },
            FingerPose{ handPose.ring.prox, handPose.ring.mid, handPose.ring.dist, handPose.ring.splay },
            FingerPose{ handPose.pinky.prox, handPose.pinky.mid, handPose.pinky.dist, handPose.pinky.splay },
            handPose.palmPitch,
            handPose.palmYaw,
            skeleton::data::HandPoseKind::Custom };
    }

    /**
     * Run the callbacks registered for a phase; called by FRIK at each point of its frame.
     */
    void invokeFramePhase(FramePhase phase);

    bool getTrackedHandTransform(bool isLeft, TrackedHandKind kind, RE::NiTransform& outTransform);
    bool getArmChain(bool isLeft, ArmChainTransforms& outChain);

    RE::NiPoint3 getIndexFingerTipPosition(Hand hand);

    skeleton::data::HandPoseOverrideTagState getHandPoseSetTagState(std::string_view tag, bool isLeft);
    skeleton::data::HandPoseKind getCurrentHandPoseKind(bool isLeft);

    void setHandPose(std::string_view tag, bool isLeft, const HandFingersPose& pose, int priority);
    void clearHandPose(std::string_view tag, bool isLeft);

    bool setHandWorldTransform(std::string_view tag, bool isLeft, const RE::NiTransform& worldTransform, int priority);
    bool clearHandWorldTransform(std::string_view tag, bool isLeft);

    bool setHandPoseLocalTransforms(std::string_view tag, bool isLeft, const std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& localTransforms,
        std::uint16_t enabledMask, int priority);

    bool getHandPoseLocalTransformsForPose(bool isLeft, const HandFingersPose& pose, std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& outTransforms,
        std::uint16_t& outEnabledMask);

    bool mirrorFingerLocalTransforms(bool sourceIsLeft, const std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& sourceTransforms, std::uint16_t sourceEnabledMask,
        std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask);

    bool blockFeature(std::string_view tag, Feature feature, bool block);
    bool isFeatureBlocked(Feature feature);

    bool registerOpenModSettingButtonToMainConfig(const char* buttonIconNifPath, const char* callbackReceiverName, std::uint32_t callbackMessageType);
}
