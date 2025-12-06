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
//     logger::info("FRIK API init {}!", err == 0 ? "successful" : "failed with error: " + std::to_string(err));
//
//     if (!frik::api::FRIKApi::inst->isSkeletonReady())
//         return;
//
//     RE::NiPoint3 tip = frik::api::FRIKApi::inst->getIndexFingerTipPosition(true);
//
//     // Override left hand pose
//     frik::api::FRIKApi::inst->setHandPoseFingerPositions(true, 1.0f, 0.5f, 0.2f, 0.0f, 0.0f);
//
//     // Later:
//     frik::api::FRIKApi::inst->clearHandPoseFingerPositions(true);
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
    inline constexpr std::uint32_t FRIK_API_VERSION = 1;

    struct FRIKApi
    {
        /**
         * Get the API version number.
         * Use this to check compatibility before calling other functions.
         */
        std::uint32_t (FRIK_CALL*getVersion)();

        /**
         * Check if FRIK is ready and the skeleton is initialized.
         */
        bool (FRIK_CALL*isSkeletonReady)();

        /**
         * Get the world position of the index fingertip .
         * @param primaryHand - true for primary (dominant) hand, false for offhand
         */
        RE::NiPoint3 (FRIK_CALL*getIndexFingerTipPosition)(bool primaryHand);

        /**
         * Set a hand pose override to specific values for each finger.
         * Each value is between 0 and 1 where 0 is bent and 1 is straight.
         */
        void (FRIK_CALL*setHandPoseFingerPositions)(bool isLeft, float thumb, float index, float middle, float ring, float pinky);

        /**
         * Clear the set values in "setHandPoseFingerPositions" for FRIK to have control over the hand pose.
         */
        void (FRIK_CALL*clearHandPoseFingerPositions)(bool isLeft);

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
        static int initialize(const uint32_t minVersion = FRIK_API_VERSION)
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
