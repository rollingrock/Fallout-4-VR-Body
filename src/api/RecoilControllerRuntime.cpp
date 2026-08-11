#include "RecoilControllerRuntime.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ApiCore.h"
#include "common/MatrixUtils.h"

namespace
{
    using Api = frik::api::FRIKApiV2;

    namespace core = frik::api::core;

    /**
     * The registry is a fixed array of slots, so the per-frame walk is a bounded scan
     * over contiguous memory however many mods have registered, and registration simply
     * fails once the array is full.
     */
    constexpr std::size_t MAX_RECOIL_CONTROLLERS = 16;

    /**
     * Bounds a response must respect to be believed (see isPlausibleRigidTransform).
     * These are deliberately loose - they exist to reject garbage or uninitialized data
     * crossing the C ABI, not to hold a controller to any particular precision.
     */
    constexpr float MAX_RECOIL_TRANSLATION = 20.0f;
    constexpr float RECOIL_SCALE_TOLERANCE = 0.01f;
    constexpr float ROTATION_UNIT_TOLERANCE = 0.1f;
    constexpr float ROTATION_ORTHOGONAL_TOLERANCE = 0.1f;
    constexpr float ROTATION_DETERMINANT_TOLERANCE = 0.2f;

    /**
     * One registry slot. Inactive slots are free, and generation records when the slot
     * was claimed so equal priorities can be ordered (see nextGeneration).
     */
    struct RecoilControllerEntry
    {
        std::string tag;
        Api::WeaponHandRecoilController controller = nullptr;
        void* userData = nullptr;
        int priority = 0;
        std::uint64_t generation = 0;
        bool active = false;
    };

    std::array<RecoilControllerEntry, MAX_RECOIL_CONTROLLERS> g_recoilControllers{};
    std::uint64_t g_recoilControllerGeneration = 0;

    /**
     * Set while a controller callback is on the stack. Registration and unregistration
     * fail closed against it, so a callback cannot add or drop a slot while resolve is
     * iterating the array it is holding pointers into. Not atomic because the whole
     * registry only ever runs on the game update thread.
     */
    bool g_invokingRecoilController = false;

    /**
     * NiTransform has no identity constructor, so build one explicitly.
     */
    RE::NiTransform makeIdentityTransform()
    {
        RE::NiTransform identity;
        identity.MakeIdentity();
        return identity;
    }

    /**
     * Find the live slot holding a tag, or null. Tags are unique, so re-registering an
     * existing tag replaces that slot rather than consuming a second one.
     */
    RecoilControllerEntry* findEntry(const std::string_view tag)
    {
        const auto it = std::ranges::find_if(g_recoilControllers, [&](const RecoilControllerEntry& entry) {
            return entry.active && entry.tag == tag;
        });
        return it == g_recoilControllers.end() ? nullptr : &*it;
    }

    /**
     * Claim the first inactive slot, or null once the registry is full.
     */
    RecoilControllerEntry* findFreeEntry()
    {
        const auto it = std::ranges::find_if(g_recoilControllers, [](const RecoilControllerEntry& entry) {
            return !entry.active;
        });
        return it == g_recoilControllers.end() ? nullptr : &*it;
    }

    /**
     * Stamp a registration with a monotonically increasing counter, used to order
     * controllers that registered at equal priority. Zero is skipped so it stays
     * reserved for a cleared slot and can never collide with a live one.
     */
    std::uint64_t nextGeneration()
    {
        ++g_recoilControllerGeneration;
        if (g_recoilControllerGeneration == 0) {
            ++g_recoilControllerGeneration;
        }
        return g_recoilControllerGeneration;
    }

    /**
     * Dot product of two rows of a rotation matrix, over the 3x3 rotation only.
     * A row against itself gives its squared length, two different rows their
     * alignment - together enough to tell whether the rows are orthonormal.
     */
    float dotRows(const RE::NiMatrix3& rotation, const int lhs, const int rhs)
    {
        return rotation.entry[lhs][0] * rotation.entry[rhs][0] + rotation.entry[lhs][1] * rotation.entry[rhs][1] + rotation.entry[lhs][2] * rotation.entry[rhs][2];
    }

    /**
     * Determinant of the 3x3 rotation. Orthonormal rows alone do not rule out a
     * reflection, which a determinant of +1 rather than -1 does.
     */
    float rotationDeterminant(const RE::NiMatrix3& rotation)
    {
        return rotation.entry[0][0] * (rotation.entry[1][1] * rotation.entry[2][2] - rotation.entry[1][2] * rotation.entry[2][1]) -
               rotation.entry[0][1] * (rotation.entry[1][0] * rotation.entry[2][2] - rotation.entry[1][2] * rotation.entry[2][0]) +
               rotation.entry[0][2] * (rotation.entry[1][0] * rotation.entry[2][1] - rotation.entry[1][1] * rotation.entry[2][0]);
    }

    /**
     * Decide whether a transform handed over the C ABI is safe to drive the hands with.
     *
     * This is the trust boundary: the value came from another mod's DLL and is about to
     * reach the scene graph, so it must be a plain rigid motion of a believable size. The
     * rotation is required to be orthonormal and non-reflecting, the scale to be near 1,
     * and the offset to stay within MAX_RECOIL_TRANSLATION game units of the hand.
     */
    bool isPlausibleRigidTransform(const RE::NiTransform& transform)
    {
        if (!std::isfinite(transform.translate.x) || !std::isfinite(transform.translate.y) || !std::isfinite(transform.translate.z) || !std::isfinite(transform.scale) ||
            std::abs(transform.scale - 1.0f) > RECOIL_SCALE_TOLERANCE || common::MatrixUtils::vec3Len(transform.translate) > MAX_RECOIL_TRANSLATION) {
            return false;
        }

        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                if (!std::isfinite(transform.rotate.entry[row][column])) {
                    return false;
                }
            }
            if (std::abs(dotRows(transform.rotate, row, row) - 1.0f) > ROTATION_UNIT_TOLERANCE) {
                return false;
            }
        }

        if (std::abs(dotRows(transform.rotate, 0, 1)) > ROTATION_ORTHOGONAL_TOLERANCE || std::abs(dotRows(transform.rotate, 0, 2)) > ROTATION_ORTHOGONAL_TOLERANCE ||
            std::abs(dotRows(transform.rotate, 1, 2)) > ROTATION_ORTHOGONAL_TOLERANCE) {
            return false;
        }

        return std::abs(rotationDeterminant(transform.rotate) - 1.0f) <= ROTATION_DETERMINANT_TOLERANCE;
    }

    /**
     * Vet a filled-in response before it is allowed to win the frame.
     *
     * structSize must cover the struct FRIK knows. FRIK owns and pre-fills the buffer, so
     * this is the version gate: a client built against an older, shorter header is caught
     * here rather than having FRIK read fields it never knew to write. Unknown hand mask
     * bits and unknown delivery modes are refused rather than guessed at, so a response
     * written for a future API cannot be silently misread as one of today's.
     */
    bool isValidResponse(const Api::RecoilResponse& response)
    {
        constexpr auto supportedHandMask = static_cast<std::uint32_t>(Api::RecoilHandMask::Primary) | static_cast<std::uint32_t>(Api::RecoilHandMask::Offhand);

        return response.structSize >= sizeof(Api::RecoilResponse) && (response.handMask & ~supportedHandMask) == 0 &&
               (response.delivery == Api::RecoilDelivery::Damped || response.delivery == Api::RecoilDelivery::Direct) && isPlausibleRigidTransform(response.controlledKickLocal);
    }

}

namespace frik::api
{
    /**
     * Register a controller that wants to drive weapon recoil, keyed by tag.
     *
     * Re-registering a tag replaces that controller in place and re-stamps its generation,
     * so a mod can update its callback or priority without first unregistering, and cannot
     * leak slots by registering repeatedly. Higher priority wins the frame; see
     * resolveWeaponHandRecoil for how ties are broken.
     *
     * @return false for a bad tag, a null controller, a negative priority, a full registry,
     * or any call made from inside a controller callback.
     */
    bool FRIK_CALL registerWeaponHandRecoilController(const char* const tag, const FRIKApiV2::WeaponHandRecoilController controller, void* const userData, const int priority)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (g_invokingRecoilController || !normalizedTag || !controller || priority < 0) {
            return false;
        }

        auto* entry = findEntry(*normalizedTag);
        if (!entry) {
            entry = findFreeEntry();
        }
        if (!entry) {
            return false;
        }

        *entry = {};
        entry->tag = *normalizedTag;
        entry->controller = controller;
        entry->userData = userData;
        entry->priority = priority;
        entry->generation = nextGeneration();
        entry->active = true;
        return true;
    }

    /**
     * Release the slot a tag holds, giving recoil back to the next controller down.
     *
     * Idempotent: an unknown tag still succeeds, since the caller's intent - that the tag
     * holds nothing once this returns - is satisfied either way.
     *
     * @return false only for a bad tag or a call made from inside a controller callback.
     */
    bool FRIK_CALL unregisterWeaponHandRecoilController(const char* const tag)
    {
        const auto normalizedTag = core::normalizeTag(tag);
        if (g_invokingRecoilController || !normalizedTag) {
            return false;
        }

        if (auto* entry = findEntry(*normalizedTag)) {
            *entry = {};
        }
        return true;
    }

    /**
     * Offer this frame's native kick to the registered controllers and return the first
     * usable answer.
     *
     * Controllers are tried by descending priority, and within one priority the most
     * recently registered goes first, so a mod loading later can take over from an equal
     * peer. The first controller that both claims the frame and returns a response passing
     * isValidResponse wins outright - the rest are not consulted, because recoil is a
     * single rigid motion and blending two of them has no meaningful result.
     *
     * Each callback gets a response pre-filled with neutral defaults, so a controller that
     * sets only the fields it cares about still yields a coherent answer. Declining or
     * failing validation is not an error: it just passes the frame to the next controller,
     * with a rejection logged at a rate limit since a broken controller would otherwise
     * log every frame.
     */
    RecoilControllerResolution resolveWeaponHandRecoil(const FRIKApiV2::RecoilSample& sample) noexcept
    {
        RecoilControllerResolution resolution{};
        resolution.response.structSize = sizeof(FRIKApiV2::RecoilResponse);
        resolution.response.controlledKickLocal = makeIdentityTransform();

        std::array<RecoilControllerEntry*, MAX_RECOIL_CONTROLLERS> ordered{};
        std::size_t count = 0;
        for (auto& entry : g_recoilControllers) {
            if (entry.active) {
                ordered[count++] = &entry;
            }
        }
        std::sort(ordered.begin(), ordered.begin() + count, [](const RecoilControllerEntry* const lhs, const RecoilControllerEntry* const rhs) {
            return lhs->priority > rhs->priority || (lhs->priority == rhs->priority && lhs->generation > rhs->generation);
        });

        g_invokingRecoilController = true;
        for (std::size_t index = 0; index < count; ++index) {
            FRIKApiV2::RecoilResponse response{};
            response.structSize = sizeof(FRIKApiV2::RecoilResponse);
            response.handMask = static_cast<std::uint32_t>(FRIKApiV2::RecoilHandMask::Primary);
            response.delivery = FRIKApiV2::RecoilDelivery::Direct;
            response.controlledKickLocal = makeIdentityTransform();

            if (!ordered[index]->controller(&sample, &response, ordered[index]->userData)) {
                continue;
            }
            if (!isValidResponse(response)) {
                logger::sample("Rejected invalid weapon-hand recoil response from controller '{}'", ordered[index]->tag);
                continue;
            }

            resolution.accepted = true;
            resolution.response = response;
            break;
        }
        g_invokingRecoilController = false;

        return resolution;
    }

    /**
     * Drop every registration when the skeleton is released.
     *
     * The nodes a controller was driving are gone, so its registration is meaningless until
     * it re-registers against the next skeleton-ready event - the same contract as the hand
     * pose and weapon node blocks. Also clears the reentrancy latch, so a release that
     * happened underneath a callback cannot leave the registry permanently closed.
     */
    void clearWeaponHandRecoilControllersForSkeletonRelease() noexcept
    {
        for (auto& entry : g_recoilControllers) {
            entry = {};
        }
        g_recoilControllerGeneration = 0;
        g_invokingRecoilController = false;
    }
}
