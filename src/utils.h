#pragma once

#include "Offsets.h"
#include "VR.h"
#include "f4se/GameReferences.h"
#include "f4se/GameSettings.h"

#include <chrono>

namespace frik {
	using _AIProcess_ClearMuzzleFlashes = void* (*)(Actor::MiddleProcess* middleProcess);
	extern RelocAddr<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes;

	using _AIProcess_CreateMuzzleFlash = void* (*)(Actor::MiddleProcess* middleProcess, uint64_t projectile, Actor* actor);
	extern RelocAddr<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash;

	uint64_t nowMillis();
	std::string toStringWithPrecision(double value, int precision = 2);
	float vec3Len(const NiPoint3& v1);
	NiPoint3 vec3Norm(NiPoint3 v1);

	float vec3Dot(const NiPoint3& v1, const NiPoint3& v2);

	NiPoint3 vec3Cross(const NiPoint3& v1, const NiPoint3& v2);

	float vec3Det(NiPoint3 v1, NiPoint3 v2, NiPoint3 n);

	float degreesToRads(float deg);
	float radsToDegrees(float rad);

	NiPoint3 rotateXY(NiPoint3 vec, float angle);
	NiPoint3 pitchVec(NiPoint3 vec, float angle);

	NiMatrix43 getRotationAxisAngle(NiPoint3 axis, float theta);

	void updateTransforms(NiNode* node);

	void updateTransformsDown(NiNode* nde, bool updateSelf);

	void toggleVis(NiNode* nde, bool hide, bool updateSelf);

	void setINIBool(BSFixedString name, bool value);
	void setINIFloat(BSFixedString name, float value);
	//void SetINIInt(BSFixedString name, int value);
	void configureGameVars();
	void windowFocus();
	void turnPlayerRadioOn(bool isActive);
	void simulateExtendedButtonPress(WORD vkey);
	//void PipboyReopen();
	void showMessagebox(const std::string& asText);
	void showNotification(const std::string& asText);

	void turnPipBoyOn();
	void turnPipBoyOff();
	bool isAnyPipboyOpen();
	void setControlsThumbstickEnableState(bool toEnable);
	void rotationStickEnabledToggle(bool enable);

	bool isNodeVisible(const NiNode* node);
	void showHideNode(NiAVObject* node, bool toHide);
	vr::VRControllerState_t getControllerState(bool primary);
	bool isButtonPressedOnController(bool primary, int buttonId);
	bool isButtonPressHeldDownOnController(bool primary, int buttonId);
	bool isButtonReleasedOnController(bool primary, int buttonId);
	bool isButtonLongPressedOnController(bool primary, int buttonId, int longPressDuration = 1500);
	bool checkAndClearButtonLongPressedOnController(bool primary, int buttonId, int longPressDuration = 1500);

	bool isCameraLookingAtObject(const NiAVObject* cameraNode, const NiAVObject* objectNode, float detectThresh);

	bool isMeleeWeaponEquipped();
	std::string getEquippedWeaponName();
	bool getLeftHandedMode();

	NiNode* getChildNode(const char* nodeName, NiNode* nde);
	NiNode* get1StChildNode(const char* nodeName, const NiNode* nde);

	Setting* getINISettingNative(const char* name);

	// get elapsed time when needed
	template <
		class result_t = std::chrono::milliseconds,
		class clock_t = std::chrono::steady_clock,
		class duration_t = std::chrono::milliseconds>
	auto since(const std::chrono::time_point<clock_t, duration_t>& start) {
		return std::chrono::duration_cast<result_t>(clock_t::now() - start);
	}

	std::string str_tolower(std::string s);
	std::string ltrim(std::string s);
	std::string rtrim(std::string s);
	std::string trim(const std::string& s);

	std::optional<std::string> getEmbeddedResourceAsStringIfExists(WORD resourceId);
	std::string getEmbededResourceAsString(WORD resourceId);
	std::string getCurrentTimeString();
	std::vector<std::string> loadListFromFile(const std::string& filePath);
	void createDirDeep(const std::string& pathStr);
	void createFileFromResourceIfNotExists(const std::string& filePath, WORD resourceId, bool fixNewline);
	std::string getRelativePathInDocuments(const std::string& relPath);
	void moveFileSafe(const std::string& fromPath, const std::string& toPath);
	void moveAllFilesInFolderSafe(const std::string& fromPath, const std::string& toPath);

	bool isBetterScopesVRModLoaded();
	bool isModLoaded(const char* modName);
}
