#pragma once

namespace frik::grip
{
    /**
     * The off-hand-on-barrel cone. Enter is tighter than exit so an engaged grip does not flicker at the edge;
     * the range keeps hands that merely touch (below min) or are nowhere near the barrel (above max) out.
     */
    struct ConeParams
    {
        float enterCosine = 0.955f; // ~17 degrees
        float exitCosine = 0.90f; // ~26 degrees
        float minDistance = 15.0f;
        float maxDistance = 90.0f;
    };

    /**
     * Whether the off-hand counts as on the barrel, given the cosine of its angle to the barrel and its distance from the primary hand.
     */
    constexpr bool isInCone(const float cosine, const float distance, const bool exitCone, const ConeParams& params = {})
    {
        return cosine > (exitCone ? params.exitCosine : params.enterCosine) && distance > params.minDistance && distance < params.maxDistance;
    }

    /**
     * The grip's memory across frames: a grip survives the weapon being hidden and is re-checked when the same weapon
     * returns (#142), only a different weapon drops it; a mode-2 let-go inside the cone waits for the hand to leave it.
     */
    struct GripLatch
    {
        bool gripping = false;
        bool revalidatePending = false;
        bool rearmRequired = false;

        // The drawn weapon went away (holster, Pip-Boy, cell load): keep the grip, re-check it when a weapon is back.
        void onWeaponHidden()
        {
            revalidatePending = gripping;
        }

        // A weapon is drawn. @return true when an active grip was released (a different weapon while gripping).
        bool onWeaponDrawn(const bool sameWeapon)
        {
            if (sameWeapon) {
                return false;
            }
            const bool wasGripping = gripping;
            release();
            return wasGripping;
        }

        // Called while gripping. @return true when the grip is kept, false when the hand left the exit cone and it was released.
        bool revalidate(const bool inExitCone)
        {
            if (!revalidatePending) {
                return true;
            }
            revalidatePending = false;
            if (inExitCone) {
                return true;
            }
            release();
            return false;
        }

        // Let go by button; in mode 2 (auto grip, button release) no auto-grip until the hand leaves the cone.
        void releaseByButton(const bool autoGripMode)
        {
            rearmRequired = autoGripMode;
            release();
        }

        // Whether an auto-grip may engage this frame, clearing the rearm once the hand has left the exit cone.
        bool mayAutoGrip(const bool inEnterCone, const bool inExitCone)
        {
            if (rearmRequired) {
                rearmRequired = inExitCone;
                return false;
            }
            return inEnterCone;
        }

        void grip()
        {
            gripping = true;
            revalidatePending = false;
        }

        void release()
        {
            gripping = false;
            revalidatePending = false;
        }
    };
}
