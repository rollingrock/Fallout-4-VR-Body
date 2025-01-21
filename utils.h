#pragma once

#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"
#include "f4se/GameSettings.h"
#include "f4se/GameRTTI.h"
#include "matrix.h"
#include "Offsets.h"
#include "F4VRBody.h"

#include <chrono>

namespace F4VRBody {

	typedef void* (*_AIProcess_ClearMuzzleFlashes)(Actor::MiddleProcess* middleProcess);
	extern RelocAddr<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes;

	typedef void* (*_AIProcess_CreateMuzzleFlash)(Actor::MiddleProcess* middleProcess, uint64_t projectile, Actor* actor);
	extern RelocAddr<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash;

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

	void TurnPlayerRadioOn(bool isActive);
	void SimulateExtendedButtonPress(WORD vkey);
	void RightStickXSleep(int time);
	void RightStickYSleep(int time);
	void SecondaryTriggerSleep(int time);
	//void PipboyReopen();
	void ShowMessagebox(std::string asText);
	void ShowNotification(std::string asText);

	void turnPipBoyOn();
	void turnPipBoyOff();

	bool getLeftHandedMode();

	NiNode* getChildNode(const char* nodeName, NiNode* nde);
	NiNode* get1stChildNode(const char* nodeName, NiNode* nde);

	Setting* GetINISettingNative(const char* name);

	// get elapsed time when needed
	template <
		class result_t = std::chrono::milliseconds,
		class clock_t = std::chrono::steady_clock,
		class duration_t = std::chrono::milliseconds
	>
	auto since(std::chrono::time_point<clock_t, duration_t> const& start)
	{
		return std::chrono::duration_cast<result_t>(clock_t::now() - start);
	}

	std::string str_tolower(std::string s);
	std::string ltrim(std::string s);
	std::string rtrim(std::string s);
	std::string trim(std::string s);
}
