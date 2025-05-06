#pragma once

#include "UIElement.h"

// TODO: refactor to remove this dependency!!!
#include "../Skeleton.h"

namespace vrui {
	static std::string getDebugSphereNifName() { return "FRIK/1x1Sphere.nif"; }
	static std::string getToggleButtonFrameNifName() { return "FRIK/ui_common_btn_border.nif"; }

	static UISize getButtonDefaultSize() { return {2.2f, 2.2f}; }

	frik::PlayerNodes* getPlayerNodes();
	void setNodeVisibility(NiNode* node, bool visible, float originalScale);
	NiNode* getClonedNiNodeForNifFile(const std::string& path);
	NiNode* loadNifFromFile(const char* path);
	NiNode* findNode(const char* nodeName, NiNode* node);
	bool isBetterScopesVRModLoaded();
}
