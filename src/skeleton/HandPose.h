#pragma once

#include <map>
#include <string>
#include <vector>
#include "Skeleton.h"

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

        [[nodiscard]] float getFlexAt(int boneIndex) const noexcept;

        [[nodiscard]] float getSplayAt(int fingerIndex) const noexcept;
    };

    class HandPose
    {
    public:
        // Lifecycle
        static void initHandPoses(bool inPowerArmor);

        // Accessors for Skeleton — read the current pose state
        static bool hasPapyrusControl(const std::string& bone);
        static float getPapyrusPose(const std::string& bone);
        static float getSplayPose(const std::string& bone); // returns 0 if not set
        static const RE::NiTransform& getOpenPose(const std::string& bone);
        static const RE::NiTransform& getClosedPose(const std::string& bone);
        static const std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator>& getOpenPoses();

        // Predefined pose value for the given bone (gun or melee grip)
        static float getHandBonePose(const std::string& bone, bool melee);

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

    private:
        static int boneToFlexIndex(const std::string& bone);
        static void copyDataIntoHand(const std::vector<float>& fingerData,
            std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator>& hand,
            const char* finger);
        static void copyDataIntoHand(const std::vector<std::vector<float>>& data,
            std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator>& hand);
        static void setHandPoseOverride(bool setActive, bool rightHand, const HandFingersPose& pose);

        inline static std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator> _handClosed;
        inline static std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator> _handOpen;
        inline static std::map<std::string, float> _handPapyrusPose;
        inline static std::map<std::string, float> _handSplayPose;
        inline static std::map<std::string, bool> _handPapyrusHasControl;
        inline static bool _leftHandPoseSet = false;
        inline static bool _rightHandPoseSet = false;
    };
}
