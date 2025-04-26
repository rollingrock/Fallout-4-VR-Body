#pragma once

#include "UIElement.h"
#include "f4se/NiNodes.h"

// TODO: refactor to remove this dependency!!!
#include "../Skeleton.h"

namespace ui {
	static std::string getDebugSphereNifName() { return "FRIK/1x1Sphere.nif"; }
	static std::string getToggleButtonFrameNifName() { return "FRIK/ui_common_btn_border.nif"; }

	static UISize getButtonDefaultSize() { return {2.2f, 2.2f}; }

	F4VRBody::PlayerNodes* getPlayerNodes();
	void setNodeVisibility(NiNode* node, bool visible, float originalScale);
	NiNode* getClonedNiNodeForNifFile(const std::string& path);
	NiNode* loadNifFromFile(const char* path);
	NiNode* findNode(const char* nodeName, NiNode* node);
	bool isBetterScopesVRModLoaded();

	// Const matrix to invert objects for left-handed mode.
	static const F4VRBody::Matrix44& getLeftHandInvertMatrix() {
		static const F4VRBody::Matrix44 ROT = [] {
			F4VRBody::Matrix44 rot;
			rot.setEulerAngles(0, F4VRBody::degrees_to_rads(180), 0);
			return rot;
		}();
		return ROT;
	}
}
