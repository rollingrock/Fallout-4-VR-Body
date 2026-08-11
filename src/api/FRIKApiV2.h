#pragma once

#include <Windows.h>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "RE/NetImmerse/NiPoint.h"
#include "RE/NetImmerse/NiTransform.h"

// ----------------------------------------------------------------------------------------
// EXAMPLE USAGE:
// Copy this whole file into your project AS IS
// Use the code below as a reference of FRIK API v2 use
// Call initialize in GameLoaded event

// {
//     const int err = frik::api::FRIKApiV2::initialize();
//     if (err != 0) {
//         logger::error("FRIK API v2 init failed with error: {}!", err);
//     }
//     logger::info("FRIK (v{}) API v2 (v{}) init successful!", frik::api::FRIKApiV2::inst->getModVersion(), frik::api::FRIKApiV2::inst->getVersion());
//
//     // later...
//     if (!frik::api::FRIKApiV2::inst->isSkeletonReady())
//         return;
//
//     RE::NiPoint3 tip = frik::api::FRIKApiV2::inst->getIndexFingerTipPosition(frik::api::FRIKApiV2::Hand::Left);
//
//     // Override the primary hand, tagged so it never clobbers another system
//     frik::api::FRIKApiV2::inst->setHandPose("MyMod_Interaction",
//         frik::api::FRIKApiV2::Hand::Primary,
//         frik::api::FRIKApiV2::HandPoseKind::Pointing,
//         frik::api::FRIKApiV2::HAND_POSE_PRIORITY_DEFAULT);
//
//     // Later:
//     frik::api::FRIKApiV2::inst->clearHandPose("MyMod_Interaction", frik::api::FRIKApiV2::Hand::Primary);
// }

namespace frik::api
{
#ifndef FRIK_API
#if defined(FRIK_API_EXPORTS)
#define FRIK_API extern "C" __declspec(dllexport)
#else
#define FRIK_API extern "C" __declspec(dllimport)
#endif
#endif

#ifndef FRIK_CALL
#define FRIK_CALL __cdecl
#endif

    /**
     * Version of the FRIK API v2 contract, independent of the v1-v4 table.
     * A v2 client never reads the older table and vice versa.
     */
    inline constexpr std::uint32_t FRIK_API_V2_VERSION = 1;

    struct FRIKApiV2
    {
        /**
         * The name of FRIK mod as registered in F4SE used to be able to send/receive messages from to FRIK.
         * Example:
         * _messaging->RegisterListener(onFRIKMessage, frik::api::FRIKApiV2::FRIK_F4SE_MOD_NAME);
         */
        static constexpr auto FRIK_F4SE_MOD_NAME = "F4VRBody";

        /**
         * Priority for a hand-pose override with no stronger claim.
         * This is the level every unprioritized FRIK API v1-v4 client sits at.
         */
        static constexpr int HAND_POSE_PRIORITY_DEFAULT = 50;

        /**
         * Priority FRIK's own interaction poses use - Pip-Boy pointing, forced
         * pointing, offhand grip, and Attaboy. Match it to tie with FRIK
         * (newest registration wins); exceed it to reliably outrank FRIK itself.
         *
         * Outranking FRIK means FRIK cannot reclaim the hand for its own
         * interactions, so prefer blockFeature for wholesale takeover.
         */
        static constexpr int HAND_POSE_PRIORITY_FRIK_INTERNAL = 90;

        /**
         * The player hand to act on with support of left-handed if needed.
         */
        enum class Hand : std::uint8_t
        {
            Primary,
            Offhand,
            Right,
            Left,
        };

        /**
         * Readable name of a hand enum for logging (scoped enums are not directly formattable).
         */
        static std::string_view handName(const Hand hand)
        {
            switch (hand) {
            case Hand::Primary:
                return "Primary";
            case Hand::Offhand:
                return "Offhand";
            case Hand::Right:
                return "Right";
            case Hand::Left:
                return "Left";
            }
            return "?";
        }

        /**
         * Predefined hand pose kinds exposed by the FRIK API.
         * Matches FRIK runtime hand pose kinds.
         */
        enum class HandPoseKind : std::uint8_t
        {
            // no specific pose is set
            Unset = 0,
            // pose set with custom finger positions
            Custom = 1,
            Open = 2,
            Pointing = 3,
            HoldingWeapon = 4,
            OffhandGrip = 5,
            Attaboy = 6,
            ThumbsUp = 7,
            Fist = 8,
            HoldingGun = 9,
            HoldingMelee = 10,
        };

        /**
         * Full pose values for a single finger.
         */
        struct FingerPoseData
        {
            float prox = 0.0f;
            float mid = 0.0f;
            float dist = 0.0f;
            float splay = 0.0f;
        };

        /**
         * Full pose data for one hand, including per-joint finger values and palm motion.
         *
         * Canonical float layout (22 floats, used by fromFloats / toFloats / asFloatView):
         *   [0..3]   thumb  { prox, mid, dist, splay }
         *   [4..7]   index  { prox, mid, dist, splay }
         *   [8..11]  middle { prox, mid, dist, splay }
         *   [12..15] ring   { prox, mid, dist, splay }
         *   [16..19] pinky  { prox, mid, dist, splay }
         *   [20]     palmPitch
         *   [21]     palmYaw
         */
        struct HandPoseData
        {
            static constexpr std::size_t FLOAT_COUNT = 22;

            FingerPoseData thumb;
            FingerPoseData index;
            FingerPoseData middle;
            FingerPoseData ring;
            FingerPoseData pinky;
            float palmPitch = 0.0f;
            float palmYaw = 0.0f;

            /**
             * Build a HandPoseData from the canonical 22-float packed layout.
             */
            static HandPoseData fromFloats(std::span<const float, FLOAT_COUNT> v)
            {
                return HandPoseData{
                    .thumb = { .prox = v[0], .mid = v[1], .dist = v[2], .splay = v[3] },
                    .index = { .prox = v[4], .mid = v[5], .dist = v[6], .splay = v[7] },
                    .middle = { .prox = v[8], .mid = v[9], .dist = v[10], .splay = v[11] },
                    .ring = { .prox = v[12], .mid = v[13], .dist = v[14], .splay = v[15] },
                    .pinky = { .prox = v[16], .mid = v[17], .dist = v[18], .splay = v[19] },
                    .palmPitch = v[20],
                    .palmYaw = v[21],
                };
            }

            /**
             * Copy this pose into the canonical 22-float packed layout.
             */
            std::array<float, FLOAT_COUNT> toFloats() const
            {
                return { thumb.prox,
                    thumb.mid,
                    thumb.dist,
                    thumb.splay,
                    index.prox,
                    index.mid,
                    index.dist,
                    index.splay,
                    middle.prox,
                    middle.mid,
                    middle.dist,
                    middle.splay,
                    ring.prox,
                    ring.mid,
                    ring.dist,
                    ring.splay,
                    pinky.prox,
                    pinky.mid,
                    pinky.dist,
                    pinky.splay,
                    palmPitch,
                    palmYaw };
            }

            /**
             * Zero-copy view of this pose as the canonical 22-float packed layout.
             * The struct's standard-layout / no-padding contract is enforced by static_assert below,
             * so this aliases directly onto the member fields with no copy.
             */
            std::span<const float, FLOAT_COUNT> asFloatView() const
            {
                return std::span<const float, FLOAT_COUNT>(reinterpret_cast<const float*>(this), FLOAT_COUNT);
            }

            std::span<float, FLOAT_COUNT> asFloatView()
            {
                return std::span<float, FLOAT_COUNT>(reinterpret_cast<float*>(this), FLOAT_COUNT);
            }
        };

        static_assert(std::is_standard_layout_v<HandPoseData>, "HandPoseData must be standard-layout for asFloatView() to alias safely");
        static_assert(sizeof(HandPoseData) == HandPoseData::FLOAT_COUNT * sizeof(float), "HandPoseData must be tightly packed (22 contiguous floats, no padding)");

        /**
         * The potential state of a set hand pose for specific tag as returned from FRIK.
         */
        enum class HandPoseTagState : std::uint8_t
        {
            // the tag is not set at all
            None,
            // the tag is set and actively used to override the hand pose
            Active,
            // the tag is set but currently overridden by another tag
            Overriden,
        };

        /**
         * FRIK subsystems that can be turned off by other mods via blockFeature.
         * Use when your mod replaces or conflicts with one of these parts of FRIK.
         */
        enum class Feature : std::uint8_t
        {
            // Embedded flashlight: head/hand switching and light positioning.
            Flashlight,
            // Weapon repositioning: per-weapon offsets, offhand two-handed grip, reposition mode.
            WeaponPositioning,
            // Wrist Pipboy: show/hide on arm, physical finger interaction, open/close (flashlight is unaffected).
            Pipboy,
            // Smooth movement (anti motion-sickness locomotion smoothing).
            SmoothMovement,
        };

        /**
         * Data needed to register a button to open external mod config from FRIK main config UI.
         */
        struct OpenExternalModConfigData
        {
            const char* buttonIconNifPath;
            const char* callbackReceiverName;
            std::uint32_t callbackMessageType;
        };

        /**
         * Explicit per-bone local transforms for the 15 finger bones of one hand.
         * Only bones whose bit is set in enabledMask are read.
         */
        struct FingerLocalTransformOverride
        {
            std::uint16_t enabledMask = 0;
            std::uint16_t reserved[3] = {};
            RE::NiTransform localTransforms[15] = {};
        };

        /**
         * How a controlled recoil response should be applied.
         */
        enum class RecoilDelivery : std::uint8_t
        {
            Damped = 0,
            Direct = 1,
        };

        /**
         * Which hands a recoil response applies to.
         */
        enum class RecoilHandMask : std::uint8_t
        {
            None = 0,
            Primary = 1u << 0,
            Offhand = 1u << 1,
        };

        /**
         * Immutable native recoil sampled immediately before FRIK neutralizes
         * the game kickback node and solves both arms for the current frame.
         *
         * nativeKickLocal is expressed in the native PrimaryWeaponKickbackRecoil
         * node's local frame. General game state is intentionally not mirrored
         * through this contract; controllers obtain it directly from FO4VR.
         */
        struct RecoilSample
        {
            std::uint32_t structSize = 0;
            std::uint32_t reserved0[3] = {};
            RE::NiTransform nativeKickLocal{};
            std::uint32_t reserved[8] = {};
        };

        /**
         * Controlled visual hand/arm recoil returned by an external controller.
         *
         * FRIK pre-fills this struct with neutral defaults before every callback -
         * identity kick, Primary hand, Direct delivery - so the usual shape of a
         * controller is to edit the fields it cares about through outResponse and
         * leave the rest alone. structSize is FRIK's own bookkeeping, reported for
         * your information only; it is ignored on the way back in, so you need
         * neither set nor preserve it.
         *
         * If you would rather build a response locally and assign it wholesale,
         * fill controlledKickLocal yourself even when you only mean to suppress
         * recoil: a default-constructed RE::NiTransform carries a zero rotation
         * matrix rather than identity, and is refused as non-rigid.
         *
         * Returning true from the callback consumes FRIK's native hand-recoil
         * contribution for that frame. controlledKickLocal uses the same local
         * frame as RecoilSample::nativeKickLocal. A zero handMask intentionally
         * suppresses hand recoil. This does not alter gameplay recoil, spread,
         * camera shake, or the engine's visual kickback node.
         */
        struct RecoilResponse
        {
            std::uint32_t structSize = 0;
            std::uint32_t handMask = static_cast<std::uint32_t>(RecoilHandMask::Primary);
            RecoilDelivery delivery = RecoilDelivery::Direct;
            std::uint32_t reserved0 = 0;
            RE::NiTransform controlledKickLocal{};
            std::uint32_t reserved[8] = {};
        };

        /**
         * Called synchronously on FRIK's game update thread at most once per
         * skeleton frame, after the native kick node and its parent frame have
         * been validated. The callback must be noexcept, bounded, nonblocking,
         * must not mutate scene nodes, and must not re-enter recoil registration
         * APIs.
         *
         * Return true to consume native hand recoil and use outResponse.
         * Return false to decline ownership for the frame; FRIK tries the next
         * registered controller and finally falls back to its regular recoil.
         */
        using WeaponHandRecoilController = bool(FRIK_CALL*)(const RecoilSample* sample, RecoilResponse* outResponse, void* userData) noexcept;

        static_assert(sizeof(RecoilSample) == 112, "RecoilSample ABI changed");
        static_assert(sizeof(RecoilResponse) == 112, "RecoilResponse ABI changed");

        /**
         * F4SE message types FRIK broadcasts over FRIK_F4SE_MOD_NAME.
         */
        enum class LifecycleEvent : std::uint8_t
        {
            kSkeletonReady = 100,
            kSkeletonDestroying = 101,
        };

        /**
         * Get the API v2 version number.
         * Use this to check compatibility before calling other functions.
         */
        std::uint32_t(FRIK_CALL* getVersion)();

        /**
         * Get the mod version string. i.e. "0.78.0"
         */
        const char*(FRIK_CALL* getModVersion)();

        /**
         * Check if FRIK is ready and the skeleton is initialized.
         */
        bool(FRIK_CALL* isSkeletonReady)();

        /**
         * Is any of the FRIK config UI open (main, Pipboy, weapon adjustment).
         */
        bool(FRIK_CALL* isConfigOpen)();

        /**
         * Is FRIK selfie mode currently on or off.
         */
        bool(FRIK_CALL* isSelfieModeOn)();

        /**
         * Set FRIK selfie mode on or off.
         */
        void(FRIK_CALL* setSelfieModeOn)(bool setOn);

        /**
         * Is the player currently holding the weapon with two hands. i.e. offhand is holding the weapon.
         */
        bool(FRIK_CALL* isOffHandGrippingWeapon)();

        /**
         * Is the player currently have the FRIK Pipboy open.
         */
        bool(FRIK_CALL* isWristPipboyOpen)();

        /**
         * Get the world position of the index fingertip.
         */
        RE::NiPoint3(FRIK_CALL* getIndexFingerTipPosition)(Hand hand);

        /**
         * Get the current state of given hand pose tag to identify if it is active in FRIK.
         * Can be used to identify if another system is overriding the hand pose and your mod should react accordingly.
         */
        HandPoseTagState(FRIK_CALL* getHandPoseSetTagState)(const char* tag, Hand hand);

        /**
         * Get the current hand pose as active in FRIK.
         */
        HandPoseKind(FRIK_CALL* getCurrentHandPose)(Hand hand);

        /**
         * Set a predefined hand pose override at an explicit priority.
         *
         * Overrides are ordered by priority (highest wins); equal priorities are
         * broken by the most recently registered tag. Re-setting a tag you already
         * hold updates its pose in place and keeps its original position in that
         * order, so refreshing every frame never walks you past an equal-priority
         * peer - clear the tag and set it again to claim the tie.
         * Use HAND_POSE_PRIORITY_DEFAULT unless you have a reason not to, and see
         * HAND_POSE_PRIORITY_FRIK_INTERNAL for where FRIK's own poses sit.
         *
         * Use the tag to uniquely identify different systems using hand pose overrides.
         * Passing HandPoseKind::Unset clears this tag's override.
         * Use setHandPoseCustom for HandPoseKind::Custom.
         *
         * Overrides are cleared on skeleton destruction and must be republished
         * after LifecycleEvent::kSkeletonReady.
         * @param priority must be >= 0.
         * @return true if successful.
         */
        bool(FRIK_CALL* setHandPose)(const char* tag, Hand hand, HandPoseKind handPose, int priority);

        /**
         * Set a full hand pose override with per-joint finger values, per-finger splay,
         * and palm motion, at an explicit priority.
         * For a uniform per-finger flex, set prox/mid/dist of each finger to the same value.
         * Use clearHandPose to release the override.
         *
         * Overrides are cleared on skeleton destruction and must be republished
         * after LifecycleEvent::kSkeletonReady.
         * @return true if successful.
         */
        bool(FRIK_CALL* setHandPoseCustom)(const char* tag, Hand hand, const HandPoseData& handPose, int priority);

        /**
         * Replace the finger bone local transforms of an existing tagged override.
         *
         * The tag must already hold an override (set one of the setHandPose*
         * functions first); this call fails if it does not. Any later setHandPose*
         * call on the same tag clears these transforms, so republish them after
         * every pose update.
         * @return true if successful.
         */
        bool(FRIK_CALL* setHandPoseCustomLocalTransforms)(const char* tag, Hand hand, const FingerLocalTransformOverride* overrideData, int priority);

        /**
         * Resolve the finger bone local transforms FRIK would use for a given pose,
         * without applying it. Accounts for power armor.
         * @return true if every finger bone resolved.
         */
        bool(FRIK_CALL* getHandPoseLocalTransformsForPose)(Hand hand, const HandPoseData& handPose, FingerLocalTransformOverride* outTransforms);

        /**
         * Convert a complete physical-hand finger pose into the opposite hand's
         * anatomical pose. sourceHand must be Hand::Left or Hand::Right.
         * @return true if every finger bone mirrored.
         */
        bool(FRIK_CALL* mirrorFingerLocalTransforms)(Hand sourceHand, const FingerLocalTransformOverride* sourceTransforms, FingerLocalTransformOverride* outTargetTransforms);

        /**
         * Clear the hand pose override for this tag so FRIK regains control.
         * Only clears the specific tag hand pose override.
         * @return true if successful.
         */
        bool(FRIK_CALL* clearHandPose)(const char* tag, Hand hand);

        /**
         * Take over where a hand is placed for a specific tag, giving FRIK the world
         * transform to solve the arm to instead of the tracked controller. This is
         * independent of the hand pose functions above, which own the fingers: a tag
         * can set either, both, or neither.
         * The highest-priority tag owns the hand; equal priorities are broken by
         * the most recently published.
         *
         * The transform is consumed by FRIK's arm solve on its next skeleton frame
         * rather than applied during this call, so the arm is solved exactly once per
         * frame and everything FRIK derives from the hand stays consistent with it.
         * A published transform keeps owning the hand until it is cleared, so a client
         * holding a hand steady does not have to republish every frame; a client
         * tracking a moving target republishes whenever the target changes.
         * Call on the game update thread.
         *
         * Registrations are cleared on skeleton destruction and must be republished
         * after LifecycleEvent::kSkeletonReady.
         * @param worldTransform the wrist transform in world space, not hand-local space.
         * @param priority must be >= 0.
         * @return true if the transform was accepted. This reports validation only -
         * whether the arm can reach the target is decided per frame by the solver,
         * which falls back to FRIK's own posing for a frame it cannot solve.
         */
        bool(FRIK_CALL* setHandWorldTransform)(const char* tag, Hand hand, const RE::NiTransform& worldTransform, int priority);

        /**
         * Clear the hand transform for this tag, handing the hand back on the next
         * frame to FRIK (or to the next highest-priority tag).
         * @return true if successful.
         */
        bool(FRIK_CALL* clearHandWorldTransform)(const char* tag, Hand hand);

        /**
         * Adds a button to open external mod config via a button in FRIK main config UI.
         */
        bool(FRIK_CALL* registerOpenModSettingButtonToMainConfig)(const OpenExternalModConfigData& data);

        /**
         * Enable/disable FRIK offhand weapon gripping for a specific tag.
         * The tag must be unique per external system using this API.
         * FRIK keeps only the blocked state, so gripping remains disabled while any tag is blocking it.
         * @return true if successful.
         */
        bool(FRIK_CALL* blockOffHandWeaponGripping)(const char* tag, bool block);

        /**
         * Enable/disable a FRIK subsystem/feature for a specific tag.
         * The tag must be unique per external system using this API.
         * FRIK keeps only the blocked state, so the feature stays disabled while any tag is still blocking it.
         * Use when your mod replaces a FRIK feature (e.g. provides its own flashlight) and wants FRIK's turned off.
         * @param block true to disable the feature, false to release this tag's block.
         * @return true if successful.
         */
        bool(FRIK_CALL* blockFeature)(const char* tag, Feature feature, bool block);

        /**
         * Check whether a FRIK subsystem/feature is currently disabled (blocked by any tag).
         * @return true if the feature is currently blocked/disabled.
         */
        bool(FRIK_CALL* isFeatureBlocked)(Feature feature);

        /**
         * Block FRIK's built-in primary weapon hand pose for a specific tag.
         * While blocked, FRIK yields every built-in primary weapon hand-pose
         * contributor, including its per-weapon primary-hand grip rotation.
         * @return true if successful.
         */
        bool(FRIK_CALL* blockPrimaryHandWeaponPose)(const char* tag, bool block);

        /**
         * Block FRIK's ownership of the primary weapon scene node for a specific tag,
         * so an external system can drive the weapon transform itself.
         * @return true if successful.
         */
        bool(FRIK_CALL* blockPrimaryWeaponNodeOwnership)(const char* tag, bool block);

        /**
         * Read the current effective value for a FRIK config section/key into the caller-provided buffer.
         * The value returned is the active session override if one is set (see setConfigValueOverride),
         * otherwise the on-disk FRIK.ini value, otherwise defaultValue. The value is returned as a raw
         * string; the caller parses it to the type it expects (e.g. std::strtof / std::atoi).
         * Always null-terminates outBuf when bufLen > 0; writes at most bufLen-1 characters plus the null.
         * Note: when the key is absent from the .ini the caller's defaultValue is returned, which may
         * differ from FRIK's own built-in default for that key.
         * @param defaultValue value returned when the key is missing; may be null (treated as empty).
         * @return the full value length excluding the null terminator; if >= bufLen the value was truncated.
         */
        int(FRIK_CALL* getConfigValue)(const char* section, const char* key, char* outBuf, int bufLen, const char* defaultValue);

        /**
         * Check whether a session override is currently set for a FRIK config section/key.
         */
        bool(FRIK_CALL* hasConfigValueOverride)(const char* section, const char* key);

        /**
         * Set an in-memory override for a FRIK config section/key for the rest of this game session.
         * The override is re-applied on every config (re)load, so it survives FRIK.ini live-reload, and
         * is never written to disk (cleared on game restart). The value is given as a string and parsed
         * by the type-appropriate reader when the config is loaded, so it works for any config value
         * (bool/int/float/string and the compound transform/binding/hand-pose values). FRIK reloads its
         * config immediately so the change takes effect right away.
         * @param caller name of the calling mod, used only for FRIK logging.
         * @return true if section, key, and value are all non-null.
         */
        bool(FRIK_CALL* setConfigValueOverride)(const char* caller, const char* section, const char* key, const char* value);

        /**
         * Remove a previously set session override for section/key; FRIK reloads so the value reverts to
         * its on-disk FRIK.ini value.
         * @param caller name of the calling mod, used only for FRIK logging.
         * @return true if an override was set for that key and has been removed.
         */
        bool(FRIK_CALL* clearConfigValueOverride)(const char* caller, const char* section, const char* key);

        /**
         * Register or replace a tagged visual hand-recoil controller.
         *
         * Controllers are considered in descending priority and newest-first
         * order. A controller that returns false declines only the current frame,
         * allowing the next controller to respond. Registrations are cleared on
         * skeleton destruction and must be republished after kSkeletonReady.
         * Register and unregister on the game update thread; callbacks execute
         * synchronously on that same thread.
         * @return true if successful.
         */
        bool(FRIK_CALL* registerWeaponHandRecoilController)(const char* tag, WeaponHandRecoilController controller, void* userData, int priority);

        /**
         * Remove a tagged recoil controller. Removing a missing tag is idempotent.
         * @return true if successful.
         */
        bool(FRIK_CALL* unregisterWeaponHandRecoilController)(const char* tag);

        /**
         * Initialize the FRIK API v2 object.
         * NOTE: call after all mods have been loaded in the game (GameLoaded event).
         *
         * @param minVersion the minimal version required (default: the compiled against version)
         * @return error codes:
         * 0 - Successful
         * 1 - Failed to find FRIK.dll (trying to init too early?)
         * 2 - No FRIKAPI_V2_GetApi API found
         * 3 - Failed FRIKAPI_V2_GetApi call
         * 4 - FRIK API v2 version is older than the minimal required version
         * 5 - Loaded API v2 contract does not exactly match this header
         */
        [[nodiscard]] static int initialize(const uint32_t minVersion = FRIK_API_V2_VERSION)
        {
            if (inst) {
                return 0;
            }

            // get FRIK.dll
            const auto frikDll = GetModuleHandleA("FRIK.dll");
            if (!frikDll) {
                return 1;
            }

            const auto getApi = reinterpret_cast<const FRIKApiV2*(FRIK_CALL*)()>(GetProcAddress(frikDll, "FRIKAPI_V2_GetApi"));
            if (!getApi) {
                return 2;
            }

            const auto frikApi = getApi();
            if (!frikApi) {
                return 3;
            }

            // check against expected version
            if (frikApi->getVersion() < minVersion) {
                return 4;
            }

            const auto getApiStructSize = reinterpret_cast<std::uint32_t(FRIK_CALL*)()>(GetProcAddress(frikDll, "FRIKAPI_V2_GetApiStructSize"));
            if (!getApiStructSize || getApiStructSize() != sizeof(FRIKApiV2)) {
                return 5;
            }

            inst = frikApi;
            return 0;
        }

        /**
         * The initialized instance of FRIK API v2 interface.
         * Use after successful call to initialize.
         */
        inline static const FRIKApiV2* inst = nullptr;
    };

    inline constexpr std::size_t FRIK_API_V2_FUNCTION_POINTER_SIZE = sizeof(decltype(FRIKApiV2::getVersion));
    static_assert(std::is_standard_layout_v<FRIKApiV2>, "FRIKApiV2 must remain standard-layout for its exported function table ABI");
    static_assert(sizeof(FRIKApiV2) == 31 * FRIK_API_V2_FUNCTION_POINTER_SIZE, "FRIK API v2 function table layout changed");
}
