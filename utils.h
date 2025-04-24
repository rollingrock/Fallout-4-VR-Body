#pragma once

#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"
#include "f4se/GameSettings.h"
#include "f4se/GameRTTI.h"
#include "matrix.h"
#include "Offsets.h"
#include "VR.h"

#include <chrono>

namespace F4VRBody {
	typedef void* (*_AIProcess_ClearMuzzleFlashes)(Actor::MiddleProcess* middleProcess);
	extern RelocAddr<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes;

	typedef void* (*_AIProcess_CreateMuzzleFlash)(Actor::MiddleProcess* middleProcess, uint64_t projectile, Actor* actor);
	extern RelocAddr<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash;

	uint64_t nowMillis();
	std::string toStringWithPrecision(double value, int precision = 2);
	float vec3_len(const NiPoint3& v1);
	NiPoint3 vec3_norm(NiPoint3 v1);

	float vec3_dot(const NiPoint3& v1, const NiPoint3& v2);

	NiPoint3 vec3_cross(const NiPoint3& v1, const NiPoint3& v2);

	float vec3_det(NiPoint3 v1, NiPoint3 v2, NiPoint3 n);

	float degrees_to_rads(float deg);
	float rads_to_degrees(float deg);

	NiPoint3 rotateXY(NiPoint3 vec, float angle);
	NiPoint3 pitchVec(NiPoint3 vec, float angle);

	NiMatrix43 getRotationAxisAngle(NiPoint3 axis, float theta);

	void updateTransforms(NiNode* node);

	void updateTransformsDown(NiNode* nde, bool updateSelf);

	void toggleVis(NiNode* nde, bool hide, bool updateSelf);

	void SetINIBool(BSFixedString name, bool value);
	void SetINIFloat(BSFixedString name, float value);
	//void SetINIInt(BSFixedString name, int value);
	void ConfigureGameVars();
	void WindowFocus();
	void TurnPlayerRadioOn(bool isActive);
	void SimulateExtendedButtonPress(WORD vkey);
	//void PipboyReopen();
	void ShowMessagebox(std::string asText);
	void ShowNotification(std::string asText);

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
	bool isButtonLongPressedOnController(bool primary, int buttonId, int longPressSuration = 1500);
	bool checkAndClearButtonLongPressedOnController(bool primary, int buttonId, int longPressSuration = 1500);

	bool isCameraLookingAtObject(NiAVObject* cameraNode, NiAVObject* objectNode, float detectThresh);

	std::string getEquippedWeaponName();
	bool getLeftHandedMode();

	NiNode* getChildNode(const char* nodeName, NiNode* nde);
	NiNode* get1stChildNode(const char* nodeName, NiNode* nde);

	Setting* GetINISettingNative(const char* name);

	// get elapsed time when needed
	template <
		class result_t = std::chrono::milliseconds,
		class clock_t = std::chrono::steady_clock,
		class duration_t = std::chrono::milliseconds>
	auto since(std::chrono::time_point<clock_t, duration_t> const& start) {
		return std::chrono::duration_cast<result_t>(clock_t::now() - start);
	}

	std::string str_tolower(std::string s);
	std::string ltrim(std::string s);
	std::string rtrim(std::string s);
	std::string trim(std::string s);

	std::optional<std::string> getEmbeddedResourceAsStringIfExists(const WORD resourceId);
	std::string getEmbededResourceAsString(WORD idr);
	std::string getCurrentTimeString();
	std::vector<std::string> loadListFromFile(std::string filePath);
	void createDirDeep(std::string pathStr);
	void createFileFromResourceIfNotExists(const std::string& filePath, WORD resourceId);

	bool isBetterScopesVRModLoaded();
	bool isModLoaded(const char* modName);
}
