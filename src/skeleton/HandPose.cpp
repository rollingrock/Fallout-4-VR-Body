#include "HandPose.h"

#include <algorithm>
#include <string>

#include "Config.h"
#include "FRIK.h"
#include "HandPoseData.h"
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
     * Return the fixed authored weapon pose used when the runtime cannot copy the first-person hand.
     * Can be used to return specific pose for specific weapons.
     */
    const frik::HandFingersPose& getFixedPrimaryWeaponPose()
    {
        return isUnarmedWeaponEquipped()
            ? getFistPose()
            : (frik::g_frik.isMeleeWeaponDrawn() ? getMeleeGripPose() : getGunGripPose());
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
}

// TODO: this code is terrible, primarily it doesn't handle multiple code paths set hand pose, release will release all of them
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
    void HandPose::setFingerPose(const bool isLeft, const HandFingersPose& pose)
    {
        auto& overrideState = getHandOverrideState(isLeft);
        overrideState.pose = pose;
        overrideState.active = true;
    }

    /**
     * Release any explicit override and return control to runtime hand logic.
     */
    void HandPose::restoreFingerPoseControl(const bool isLeft)
    {
        logger::debug("Hand pose: Restore control for {} hand", isLeft ? "Left" : "Right");
        getHandOverrideState(isLeft) = HandOverrideState{};
    }

    /**
     * Force the Pip-Boy interaction hand into the pointing pose.
     */
    void HandPose::setPipboyHandPose()
    {
        setHandPoseOverride(true, g_config.leftHandedPipBoy, getPointingPose());
    }

    /**
     * Release the temporary Pip-Boy pointing pose.
     */
    void HandPose::disablePipboyHandPose()
    {
        setHandPoseOverride(false, g_config.leftHandedPipBoy, getPointingPose());
    }

    /**
     * Reuse the pointing pose while config mode is active.
     */
    void HandPose::setConfigModeHandPose()
    {
        setForceHandPointingPose(false, true);
    }

    /**
     * Release the config mode pointing pose.
     */
    void HandPose::disableConfigModePose()
    {
        setForceHandPointingPose(false, false);
    }

    /**
     * Toggle a pointing override on either the primary or offhand.
     */
    void HandPose::setForceHandPointingPose(const bool primaryHand, const bool forcePointing)
    {
        setHandPoseOverride(forcePointing, primaryHand == isLeftHandedMode(), getPointingPose());
    }

    /**
     * Toggle the authored offhand weapon support pose.
     */
    void HandPose::setOffhandGripHandPose(const bool toSet)
    {
        setHandPoseOverride(toSet, !isLeftHandedMode(), getOffhandWeaponGripPose());
    }

    /**
     * Toggle the authored Attaboy interaction pose on the left hand.
     */
    void HandPose::setAttaboyHandPose(const bool toSet)
    {
        setHandPoseOverride(toSet, true, getAttaboyPose());
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
                applyOverrideHandPose(boneName, source.pose, frameTime);
            } else {
                applyDynamicHandPose(boneName, frameTime);
            }

            rt->transforms[pos].local.rotate = handBoneIt->second.rotate;
            rt->transforms[pos].local.translate = _handOpen.at(boneName).translate;
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
        const bool shouldUseWeaponPoseForPrimaryHand = IsWeaponDrawn() && (isLeftHandedMode() || !g_frik.isPipboyOperatingWithFinger());

        if (shouldUseWeaponPoseForPrimaryHand && isLeftHandedMode() && isUnarmedWeaponEquipped()) {
            // Left-handed unarmed is a special authored fist case that applies to both hands.
            return HandPoseSource{ .kind = HandPoseSourceKind::PrimaryWeaponPose, .pose = &getFistPose() };
        }

        const bool isPrimaryHand = isLeft == isLeftHandedMode();
        if (isPrimaryHand && shouldUseWeaponPoseForPrimaryHand) {
            // Right-handed mode can copy the first-person hand transform directly. Left-handed mode
            // cannot, so it uses the fixed authored weapon pose instead.
            return HandPoseSource{
                .kind = HandPoseSourceKind::PrimaryWeaponPose,
                .pose = isLeftHandedMode() ? &getFixedPrimaryWeaponPose() : nullptr
            };
        }

        const auto& overrideState = getHandOverrideState(isLeft);
        if (overrideState.active) {
            // Explicit mod/API override wins over gesture-driven posing.
            return HandPoseSource{ .kind = HandPoseSourceKind::OverridePose, .pose = &overrideState.pose };
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
            _handBones[boneName].rotate = getPoseBoneRotation(boneName, *source.pose);
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

    /**
     * Apply hand pose by the current active hand pose override from external source.
     */
    void HandPose::applyOverrideHandPose(const std::string& boneName, const HandFingersPose* const activePose, const float frameTime)
    {
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
        if (controllerButtonForBone == k_EButton_Grip) {
            flex = 1.0f - VRControllers.getAxisValue(boneHand, Axis::Grip).x;
        } else if (controllerButtonForBone == k_EButton_SteamVR_Trigger) {
            flex = 1.0f - 2 * VRControllers.getAxisValue(boneHand, Axis::Trigger).x;
        } else if (VRControllers.isTouching(boneHand, controllerButtonForBone)) {
            flex = 0.0F;
        }

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

    /**
     * Return the mutable override state bucket for one hand.
     */
    HandOverrideState& HandPose::getHandOverrideState(const bool isLeft)
    {
        return isLeft ? _leftHandOverride : _rightHandOverride;
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
     * Toggle one of the named authored hand pose overrides.
     */
    void HandPose::setHandPoseOverride(const bool setActive, const bool isLeft, const HandFingersPose& pose)
    {
        auto& overrideState = getHandOverrideState(isLeft);
        if (overrideState.active == setActive) {
            return;
        }
        logger::debug("Hand pose: Set force hand pose for '{}' hand: {})", isLeft ? "Left" : "Right", setActive ? "Set" : "Release");
        overrideState.active = setActive;
        overrideState.pose = pose;
    }
}
