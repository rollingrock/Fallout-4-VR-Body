#pragma once

#include <cstdint>

namespace frik::api
{
    // Export/import macro for the *functions*.
    // We also give them C linkage so GetProcAddress("FRIKAPI_xxx") works.
#if defined(FRIK_API_EXPORTS)
#   define FRIK_API extern "C" __declspec(dllexport)
#else
#   define FRIK_API extern "C" __declspec(dllimport)
#endif

#pragma warning(push)
#pragma warning(disable : 4190)

    // API version for compatibility checking
    inline constexpr std::uint32_t FRIK_API_VERSION = 1;

    /**
     * Get the API version number.
     * Use this to check compatibility before calling other functions.
     */
    FRIK_API std::uint32_t FRIKAPI_getVersion();

    /**
     * Check if FRIK is ready and the skeleton is initialized.
     */
    FRIK_API bool FRIKAPI_isSkeletonReady();

    /**
     * Get the world position of the index fingertip .
     * @param primaryHand - true for primary (dominant) hand, false for offhand
     */
    FRIK_API RE::NiPoint3 FRIKAPI_getIndexFingerTipPosition(bool primaryHand);

    /**
     * Set a hand pose override to specific values for each finger.
     * Each value is between 0 and 1 where 0 is bent and 1 is straight.
     */
    FRIK_API void FRIKAPI_setHandPoseFingerPositions(bool isLeft, float thumb, float index, float middle, float ring, float pinky);

    /**
     * Clear the set values in "setHandPoseFingerPositions" for FRIK to have control over the hand pose.
     */
    FRIK_API void FRIKAPI_clearHandPoseFingerPositions(bool isLeft);

#pragma warning(pop)
}

// ----------------------------------------------------------------------------------------
// EXAMPLE USAGE:
//
// #include <Windows.h>
//
// namespace {
//     using frik::api::FRIK_API_VERSION;
//
//     using FRIKAPI_getVersion_t                = std::uint32_t(__cdecl*)();
//     using FRIKAPI_isSkeletonReady_t           = bool(__cdecl*)();
//     using FRIKAPI_getIndexFingerTipPosition_t = RE::NiPoint3(__cdecl*)(bool);
//     using FRIKAPI_setHandPoseFingerPositions_t =
//         void(__cdecl*)(bool, float, float, float, float, float);
//     using FRIKAPI_clearHandPoseFingerPositions_t = void(__cdecl*)(bool);
//
//     struct FrikApi {
//         FRIKAPI_getVersion_t                getVersion{};
//         FRIKAPI_isSkeletonReady_t           isSkeletonReady{};
//         FRIKAPI_getIndexFingerTipPosition_t getIndexFingerTipPosition{};
//         FRIKAPI_setHandPoseFingerPositions_t setHandPoseFingerPositions{};
//         FRIKAPI_clearHandPoseFingerPositions_t clearHandPoseFingerPositions{};
//         bool loaded = false;
//     } g_frik;
// }
//
// bool InitFrikApi()
// {
//     if (g_frik.loaded)
//         return true;
//
//     HMODULE frikDll = GetModuleHandleW(L"FRIK.dll");
//     if (!frikDll)
//         return false;
//
//     auto load = [frikDll](auto& fn, const char* name) {
//         fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(
//             GetProcAddress(frikDll, name));
//         return fn != nullptr;
//     };
//
//     if (!load(g_frik.getVersion, "FRIKAPI_getVersion"))
//         return false;
//
//     if (g_frik.getVersion() < FRIK_API_VERSION)
//         return false;
//
//     if (!load(g_frik.isSkeletonReady, "FRIKAPI_isSkeletonReady"))               return false;
//     if (!load(g_frik.getIndexFingerTipPosition, "FRIKAPI_getIndexFingerTipPosition")) return false;
//     if (!load(g_frik.setHandPoseFingerPositions, "FRIKAPI_setHandPoseFingerPositions")) return false;
//     if (!load(g_frik.clearHandPoseFingerPositions, "FRIKAPI_clearHandPoseFingerPositions")) return false;
//
//     g_frik.loaded = true;
//     return true;
// }
//
// void ExampleUse()
// {
//     if (!InitFrikApi())
//         return;
//
//     if (!g_frik.isSkeletonReady())
//         return;
//
//     RE::NiPoint3 tip = g_frik.getIndexFingerTipPosition(true);
//
//     // Override left hand pose
//     g_frik.setHandPoseFingerPositions(true,1.0f,0.5f,0.2f,0.0f,0.0f);
//
//     // Later:
//     g_frik.clearHandPoseFingerPositions(true);
// }
