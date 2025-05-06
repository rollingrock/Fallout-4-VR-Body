#pragma once

#include "Skeleton.h"
#include "utils.h"
#include "common/Matrix.h"

namespace frik {
	void printMatrix(const common::Matrix44* mat);
	void positionDiff(const Skeleton* skelly);
	void printAllNodes(const Skeleton* skelly);
	void printNodes(const NiNode* nde);
	void printChildren(NiNode* child, std::string padding);
	void printNodes(NiNode* nde, long long curTime);
	void printNodesTransform(NiNode* node, std::string padding = "");
	void printTransform(const std::string& name, const NiTransform& transform);
	void dumpPlayerGeometry(BSFadeNode* rn);
	void debug(const Skeleton* skelly);
}
