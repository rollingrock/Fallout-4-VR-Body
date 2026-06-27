#define FRIK_API_EXPORTS
#include "FRIKAPI.h"

#include "Config.h"
#include "FRIK.h"
#include "common/CommonUtils.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "skeleton/HandPose.h"
#include "skeleton/HandPoseData.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

namespace
{
    using namespace frik;
    using namespace frik::api;
    using namespace frik::skeleton::data;

    /**
     * Used to keep track of external tags blocking offhand gripping to prevent conflicts between client mods.
     * The actual tag values are not relevant to FRIK, only the fact that there is at least one tag blocking it.
     */
    std::unordered_set<std::string> g_offHandGripBlockingTags;
    constexpr std::string_view LEGACY_API_HAND_POSE_TAG = "frik.api.legacy";

    /**
     * Per-feature sets of external tags blocking each FRIK subsystem (see blockFeature).
     * A feature stays disabled while at least one tag is still blocking it. Indexed by FRIKApi::Feature.
     */
    constexpr std::size_t FEATURE_COUNT = 4;
    std::array<std::unordered_set<std::string>, FEATURE_COUNT> g_featureBlockingTags;

    /**
     * Apply the resolved enabled state of a feature to its FRIK subsystem.
     */
    void applyFeatureEnabled(const FRIKApi::Feature feature, const bool enabled)
    {
        switch (feature) {
        case FRIKApi::Feature::Flashlight:
            g_frik.setFlashlightEnabled(enabled);
            break;
        case FRIKApi::Feature::WeaponPositioning:
            g_frik.setWeaponPositionEnabled(enabled);
            break;
        case FRIKApi::Feature::Pipboy:
            g_frik.setPipboyEnabled(enabled);
            break;
        case FRIKApi::Feature::SmoothMovement:
            g_frik.setSmoothMovementEnabled(enabled);
            break;
        }
    }

    bool getIsLeftForHandEnum(const FRIKApi::Hand hand)
    {
        switch (hand) {
        case FRIKApi::Hand::Primary:
            return f4vr::isLeftHandedMode();
        case FRIKApi::Hand::Offhand:
            return !f4vr::isLeftHandedMode();
        case FRIKApi::Hand::Right:
            return false;
        case FRIKApi::Hand::Left:
            return true;
        }
        return false;
    }

    std::optional<std::string> getNormalizedTag(const char* tag)
    {
        if (!f4cf::common::hasNonWhitespaceText(tag)) {
            return std::nullopt;
        }

        return f4cf::common::trim(tag);
    }

    HandFingersPose makeUniformFingerPose(const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        return HandFingersPose{ FingerPose{ thumb, thumb, thumb },
            FingerPose{ index, index, index },
            FingerPose{ middle, middle, middle },
            FingerPose{ ring, ring, ring },
            FingerPose{ pinky, pinky, pinky } };
    }

    FRIKApi::HandPoseTagState toApiHandPoseTagState(const HandPoseOverrideTagState state)
    {
        switch (state) {
        case HandPoseOverrideTagState::None:
            return FRIKApi::HandPoseTagState::None;
        case HandPoseOverrideTagState::Active:
            return FRIKApi::HandPoseTagState::Active;
        case HandPoseOverrideTagState::Overridden:
            return FRIKApi::HandPoseTagState::Overriden;
        default:
            return FRIKApi::HandPoseTagState::None;
        }
    }

    FRIKApi::HandPoseKind toApiHandPoseKind(const frik::skeleton::data::HandPoseKind kind)
    {
        switch (kind) {
        case HandPoseKind::Unset:
            return FRIKApi::HandPoseKind::Unset;
        case HandPoseKind::Custom:
            return FRIKApi::HandPoseKind::Custom;
        case HandPoseKind::Open:
            return FRIKApi::HandPoseKind::Open;
        case HandPoseKind::Pointing:
            return FRIKApi::HandPoseKind::Pointing;
        case HandPoseKind::HoldingWeapon:
            return FRIKApi::HandPoseKind::HoldingWeapon;
        case HandPoseKind::OffhandGrip:
            return FRIKApi::HandPoseKind::OffhandGrip;
        case HandPoseKind::Attaboy:
            return FRIKApi::HandPoseKind::Attaboy;
        case HandPoseKind::ThumbsUp:
            return FRIKApi::HandPoseKind::ThumbsUp;
        default:
            return FRIKApi::HandPoseKind::Unset;
        }
    }

    std::uint32_t FRIK_CALL getVersion()
    {
        return FRIK_API_VERSION;
    }

    const char* FRIK_CALL getModVersion()
    {
        // Safe to return pointer to static data
        static_assert(Version::NAME.back() != '\0' || true, "Version must be backed by a string literal");
        return Version::NAME.data();
    }

    bool FRIK_CALL isSkeletonReady()
    {
        return g_frik.isSkeletonReady();
    }

    bool FRIK_CALL isConfigOpen()
    {
        return g_frik.isMainConfigurationModeActive() || g_frik.isPipboyConfigurationModeActive() || g_frik.inWeaponRepositionMode();
    }

    bool FRIK_CALL isSelfieModeOn()
    {
        return g_frik.isSelfieModeOn();
    }

    void FRIK_CALL setSelfieModeOn(const bool setOn)
    {
        g_frik.setSelfieMode(setOn);
    }

    bool FRIK_CALL isOffHandGrippingWeapon()
    {
        return g_frik.isOffHandGrippingWeapon();
    }

    /**
     * Enable/disable FRIK offhand weapon gripping for a specific external tag.
     * Offhand gripping remains disabled while at least one tag is still blocking it.
     */
    bool FRIK_CALL blockOffHandWeaponGripping(const char* tag, const bool block)
    {
        if (!f4cf::common::hasNonWhitespaceText(tag)) {
            return false;
        }

        const std::string normalizedTag = f4cf::common::trim(tag);
        if (block) {
            g_offHandGripBlockingTags.emplace(normalizedTag);
        } else {
            g_offHandGripBlockingTags.erase(normalizedTag);
        }

        logger::sample("API blockOffHandWeaponGripping tag:'{}' block:{} activeBlocks:{}", normalizedTag, block, g_offHandGripBlockingTags.size());
        g_frik.setOffHandGrippingEnabled(g_offHandGripBlockingTags.empty());
        return true;
    }

    /**
     * Enable/disable a FRIK subsystem for a specific external tag.
     * The feature remains disabled while at least one tag is still blocking it.
     */
    bool FRIK_CALL blockFeature(const char* tag, const FRIKApi::Feature feature, const bool block)
    {
        if (!f4cf::common::hasNonWhitespaceText(tag)) {
            return false;
        }

        const auto featureIndex = static_cast<std::size_t>(feature);
        if (featureIndex >= g_featureBlockingTags.size()) {
            return false;
        }

        const std::string normalizedTag = f4cf::common::trim(tag);
        auto& blockingTags = g_featureBlockingTags[featureIndex];
        if (block) {
            blockingTags.emplace(normalizedTag);
        } else {
            blockingTags.erase(normalizedTag);
        }

        logger::info("API blockFeature tag:'{}' - feature:{}, block:{}, activeBlocks:{}", normalizedTag, featureIndex, block, blockingTags.size());
        applyFeatureEnabled(feature, blockingTags.empty());
        return true;
    }

    /**
     * Check whether a FRIK subsystem is currently disabled (blocked by any tag).
     */
    bool FRIK_CALL isFeatureBlocked(const FRIKApi::Feature feature)
    {
        switch (feature) {
        case FRIKApi::Feature::Flashlight:
            return !g_frik.isFlashlightEnabled();
        case FRIKApi::Feature::WeaponPositioning:
            return !g_frik.isWeaponPositionEnabled();
        case FRIKApi::Feature::Pipboy:
            return !g_frik.isPipboyEnabled();
        case FRIKApi::Feature::SmoothMovement:
            return !g_frik.isSmoothMovementEnabled();
        }
        return false;
    }

    bool FRIK_CALL isWristPipboyOpen()
    {
        return g_frik.isPipboyOn();
    }

    RE::NiPoint3 FRIK_CALL getIndexFingerTipPosition(const FRIKApi::Hand hand)
    {
        return f4vr::Skelly::getIndexFingerTipWorldPosition(static_cast<vrcf::Hand>(hand));
    }

    FRIKApi::HandPoseTagState FRIK_CALL getHandPoseSetTagState(const char* tag, const FRIKApi::Hand hand)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag) {
            return FRIKApi::HandPoseTagState::None;
        }

        return toApiHandPoseTagState(HandPose::getHandPoseSetTagState(getIsLeftForHandEnum(hand), *normalizedTag));
    }

    FRIKApi::HandPoseKind FRIK_CALL getCurrentHandPose(const FRIKApi::Hand hand)
    {
        return toApiHandPoseKind(HandPose::getCurrentHandPoseKind(getIsLeftForHandEnum(hand)));
    }

    bool FRIK_CALL setHandPose(const char* tag, const FRIKApi::Hand hand, const FRIKApi::HandPoseKind handPose)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag) {
            return false;
        }

        const bool isLeft = getIsLeftForHandEnum(hand);
        if (handPose == FRIKApi::HandPoseKind::Unset) {
            HandPose::clearHandPoseOverride(isLeft, *normalizedTag);
            return true;
        }

        if (handPose == FRIKApi::HandPoseKind::Custom) {
            return false;
        }

        const HandFingersPose* pose;
        if (handPose == FRIKApi::HandPoseKind::Open) {
            pose = &getOpenPose();
        } else if (handPose == FRIKApi::HandPoseKind::Pointing) {
            pose = &getPointingPose();
        } else if (handPose == FRIKApi::HandPoseKind::HoldingWeapon) {
            pose = &HandPose::getFixedPrimaryWeaponPose();
        } else if (handPose == FRIKApi::HandPoseKind::OffhandGrip) {
            pose = &getOffhandWeaponGripPose();
        } else if (handPose == FRIKApi::HandPoseKind::Attaboy) {
            pose = &getAttaboyPose();
        } else if (handPose == FRIKApi::HandPoseKind::ThumbsUp) {
            pose = &getThumbsUpPose();
        } else {
            return false;
        }

        HandPose::setHandPoseOverride(isLeft, *normalizedTag, *pose, false);
        return true;
    }

    bool FRIK_CALL setHandPoseCustomFingerPositions(const char* tag, const FRIKApi::Hand hand, const float thumb, const float index, const float middle, const float ring,
        const float pinky)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag) {
            return false;
        }

        HandPose::setHandPoseOverride(getIsLeftForHandEnum(hand), *normalizedTag, makeUniformFingerPose(thumb, index, middle, ring, pinky), false);
        return true;
    }

    bool FRIK_CALL setHandPoseCustom(const char* tag, const FRIKApi::Hand hand, const FRIKApi::HandPoseData& handPose, const bool forceTop)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag) {
            return false;
        }

        HandPose::setHandPoseOverride(getIsLeftForHandEnum(hand),
            *normalizedTag,
            HandFingersPose{ FingerPose{ handPose.thumb.prox, handPose.thumb.mid, handPose.thumb.dist, handPose.thumb.splay },
                FingerPose{ handPose.index.prox, handPose.index.mid, handPose.index.dist, handPose.index.splay },
                FingerPose{ handPose.middle.prox, handPose.middle.mid, handPose.middle.dist, handPose.middle.splay },
                FingerPose{ handPose.ring.prox, handPose.ring.mid, handPose.ring.dist, handPose.ring.splay },
                FingerPose{ handPose.pinky.prox, handPose.pinky.mid, handPose.pinky.dist, handPose.pinky.splay },
                handPose.palmPitch,
                handPose.palmYaw,
                HandPoseKind::Custom },
            forceTop);
        return true;
    }

    bool FRIK_CALL clearHandPose(const char* tag, const FRIKApi::Hand hand)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag) {
            return false;
        }

        HandPose::clearHandPoseOverride(getIsLeftForHandEnum(hand), *normalizedTag);
        return true;
    }

    void FRIK_CALL setHandPoseFingerPositions(const FRIKApi::Hand hand, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        HandPose::setHandPoseOverride(getIsLeftForHandEnum(hand), LEGACY_API_HAND_POSE_TAG, makeUniformFingerPose(thumb, index, middle, ring, pinky), false);
    }

    void FRIK_CALL clearHandPoseFingerPositions(const FRIKApi::Hand hand)
    {
        HandPose::clearHandPoseOverride(getIsLeftForHandEnum(hand), LEGACY_API_HAND_POSE_TAG);
    }

    /**
     * Read the current effective config value (override, else on-disk, else default) into outBuf.
     */
    int FRIK_CALL getConfigValue(const char* caller, const char* section, const char* key, char* outBuf, const int bufLen, const char* defaultValue)
    {
        if (!section || !key) {
            if (outBuf && bufLen > 0) {
                outBuf[0] = '\0';
            }
            return 0;
        }

        const std::string value = g_config.getConfigValue(section, key, defaultValue);
        logger::debug("API getConfigValue caller:'{}' {}.{} = '{}'", caller ? caller : "?", section, key, value);
        if (outBuf && bufLen > 0) {
            const auto copied = value.copy(outBuf, static_cast<std::size_t>(bufLen) - 1);
            outBuf[copied] = '\0';
        }
        return static_cast<int>(value.size());
    }

    /**
     * Check whether a session override is currently set for a config section/key.
     */
    bool FRIK_CALL hasConfigValueOverride(const char* caller, const char* section, const char* key)
    {
        const bool result = section && key && g_config.hasConfigOverride(section, key);
        logger::debug("API hasConfigValueOverride caller:'{}' {}.{} = {}", caller ? caller : "?", section ? section : "?", key ? key : "?", result);
        return result;
    }

    /**
     * Set a session-only override for a config section/key (string parsed by the type-appropriate reader).
     */
    bool FRIK_CALL setConfigValueOverride(const char* caller, const char* section, const char* key, const char* value)
    {
        if (!section || !key || !value) {
            return false;
        }
        logger::info("API setConfigValueOverride caller:'{}' {}.{} = '{}'", caller ? caller : "?", section, key, value);
        g_config.setConfigOverride(section, key, value);
        return true;
    }

    /**
     * Remove a previously set session override for a config section/key.
     */
    bool FRIK_CALL clearConfigValueOverride(const char* caller, const char* section, const char* key)
    {
        if (!section || !key || !g_config.hasConfigOverride(section, key)) {
            return false;
        }
        logger::info("API clearConfigValueOverride caller:'{}' {}.{}", caller ? caller : "?", section, key);
        g_config.clearConfigOverride(section, key);
        return true;
    }

    bool FRIK_CALL registerOpenModSettingButtonToMainConfig(const FRIKApi::OpenExternalModConfigData& data)
    {
        if (!data.buttonIconNifPath || !data.callbackReceiverName) {
            return false;
        }
        g_frik.registerOpenSettingButton(
            { .buttonIconNifPath = data.buttonIconNifPath, .callbackReceiverName = data.callbackReceiverName, .callbackMessageType = data.callbackMessageType });
        return true;
    }

    constexpr FRIKApi FRIK_API_FUNCTIONS_TABLE{ .getVersion = &getVersion,
        .getModVersion = &getModVersion,
        .isSkeletonReady = &isSkeletonReady,
        .isConfigOpen = &isConfigOpen,
        .isSelfieModeOn = &isSelfieModeOn,
        .setSelfieModeOn = &setSelfieModeOn,
        .isOffHandGrippingWeapon = &isOffHandGrippingWeapon,
        .isWristPipboyOpen = &isWristPipboyOpen,
        .getIndexFingerTipPosition = &getIndexFingerTipPosition,
        .getHandPoseSetTagState = &getHandPoseSetTagState,
        .getCurrentHandPose = &getCurrentHandPose,
        .setHandPose = &setHandPose,
        .setHandPoseCustomFingerPositions = &setHandPoseCustomFingerPositions,
        .clearHandPose = &clearHandPose,
        .setHandPoseFingerPositions = &setHandPoseFingerPositions,
        .clearHandPoseFingerPositions = &clearHandPoseFingerPositions,
        .registerOpenModSettingButtonToMainConfig = &registerOpenModSettingButtonToMainConfig,
        .blockOffHandWeaponGripping = &blockOffHandWeaponGripping,
        .setHandPoseCustom = &setHandPoseCustom,
        .blockFeature = &blockFeature,
        .isFeatureBlocked = &isFeatureBlocked,
        .getConfigValue = &getConfigValue,
        .hasConfigValueOverride = &hasConfigValueOverride,
        .setConfigValueOverride = &setConfigValueOverride,
        .clearConfigValueOverride = &clearConfigValueOverride };
}

namespace frik::api
{
    FRIK_API const FRIKApi* FRIK_CALL FRIKAPI_GetApi()
    {
        return &FRIK_API_FUNCTIONS_TABLE;
    }
}
