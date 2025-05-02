#pragma once

#include "utils.h"
#include "Skeleton.h"

namespace FRIK {
	void _SmsMESSAGE(const std::string key, int time, const char* fmt, ...);
	void _S1sMESSAGE(const std::string key, const char* fmt, ...);
	void positionDiff(Skeleton* skelly);
	void printAllNodes(Skeleton* skelly);
	void printNodes(NiNode* nde);
	void printChildren(NiNode* child, std::string padding);
	void printNodes(NiNode* nde, long long curTime);
	void printNodesTransform(NiNode* node, std::string padding = "");
	void printTransform(std::string name, NiTransform& transform);
	void dumpPlayerGeometry(BSFadeNode* rn);
	void debug(Skeleton* skelly);
}