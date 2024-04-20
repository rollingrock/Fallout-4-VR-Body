#pragma once

#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"
#include "f4se/GameSettings.h"

#include "matrix.h"

namespace F4VRBody {

	typedef void* (*_AIProcess_ClearMuzzleFlashes)(Actor::MiddleProcess* middleProcess);
	extern RelocAddr<_AIProcess_ClearMuzzleFlashes> AIProcess_ClearMuzzleFlashes;

	typedef void* (*_AIProcess_CreateMuzzleFlash)(Actor::MiddleProcess* middleProcess, uint64_t projectile, Actor* actor);
	extern RelocAddr<_AIProcess_CreateMuzzleFlash> AIProcess_CreateMuzzleFlash;

	float vec3_len(NiPoint3 v1);
	NiPoint3 vec3_norm(NiPoint3 v1);

	float vec3_dot(NiPoint3 v1, NiPoint3 v2);

	NiPoint3 vec3_cross(NiPoint3 v1, NiPoint3 v2);

	float vec3_det(NiPoint3 v1, NiPoint3 v2, NiPoint3 n);

	float degrees_to_rads(float deg);
	float rads_to_degrees(float deg);

	NiPoint3 rotateXY(NiPoint3 vec, float angle);
	NiPoint3 pitchVec(NiPoint3 vec, float angle);

	NiMatrix43 getRotationAxisAngle(NiPoint3 axis, float theta);

	void updateTransforms(NiNode* node);

	void updateTransformsDown(NiNode* nde, bool updateSelf);

	void toggleVis(NiNode* nde, bool hide, bool updateSelf);

	void turnPipBoyOn();
	void turnPipBoyOff();

	bool getLeftHandedMode();

	NiNode* getChildNode(const char* nodeName, NiNode* nde);
	NiNode* get1stChildNode(const char* nodeName, NiNode* nde);

	Setting* GetINISettingNative(const char* name);

	void printNodePos(std::string nodeName, NiPoint3 node);

}
