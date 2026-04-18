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
        const bool leftHandedMode = isLeftHandedMode();
        const bool shouldUseWeaponPoseForPrimaryHand = IsWeaponDrawn() && (leftHandedMode || !g_frik.isPipboyOperatingWithFinger());
        const bool shouldUseFistPoseForBothHands = shouldUseWeaponPoseForPrimaryHand && leftHandedMode && isUnarmedWeaponEquipped();
        const auto leftHandOverridePose = tryGetHandOverridePose(true);
        const auto rightHandOverridePose = tryGetHandOverridePose(false);

        const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(root);
        for (auto pos = 0; pos < rt->numTransforms; pos++) {
            const auto& boneName = Skelly::getBoneName(pos);
            auto handBoneIt = _handBones.find(boneName);
            if (handBoneIt == _handBones.end()) {
                continue;
            }

            const bool leftHandBone = isLeftHandBone(boneName);
            if (shouldUseFistPoseForBothHands || (leftHandBone == leftHandedMode && shouldUseWeaponPoseForPrimaryHand)) {
                applyPrimaryWeaponHandPose(boneName);
            } else if (const auto activePose = leftHandBone ? leftHandOverridePose : rightHandOverridePose) {
                applyOverrideHandPose(boneName, activePose, frameTime);
            } else {
                applyDynamicHandPose(boneName, frameTime);
            }

            rt->transforms[pos].local.rotate = handBoneIt->second.rotate;
            rt->transforms[pos].local.translate = _handOpen.at(boneName).translate;
            if (rt->transforms[pos].refNode) {
                rt->transforms[pos].refNode->local = rt->transforms[pos].local;
                rt->transforms[pos].world = rt->transforms[pos].refNode->world;
            } else {
                const auto parent = rt->transforms[pos].parPos;
                RE::NiPoint3 p = rt->transforms[pos].local.translate;
                p = rt->transforms[parent].world.rotate.Transpose() * (p * rt->transforms[parent].world.scale);
                rt->transforms[pos].world.translate = rt->transforms[parent].world.translate + p;
                rt->transforms[pos].world.rotate = rt->transforms[pos].local.rotate * rt->transforms[parent].world.rotate;
            }
        }
    }

    /**
     * Return the active override pose for a hand, including the implicit thumbs-up pose.
     */
    const HandFingersPose* HandPose::tryGetHandOverridePose(const bool isLeft)
    {
        const auto& overrideState = getHandOverrideState(isLeft);
        if (overrideState.active) {
            return &overrideState.pose;
        }
        if (shouldUseThumbsUpPose(isLeft)) {
            return &getThumbsUpPose();
        }
        return nullptr;
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
    void HandPose::applyPrimaryWeaponHandPose(const std::string& boneName)
    {
        if (isLeftHandedMode()) {
            const auto& pose = isUnarmedWeaponEquipped()
                ? getFistPose()
                : (g_frik.isMeleeWeaponDrawn() ? getMeleeGripPose() : getGunGripPose());
            _handBones[boneName].rotate = getPoseBoneRotation(boneName, pose);
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
