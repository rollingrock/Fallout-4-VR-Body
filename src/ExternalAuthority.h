#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "TagBlockSet.h"

namespace frik
{
    /**
     * The authority external mods hold over FRIK's hands and primary weapon, published through the API.
     *
     * FRIK drives both hands and the primary weapon by default. A client mod takes that over by
     * registering under a tag, so several mods can claim at once without clobbering each other,
     * exactly like hand-pose overrides. Registrations only record intent - the skeleton frame update
     * resolves them, so each hand is solved exactly once per frame from whichever source owns it.
     *
     * Every registration here is scoped to one skeleton and dropped by clearForSkeletonRelease, after
     * which clients must republish. API state that deliberately outlives the skeleton (feature blocks,
     * offhand grip blocks) does not belong here and stays in ApiCore.
     *
     * Registration arrives from client mods on whatever thread they choose, so every member is
     * internally synchronized - TagBlockSet locks its own state and the hand transforms take
     * _handWorldTransformsLock. That only makes this data safe to touch off-thread, not correct to:
     * a transform published while the skeleton frame update is running still lands on one hand and
     * not the other, or is measured against a scene graph that is mid-update. The API therefore asks
     * clients to publish on the game update thread, and the locking here exists so a client that
     * ignores that cannot corrupt FRIK, not to widen the contract.
     */
    class ExternalAuthority
    {
    public:
        /**
         * Add or remove one tag's claim on the primary weapon node.
         *
         * @param outChanged optional; see TagBlockSet::setBlocked.
         * @return false if the tag is empty, which cannot identify a claimant.
         */
        bool blockPrimaryWeaponNodeOwnership(const std::string_view tag, const bool block, bool* const outChanged = nullptr)
        {
            return _primaryWeaponNodeOwnershipBlocks.setBlocked(tag, block, outChanged);
        }

        /**
         * Return whether any tag currently owns the primary weapon node instead of FRIK.
         */
        bool isPrimaryWeaponNodeOwnershipBlocked() const
        {
            return _primaryWeaponNodeOwnershipBlocks.isBlocked();
        }

        /**
         * Add or remove one tag's block on the primary weapon hand pose.
         *
         * @param outChanged optional; see TagBlockSet::setBlocked.
         * @return false if the tag is empty, which cannot identify a blocker.
         */
        bool blockPrimaryWeaponPose(const std::string_view tag, const bool block, bool* const outChanged = nullptr)
        {
            return _primaryWeaponPoseBlocks.setBlocked(tag, block, outChanged);
        }

        /**
         * Return whether any tag is currently blocking the primary weapon hand pose.
         */
        bool isPrimaryWeaponPoseBlocked() const
        {
            return _primaryWeaponPoseBlocks.isBlocked();
        }

        bool setHandWorldTransform(std::string_view tag, bool isLeft, const RE::NiTransform& worldTransform, int priority);
        bool clearHandWorldTransform(std::string_view tag, bool isLeft);
        bool getHandWorldTransform(bool isLeft, RE::NiTransform& outWorldTransform) const;

        void clearForSkeletonRelease();

    private:
        /**
         * A hand world transform published by an external mod, replacing the tracked controller as the
         * target the arm is solved to. The transform stays in effect until it is cleared, so a client
         * that owns a hand for a while does not have to republish every frame.
         */
        struct HandWorldTransformClaim
        {
            std::string tag;
            RE::NiTransform worldTransform;
            int priority = 0;
            std::uint64_t sequence = 0;
        };

        // Both overloads require _handWorldTransformsLock to be held.
        std::vector<HandWorldTransformClaim>& claimsForHand(const bool isLeft)
        {
            return _handWorldTransforms[isLeft ? 1 : 0];
        }

        const std::vector<HandWorldTransformClaim>& claimsForHand(const bool isLeft) const
        {
            return _handWorldTransforms[isLeft ? 1 : 0];
        }

        TagBlockSet _primaryWeaponNodeOwnershipBlocks;
        TagBlockSet _primaryWeaponPoseBlocks;

        // Guards both hands' claim lists and the sequence counter shared between them.
        mutable std::mutex _handWorldTransformsLock;

        // Indexed by hand: 0 is right, 1 is left.
        std::array<std::vector<HandWorldTransformClaim>, 2> _handWorldTransforms;
        std::uint64_t _nextHandWorldTransformSequence = 0;
    };

    // Global singleton for easy access
    inline ExternalAuthority g_externalAuthority;
}
