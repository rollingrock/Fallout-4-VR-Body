#pragma once

#include "UIElement.h"
#include "f4se/NiNodes.h"

// TODO: refactor to remove this dependency!!!
#include "../Skeleton.h"

namespace ui {
	static std::string getDebugSphereNifName() { return "FRIK/1x1Sphere.nif"; }
	static std::string getToggleButtonFrameNifName() { return "FRIK/UI-ConfigMarker.nif"; }
	static std::string getPrimaryWandNodeName() { return "world_primaryWand.nif"; }

	F4VRBody::PlayerNodes* getPlayerNodes();
	void setNodeVisibility(NiNode* node, bool visible);
	NiNode* getClonedNiNodeForNifFile(const std::string& path);
	NiNode* loadNifFromFile(const char* path);
	NiNode* findNode(const char* nodeName, NiNode* node);
	void attachNodeToPrimaryWand(const NiNode* node);
}
