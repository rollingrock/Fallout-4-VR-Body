#define FRIK_API_EXPORTS
#include "FRIKApi.h"
#include "RecoilControllerRuntime.h"

#include "FRIK.h"
#include "common/CommonUtils.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "skeleton/HandPose.h"
#include "skeleton/HandPoseData.h"
#include "skeleton/Skeleton.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

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

    std::size_t handAuthorityIndex(const bool isLeft) { return isLeft ? 1U : 0U; }

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
        return HandFingersPose{
            FingerPose{ thumb, thumb, thumb },
            FingerPose{ index, index, index },
            FingerPose{ middle, middle, middle },
            FingerPose{ ring, ring, ring },
            FingerPose{ pinky, pinky, pinky }
        };
    }

    HandFingersPose makeHandPoseFromApiData(const FRIKApi::HandPoseData& handPose, const HandPoseKind kind = HandPoseKind::Custom)
    {
        return HandFingersPose{
            FingerPose{ handPose.thumb.prox, handPose.thumb.mid, handPose.thumb.dist, handPose.thumb.splay },
            FingerPose{ handPose.index.prox, handPose.index.mid, handPose.index.dist, handPose.index.splay },
            FingerPose{ handPose.middle.prox, handPose.middle.mid, handPose.middle.dist, handPose.middle.splay },
            FingerPose{ handPose.ring.prox, handPose.ring.mid, handPose.ring.dist, handPose.ring.splay },
            FingerPose{ handPose.pinky.prox, handPose.pinky.mid, handPose.pinky.dist, handPose.pinky.splay },
            handPose.palmPitch,
            handPose.palmYaw,
            kind
        };
    }

    void copyLocalTransformsToApiData(
        const std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT>& localTransforms,
        const std::uint16_t enabledMask,
        FRIKApi::FingerLocalTransformOverride& outTransforms)
    {
        outTransforms = {};
        outTransforms.enabledMask = enabledMask;
        for (std::size_t i = 0; i < localTransforms.size(); ++i) {
            outTransforms.localTransforms[i] = localTransforms[i];
        }
    }

    std::optional<HandFingersPose> makePredefinedHandPose(const FRIKApi::HandPoseKind handPose)
    {
        switch (handPose) {
        case FRIKApi::HandPoseKind::Open:
            return getOpenPose();
        case FRIKApi::HandPoseKind::Pointing:
            return getPointingPose();
        case FRIKApi::HandPoseKind::HoldingWeapon:
            return HandPose::getFixedPrimaryWeaponPose();
        case FRIKApi::HandPoseKind::OffhandGrip:
            return getOffhandWeaponGripPose();
        case FRIKApi::HandPoseKind::Attaboy:
            return getAttaboyPose();
        case FRIKApi::HandPoseKind::ThumbsUp:
            return getThumbsUpPose();
        case FRIKApi::HandPoseKind::HoldingGun:
            return getGunGripPose();
        case FRIKApi::HandPoseKind::HoldingMelee:
            return getMeleeGripPose();
        default:
            return std::nullopt;
        }
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
        case HandPoseKind::Fist:
            return FRIKApi::HandPoseKind::Unset;
        case HandPoseKind::HoldingGun:
            return FRIKApi::HandPoseKind::HoldingGun;
        case HandPoseKind::HoldingMelee:
            return FRIKApi::HandPoseKind::HoldingMelee;
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

    bool FRIK_CALL blockPrimaryHandWeaponPose(const char* tag, const bool block)
    {
        if (!f4cf::common::hasNonWhitespaceText(tag)) {
            return false;
        }

        const std::string normalizedTag = f4cf::common::trim(tag);
        if (!HandPose::blockPrimaryWeaponPose(normalizedTag, block)) {
            return false;
        }

        return true;
    }

    bool FRIK_CALL blockPrimaryWeaponNodeOwnership(const char* tag, const bool block)
    {
        if (!f4cf::common::hasNonWhitespaceText(tag)) {
            return false;
        }

        const std::string normalizedTag = f4cf::common::trim(tag);
        if (!Skeleton::blockPrimaryWeaponNodeOwnership(normalizedTag, block)) {
            return false;
        }

        return true;
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

        const auto pose = makePredefinedHandPose(handPose);
        if (!pose) {
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

        HandPose::setHandPoseOverride(getIsLeftForHandEnum(hand), *normalizedTag, makeHandPoseFromApiData(handPose), forceTop);
        return true;
    }

    bool FRIK_CALL setHandPoseCustomWithPriority(const char* tag, const FRIKApi::Hand hand, const FRIKApi::HandPoseData& handPose, const int priority)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag || priority < 0) {
            return false;
        }

        HandPose::setHandPoseOverrideWithPriority(getIsLeftForHandEnum(hand), *normalizedTag, makeHandPoseFromApiData(handPose), priority);
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

    bool FRIK_CALL registerOpenModSettingButtonToMainConfig(const FRIKApi::OpenExternalModConfigData& data)
    {
        if (!data.buttonIconNifPath || !data.callbackReceiverName) {
            return false;
        }
        g_frik.registerOpenSettingButton({
            .buttonIconNifPath = data.buttonIconNifPath,
            .callbackReceiverName = data.callbackReceiverName,
            .callbackMessageType = data.callbackMessageType
        });
        return true;
    }

    bool FRIK_CALL setHandPoseWithPriority(const char* tag, const FRIKApi::Hand hand, const FRIKApi::HandPoseKind handPose, const int priority)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag || priority < 0) {
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

        const auto pose = makePredefinedHandPose(handPose);
        if (!pose) {
            return false;
        }

        HandPose::setHandPoseOverrideWithPriority(isLeft, *normalizedTag, *pose, priority);
        return true;
    }

    bool FRIK_CALL applyExternalHandWorldTransform(const char* tag, const FRIKApi::Hand hand, const RE::NiTransform& worldTarget, const int priority)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        auto* skelly = g_frik.getSkeleton();
        if (!normalizedTag || priority < 0 || !skelly) {
            return false;
        }

        const bool isLeft = getIsLeftForHandEnum(hand);
        auto& entries = g_externalHandAuthorities[handAuthorityIndex(isLeft)];
        auto updatedEntries = entries;
        const auto nextGeneration = g_externalHandAuthorityGeneration + 1;
        auto it = std::ranges::find_if(updatedEntries, [&](const ExternalHandAuthorityEntry& entry) { return entry.tag == *normalizedTag; });
        if (it == updatedEntries.end()) {
            updatedEntries.push_back(ExternalHandAuthorityEntry{
                .tag = *normalizedTag,
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

    bool FRIK_CALL setHandPoseCustomLocalTransformsWithPriority(
        const char* tag,
        const FRIKApi::Hand hand,
        const FRIKApi::FingerLocalTransformOverride* overrideData,
        const int priority)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag || !overrideData || priority < 0) {
            return false;
        }

        std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT> localTransforms{};
        for (std::size_t i = 0; i < localTransforms.size(); ++i) {
            localTransforms[i] = overrideData->localTransforms[i];
        }

        return HandPose::setHandPoseLocalTransformsWithPriority(
            getIsLeftForHandEnum(hand),
            *normalizedTag,
            localTransforms,
            overrideData->enabledMask,
            priority);
    }

    bool FRIK_CALL getHandPoseLocalTransformsForPose(
        const FRIKApi::Hand hand,
        const FRIKApi::HandPoseData& handPose,
        FRIKApi::FingerLocalTransformOverride* outTransforms)
    {
        if (!outTransforms) {
            return false;
        }

        std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT> localTransforms{};
        std::uint16_t enabledMask = 0;
        if (!HandPose::buildFingerLocalTransformsForPose(getIsLeftForHandEnum(hand), makeHandPoseFromApiData(handPose), localTransforms, enabledMask)) {
            *outTransforms = {};
            return false;
        }

        copyLocalTransformsToApiData(localTransforms, enabledMask, *outTransforms);
        return true;
    }

    bool FRIK_CALL clearExternalHandWorldTransform(const char* tag, const FRIKApi::Hand hand)
    {
        const auto normalizedTag = getNormalizedTag(tag);
        if (!normalizedTag) {
            return false;
        }

        const bool isLeft = getIsLeftForHandEnum(hand);
        auto& entries = g_externalHandAuthorities[handAuthorityIndex(isLeft)];
        auto updatedEntries = entries;
        const auto oldSize = updatedEntries.size();
        updatedEntries.erase(std::remove_if(updatedEntries.begin(), updatedEntries.end(), [&](const ExternalHandAuthorityEntry& entry) { return entry.tag == *normalizedTag; }),
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

    constexpr FRIKApi FRIK_API_FUNCTIONS_TABLE{
        .getVersion = &getVersion,
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
        .setHandPoseWithPriority = &setHandPoseWithPriority,
        .setHandPoseCustomWithPriority = &setHandPoseCustomWithPriority,
        .applyExternalHandWorldTransform = &applyExternalHandWorldTransform,
        .clearExternalHandWorldTransform = &clearExternalHandWorldTransform,
        .setHandPoseCustomLocalTransformsWithPriority = &setHandPoseCustomLocalTransformsWithPriority,
        .getHandPoseLocalTransformsForPose = &getHandPoseLocalTransformsForPose,
        .blockPrimaryHandWeaponPose = &blockPrimaryHandWeaponPose,
        .blockPrimaryWeaponNodeOwnership = &blockPrimaryWeaponNodeOwnership,
        .registerWeaponHandRecoilController = &frik::api::registerWeaponHandRecoilController,
        .unregisterWeaponHandRecoilController = &frik::api::unregisterWeaponHandRecoilController
    };
}

namespace frik::api
{
    void clearExternalHandAuthorityStateForSkeletonRelease()
    {
        for (auto& entries : g_externalHandAuthorities) {
            entries.clear();
        }
        g_externalHandAuthorityGeneration = 0;
        HandPose::clearPrimaryWeaponPoseBlocks();
        Skeleton::clearPrimaryWeaponNodeOwnershipBlocks();
        clearWeaponHandRecoilControllersForSkeletonRelease();
    }

    FRIK_API const FRIKApi* FRIK_CALL FRIKAPI_GetApi()
    {
        return &FRIK_API_FUNCTIONS_TABLE;
    }

    FRIK_API std::uint32_t FRIK_CALL FRIKAPI_GetApiStructSize()
    {
        return sizeof(FRIKApi);
    }

    FRIK_API bool FRIK_CALL FRIKAPI_MirrorFingerLocalTransforms(const FRIKApi::Hand sourceHand,
        const FRIKApi::FingerLocalTransformOverride* sourceTransforms,
        FRIKApi::FingerLocalTransformOverride* outTargetTransforms)
    {
        if ((sourceHand != FRIKApi::Hand::Left && sourceHand != FRIKApi::Hand::Right) || !sourceTransforms || !outTargetTransforms) {
            return false;
        }
        *outTargetTransforms = {};

        auto* skeleton = g_frik.getSkeleton();
        if (!skeleton) {
            return false;
        }

        std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT> source{};
        for (std::size_t index = 0; index < source.size(); ++index) {
            source[index] = sourceTransforms->localTransforms[index];
        }

        std::array<RE::NiTransform, HandPose::FINGER_BONE_COUNT> target{};
        std::uint16_t targetMask = 0;
        if (!skeleton->mirrorFingerLocalTransforms(sourceHand == FRIKApi::Hand::Left, source, sourceTransforms->enabledMask, target, targetMask)) {
            return false;
        }

        copyLocalTransformsToApiData(target, targetMask, *outTargetTransforms);
        return true;
    }
}
