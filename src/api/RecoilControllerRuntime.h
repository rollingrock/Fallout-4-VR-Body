#pragma once

#include "FRIKApi.h"

namespace frik::api
{
    struct RecoilControllerResolution
    {
        bool accepted = false;
        FRIKApi::RecoilResponse response{};
    };

    /**
     * The recoil-controller registry is owned and invoked by the game update
     * thread. Registration APIs fail closed while a callback is executing, so
     * callbacks cannot invalidate the bounded registry during iteration.
     */
    bool FRIK_CALL registerWeaponHandRecoilController(
        const char* tag,
        FRIKApi::WeaponHandRecoilController controller,
        void* userData,
        int priority);

    bool FRIK_CALL unregisterWeaponHandRecoilController(const char* tag);

    RecoilControllerResolution resolveWeaponHandRecoil(const FRIKApi::RecoilSample& sample) noexcept;

    void clearWeaponHandRecoilControllersForSkeletonRelease() noexcept;
}
