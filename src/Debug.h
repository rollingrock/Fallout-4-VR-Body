#pragma once

#include "Skeleton.h"
#include "utils.h"
#include "common/Matrix.h"

namespace frik {
	void printMatrix(const common::Matrix44* mat);
	void positionDiff(const Skeleton* skelly);
	void printAllNodes();
	void printNodes(RE::NiNode* node, bool printAncestors = true);
	void printNodes(RE::NiNode* nde, long long curTime);
	void printNodesTransform(RE::NiNode* node, std::string padding = "");
	void printTransform(const std::string& name, const RE::NiTransform& transform, bool sample = false);
	void printPosition(const std::string& name, const RE::NiPoint3& pos, bool sample);
	void dumpPlayerGeometry(RE::BSFadeNode* rn);
	void debug(const Skeleton* skelly);
	void printScaleFormElements(GFxValue* elm, const std::string& padding = "");
}
