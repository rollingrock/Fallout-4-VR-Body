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
    /**
     * Report or drop an external mod's two-handed grip on the current weapon. Re-setting a tag updates it in place.
     *
     * @return false if the tag is empty or the support transform is not finite.
     */
    bool ExternalAuthority::setOffHandGrip(const std::string_view tag, const bool active, const bool supportIsLeft, const RE::NiTransform* supportWorld)
    {
        if (tag.empty() || (supportWorld && !isFiniteTransform(*supportWorld))) {
            return false;
        }

        std::lock_guard lock(_offHandGripsLock);
        const auto it = std::ranges::find_if(_offHandGrips, [tag](const OffHandGripClaim& claim) {
            return claim.tag == tag;
        });
        if (!active) {
            if (it != _offHandGrips.end()) {
                _offHandGrips.erase(it);
            }
            return true;
        }
        OffHandGripClaim claim{ .tag = std::string(tag), .supportIsLeft = supportIsLeft, .hasSupportWorld = supportWorld != nullptr };
        if (supportWorld) {
            claim.supportWorld = *supportWorld;
        }
        if (it == _offHandGrips.end()) {
            _offHandGrips.push_back(std::move(claim));
        } else {
            *it = std::move(claim);
        }
        return true;
    }

    bool ExternalAuthority::isOffHandGripping() const
    {
        std::lock_guard lock(_offHandGripsLock);
        return !_offHandGrips.empty();
    }

    /**
     * A grip is tied to the weapon it was reported on, so a drawn weapon change drops every external grip.
     */
    void ExternalAuthority::clearOffHandGripsForWeaponChange()
    {
        std::lock_guard lock(_offHandGripsLock);
        if (!_offHandGrips.empty()) {
            logger::info("Weapon change: dropped {} external off-hand grip(s)", _offHandGrips.size());
            _offHandGrips.clear();
        }
    }

    void ExternalAuthority::clearForSkeletonRelease()
    {
        {
            std::lock_guard lock(_offHandGripsLock);
            _offHandGrips.clear();
        }
        // From a client's side its registrations simply evaporate here, so say what was dropped and
        // tie the two ends of that contract together in the log. Silent when nothing was registered,
        // which is every release in a game running without API clients.
        const auto droppedBlocks = _primaryWeaponNodeOwnershipBlocks.blockingCount() + _primaryWeaponPoseBlocks.blockingCount();
        _primaryWeaponNodeOwnershipBlocks.clear();
        _primaryWeaponPoseBlocks.clear();

        std::lock_guard lock(_handWorldTransformsLock);
        std::size_t droppedClaims = 0;
        for (auto& claims : _handWorldTransforms) {
            droppedClaims += claims.size();
            claims.clear();
        }
        _nextHandWorldTransformSequence = 0;

        if (droppedBlocks > 0 || droppedClaims > 0) {
            logger::info("Skeleton release: dropped {} external weapon block(s) and {} hand transform claim(s), clients must republish", droppedBlocks, droppedClaims);
        }
    }
}
