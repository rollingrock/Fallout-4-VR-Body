#pragma once
#include "f4se/NiTypes.h"

namespace F4VRBody {

	extern std::map<std::string, NiTransform> handClosed;
	extern std::map<std::string, NiTransform> handOpen;

	extern std::map<std::string, float> handPapyrusPose;
	extern std::map<std::string, bool> handPapyrusHasControl;

	void initHandPoses();
}
