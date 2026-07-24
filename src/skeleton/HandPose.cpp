#include "HandPose.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>

#include "Config.h"
#include "FRIK.h"
#include "HandPoseData.h"
#include "Skeleton.h"
#include "common/MatrixUtils.h"
#include "common/Quaternion.h"
#include "f4vr/BSFlattenedBoneTree.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "vrcf/VRControllersManager.h"

using namespace common;
using namespace f4vr;
using namespace vrcf;
using namespace frik::skeleton::data;

namespace
{
    std::mutex g_primaryWeaponPoseBlockingTagsLock;
    std::unordered_set<std::string> g_primaryWeaponPoseBlockingTags;
    std::atomic<std::uint32_t> g_primaryWeaponPoseBlockingTagCount{ 0 };

    /**
     * Copy the authored 3x4 rotation rows from pose data into a runtime transform.
     */
    void copyRotationIntoTransform(const RotationData& rotationData, RE::NiTransform& transform)
    {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                transform.rotate.entry[row][col] = rotationData[row * 4 + col];
            }
        }
    }

    /**
     * Return whether a hand bone belongs to the left hand based on its name prefix.
     */
    bool isLeftHandBone(const std::string& boneName)
    {
        return boneName[0] == 'L';
    }

    /**
     * Convert a finger bone name like Finger31 into a zero-based finger index.
     */
    int boneToFingerIndex(const std::string& bone)
    {
        return bone[bone.size() - 2] - '1';
    }

    int boneToFlexIndex(const std::string& bone)
    {
        return boneToFingerIndex(bone) * 3 + (bone.back() - '1');
    }

    /**
     * Map a finger to the controller input that should drive its dynamic curl.
     */
    VRButtonId getTrackedButton(const std::string& bone)
    {
        switch (boneToFingerIndex(bone)) {
        case 0:
            return k_EButton_SteamVR_Touchpad;
        case 1:
            return k_EButton_SteamVR_Trigger;
        default:
            return k_EButton_Grip;
        }
    }

    /**
     * Refresh one flattened bone transform after its local transform changes.
     */
    void refreshFlattenedBoneTransform(const BSFlattenedBoneTree* const boneTree, const int pos)
    {
        auto& transform = boneTree->transforms[pos];
        if (transform.refNode) {
            transform.refNode->local = transform.local;
        }

        const auto parentWorld = transform.refNode && transform.refNode->parent
            ? transform.refNode->parent->world
            : boneTree->transforms[transform.parPos].world;
        RE::NiPoint3 p = transform.local.translate;
        p = parentWorld.rotate.Transpose() * (p * parentWorld.scale);
        transform.world.translate = parentWorld.translate + p;
        transform.world.rotate = transform.local.rotate * parentWorld.rotate;
        transform.world.scale = transform.local.scale * parentWorld.scale;

        if (transform.refNode) {
            transform.refNode->world = transform.world;
        }
    }

    /**
     * Blend one palm axis toward its target and snap the tail to avoid endless tiny drift.
     */
    void blendPalmAxisToward(float& currentValue, const float targetValue, const float frameTime)
    {
        if (fEqual(currentValue, targetValue)) {
            currentValue = targetValue;
            return;
        }

        currentValue += (targetValue - currentValue) * std::clamp(frameTime * 7.0f, 0.0f, 1.0f);
        if (fEqual(currentValue, targetValue)) {
            currentValue = targetValue;
        }
    }

    constexpr std::string_view PIPBOY_HAND_POSE_TAG = "frik.pipboy";
    constexpr std::string_view CONFIG_MODE_HAND_POSE_TAG = "frik.config_mode";
    constexpr std::string_view FORCE_POINTING_HAND_POSE_TAG = "frik.force_pointing";
    constexpr std::string_view OFFHAND_GRIP_HAND_POSE_TAG = "frik.offhand_grip";
    constexpr std::string_view ATTABOY_HAND_POSE_TAG = "frik.attaboy";

    constexpr int EXTERNAL_HAND_POSE_PRIORITY = 50;
    constexpr int FORCED_HAND_POSE_PRIORITY = 90;

    int priorityFromForceTop(const bool forceTop)
    {
        return forceTop ? FORCED_HAND_POSE_PRIORITY : EXTERNAL_HAND_POSE_PRIORITY;
    }

    constexpr std::uint16_t FULL_LOCAL_TRANSFORM_MASK = 0x7FFF;

    bool isFiniteRotation(const RE::NiMatrix3& rotation)
    {
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                if (!std::isfinite(rotation.entry[row][column])) {
                    return false;
                }
            }
        }
        return true;
    }

    bool isFiniteTransform(const RE::NiTransform& transform)
    {
        return isFiniteRotation(transform.rotate) &&
               std::isfinite(transform.translate.x) &&
               std::isfinite(transform.translate.y) &&
               std::isfinite(transform.translate.z) &&
               std::isfinite(transform.scale) &&
               std::abs(transform.scale) > 0.0001f;
    }

    RE::NiTransform buildPoseBoneLocalTransform(
        const frik::skeleton::data::HandBonePoseData& boneData,
        const frik::HandFingersPose& pose,
        const bool inPowerArmor)
    {
        RE::NiTransform openTransform{};
        RE::NiTransform closedTransform{};
        copyRotationIntoTransform(boneData.openRotation, openTransform);
        copyRotationIntoTransform(boneData.closedRotation, closedTransform);

        openTransform.translate = inPowerArmor ? boneData.openTranslationInPowerArmor : boneData.openTranslation;
        closedTransform.translate = openTransform.translate;
        openTransform.scale = 1.0f;
        closedTransform.scale = 1.0f;

        Quaternion qOpen;
        Quaternion qClosed;
        qOpen.fromMatrix(openTransform.rotate);
        qClosed.fromMatrix(closedTransform.rotate);
        qClosed.slerp(std::clamp(pose.getFlexAt(boneToFlexIndex(boneData.boneName)), -1.0f, 2.0f), qOpen);

        RE::NiTransform result = openTransform;
        result.rotate = qClosed.getMatrix();
        if (std::string_view(boneData.boneName).back() == '1') {
            const float sign = boneData.boneName[0] == 'L' ? -1.0f : 1.0f;
            result.rotate = MatrixUtils::getMatrixFromEulerAngles(0.0f, sign * pose.getFingerAt(boneToFingerIndex(boneData.boneName)).splay, 0.0f) * result.rotate;
        }
        return result;
    }
}

namespace frik
{
    // -- HandFingersPose ----------------------------------------------------------------

    /**
     * Return a mutable finger pose by zero-based finger index.
     */
    FingerPose& HandFingersPose::getFingerAt(const int fingerIndex) noexcept
    {
        FingerPose* const fingers[5] = { &thumb, &index, &middle, &ring, &pinky };
        return *fingers[fingerIndex];
    }

    /**
     * Return a read-only finger pose by zero-based finger index.
     */
    const FingerPose& HandFingersPose::getFingerAt(const int fingerIndex) const noexcept
    {
        const FingerPose* const fingers[5] = { &thumb, &index, &middle, &ring, &pinky };
        return *fingers[fingerIndex];
    }

    /**
     * Return a flex value by flat bone index.
     *
     * Bone order is 0-2: thumb, 3-5: index, 6-8: middle, 9-11: ring, 12-14: pinky,
     * with each finger laid out as prox, mid, dist.
     */
    float HandFingersPose::getFlexAt(const int boneIndex) const noexcept
    {
        const FingerPose& f = getFingerAt(boneIndex / 3);
        switch (boneIndex % 3) {
        case 0:
            return f.prox;
        case 1:
            return f.mid;
        default:
            return f.dist;
        }
    }

    // -- Lifecycle ----------------------------------------------------------------------

    /**
     * Build the authored open/closed reference transforms used to drive hand posing.
     */
    HandPose::HandPose(const bool inPowerArmor)
    {
        _handClosed.clear();
        _handOpen.clear();
        _handBones.clear();
        _leftHandOverrides.clear();
        _rightHandOverrides.clear();

        for (const auto& boneData : getHandBoneData()) {
            copyRotationIntoTransform(boneData.closedRotation, _handClosed[boneData.boneName]);
            copyRotationIntoTransform(boneData.openRotation, _handOpen[boneData.boneName]);
            _handOpen[boneData.boneName].translate = inPowerArmor ? boneData.openTranslationInPowerArmor : boneData.openTranslation;
        }

        _handBones = _handOpen;
    }

    /**
     * Activate an explicit pose override for one hand.
     */
    void HandPose::setHandPoseOverride(const bool isLeft, const std::string_view tag, const HandFingersPose& pose, const bool forceTop = false)
    {
        setHandPoseOverrideIntr(isLeft, tag, pose, priorityFromForceTop(forceTop));
    }

    void HandPose::setHandPoseOverrideWithPriority(const bool isLeft, const std::string_view tag, const HandFingersPose& pose, const int priority)
    {
        setHandPoseOverrideIntr(isLeft, tag, pose, priority);
    }

    bool HandPose::setHandPoseLocalTransformsWithPriority(
        const bool isLeft,
        const std::string_view tag,
        const std::array<RE::NiTransform, FINGER_BONE_COUNT>& localTransforms,
        const std::uint16_t enabledMask,
        const int priority)
    {
        if (tag.empty() || priority < 0) {
            return false;
        }

        auto& overrides = getHandOverrides(isLeft);
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) { return overrideEntry.tag == tag; });
        if (overrideIt == overrides.end()) {
            return false;
        }

        overrideIt->priority = priority;
        overrideIt->sequence = ++_nextOverrideSequence;
        overrideIt->localTransformMask = static_cast<std::uint16_t>(enabledMask & FULL_LOCAL_TRANSFORM_MASK);
        overrideIt->localTransforms = localTransforms;

        std::ranges::sort(overrides, [](const TaggedHandPoseOverride& lhs, const TaggedHandPoseOverride& rhs) {
            if (lhs.priority != rhs.priority) {
                return lhs.priority > rhs.priority;
            }
            return lhs.sequence > rhs.sequence;
        });

        return true;
    }

    /**
     * Release any explicit override and return control to runtime hand logic.
     */
    void HandPose::clearHandPoseOverride(const bool isLeft, const std::string_view tag)
    {
        clearHandPoseOverrideIntr(isLeft, tag);
    }

    /**
     * Return whether a specific override tag is absent, active, or shadowed by another tag.
     */
    HandPoseOverrideTagState HandPose::getHandPoseSetTagState(const bool isLeft, const std::string_view tag)
    {
        const auto& overrides = getHandOverrides(isLeft);
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) { return overrideEntry.tag == tag; });
        if (overrideIt == overrides.end()) {
            return HandPoseOverrideTagState::None;
        }

        const auto* activeOverride = getActiveHandPoseOverride(isLeft);
        if (activeOverride && activeOverride->tag == tag) {
            const auto source = resolveHandPoseSource(isLeft);
            if (source.kind == HandPoseSourceKind::OverridePose && source.pose == &activeOverride->pose) {
                return HandPoseOverrideTagState::Active;
            }
        }

        return HandPoseOverrideTagState::Overridden;
    }

    /**
     * Return the effective hand pose kind resolved for the current frame.
     */
    HandPoseKind HandPose::getCurrentHandPoseKind(const bool isLeft)
    {
        const auto source = resolveHandPoseSource(isLeft);
        if (source.kind == HandPoseSourceKind::PrimaryWeaponPose) {
            return source.pose ? source.pose->kind : HandPoseKind::HoldingWeapon;
        }

        if (source.kind == HandPoseSourceKind::OverridePose && source.pose) {
            return source.pose->kind;
        }

        return HandPoseKind::Unset;
    }

    bool HandPose::blockPrimaryWeaponPose(const std::string_view tag, const bool block)
    {
        if (tag.empty()) {
            return false;
        }

        std::lock_guard lock(g_primaryWeaponPoseBlockingTagsLock);
        if (block) {
            g_primaryWeaponPoseBlockingTags.emplace(std::string(tag));
        } else {
            g_primaryWeaponPoseBlockingTags.erase(std::string(tag));
        }
        g_primaryWeaponPoseBlockingTagCount.store(static_cast<std::uint32_t>(g_primaryWeaponPoseBlockingTags.size()), std::memory_order_release);
        return true;
    }

    bool HandPose::isPrimaryWeaponPoseBlocked()
    {
        return g_primaryWeaponPoseBlockingTagCount.load(std::memory_order_acquire) != 0;
    }

    void HandPose::clearPrimaryWeaponPoseBlocks()
    {
        std::lock_guard lock(g_primaryWeaponPoseBlockingTagsLock);
        g_primaryWeaponPoseBlockingTags.clear();
        g_primaryWeaponPoseBlockingTagCount.store(0, std::memory_order_release);
    }

    /**
     * Return the fixed authored weapon pose used when runtime hand posing cannot copy the first-person hand.
     */
    const HandFingersPose& HandPose::getFixedPrimaryWeaponPose()
    {
        return isUnarmedWeaponEquipped()
            ? getFistPose()
            : (g_frik.isMeleeWeaponDrawn() ? getMeleeGripPose() : getGunGripPose());
    }

    bool HandPose::buildFingerLocalTransformsForPose(
        const bool isLeft,
        const HandFingersPose& pose,
        std::array<RE::NiTransform, FINGER_BONE_COUNT>& outTransforms,
        std::uint16_t& outEnabledMask)
    {
        outTransforms = {};
        outEnabledMask = 0;

        const bool inPowerArmor = f4vr::isInPowerArmor();
        for (const auto& boneData : getHandBoneData()) {
            if (isLeft != (boneData.boneName[0] == 'L')) {
                continue;
            }

            const int flatBoneIndex = boneToFlexIndex(boneData.boneName);
            if (flatBoneIndex < 0 || flatBoneIndex >= static_cast<int>(FINGER_BONE_COUNT)) {
                outTransforms = {};
                outEnabledMask = 0;
                return false;
            }

            const auto transform = buildPoseBoneLocalTransform(boneData, pose, inPowerArmor);
            if (!isFiniteTransform(transform)) {
                outTransforms = {};
                outEnabledMask = 0;
                return false;
            }

            outTransforms[flatBoneIndex] = transform;
            outEnabledMask = static_cast<std::uint16_t>(outEnabledMask | (1U << flatBoneIndex));
        }

        outEnabledMask = static_cast<std::uint16_t>(outEnabledMask & FULL_LOCAL_TRANSFORM_MASK);
        return outEnabledMask == FULL_LOCAL_TRANSFORM_MASK;
    }

    bool HandPose::mirrorFingerLocalTransforms(const bool sourceIsLeft, const std::array<RE::NiTransform, FINGER_BONE_COUNT>& sourceTransforms, const std::uint16_t sourceEnabledMask,
        std::array<RE::NiTransform, FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask) const
    {
        outTargetTransforms = {};
        outTargetEnabledMask = 0;
        if ((sourceEnabledMask & FULL_LOCAL_TRANSFORM_MASK) != FULL_LOCAL_TRANSFORM_MASK) {
            return false;
        }

        const char sourcePrefix = sourceIsLeft ? 'L' : 'R';
        const char targetPrefix = sourceIsLeft ? 'R' : 'L';
        for (const auto& boneData : getHandBoneData()) {
            if (!boneData.boneName || boneData.boneName[0] != sourcePrefix) {
                continue;
            }

            const std::string sourceBoneName{ boneData.boneName };
            const int flatBoneIndex = boneToFlexIndex(sourceBoneName);
            if (flatBoneIndex < 0 || flatBoneIndex >= static_cast<int>(FINGER_BONE_COUNT) || !isFiniteTransform(sourceTransforms[static_cast<std::size_t>(flatBoneIndex)])) {
                outTargetTransforms = {};
                outTargetEnabledMask = 0;
                return false;
            }

            std::string targetBoneName = sourceBoneName;
            targetBoneName[0] = targetPrefix;
            const auto openTarget = _handOpen.find(targetBoneName);
            if (openTarget == _handOpen.end()) {
                outTargetTransforms = {};
                outTargetEnabledMask = 0;
                return false;
            }

            const auto& animatedSource = sourceTransforms[static_cast<std::size_t>(flatBoneIndex)];
            RE::NiTransform mirroredTarget = openTarget->second;
            RE::NiMatrix3 thumbRotation{};
            if (sourceBoneName.ends_with("Arm_Finger11") && tryTransferMirroredThumbBase(sourceBoneName, targetBoneName, animatedSource.rotate, thumbRotation)) {
                mirroredTarget.rotate = thumbRotation;
            } else {
                float flex = 1.0f;
                float splay = 0.0f;
                measureAnimatedFlexSplay(sourceBoneName, animatedSource.rotate, flex, splay);
                mirroredTarget.rotate = blendBoneRotation(targetBoneName, flex, splay);
            }
            mirroredTarget.scale = 1.0f;
            if (!isFiniteTransform(mirroredTarget)) {
                outTargetTransforms = {};
                outTargetEnabledMask = 0;
                return false;
            }

            outTargetTransforms[static_cast<std::size_t>(flatBoneIndex)] = mirroredTarget;
            outTargetEnabledMask = static_cast<std::uint16_t>(outTargetEnabledMask | (1U << flatBoneIndex));
        }

        outTargetEnabledMask = static_cast<std::uint16_t>(outTargetEnabledMask & FULL_LOCAL_TRANSFORM_MASK);
        return outTargetEnabledMask == FULL_LOCAL_TRANSFORM_MASK;
    }

    /**
     * Force the Pip-Boy interaction hand into the pointing pose.
     */
    void HandPose::setPipboyHandPose()
    {
        setHandPoseOverrideIntr(g_config.leftHandedPipBoy, PIPBOY_HAND_POSE_TAG, getPointingPose(), FORCED_HAND_POSE_PRIORITY);
    }

    /**
     * Release the temporary Pip-Boy pointing pose.
     */
    void HandPose::disablePipboyHandPose()
    {
        clearHandPoseOverrideIntr(g_config.leftHandedPipBoy, PIPBOY_HAND_POSE_TAG);
    }

    /**
     * Reuse the pointing pose while config mode is active.
     */
    void HandPose::setConfigModeHandPose()
    {
        setHandPoseOverrideIntr(!isLeftHandedMode(), CONFIG_MODE_HAND_POSE_TAG, getPointingPose(), FORCED_HAND_POSE_PRIORITY);
    }

    /**
     * Release the config mode pointing pose.
     */
    void HandPose::disableConfigModePose()
    {
        clearHandPoseOverrideIntr(!isLeftHandedMode(), CONFIG_MODE_HAND_POSE_TAG);
    }

    /**
     * Toggle a pointing override on either the primary or offhand.
     */
    void HandPose::setForceHandPointingPose(const bool primaryHand, const bool forcePointing)
    {
        if (forcePointing) {
            setHandPoseOverrideIntr(primaryHand == isLeftHandedMode(), FORCE_POINTING_HAND_POSE_TAG, getPointingPose(), FORCED_HAND_POSE_PRIORITY);
        } else {
            clearHandPoseOverrideIntr(primaryHand == isLeftHandedMode(), FORCE_POINTING_HAND_POSE_TAG);
        }
    }

    /**
     * Toggle the authored offhand weapon support pose.
     */
    void HandPose::setOffhandGripHandPose(const bool toSet)
    {
        if (toSet) {
            setHandPoseOverrideIntr(!isLeftHandedMode(), OFFHAND_GRIP_HAND_POSE_TAG, getOffhandWeaponGripPose(), FORCED_HAND_POSE_PRIORITY);
        } else {
            clearHandPoseOverrideIntr(!isLeftHandedMode(), OFFHAND_GRIP_HAND_POSE_TAG);
        }
    }

    /**
     * Toggle the authored Attaboy interaction pose on the left hand.
     */
    void HandPose::setAttaboyHandPose(const bool toSet)
    {
        if (toSet) {
            setHandPoseOverrideIntr(true, ATTABOY_HAND_POSE_TAG, getAttaboyPose(), FORCED_HAND_POSE_PRIORITY);
        } else {
            clearHandPoseOverrideIntr(true, ATTABOY_HAND_POSE_TAG);
        }
    }

    /**
     * Update all tracked hand bones for the current frame.
     *
     * First, apply any pose-driven palm offset on the hand root so child finger bones inherit it.
     *
     * For each finger bone in the flattened bone tree, this selects the highest-priority
     * pose source in this order:
     * 1. primary weapon pose for the dominant hand while a weapon is drawn
     * 2. explicit per-hand override poses, including named poses and thumbs-up
     * 3. dynamic controller-driven curl based on touch state and grip axis
     *
     * After the target local rotation is chosen, the result is written back into the
     * flattened bone tree and the bone's world transform is refreshed so downstream code
     * sees a consistent hierarchy even when a ref node is not present.
     */
    void HandPose::onFrameUpdate(RE::NiNode* root, const float frameTime)
    {
        const auto leftHandSource = resolveHandPoseSource(true);
        const auto rightHandSource = resolveHandPoseSource(false);

        const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(root);
        applyPalmPose(rt, true, leftHandSource, _leftPalmBlend, frameTime);
        applyPalmPose(rt, false, rightHandSource, _rightPalmBlend, frameTime);

        for (auto pos = 0; pos < rt->numTransforms; pos++) {
            const auto& boneName = Skelly::getBoneName(pos);
            auto handBoneIt = _handBones.find(boneName);
            if (handBoneIt == _handBones.end()) {
                continue;
            }

            const bool leftHandBone = isLeftHandBone(boneName);
            const auto& source = leftHandBone ? leftHandSource : rightHandSource;
            if (source.kind == HandPoseSourceKind::PrimaryWeaponPose) {
                applyPrimaryWeaponHandPose(boneName, source);
            } else if (source.kind == HandPoseSourceKind::OverridePose) {
                applyOverrideHandPose(boneName, source.pose, source.overrideEntry, frameTime);
            } else {
                applyDynamicHandPose(boneName, frameTime);
            }

            rt->transforms[pos].local.rotate = handBoneIt->second.rotate;
            rt->transforms[pos].local.scale = handBoneIt->second.scale;
            if (source.kind == HandPoseSourceKind::OverridePose) {
                if (const auto* localTransform = getLocalTransformOverride(source.overrideEntry, boneName)) {
                    rt->transforms[pos].local.translate = localTransform->translate;
                } else {
                    rt->transforms[pos].local.translate = _handOpen.at(boneName).translate;
                }
            } else {
                rt->transforms[pos].local.translate = _handOpen.at(boneName).translate;
            }
            refreshFlattenedBoneTransform(rt, pos);
        }
    }

    /**
     * Resolve the active hand pose source for one hand for this frame.
     *
     * This is the single hand-level source selection used by both pose consumers:
     * 1. the palm prepass, which needs an authored pose before child finger bones are updated
     * 2. the per-finger loop, which needs to know whether to use weapon, override, or dynamic logic
     *
     * Source priority matches the existing runtime behavior:
     * 1. primary weapon pose for the dominant hand while a weapon is drawn
     * 2. explicit hand override, then implicit thumbs-up
     * 3. dynamic controller-driven curl
     *
     * The returned source may intentionally have `pose == nullptr` when the active source is the
     * right-handed primary weapon path. In that case, finger bones should copy the first-person hand
     * transform instead of using authored pose data, and the palm prepass should do nothing.
     */
    HandPose::HandPoseSource HandPose::resolveHandPoseSource(const bool isLeft)
    {
        const bool isPrimaryHand = isLeft == isLeftHandedMode();
        const bool shouldUseWeaponPoseForPrimaryHand =
            IsWeaponDrawn() && !isPrimaryWeaponPoseBlocked() && (isLeftHandedMode() || !g_frik.isPipboyOperatingWithFinger());

        if (shouldUseWeaponPoseForPrimaryHand && isLeftHandedMode() && isUnarmedWeaponEquipped()) {
            // Left-handed unarmed is a special authored fist case that applies to both hands.
            return HandPoseSource{ .kind = HandPoseSourceKind::PrimaryWeaponPose, .pose = &getFistPose() };
        }

        if (isLeft && !isLeftHandedMode() &&
            IsWeaponDrawn() && !g_frik.isPipboyOperatingWithFinger() &&
            Skeleton::isPrimaryWeaponNodeOwnershipBlocked() &&
            !isPrimaryWeaponPoseBlocked()) {
            return HandPoseSource{ .kind = HandPoseSourceKind::PrimaryWeaponPose, .pose = nullptr };
        }

        if (isPrimaryHand && shouldUseWeaponPoseForPrimaryHand) {
            // Right-handed mode can copy the first-person hand transform directly. Left-handed mode
            // cannot, so it uses the fixed authored weapon pose instead.
            return HandPoseSource{
                .kind = HandPoseSourceKind::PrimaryWeaponPose,
                .pose = isLeftHandedMode() ? &HandPose::getFixedPrimaryWeaponPose() : nullptr
            };
        }

        if (const auto* activeOverride = getActiveHandPoseOverride(isLeft)) {
            // Explicit mod/API override wins over gesture-driven posing.
            return HandPoseSource{ .kind = HandPoseSourceKind::OverridePose, .pose = &activeOverride->pose, .overrideEntry = activeOverride };
        }

        if (shouldUseThumbsUpPose(isLeft)) {
            // Thumbs-up is treated as an implicit authored override.
            return HandPoseSource{ .kind = HandPoseSourceKind::OverridePose, .pose = &getThumbsUpPose() };
        }

        // Otherwise, let controller input drive the fingers and leave the palm neutral.
        return HandPoseSource{};
    }

    /**
     * Blend and apply the authored palm offset from the active hand source to one hand root transform.
     */
    void HandPose::applyPalmPose(BSFlattenedBoneTree* const boneTree, const bool isLeft, const HandPoseSource& source, PalmBlendState& blendState, const float frameTime)
    {
        const int pos = boneTree->GetBoneIndex(isLeft ? "LArm_Hand" : "RArm_Hand");
        if (pos < 0) {
            return;
        }

        auto& transform = boneTree->transforms[pos];

        // The flattened tree locals/worlds are not rebuilt by Skeleton arm IK. Sync the hand root
        // from the live refNode every frame so child finger refreshes use the correct wrist basis
        // even when there is no additional palm offset applied.
        if (transform.refNode) {
            transform.local = transform.refNode->local;
            transform.world = transform.refNode->world;
        }

        const float targetPalmPitch = source.pose ? source.pose->palmPitch : 0.0f;
        const float targetPalmYaw = source.pose ? source.pose->palmYaw : 0.0f;
        blendPalmAxisToward(blendState.pitch, targetPalmPitch, frameTime);
        blendPalmAxisToward(blendState.yaw, targetPalmYaw, frameTime);

        if (blendState.pitch == 0.0f && blendState.yaw == 0.0f) {
            return;
        }

        // Apply the offset in the hand's local space so flexion/deviation follow the hand basis
        // instead of the parent forearm basis.
        const float deviationSign = isLeft ? -1.0f : 1.0f;
        transform.local.rotate = transform.local.rotate * MatrixUtils::getMatrixFromEulerAnglesDegrees(0.0f, deviationSign * blendState.yaw, blendState.pitch);
        refreshFlattenedBoneTransform(boneTree, pos);
    }

    /**
     * Apply the special primary-hand weapon pose for the current handedness mode.
     *
     * Right-handed: Copy the 1st-person bone position for the given hand bone.
     * Useful for different weapon holding hand poses.
     *
     * Left-handed: the 1st-person skeleton is not using the correct hand,
     * so use a fixed grip pose instead of copying the 1st-person weapon hand.
     */
    void HandPose::applyPrimaryWeaponHandPose(const std::string& boneName, const HandPoseSource& source)
    {
        if (source.pose) {
            auto& bone = _handBones[boneName];
            bone.rotate = getPoseBoneRotation(boneName, *source.pose);
            bone.scale = 1.0f;
        } else if (isLeftHandBone(boneName)) {
            const auto fpTree = getFirstPersonBoneTree();
            const std::string rightBoneName = "RArm_" + boneName.substr(5);
            const int pos = fpTree ? fpTree->GetBoneIndex(rightBoneName) : -1;
            if (pos < 0) {
                return;
            }
            const RE::NiTransform& animated = fpTree->transforms[pos].refNode
                ? fpTree->transforms[pos].refNode->local
                : fpTree->transforms[pos].local;
            auto& bone = _handBones[boneName];
            RE::NiMatrix3 thumbRotation;
            if (rightBoneName == "RArm_Finger11" &&
                tryTransferMirroredThumbBase(rightBoneName, boneName, animated.rotate, thumbRotation)) {
                bone.rotate = thumbRotation;
            } else {
                float flex = 1.0f;
                float splay = 0.0f;
                measureAnimatedFlexSplay(rightBoneName, animated.rotate, flex, splay);
                bone.rotate = blendBoneRotation(boneName, flex, splay);
            }
            bone.scale = 1.0f;
        } else {
            const auto fpTree = getFirstPersonBoneTree();
            const int pos = fpTree->GetBoneIndex(boneName);
            if (pos >= 0) {
                _handBones[boneName] = fpTree->transforms[pos].refNode
                    ? fpTree->transforms[pos].refNode->local
                    : fpTree->transforms[pos].local;
            }
        }
    }

    float HandPose::inverseBlendFlex(const std::string& boneName, const RE::NiMatrix3& animatedRotation) const
    {
        Quaternion qOpen, qClosed, qAnimated;
        qOpen.fromMatrix(_handOpen.at(boneName).rotate);
        qClosed.fromMatrix(_handClosed.at(boneName).rotate);
        qAnimated.fromMatrix(animatedRotation);

        const auto relativeFrom = [](const Quaternion& from, Quaternion to) {
            if (from.dot(to) < 0.0f) {
                to *= -1.0f;
            }
            return from.conjugate() * to;
        };
        const auto axisAngle = [](const Quaternion& q, RE::NiPoint3& axis) {
            const float w = std::clamp(q.w, -1.0f, 1.0f);
            const float sinHalf = std::sqrt((std::max)(0.0f, 1.0f - w * w));
            if (sinHalf < 1e-6f) {
                axis = RE::NiPoint3(0, 0, 0);
                return 0.0f;
            }
            axis = RE::NiPoint3(q.x / sinHalf, q.y / sinHalf, q.z / sinHalf);
            return 2.0f * std::acos(w);
        };

        RE::NiPoint3 arcAxis;
        const float arcAngle = axisAngle(relativeFrom(qClosed, qOpen), arcAxis);
        constexpr float kMinArcAngle = 0.001f;
        if (arcAngle < kMinArcAngle) {
            return 1.0f;
        }

        RE::NiPoint3 animatedAxis;
        const float animatedAngle = axisAngle(relativeFrom(qClosed, qAnimated), animatedAxis);
        const float angleAlongArc = animatedAngle * MatrixUtils::vec3Dot(animatedAxis, arcAxis);
        return std::clamp(angleAlongArc / arcAngle, -1.0f, 2.0f);
    }

    void HandPose::measureAnimatedFlexSplay(
        const std::string& sourceBoneName,
        const RE::NiMatrix3& animatedRotation,
        float& outFlex,
        float& outSplay) const
    {
        outFlex = inverseBlendFlex(sourceBoneName, animatedRotation);
        outSplay = 0.0f;
        if (sourceBoneName.back() != '1') {
            return;
        }

        const RE::NiMatrix3 flexOnly = blendBoneRotation(sourceBoneName, outFlex, 0.0f);
        const RE::NiMatrix3 residual = animatedRotation * flexOnly.Transpose();
        Quaternion residualQ;
        residualQ.fromMatrix(residual);
        if (residualQ.w < 0.0f) {
            residualQ *= -1.0f;
        }
        const float w = std::clamp(residualQ.w, -1.0f, 1.0f);
        const float sinHalf = std::sqrt((std::max)(0.0f, 1.0f - w * w));
        if (sinHalf < 1e-6f) {
            return;
        }
        const float residualAngle = 2.0f * std::acos(w);
        const float splayCandidate = residualAngle * (residualQ.y / sinHalf);
        constexpr float kMaxAuthoredSplayRadians = 0.6f;
        if (!std::isfinite(splayCandidate) || std::fabs(splayCandidate) > kMaxAuthoredSplayRadians) {
            return;
        }
        const bool sourceIsLeft = isLeftHandBone(sourceBoneName);
        outSplay = sourceIsLeft ? -splayCandidate : splayCandidate;

        const RE::NiMatrix3 desplayed =
            MatrixUtils::getMatrixFromEulerAngles(0, sourceIsLeft ? outSplay : -outSplay, 0) * animatedRotation;
        outFlex = inverseBlendFlex(sourceBoneName, desplayed);
    }

    bool HandPose::tryTransferMirroredThumbBase(
        const std::string& sourceBoneName,
        const std::string& targetBoneName,
        const RE::NiMatrix3& animatedRotation,
        RE::NiMatrix3& outRotation) const
    {
        const RE::NiMatrix3& closedSource = _handClosed.at(sourceBoneName).rotate;
        const RE::NiMatrix3& openSource = _handOpen.at(sourceBoneName).rotate;
        const RE::NiMatrix3& closedTarget = _handClosed.at(targetBoneName).rotate;
        const RE::NiMatrix3& openTarget = _handOpen.at(targetBoneName).rotate;

        const auto parentDeltaAxis = [](const RE::NiMatrix3& open, const RE::NiMatrix3& closed, RE::NiPoint3& outAxis) {
            Quaternion q;
            q.fromMatrix(open * closed.Transpose());
            if (q.w < 0.0f) {
                q *= -1.0f;
            }
            const float w = std::clamp(q.w, -1.0f, 1.0f);
            const float sinHalf = std::sqrt((std::max)(0.0f, 1.0f - w * w));
            if (sinHalf < 1e-4f) {
                return false;
            }
            outAxis = RE::NiPoint3(q.x / sinHalf, q.y / sinHalf, q.z / sinHalf);
            return true;
        };
        RE::NiPoint3 sourceArcAxis{}, targetArcAxis{};
        if (!parentDeltaAxis(openSource, closedSource, sourceArcAxis) || !parentDeltaAxis(openTarget, closedTarget, targetArcAxis)) {
            return false;
        }

        const RE::NiPoint3 y{ 0.0f, 1.0f, 0.0f };
        const RE::NiPoint3 mirroredTargetArc{ -targetArcAxis.x, -targetArcAxis.y, -targetArcAxis.z };
        RE::NiPoint3 sourcePerpendicular{ sourceArcAxis.x, 0.0f, sourceArcAxis.z };
        RE::NiPoint3 targetPerpendicular{ mirroredTargetArc.x, 0.0f, mirroredTargetArc.z };
        constexpr float kMinPerpComponent = 0.05f;
        if (MatrixUtils::vec3Len(sourcePerpendicular) < kMinPerpComponent || MatrixUtils::vec3Len(targetPerpendicular) < kMinPerpComponent) {
            return false;
        }
        sourcePerpendicular = MatrixUtils::vec3Norm(sourcePerpendicular);
        targetPerpendicular = MatrixUtils::vec3Norm(targetPerpendicular);
        const RE::NiPoint3 sourceCross = MatrixUtils::vec3Cross(y, sourcePerpendicular);
        const RE::NiPoint3 targetCrossFlipped = MatrixUtils::vec3Cross(y, targetPerpendicular) * -1.0f;

        RE::NiMatrix3 sourceBasis, targetBasis;
        for (int row = 0; row < 3; ++row) {
            const float* yv = &y.x;
            sourceBasis.entry[row][0] = yv[row];
            sourceBasis.entry[row][1] = (&sourcePerpendicular.x)[row];
            sourceBasis.entry[row][2] = (&sourceCross.x)[row];
            targetBasis.entry[row][0] = yv[row];
            targetBasis.entry[row][1] = (&targetPerpendicular.x)[row];
            targetBasis.entry[row][2] = (&targetCrossFlipped.x)[row];
        }
        const RE::NiMatrix3 mirrorMap = targetBasis * sourceBasis.Transpose();

        Quaternion deltaQ;
        deltaQ.fromMatrix(animatedRotation * closedSource.Transpose());
        if (deltaQ.w < 0.0f) {
            deltaQ *= -1.0f;
        }
        const RE::NiPoint3 v{ deltaQ.x, deltaQ.y, deltaQ.z };
        const RE::NiPoint3 mv = mirrorMap * v;
        Quaternion mirroredDelta{ -mv.x, -mv.y, -mv.z, deltaQ.w };
        mirroredDelta.normalize();
        outRotation = mirroredDelta.getMatrix() * closedTarget;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                if (!std::isfinite(outRotation.entry[row][col])) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Apply hand pose by the current active hand pose override from external source.
     */
    void HandPose::applyOverrideHandPose(
        const std::string& boneName,
        const HandFingersPose* const activePose,
        const TaggedHandPoseOverride* const activeOverride,
        const float frameTime)
    {
        if (const auto* localTransform = getLocalTransformOverride(activeOverride, boneName)) {
            blendBoneTowardTransform(boneName, *localTransform, frameTime);
            return;
        }

        auto& bone = _handBones.at(boneName);
        bone.translate = _handOpen.at(boneName).translate;
        bone.scale = 1.0f;
        const auto targetRotation = getPoseBoneRotation(boneName, *activePose);
        blendBoneTowardRotation(boneName, targetRotation, frameTime);
    }

    /**
     * Apply hand pose by what controller buttons are touched/pressed.
     */
    void HandPose::applyDynamicHandPose(const std::string& boneName, const float frameTime)
    {
        float flex = 1.0F; // open
        const auto boneHand = isLeftHandBone(boneName) ? Hand::Left : Hand::Right;
        const auto controllerButtonForBone = getTrackedButton(boneName);
        const bool rightTriggerIdentityRemapped =
            boneHand == Hand::Right &&
            controllerButtonForBone == k_EButton_SteamVR_Trigger &&
            !isLeftHandedMode() &&
            Skeleton::isPrimaryWeaponNodeOwnershipBlocked();
        if (rightTriggerIdentityRemapped) {
            flex = 1.0F;
        } else if (controllerButtonForBone == k_EButton_Grip) {
            flex = 1.0f - VRControllers.getAxisValue(boneHand, Axis::Grip).x;
        } else if (controllerButtonForBone == k_EButton_SteamVR_Trigger) {
            flex = 1.0f - 2 * VRControllers.getAxisValue(boneHand, Axis::Trigger).x;
        } else if (VRControllers.isTouching(boneHand, controllerButtonForBone)) {
            flex = 0.0F;
        }

        auto& bone = _handBones.at(boneName);
        bone.translate = _handOpen.at(boneName).translate;
        bone.scale = 1.0f;
        blendBoneTowardRotation(boneName, blendBoneRotation(boneName, fmax(0.0f, fmin(1.0f, flex)), 0), frameTime);
    }

    /**
     * Resolve the target rotation for one bone from a full hand pose definition.
     */
    RE::NiMatrix3 HandPose::getPoseBoneRotation(const std::string& boneName, const HandFingersPose& pose) const
    {
        const int fingerIndex = boneToFingerIndex(boneName);
        const int boneToFlexIndex = fingerIndex * 3 + (boneName.back() - '1');
        const float flex = std::clamp(pose.getFlexAt(boneToFlexIndex), -1.0f, 2.0f);
        const float splay = boneName.back() == '1' ? pose.getFingerAt(fingerIndex).splay : 0.0f;
        return blendBoneRotation(boneName, flex, splay);
    }

    /**
     * Smoothly blend a runtime bone toward a target rotation for this frame.
     */
    void HandPose::blendBoneTowardRotation(const std::string& boneName, const RE::NiMatrix3& targetRotation, const float frameTime)
    {
        Quaternion qc, qt;
        auto& currentBone = _handBones.at(boneName);
        qc.fromMatrix(currentBone.rotate);
        qt.fromMatrix(targetRotation);
        const float blend = std::clamp(frameTime * 7, -1.0f, 2.0f);
        qc.slerp(blend, qt);
        currentBone.rotate = qc.getMatrix();
    }

    void HandPose::blendBoneTowardTransform(const std::string& boneName, const RE::NiTransform& targetTransform, const float frameTime)
    {
        blendBoneTowardRotation(boneName, targetTransform.rotate, frameTime);

        auto& currentBone = _handBones.at(boneName);
        currentBone.translate = targetTransform.translate;
        currentBone.scale = targetTransform.scale;
    }

    /**
     * Blend between the authored closed/open rotations and optionally apply proximal splay.
     */
    RE::NiMatrix3 HandPose::blendBoneRotation(const std::string& boneName, const float flex, const float splay) const
    {
        Quaternion qOpen, qClosed;
        qOpen.fromMatrix(_handOpen.at(boneName).rotate);
        qClosed.fromMatrix(_handClosed.at(boneName).rotate);
        qClosed.slerp(flex, qOpen);
        if (splay == 0.0f) {
            return qClosed.getMatrix();
        }
        const float sign = isLeftHandBone(boneName) ? -1.0f : 1.0f;
        return MatrixUtils::getMatrixFromEulerAngles(0, sign * splay, 0) * qClosed.getMatrix();
    }

    const RE::NiTransform* HandPose::getLocalTransformOverride(const TaggedHandPoseOverride* const activeOverride, const std::string& boneName)
    {
        if (!activeOverride) {
            return nullptr;
        }

        const int flatBoneIndex = boneToFlexIndex(boneName);
        if (flatBoneIndex < 0 || flatBoneIndex >= static_cast<int>(FINGER_BONE_COUNT)) {
            return nullptr;
        }

        const std::uint16_t bit = static_cast<std::uint16_t>(1U << flatBoneIndex);
        if ((activeOverride->localTransformMask & bit) == 0) {
            return nullptr;
        }

        const auto& localTransform = activeOverride->localTransforms[flatBoneIndex];
        return isFiniteTransform(localTransform) ? &localTransform : nullptr;
    }

    /**
     * Return the current override list for one hand, ordered from oldest to newest.
     */
    std::vector<HandPose::TaggedHandPoseOverride>& HandPose::getHandOverrides(const bool isLeft)
    {
        return isLeft ? _leftHandOverrides : _rightHandOverrides;
    }

    /**
     * Return the newest explicit override for one hand, if any.
     */
    const HandPose::TaggedHandPoseOverride* HandPose::getActiveHandPoseOverride(const bool isLeft)
    {
        const auto& overrides = getHandOverrides(isLeft);
        return overrides.empty() ? nullptr : &overrides.front();
    }

    /**
     * Detect the controller gesture that should temporarily map to thumbs-up.
     */
    bool HandPose::shouldUseThumbsUpPose(const bool isLeft)
    {
        const auto hand = isLeft ? Hand::Left : Hand::Right;
        return VRControllers.isTouching(hand, k_EButton_Grip)
            && VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Trigger)
            && !VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Touchpad);
    }

    /**
     * Set, update, or promote one tagged explicit hand pose override.
     */
    void HandPose::setHandPoseOverrideIntr(const bool isLeft, const std::string_view tag, const HandFingersPose& pose, const int priority)
    {
        if (tag.empty()) {
            return;
        }

        auto& overrides = getHandOverrides(isLeft);
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) { return overrideEntry.tag == tag; });
        const bool wasInserted = overrideIt == overrides.end();

        TaggedHandPoseOverride updatedOverride = wasInserted ? TaggedHandPoseOverride{} : *overrideIt;
        updatedOverride.tag = std::string(tag);
        updatedOverride.pose = pose;
        updatedOverride.priority = priority;
        updatedOverride.sequence = ++_nextOverrideSequence;
        updatedOverride.localTransformMask = 0;
        updatedOverride.localTransforms = {};

        if (wasInserted) {
            overrides.push_back(updatedOverride);
        } else {
            *overrideIt = updatedOverride;
        }

        std::ranges::sort(overrides, [](const TaggedHandPoseOverride& lhs, const TaggedHandPoseOverride& rhs) {
            if (lhs.priority != rhs.priority) {
                return lhs.priority > rhs.priority;
            }
            return lhs.sequence > rhs.sequence;
        });
    }

    /**
     * Clear one tagged explicit hand pose override.
     */
    void HandPose::clearHandPoseOverrideIntr(const bool isLeft, const std::string_view tag)
    {
        if (tag.empty()) {
            return;
        }

        auto& overrides = getHandOverrides(isLeft);
        const auto overrideIt = std::ranges::find_if(overrides, [tag](const TaggedHandPoseOverride& overrideEntry) { return overrideEntry.tag == tag; });
        if (overrideIt == overrides.end()) {
            return;
        }

        overrides.erase(overrideIt);
    }
}
