#pragma once

#include "utils.h"
#include "Skeleton.h"

namespace F4VRBody {
	void printRoot();
	void positionDiff(Skeleton* skelly);
	void printNodes(NiNode* nde);
	void printChildren(NiNode* child, std::string padding);
	void printNodes(NiNode* nde, long long curTime);
	void printTransform(std::string name, NiTransform& transform);
	void dumpPlayerGeometry(BSFadeNode* rn);
	void debug(Skeleton* skelly);
}