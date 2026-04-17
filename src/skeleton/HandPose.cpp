#include "HandPose.h"

#include <algorithm>
#include <map>
#include <numbers>
#include <string>

#include "Config.h"
#include "FRIK.h"
#include "common/MatrixUtils.h"
#include "common/Quaternion.h"
#include "f4vr/BSFlattenedBoneTree.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "vrcf/VRControllersManager.h"

using namespace common;
using namespace f4vr;
using namespace vrcf;

namespace
{
    constexpr int FINGERS_COUNT = 15;
    constexpr int FINGER_COUNT = 5;

    const std::string LEFT_HAND_FINGERS[] = {
        "LArm_Finger11", "LArm_Finger12", "LArm_Finger13",
        "LArm_Finger21", "LArm_Finger22", "LArm_Finger23",
        "LArm_Finger31", "LArm_Finger32", "LArm_Finger33",
        "LArm_Finger41", "LArm_Finger42", "LArm_Finger43",
        "LArm_Finger51", "LArm_Finger52", "LArm_Finger53",
    };
    const std::string RIGHT_HAND_FINGERS[] = {
        "RArm_Finger11", "RArm_Finger12", "RArm_Finger13",
        "RArm_Finger21", "RArm_Finger22", "RArm_Finger23",
        "RArm_Finger31", "RArm_Finger32", "RArm_Finger33",
        "RArm_Finger41", "RArm_Finger42", "RArm_Finger43",
        "RArm_Finger51", "RArm_Finger52", "RArm_Finger53",
    };

    // Proximal (knuckle/MCP) bone of each finger — used for splay
    const std::string LEFT_PROXIMAL_BONES[] = {
        "LArm_Finger11", "LArm_Finger21", "LArm_Finger31", "LArm_Finger41", "LArm_Finger51",
    };
    const std::string RIGHT_PROXIMAL_BONES[] = {
        "RArm_Finger11", "RArm_Finger21", "RArm_Finger31", "RArm_Finger41", "RArm_Finger51",
    };

    // Predefined hand poses (flex values: 0.0 = bent/closed, 1.0 = straight/open)
    // clang-format off
    static constexpr frik::HandFingersPose GUN_GRIP_POSE = {
        .thumb  = { .prox=0.7f, .mid=0.4f, .dist=0.5f },
        .index  = { .prox=0.9f, .mid=0.6f, .dist=0.5f },
        .middle = { .prox=0.3f, .mid=0.5f, .dist=0.5f },
        .ring   = { .prox=0.1f, .mid=0.5f, .dist=0.5f },
        .pinky  = { .prox=0.0f, .mid=0.5f, .dist=0.7f },
    };
    static constexpr frik::HandFingersPose MELEE_GRIP_POSE = {
        .thumb  = { .prox=0.7f, .mid=0.5f, .dist=0.8f },
        .index  = { .prox=0.4f, .mid=0.3f, .dist=0.9f },
        .middle = { .prox=0.1f, .mid=0.5f, .dist=0.9f },
        .ring   = { .prox=0.0f, .mid=0.5f, .dist=0.9f },
        .pinky  = { .prox=0.0f, .mid=0.4f, .dist=0.9f },
    };
    static constexpr frik::HandFingersPose POINTING_POSE = {
        .thumb  = { .prox=0.0f, .mid=0.0f, .dist=0.0f },
        .index  = { .prox=1.0f, .mid=1.0f, .dist=1.0f },
        .middle = { .prox=0.0f, .mid=0.0f, .dist=0.0f },
        .ring   = { .prox=0.0f, .mid=0.0f, .dist=0.0f },
        .pinky  = { .prox=0.0f, .mid=0.0f, .dist=0.0f },
    };
    static constexpr frik::HandFingersPose ATTABOY_POSE = {
        .thumb  = { .prox=0.7f, .mid=1.2f, .dist=1.3f },
        .index  = { .prox=1.1f, .mid=1.1f, .dist=1.2f },
        .middle = { .prox=0.8f, .mid=0.6f, .dist=1.0f },
        .ring   = { .prox=0.4f, .mid=0.8f, .dist=1.0f },
        .pinky  = { .prox=0.1f, .mid=1.0f, .dist=1.4f },
    };
    static constexpr frik::HandFingersPose OFFHAND_GRIP_POSE = {
        .thumb  = { .prox=1.0f, .mid=1.0f,  .dist=0.9f  },
        .index  = { .prox=0.6f, .mid=0.6f,  .dist=0.6f  },
        .middle = { .prox=0.5f, .mid=0.6f,  .dist=0.55f },
        .ring   = { .prox=0.5f, .mid=0.5f,  .dist=0.5f  },
        .pinky  = { .prox=0.5f, .mid=0.5f,  .dist=0.5f  },
    };
    // clang-format on

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

    // Get flex by flat bone index (0-14):
    //   0-2: thumb prox/mid/dist, 3-5: index, 6-8: middle, 9-11: ring, 12-14: pinky
    float HandFingersPose::getFlexAt(const int boneIndex) const noexcept
    {
        const FingerPose* const fingers[5] = { &thumb, &index, &middle, &ring, &pinky };
        const FingerPose& f = *fingers[boneIndex / 3];
        switch (boneIndex % 3) {
        case 0:
            return f.prox;
        case 1:
            return f.mid;
        default:
            return f.dist;
        }
    }

    // Get splay by finger index (0=thumb, 1=index, 2=middle, 3=ring, 4=pinky)
    float HandFingersPose::getSplayAt(const int fingerIndex) const noexcept
    {
        const FingerPose* const fingers[5] = { &thumb, &index, &middle, &ring, &pinky };
        return fingers[fingerIndex]->splay;
    }

    // -- Lifecycle ----------------------------------------------------------------------

    HandPose::HandPose(const bool inPowerArmor)
    {
        initHandPoses(inPowerArmor);
        _handBones = _handOpen;
    }

    // -- Papyrus / API-driven pose overrides --------------------------------------------

    void HandPose::setFingerPositionScalar(const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        const HandFingersPose pose = {
            .thumb = { .prox = thumb, .mid = thumb, .dist = thumb },
            .index = { .prox = index, .mid = index, .dist = index },
            .middle = { .prox = middle, .mid = middle, .dist = middle },
            .ring = { .prox = ring, .mid = ring, .dist = ring },
            .pinky = { .prox = pinky, .mid = pinky, .dist = pinky },
        };
        const auto* const fingers = isLeft ? LEFT_HAND_FINGERS : RIGHT_HAND_FINGERS;
        for (auto i = 0; i < FINGERS_COUNT; i++) {
            _handPapyrusHasControl[fingers[i]] = true;
            _handPapyrusPose[fingers[i]] = pose.getFlexAt(i);
        }
    }

    void HandPose::setFingerSplayScalar(const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky)
    {
        const auto* const proximal = isLeft ? LEFT_PROXIMAL_BONES : RIGHT_PROXIMAL_BONES;
        const float values[FINGER_COUNT] = { thumb, index, middle, ring, pinky };
        for (auto i = 0; i < FINGER_COUNT; i++) {
            _handSplayPose[proximal[i]] = values[i];
        }
    }

    void HandPose::restoreFingerPoseControl(const bool isLeft)
    {
        logger::debug("Hand pose: Restore control for {} hand", isLeft ? "Left" : "Right");
        const auto* const fingers = isLeft ? LEFT_HAND_FINGERS : RIGHT_HAND_FINGERS;
        for (auto i = 0; i < FINGERS_COUNT; i++) {
            _handPapyrusHasControl[fingers[i]] = false;
        }
        const auto* const proximal = isLeft ? LEFT_PROXIMAL_BONES : RIGHT_PROXIMAL_BONES;
        for (auto i = 0; i < FINGER_COUNT; i++) {
            _handSplayPose.erase(proximal[i]);
        }
    }

    void HandPose::setPipboyHandPose()
    {
        setHandPoseOverride(true, !g_config.leftHandedPipBoy, POINTING_POSE);
    }

    void HandPose::disablePipboyHandPose()
    {
        setHandPoseOverride(false, !g_config.leftHandedPipBoy, POINTING_POSE);
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
        setHandPoseOverride(forcePointing, primaryHand ^ isLeftHandedMode(), POINTING_POSE);
    }

    void HandPose::setOffhandGripHandPose(const bool toSet)
    {
        setHandPoseOverride(toSet, isLeftHandedMode(), OFFHAND_GRIP_POSE);
    }

    void HandPose::setAttaboyHandPose(const bool toSet)
    {
        setHandPoseOverride(toSet, false, ATTABOY_POSE);
    }

    // -- Bone rotation blending ---------------------------------------------------------

    void HandPose::onFrameUpdate(RE::NiNode* root, const float frameTime)
    {
        const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(root);
        for (auto pos = 0; pos < rt->numTransforms; pos++) {
            auto boneName = Skelly::getBoneName(pos);
            auto handBone = _handBones.find(boneName);
            if (handBone != _handBones.end()) {
                const bool isLeft = boneName[0] == 'L';

                if (IsWeaponDrawn()
                    && (isLeftHandedMode() || !g_frik.isPipboyOperatingWithFinger())
                    && !(isLeft ^ isLeftHandedMode())) {
                    if (isLeftHandedMode()) {
                        // In left-handed mode the 1st-person skeleton is not using the correct hand so we can't use "copy1StPerson" method.
                        // Instead, we just force a specific hand pose that makes sense.
                        handBone->second.rotate = getGripBoneRotation(boneName, g_frik.isMeleeWeaponDrawn());
                    } else {
                        // use the game hand position for the weapon in hand
                        copy1StPerson(boneName);
                    }
                } else {
                    // use the forced hand position
                    calculateHandPose(boneName, isLeft, frameTime);
                }

                rt->transforms[pos].local.rotate = handBone->second.rotate;
                rt->transforms[pos].local.translate = _handOpen.at(boneName).translate;

                if (rt->transforms[pos].refNode) {
                    rt->transforms[pos].refNode->local = rt->transforms[pos].local;
                }
            }

            if (rt->transforms[pos].refNode) {
                rt->transforms[pos].world = rt->transforms[pos].refNode->world;
            } else {
                const short parent = rt->transforms[pos].parPos;
                RE::NiPoint3 p = rt->transforms[pos].local.translate;
                p = rt->transforms[parent].world.rotate.Transpose() * (p * rt->transforms[parent].world.scale);

                rt->transforms[pos].world.translate = rt->transforms[parent].world.translate + p;

                rt->transforms[pos].world.rotate = rt->transforms[pos].local.rotate * rt->transforms[parent].world.rotate;
            }
        }
    }

    void HandPose::calculateHandPose(const std::string& bone, const bool isLeft, const float frameTime)
    {
        Quaternion qc, qt;

        const auto hand = isLeft ? Hand::Left : Hand::Right;
        const float gripProx = VRControllers.getAxisValue(hand, Axis::Grip).x;
        const bool thumbUp = VRControllers.isTouching(hand, k_EButton_Grip)
            && VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Trigger)
            && !VRControllers.isTouching(hand, vr::k_EButton_SteamVR_Touchpad);
        const bool buttonTouched = [&]() {
            const uint64_t touched = isLeft
                ? VRControllers.getControllerState_DEPRECATED(TrackerType::Left).ulButtonTouched
                : VRControllers.getControllerState_DEPRECATED(TrackerType::Right).ulButtonTouched;
            return touched & ButtonMaskFromId(getTrackedButton(bone));
        }();

        RE::NiMatrix3 papyrusRot;
        if (tryGetPapyrusRotation(bone, isLeft, papyrusRot)) {
            qt.fromMatrix(papyrusRot);
        } else if (thumbUp && bone.find("Finger1") != std::string::npos) {
            qt.fromMatrix(getThumbsUpBoneRotation(bone, isLeft));
        } else if (buttonTouched) {
            qt.fromMatrix(blendBoneRotation(bone, 0.0f));
        } else {
            const float flex = getTrackedButton(bone) == k_EButton_Grip ? (1.0f - gripProx) : 1.0f;
            qt.fromMatrix(blendBoneRotation(bone, flex));
        }

        auto& currentBone = _handBones.at(bone);
        qc.fromMatrix(currentBone.rotate);
        const float blend = std::clamp(frameTime * 7, -1.0f, 2.0f);
        qc.slerp(blend, qt);
        currentBone.rotate = qc.getMatrix();
    }

    /**
     * Copy the 1st-person bone position for the given hand bone.
     * Useful for different weapons holding hand poses.
     */
    void HandPose::copy1StPerson(const std::string& bone)
    {
        const auto fpTree = getFirstPersonBoneTree();
        const int pos = fpTree->GetBoneIndex(bone);
        if (pos >= 0) {
            if (fpTree->transforms[pos].refNode) {
                _handBones[bone] = fpTree->transforms[pos].refNode->local;
            } else {
                _handBones[bone] = fpTree->transforms[pos].local;
            }
        }
    }

    bool HandPose::tryGetPapyrusRotation(const std::string& bone, const bool isLeft, RE::NiMatrix3& outRotation) const
    {
        if (!_handPapyrusHasControl[bone]) {
            return false;
        }
        const float flex = std::clamp(_handPapyrusPose[bone], -1.0f, 2.0f);
        const auto boneSplay = _handSplayPose.find(bone);
        outRotation = blendBoneRotation(bone, flex, boneSplay != _handSplayPose.end() ? boneSplay->second : 0.0f, isLeft);
        return true;
    }

    RE::NiMatrix3 HandPose::blendBoneRotation(const std::string& bone, const float flex) const
    {
        Quaternion qOpen, qClosed;
        qOpen.fromMatrix(_handOpen.at(bone).rotate);
        qClosed.fromMatrix(_handClosed.at(bone).rotate);
        qClosed.slerp(flex, qOpen);
        return qClosed.getMatrix();
    }

    // -- Predefined gesture rotations -----------------------------------------------------

    RE::NiMatrix3 HandPose::getGripBoneRotation(const std::string& bone, const bool melee) const
    {
        const HandFingersPose& pose = melee ? MELEE_GRIP_POSE : GUN_GRIP_POSE;

        // Bone name format: "[LR]Arm_Finger[1-5][1-3]"
        // Maps to: thumb=0-2, index=3-5, middle=6-8, ring=9-11, pinky=12-14
        const auto boneToFlexIndex = boneToFingerIndex(bone) * 3 + (bone.back() - '1');

        const float flex = std::clamp(pose.getFlexAt(boneToFlexIndex), -1.0f, 2.0f);
        return blendBoneRotation(bone, flex);
    }

    RE::NiMatrix3 HandPose::getThumbsUpBoneRotation(const std::string& bone, const bool isLeft) const
    {
        const float sign = isLeft ? -1.0f : 1.0f;
        RE::NiMatrix3 rot = _handOpen.at(bone).rotate;
        if (bone.find("Finger11") != std::string::npos) {
            rot = MatrixUtils::getMatrixFromEulerAngles(sign * 0.5f, sign * 0.4f, -0.3f) * rot;
        } else if (bone.find("Finger13") != std::string::npos) {
            rot = MatrixUtils::getMatrixFromEulerAngles(0, 0, MatrixUtils::degreesToRads(-35.0f)) * rot;
        }
        return rot;
    }

    // -- Private helpers ----------------------------------------------------------------

    RE::NiMatrix3 HandPose::blendBoneRotation(const std::string& bone, const float flex, const float splay, const bool isLeft) const
    {
        RE::NiMatrix3 result = blendBoneRotation(bone, flex);
        if (splay != 0.0f) {
            const float sign = isLeft ? -1.0f : 1.0f;
            result = MatrixUtils::getMatrixFromEulerAngles(0, sign * splay, 0) * result;
        }
        return result;
    }

    void HandPose::setHandPoseOverride(const bool setActive, const bool rightHand, const HandFingersPose& pose)
    {
        bool& poseSet = rightHand ? _rightHandPoseSet : _leftHandPoseSet;
        if (poseSet == setActive) {
            return;
        }
        logger::debug("Hand pose: Set force hand pose for '{}' hand: {})", rightHand ? "Right" : "Left", setActive ? "Set" : "Release");
        poseSet = setActive;
        const auto* const fingers = rightHand ? RIGHT_HAND_FINGERS : LEFT_HAND_FINGERS;
        for (auto i = 0; i < FINGERS_COUNT; i++) {
            _handPapyrusHasControl[fingers[i]] = setActive;
            _handPapyrusPose[fingers[i]] = pose.getFlexAt(i);
        }
    }

    void HandPose::initHandPoses(const bool inPowerArmor)
    {
        _handClosed.clear();
        _handOpen.clear();
        _handBones.clear();

        std::vector<std::vector<float>> data;

        // pulled from the game engine while running idle animations

        // closed fist
        data.push_back({ 0.849409F, -0.270577F, 0.453092F, 0, -0.382631F, 0.275533F, 0.881859F, 0, -0.363453F, -0.922426F, 0.130509F, 0 });
        data.push_back({ 0.698533F, -0.713903F, 0.048938F, 0, 0.710545F, 0.700093F, 0.070685F, 0, -0.084723F, -0.014603F, 0.996297F, 0 });
        data.push_back({ 0.125157F, -0.992116F, -0.006447F, 0, 0.990953F, 0.125323F, -0.048036F, 0, 0.048466F, -0.000376F, 0.998825F, 0 });
        data.push_back({ 0.088989F, -0.995196F, 0.04083F, 0, 0.995157F, 0.090554F, 0.038248F, 0, -0.041762F, 0.037228F, 0.998434F, 0 });
        data.push_back({ -0.473616F, -0.880732F, 0, 0, 0.880732F, -0.473616F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ -0.123119F, -0.992392F, 0, 0, 0.992392F, -0.123119F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.159314F, -0.982871F, 0.09265F, 0, 0.983889F, 0.150362F, -0.096712F, 0, 0.081124F, 0.106565F, 0.990991F, 0 });
        data.push_back({ -0.45663F, -0.889657F, 0, 0, 0.889657F, -0.45663F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ -0.076698F, -0.997054F, 0, 0, 0.997054F, -0.076698F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.123006F, -0.978335F, 0.166524F, 0, 0.978335F, 0.091386F, -0.185766F, 0, 0.166524F, 0.185766F, 0.96838F, 0 });
        data.push_back({ -0.366717F, -0.930333F, 0, 0, 0.930333F, -0.366717F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.324171F, -0.945999F, 0, 0, 0.945999F, 0.324171F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.204525F, -0.935955F, 0.286631F, 0, 0.952761F, 0.123178F, -0.277623F, 0, 0.224536F, 0.329871F, 0.916934F, 0 });
        data.push_back({ -0.190355F, -0.981715F, -0.00044F, 0, 0.981715F, -0.190355F, -0.000533F, 0, 0.00044F, -0.000533F, 1, 0 });
        data.push_back({ -0.188246F, -0.982122F, 0, 0, 0.982122F, -0.188246F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.752071F, -0.282712F, -0.595368F, 0, -0.397682F, 0.525706F, -0.751986F, 0, 0.525584F, 0.802314F, 0.282939F, 0 });
        data.push_back({ 0.556184F, -0.830294F, -0.035639F, 0, 0.826703F, 0.557145F, -0.078435F, 0, 0.084981F, 0.014162F, 0.996282F, 0 });
        data.push_back({ 0.620726F, -0.783447F, 0.030166F, 0, 0.782545F, 0.621458F, 0.037589F, 0, -0.048196F, 0.000274F, 0.998838F, 0 });
        data.push_back({ 0.38695F, -0.915355F, -0.111332F, 0, 0.917694F, 0.394073F, -0.050434F, 0, 0.090038F, -0.082654F, 0.992503F, 0 });
        data.push_back({ -0.152033F, -0.988376F, 0, 0, 0.988376F, -0.152033F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.397566F, -0.917574F, 0, 0, 0.917574F, 0.397566F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.076671F, -0.99201F, -0.100188F, 0, 0.996805F, 0.078521F, -0.014653F, 0, 0.022403F, -0.098745F, 0.994861F, 0 });
        data.push_back({ -0.068391F, -0.997659F, 0, 0, 0.997659F, -0.068391F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ -0.050058F, -0.998746F, 0, 0, 0.998746F, -0.050058F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.068248F, -0.982702F, -0.172158F, 0, 0.997656F, 0.068079F, 0.006893F, 0, 0.004947F, -0.172225F, 0.985045F, 0 });
        data.push_back({ 0.093539F, -0.995616F, 0, 0, 0.995616F, 0.093539F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ -0.33522F, -0.94214F, 0, 0, 0.94214F, -0.33522F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.257096F, -0.93156F, -0.257096F, 0, 0.955995F, 0.206258F, 0.208641F, 0, -0.141334F, -0.299423F, 0.943595F, 0 });
        data.push_back({ -0.21201F, -0.977267F, -0.000434F, 0, 0.977267F, -0.21201F, -0.000538F, 0, 0.000434F, -0.000538F, 1, 0 });
        data.push_back({ -0.276492F, -0.961017F, 0, 0, 0.961017F, -0.276492F, 0, 0, 0, 0, 1, 0 });

        copyDataIntoHand(data, _handClosed);

        data.clear();

        // open hand
        data.push_back({ 0.617716F, -0.400404F, 0.676834F, 0, -0.65398F, 0.216427F, 0.724893F, 0, -0.436735F, -0.890414F, -0.128165F, 0 });
        data.push_back({ 0.899514F, -std::numbers::log10e_v<float>, -0.048362F, 0, 0.435479F, 0.89999F, 0.019389F, 0, 0.035107F, -0.038501F, 0.998642F, 0 });
        data.push_back({ 0.945701F, -0.321798F, -0.045777F, 0, 0.321435F, 0.946808F, -0.015267F, 0, 0.048255F, -0.000276F, 0.998835F, 0 });
        data.push_back({ 0.990258F, -0.114774F, 0.078839F, 0, 0.111225F, 0.992634F, 0.048027F, 0, -0.08377F, -0.03879F, 0.99573F, 0 });
        data.push_back({ 0.958294F, -0.285783F, 0, 0, 0.285783F, 0.958294F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.992354F, -0.123425F, 0, 0, 0.123425F, 0.992354F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.951661F, -0.27608F, -0.134618F, 0, 0.266956F, 0.960211F, -0.082032F, 0, 0.151909F, 0.04213F, 0.987496F, 0 });
        data.push_back({ 0.902528F, -0.430632F, -0.000153F, 0, 0.430632F, 0.902527F, 0.000674F, 0, -0.000153F, -0.000674F, 1, 0 });
        data.push_back({ 0.953147F, -0.302508F, 0.000106F, 0, 0.302508F, 0.953147F, -0.000683F, 0, 0.000106F, 0.000683F, 1, 0 });
        data.push_back({ 0.919043F, -0.392269F, -0.038525F, 0, 0.384414F, 0.913631F, -0.132302F, 0, 0.087095F, 0.106782F, 0.990461F, 0 });
        data.push_back({ 0.927023F, -0.375003F, 0, 0, 0.375003F, 0.927023F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.984968F, -0.172734F, 0, 0, 0.172734F, 0.984968F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.825976F, -0.557004F, -0.086665F, 0, 0.534941F, 0.822993F, -0.191102F, 0, 0.17777F, 0.111485F, 0.977737F, 0 });
        data.push_back({ 0.935958F, -0.352111F, 0, 0, 0.352111F, 0.935958F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.833619F, -0.552339F, 0, 0, 0.552339F, 0.833619F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.584889F, -0.400611F, -0.705277F, 0, -0.656401F, 0.277021F, -0.70171F, 0, 0.47649F, 0.873367F, -0.100935F, 0 });
        data.push_back({ 0.812239F, -0.583324F, 0, 0, 0.583324F, 0.812239F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.970436F, -0.241361F, 0, 0, 0.241361F, 0.970436F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.969328F, -0.20464F, -0.136108F, 0, 0.195507F, 0.977633F, -0.077531F, 0, 0.148929F, 0.048543F, 0.987656F, 0 });
        data.push_back({ 0.949484F, -0.313814F, 0, 0, 0.313814F, 0.949484F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.980211F, -0.197957F, 0, 0, 0.197957F, 0.980211F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.954206F, -0.298892F, 0.01245F, 0, 0.29779F, 0.953005F, 0.055697F, 0, -0.028512F, -0.049439F, 0.99837F, 0 });
        data.push_back({ 0.903441F, -0.428712F, 0, 0, 0.428712F, 0.903441F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.967689F, -0.252149F, 0, 0, 0.252149F, 0.967689F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.926338F, -0.376682F, -0.002837F, 0, 0.37216F, 0.914003F, 0.161543F, 0, -0.058257F, -0.1507F, 0.986862F, 0 });
        data.push_back({ 0.914348F, -0.40493F, 0, 0, 0.40493F, 0.914348F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.919149F, -0.39391F, 0, 0, 0.39391F, 0.919149F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.921646F, -0.376617F, 0.09343F, 0, 0.345979F, 0.906603F, 0.241599F, 0, -0.175694F, -0.190344F, 0.965868F, 0 });
        data.push_back({ 0.957083F, -0.289814F, 0, 0, 0.289814F, 0.957083F, 0, 0, 0, 0, 1, 0 });
        data.push_back({ 0.758452F, -0.651728F, 0, 0, 0.651728F, 0.758452F, 0, 0, 0, 0, 1, 0 });

        copyDataIntoHand(data, _handOpen);

        if (inPowerArmor) {
            _handOpen["LArm_Finger11"].translate = RE::NiPoint3(3.993323F, -4.156268F, 3.585619F);
            _handOpen["LArm_Finger12"].translate = RE::NiPoint3(2.893830F, 0.000042F, 0.000004F);
            _handOpen["LArm_Finger13"].translate = RE::NiPoint3(4.687409F, 0, 0);
            _handOpen["LArm_Finger21"].translate = RE::NiPoint3(8.474635F, -2.161191F, 3.789806F);
            _handOpen["LArm_Finger22"].translate = RE::NiPoint3(2.613208F, 0.000026F, 0.000011F);
            _handOpen["LArm_Finger23"].translate = RE::NiPoint3(5.145684F, 0, 0);
            _handOpen["LArm_Finger31"].translate = RE::NiPoint3(8.151892F, -2.576661F, 1.100114F);
            _handOpen["LArm_Finger32"].translate = RE::NiPoint3(3.722714F, 0.000021F, -0.000004F);
            _handOpen["LArm_Finger33"].translate = RE::NiPoint3(4.984375F, 0, 0);
            _handOpen["LArm_Finger41"].translate = RE::NiPoint3(7.967844F, -2.258833F, -1.337387F);
            _handOpen["LArm_Finger42"].translate = RE::NiPoint3(2.933939F, 0.000027F, 0.000004F);
            _handOpen["LArm_Finger43"].translate = RE::NiPoint3(5.102559F, 0, 0);
            _handOpen["LArm_Finger51"].translate = RE::NiPoint3(8.365221F, -2.603350F, -3.706458F);
            _handOpen["LArm_Finger52"].translate = RE::NiPoint3(2.128304F, 0.000018F, 0.000003F);
            _handOpen["LArm_Finger53"].translate = RE::NiPoint3(4.594295F, 0, 0);
            _handOpen["RArm_Finger11"].translate = RE::NiPoint3(3.993090F, -4.156340F, -3.585553F);
            _handOpen["RArm_Finger12"].translate = RE::NiPoint3(2.893783F, 0.000042F, 0.000004F);
            _handOpen["RArm_Finger13"].translate = RE::NiPoint3(4.686954F, 0, 0);
            _handOpen["RArm_Finger21"].translate = RE::NiPoint3(8.474229F, -2.161169F, -3.789712F);
            _handOpen["RArm_Finger22"].translate = RE::NiPoint3(2.613165F, 0.000026F, 0.000011F);
            _handOpen["RArm_Finger23"].translate = RE::NiPoint3(5.145271F, 0, 0);
            _handOpen["RArm_Finger31"].translate = RE::NiPoint3(8.151529F, -2.576689F, -1.100008F);
            _handOpen["RArm_Finger32"].translate = RE::NiPoint3(3.722677F, 0.000021F, -0.000004F);
            _handOpen["RArm_Finger33"].translate = RE::NiPoint3(4.973974F, 0, 0);
            _handOpen["RArm_Finger41"].translate = RE::NiPoint3(7.967505F, -2.258873F, 1.337498F);
            _handOpen["RArm_Finger42"].translate = RE::NiPoint3(2.933841F, 0.000027F, 0.000004F);
            _handOpen["RArm_Finger43"].translate = RE::NiPoint3(5.102017F, 0, 0);
            _handOpen["RArm_Finger51"].translate = RE::NiPoint3(8.364894F, -2.603419F, 3.706582F);
            _handOpen["RArm_Finger52"].translate = RE::NiPoint3(2.128275F, 0.000018F, 0.000003F);
            _handOpen["RArm_Finger53"].translate = RE::NiPoint3(4.593989F, 0, 0);
        } else {
            _handOpen["LArm_Finger11"].translate = RE::NiPoint3(1.582972F, -1.262648F, 1.853201F);
            _handOpen["LArm_Finger12"].translate = RE::NiPoint3(3.569515F, 0.000042F, 0.000004F);
            _handOpen["LArm_Finger13"].translate = RE::NiPoint3(2.401824F, 0, 0);
            _handOpen["LArm_Finger21"].translate = RE::NiPoint3(7.501364F, 0.430291F, 2.277657F);
            _handOpen["LArm_Finger22"].translate = RE::NiPoint3(3.018186F, 0.000026F, 0.000011F);
            _handOpen["LArm_Finger23"].translate = RE::NiPoint3(1.850236F, 0, 0);
            _handOpen["LArm_Finger31"].translate = RE::NiPoint3(7.595781F, 0.62098F, 0.457392F);
            _handOpen["LArm_Finger32"].translate = RE::NiPoint3(3.091653F, 0.000021F, -0.000004F);
            _handOpen["LArm_Finger33"].translate = RE::NiPoint3(2.187974F, 0, 0);
            _handOpen["LArm_Finger41"].translate = RE::NiPoint3(7.464033F, 0.350152F, -1.438817F);
            _handOpen["LArm_Finger42"].translate = RE::NiPoint3(2.664419F, 0.000027F, 0.000004F);
            _handOpen["LArm_Finger43"].translate = RE::NiPoint3(1.89974F, 0, 0);
            _handOpen["LArm_Finger51"].translate = RE::NiPoint3(6.637259F, -0.35742F, -3.01848F);
            _handOpen["LArm_Finger52"].translate = RE::NiPoint3(2.238261F, 0.000018F, 0.000003F);
            _handOpen["LArm_Finger53"].translate = RE::NiPoint3(1.665912F, 0, 0);
            _handOpen["RArm_Finger11"].translate = RE::NiPoint3(1.582972F, -1.262648F, -1.853201F);
            _handOpen["RArm_Finger12"].translate = RE::NiPoint3(3.569515F, 0.000042F, 0.000004F);
            _handOpen["RArm_Finger13"].translate = RE::NiPoint3(2.401824F, 0, 0);
            _handOpen["RArm_Finger21"].translate = RE::NiPoint3(7.501364F, 0.430291F, -2.277657F);
            _handOpen["RArm_Finger22"].translate = RE::NiPoint3(3.018186F, 0.000026F, 0.000011F);
            _handOpen["RArm_Finger23"].translate = RE::NiPoint3(1.850236F, 0, 0);
            _handOpen["RArm_Finger31"].translate = RE::NiPoint3(7.595781F, 0.62098F, -0.457392F);
            _handOpen["RArm_Finger32"].translate = RE::NiPoint3(3.091653F, 0.000021F, -0.000004F);
            _handOpen["RArm_Finger33"].translate = RE::NiPoint3(2.187974F, 0, 0);
            _handOpen["RArm_Finger41"].translate = RE::NiPoint3(7.464033F, 0.350152F, 1.438817F);
            _handOpen["RArm_Finger42"].translate = RE::NiPoint3(2.664419F, 0.000027F, 0.000004F);
            _handOpen["RArm_Finger43"].translate = RE::NiPoint3(1.89974F, 0, 0);
            _handOpen["RArm_Finger51"].translate = RE::NiPoint3(6.637259F, -0.35742F, 3.01848F);
            _handOpen["RArm_Finger52"].translate = RE::NiPoint3(2.238261F, 0.000018F, 0.000003F);
            _handOpen["RArm_Finger53"].translate = RE::NiPoint3(1.665912F, 0, 0);
        }
    }

    void HandPose::copyDataIntoHand(const std::vector<std::vector<float>>& data, std::map<std::string, RE::NiTransform>& hand)
    {
        // Left hand fingers
        copyDataIntoHand(data[0], hand, "LArm_Finger11");
        copyDataIntoHand(data[1], hand, "LArm_Finger12");
        copyDataIntoHand(data[2], hand, "LArm_Finger13");
        copyDataIntoHand(data[3], hand, "LArm_Finger21");
        copyDataIntoHand(data[4], hand, "LArm_Finger22");
        copyDataIntoHand(data[5], hand, "LArm_Finger23");
        copyDataIntoHand(data[6], hand, "LArm_Finger31");
        copyDataIntoHand(data[7], hand, "LArm_Finger32");
        copyDataIntoHand(data[8], hand, "LArm_Finger33");
        copyDataIntoHand(data[9], hand, "LArm_Finger41");
        copyDataIntoHand(data[10], hand, "LArm_Finger42");
        copyDataIntoHand(data[11], hand, "LArm_Finger43");
        copyDataIntoHand(data[12], hand, "LArm_Finger51");
        copyDataIntoHand(data[13], hand, "LArm_Finger52");
        copyDataIntoHand(data[14], hand, "LArm_Finger53");

        // Right hand fingers
        copyDataIntoHand(data[15], hand, "RArm_Finger11");
        copyDataIntoHand(data[16], hand, "RArm_Finger12");
        copyDataIntoHand(data[17], hand, "RArm_Finger13");
        copyDataIntoHand(data[18], hand, "RArm_Finger21");
        copyDataIntoHand(data[19], hand, "RArm_Finger22");
        copyDataIntoHand(data[20], hand, "RArm_Finger23");
        copyDataIntoHand(data[21], hand, "RArm_Finger31");
        copyDataIntoHand(data[22], hand, "RArm_Finger32");
        copyDataIntoHand(data[23], hand, "RArm_Finger33");
        copyDataIntoHand(data[24], hand, "RArm_Finger41");
        copyDataIntoHand(data[25], hand, "RArm_Finger42");
        copyDataIntoHand(data[26], hand, "RArm_Finger43");
        copyDataIntoHand(data[27], hand, "RArm_Finger51");
        copyDataIntoHand(data[28], hand, "RArm_Finger52");
        copyDataIntoHand(data[29], hand, "RArm_Finger53");
    }

    void HandPose::copyDataIntoHand(const std::vector<float>& fingerData, std::map<std::string, RE::NiTransform>& hand, const char* finger)
    {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                hand[finger].rotate.entry[row][col] = fingerData[row * 4 + col];
            }
        }
    }
}
