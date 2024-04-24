#pragma once
#include "f4se/NiTypes.h"
#include "Skeleton.h"

namespace F4VRBody {

	extern std::map<std::string, NiTransform, CaseInsensitiveComparator> handClosed;
	extern std::map<std::string, NiTransform, CaseInsensitiveComparator> handOpen;

	extern std::map<std::string, float> handPapyrusPose;
	extern std::map<std::string, bool> handPapyrusHasControl;

	void initHandPoses();
}
