#include "ExternalAuthority.h"

#include <algorithm>

#include "utils.h"

namespace frik
{
    /**
     * Publish a tagged world transform for one hand.
     *
     * This only records the transform; the skeleton frame update consumes it, so the arm is always
     * solved exactly once per frame from whichever source owns the hand. The transform stays in
     * effect until it is cleared, so a client that owns a hand for a while does not have to republish.
     * The highest priority tag owns the hand and equal priorities are broken by the most recently
     * published. Call on the game update thread.
     *
     * @return false if the tag is empty, the priority is negative, or the transform is not finite.
     */
    bool ExternalAuthority::setHandWorldTransform(const std::string_view tag, const bool isLeft, const RE::NiTransform& worldTransform, const int priority)
    {
        if (tag.empty() || priority < 0 || !isFiniteTransform(worldTransform)) {
            return false;
        }

        std::lock_guard lock(_handWorldTransformsLock);
        auto& claims = claimsForHand(isLeft);
        const auto claimIt = std::ranges::find_if(claims, [tag](const HandWorldTransformClaim& claim) {
            return claim.tag == tag;
        });
        if (claimIt == claims.end()) {
            claims.push_back(HandWorldTransformClaim{
                .tag = std::string(tag),
                .worldTransform = worldTransform,
                .priority = priority,
                .sequence = ++_nextHandWorldTransformSequence,
            });
        } else {
            claimIt->worldTransform = worldTransform;
            claimIt->priority = priority;
            claimIt->sequence = ++_nextHandWorldTransformSequence;
        }
        return true;
    }

    /**
     * Release a tagged hand transform, handing the hand back on the next frame to the next
     * highest-priority tag, or to FRIK's own tracked arm solve when no tag is left.
     *
     * @return false if the tag is empty.
     */
    bool ExternalAuthority::clearHandWorldTransform(const std::string_view tag, const bool isLeft)
    {
        if (tag.empty()) {
            return false;
        }

        std::lock_guard lock(_handWorldTransformsLock);
        std::erase_if(claimsForHand(isLeft), [tag](const HandWorldTransformClaim& claim) {
            return claim.tag == tag;
        });
        return true;
    }

    /**
     * Copy out the transform owning one hand this frame.
     *
     * Copies under the lock rather than handing back a pointer into the claim list, because such a
     * pointer would outlive the lock and dangle if a client published or cleared a claim before the
     * caller read through it - a lock inside this function alone would not have prevented that.
     *
     * @return false if no tag owns the hand, leaving outWorldTransform untouched, so the caller's
     * own tracked hand drives the arm.
     */
    bool ExternalAuthority::getHandWorldTransform(const bool isLeft, RE::NiTransform& outWorldTransform) const
    {
        std::lock_guard lock(_handWorldTransformsLock);

        const HandWorldTransformClaim* owner = nullptr;
        for (const auto& claim : claimsForHand(isLeft)) {
            if (!owner || claim.priority > owner->priority || (claim.priority == owner->priority && claim.sequence > owner->sequence)) {
                owner = &claim;
            }
        }
        if (!owner) {
            return false;
        }

        outWorldTransform = owner->worldTransform;
        return true;
    }

    /**
     * Drop every external registration when the skeleton is released, because the nodes they were
     * published against are gone. Clients must republish after the next skeleton-ready event.
     */
    void ExternalAuthority::clearForSkeletonRelease()
    {
        _primaryWeaponNodeOwnershipBlocks.clear();
        _primaryWeaponPoseBlocks.clear();

        std::lock_guard lock(_handWorldTransformsLock);
        for (auto& claims : _handWorldTransforms) {
            claims.clear();
        }
        _nextHandWorldTransformSequence = 0;
    }
}
