#pragma once

#include "Skeleton.h"
#include "utils.h"
#include "common/Matrix.h"

namespace frik {
	void printMatrix(const common::Matrix44* mat);
	void positionDiff(const Skeleton* skelly);
	void printAllNodes();
	void printNodes(NiNode* node, bool printAncestors = true);
	void printNodes(NiNode* nde, long long curTime);
	void printNodesTransform(NiNode* node, std::string padding = "");
	void printTransform(const std::string& name, const NiTransform& transform, bool sample = false);
	void dumpPlayerGeometry(BSFadeNode* rn);
	void debug(const Skeleton* skelly);
}
