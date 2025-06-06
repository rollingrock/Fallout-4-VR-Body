#pragma once

#include "f4se/GameReferences.h"
#include "f4vr/MiscStructs.h"

namespace frik {
	void turnPlayerRadioOn(bool isActive);

	void turnPipBoyOn();
	void turnPipBoyOff();
	bool isAnyPipboyOpen();

	bool isCameraLookingAtObject(const NiAVObject* cameraNode, const NiAVObject* objectNode, float detectThresh);
	bool isArmorHasHeadLamp();

	bool isBetterScopesVRModLoaded();

	f4vr::MuzzleFlash* getMuzzleFlashNodes();
}
