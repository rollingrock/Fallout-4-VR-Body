#pragma once

#include "f4vr/Offsets.h"
#include "f4se/GameReferences.h"
#include "f4se/GameSettings.h"
#include "f4vr/VR.h"

#include <chrono>

namespace frik {
	using _AIProcess_ClearMuzzleFlashes = void* (*)(Actor::MiddleProcess* middleProcess);
	extern RelocAddr<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes;

	using _AIProcess_CreateMuzzleFlash = void* (*)(Actor::MiddleProcess* middleProcess, uint64_t projectile, Actor* actor);
	extern RelocAddr<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash;

	void updateTransforms(NiNode* node);

	void updateTransformsDown(NiNode* nde, bool updateSelf);

	void toggleVis(NiNode* nde, bool hide, bool updateSelf);

	void configureGameVars();
	void windowFocus();
	void turnPlayerRadioOn(bool isActive);
	void simulateExtendedButtonPress(WORD vkey);
	void showMessagebox(const std::string& asText);
	void showNotification(const std::string& asText);

	void turnPipBoyOn();
	void turnPipBoyOff();
	bool isAnyPipboyOpen();

	bool isNodeVisible(const NiNode* node);
	void showHideNode(NiAVObject* node, bool toHide);

	bool isCameraLookingAtObject(const NiAVObject* cameraNode, const NiAVObject* objectNode, float detectThresh);

	bool isMeleeWeaponEquipped();
	std::string getEquippedWeaponName();
	bool getLeftHandedMode();

	NiNode* getChildNode(const char* nodeName, NiNode* nde);
	NiNode* get1StChildNode(const char* nodeName, const NiNode* nde);

	Setting* getINISettingNative(const char* name);

	bool isBetterScopesVRModLoaded();
	bool isModLoaded(const char* modName);
}
