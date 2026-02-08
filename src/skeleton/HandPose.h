#pragma once

#include "Skeleton.h"

namespace frik
{
    extern std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator> handClosed;
    extern std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator> handOpen;

    extern std::map<std::string, float> handPapyrusPose;
    extern std::map<std::string, bool> handPapyrusHasControl;

    void initHandPoses(bool inPowerArmor);

    float getHandBonePose(const std::string& bone, const bool melee);

    void setFingerPositionScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky);
    void setFingerJointPositions(bool isLeft, const float values[15]);
    void restoreFingerPoseControl(bool isLeft);

    void setPipboyHandPose();
    void disablePipboyHandPose();
    void setConfigModeHandPose();
    void disableConfigModePose();

    void setForceHandPointingPose(bool primaryHand, bool forcePointing);

    void setOffhandGripHandPose(bool toSet);
    void setAttaboyHandPose(bool toSet);
    void setHandPoseOverride(bool override, bool rightHand, const float* handPose);
}
