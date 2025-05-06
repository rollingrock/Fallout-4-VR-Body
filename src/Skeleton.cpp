#include "Skeleton.h"
#include <array>
#include <chrono>
#include <cstring>
#include <ctime>
#include <f4se/GameRTTI.h>
#include "BSFlattenedBoneTree.h"
#include "Config.h"
#include "F4VRBody.h"
#include "HandPose.h"
#include "Menu.h"
#include "Pipboy.h"
#include "f4vr/VR.h"
#include "common/CommonUtils.h"
#include "common/Quaternion.h"
#include "f4se/GameForms.h"
#include "f4vr/F4VRUtils.h"

extern PapyrusVRAPI* g_papyrusvr;
extern OpenVRHookManagerAPI* _vrhook;

using namespace std::chrono;
using namespace common;
using namespace f4vr;

namespace frik {
	float defaultCameraHeight = 120.4828f;
	float PACameraHeightDiff = 20.7835f;

	UInt32 KeywordPowerArmor = 0x4D8A1;
	UInt32 KeywordPowerArmorFrame = 0x15503F;
	bool tempsticky = false;
	std::map<std::string, int> boneTreeMap;
	std::vector<std::string> boneTreeVec;
	NiPoint3 _lasthmdtoNewHip;

	void addFingerRelations(std::map<std::string, std::pair<std::string, std::string>>* map, std::string hand, std::string finger1, std::string finger2, std::string finger3) {
		map->insert({finger1, {hand, finger2}});
		map->insert({finger2, {finger1, finger3}});
		map->insert({finger3, {finger2, std::string()}});
	}

	std::map<std::string, std::pair<std::string, std::string>> makeFingerRelations() {
		std::map<std::string, std::pair<std::string, std::string>> map;
		//left hand
		addFingerRelations(&map, "LArm_Hand", "LArm_Finger11", "LArm_Finger12", "LArm_Finger13");
		addFingerRelations(&map, "LArm_Hand", "LArm_Finger21", "LArm_Finger22", "LArm_Finger23");
		addFingerRelations(&map, "LArm_Hand", "LArm_Finger31", "LArm_Finger32", "LArm_Finger33");
		addFingerRelations(&map, "LArm_Hand", "LArm_Finger41", "LArm_Finger42", "LArm_Finger43");
		addFingerRelations(&map, "LArm_Hand", "LArm_Finger51", "LArm_Finger52", "LArm_Finger53");
		//right hand
		addFingerRelations(&map, "RArm_Hand", "RArm_Finger11", "RArm_Finger12", "RArm_Finger13");
		addFingerRelations(&map, "RArm_Hand", "RArm_Finger21", "RArm_Finger22", "RArm_Finger23");
		addFingerRelations(&map, "RArm_Hand", "RArm_Finger31", "RArm_Finger32", "RArm_Finger33");
		addFingerRelations(&map, "RArm_Hand", "RArm_Finger41", "RArm_Finger42", "RArm_Finger43");
		addFingerRelations(&map, "RArm_Hand", "RArm_Finger51", "RArm_Finger52", "RArm_Finger53");
		return map;
	}

	std::map<std::string, std::pair<std::string, std::string>> fingerRelations = makeFingerRelations();

	void initBoneTreeMap(NiNode* root) {
		const auto rt = static_cast<BSFlattenedBoneTree*>(root);

		boneTreeMap.clear();
		boneTreeVec.clear();

		for (auto i = 0; i < rt->numTransforms; i++) {
			_MESSAGE("BoneTree Init -> Push %s into position %d", rt->transforms[i].name.c_str(), i);
			boneTreeMap.insert({rt->transforms[i].name.c_str(), i});
			boneTreeVec.emplace_back(rt->transforms[i].name.c_str());
		}
	}

	// Native function that takes the 1st person skeleton weapon node and calculates the skeleton from upperarm down based off the offsetNode
	void update1StPersonArm(const PlayerCharacter* pc, NiNode** weapon, NiNode** offsetNode) {
		using func_t = decltype(&update1StPersonArm);
		RelocAddr<func_t> func(0xef6280);

		return func(pc, weapon, offsetNode);
	}

	int Skeleton::getBoneInMap(const std::string& boneName) {
		return boneTreeMap[boneName];
	}

	void Skeleton::rotateWorld(NiNode* nde) {
		Matrix44 mat;
		mat.data[0][0] = -1.0;
		mat.data[0][1] = 0.0;
		mat.data[0][2] = 0.0;
		mat.data[0][3] = 0.0;
		mat.data[1][0] = 0.0;
		mat.data[1][1] = -1.0;
		mat.data[1][2] = 0.0;
		mat.data[1][3] = 0.0;
		mat.data[2][0] = 0.0;
		mat.data[2][1] = 0.0;
		mat.data[2][2] = 1.0;
		mat.data[2][3] = 0.0;
		mat.data[3][0] = 0.0;
		mat.data[3][1] = 0.0;
		mat.data[3][2] = 0.0;
		mat.data[3][3] = 0.0;

		const auto local = (Matrix44*)&nde->m_worldTransform.rot;
		const Matrix44 result;
		Matrix44::matrixMultiply(local, &result, &mat);

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				nde->m_worldTransform.rot.data[i][j] = result.data[i][j];
			}
		}

		nde->m_worldTransform.pos.x = result.data[3][0];
		nde->m_worldTransform.pos.y = result.data[3][1];
		nde->m_worldTransform.pos.z = result.data[3][2];
	}

	void Skeleton::updatePos(NiNode* nde, const NiPoint3 offset) {
		nde->m_worldTransform.pos += offset;

		for (UInt16 i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				this->updatePos(nextNode, offset);
			}
		}
	}

	void Skeleton::setTime() {
		_prevTime = _timer;
		QueryPerformanceFrequency(&_freqCounter);
		QueryPerformanceCounter(&_timer);

		_frameTime = static_cast<float>(_timer.QuadPart - _prevTime.QuadPart) / _freqCounter.QuadPart;

		//also save last position at this time for anyone doing speed calcs
		_lastPos = _curPos;
	}

	void Skeleton::selfieSkelly() {
		// Projects the 3rd person body out in front of the player by offset amount
		if (!c_selfieMode || !_root) {
			return;
		}

		const float z = _root->m_localTransform.pos.z;
		const NiNode* body = _root->m_parent->GetAsNiNode();

		const NiPoint3 back = vec3Norm(NiPoint3(-_forwardDir.x, -_forwardDir.y, 0));
		const auto bodyDir = NiPoint3(0, 1, 0);

		Matrix44 mat;
		mat.makeIdentity();
		mat.rotateVectorVec(back, bodyDir);
		_root->m_localTransform.rot = mat.multiply43Left(body->m_worldTransform.rot.Transpose());
		_root->m_localTransform.pos = body->m_worldTransform.pos - this->getPosition();
		_root->m_localTransform.pos.y += g_config->selfieOutFrontDistance;
		_root->m_localTransform.pos.z = z;
	}

	NiNode* Skeleton::getWeaponNode() const {
		return getNode("Weapon", (*g_player)->firstPersonSkeleton);
	}

	NiNode* Skeleton::getPrimaryWandNode() const {
		return getNode("world_primaryWand.nif", _playerNodes->primaryUIAttachNode);
	}

	/**
	 * The throwable weapon is attached to the melee node but only exists if the player is actively throwing the weapon.
	 * @return found throwable node or nullptr if not
	 */
	NiNode* Skeleton::getThrowableWeaponNode() const {
		const auto meleeNode = _playerNodes->primaryMeleeWeaponOffsetNode;
		return meleeNode->m_children.m_emptyRunStart > 0
			? meleeNode->m_children.m_data[0]->GetAsNiNode()
			: nullptr;
	}

	/**
	 * Get the world position of the offhand index fingertip .
	 * Make small adjustment as the finger bone position is the center of the finger.
	 * Would be nice to know how long the bone is instead of magic numbers, didn't find a way so far.
	 */
	NiPoint3 Skeleton::getOffhandIndexFingerTipWorldPosition() const {
		const auto offhandIndexFinger = g_config->leftHandedMode ? "RArm_Finger23" : "LArm_Finger23";
		const auto boneTransform = reinterpret_cast<BSFlattenedBoneTree*>(_root)->transforms[getBoneInMap(offhandIndexFinger)];
		const auto forward = boneTransform.world.rot * NiPoint3(1, 0, 0);
		return boneTransform.world.pos + forward * (_inPowerArmor ? 3 : 1.8f);
	}

	void Skeleton::setupHead(NiNode* headNode, bool hideHead) {
		if (!headNode) {
			return;
		}

		headNode->m_localTransform.rot.data[0][0] = 0.967f;
		headNode->m_localTransform.rot.data[0][1] = -0.251f;
		headNode->m_localTransform.rot.data[0][2] = 0.047f;
		headNode->m_localTransform.rot.data[1][0] = 0.249f;
		headNode->m_localTransform.rot.data[1][1] = 0.967f;
		headNode->m_localTransform.rot.data[1][2] = 0.051f;
		headNode->m_localTransform.rot.data[2][0] = -0.058f;
		headNode->m_localTransform.rot.data[2][1] = -0.037f;
		headNode->m_localTransform.rot.data[2][2] = 0.998f;

		NiAVObject::NiUpdateData* ud = nullptr;
		headNode->UpdateWorldData(ud);
	}

	void Skeleton::insertSaveState(const std::string& name, NiNode* node) {
		NiNode* fn = getNode(name.c_str(), node);

		if (!fn) {
			_MESSAGE("cannot find node %s", name.c_str());
			return;
		}

		const auto it = _savedStates.find(fn->m_name.c_str());
		if (it == _savedStates.end()) {
			fn->m_localTransform.pos = _boneLocalDefault[fn->m_name.c_str()];
			_savedStates.emplace(fn->m_name.c_str(), fn->m_localTransform);
		} else {
			node->m_localTransform.pos = _boneLocalDefault[name];
			it->second = node->m_localTransform;
		}
		_MESSAGE("inserted %s", name.c_str());
	}

	void Skeleton::initLocalDefaults() {
		detectInPowerArmor();

		_boneLocalDefault.clear();
		if (_inPowerArmor) {
			_boneLocalDefault.insert({"Root", NiPoint3(-0.000000f, -0.000000f, 0.000000f)});
			_boneLocalDefault["COM"] = NiPoint3(-0.000031f, -3.749783f, 89.419518f);
			_boneLocalDefault["Pelvis"] = NiPoint3(0.000000f, 0.000000f, 0.000000f);
			_boneLocalDefault["LLeg_Thigh"] = NiPoint3(4.548744f, -1.329979f, 6.908315f);
			_boneLocalDefault["LLeg_Calf"] = NiPoint3(34.297993f, 0.000030f, 0.000008f);
			_boneLocalDefault["LLeg_Foot"] = NiPoint3(52.541229f, -0.000018f, -0.000018f);
			_boneLocalDefault["RLeg_Thigh"] = NiPoint3(4.547630f, -1.324275f, -6.897992f);
			_boneLocalDefault["RLeg_Calf"] = NiPoint3(34.297920f, 0.000020f, 0.000029f);
			_boneLocalDefault["RLeg_Foot"] = NiPoint3(52.540794f, 0.000001f, -0.000002f);
			_boneLocalDefault["SPINE1"] = NiPoint3(5.750534f, -0.002948f, -0.000008f);
			_boneLocalDefault["SPINE2"] = NiPoint3(5.625465f, 0.000000f, 0.000000f);
			_boneLocalDefault["Chest"] = NiPoint3(5.536583f, -0.000000f, 0.000000f);
			_boneLocalDefault["LArm_Collarbone"] = NiPoint3(22.192039f, 0.348167f, 1.004177f);
			_boneLocalDefault["LArm_UpperArm"] = NiPoint3(14.598365f, 0.000075f, 0.000146f);
			_boneLocalDefault["LArm_ForeArm1"] = NiPoint3(19.536854f, 0.419780f, 0.045785f);
			_boneLocalDefault["LArm_ForeArm2"] = NiPoint3(0.000168f, 0.000150f, 0.000153f);
			_boneLocalDefault["LArm_ForeArm3"] = NiPoint3(10.000494f, 0.000162f, -0.000004f);
			_boneLocalDefault["LArm_Hand"] = NiPoint3(26.964424f, 0.000185f, 0.000400f);
			_boneLocalDefault["RArm_Collarbone"] = NiPoint3(22.191887f, 0.348147f, -1.003999f);
			_boneLocalDefault["RArm_UpperArm"] = NiPoint3(14.598806f, 0.000044f, 0.000044f);
			_boneLocalDefault["RArm_ForeArm1"] = NiPoint3(19.536560f, 0.419853f, -0.046183f);
			_boneLocalDefault["RArm_ForeArm2"] = NiPoint3(-0.000053f, -0.000068f, -0.000137f);
			_boneLocalDefault["RArm_ForeArm3"] = NiPoint3(10.000502f, -0.000104f, 0.000120f);
			_boneLocalDefault["RArm_Hand"] = NiPoint3(26.964560f, 0.000064f, 0.001247f);
			_boneLocalDefault["Neck"] = NiPoint3(24.293526f, -2.841563f, 0.000019f);
			_boneLocalDefault["Head"] = NiPoint3(8.224354f, 0.0, 0.0);
		} else {
			_boneLocalDefault.insert({"Root", NiPoint3(-0.000000f, -0.000000f, 0.000000f)});
			_boneLocalDefault["COM"] = NiPoint3(2.0, 1.5, 68.911301f);
			_boneLocalDefault["Pelvis"] = NiPoint3(0.000000f, 0.000000f, 0.000000f);
			_boneLocalDefault["LLeg_Thigh"] = NiPoint3(0.000008f, 0.000431f, 6.615070f);
			_boneLocalDefault["LLeg_Calf"] = NiPoint3(31.595196f, 0.000021f, -0.000031f);
			_boneLocalDefault["LLeg_Foot"] = NiPoint3(31.942888f, -0.000018f, -0.000018f);
			_boneLocalDefault["RLeg_Thigh"] = NiPoint3(0.000008f, 0.000431f, -6.615068f);
			_boneLocalDefault["RLeg_Calf"] = NiPoint3(31.595139f, 0.000020f, 0.000029f);
			_boneLocalDefault["RLeg_Foot"] = NiPoint3(31.942570f, 0.000001f, -0.000002f);
			_boneLocalDefault["SPINE1"] = NiPoint3(3.791992f, -0.002948f, -0.000008f);
			_boneLocalDefault["SPINE2"] = NiPoint3(8.704666f, 0.000000f, 0.000000f);
			_boneLocalDefault["Chest"] = NiPoint3(9.956283f, -0.000000f, 0.000000f);
			_boneLocalDefault["LArm_Collarbone"] = NiPoint3(19.153227f, -0.510421f, 1.695077f);
			_boneLocalDefault["LArm_UpperArm"] = NiPoint3(12.536572f, 0.000003f, 0.000000f);
			_boneLocalDefault["LArm_ForeArm1"] = NiPoint3(17.968307f, 0.000019f, 0.000007f);
			_boneLocalDefault["LArm_ForeArm2"] = NiPoint3(6.151599f, -0.000022f, -0.000045f);
			_boneLocalDefault["LArm_ForeArm3"] = NiPoint3(6.151586f, -0.000068f, 0.000016f);
			_boneLocalDefault["LArm_Hand"] = NiPoint3(6.151584f, 0.000033f, -0.000065f);
			_boneLocalDefault["RArm_Collarbone"] = NiPoint3(19.153219f, -0.510418f, -1.695124f);
			_boneLocalDefault["RArm_UpperArm"] = NiPoint3(12.534259f, 0.000007f, -0.000013f);
			_boneLocalDefault["RArm_ForeArm1"] = NiPoint3(17.970503f, 0.000103f, -0.000056f);
			_boneLocalDefault["RArm_ForeArm2"] = NiPoint3(6.152768f, 0.000040f, -0.000039f);
			_boneLocalDefault["RArm_ForeArm3"] = NiPoint3(6.152856f, -0.000009f, -0.000091f);
			_boneLocalDefault["RArm_Hand"] = NiPoint3(6.152939f, 0.000044f, -0.000029f);
			_boneLocalDefault["Neck"] = NiPoint3(22.084f, -3.767f, 0.0);
			_boneLocalDefault["Head"] = NiPoint3(8.224354f, 0.0, 0.0);
		}
	}

	void Skeleton::saveStatesTree(NiNode* node) {
		initLocalDefaults();

		if (!node) {
			_MESSAGE("Cannot save states Tree");
			return;
		}

		insertSaveState("Root", node);
		insertSaveState("COM", node);
		insertSaveState("Pelvis", node);
		insertSaveState("LLeg_Thigh", node);
		insertSaveState("LLeg_Calf", node);
		insertSaveState("LLeg_Foot", node);
		insertSaveState("RLeg_Thigh", node);
		insertSaveState("RLeg_Calf", node);
		insertSaveState("RLeg_Foot", node);
		insertSaveState("SPINE1", node);
		insertSaveState("SPINE2", node);
		insertSaveState("Chest", node);
		insertSaveState("LArm_Collarbone", node);
		insertSaveState("LArm_UpperArm", node);
		insertSaveState("LArm_ForeArm1", node);
		insertSaveState("LArm_ForeArm2", node);
		insertSaveState("LArm_ForeArm3", node);
		insertSaveState("LArm_Hand", node);
		insertSaveState("RArm_Collarbone", node);
		insertSaveState("RArm_UpperArm", node);
		insertSaveState("RArm_ForeArm1", node);
		insertSaveState("RArm_ForeArm2", node);
		insertSaveState("RArm_ForeArm3", node);
		insertSaveState("RArm_Hand", node);
		insertSaveState("Neck", node);
		insertSaveState("Head", node);
	}

	void Skeleton::restoreLocals(NiNode* node) {
		if (!node || !node->m_name) {
			_MESSAGE("cannot restore locals");
			return;
		}

		const std::string name(node->m_name.c_str());
		const auto it = _savedStates.find(name);
		if (it != _savedStates.end()) {
			node->m_localTransform = it->second;
		}

		for (UInt16 i = 0; i < node->m_children.m_emptyRunStart; ++i) {
			if (const auto nextNode = node->m_children.m_data[i] ? node->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				this->restoreLocals(nextNode);
			}
		}
	}

	bool Skeleton::setNodes() {
		QueryPerformanceFrequency(&_freqCounter);
		QueryPerformanceCounter(&_timer);

		std::srand(static_cast<unsigned int>(time(nullptr)));

		_prevSpeed = 0.0;

		_playerNodes = reinterpret_cast<f4vr::PlayerNodes*>(reinterpret_cast<char*>(*g_player) + 0x6E0);

		if (!_playerNodes) {
			_MESSAGE("player nodes not set");
			return false;
		}

		_curPos = (*g_playerCamera)->cameraNode->m_worldTransform.pos;

		setCommonNode();

		if (!_common) {
			_MESSAGE("Common Node Not Set");
			return false;
		}

		_rightHand = getNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		_leftHand = getNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		if (!_rightHand || !_leftHand) {
			return false;
		}

		_rightHandPrevFrame = _rightHand->m_worldTransform;
		_leftHandPrevFrame = _leftHand->m_worldTransform;

		NiNode* screenNode = _playerNodes->ScreenNode;

		if (g_pipboy) {
			g_pipboy->onSetNodes();
		}

		_spine = getNode("SPINE2", _root);
		_chest = getNode("Chest", _root);

		_MESSAGE("common node = %016I64X", _common);
		_MESSAGE("righthand node = %016I64X", _rightHand);
		_MESSAGE("lefthand node = %016I64X", _leftHand);

		// Setup Arms
		const std::vector<std::pair<BSFixedString, NiAVObject**>> armNodes = {
			{"RArm_Collarbone", &_rightArm.shoulder},
			{"RArm_UpperArm", &_rightArm.upper},
			{"RArm_UpperTwist1", &_rightArm.upperT1},
			{"RArm_ForeArm1", &_rightArm.forearm1},
			{"RArm_ForeArm2", &_rightArm.forearm2},
			{"RArm_ForeArm3", &_rightArm.forearm3},
			{"RArm_Hand", &_rightArm.hand},
			{"LArm_Collarbone", &_leftArm.shoulder},
			{"LArm_UpperArm", &_leftArm.upper},
			{"LArm_UpperTwist1", &_leftArm.upperT1},
			{"LArm_ForeArm1", &_leftArm.forearm1},
			{"LArm_ForeArm2", &_leftArm.forearm2},
			{"LArm_ForeArm3", &_leftArm.forearm3},
			{"LArm_Hand", &_leftArm.hand}
		};

		for (const auto& [name, node] : armNodes) {
			*node = _common->GetObjectByName(&name);
		}
		_MESSAGE("finished set arm nodes");

		_handBones = handOpen;

		// setup hand bones to openvr button mapping
		_handBonesButton["LArm_Finger11"] = vr::k_EButton_SteamVR_Touchpad;
		_handBonesButton["LArm_Finger12"] = vr::k_EButton_SteamVR_Touchpad;
		_handBonesButton["LArm_Finger13"] = vr::k_EButton_SteamVR_Touchpad;
		_handBonesButton["LArm_Finger21"] = vr::k_EButton_SteamVR_Trigger;
		_handBonesButton["LArm_Finger22"] = vr::k_EButton_SteamVR_Trigger;
		_handBonesButton["LArm_Finger23"] = vr::k_EButton_SteamVR_Trigger;
		_handBonesButton["LArm_Finger31"] = vr::k_EButton_Grip;
		_handBonesButton["LArm_Finger32"] = vr::k_EButton_Grip;
		_handBonesButton["LArm_Finger33"] = vr::k_EButton_Grip;
		_handBonesButton["LArm_Finger41"] = vr::k_EButton_Grip;
		_handBonesButton["LArm_Finger42"] = vr::k_EButton_Grip;
		_handBonesButton["LArm_Finger43"] = vr::k_EButton_Grip;
		_handBonesButton["LArm_Finger51"] = vr::k_EButton_Grip;
		_handBonesButton["LArm_Finger52"] = vr::k_EButton_Grip;
		_handBonesButton["LArm_Finger53"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger11"] = vr::k_EButton_SteamVR_Touchpad;
		_handBonesButton["RArm_Finger12"] = vr::k_EButton_SteamVR_Touchpad;
		_handBonesButton["RArm_Finger13"] = vr::k_EButton_SteamVR_Touchpad;
		_handBonesButton["RArm_Finger21"] = vr::k_EButton_SteamVR_Trigger;
		_handBonesButton["RArm_Finger22"] = vr::k_EButton_SteamVR_Trigger;
		_handBonesButton["RArm_Finger23"] = vr::k_EButton_SteamVR_Trigger;
		_handBonesButton["RArm_Finger31"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger32"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger33"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger41"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger42"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger43"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger51"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger52"] = vr::k_EButton_Grip;
		_handBonesButton["RArm_Finger53"] = vr::k_EButton_Grip;

		_savedStates.clear();
		saveStatesTree(_root->m_parent->GetAsNiNode());
		_MESSAGE("finished saving tree");

		initBoneTreeMap(_root);
		return true;
	}

	NiPoint3 Skeleton::getPosition() {
		//	_curPos = _playerNodes->UprightHmdNode->m_worldTransform.pos;
		_curPos = (*g_playerCamera)->cameraNode->m_worldTransform.pos;

		return _curPos;
	}

	// below takes the two vectors from hmd to each hand and sums them to determine a center axis in which to see how much the hmd has rotated
	// A secondary angle is also calculated which is 90 degrees on the z axis up to handle when the hands are approaching the z plane of the hmd
	// this helps keep the body stable through a wide range of hand poses
	// this still struggles with hands close to the face and with one hand low and one hand high.    Will need to take progs advice to add weights
	// to these positions which i'll do at a later date.

	float Skeleton::getNeckYaw() const {
		if (!_playerNodes) {
			_MESSAGE("player nodes not set in neck yaw");
			return 0.0;
		}

		const NiPoint3 pos = _playerNodes->UprightHmdNode->m_worldTransform.pos;
		const NiPoint3 hmdToLeft = _playerNodes->SecondaryWandNode->m_worldTransform.pos - pos;
		const NiPoint3 hmdToRight = _playerNodes->primaryWandNode->m_worldTransform.pos - pos;
		float weight = 1.0f;

		if (vec3Len(hmdToLeft) < 10.0f || vec3Len(hmdToRight) < 10.0f) {
			return 0.0;
		}

		// handle excessive angles when hand is above the head.
		if (hmdToLeft.z > 0) {
			weight = (std::max)(weight - 0.05f * hmdToLeft.z, 0.0f);
		}

		if (hmdToRight.z > 0) {
			weight = (std::max)(weight - 0.05f * hmdToRight.z, 0.0f);
		}

		// hands moving across the chest rotate too much.   try to handle with below
		// wp = parWp + parWr * lp =>   lp = (wp - parWp) * parWr'
		const NiPoint3 locLeft = _playerNodes->HmdNode->m_worldTransform.rot.Transpose() * hmdToLeft;
		const NiPoint3 locRight = _playerNodes->HmdNode->m_worldTransform.rot.Transpose() * hmdToRight;

		if (locLeft.x > locRight.x) {
			const float delta = locRight.x - locLeft.x;
			weight = (std::max)(weight + 0.02f * delta, 0.0f);
		}

		const NiPoint3 sum = hmdToRight + hmdToLeft;

		const NiPoint3 forwardDir = vec3Norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * vec3Norm(sum)); // rotate sum to local hmd space to get the proper angle
		const NiPoint3 hmdForwardDir = vec3Norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * _playerNodes->HmdNode->m_localTransform.pos);

		const float anglePrime = atan2f(forwardDir.x, forwardDir.y);
		const float angleSec = atan2f(forwardDir.x, forwardDir.z);

		const float pitchDiff = atan2f(hmdForwardDir.y, hmdForwardDir.z) - atan2f(forwardDir.z, forwardDir.y);

		const float angleFinal = fabs(pitchDiff) > degreesToRads(80.0f) ? angleSec : anglePrime;
		return std::clamp(-angleFinal * weight, degreesToRads(-50.0f), degreesToRads(50.0f));
	}

	float Skeleton::getNeckPitch() const {
		const NiPoint3& lookDir = vec3Norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * _playerNodes->HmdNode->m_localTransform.pos);
		return atan2f(lookDir.y, lookDir.z);
	}

	float Skeleton::getBodyPitch() const {
		constexpr float basePitch = 105.3f;
		constexpr float weight = 0.1f;

		const float curHeight = g_config->playerHeight;
		const float heightCalc = std::abs((curHeight - _playerNodes->UprightHmdNode->m_localTransform.pos.z) / curHeight);

		const float angle = heightCalc * (basePitch + weight * radsToDegrees(getNeckPitch()));

		return degreesToRads(angle);
	}

	void Skeleton::setUnderHMD(float groundHeight) {
		detectInPowerArmor();

		if (g_config->disableSmoothMovement) {
			_playerNodes->playerworldnode->m_localTransform.pos.z = _inPowerArmor
				? g_config->PACameraHeight + c_dynamicCameraHeight
				: g_config->cameraHeight + c_dynamicCameraHeight;
			updateDown(_playerNodes->playerworldnode, true);
		}

		Matrix44 mat;
		mat = 0.0;
		//		float y    = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][1];  // Middle column is y vector.   Grab just x and y portions and make a unit vector.    This can be used to rotate body to always be orientated with the hmd.
		//	float x    = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][0];  //  Later will use this vector as the basis for the rest of the IK
		const float z = _root->m_localTransform.pos.z;
		//	float z = groundHeight;

		const float neckYaw = getNeckYaw();
		const float neckPitch = getNeckPitch();

		Quaternion qa;
		qa.setAngleAxis(-neckPitch, NiPoint3(-1, 0, 0));

		mat = qa.getRot();
		const NiMatrix43 newRot = mat.multiply43Left(_playerNodes->HmdNode->m_localTransform.rot);

		_forwardDir = rotateXY(NiPoint3(newRot.data[1][0], newRot.data[1][1], 0), neckYaw * 0.7f);
		_sidewaysRDir = NiPoint3(_forwardDir.y, -_forwardDir.x, 0);

		NiNode* body = _root->m_parent->GetAsNiNode();
		body->m_localTransform.pos *= 0.0f;
		body->m_worldTransform.pos.x = this->getPosition().x;
		body->m_worldTransform.pos.y = this->getPosition().y;
		body->m_worldTransform.pos.z += _playerNodes->playerworldnode->m_localTransform.pos.z;

		const NiPoint3 back = vec3Norm(NiPoint3(_forwardDir.x, _forwardDir.y, 0));
		const auto bodyDir = NiPoint3(0, 1, 0);

		mat.rotateVectorVec(back, bodyDir);
		_root->m_localTransform.rot = mat.multiply43Left(body->m_worldTransform.rot.Transpose());
		_root->m_localTransform.pos = body->m_worldTransform.pos - getPosition();
		_root->m_localTransform.pos.z = z;
		//_root->m_localTransform.pos *= 0.0f;
		//_root->m_localTransform.pos.y = g_config->playerOffset_forward - 6.0f;
		_root->m_localTransform.scale = g_config->playerHeight / defaultCameraHeight; // set scale based off specified user height
	}

	void Skeleton::setBodyPosture() {
		const float neckPitch = getNeckPitch();
		const float bodyPitch = _inPowerArmor ? getBodyPitch() : getBodyPitch() / 1.2f;

		const NiNode* camera = (*g_playerCamera)->cameraNode;
		NiNode* com = getNode("COM", _root);
		const NiNode* neck = getNode("Neck", _root);
		NiNode* spine = getNode("SPINE1", _root);

		_leftKneePos = getNode("LLeg_Calf", com)->m_worldTransform.pos;
		_rightKneePos = getNode("RLeg_Calf", com)->m_worldTransform.pos;

		com->m_localTransform.pos.x = 0.0;
		com->m_localTransform.pos.y = 0.0;

		const float z_adjust = (_inPowerArmor ? g_config->powerArmor_up : g_config->playerOffset_up) - cosf(neckPitch) * (5.0 * _root->m_localTransform.scale);
		//float z_adjust = g_config->playerOffset_up - cosf(neckPitch) * (5.0 * _root->m_localTransform.scale);
		const auto neckAdjust = NiPoint3(-_forwardDir.x * g_config->playerOffset_forward / 2, -_forwardDir.y * g_config->playerOffset_forward / 2, z_adjust);
		const NiPoint3 neckPos = camera->m_worldTransform.pos + neckAdjust;

		_torsoLen = vec3Len(neck->m_worldTransform.pos - com->m_worldTransform.pos);

		const NiPoint3 hmdToHip = neckPos - com->m_worldTransform.pos;
		const auto dir = NiPoint3(-_forwardDir.x, -_forwardDir.y, 0);

		const float dist = tanf(bodyPitch) * vec3Len(hmdToHip);
		NiPoint3 tmpHipPos = com->m_worldTransform.pos + dir * (dist / vec3Len(dir));
		tmpHipPos.z = com->m_worldTransform.pos.z;

		const NiPoint3 hmdToNewHip = tmpHipPos - neckPos;
		const NiPoint3 newHipPos = neckPos + hmdToNewHip * (_torsoLen / vec3Len(hmdToNewHip));

		const NiPoint3 newPos = com->m_localTransform.pos + _root->m_worldTransform.rot.Transpose() * (newHipPos - com->m_worldTransform.pos);
		const float offsetFwd = _inPowerArmor ? g_config->powerArmor_forward : g_config->playerOffset_forward;
		com->m_localTransform.pos.y += newPos.y + offsetFwd;
		com->m_localTransform.pos.z = _inPowerArmor ? newPos.z / 1.7 : newPos.z / 1.5;
		//com->m_localTransform.pos.z -= _inPowerArmor ? g_config->powerArmor_up + g_config->PACameraHeight : 0.0f;
		NiNode* body = _root->m_parent->GetAsNiNode();
		_inPowerArmor
			? body->m_worldTransform.pos.z = body->m_worldTransform.pos.z - (g_config->PACameraHeight + g_config->PARootOffset)
			: body->m_worldTransform.pos.z = body->m_worldTransform.pos.z - (g_config->cameraHeight + g_config->rootOffset);

		Matrix44 rot;
		rot.rotateVectorVec(neckPos - tmpHipPos, hmdToHip);
		const NiMatrix43 mat = rot.multiply43Left(spine->m_parent->m_worldTransform.rot.Transpose());
		rot.makeTransformMatrix(mat, NiPoint3(0, 0, 0));
		spine->m_localTransform.rot = rot.multiply43Right(spine->m_worldTransform.rot);
	}

	void Skeleton::setKneePos() {
		NiNode* lKnee = getNode("LLeg_Calf", _root);
		NiNode* rKnee = getNode("RLeg_Calf", _root);

		if (!lKnee || !rKnee) {
			return;
		}

		lKnee->m_worldTransform.pos.z = _leftKneePos.z;
		rKnee->m_worldTransform.pos.z = _rightKneePos.z;

		_leftKneePos = lKnee->m_worldTransform.pos;
		_rightKneePos = rKnee->m_worldTransform.pos;

		updateDown(lKnee, false);
		updateDown(rKnee, false);
	}

	void Skeleton::fixArmor() const {
		NiNode* lPauldron = getNode("L_Pauldron", _root);
		NiNode* rPauldron = getNode("R_Pauldron", _root);

		if (!lPauldron || !rPauldron) {
			return;
		}

		//float delta = getNode("LArm_Collarbone", _root)->m_worldTransform.pos.z - _root->m_worldTransform.pos.z;
		const float delta = getNode("LArm_UpperArm", _root)->m_worldTransform.pos.z - _root->m_worldTransform.pos.z;
		float delta2 = getNode("RArm_UpperArm", _root)->m_worldTransform.pos.z - _root->m_worldTransform.pos.z;
		if (lPauldron) {
			lPauldron->m_localTransform.pos.z = delta - 15.0f;
		}
		if (rPauldron) {
			rPauldron->m_localTransform.pos.z = delta - 15.0f;
		}
	}

	void Skeleton::walk() {
		NiNode* lHip = getNode("LLeg_Thigh", _root);
		NiNode* rHip = getNode("RLeg_Thigh", _root);

		if (!lHip || !rHip) {
			return;
		}

		const NiNode* lKnee = getNode("LLeg_Calf", lHip);
		const NiNode* rKnee = getNode("RLeg_Calf", rHip);
		NiNode* lFoot = getNode("LLeg_Foot", lHip);
		NiNode* rFoot = getNode("RLeg_Foot", rHip);

		if (!lKnee || !rKnee || !lFoot || !rFoot) {
			return;
		}

		// move feet closer together
		const NiPoint3 leftToRight = _inPowerArmor
			? (rFoot->m_worldTransform.pos - lFoot->m_worldTransform.pos) * -0.15f
			: (rFoot->m_worldTransform.pos - lFoot->m_worldTransform.pos) * 0.3f;
		lFoot->m_worldTransform.pos += leftToRight;
		rFoot->m_worldTransform.pos -= leftToRight;

		// want to calculate direction vector first.     Will only concern with x-y vector to start.
		NiPoint3 lastPos = _lastPos;
		NiPoint3 curPos = _curPos;
		curPos.z = 0;
		lastPos.z = 0;

		NiPoint3 dir = curPos - lastPos;

		float curSpeed = std::clamp(abs(vec3Len(dir)) / _frameTime, 0.0, 350.0);
		if (_prevSpeed > 20.0) {
			curSpeed = (curSpeed + _prevSpeed) / 2;
		}

		const float stepTime = std::clamp(cos(curSpeed / 140.0), 0.28, 0.50);
		dir = vec3Norm(dir);

		// if decelerating reset target
		if (curSpeed - _prevSpeed < -20.0f) {
			_walkingState = 3;
		}

		_prevSpeed = curSpeed;

		static float spineAngle = 0.0;

		// setup current walking state based on velocity and previous state
		if (!c_jumping) {
			switch (_walkingState) {
			case 0: {
				if (curSpeed >= 35.0) {
					_walkingState = 1; // start walking
					_footStepping = std::rand() % 2 + 1; // pick a random foot to take a step
					_stepDir = dir;
					_stepTimeinStep = stepTime;
					_delayFrame = 2;

					if (_footStepping == 1) {
						_rightFootTarget = rFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 1.5);
						_rightFootStart = rFoot->m_worldTransform.pos;
						_leftFootTarget = lFoot->m_worldTransform.pos;
						_leftFootStart = lFoot->m_worldTransform.pos;
						_leftFootPos = _leftFootStart;
						_rightFootPos = _rightFootStart;
					} else {
						_rightFootTarget = rFoot->m_worldTransform.pos;
						_rightFootStart = rFoot->m_worldTransform.pos;
						_leftFootTarget = lFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 1.5);
						_leftFootStart = lFoot->m_worldTransform.pos;
						_leftFootPos = _leftFootStart;
						_rightFootPos = _rightFootStart;
					}
					_currentStepTime = stepTime / 2;
					break;
				}
				_currentStepTime = 0.0;
				_footStepping = 0;
				spineAngle = 0.0;
				break;
			}
			case 1: {
				if (curSpeed < 20.0) {
					_walkingState = 2; // begin process to stop walking
					_currentStepTime = 0.0;
				}
				break;
			}
			case 2: {
				if (curSpeed >= 20.0) {
					_walkingState = 1; // resume walking
					_currentStepTime = 0.0;
				}
				break;
			}
			case 3: {
				_stepDir = dir;
				if (_footStepping == 1) {
					_rightFootTarget = rFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 0.1f);
				} else {
					_leftFootTarget = lFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 0.1f);
				}
				_walkingState = 1;
				break;
			}
			default: {
				_walkingState = 0;
				break;
			}
			}
		} else {
			_walkingState = 0;
		}

		if (_walkingState == 0) {
			// we're standing still so just set foot positions accordingly.
			_leftFootPos = lFoot->m_worldTransform.pos;
			_rightFootPos = rFoot->m_worldTransform.pos;
			_leftFootPos.z = _root->m_worldTransform.pos.z;
			_rightFootPos.z = _root->m_worldTransform.pos.z;

			return;
		}
		if (_walkingState == 1) {
			NiPoint3 dirOffset = dir - _stepDir;
			const float dot = vec3Dot(dir, _stepDir);
			const float scale = (std::min)(curSpeed * stepTime * 1.5, 140.0);
			dirOffset = dirOffset * scale;

			int sign = 1;

			_currentStepTime += _frameTime;

			const float frameStep = _frameTime / _stepTimeinStep;
			const float interp = std::clamp(frameStep * (_currentStepTime / _frameTime), 0.0, 1.0);

			if (_footStepping == 1) {
				sign = -1;
				if (dot < 0.9) {
					if (!_delayFrame) {
						_rightFootTarget += dirOffset;
						_stepDir = dir;
						_delayFrame = 2;
					} else {
						_delayFrame--;
					}
				} else {
					_delayFrame = _delayFrame == 2 ? _delayFrame : _delayFrame + 1;
				}
				_rightFootTarget.z = _root->m_worldTransform.pos.z;
				_rightFootStart.z = _root->m_worldTransform.pos.z;
				_rightFootPos = _rightFootStart + (_rightFootTarget - _rightFootStart) * interp;
				const float stepAmount = std::clamp(vec3Len(_rightFootTarget - _rightFootStart) / 150.0, 0.0, 1.0);
				const float stepHeight = (std::max)(stepAmount * 9.0, 1.0);
				const float up = sinf(interp * PI) * stepHeight;
				_rightFootPos.z += up;
			} else {
				if (dot < 0.9) {
					if (!_delayFrame) {
						_leftFootTarget += dirOffset;
						_stepDir = dir;
						_delayFrame = 2;
					} else {
						_delayFrame--;
					}
				} else {
					_delayFrame = _delayFrame == 2 ? _delayFrame : _delayFrame + 1;
				}
				_leftFootTarget.z = _root->m_worldTransform.pos.z;
				_leftFootStart.z = _root->m_worldTransform.pos.z;
				_leftFootPos = _leftFootStart + (_leftFootTarget - _leftFootStart) * interp;
				const float stepAmount = std::clamp(vec3Len(_leftFootTarget - _leftFootStart) / 150.0, 0.0, 1.0);
				const float stepHeight = (std::max)(stepAmount * 9.0, 1.0);
				const float up = sinf(interp * PI) * stepHeight;
				_leftFootPos.z += up;
			}

			spineAngle = sign * sinf(interp * PI) * 3.0;
			Matrix44 rot;

			rot.setEulerAngles(degreesToRads(spineAngle), 0.0, 0.0);
			_spine->m_localTransform.rot = rot.multiply43Left(_spine->m_localTransform.rot);

			if (_currentStepTime > stepTime) {
				_currentStepTime = 0.0;
				_stepDir = dir;
				_stepTimeinStep = stepTime;
				//_MESSAGE("%2f %2f", curSpeed, stepTime);

				if (_footStepping == 1) {
					_footStepping = 2;
					_leftFootTarget = lFoot->m_worldTransform.pos + _stepDir * scale;
					_leftFootStart = _leftFootPos;
				} else {
					_footStepping = 1;
					_rightFootTarget = rFoot->m_worldTransform.pos + _stepDir * scale;
					_rightFootStart = _rightFootPos;
				}
			}
			return;
		}
		if (_walkingState == 2) {
			_leftFootPos = lFoot->m_worldTransform.pos;
			_rightFootPos = rFoot->m_worldTransform.pos;
			_walkingState = 0;
		}
	}

	void Skeleton::setLegs() {
		Matrix44 rotatedM;

		NiNode* lHip = getNode("LLeg_Thigh", _root);
		NiNode* rHip = getNode("RLeg_Thigh", _root);
		NiNode* lKnee = getNode("LLeg_Calf", lHip);
		NiNode* rKnee = getNode("RLeg_Calf", rHip);
		const NiNode* lFoot = getNode("LLeg_Foot", lHip);
		const NiNode* rFoot = getNode("RLeg_Foot", rHip);

		NiPoint3 pos = _leftKneePos - lHip->m_worldTransform.pos;
		NiPoint3 uLocalDir = lHip->m_worldTransform.rot.Transpose() * vec3Norm(pos) / lHip->m_worldTransform.scale;
		rotatedM.rotateVectorVec(uLocalDir, lKnee->m_localTransform.pos);
		lHip->m_localTransform.rot = rotatedM.multiply43Left(lHip->m_localTransform.rot);

		pos = _rightKneePos - rHip->m_worldTransform.pos;
		uLocalDir = rHip->m_worldTransform.rot.Transpose() * vec3Norm(pos) / rHip->m_worldTransform.scale;
		rotatedM.rotateVectorVec(uLocalDir, rKnee->m_localTransform.pos);
		rHip->m_localTransform.rot = rotatedM.multiply43Left(rHip->m_localTransform.rot);

		updateDown(lHip, true);
		updateDown(rHip, true);

		// now calves
		pos = _leftFootPos - lKnee->m_worldTransform.pos;
		uLocalDir = lKnee->m_worldTransform.rot.Transpose() * vec3Norm(pos) / lKnee->m_worldTransform.scale;
		rotatedM.rotateVectorVec(uLocalDir, lFoot->m_localTransform.pos);
		lKnee->m_localTransform.rot = rotatedM.multiply43Left(lKnee->m_localTransform.rot);

		pos = _rightFootPos - rKnee->m_worldTransform.pos;
		uLocalDir = rKnee->m_worldTransform.rot.Transpose() * vec3Norm(pos) / rKnee->m_worldTransform.scale;
		rotatedM.rotateVectorVec(uLocalDir, rFoot->m_localTransform.pos);
		rKnee->m_localTransform.rot = rotatedM.multiply43Left(rKnee->m_localTransform.rot);
	}

	// adapted solver from VRIK.  Thanks prog!
	void Skeleton::setSingleLeg(const bool isLeft) const {
		Matrix44 rotMat;

		NiNode* footNode = isLeft ? getNode("LLeg_Foot", _root) : getNode("RLeg_Foot", _root);
		NiNode* kneeNode = isLeft ? getNode("LLeg_Calf", _root) : getNode("RLeg_Calf", _root);
		NiNode* hipNode = isLeft ? getNode("LLeg_Thigh", _root) : getNode("RLeg_Thigh", _root);

		const NiPoint3 footPos = isLeft ? _leftFootPos : _rightFootPos;
		const NiPoint3 hipPos = hipNode->m_worldTransform.pos;

		const NiPoint3 footToHip = hipNode->m_worldTransform.pos - footPos;

		auto rotV = NiPoint3(0, 1, 0);
		if (_inPowerArmor) {
			rotV.y = 0;
			rotV.z = isLeft ? 1 : -1;
		}
		const NiPoint3 hipDir = hipNode->m_worldTransform.rot * rotV;
		const NiPoint3 xDir = vec3Norm(footToHip);
		const NiPoint3 yDir = vec3Norm(hipDir - xDir * vec3Dot(hipDir, xDir));

		const float thighLenOrig = vec3Len(kneeNode->m_localTransform.pos);
		const float calfLenOrig = vec3Len(footNode->m_localTransform.pos);
		float thighLen = thighLenOrig;
		float calfLen = calfLenOrig;

		const float ftLen = (std::max)(vec3Len(footToHip), 0.1f);

		if (ftLen > thighLen + calfLen) {
			const float diff = ftLen - thighLen - calfLen;
			const float ratio = calfLen / (calfLen + thighLen);
			calfLen += ratio * diff + 0.1f;
			thighLen += (1.0f - ratio) * diff + 0.1f;
		}
		// Use the law of cosines to calculate the angle the calf must bend to reach the knee position
		// In cases where this is impossible (foot too close to thigh), then set calfLen = thighLen so
		// there is always a solution
		float footAngle = acosf((calfLen * calfLen + ftLen * ftLen - thighLen * thighLen) / (2 * calfLen * ftLen));
		if (std::isnan(footAngle) || std::isinf(footAngle)) {
			calfLen = thighLen = (thighLenOrig + calfLenOrig) / 2.0f;
			footAngle = acosf((calfLen * calfLen + ftLen * ftLen - thighLen * thighLen) / (2 * calfLen * ftLen));
		}
		// Get the desired world coordinate of the knee
		const float xDist = cosf(footAngle) * calfLen;
		const float yDist = sinf(footAngle) * calfLen;
		const NiPoint3 kneePos = footPos + xDir * xDist + yDir * yDist;

		const NiPoint3 pos = kneePos - hipPos;
		NiPoint3 uLocalDir = hipNode->m_worldTransform.rot.Transpose() * vec3Norm(pos) / hipNode->m_worldTransform.scale;
		rotMat.rotateVectorVec(uLocalDir, kneeNode->m_localTransform.pos);
		hipNode->m_localTransform.rot = rotMat.multiply43Left(hipNode->m_localTransform.rot);

		rotMat.makeTransformMatrix(hipNode->m_localTransform.rot, NiPoint3(0, 0, 0));
		const NiMatrix43 hipWR = rotMat.multiply43Left(hipNode->m_parent->m_worldTransform.rot);

		rotMat.makeTransformMatrix(kneeNode->m_localTransform.rot, NiPoint3(0, 0, 0));
		NiMatrix43 calfWR = rotMat.multiply43Left(hipWR);

		uLocalDir = calfWR.Transpose() * vec3Norm(footPos - kneePos) / kneeNode->m_worldTransform.scale;
		rotMat.rotateVectorVec(uLocalDir, footNode->m_localTransform.pos);
		kneeNode->m_localTransform.rot = rotMat.multiply43Left(kneeNode->m_localTransform.rot);

		rotMat.makeTransformMatrix(kneeNode->m_localTransform.rot, NiPoint3(0, 0, 0));
		calfWR = rotMat.multiply43Left(hipWR);

		// Calculate Clp:  Cwp = Twp + Twr * (Clp * Tws) = kneePos   ===>   Clp = Twr' * (kneePos - Twp) / Tws
		//LPOS(nodeCalf) = Twr.Transpose() * (kneePos - thighPos) / WSCL(nodeThigh);
		kneeNode->m_localTransform.pos = hipWR.Transpose() * (kneePos - hipPos) / hipNode->m_worldTransform.scale;
		if (vec3Len(kneeNode->m_localTransform.pos) > thighLenOrig) {
			kneeNode->m_localTransform.pos = vec3Norm(kneeNode->m_localTransform.pos) * thighLenOrig;
		}

		// Calculate Flp:  Fwp = Cwp + Cwr * (Flp * Cws) = footPos   ===>   Flp = Cwr' * (footPos - Cwp) / Cws
		//LPOS(nodeFoot) = Cwr.Transpose() * (footPos - kneePos) / WSCL(nodeCalf);
		footNode->m_localTransform.pos = calfWR.Transpose() * (footPos - kneePos) / kneeNode->m_worldTransform.scale;
		if (vec3Len(footNode->m_localTransform.pos) > calfLenOrig) {
			footNode->m_localTransform.pos = vec3Norm(footNode->m_localTransform.pos) * calfLenOrig;
		}
	}

	void Skeleton::rotateLeg(const uint32_t pos, const float angle) const {
		const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(_root);
		Matrix44 rot;
		rot.setEulerAngles(degreesToRads(angle), 0, 0);

		auto& transform = rt->transforms[pos];
		transform.local.rot = rot.multiply43Left(transform.local.rot);

		const auto& parentTransform = rt->transforms[transform.parPos];
		const NiPoint3 p = parentTransform.world.rot * (transform.local.pos * parentTransform.world.scale);
		transform.world.pos = parentTransform.world.pos + p;

		rot.makeTransformMatrix(transform.local.rot, NiPoint3(0, 0, 0));
		transform.world.rot = rot.multiply43Left(parentTransform.world.rot);
	}

	void Skeleton::fixPAArmor() const {
		static bool oneTime = false;

		if (_inPowerArmor) {
			if (!oneTime) {
				oneTime = true;
				rotateLeg(boneTreeMap["LLeg_Calf_Armor1"], 90.0f);
				rotateLeg(boneTreeMap["LLeg_Calf_Armor2"], 90.0f);
				rotateLeg(boneTreeMap["LLeg_Thigh_Armor"], 90.0f);
				rotateLeg(boneTreeMap["LLeg_Thigh_Armor1"], 90.0f);
				rotateLeg(boneTreeMap["LLeg_Thigh_Armor2"], 90.0f);
				rotateLeg(boneTreeMap["RLeg_Calf_Armor1"], 90.0f);
				rotateLeg(boneTreeMap["RLeg_Calf_Armor2"], 90.0f);
				rotateLeg(boneTreeMap["RLeg_Thigh_Armor"], 90.0f);
				rotateLeg(boneTreeMap["RLeg_Thigh_Armor1"], 90.0f);
				rotateLeg(boneTreeMap["RLeg_Thigh_Armor2"], 90.0f);
				rotateLeg(boneTreeMap["LArm_UpperArm_Armor"], 90.0f);
				rotateLeg(boneTreeMap["RArm_UpperArm_Armor"], 90.0f);
				rotateLeg(boneTreeMap["LArm_ForeArm_Armor"], 90.0f);
				rotateLeg(boneTreeMap["RArm_ForeArm_Armor"], 90.0f);
			}
		} else {
			oneTime = false;
		}
	}

	void Skeleton::setBodyLen() {
		_torsoLen = vec3Len(getNode("Camera", _root)->m_worldTransform.pos - getNode("COM", _root)->m_worldTransform.pos);
		_torsoLen *= g_config->playerHeight / defaultCameraHeight;

		_legLen = vec3Len(getNode("LLeg_Thigh", _root)->m_worldTransform.pos - getNode("Pelvis", _root)->m_worldTransform.pos);
		_legLen += vec3Len(getNode("LLeg_Calf", _root)->m_worldTransform.pos - getNode("LLeg_Thigh", _root)->m_worldTransform.pos);
		_legLen += vec3Len(getNode("LLeg_Foot", _root)->m_worldTransform.pos - getNode("LLeg_Calf", _root)->m_worldTransform.pos);
		_legLen *= g_config->playerHeight / defaultCameraHeight;
	}

	void Skeleton::hideWeapon() const {
		static BSFixedString nodeName("Weapon");

		if (NiAVObject* weapon = _rightArm.hand->GetObjectByName(&nodeName)) {
			weapon->flags |= 0x1;
			weapon->m_localTransform.scale = 0.0;
			if (const NiNode* weaponNode = weapon->GetAsNiNode()) {
				for (auto i = 0; i < weaponNode->m_children.m_emptyRunStart; ++i) {
					if (NiNode* nextNode = weaponNode->m_children.m_data[i]->GetAsNiNode()) {
						nextNode->flags |= 0x1;
						nextNode->m_localTransform.scale = 0.0;
					}
				}
			}
		}
	}

	void Skeleton::positionPipboy() const {
		static BSFixedString wandPipName("PipboyRoot_NIF_ONLY");
		NiAVObject* wandPip = _playerNodes->SecondaryWandNode->GetObjectByName(&wandPipName);

		if (wandPip == nullptr) {
			return;
		}

		static BSFixedString nodeName("PipboyBone");
		NiAVObject* pipboyBone;
		if (g_config->leftHandedPipBoy) {
			pipboyBone = _rightArm.forearm1->GetObjectByName(&nodeName);
		} else {
			pipboyBone = _leftArm.forearm1->GetObjectByName(&nodeName);
		}

		if (pipboyBone == nullptr) {
			return;
		}

		auto locPos = NiPoint3(0, 0, 0);

		locPos = pipboyBone->m_worldTransform.rot * (locPos * pipboyBone->m_worldTransform.scale);

		const NiPoint3 wandWP = pipboyBone->m_worldTransform.pos + locPos;

		const NiPoint3 delta = wandWP - wandPip->m_parent->m_worldTransform.pos;

		wandPip->m_localTransform.pos = wandPip->m_parent->m_worldTransform.rot.Transpose() * (delta / wandPip->m_parent->m_worldTransform.scale);

		// Slr = LHwr' * RHwr * Slr
		Matrix44 loc;
		loc.setEulerAngles(degreesToRads(30), 0, 0);

		const NiMatrix43 wandWROT = loc.multiply43Left(pipboyBone->m_worldTransform.rot);

		loc.makeTransformMatrix(wandWROT, NiPoint3(0, 0, 0));
		wandPip->m_localTransform.rot = loc.multiply43Left(wandPip->m_parent->m_worldTransform.rot.Transpose());
	}

	void Skeleton::leftHandedModePipboy() const {
		if (g_config->leftHandedPipBoy) {
			NiNode* pipbone = getNode("PipboyBone", _rightArm.forearm1->GetAsNiNode());

			if (!pipbone) {
				pipbone = getNode("PipboyBone", _leftArm.forearm1->GetAsNiNode());

				if (!pipbone) {
					return;
				}

				pipbone->m_parent->RemoveChild(pipbone);
				_rightArm.forearm3->GetAsNiNode()->AttachChild(pipbone, true);
			}

			Matrix44 rot;
			rot.setEulerAngles(0, degreesToRads(180.0), 0);

			pipbone->m_localTransform.rot = rot.multiply43Left(pipbone->m_localTransform.rot);
			pipbone->m_localTransform.pos *= -1.5;
		}
	}

	void Skeleton::makeArmsT(const bool isLeft) const {
		const ArmNodes arm = isLeft ? _leftArm : _rightArm;
		Matrix44 mat;
		mat.makeIdentity();

		arm.forearm1->m_localTransform.rot = mat.make43();
		arm.forearm2->m_localTransform.rot = mat.make43();
		arm.forearm3->m_localTransform.rot = mat.make43();
		arm.hand->m_localTransform.rot = mat.make43();
		arm.upper->m_localTransform.rot = mat.make43();
		//		arm.shoulder->m_localTransform.rot = mat.make43();
	}

	ArmNodes Skeleton::getArm(const bool isLeft) const {
		return isLeft ? _leftArm : _rightArm;
	}

	// Thanks Shizof and SmoothMovementVR for below code
	bool hasKeyword(const TESObjectARMO* armor, const UInt32 keywordFormId) {
		if (!armor) {
			return false;
		}

		for (UInt32 i = 0; i < armor->keywordForm.numKeywords; ++i) {
			if (armor->keywordForm.keywords[i] && armor->keywordForm.keywords[i]->formID == keywordFormId) {
				return true;
			}
		}

		return false;
	}

	bool Skeleton::detectInPowerArmor() {
		// Thanks Shizof and SmoothMovementVR for below code
		if ((*g_player)->equipData) {
			if ((*g_player)->equipData->slots[0x03].item != nullptr) {
				if (const auto equippedForm = (*g_player)->equipData->slots[0x03].item) {
					if (equippedForm->formType == TESObjectARMO::kTypeID) {
						if (const auto armor = DYNAMIC_CAST(equippedForm, TESForm, TESObjectARMO)) {
							if (hasKeyword(armor, KeywordPowerArmor) || hasKeyword(armor, KeywordPowerArmorFrame)) {
								_inPowerArmor = true;
								return true;
							}
							_inPowerArmor = false;
							return false;
						}
					}
				}
			}
		}
		return false;
	}

	void Skeleton::setWandsVisibility(bool show, const WandMode mode) const {
		auto setVisibilityForNode = [this, show](const NiNode* node) {
			for (UInt16 i = 0; i < node->m_children.m_emptyRunStart; ++i) {
				if (const auto child = node->m_children.m_data[i]) {
					if (const auto triShape = child->GetAsBSTriShape()) {
						setVisibility(triShape, show);
						break;
					}
					if (!_stricmp(child->m_name.c_str(), "")) {
						setVisibility(child, show);
						if (const auto grandChild = child->GetAsNiNode()->m_children.m_data[0]) {
							setVisibility(grandChild, show);
						}
						break;
					}
				}
			}
		};

		if (mode == WandMode::Both || (mode == WandMode::PrimaryHandWand && !g_config->leftHandedMode) || (mode == WandMode::OffhandWand && g_config->leftHandedMode)) {
			setVisibilityForNode(_playerNodes->primaryWandNode);
		}

		if (mode == WandMode::Both || (mode == WandMode::PrimaryHandWand && g_config->leftHandedMode) || (mode == WandMode::OffhandWand && !g_config->leftHandedMode)) {
			setVisibilityForNode(_playerNodes->SecondaryWandNode);
		}
	}

	void Skeleton::showWands(const WandMode mode) const {
		setWandsVisibility(true, mode);
	}

	void Skeleton::hideWands(const WandMode mode) const {
		setWandsVisibility(false, mode);
	}

	void Skeleton::hideFistHelpers() const {
		if (!g_config->leftHandedMode) {
			NiAVObject* node = getNode("fist_M_Right_HELPER", _playerNodes->primaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1; // first bit sets the cull flag so it will be hidden;
			}

			node = getNode("fist_F_Right_HELPER", _playerNodes->primaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}

			node = getNode("PA_fist_R_HELPER", _playerNodes->primaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}

			node = getNode("fist_M_Left_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1; // first bit sets the cull flag so it will be hidden;
			}

			node = getNode("fist_F_Left_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}

			node = getNode("PA_fist_L_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}
		} else {
			NiAVObject* node = getNode("fist_M_Right_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1; // first bit sets the cull flag so it will be hidden;
			}

			node = getNode("fist_F_Right_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}

			node = getNode("PA_fist_R_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}

			node = getNode("fist_M_Left_HELPER", _playerNodes->primaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1; // first bit sets the cull flag so it will be hidden;
			}

			node = getNode("fist_F_Left_HELPER", _playerNodes->primaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}

			node = getNode("PA_fist_L_HELPER", _playerNodes->primaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}
		}

		if (const auto uiNode = getNode("Point002", _playerNodes->SecondaryWandNode)) {
			uiNode->m_localTransform.scale = 0.0;
		}
	}

	void Skeleton::hidePipboy() const {
		const BSFixedString pipName("PipboyBone");
		NiAVObject* pipboy = nullptr;

		if (!g_config->leftHandedPipBoy) {
			if (_leftArm.forearm3) {
				pipboy = _leftArm.forearm3->GetObjectByName(&pipName);
			}
		} else {
			pipboy = _rightArm.forearm3->GetObjectByName(&pipName);
		}

		if (!pipboy) {
			return;
		}

		// Changed to allow scaling of third person Pipboy --->
		if (!g_config->hidePipboy) {
			if (pipboy->m_localTransform.scale != g_config->pipBoyScale) {
				pipboy->m_localTransform.scale = g_config->pipBoyScale;
				toggleVis(pipboy->GetAsNiNode(), false, true);
			}
		} else {
			if (pipboy->m_localTransform.scale != 0.0) {
				pipboy->m_localTransform.scale = 0.0;
				toggleVis(pipboy->GetAsNiNode(), true, true);
			}
		}
	}

	/**
	 * detect if the player has an armor item which uses the headlamp equipped as not to overwrite it
	 */
	bool Skeleton::armorHasHeadLamp() {
		if (const auto equippedItem = (*g_player)->equipData->slots[0].item) {
			if (const auto torchEnabledArmor = DYNAMIC_CAST(equippedItem, TESForm, TESObjectARMO)) {
				return hasKeyword(torchEnabledArmor, 0xB34A6);
			}
		}
		return false;
	}

	void Skeleton::set1StPersonArm(NiNode* weapon, NiNode* offsetNode) {
		NiNode** wp = &weapon;
		NiNode** op = &offsetNode;

		update1StPersonArm(*g_player, wp, op);
	}

	void Skeleton::showHidePAHud() const {
		if (const auto hud = getNode("PowerArmorHelmetRoot", _playerNodes->roomnode)) {
			hud->m_localTransform.scale = g_config->showPAHUD ? 1.0f : 0.0f;
		}
	}

	void Skeleton::setLeftHandedSticky() {
		_leftHandedSticky = !g_config->leftHandedMode;
	}

	void Skeleton::handleWeaponNodes() {
		if (_leftHandedSticky == g_config->leftHandedMode) {
			return;
		}

		NiNode* rightWeapon = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		NiNode* leftWeapon = _playerNodes->WeaponLeftNode;
		NiNode* rHand = getNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		NiNode* lHand = getNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		if (!rightWeapon || !rHand || !leftWeapon || !lHand) {
			_DMESSAGE("Cannot set up weapon nodes");
			_leftHandedSticky = g_config->leftHandedMode;
			return;
		}

		rHand->RemoveChild(rightWeapon);
		rHand->RemoveChild(leftWeapon);
		lHand->RemoveChild(rightWeapon);
		lHand->RemoveChild(leftWeapon);

		if (g_config->leftHandedMode) {
			rHand->AttachChild(leftWeapon, true);
			lHand->AttachChild(rightWeapon, true);
		} else {
			rHand->AttachChild(rightWeapon, true);
			lHand->AttachChild(leftWeapon, true);
		}

		if (g_pipboy->isOperatingPipboy()) {
			rightWeapon->m_localTransform.scale = 0.0;
		}

		_leftHandedSticky = g_config->leftHandedMode;
	}

	// This is the main arm IK solver function - Algo credit to prog from SkyrimVR VRIK mod - what a beast!
	void Skeleton::setArms(bool isLeft) {
		ArmNodes arm;

		arm = isLeft ? _leftArm : _rightArm;

		// This first part is to handle the game calculating the first person hand based off two offset nodes
		// PrimaryWeaponOffset and PrimaryMeleeOffset
		// Unfortunately neither of these two nodes are that close to each other so when you equip a melee or ranged weapon
		// the hand will jump which completely messes up the solver and looks bad to boot.
		// So this code below does a similar operation as the in game function that solves the first person arm by forcing
		// everything to go to the PrimaryWeaponNode.  I have hardcoded a rotation below based off one of the guns that
		// matches my real life hand pose with an index controller very well.   I use this as the baseline for everything

		if ((*g_player)->firstPersonSkeleton == nullptr) {
			return;
		}

		NiNode* rightWeapon = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		//NiNode* rightWeapon = _playerNodes->primaryWandNode;
		NiNode* leftWeapon = _playerNodes->WeaponLeftNode; // "WeaponLeft" can return incorrect node for left-handed with throwable weapons

		// handle the NON-primary hand (i.e. the hand that is NOT holding the weapon)
		bool handleOffhand = g_config->leftHandedMode ^ isLeft;

		NiNode* weaponNode = handleOffhand ? leftWeapon : rightWeapon;
		NiNode* offsetNode = handleOffhand ? _playerNodes->SecondaryMeleeWeaponOffsetNode2 : _playerNodes->primaryWeaponOffsetNOde;

		if (handleOffhand) {
			_playerNodes->SecondaryMeleeWeaponOffsetNode2->m_localTransform = _playerNodes->primaryWeaponOffsetNOde->m_localTransform;
			Matrix44 lr;
			lr.setEulerAngles(0, degreesToRads(180), 0);
			_playerNodes->SecondaryMeleeWeaponOffsetNode2->m_localTransform.rot = lr.multiply43Right(_playerNodes->SecondaryMeleeWeaponOffsetNode2->m_localTransform.rot);
			_playerNodes->SecondaryMeleeWeaponOffsetNode2->m_localTransform.pos = NiPoint3(-2, -9, 2);
			updateTransforms(_playerNodes->SecondaryMeleeWeaponOffsetNode2);
		}

		Matrix44 w;
		if (!g_config->leftHandedMode) {
			w.data[0][0] = -0.122f;
			w.data[1][0] = 0.987f;
			w.data[2][0] = 0.100f;
			w.data[0][1] = 0.990f;
			w.data[1][1] = 0.114f;
			w.data[2][1] = 0.081f;
			w.data[0][2] = 0.069f;
			w.data[1][2] = 0.109f;
			w.data[2][2] = -0.992f;
		} else {
			w.data[0][0] = -0.122f;
			w.data[1][0] = 0.987f;
			w.data[2][0] = 0.100f;
			w.data[0][1] = -0.990f;
			w.data[1][1] = -0.114f;
			w.data[2][1] = -0.081f;
			w.data[0][2] = -0.069f;
			w.data[1][2] = -0.109f;
			w.data[2][2] = 0.992f;
		}
		weaponNode->m_localTransform.rot = w.make43();

		if (handleOffhand) {
			w.setEulerAngles(degreesToRads(0), degreesToRads(isLeft ? 45 : -45), degreesToRads(0));
			weaponNode->m_localTransform.rot = w.multiply43Right(weaponNode->m_localTransform.rot);
		}

		weaponNode->m_localTransform.pos = g_config->leftHandedMode
			? (isLeft ? NiPoint3(3.389f, -2.099f, 3.133f) : NiPoint3(0, -4.8f, 0))
			: isLeft
			? NiPoint3(0, 0, 0)
			: NiPoint3(4.389f, -1.899f, -3.133f);

		dampenHand(offsetNode, isLeft);

		weaponNode->IncRef();
		set1StPersonArm(weaponNode, offsetNode);

		NiPoint3 handPos = isLeft ? _leftHand->m_worldTransform.pos : _rightHand->m_worldTransform.pos;
		NiMatrix43 handRot = isLeft ? _leftHand->m_worldTransform.rot : _rightHand->m_worldTransform.rot;

		// Detect if the 1st person hand position is invalid. This can happen when a controller loses tracking.
		// If it is, do not handle IK and let Fallout use its normal animations for that arm instead.
		if (isnan(handPos.x) || isnan(handPos.y) || isnan(handPos.z) ||
			isinf(handPos.x) || isinf(handPos.y) || isinf(handPos.z) ||
			vec3Len(arm.upper->m_worldTransform.pos - handPos) > 200.0) {
			return;
		}

		float adjustedArmLength = g_config->armLength / 36.74f;

		// Shoulder IK is done in a very simple way

		NiPoint3 shoulderToHand = handPos - arm.upper->m_worldTransform.pos;
		float armLength = g_config->armLength;
		float adjustAmount = (std::clamp)(vec3Len(shoulderToHand) - armLength * 0.5f, 0.0f, armLength * 0.85f) / (armLength * 0.85f);
		NiPoint3 shoulderOffset = vec3Norm(shoulderToHand) * (adjustAmount * armLength * 0.08f);

		NiPoint3 clavicalToNewShoulder = arm.upper->m_worldTransform.pos + shoulderOffset - arm.shoulder->m_worldTransform.pos;

		NiPoint3 sLocalDir = arm.shoulder->m_worldTransform.rot.Transpose() * clavicalToNewShoulder / arm.shoulder->m_worldTransform.scale;

		Matrix44 rotatedM;
		rotatedM = 0.0;
		rotatedM.rotateVectorVec(sLocalDir, NiPoint3(1, 0, 0));

		NiMatrix43 result = rotatedM.multiply43Left(arm.shoulder->m_localTransform.rot);
		arm.shoulder->m_localTransform.rot = result;

		updateDown(arm.shoulder->GetAsNiNode(), true);

		// The bend of the arm depends on its distance to the body.  Its distance as well as the lengths of
		// the upper arm and forearm define the sides of a triangle:
		//                 ^
		//                /|\         Let a,b be the arm lengths, c be the distance from hand-to-shoulder
		//               /^| \        Let A be the total angle at which the wrist must bend
		//              / ||  \       Let x be the width of the right triangle
		//            a/  y|   \  b   Let y be the height of the right triangle
		//            /   ||    \
	    //           /    v|<-x->\
	    // Shoulder /______|_____A\ Hand
		//                c
		// Law of cosines: Wrist angle A = acos( (b^2 + c^2 - a^2) / (2*b*c) )
		// The wrist angle is used to calculate x and y, which are used to position the elbow

		float negLeft = isLeft ? -1 : 1;

		float originalUpperLen = vec3Len(arm.forearm1->m_localTransform.pos);
		float originalForearmLen;

		if (_inPowerArmor) {
			originalForearmLen = vec3Len(arm.hand->m_localTransform.pos);
		} else {
			originalForearmLen = vec3Len(arm.hand->m_localTransform.pos) + vec3Len(arm.forearm2->m_localTransform.pos) + vec3Len(arm.forearm3->m_localTransform.pos);
		}
		float upperLen = originalUpperLen * adjustedArmLength;
		float forearmLen = originalForearmLen * adjustedArmLength;

		NiPoint3 Uwp = arm.upper->m_worldTransform.pos;
		NiPoint3 handToShoulder = Uwp - handPos;
		float hsLen = (std::max)(vec3Len(handToShoulder), 0.1f);

		if (hsLen > (upperLen + forearmLen) * 2.25f) {
			return;
		}

		// Stretch the upper arm and forearm proportionally when the hand distance exceeds the arm length
		if (hsLen > upperLen + forearmLen) {
			float diff = hsLen - upperLen - forearmLen;
			float ratio = forearmLen / (forearmLen + upperLen);
			forearmLen += ratio * diff + 0.1f;
			upperLen += (1.0f - ratio) * diff + 0.1f;
		}

		NiPoint3 forwardDir = vec3Norm(_forwardDir);
		NiPoint3 sidewaysDir = vec3Norm(_sidewaysRDir * negLeft);

		// The primary twist angle comes from the direction the wrist is pointing into the forearm
		NiPoint3 handBack = handRot * NiPoint3(-1, 0, 0);
		float twistAngle = asinf((std::clamp)(handBack.z, -0.999f, 0.999f));

		// The second twist angle comes from a side vector pointing "outward" from the side of the wrist
		NiPoint3 handSide = handRot * NiPoint3(0, -1, 0);
		NiPoint3 handInSide = handSide * negLeft;
		float twistAngle2 = -1 * asinf((std::clamp)(handSide.z, -0.599f, 0.999f));

		// Blend the two twist angles together, using the primary angle more when the wrist is pointing downward
		//float interpTwist = (std::clamp)((handBack.z + 0.866f) * 1.155f, 0.25f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
		float interpTwist = (std::clamp)((handBack.z + 0.866f) * 1.155f, 0.45f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
		//		_MESSAGE("%2f %2f %2f", rads_to_degrees(twistAngle), rads_to_degrees(twistAngle2), interpTwist);
		twistAngle = twistAngle + interpTwist * (twistAngle2 - twistAngle);
		// Wonkiness is bad.  Interpolate twist angle towards zero to correct it when the angles are pointed a certain way.
		/*	float fixWonkiness1 = (std::clamp)(vec3_dot(handSide, vec3_norm(-sidewaysDir - forwardDir * 0.25f + NiPoint3(0, 0, -0.25f))), 0.0f, 1.0f);
			float fixWonkiness2 = 1.0f - (std::clamp)(vec3_dot(handBack, vec3_norm(forwardDir + sidewaysDir)), 0.0f, 1.0f);
			twistAngle = twistAngle + fixWonkiness1 * fixWonkiness2 * (-PI / 2.0f - twistAngle);*/

		//		_MESSAGE("final angle %2f", rads_to_degrees(twistAngle));

		// Smooth out sudden changes in the twist angle over time to reduce elbow shake
		static std::array<float, 2> prevAngle = {0, 0};
		twistAngle = prevAngle[isLeft ? 0 : 1] + (twistAngle - prevAngle[isLeft ? 0 : 1]) * 0.25f;
		prevAngle[isLeft ? 0 : 1] = twistAngle;

		// Calculate the hand's distance behind the body - It will increase the minimum elbow rotation angle
		float size = 1.0;
		float behindD = -(forwardDir.x * arm.shoulder->m_worldTransform.pos.x + forwardDir.y * arm.shoulder->m_worldTransform.pos.y) - 10.0f;
		float handBehindDist = -(handPos.x * forwardDir.x + handPos.y * forwardDir.y + behindD);
		float behindAmount = (std::clamp)(handBehindDist / (40.0f * size), 0.0f, 1.0f);

		// Holding hands in front of chest increases the minimum elbow rotation angle (elbows lift) and decreases the maximum angle
		NiPoint3 planeDir = rotateXY(forwardDir, negLeft * degreesToRads(135));
		float planeD = -(planeDir.x * arm.shoulder->m_worldTransform.pos.x + planeDir.y * arm.shoulder->m_worldTransform.pos.y) + 16.0f * size;
		float armCrossAmount = (std::clamp)((handPos.x * planeDir.x + handPos.y * planeDir.y + planeD) / (20.0f * size), 0.0f, 1.0f);

		// The arm lift limits how much the crossing amount can influence minimum elbow rotation
		// The maximum rotation is also decreased as hands lift higher (elbows point further downward)
		float armLiftLimitZ = _chest->m_worldTransform.pos.z * size;
		float armLiftThreshold = 60.0f * size;
		float armLiftLimit = (std::clamp)((armLiftLimitZ + armLiftThreshold - handPos.z) / armLiftThreshold, 0.0f, 1.0f); // 1 at bottom, 0 at top
		float upLimit = (std::clamp)((1.0f - armLiftLimit) * 1.4f, 0.0f, 1.0f); // 0 at bottom, 1 at a much lower top

		// Determine overall amount the elbows minimum rotation will be limited
		float adjustMinAmount = (std::max)(behindAmount, (std::min)(armCrossAmount, armLiftLimit));

		// Get the minimum and maximum angles at which the elbow is allowed to twist
		float twistMinAngle = degreesToRads(-85.0) + degreesToRads(50) * adjustMinAmount;
		float twistMaxAngle = degreesToRads(55.0) - (std::max)(degreesToRads(90) * armCrossAmount, degreesToRads(70) * upLimit);

		// Twist angle ranges from -PI/2 to +PI/2; map that range to go from the minimum to the maximum instead
		float twistLimitAngle = twistMinAngle + (twistAngle + PI / 2.0f) / PI * (twistMaxAngle - twistMinAngle);

		//_MESSAGE("%f %f %f %f", rads_to_degrees(twistAngle), rads_to_degrees(twistAngle2), rads_to_degrees(twistAngle), rads_to_degrees(twistLimitAngle));
		// The bendDownDir vector points in the direction the player faces, and bends up/down with the final elbow angle
		NiMatrix43 rot = getRotationAxisAngle(sidewaysDir * negLeft, twistLimitAngle);
		NiPoint3 bendDownDir = rot * forwardDir;

		// Get the "X" direction vectors pointing to the shoulder
		NiPoint3 xDir = vec3Norm(handToShoulder);

		// Get the final "Y" vector, perpendicular to "X", and pointing in elbow direction (as in the diagram above)
		float sideD = -(sidewaysDir.x * arm.shoulder->m_worldTransform.pos.x + sidewaysDir.y * arm.shoulder->m_worldTransform.pos.y) - 1.0 * 8.0f;
		float acrossAmount = -(handPos.x * sidewaysDir.x + handPos.y * sidewaysDir.y + sideD) / (16.0f * 1.0);
		float handSideTwistOutward = vec3Dot(handSide, vec3Norm(sidewaysDir + forwardDir * 0.5f));
		float armTwist = (std::clamp)(handSideTwistOutward - (std::max)(0.0f, acrossAmount + 0.25f), 0.0f, 1.0f);

		if (acrossAmount < 0) {
			acrossAmount *= 0.2f;
		}

		float handBehindHead = (std::clamp)((handBehindDist + 0.0f * size) / (15.0f * size), 0.0f, 1.0f) * (std::clamp)(upLimit * 1.2f, 0.0f, 1.0f);
		float elbowsTwistForward = (std::max)(acrossAmount * degreesToRads(90), handBehindHead * degreesToRads(120));
		NiPoint3 elbowDir = rotateXY(bendDownDir, -negLeft * (degreesToRads(150) - armTwist * degreesToRads(25) - elbowsTwistForward));
		NiPoint3 yDir = elbowDir - xDir * vec3Dot(elbowDir, xDir);
		yDir = vec3Norm(yDir);

		// Get the angle wrist must bend to reach elbow position
		// In cases where this is impossible (hand too close to shoulder), then set forearmLen = upperLen so there is always a solution
		float wristAngle = acosf((forearmLen * forearmLen + hsLen * hsLen - upperLen * upperLen) / (2 * forearmLen * hsLen));
		if (isnan(wristAngle) || isinf(wristAngle)) {
			forearmLen = upperLen = (originalUpperLen + originalForearmLen) / 2.0 * adjustedArmLength;
			wristAngle = acosf((forearmLen * forearmLen + hsLen * hsLen - upperLen * upperLen) / (2 * forearmLen * hsLen));
		}

		// Get the desired world coordinate of the elbow
		float xDist = cosf(wristAngle) * forearmLen;
		float yDist = sinf(wristAngle) * forearmLen;
		NiPoint3 elbowWorld = handPos + xDir * xDist + yDir * yDist;

		// This code below rotates and positions the upper arm, forearm, and hand bones
		// Notation: C=Clavicle, U=Upper arm, F=Forearm, H=hand   w=world, l=local   p=position, r=rotation, s=scale
		//    Rules: World position = Parent world pos + Parent world rot * (Local pos * Parent World scale)
		//           World Rotation = Parent world rotation * Local Rotation
		// ---------------------------------------------------------------------------------------------------------

		// The upper arm bone must be rotated from its forward vector to its shoulder-to-elbow vector in its local space
		// Calculate Ulr:  baseUwr * rotTowardElbow = Cwr * Ulr   ===>   Ulr = Cwr' * baseUwr * rotTowardElbow
		NiMatrix43 Uwr = arm.upper->m_worldTransform.rot;
		NiPoint3 pos = elbowWorld - Uwp;
		NiPoint3 uLocalDir = Uwr.Transpose() * vec3Norm(pos) / arm.upper->m_worldTransform.scale;

		rotatedM.rotateVectorVec(uLocalDir, arm.forearm1->m_localTransform.pos);
		arm.upper->m_localTransform.rot = rotatedM.multiply43Left(arm.upper->m_localTransform.rot);

		rotatedM.makeTransformMatrix(arm.upper->m_localTransform.rot, arm.upper->m_localTransform.pos);

		Uwr = rotatedM.multiply43Left(arm.shoulder->m_worldTransform.rot);

		// Find the angle of the forearm twisted around the upper arm and twist the upper arm to align it
		//    Uwr * twist = Cwr * Ulr   ===>   Ulr = Cwr' * Uwr * twist
		pos = handPos - elbowWorld;
		NiPoint3 uLocalTwist = Uwr.Transpose() * vec3Norm(pos);
		uLocalTwist.x = 0;
		NiPoint3 upperSide = arm.upper->m_worldTransform.rot * NiPoint3(0, 1, 0);
		NiPoint3 uloc = arm.shoulder->m_worldTransform.rot.Transpose() * upperSide;
		uloc.x = 0;
		float upperAngle = acosf(vec3Dot(vec3Norm(uLocalTwist), vec3Norm(uloc))) * (uLocalTwist.z > 0 ? 1 : -1);

		Matrix44 twist;
		twist.setEulerAngles(-upperAngle, 0, 0);
		arm.upper->m_localTransform.rot = twist.multiply43Left(arm.upper->m_localTransform.rot);

		rotatedM.makeTransformMatrix(arm.upper->m_localTransform.rot, arm.upper->m_localTransform.pos);
		Uwr = rotatedM.multiply43Left(arm.shoulder->m_worldTransform.rot);

		twist.setEulerAngles(-upperAngle, 0, 0);
		arm.forearm1->m_localTransform.rot = twist.multiply43Left(arm.forearm1->m_localTransform.rot);

		// The forearm arm bone must be rotated from its forward vector to its elbow-to-hand vector in its local space
		// Calculate Flr:  Fwr * rotTowardHand = Uwr * Flr   ===>   Flr = Uwr' * Fwr * rotTowardHand
		rotatedM.makeTransformMatrix(arm.forearm1->m_localTransform.rot, arm.forearm1->m_localTransform.pos);
		NiMatrix43 Fwr = rotatedM.multiply43Left(Uwr);
		NiPoint3 elbowHand = handPos - elbowWorld;
		NiPoint3 fLocalDir = Fwr.Transpose() * vec3Norm(elbowHand);

		rotatedM.rotateVectorVec(fLocalDir, NiPoint3(1, 0, 0));
		arm.forearm1->m_localTransform.rot = rotatedM.multiply43Left(arm.forearm1->m_localTransform.rot);
		rotatedM.makeTransformMatrix(arm.forearm1->m_localTransform.rot, arm.forearm1->m_localTransform.pos);
		Fwr = rotatedM.multiply43Left(Uwr);

		NiMatrix43 Fwr2, Fwr3;

		if (!_inPowerArmor && arm.forearm2 != nullptr && arm.forearm3 != nullptr) {
			rotatedM.makeTransformMatrix(arm.forearm2->m_localTransform.rot, arm.forearm2->m_localTransform.pos);
			Fwr2 = rotatedM.multiply43Left(Fwr);
			rotatedM.makeTransformMatrix(arm.forearm3->m_localTransform.rot, arm.forearm3->m_localTransform.pos);
			Fwr3 = rotatedM.multiply43Left(Fwr2);

			// Find the angle the wrist is pointing and twist forearm3 appropriately
			//    Fwr * twist = Uwr * Flr   ===>   Flr = (Uwr' * Fwr) * twist = (Flr) * twist

			NiPoint3 wLocalDir = Fwr3.Transpose() * vec3Norm(handInSide);
			wLocalDir.x = 0;
			NiPoint3 forearm3Side = Fwr3 * NiPoint3(0, 0, -1); // forearm is rotated 90 degrees already from hand so need this vector instead of 0,-1,0
			NiPoint3 floc = Fwr2.Transpose() * vec3Norm(forearm3Side);
			floc.x = 0;
			float fcos = vec3Dot(vec3Norm(wLocalDir), vec3Norm(floc));
			float fsin = vec3Det(vec3Norm(wLocalDir), vec3Norm(floc), NiPoint3(-1, 0, 0));
			float forearmAngle = -1 * negLeft * atan2f(fsin, fcos);

			// old way of doing this
			//NiPoint3 cross = vec3_cross(vec3_norm(floc), vec3_norm(wLocalDir));

			//float forearmAngle = acosf(fcos) - degrees_to_rads(90.0f);

			//if (cross.x < 0) {
			//	float d = forearmAngle >= 0 ? degrees_to_rads(180.0f) : -1 * degrees_to_rads(180.0f);
			//	forearmAngle = d - forearmAngle;
			//}

			twist.setEulerAngles(negLeft * forearmAngle / 2, 0, 0);
			arm.forearm2->m_localTransform.rot = twist.multiply43Left(arm.forearm2->m_localTransform.rot);

			twist.setEulerAngles(negLeft * forearmAngle / 2, 0, 0);
			arm.forearm3->m_localTransform.rot = twist.multiply43Left(arm.forearm3->m_localTransform.rot);

			rotatedM.makeTransformMatrix(arm.forearm2->m_localTransform.rot, arm.forearm2->m_localTransform.pos);
			Fwr2 = rotatedM.multiply43Left(Fwr);
			rotatedM.makeTransformMatrix(arm.forearm3->m_localTransform.rot, arm.forearm3->m_localTransform.pos);
			Fwr3 = rotatedM.multiply43Left(Fwr2);
		}

		// Calculate Hlr:  Fwr * Hlr = handRot   ===>   Hlr = Fwr' * handRot
		rotatedM.makeTransformMatrix(handRot, handPos);
		if (!_inPowerArmor) {
			arm.hand->m_localTransform.rot = rotatedM.multiply43Left(Fwr3.Transpose());
		} else {
			arm.hand->m_localTransform.rot = rotatedM.multiply43Left(Fwr.Transpose());
		}

		// Calculate Flp:  Fwp = Uwp + Uwr * (Flp * Uws) = elbowWorld   ===>   Flp = Uwr' * (elbowWorld - Uwp) / Uws
		arm.forearm1->m_localTransform.pos = Uwr.Transpose() * ((elbowWorld - Uwp) / arm.upper->m_worldTransform.scale);

		float origEHLen = vec3Len(arm.hand->m_worldTransform.pos - arm.forearm1->m_worldTransform.pos);
		float forearmRatio = forearmLen / origEHLen * _root->m_localTransform.scale;

		if (arm.forearm2 && !_inPowerArmor) {
			arm.forearm2->m_localTransform.pos *= forearmRatio;
			arm.forearm3->m_localTransform.pos *= forearmRatio;
		}
		arm.hand->m_localTransform.pos *= forearmRatio;
	}

	void Skeleton::showOnlyArms() {
		const NiPoint3 rwp = _rightArm.shoulder->m_worldTransform.pos;
		const NiPoint3 lwp = _leftArm.shoulder->m_worldTransform.pos;
		_root->m_localTransform.scale = 0.00001f;
		updateTransforms(_root);
		_root->m_worldTransform.pos += _forwardDir * -10.0f;
		_root->m_worldTransform.pos.z = rwp.z;
		updateDown(_root, false);

		_rightArm.shoulder->m_localTransform.scale = 100000;
		_leftArm.shoulder->m_localTransform.scale = 100000;

		updateTransforms(static_cast<NiNode*>(_rightArm.shoulder));
		updateTransforms(static_cast<NiNode*>(_leftArm.shoulder));

		_rightArm.shoulder->m_worldTransform.pos = rwp;
		_leftArm.shoulder->m_worldTransform.pos = lwp;

		updateDown(static_cast<NiNode*>(_rightArm.shoulder), false);
		updateDown(static_cast<NiNode*>(_leftArm.shoulder), false);
	}

	void Skeleton::hideHands() {
		const NiPoint3 rwp = _rightArm.shoulder->m_worldTransform.pos;
		_root->m_localTransform.scale = 0.00001f;
		updateTransforms(_root);
		_root->m_worldTransform.pos += _forwardDir * -10.0f;
		_root->m_worldTransform.pos.z = rwp.z;
		updateDown(_root, false);
	}

	void Skeleton::calculateHandPose(const std::string& bone, const float gripProx, const bool thumbUp, const bool isLeft) {
		Quaternion qc, qt;
		const int sign = isLeft ? -1 : 1;

		// if a mod is using the papyrus interface to manually set finger poses
		if (handPapyrusHasControl[bone]) {
			qt.fromRot(handOpen[bone].rot);
			Quaternion qo;
			qo.fromRot(handClosed[bone].rot);
			qo.slerp(std::clamp(handPapyrusPose[bone], 0.0f, 1.0f), qt);
			qt = qo;
		}
		// thumbUp pose
		else if (thumbUp && bone.find("Finger1") != std::string::npos) {
			Matrix44 rot;
			if (bone.find("Finger11") != std::string::npos) {
				rot.setEulerAngles(sign * 0.5f, sign * 0.4f, -0.3f);
				NiMatrix43 wr = handOpen[bone].rot;
				wr = rot.multiply43Left(wr);
				qt.fromRot(wr);
			} else if (bone.find("Finger13") != std::string::npos) {
				rot.setEulerAngles(0, 0, degreesToRads(-35.0));
				NiMatrix43 wr = handOpen[bone].rot;
				wr = rot.multiply43Left(wr);
				qt.fromRot(wr);
			}
		} else if (_closedHand[bone]) {
			qt.fromRot(handClosed[bone].rot);
		} else {
			qt.fromRot(handOpen[bone].rot);
			if (_handBonesButton[bone] == vr::k_EButton_Grip) {
				Quaternion qo;
				qo.fromRot(handClosed[bone].rot);
				qo.slerp(1.0 - gripProx, qt);
				qt = qo;
			}
		}

		qc.fromRot(_handBones[bone].rot);
		const float blend = std::clamp(_frameTime * 7, 0.0, 1.0);
		qc.slerp(blend, qt);
		_handBones[bone].rot = qc.getRot().make43();
	}

	void Skeleton::copy1StPerson(const std::string& bone) {
		const auto fpTree = static_cast<BSFlattenedBoneTree*>((*g_player)->firstPersonSkeleton->m_children.m_data[0]->GetAsNiNode());

		const int pos = fpTree->GetBoneIndex(bone);

		if (pos >= 0) {
			if (fpTree->transforms[pos].refNode) {
				_handBones[bone] = fpTree->transforms[pos].refNode->m_localTransform;
			} else {
				_handBones[bone] = fpTree->transforms[pos].local;
			}
		}
	}

	void Skeleton::fixBoneTree() const {
		const auto rt = reinterpret_cast<BSFlattenedBoneTree*>(_root);

		if (rt->numTransforms > 145) {
			return;
		}

		for (auto pos = 0; pos < rt->numTransforms; ++pos) {
			const auto& name = rt->transforms[pos].name;
			auto found = fingerRelations.find(name.c_str());
			if (found != fingerRelations.end()) {
				const auto& relation = found->second;
				const int parentPos = boneTreeMap[relation.first];
				rt->transforms[pos].parPos = parentPos;
				rt->transforms[pos].childPos = relation.second.empty() ? -1 : boneTreeMap[relation.second];

				NiNode* meshNode = getNode(boneTreeVec[pos].c_str(), _root);
				rt->transforms[pos].refNode = nullptr;
				if (meshNode && meshNode->m_parent) {
					meshNode->m_parent->RemoveChild(meshNode);
				}
			}
		}
	}

	void Skeleton::setHandPose() {
		const auto rt = (BSFlattenedBoneTree*)_root;
		bool isLeft = false;

		//	fixBoneTree();

		//if (rt->numTransforms > 145) {
		//	return;
		//}
		const bool isWeaponVisible = isNodeVisible(getWeaponNode());

		for (auto pos = 0; pos < rt->numTransforms; pos++) {
			std::string name = boneTreeVec[pos];
			auto found = fingerRelations.find(name.c_str());
			if (found != fingerRelations.end()) {
				isLeft = name[0] == 'L';
				const uint64_t reg = isLeft
					? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonTouched
					: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonTouched;
				const float gripProx = isLeft
					? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).rAxis[2].x
					: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).rAxis[2].x;
				const bool thumbUp = reg & vr::ButtonMaskFromId(vr::k_EButton_Grip) && reg & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger) && !(reg & vr::ButtonMaskFromId(
					vr::k_EButton_SteamVR_Touchpad));
				_closedHand[name] = reg & vr::ButtonMaskFromId(_handBonesButton[name]);

				if (isWeaponVisible && !g_pipboy->status() && !g_pipboy->isOperatingPipboy() && !(isLeft ^ g_config->leftHandedMode)) {
					// CylonSurfer Updated conditions to cater for Virtual Pipboy usage (Ensures Index Finger is extended when weapon is drawn)
					this->copy1StPerson(name);
				} else {
					calculateHandPose(name, gripProx, thumbUp, isLeft);
				}

				const NiTransform trans = _handBones[name];

				rt->transforms[pos].local.rot = trans.rot;
				rt->transforms[pos].local.pos = handOpen[name.c_str()].pos;

				if (rt->transforms[pos].refNode) {
					rt->transforms[pos].refNode->m_localTransform = rt->transforms[pos].local;
				}
			}

			if (rt->transforms[pos].refNode) {
				rt->transforms[pos].world = rt->transforms[pos].refNode->m_worldTransform;
			} else {
				const short parent = rt->transforms[pos].parPos;
				NiPoint3 p = rt->transforms[pos].local.pos;
				p = rt->transforms[parent].world.rot * (p * rt->transforms[parent].world.scale);

				rt->transforms[pos].world.pos = rt->transforms[parent].world.pos + p;

				Matrix44 rot;
				rot.makeTransformMatrix(rt->transforms[pos].local.rot, NiPoint3(0, 0, 0));

				rt->transforms[pos].world.rot = rot.multiply43Left(rt->transforms[parent].world.rot);
			}
		}
	}

	void Skeleton::moveBack() {
		const NiNode* body = _root->m_parent->GetAsNiNode();

		_root->m_localTransform.pos = body->m_worldTransform.pos - this->getPosition();
		_root->m_localTransform.pos.y -= 50.0;
	}

	void Skeleton::dampenHand(NiNode* node, const bool isLeft) {
		if (!g_config->dampenHands) {
			return;
		}

		if (isInScopeMenu() && !g_config->dampenHandsInVanillaScope) {
			return;
		}

		// Get the previous frame transform
		const NiTransform& prevFrame = isLeft ? _leftHandPrevFrame : _rightHandPrevFrame;

		// Spherical interpolation between previous frame and current frame for the world rotation matrix
		Quaternion rq, rt;
		rq.fromRot(prevFrame.rot);
		rt.fromRot(node->m_worldTransform.rot);
		rq.slerp(1 - (isInScopeMenu() ? g_config->dampenHandsRotationInVanillaScope : g_config->dampenHandsRotation), rt);
		node->m_worldTransform.rot = rq.getRot().make43();

		// Linear interpolation between the position from the previous frame to current frame
		const NiPoint3 dir = _curPos - _lastPos; // Offset the player movement from this interpolation
		NiPoint3 deltaPos = node->m_worldTransform.pos - prevFrame.pos - dir; // Add in player velocity
		deltaPos *= isInScopeMenu() ? g_config->dampenHandsTranslationInVanillaScope : g_config->dampenHandsTranslation;
		node->m_worldTransform.pos -= deltaPos;

		// Update the previous frame transform
		if (isLeft) {
			_leftHandPrevFrame = node->m_worldTransform;
		} else {
			_rightHandPrevFrame = node->m_worldTransform;
		}

		updateDown(node, false);
	}
}
