#pragma once
#include "f4se/NiTypes.h"

namespace F4VRBody {

	extern std::map<std::string, NiTransform> handClosed;
	extern std::map<std::string, NiTransform> handOpen;


	void initHandPoses();
}
