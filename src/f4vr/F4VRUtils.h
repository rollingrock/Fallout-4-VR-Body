#pragma once

#include <f4se/GameTypes.h>
#include <f4se/NiNodes.h>

#include "../api/openvr.h"

namespace f4vr {
	void setControlsThumbstickEnableState(bool toEnable);

	vr::VRControllerState_t getControllerState(bool primary);
	bool isButtonPressedOnController(bool primary, int buttonId);
	bool isButtonPressHeldDownOnController(bool primary, int buttonId);
	bool isButtonReleasedOnController(bool primary, int buttonId);
	bool isButtonLongPressedOnController(bool primary, int buttonId, int longPressDuration = 1500);
	bool checkAndClearButtonLongPressedOnController(bool primary, int buttonId, int longPressDuration = 1500);

	void setINIBool(BSFixedString name, bool value);
	void setINIFloat(BSFixedString name, float value);

	NiNode* getNode(const char* name, NiNode* fromNode);
	NiNode* getNode2(const char* name, NiNode* fromNode);
	void setVisibility(NiAVObject* nde, bool show = true);
	void updateDown(NiNode* nde, bool updateSelf);
	void updateDownTo(NiNode* toNode, NiNode* fromNode, bool updateSelf);
	void updateUpTo(NiNode* toNode, NiNode* fromNode, bool updateSelf);
}
