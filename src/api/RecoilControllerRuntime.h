#pragma once

#include "FRIKApiV2.h"

namespace frik::api
{
    /**
     * Outcome of offering one frame's native kick to the registered controllers.
     *
     * The response is only meaningful when accepted. Otherwise it carries neutral
     * defaults and the caller should leave the game's own recoil untouched, which is
     * what keeps vanilla behavior when no mod is driving recoil.
     */
    struct RecoilControllerResolution
    {
        bool accepted = false;
        FRIKApiV2::RecoilResponse response{};
    };

    /**
     * The recoil-controller registry is owned and invoked by the game update
     * thread. Registration APIs fail closed while a callback is executing, so
     * callbacks cannot invalidate the bounded registry during iteration.
     */
    bool FRIK_CALL registerWeaponHandRecoilController(const char* tag, FRIKApiV2::WeaponHandRecoilController controller, void* userData, int priority);

    bool FRIK_CALL unregisterWeaponHandRecoilController(const char* tag);

    RecoilControllerResolution resolveWeaponHandRecoil(const FRIKApiV2::RecoilSample& sample) noexcept;

    void clearWeaponHandRecoilControllersForSkeletonRelease() noexcept;
}
