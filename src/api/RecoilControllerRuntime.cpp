#include "RecoilControllerRuntime.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "common/MatrixUtils.h"

namespace
{
    using Api = frik::api::FRIKApi;

    constexpr std::size_t MAX_RECOIL_CONTROLLERS = 16;
    constexpr std::size_t RECOIL_CONTROLLER_TAG_CAPACITY = 64;
    constexpr float MAX_RECOIL_TRANSLATION = 20.0f;
    constexpr float RECOIL_SCALE_TOLERANCE = 0.01f;
    constexpr float ROTATION_UNIT_TOLERANCE = 0.1f;
    constexpr float ROTATION_ORTHOGONAL_TOLERANCE = 0.1f;
    constexpr float ROTATION_DETERMINANT_TOLERANCE = 0.2f;

    struct RecoilControllerEntry
    {
        std::array<char, RECOIL_CONTROLLER_TAG_CAPACITY> tag{};
        std::size_t tagLength = 0;
        Api::WeaponHandRecoilController controller = nullptr;
        void* userData = nullptr;
        int priority = 0;
        std::uint64_t generation = 0;
        bool active = false;
    };

    std::array<RecoilControllerEntry, MAX_RECOIL_CONTROLLERS> g_recoilControllers{};
    std::uint64_t g_recoilControllerGeneration = 0;
    bool g_invokingRecoilController = false;

    RE::NiTransform makeIdentityTransform()
    {
        RE::NiTransform identity;
        identity.MakeIdentity();
        return identity;
    }

    std::optional<std::string_view> normalizeTag(const char* const tag)
    {
        if (!tag) {
            return std::nullopt;
        }

        std::string_view normalized(tag);
        const auto isWhitespace = [](const char value) {
            return value == ' ' || value == '\t' || value == '\r' || value == '\n' || value == '\f' || value == '\v';
        };
        while (!normalized.empty() && isWhitespace(normalized.front())) {
            normalized.remove_prefix(1);
        }
        while (!normalized.empty() && isWhitespace(normalized.back())) {
            normalized.remove_suffix(1);
        }

        if (normalized.empty() || normalized.size() >= RECOIL_CONTROLLER_TAG_CAPACITY) {
            return std::nullopt;
        }
        return normalized;
    }

    std::string_view entryTag(const RecoilControllerEntry& entry)
    {
        return std::string_view(entry.tag.data(), entry.tagLength);
    }

    RecoilControllerEntry* findEntry(const std::string_view tag)
    {
        const auto it = std::ranges::find_if(g_recoilControllers, [&](const RecoilControllerEntry& entry) {
            return entry.active && entryTag(entry) == tag;
        });
        return it == g_recoilControllers.end() ? nullptr : &*it;
    }

    RecoilControllerEntry* findFreeEntry()
    {
        const auto it = std::ranges::find_if(g_recoilControllers, [](const RecoilControllerEntry& entry) {
            return !entry.active;
        });
        return it == g_recoilControllers.end() ? nullptr : &*it;
    }

    std::uint64_t nextGeneration()
    {
        ++g_recoilControllerGeneration;
        if (g_recoilControllerGeneration == 0) {
            ++g_recoilControllerGeneration;
        }
        return g_recoilControllerGeneration;
    }

    float dotRows(const RE::NiMatrix3& rotation, const int lhs, const int rhs)
    {
        return rotation.entry[lhs][0] * rotation.entry[rhs][0] + rotation.entry[lhs][1] * rotation.entry[rhs][1] + rotation.entry[lhs][2] * rotation.entry[rhs][2];
    }

    float rotationDeterminant(const RE::NiMatrix3& rotation)
    {
        return rotation.entry[0][0] * (rotation.entry[1][1] * rotation.entry[2][2] - rotation.entry[1][2] * rotation.entry[2][1]) -
               rotation.entry[0][1] * (rotation.entry[1][0] * rotation.entry[2][2] - rotation.entry[1][2] * rotation.entry[2][0]) +
               rotation.entry[0][2] * (rotation.entry[1][0] * rotation.entry[2][1] - rotation.entry[1][1] * rotation.entry[2][0]);
    }

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

    bool isValidResponse(const Api::RecoilResponse& response)
    {
        constexpr auto supportedHandMask = static_cast<std::uint32_t>(Api::RecoilHandMask::Primary) | static_cast<std::uint32_t>(Api::RecoilHandMask::Offhand);

        return response.structSize >= sizeof(Api::RecoilResponse) && (response.handMask & ~supportedHandMask) == 0 &&
               (response.delivery == Api::RecoilDelivery::Damped || response.delivery == Api::RecoilDelivery::Direct) && isPlausibleRigidTransform(response.controlledKickLocal);
    }

}

namespace frik::api
{
    bool FRIK_CALL registerWeaponHandRecoilController(const char* const tag, const FRIKApi::WeaponHandRecoilController controller, void* const userData, const int priority)
    {
        const auto normalizedTag = normalizeTag(tag);
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
        std::copy(normalizedTag->begin(), normalizedTag->end(), entry->tag.begin());
        entry->tagLength = normalizedTag->size();
        entry->controller = controller;
        entry->userData = userData;
        entry->priority = priority;
        entry->generation = nextGeneration();
        entry->active = true;
        return true;
    }

    bool FRIK_CALL unregisterWeaponHandRecoilController(const char* const tag)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (g_invokingRecoilController || !normalizedTag) {
            return false;
        }

        if (auto* entry = findEntry(*normalizedTag)) {
            *entry = {};
        }
        return true;
    }

    RecoilControllerResolution resolveWeaponHandRecoil(const FRIKApi::RecoilSample& sample) noexcept
    {
        RecoilControllerResolution resolution{};
        resolution.response.structSize = sizeof(FRIKApi::RecoilResponse);
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
            FRIKApi::RecoilResponse response{};
            response.structSize = sizeof(FRIKApi::RecoilResponse);
            response.handMask = static_cast<std::uint32_t>(FRIKApi::RecoilHandMask::Primary);
            response.delivery = FRIKApi::RecoilDelivery::Direct;
            response.controlledKickLocal = makeIdentityTransform();

            if (!ordered[index]->controller(&sample, &response, ordered[index]->userData)) {
                continue;
            }
            if (!isValidResponse(response)) {
                logger::sample("Rejected invalid weapon-hand recoil response from controller '{}'", entryTag(*ordered[index]));
                continue;
            }

            resolution.accepted = true;
            resolution.response = response;
            break;
        }
        g_invokingRecoilController = false;

        return resolution;
    }

    void clearWeaponHandRecoilControllersForSkeletonRelease() noexcept
    {
        for (auto& entry : g_recoilControllers) {
            entry = {};
        }
        g_recoilControllerGeneration = 0;
        g_invokingRecoilController = false;
    }
}
