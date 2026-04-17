#pragma once

#include <map>
#include <string>

#include "common/CommonUtils.h"

namespace frik
{
    // Pose values for a single finger.
    // Flex: 0.0 = bent/closed, 1.0 = straight/open.
    // Splay: positive/negative Y-axis rotation on the proximal joint (side-to-side).
    struct FingerPose
    {
        float prox = 0.0f; // proximal (MCP / knuckle) joint flex
        float mid = 0.0f; // middle (PIP) joint flex
        float dist = 0.0f; // distal (DIP / fingertip) joint flex
        float splay = 0.0f; // lateral abduction/adduction of the proximal joint
    };

    // Full hand pose: all 5 fingers, each with flex + splay.
    struct HandFingersPose
    {
        FingerPose thumb;
        FingerPose index;
        FingerPose middle;
        FingerPose ring;
        FingerPose pinky;

        float getFlexAt(int boneIndex) const noexcept;
        float getSplayAt(int fingerIndex) const noexcept;
    };

    class HandPose
    {
    public:
        explicit HandPose(bool inPowerArmor);

        // Papyrus / API-driven pose overrides
        static void setFingerPositionScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky);
        static void setFingerSplayScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky);
        static void restoreFingerPoseControl(bool isLeft);

        // Named pose setters
        static void setPipboyHandPose();
        static void disablePipboyHandPose();
        static void setConfigModeHandPose();
        static void disableConfigModePose();
        static void setForceHandPointingPose(bool primaryHand, bool forcePointing);
        static void setOffhandGripHandPose(bool toSet);
        static void setAttaboyHandPose(bool toSet);

        void onFrameUpdate(RE::NiNode* root, float frameTime);

    private:
        void calculateHandPose(const std::string& bone, bool isLeft, float frameTime);
        void copy1StPerson(const std::string& bone);

        bool tryGetPapyrusRotation(const std::string& bone, bool isLeft, RE::NiMatrix3& outRotation) const;
        RE::NiMatrix3 blendBoneRotation(const std::string& bone, float flex) const;
        RE::NiMatrix3 getGripBoneRotation(const std::string& bone, bool melee) const;
        RE::NiMatrix3 getThumbsUpBoneRotation(const std::string& bone, bool isLeft) const;
        RE::NiMatrix3 blendBoneRotation(const std::string& bone, float flex, float splay, bool isLeft) const;

        static void setHandPoseOverride(bool setActive, bool rightHand, const HandFingersPose& pose);

        std::map<std::string, RE::NiTransform> _handClosed;
        std::map<std::string, RE::NiTransform> _handOpen;
        std::map<std::string, RE::NiTransform> _handBones;
        inline static std::map<std::string, float> _handPapyrusPose;
        inline static std::map<std::string, float> _handSplayPose;
        inline static std::map<std::string, bool> _handPapyrusHasControl;
        inline static bool _leftHandPoseSet = false;
        inline static bool _rightHandPoseSet = false;
    };
}
