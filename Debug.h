#pragma once

#include "utils.h"
#include "Skeleton.h"

namespace F4VRBody {
	static void positionDiff(Skeleton* skelly);
	static void printNodes(NiNode* nde);
	static void printChildren(NiNode* child, std::string padding);
	void printNodes(NiNode* nde, long long curTime);
}