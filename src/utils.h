#pragma once

#include "f4se/GameReferences.h"
#include "f4se/GameSettings.h"

#include <chrono>

namespace frik {
	void configureGameVars();
	void windowFocus();
	void turnPlayerRadioOn(bool isActive);
	void simulateExtendedButtonPress(WORD vkey);

	void turnPipBoyOn();
	void turnPipBoyOff();
	bool isAnyPipboyOpen();

	bool isCameraLookingAtObject(const NiAVObject* cameraNode, const NiAVObject* objectNode, float detectThresh);

	bool isBetterScopesVRModLoaded();
	bool isModLoaded(const char* modName);
}
