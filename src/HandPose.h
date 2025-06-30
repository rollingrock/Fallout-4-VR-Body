#pragma once

#include "Skeleton.h"
#include "f4se/NiTypes.h"

namespace frik {
	extern std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator> handClosed;
	extern std::map<std::string, RE::NiTransform, common::CaseInsensitiveComparator> handOpen;

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
	void setForceHandPointingPoseExplicitHand(bool rightHand, bool override);

	void setOffhandGripHandPose();
	void releaseOffhandGripHandPose();
	void setOffhandGripHandPoseOverride(bool override);
}
