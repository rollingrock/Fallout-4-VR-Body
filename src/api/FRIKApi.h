#pragma once

#include <cstdint>
#include <Windows.h>

#include "RE/NetImmerse/NiPoint.h"

// ----------------------------------------------------------------------------------------
// EXAMPLE USAGE:
// Copy this whole file into your project AS IS
// Use the code below as a reference of FRIK API use
// Call initialize in GameLoaded event

// {
//     const int err = frik::api::FRIKApi::initialize();
//     if (err != 0) {
//         logger::error("FRIK API init failed with error: {}!", err);
//     }
//     logger::info("FRIK (v{}) API (v{}) init successful!", frik::api::FRIKApi::inst->getModVersion(), frik::api::FRIKApi::inst->getVersion());
//
//     // later...
//     if (!frik::api::FRIKApi::inst->isSkeletonReady())
//         return;
//
//     RE::NiPoint3 tip = frik::api::FRIKApi::inst->getIndexFingerTipPosition(frik::api::FRIKApi::Hand::Left);
//
//     // Override left hand pose
//     frik::api::FRIKApi::inst->setHandPoseFingerPositions(frik::api::FRIKApi::Hand::Primary, 1.0f, 0.5f, 0.2f, 0.0f, 0.0f);
//
//     // Later:
//     frik::api::FRIKApi::inst->clearHandPoseFingerPositions(frik::api::FRIKApi::Hand::Primary);
// }

namespace frik::api
{
#if defined(FRIK_API_EXPORTS)
#   define FRIK_API extern "C" __declspec(dllexport)
#else
#   define FRIK_API extern "C" __declspec(dllimport)
#endif

#define FRIK_CALL __cdecl

    // API version for compatibility checking
    inline constexpr std::uint32_t FRIK_API_VERSION = 2;

    struct FRIKApi
    {
        /**
         * The name of FRIK mod as registered in F4SE used to be able to send/receive messages from to FRIK.
         * Example:
         * _messaging->RegisterListener(onFRIKMessage, frik::api::FRIKApi::FRIK_F4SE_MOD_NAME);
         */
        static constexpr auto FRIK_F4SE_MOD_NAME = "F4VRBody";

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
         * Data needed to register a button to open external mod config from FRIK main config UI.
         */
        struct OpenExternalModConfigData
        {
            std::string buttonIconNifPath;
            std::string callbackReceiverName;
            std::uint32_t callbackMessageType;
        };

        /**
         * Get the API version number.
         * Use this to check compatibility before calling other functions.
         */
        std::uint32_t (FRIK_CALL*getVersion)();

        /**
         * Get the mod version string. i.e. "0.12.5"
         */
        std::string_view (FRIK_CALL*getModVersion)();

        /**
         * Check if FRIK is ready and the skeleton is initialized.
         */
        bool (FRIK_CALL*isSkeletonReady)();

        /**
         * Is any of the FRIK config UI is open (main, Pipboy, weapon adjustment)
         */
        bool (FRIK_CALL*isConfigOpen)();

        /**
         * Is FRIK selfie mode is currently on or off.
         */
        bool (FRIK_CALL*isSelfieModeOn)();

        /**
         * Set FRIK selfie mode on or off.
         */
        void (FRIK_CALL*setSelfieModeOn)(bool setOn);

        /**
         * Is the player currently holding the weapon with two hands. i.e. offhand is holding the weapon.
         */
        bool (FRIK_CALL*isOffHandGrippingWeapon)();

        /**
         * Is the player currently have the FRIK Pipboy open.
         */
        bool (FRIK_CALL*isWristPipboyOpen)();

        /**
         * Get the world position of the index fingertip .
         */
        RE::NiPoint3 (FRIK_CALL*getIndexFingerTipPosition)(Hand hand);

        /**
         * Get the tag currently used tag to set the hand pose for the given hand.
         * Can be used to identify if another system overriding the hand pose and your mod should react accordingly.
         */
        std::string (FRIK_CALL*getHandPoseSetTag)(Hand hand);

        /**
         * Set a hand pose override to specific values for each finger.
         * Use the tag to unique identify different systems using hand pose overrides.
         * Each value is between 0 and 1 where 0 is bent and 1 is straight.
         */
        void (FRIK_CALL*setHandPoseFingerPositionsV2)(Hand hand, std::string tag, float thumb, float index, float middle, float ring, float pinky);

        /**
         * Clear the set values in "setHandPoseFingerPositions" for FRIK to have control over the hand pose.
         * Only clears the specific tag hand pose override.
         */
        void (FRIK_CALL*clearHandPoseFingerPositionsV2)(Hand hand, std::string tag);

        /**
         * @deprecated Use setHandPoseFingerPositions2 instead.
         */
        void (FRIK_CALL*setHandPoseFingerPositions)(Hand hand, float thumb, float index, float middle, float ring, float pinky);

        /**
         * @deprecated Use clearHandPoseFingerPositions2 instead.
         */
        void (FRIK_CALL*clearHandPoseFingerPositions)(Hand hand);

        /**
         * Adds a button to open external mod config via a button in FRIK main config UI.
         */
        void (FRIK_CALL*registerOpenModSettingButtonToMainConfig)(const OpenExternalModConfigData& data);

        /**
         * Initialize the FRIK API object.
         * NOTE: call after all mods have been loaded in the game (GameLoaded event).
         *
         * @param minVersion the minimal version required (default: the compiled against version)
         * @return error codes:
         * 0 - Successful
         * 1 - Failed to find FRIK.dll (trying to init too early?)
         * 2 - No FRIKAPI_GetApi API found
         * 3 - Failed FRIKAPI_GetApi call
         * 4 - FRIK API version is older than the minimal required version
         */
        [[nodiscard]] static int initialize(const uint32_t minVersion = FRIK_API_VERSION)
        {
            if (inst) {
                return 0;
            }

            // get FRIK.dll
            const auto frikDll = GetModuleHandleA("FRIK.dll");
            if (!frikDll) {
                return 1;
            }

            const auto getApi = reinterpret_cast<const FRIKApi* (FRIK_CALL*)()>(GetProcAddress(frikDll, "FRIKAPI_GetApi"));
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

            inst = frikApi;
            return 0;
        }

        /**
         * The initialized instance of FRIK API interface.
         * Use after successful call to initialize.
         */
        inline static const FRIKApi* inst = nullptr;
    };
}
