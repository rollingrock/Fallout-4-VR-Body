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
    void copyRotationIntoTransform(const RotationData& rotationData, RE::NiTransform& transform)
    {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                transform.rotate.entry[row][col] = rotationData[row * 4 + col];
            }
        }
    }

    int boneToFingerIndex(const std::string& bone)
    {
        return bone[bone.size() - 2] - '1';
    }

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

    FingerPose& HandFingersPose::getFingerAt(const int fingerIndex) noexcept
    {
        FingerPose* const fingers[5] = { &thumb, &index, &middle, &ring, &pinky };
        return *fingers[fingerIndex];
    }

    const FingerPose& HandFingersPose::getFingerAt(const int fingerIndex) const noexcept
    {
        const FingerPose* const fingers[5] = { &thumb, &index, &middle, &ring, &pinky };
        return *fingers[fingerIndex];
    }

    // Get flex by flat bone index (0-14):
    //   0-2: thumb prox/mid/dist, 3-5: index, 6-8: middle, 9-11: ring, 12-14: pinky
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

    void HandPose::setFingerPose(const bool isLeft, const HandFingersPose& pose)
    {
        auto& overrideState = getHandOverrideState(isLeft);
        overrideState.pose = pose;
        overrideState.active = true;
    }

    void HandPose::restoreFingerPoseControl(const bool isLeft)
    {
        logger::debug("Hand pose: Restore control for {} hand", isLeft ? "Left" : "Right");
        getHandOverrideState(isLeft) = HandOverrideState{};
    }

    void HandPose::setPipboyHandPose()
    {
        setHandPoseOverride(true, g_config.leftHandedPipBoy, getPointingPose());
    }

    void HandPose::disablePipboyHandPose()
    {
        setHandPoseOverride(false, g_config.leftHandedPipBoy, getPointingPose());
    }

    void HandPose::setConfigModeHandPose()
    {
        setForceHandPointingPose(false, true);
    }

    void HandPose::disableConfigModePose()
    {
        setForceHandPointingPose(false, false);
    }

    void HandPose::setForceHandPointingPose(const bool primaryHand, const bool forcePointing)
    {
        setHandPoseOverride(forcePointing, primaryHand == isLeftHandedMode(), getPointingPose());
    }

    void HandPose::setOffhandGripHandPose(const bool toSet)
    {
        setHandPoseOverride(toSet, !isLeftHandedMode(), getOffhandWeaponGripPose());
    }

    void HandPose::setAttaboyHandPose(const bool toSet)
    {
        setHandPoseOverride(toSet, true, getAttaboyPose());
    }

    /**
     * TODO: doc
     */
    void HandPose::onFrameUpdate(RE::NiNode* root, const float frameTime)
    {
        const bool shouldUseWeaponPoseForPrimaryHand = IsWeaponDrawn() && (isLeftHandedMode() || !g_frik.isPipboyOperatingWithFinger());
        const HandFingersPose* leftActivePose = tryGetActiveHandPose(true);
        const HandFingersPose* rightActivePose = tryGetActiveHandPose(false);

        const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(root);
        for (auto pos = 0; pos < rt->numTransforms; pos++) {
            const auto& boneName = Skelly::getBoneName(pos);
            auto handBoneIt = _handBones.find(boneName);
            if (handBoneIt == _handBones.end()) {
                continue;
            }

            const bool isLeft = boneName[0] == 'L';
            const bool isPrimaryHandBone = isLeft == isLeftHandedMode();

            if (isPrimaryHandBone && shouldUseWeaponPoseForPrimaryHand) {
                applyPrimaryWeaponHandPose(boneName);
            } else {
                const auto targetRotation = resolveDynamicBoneRotation(boneName, isLeft, isLeft ? leftActivePose : rightActivePose);
                blendBoneTowardRotation(boneName, targetRotation, frameTime);
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

    const HandFingersPose* HandPose::tryGetActiveHandPose(const bool isLeft)
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
     * Right-handed: Copy the 1st-person bone position for the given hand bone.
     * Useful for different weapon holding hand poses.
     *
     * Left-handed: the 1st-person skeleton is not using the correct hand,
     * so use a fixed grip pose instead of copying the 1st-person weapon hand.
     */
    void HandPose::applyPrimaryWeaponHandPose(const std::string& boneName)
    {
        if (isLeftHandedMode()) {
            const auto& pose = g_frik.isMeleeWeaponDrawn() ? getMeleeGripPose() : getGunGripPose();
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

    RE::NiMatrix3 HandPose::resolveDynamicBoneRotation(const std::string& boneName, const bool isLeft, const HandFingersPose* activePose) const
    {
        if (activePose) {
            return getPoseBoneRotation(boneName, *activePose);
        }
        if (VRControllers.isTouching(isLeft ? Hand::Left : Hand::Right, getTrackedButton(boneName))) {
            return blendBoneRotation(boneName, 0.0f);
        }
        return blendBoneRotation(boneName, getBoneFlex(boneName, isLeft));
    }

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

    RE::NiMatrix3 HandPose::getPoseBoneRotation(const std::string& boneName, const HandFingersPose& pose) const
    {
        const bool isLeft = boneName[0] == 'L';
        const int fingerIndex = boneToFingerIndex(boneName);
        const int boneToFlexIndex = fingerIndex * 3 + (boneName.back() - '1');
        const float flex = std::clamp(pose.getFlexAt(boneToFlexIndex), -1.0f, 2.0f);
        const float splay = boneName.back() == '1' ? pose.getFingerAt(fingerIndex).splay : 0.0f;
        return blendBoneRotation(boneName, flex, splay, isLeft);
    }

    RE::NiMatrix3 HandPose::blendBoneRotation(const std::string& boneName, const float flex) const
    {
        Quaternion qOpen, qClosed;
        qOpen.fromMatrix(_handOpen.at(boneName).rotate);
        qClosed.fromMatrix(_handClosed.at(boneName).rotate);
        qClosed.slerp(flex, qOpen);
        return qClosed.getMatrix();
    }

    RE::NiMatrix3 HandPose::blendBoneRotation(const std::string& boneName, const float flex, const float splay, const bool isLeft) const
    {
        RE::NiMatrix3 result = blendBoneRotation(boneName, flex);
        if (splay != 0.0f) {
            const float sign = isLeft ? -1.0f : 1.0f;
            result = MatrixUtils::getMatrixFromEulerAngles(0, sign * splay, 0) * result;
        }
        return result;
    }

    HandOverrideState& HandPose::getHandOverrideState(const bool isLeft)
    {
        return isLeft ? _leftHandOverride : _rightHandOverride;
    }

    bool HandPose::shouldUseThumbsUpPose(const bool isLeft)
    {
        const auto hand = isLeft ? Hand::Left : Hand::Right;
        return VRControllers.isTouching(hand, k_EButton_Grip)
            && VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Trigger)
            && !VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Touchpad);
    }

    float HandPose::getBoneFlex(const std::string& bone, const bool isLeft)
    {
        if (getTrackedButton(bone) != k_EButton_Grip) {
            return 1.0f;
        }
        const auto hand = isLeft ? Hand::Left : Hand::Right;
        return 1.0f - VRControllers.getAxisValue(hand, Axis::Grip).x;
    }

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
