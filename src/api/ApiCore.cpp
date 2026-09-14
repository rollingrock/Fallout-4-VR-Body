#include "ApiCore.h"

#include <Windows.h>
#include <filesystem>
#include <format>
#include <unordered_set>

#include "Config.h"
#include "ExternalAuthority.h"
#include "FRIK.h"
#include "ScopeAuthority.h"
#include "TagBlockSet.h"
#include "common/CommonUtils.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"

using namespace frik::skeleton::data;

namespace
{
    using namespace frik;
    using namespace frik::api::core;

    /**
     * External tags blocking offhand gripping, so client mods cannot clobber each other.
     */
    TagBlockSet g_offHandGripBlocks;

    /**
     * Per-feature tags blocking each FRIK subsystem (see blockFeature).
     */
    std::array<TagBlockSet, FEATURE_COUNT> g_featureBlocks;

    /**
     * Frame-phase callbacks. They hold no node references, so they outlive skeleton rebuilds like feature blocks.
     */
    FramePhaseRegistry g_framePhases;

    /**
     * Client modules already reported per API table, so a mod that re-acquires - the published
     * initialize() is idempotent but nothing stops a client calling the export directly - is
     * logged once instead of on every call.
     *
     * Unsynchronized, unlike the rest of the API's cross-mod state: a client acquires the table
     * while initializing on the game thread, so two acquisitions never overlap.
     */
    std::unordered_set<std::string> g_reportedApiClients;

    /**
     * File name of the module owning an address, used to name the mod that called into us.
     */
    std::string moduleNameForAddress(const void* address)
    {
        HMODULE module = nullptr;
        if (!address || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, static_cast<LPCSTR>(address), &module) ||
            !module) {
            return "<unknown>";
        }

        char path[1024] = {};
        if (GetModuleFileNameA(module, path, static_cast<DWORD>(std::size(path))) == 0) {
            return "<unknown>";
        }
        return std::filesystem::path(path).filename().string();
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

    std::uint32_t FRIK_CORE_CALL getSkeletonGeneration()
    {
        return g_frik.getSkeletonGeneration();
    }

    bool FRIK_CORE_CALL isInPowerArmor()
    {
        return g_frik.isInPowerArmor();
    }

    bool FRIK_CORE_CALL setScopeProvider(const char* tag, const std::uint32_t capabilities)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }
        bool changed = false;
        if (!g_scopeAuthority.setProvider(*normalizedTag, capabilities, &changed)) {
            logger::warn("setScopeProvider REJECTED tag:'{}' - unknown capability bits 0x{:X}", *normalizedTag, capabilities);
            return false;
        }
        logger::info("setScopeProvider tag:'{}' capabilities:0x{:X} changed:{}", *normalizedTag, capabilities, changed);
        return true;
    }

    bool FRIK_CORE_CALL clearScopeProvider(const char* tag)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }
        bool changed = false;
        g_scopeAuthority.clearProvider(*normalizedTag, &changed);
        if (changed) {
            logger::info("clearScopeProvider tag:'{}'", *normalizedTag);
        }
        return true;
    }

    bool FRIK_CORE_CALL setLookingThroughScope(const char* tag, const bool lookingThrough)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }
        if (!g_scopeAuthority.setLookingThroughScope(*normalizedTag, lookingThrough)) {
            logger::sample("setLookingThroughScope REJECTED tag:'{}' - not a provider with PublishesLookingThrough", *normalizedTag);
            return false;
        }
        return true;
    }

    bool FRIK_CORE_CALL isLookingThroughScope()
    {
        return g_frik.isLookingThroughScope();
    }

    /**
     * Register or replace a callback for one frame phase. Registrations survive skeleton rebuilds; phases
     * only run while a skeleton exists. Refused from inside a frame callback.
     */
    bool FRIK_CORE_CALL registerFrameCallback(const char* tag, const std::uint32_t phase, const FrameCallback callback, void* const userData, const int priority)
    {
        const auto normalizedTag = normalizeTag(tag);
        const auto result = g_framePhases.set(normalizedTag.value_or(""), phase, callback, userData, priority);
        switch (result) {
        case FramePhaseRegistry::Result::Registered:
        case FramePhaseRegistry::Result::Replaced:
            logger::info("{} frame callback tag:'{}' phase:{} priority:{}",
                result == FramePhaseRegistry::Result::Replaced ? "replaced" : "registered",
                *normalizedTag,
                phase,
                priority);
            return true;
        case FramePhaseRegistry::Result::BadTag:
            logger::sample("registerFrameCallback REJECTED - tag is null or blank");
            return false;
        case FramePhaseRegistry::Result::NullCallback:
            logger::sample("registerFrameCallback REJECTED tag:'{}' - callback is null", *normalizedTag);
            return false;
        case FramePhaseRegistry::Result::BadPhase:
            logger::sample("registerFrameCallback REJECTED tag:'{}' - unknown phase {}", *normalizedTag, phase);
            return false;
        case FramePhaseRegistry::Result::NegativePriority:
            logger::sample("registerFrameCallback REJECTED tag:'{}' - priority {} is negative", *normalizedTag, priority);
            return false;
        case FramePhaseRegistry::Result::Full:
            logger::sample("registerFrameCallback REJECTED tag:'{}' - registry is full at {} callbacks", *normalizedTag, FramePhaseRegistry::CAPACITY);
            return false;
        case FramePhaseRegistry::Result::Reentrant:
            logger::sample("registerFrameCallback REJECTED tag:'{}' - called from inside a frame callback", normalizedTag.value_or("?"));
            return false;
        }
        return false;
    }

    /**
     * Drop every phase a tag registered. Unknown tags succeed.
     */
    bool FRIK_CORE_CALL unregisterFrameCallback(const char* tag)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            logger::sample("unregisterFrameCallback REJECTED - tag is null or blank");
            return false;
        }
        std::size_t removed = 0;
        if (!g_framePhases.remove(*normalizedTag, &removed)) {
            logger::sample("unregisterFrameCallback REJECTED tag:'{}' - called from inside a frame callback", *normalizedTag);
            return false;
        }
        if (removed > 0) {
            logger::info("unregistered frame callback tag:'{}' ({} phase(s))", *normalizedTag, removed);
        }
        return true;
    }

    void invokeFramePhase(const FramePhase phase)
    {
        g_framePhases.invoke(static_cast<std::uint32_t>(phase));
    }

    bool FRIK_CORE_CALL getBoneWorldTransform(const char* boneName, RE::NiTransform* outTransform)
    {
        const auto* skelly = g_frik.getSkeleton();
        if (!skelly || !boneName || !outTransform) {
            return false;
        }
        return skelly->getBoneWorldTransform(boneName, *outTransform);
    }

    bool getTrackedHandTransform(const bool isLeft, const TrackedHandKind kind, RE::NiTransform& outTransform)
    {
        const auto* skelly = g_frik.getSkeleton();
        if (!skelly) {
            return false;
        }
        const RE::NiNode* node = nullptr;
        switch (kind) {
        case TrackedHandKind::Wand:
            node = skelly->getWandNode(isLeft);
            break;
        case TrackedHandKind::WeaponOffset:
            node = skelly->getWeaponOffsetNode(isLeft);
            break;
        case TrackedHandKind::FirstPersonHand:
            node = skelly->getFirstPersonHandNode(isLeft);
            break;
        }
        if (!node) {
            return false;
        }
        outTransform = node->world;
        return true;
    }

    bool getArmChain(const bool isLeft, ArmChainTransforms& outChain)
    {
        const auto* skelly = g_frik.getSkeleton();
        if (!skelly) {
            return false;
        }
        const auto arm = skelly->getArm(isLeft);
        const std::array<const RE::NiAVObject*, 7> nodes{ arm.shoulder, arm.upper, arm.upperT1, arm.forearm1, arm.forearm2, arm.forearm3, arm.hand };
        const std::array<RE::NiTransform*, 7>
            targets{ &outChain.shoulder, &outChain.upperArm, &outChain.upperArmTwist, &outChain.forearm1, &outChain.forearm2, &outChain.forearm3, &outChain.hand };
        outChain.structSize = sizeof(ArmChainTransforms);
        outChain.validMask = 0;
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i]) {
                *targets[i] = nodes[i]->world;
                outChain.validMask |= 1u << i;
            } else {
                targets[i]->MakeIdentity();
            }
        }
        return true;
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

        // A block is an edge, not a per-frame state, so every block logs at info and only on a real
        // transition. logger::sample would be wrong here twice over: it keys on the format string,
        // so two mods toggling within the same second lose a line, and a client repeating a call it
        // already made would keep the bucket warm without ever having changed anything.
        bool changed = false;
        g_offHandGripBlocks.setBlocked(*normalizedTag, block, &changed);
        if (changed) {
            logger::info("blockOffHandWeaponGripping tag:'{}' block:{} activeBlocks:{}", *normalizedTag, block, g_offHandGripBlocks.blockingCount());
        }

        g_frik.setOffHandGrippingEnabled(!g_offHandGripBlocks.isBlocked());
        return true;
    }

    bool FRIK_CORE_CALL blockPrimaryHandWeaponPose(const char* tag, const bool block)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        bool changed = false;
        const bool set = g_externalAuthority.blockPrimaryWeaponPose(*normalizedTag, block, &changed);
        if (changed) {
            logger::info("blockPrimaryHandWeaponPose tag:'{}' block:{}", *normalizedTag, block);
        }
        return set;
    }

    bool FRIK_CORE_CALL blockPrimaryWeaponNodeOwnership(const char* tag, const bool block)
    {
        const auto normalizedTag = normalizeTag(tag);
        if (!normalizedTag) {
            return false;
        }

        bool changed = false;
        const bool set = g_externalAuthority.blockPrimaryWeaponNodeOwnership(*normalizedTag, block, &changed);
        if (changed) {
            logger::info("blockPrimaryWeaponNodeOwnership tag:'{}' block:{}", *normalizedTag, block);
        }
        return set;
    }

    /**
     * Read the current effective config value (override, else on-disk, else default) into outBuf.
     */
    int FRIK_CORE_CALL getConfigValue(const char* section, const char* key, char* outBuf, const int bufLen, const char* defaultValue)
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
    bool FRIK_CORE_CALL hasConfigValueOverride(const char* section, const char* key)
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
        logger::sample("setConfigValueOverride caller:'{}' {}.{} = '{}'", caller ? caller : "?", section, key, value);
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
        logger::sample("clearConfigValueOverride caller:'{}' {}.{}", caller ? caller : "?", section, key);
        g_config.clearConfigOverride(section, key);
        return true;
    }

    void logApiAcquired(const std::string_view apiName, const std::uint32_t apiVersion, const std::size_t tableSize, const void* returnAddress)
    {
        const auto moduleName = moduleNameForAddress(returnAddress);
        if (!g_reportedApiClients.emplace(std::format("{}|{}", apiName, moduleName)).second) {
            return;
        }

        // The table size is what a client's own size check compares against, so logging it turns
        // a client-side "contract mismatch" bail into something diagnosable from FRIK's log alone.
        logger::info("'{}' acquired {} (contract v{}, table {} bytes)", moduleName, apiName, apiVersion, tableSize);
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

    void setHandPose(const std::string_view tag, const bool isLeft, const HandFingersPose& pose, const int priority)
    {
        HandPose::setHandPoseOverride(isLeft, tag, pose, priority);
    }

    void clearHandPose(const std::string_view tag, const bool isLeft)
    {
        HandPose::clearHandPoseOverride(isLeft, tag);
    }

    /**
     * Publish a world transform for one hand, consumed by the skeleton's arm solve every frame until
     * it is cleared. Rejected while there is no skeleton, because the registration would be dropped
     * by the skeleton release that follows.
     */
    bool setHandWorldTransform(const std::string_view tag, const bool isLeft, const RE::NiTransform& worldTransform, const int priority)
    {
        // Taking over a hand is the strongest thing a client can do, and both refusals below hand
        // back a bare false that a client cannot tell apart. Sampled rather than logged outright
        // because a client that registers too early typically retries every frame.
        if (!g_frik.isSkeletonReady()) {
            logger::sample("setHandWorldTransform REJECTED tag:'{}' - skeleton not ready, publish after kSkeletonReady", tag);
            return false;
        }

        // Tag and priority were validated by the version shim, so a refusal here is the transform.
        if (!g_externalAuthority.setHandWorldTransform(tag, isLeft, worldTransform, priority)) {
            logger::sample("setHandWorldTransform REJECTED tag:'{}' - world transform is not finite", tag);
            return false;
        }

        logger::sample("setHandWorldTransform tag:'{}' hand={} priority={}", tag, isLeft ? "Left" : "Right", priority);
        return true;
    }

    bool clearHandWorldTransform(const std::string_view tag, const bool isLeft)
    {
        logger::sample("clearHandWorldTransform tag:'{}' hand={}", tag, isLeft ? "Left" : "Right");
        return g_externalAuthority.clearHandWorldTransform(tag, isLeft);
    }

    bool setHandPoseLocalTransforms(const std::string_view tag, const bool isLeft, const std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& localTransforms,
        const std::uint16_t enabledMask, const int priority)
    {
        if (!HandPose::setHandPoseOverrideLocalTransforms(isLeft, tag, localTransforms, enabledMask, priority)) {
            logger::sample("setHandPoseCustomLocalTransforms REJECTED tag:'{}' - tag holds no pose override, set one with a setHandPose* call first", tag);
            return false;
        }

        logger::sample("setHandPoseCustomLocalTransforms tag:'{}' hand={} enabledMask=0x{:04x} priority={}", tag, isLeft ? "Left" : "Right", enabledMask, priority);
        return true;
    }

    bool getHandPoseLocalTransformsForPose(const bool isLeft, const HandFingersPose& pose, std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& outTransforms,
        std::uint16_t& outEnabledMask)
    {
        return HandPoseMath::buildFingerLocalTransformsForPose(isLeft, pose, outTransforms, outEnabledMask);
    }

    bool mirrorFingerLocalTransforms(const bool sourceIsLeft, const std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& sourceTransforms,
        const std::uint16_t sourceEnabledMask, std::array<RE::NiTransform, skeleton::data::FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask)
    {
        return HandPoseMath::mirrorFingerLocalTransforms(sourceIsLeft, sourceTransforms, sourceEnabledMask, outTargetTransforms, outTargetEnabledMask);
    }

    /**
     * Enable/disable a FRIK subsystem for a specific external tag.
     * The feature remains disabled while at least one tag is still blocking it.
     */
    bool blockFeature(const std::string_view tag, const Feature feature, const bool block)
    {
        const auto featureIndex = static_cast<std::size_t>(feature);
        if (featureIndex >= g_featureBlocks.size()) {
            return false;
        }

        auto& blocks = g_featureBlocks[featureIndex];
        bool changed = false;
        if (!blocks.setBlocked(tag, block, &changed)) {
            return false;
        }

        if (changed) {
            logger::info("blockFeature tag:'{}' - feature:{}, block:{}, activeBlocks:{}", tag, featureIndex, block, blocks.blockingCount());
        }
        applyFeatureEnabled(feature, !blocks.isBlocked());
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
            logger::warn("registerOpenModSettingButtonToMainConfig REJECTED - buttonIconNifPath or callbackReceiverName is null");
            return false;
        }

        // A button showing up in FRIK's own config menu should never be anonymous in the log.
        logger::info("registerOpenModSettingButtonToMainConfig receiver:'{}' messageType:{} icon:'{}'", callbackReceiverName, callbackMessageType, buttonIconNifPath);
        g_frik.registerOpenSettingButton({ .buttonIconNifPath = buttonIconNifPath, .callbackReceiverName = callbackReceiverName, .callbackMessageType = callbackMessageType });
        return true;
    }

}
