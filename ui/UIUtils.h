#pragma once

#include "f4se/NiNodes.h"

// TODO: refactor to remove this dependancy!!!
#include "../Skeleton.h"

namespace ui {

	static std::string getDebugSphereNif() { return "FRIK/1x1Sphere.nif"; }
	static std::string getPrimaryWandNodeName() { return "world_primaryWand.nif"; }

	F4VRBody::PlayerNodes* getPlayerNodes();
	void setNodeVisibility(NiNode* node, bool visible);
	NiNode* getClonedNiNodeForNifFile(const std::string& path);
	NiNode* loadNifFromFile(const char* path);
	NiNode* findNode(const char* nodeName, NiNode* root);
	void attachNodeToPrimaryWand(const NiNode* node);
}
