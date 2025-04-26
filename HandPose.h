#pragma once
#include "f4se/NiTypes.h"
#include "Skeleton.h"

namespace F4VRBody {
	extern std::map<std::string, NiTransform, CaseInsensitiveComparator> handClosed;
	extern std::map<std::string, NiTransform, CaseInsensitiveComparator> handOpen;

	extern std::map<std::string, float> handPapyrusPose;
	extern std::map<std::string, bool> handPapyrusHasControl;

	void initHandPoses(bool inPowerArmor);

	void setFingerPositionScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky);
	void restoreFingerPoseControl(bool isLeft);

	void setPipboyHandPose();
	void disablePipboyHandPose();
	void setConfigModeHandPose();
	void disableConfigModePose();

	void setForceHandPointingPose(bool primaryHand, bool forcePointing);
	void setForceHandPointingPoseExplicitHand(bool rightHand, bool forcePointing);
}
