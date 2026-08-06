#include "ApiCore.h"

#include "Config.h"
#include "FRIK.h"
#include "RecoilControllerRuntime.h"
#include "common/CommonUtils.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "skeleton/Skeleton.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

using namespace frik::skeleton::data;

namespace
{
    using namespace frik;
    using namespace frik::api::core;

    /**
     * External tags blocking offhand gripping, so client mods cannot clobber each other.
     * The tag values are not meaningful to FRIK, only whether at least one tag is blocking.
     */
    std::unordered_set<std::string> g_offHandGripBlockingTags;

    /**
     * Per-feature sets of external tags blocking each FRIK subsystem (see blockFeature).
     * A feature stays disabled while at least one tag is still blocking it.
     */
    std::array<std::unordered_set<std::string>, FEATURE_COUNT> g_featureBlockingTags;

    struct ExternalHandAuthorityEntry
    {
        std::string tag;
        RE::NiTransform worldTarget;
        int priority = 0;
        std::uint64_t generation = 0;
    };

    struct SelectedExternalHandAuthority
    {
        const ExternalHandAuthorityEntry* entry = nullptr;
    };

    std::array<std::vector<ExternalHandAuthorityEntry>, 2> g_externalHandAuthorities;
    std::uint64_t g_externalHandAuthorityGeneration = 0;

    std::size_t handAuthorityIndex(const bool isLeft)
    {
        return isLeft ? 1U : 0U;
    }

    SelectedExternalHandAuthority selectExternalHandAuthority(const std::vector<ExternalHandAuthorityEntry>& entries)
    {
        const ExternalHandAuthorityEntry* best = nullptr;
        for (const auto& entry : entries) {
            if (!best || entry.priority > best->priority || (entry.priority == best->priority && entry.generation > best->generation)) {
                best = &entry;
            }
        }

        if (!best) {
            return {};
        }
        return SelectedExternalHandAuthority{ .entry = best };
    }

    bool isSameExternalHandAuthoritySelection(const SelectedExternalHandAuthority& lhs, const SelectedExternalHandAuthority& rhs)
    {
        if (!lhs.entry || !rhs.entry) {
            return lhs.entry == rhs.entry;
        }

        return lhs.entry->tag == rhs.entry->tag && lhs.entry->priority == rhs.entry->priority && lhs.entry->generation == rhs.entry->generation;
    }

    /**
     * Apply the resolved enabled state of a feature to its FRIK subsystem.
     */
    void applyFeatureEnabled(const Feature feature, const bool enabled)
    {
        switch (feature) {
        case Feature::Flashlight:
            g_frik.setFlashlightEnabled(enabled);
            break;
        case Feature::WeaponPositioning:
            g_frik.setWeaponPositionEnabled(enabled);
            break;
        case Feature::Pipboy:
            g_frik.setPipboyEnabled(enabled);
            break;
        case Feature::SmoothMovement:
            g_frik.setSmoothMovementEnabled(enabled);
            break;
        }
    }
}

namespace frik::api::core
{
    const char* FRIK_CORE_CALL getModVersion()
    {
        // Safe to return pointer to static data
        static_assert(Version::NAME.back() != '\0' || true, "Version must be backed by a string literal");
        return Version::NAME.data();
    }

    bool FRIK_CORE_CALL isSkeletonReady()
    {
        return g_frik.isSkeletonReady();
    }

    bool FRIK_CORE_CALL isConfigOpen()
    {
        return g_frik.isMainConfigurationModeActive() || g_frik.isPipboyConfigurationModeActive() || g_frik.inWeaponRepositionMode();
    }

    bool FRIK_CORE_CALL isSelfieModeOn()
    {
        return g_frik.isSelfieModeOn();
    }

    void FRIK_CORE_CALL setSelfieModeOn(const bool setOn)
    {
        g_frik.setSelfieMode(setOn);
    }

    bool FRIK_CORE_CALL isOffHandGrippingWeapon()
    {
        return g_frik.isOffHandGrippingWeapon();
    }

    bool FRIK_CORE_CALL isWristPipboyOpen()
    {
        return g_frik.isPipboyOn();
    }

    /**
     * Enable/disable FRIK offhand weapon gripping for a specific external tag.
     * Offhand gripping remains disabled while at least one tag is still blocking it.
     */
    bool FRIK_CORE_CALL blockOffHandWeaponGripping(const char* tag, const bool block)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        if (block) {
            g_offHandGripBlockingTags.emplace(*normalizedTag);
        } else {
            g_offHandGripBlockingTags.erase(*normalizedTag);
        }

        logger::sample("API blockOffHandWeaponGripping tag:'{}' block:{} activeBlocks:{}", *normalizedTag, block, g_offHandGripBlockingTags.size());
        g_frik.setOffHandGrippingEnabled(g_offHandGripBlockingTags.empty());
        return true;
    }

    bool FRIK_CORE_CALL blockPrimaryHandWeaponPose(const char* tag, const bool block)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        logger::sample("API blockPrimaryHandWeaponPose tag:'{}' block:{}", *normalizedTag, block);
        return HandPose::blockPrimaryWeaponPose(*normalizedTag, block);
    }

    bool FRIK_CORE_CALL blockPrimaryWeaponNodeOwnership(const char* tag, const bool block)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        logger::sample("API blockPrimaryWeaponNodeOwnership tag:'{}' block:{}", *normalizedTag, block);
        return Skeleton::blockPrimaryWeaponNodeOwnership(*normalizedTag, block);
    }

    /**
     * Read the current effective config value (override, else on-disk, else default) into outBuf.
     */
    int FRIK_CORE_CALL getConfigValue(const char* /*caller*/, const char* section, const char* key, char* outBuf, const int bufLen, const char* defaultValue)
    {
        if (!section || !key) {
            if (outBuf && bufLen > 0) {
                outBuf[0] = '\0';
            }
            return 0;
        }

        const std::string value = g_config.getConfigValue(section, key, defaultValue);
        if (outBuf && bufLen > 0) {
            const auto copied = value.copy(outBuf, static_cast<std::size_t>(bufLen) - 1);
            outBuf[copied] = '\0';
        }
        return static_cast<int>(value.size());
    }

    /**
     * Check whether a session override is currently set for a config section/key.
     */
    bool FRIK_CORE_CALL hasConfigValueOverride(const char* /*caller*/, const char* section, const char* key)
    {
        return section && key && g_config.hasConfigOverride(section, key);
    }

    /**
     * Set a session-only override for a config section/key (string parsed by the type-appropriate reader).
     */
    bool FRIK_CORE_CALL setConfigValueOverride(const char* caller, const char* section, const char* key, const char* value)
    {
        if (!section || !key || !value) {
            return false;
        }
        logger::sample("API setConfigValueOverride caller:'{}' {}.{} = '{}'", caller ? caller : "?", section, key, value);
        g_config.setConfigOverride(section, key, value);
        return true;
    }

    /**
     * Remove a previously set session override for a config section/key.
     */
    bool FRIK_CORE_CALL clearConfigValueOverride(const char* caller, const char* section, const char* key)
    {
        if (!section || !key || !g_config.hasConfigOverride(section, key)) {
            return false;
        }
        logger::sample("API clearConfigValueOverride caller:'{}' {}.{}", caller ? caller : "?", section, key);
        g_config.clearConfigOverride(section, key);
        return true;
    }

    std::optional<std::string> normalizeTag(const char* tag)
    {
        if (!f4cf::common::hasNonWhitespaceText(tag)) {
            return std::nullopt;
        }

        return f4cf::common::trim(tag);
    }

    bool isLeftForHand(const Hand hand)
    {
        switch (hand) {
        case Hand::Primary:
            return f4vr::isLeftHandedMode();
        case Hand::Offhand:
            return !f4vr::isLeftHandedMode();
        case Hand::Right:
            return false;
        case Hand::Left:
            return true;
        }
        return false;
    }

    RE::NiPoint3 getIndexFingerTipPosition(const Hand hand)
    {
        return f4vr::Skelly::getIndexFingerTipWorldPosition(static_cast<vrcf::Hand>(hand));
    }

    HandPoseOverrideTagState getHandPoseSetTagState(const std::string_view tag, const bool isLeft)
    {
        return HandPose::getHandPoseSetTagState(isLeft, tag);
    }

    HandPoseKind getCurrentHandPoseKind(const bool isLeft)
    {
        return HandPose::getCurrentHandPoseKind(isLeft);
    }

    std::optional<HandFingersPose> makePredefinedHandPose(const HandPoseKind kind)
    {
        switch (kind) {
        case HandPoseKind::Open:
            return getOpenPose();
        case HandPoseKind::Pointing:
            return getPointingPose();
        case HandPoseKind::HoldingWeapon:
            return HandPose::getFixedPrimaryWeaponPose();
        case HandPoseKind::OffhandGrip:
            return getOffhandWeaponGripPose();
        case HandPoseKind::Attaboy:
            return getAttaboyPose();
        case HandPoseKind::ThumbsUp:
            return getThumbsUpPose();
        case HandPoseKind::Fist:
            return getFistPose();
        case HandPoseKind::HoldingGun:
            return getGunGripPose();
        case HandPoseKind::HoldingMelee:
            return getMeleeGripPose();
        default:
            return std::nullopt;
        }
    }

    HandFingersPose makeUniformFingerPose(const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        return HandFingersPose{ FingerPose{ thumb, thumb, thumb },
            FingerPose{ index, index, index },
            FingerPose{ middle, middle, middle },
            FingerPose{ ring, ring, ring },
            FingerPose{ pinky, pinky, pinky } };
    }

    void setHandPose(const std::string_view tag, const bool isLeft, const HandFingersPose& pose, const int priority)
    {
        HandPose::setHandPoseOverrideWithPriority(isLeft, tag, pose, priority);
    }

    void clearHandPose(const std::string_view tag, const bool isLeft)
    {
        HandPose::clearHandPoseOverride(isLeft, tag);
    }

    bool setHandPoseLocalTransforms(const std::string_view tag, const bool isLeft, const std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& localTransforms,
        const std::uint16_t enabledMask, const int priority)
    {
        return HandPose::setHandPoseLocalTransformsWithPriority(isLeft, tag, localTransforms, enabledMask, priority);
    }

    bool getHandPoseLocalTransformsForPose(const bool isLeft, const HandFingersPose& pose, std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& outTransforms,
        std::uint16_t& outEnabledMask)
    {
        return HandPose::buildFingerLocalTransformsForPose(isLeft, pose, outTransforms, outEnabledMask);
    }

    bool mirrorFingerLocalTransforms(const bool sourceIsLeft, const std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& sourceTransforms,
        const std::uint16_t sourceEnabledMask, std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask)
    {
        const auto* skeleton = g_frik.getSkeleton();
        if (!skeleton) {
            return false;
        }

        return skeleton->mirrorFingerLocalTransforms(sourceIsLeft, sourceTransforms, sourceEnabledMask, outTargetTransforms, outTargetEnabledMask);
    }

    /**
     * Enable/disable a FRIK subsystem for a specific external tag.
     * The feature remains disabled while at least one tag is still blocking it.
     */
    bool blockFeature(const std::string_view tag, const Feature feature, const bool block)
    {
        const auto featureIndex = static_cast<std::size_t>(feature);
        if (tag.empty() || featureIndex >= g_featureBlockingTags.size()) {
            return false;
        }

        auto& blockingTags = g_featureBlockingTags[featureIndex];
        if (block) {
            blockingTags.emplace(tag);
        } else {
            blockingTags.erase(std::string(tag));
        }

        logger::info("API blockFeature tag:'{}' - feature:{}, block:{}, activeBlocks:{}", tag, featureIndex, block, blockingTags.size());
        applyFeatureEnabled(feature, blockingTags.empty());
        return true;
    }

    /**
     * Check whether a FRIK subsystem is currently disabled (blocked by any tag).
     */
    bool isFeatureBlocked(const Feature feature)
    {
        switch (feature) {
        case Feature::Flashlight:
            return !g_frik.isFlashlightEnabled();
        case Feature::WeaponPositioning:
            return !g_frik.isWeaponPositionEnabled();
        case Feature::Pipboy:
            return !g_frik.isPipboyEnabled();
        case Feature::SmoothMovement:
            return !g_frik.isSmoothMovementEnabled();
        }
        return false;
    }

    bool registerOpenModSettingButtonToMainConfig(const char* buttonIconNifPath, const char* callbackReceiverName, const std::uint32_t callbackMessageType)
    {
        if (!buttonIconNifPath || !callbackReceiverName) {
            return false;
        }

        g_frik.registerOpenSettingButton({ .buttonIconNifPath = buttonIconNifPath, .callbackReceiverName = callbackReceiverName, .callbackMessageType = callbackMessageType });
        return true;
    }

    bool applyExternalHandWorldTransform(const std::string_view tag, const bool isLeft, const RE::NiTransform& worldTarget, const int priority)
    {
        auto* skelly = g_frik.getSkeleton();
        if (tag.empty() || priority < 0 || !skelly) {
            return false;
        }

        auto& entries = g_externalHandAuthorities[handAuthorityIndex(isLeft)];
        auto updatedEntries = entries;
        const auto nextGeneration = g_externalHandAuthorityGeneration + 1;
        const auto it = std::ranges::find_if(updatedEntries, [tag](const ExternalHandAuthorityEntry& entry) {
            return entry.tag == tag;
        });
        if (it == updatedEntries.end()) {
            updatedEntries.push_back(ExternalHandAuthorityEntry{
                .tag = std::string(tag),
                .worldTarget = worldTarget,
                .priority = priority,
                .generation = nextGeneration,
            });
        } else {
            it->worldTarget = worldTarget;
            it->priority = priority;
            it->generation = nextGeneration;
        }

        const auto oldSelected = selectExternalHandAuthority(entries);
        const auto newSelected = selectExternalHandAuthority(updatedEntries);
        if (!newSelected.entry) {
            return false;
        }

        const bool selectionChanged = !isSameExternalHandAuthoritySelection(oldSelected, newSelected);
        if (!selectionChanged) {
            entries = std::move(updatedEntries);
            g_externalHandAuthorityGeneration = nextGeneration;
            return true;
        }

        if (!skelly->applyExternalHandWorldTransform(isLeft, newSelected.entry->worldTarget)) {
            return false;
        }

        entries = std::move(updatedEntries);
        g_externalHandAuthorityGeneration = nextGeneration;
        g_frik.refreshAfterExternalHandAuthority(isLeft);
        return true;
    }

    bool clearExternalHandWorldTransform(const std::string_view tag, const bool isLeft)
    {
        if (tag.empty()) {
            return false;
        }

        auto& entries = g_externalHandAuthorities[handAuthorityIndex(isLeft)];
        auto updatedEntries = entries;
        const auto oldSize = updatedEntries.size();
        updatedEntries.erase(std::remove_if(updatedEntries.begin(),
                                 updatedEntries.end(),
                                 [tag](const ExternalHandAuthorityEntry& entry) {
                                     return entry.tag == tag;
                                 }),
            updatedEntries.end());
        if (updatedEntries.size() == oldSize) {
            return true;
        }

        auto* skelly = g_frik.getSkeleton();
        if (!skelly) {
            return false;
        }

        const auto oldSelected = selectExternalHandAuthority(entries);
        const auto newSelected = selectExternalHandAuthority(updatedEntries);
        if (!newSelected.entry) {
            if (!skelly->preserveHandPoseForTrackedAuthorityHandoff(isLeft)) {
                return false;
            }
            entries = std::move(updatedEntries);
            g_frik.refreshAfterExternalHandAuthority(isLeft);
            return true;
        }

        if (!isSameExternalHandAuthoritySelection(oldSelected, newSelected)) {
            if (!skelly->applyExternalHandWorldTransform(isLeft, newSelected.entry->worldTarget)) {
                return false;
            }
            entries = std::move(updatedEntries);
            g_frik.refreshAfterExternalHandAuthority(isLeft);
            return true;
        }

        entries = std::move(updatedEntries);
        return true;
    }

    void clearExternalStateForSkeletonRelease()
    {
        for (auto& entries : g_externalHandAuthorities) {
            entries.clear();
        }
        g_externalHandAuthorityGeneration = 0;
        HandPose::clearPrimaryWeaponPoseBlocks();
        Skeleton::clearPrimaryWeaponNodeOwnershipBlocks();
        clearWeaponHandRecoilControllersForSkeletonRelease();
    }
}
