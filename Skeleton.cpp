#include "Skeleton.h"
#include "F4VRBody.h"
#include "HandPose.h"
#include "weaponOffset.h"
#include "f4se/GameForms.h"
#include "VR.h"

#include <chrono>
#include <time.h>
#include <string.h>

extern PapyrusVRAPI* g_papyrusvr;
extern OpenVRHookManagerAPI* vrhook;

using namespace std::chrono;
namespace F4VRBody
{

	float defaultCameraHeight = 120.4828;
	float PACameraHeightDiff = 20.7835;

	UInt32 KeywordPowerArmor = 0x4D8A1;
	UInt32 KeywordPowerArmorFrame = 0x15503F;
	bool tempsticky = false;
	std::map<std::string, int> boneTreeMap;
	std::vector<std::string> boneTreeVec;
	NiPoint3 _lasthmdtoNewHip;
	void addFingerRelations(std::map<std::string, std::pair<std::string, std::string>>* map, std::string hand, std::string finger1, std::string finger2, std::string finger3) {
		map->insert({ finger1, { hand, finger2 } });
		map->insert({ finger2, { finger1, finger3 } });
		map->insert({ finger3, { finger2, std::string()} });
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
		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)root;

		boneTreeMap.clear();
		boneTreeVec.clear();

		for (auto i = 0; i < rt->numTransforms; i++) {
			_MESSAGE("BoneTree Init -> Push %s into position %d", rt->transforms[i].name.c_str(), i);
			boneTreeMap.insert({ rt->transforms[i].name.c_str(), i});
			boneTreeVec.push_back(rt->transforms[i].name.c_str());
		}
	}

	void printMatrix(Matrix44* mat) {
		_MESSAGE("Dump matrix:");
		std::string row = "";
		for (auto i = 0; i < 4; i++) {
			for (auto j = 0; j < 4; j++) {
				row += std::to_string(mat->data[i][j]);
				row += " ";
			}
			_MESSAGE("%s", row.c_str());
			row = "";
		}
	}

    // Native function that takes the 1st person skeleton weapon node and calculates the skeleton from upperarm down based off the offsetNode
	void update1stPersonArm(PlayerCharacter* pc, NiNode** weapon, NiNode** offsetNode) {
		using func_t = decltype(&update1stPersonArm);
		RelocAddr<func_t> func(0xef6280);

		return func(pc, weapon, offsetNode);
	}



	void Skeleton::printChildren(NiNode* child, std::string padding) {
		padding += "....";
		_MESSAGE("%s%s : children = %d hidden: %d: local (%2f, %2f, %2f) world (%2f, %2f, %2f)", padding.c_str(), child->m_name.c_str(), child->m_children.m_emptyRunStart, (child->flags & 0x1),
			child->m_localTransform.pos.x,
			child->m_localTransform.pos.y,
			child->m_localTransform.pos.z,
			child->m_worldTransform.pos.x,
			child->m_worldTransform.pos.y,
			child->m_worldTransform.pos.z);

		//_MESSAGE("%s%s : children = %d : worldbound %f %f %f %f", padding.c_str(), child->m_name.c_str(), child->m_children.m_emptyRunStart,
		//	child->m_worldBound.m_kCenter.x, child->m_worldBound.m_kCenter.y, child->m_worldBound.m_kCenter.z, child->m_worldBound.m_fRadius);

		if (child->GetAsNiNode())
		{
			for (auto i = 0; i < child->m_children.m_emptyRunStart; ++i) {
				//auto nextNode = child->m_children.m_data[i] ? child->m_children.m_data[i]->GetAsNiNode() : nullptr;
				auto nextNode = child->m_children.m_data[i];
				if (nextNode) {
					this->printChildren((NiNode*)nextNode, padding);
				}
			}
		}
	}


	void Skeleton::printNodes(NiNode* nde) {
		// print root node info first
		_MESSAGE("%s : children = %d hidden: %d: local (%f, %f, %f)", nde->m_name.c_str(), nde->m_children.m_emptyRunStart, (nde->flags & 0x1),
			nde->m_localTransform.pos.x,nde->m_localTransform.pos.y,nde->m_localTransform.pos.z);

		std::string padding = "";

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
		//	auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			auto nextNode = nde->m_children.m_data[i];
			if (nextNode) {
				this->printChildren((NiNode*)nextNode, padding);
			}
		}
	}

    void Skeleton::rotateWorld(NiNode *nde) {
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

        Matrix44 *local = (Matrix44*)&nde->m_worldTransform.rot;
        Matrix44 result;
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

	void Skeleton::updatePos(NiNode* nde, NiPoint3 offset) {

		nde->m_worldTransform.pos += offset;

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				this->updatePos(nextNode, offset);
			}
		}
	}

	void Skeleton::updateDown(NiNode* nde, bool updateSelf) {
		if (!nde) {
			return;
		}

		NiAVObject::NiUpdateData* ud = nullptr;

		if (updateSelf) {
			nde->UpdateWorldData(ud);
		}

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			auto nextNode = nde->m_children.m_data[i];
			if (nextNode) {
				if (auto niNode = nextNode->GetAsNiNode()) {
					this->updateDown(niNode, true);
				}
				else if (auto triNode = nextNode->GetAsBSGeometry()) {
					triNode->UpdateWorldData(ud);
				}
			}
		}
	}


	void Skeleton::updateDownTo(NiNode* toNode, NiNode* fromNode, bool updateSelf) {
		if (!toNode || !fromNode) {
			return;
		}

		NiAVObject::NiUpdateData* ud = nullptr;

		if (updateSelf) {
			fromNode->UpdateWorldData(ud);
		}

		if (_stricmp(toNode->m_name.c_str(), fromNode->m_name.c_str()) == 0) {
			return;
		}

		for (auto i = 0; i < fromNode->m_children.m_emptyRunStart; ++i) {
			auto nextNode = fromNode->m_children.m_data[i];
			if (nextNode) {
				if (auto niNode = nextNode->GetAsNiNode()) {
					this->updateDownTo(toNode, niNode, true);
				}
			}
		}
	}

	void Skeleton::updateUpTo(NiNode* toNode, NiNode* fromNode, bool updateTarget) {
		if (!toNode || !fromNode) {
			return;
		}

		NiAVObject::NiUpdateData* ud = nullptr;

		if (_stricmp(toNode->m_name.c_str(), fromNode->m_name.c_str()) == 0) {
			if (updateTarget) {
				fromNode->UpdateWorldData(ud);
			}
			return;
		}

		fromNode->UpdateWorldData(ud);
		if (auto parent = fromNode->m_parent ? fromNode->m_parent->GetAsNiNode() : nullptr) {
			updateUpTo(toNode, parent, true);
		}
	}


	void Skeleton::setTime() {
		prevTime = timer;
		QueryPerformanceFrequency(&freqCounter);
		QueryPerformanceCounter(&timer);

		_frameTime = (double)(timer.QuadPart - prevTime.QuadPart) / freqCounter.QuadPart;

		//also save last position at this time for anyone doing speed calcs
		_lastPos = _curPos;
	}

	void Skeleton::selfieSkelly(float offsetOutFront) {    // Projects the 3rd person body out in front of the player by offset amount
		if (!c_selfieMode || !_root) {
			return;
		}

		float z = _root->m_localTransform.pos.z;
		NiNode* body = _root->m_parent->GetAsNiNode();

		NiPoint3 back = vec3_norm(NiPoint3(-_forwardDir.x, -_forwardDir.y, 0));
		NiPoint3 bodydir = NiPoint3(0, 1, 0);

		Matrix44 mat;
		mat.makeIdentity();
		mat.rotateVectoVec(back, bodydir);
		_root->m_localTransform.rot = mat.multiply43Left(body->m_worldTransform.rot.Transpose());
		_root->m_localTransform.pos = body->m_worldTransform.pos - this->getPosition();
		_root->m_localTransform.pos.y += offsetOutFront;
		_root->m_localTransform.pos.z = z;
	}

	NiNode* Skeleton::getNode(const char* nodeName, NiNode* nde) {
		if (!nde || !nde->m_name) {
			return nullptr;
		}

		if (_stricmp(nodeName, nde->m_name.c_str()) == 0) {
			return nde;
		}

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			if (auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr) {
				if (auto ret = this->getNode(nodeName, nextNode)) {
					return ret;
				}
			}
		}

		return nullptr;
	}


	void Skeleton::setVisibility(NiAVObject* nde, bool a_show) {
		if (nde) {
			nde->flags = a_show ? (nde->flags & ~0x1) : (nde->flags | 0x1);
		}
	}


	void Skeleton::setupHead(NiNode* headNode, bool hideHead) {

		if (!headNode) {
			return;
		}

		headNode->m_localTransform.rot.data[0][0] =  0.967;
		headNode->m_localTransform.rot.data[0][1] = -0.251;
		headNode->m_localTransform.rot.data[0][2] =  0.047;
		headNode->m_localTransform.rot.data[1][0] =  0.249;
		headNode->m_localTransform.rot.data[1][1] =  0.967;
		headNode->m_localTransform.rot.data[1][2] =  0.051;
		headNode->m_localTransform.rot.data[2][0] = -0.058;
		headNode->m_localTransform.rot.data[2][1] = -0.037;
		headNode->m_localTransform.rot.data[2][2] =  0.998;

		NiAVObject::NiUpdateData* ud = nullptr;
		headNode->UpdateWorldData(ud);
	}

    void Skeleton::insertSaveState(const std::string& name, NiNode* node) {
        NiNode* fn = getNode(name.c_str(), node);

        if (!fn) {
            _MESSAGE("cannot find node %s", name.c_str());
            return;
        }

        auto it = savedStates.find(fn->m_name.c_str());
        if (it == savedStates.end()) {
            fn->m_localTransform.pos = boneLocalDefault[fn->m_name.c_str()];
            savedStates.emplace(fn->m_name.c_str(), fn->m_localTransform);
        } else {
            node->m_localTransform.pos = boneLocalDefault[name];
            it->second = node->m_localTransform;
        }
        _MESSAGE("inserted %s", name.c_str());
    }

	void Skeleton::initLocalDefaults() {
		detectInPowerArmor();

		boneLocalDefault.clear();
		if (_inPowerArmor) {
			boneLocalDefault.insert({ "Root", NiPoint3(-0.000000, -0.000000, 0.000000) });
			boneLocalDefault["COM"] = NiPoint3(-0.000031, -3.749783, 89.419518);
			boneLocalDefault["Pelvis"] = NiPoint3(0.000000, 0.000000, 0.000000);
			boneLocalDefault["LLeg_Thigh"] = NiPoint3(4.548744, -1.329979, 6.908315);
			boneLocalDefault["LLeg_Calf"] = NiPoint3(34.297993, 0.000030, 0.000008);
			boneLocalDefault["LLeg_Foot"] = NiPoint3(52.541229, -0.000018, -0.000018);
			boneLocalDefault["RLeg_Thigh"] = NiPoint3(4.547630, -1.324275, -6.897992);
			boneLocalDefault["RLeg_Calf"] = NiPoint3(34.297920, 0.000020, 0.000029);
			boneLocalDefault["RLeg_Foot"] = NiPoint3(52.540794, 0.000001, -0.000002);
			boneLocalDefault["SPINE1"] = NiPoint3(5.750534, -0.002948, -0.000008);
			boneLocalDefault["SPINE2"] = NiPoint3(5.625465, 0.000000, 0.000000);
			boneLocalDefault["Chest"] = NiPoint3(5.536583, -0.000000, 0.000000);
			boneLocalDefault["LArm_Collarbone"] = NiPoint3(22.192039, 0.348167, 1.004177);
			boneLocalDefault["LArm_UpperArm"] = NiPoint3(14.598365, 0.000075, 0.000146);
			boneLocalDefault["LArm_ForeArm1"] = NiPoint3(19.536854, 0.419780, 0.045785);
			boneLocalDefault["LArm_ForeArm2"] = NiPoint3(0.000168, 0.000150, 0.000153);
			boneLocalDefault["LArm_ForeArm3"] = NiPoint3(10.000494, 0.000162, -0.000004);
			boneLocalDefault["LArm_Hand"] = NiPoint3(26.964424, 0.000185, 0.000400);
			boneLocalDefault["RArm_Collarbone"] = NiPoint3(22.191887, 0.348147, -1.003999);
			boneLocalDefault["RArm_UpperArm"] = NiPoint3(14.598806, 0.000044, 0.000044);
			boneLocalDefault["RArm_ForeArm1"] = NiPoint3(19.536560, 0.419853, -0.046183);
			boneLocalDefault["RArm_ForeArm2"] = NiPoint3(-0.000053, -0.000068, -0.000137);
			boneLocalDefault["RArm_ForeArm3"] = NiPoint3(10.000502, -0.000104, 0.000120);
			boneLocalDefault["RArm_Hand"] = NiPoint3(26.964560, 0.000064, 0.001247);
			boneLocalDefault["Neck"] = NiPoint3(24.293526, -2.841563, 0.000019);
			boneLocalDefault["Head"] = NiPoint3(8.224354, 0.0, 0.0);
		}
		else {
			boneLocalDefault.insert({ "Root", NiPoint3(-0.000000, -0.000000, 0.000000) });
			boneLocalDefault["COM"] = NiPoint3(2.0, 1.5, 68.911301);
			boneLocalDefault["Pelvis"] = NiPoint3(0.000000, 0.000000, 0.000000);
			boneLocalDefault["LLeg_Thigh"] = NiPoint3(0.000008, 0.000431, 6.615070);
			boneLocalDefault["LLeg_Calf"] = NiPoint3(31.595196, 0.000021, -0.000031);
			boneLocalDefault["LLeg_Foot"] = NiPoint3(31.942888, -0.000018, -0.000018);
			boneLocalDefault["RLeg_Thigh"] = NiPoint3(0.000008, 0.000431, -6.615068);
			boneLocalDefault["RLeg_Calf"] = NiPoint3(31.595139, 0.000020, 0.000029);
			boneLocalDefault["RLeg_Foot"] = NiPoint3(31.942570, 0.000001, -0.000002);
			boneLocalDefault["SPINE1"] = NiPoint3(3.791992, -0.002948, -0.000008);
			boneLocalDefault["SPINE2"] = NiPoint3(8.704666, 0.000000, 0.000000);
			boneLocalDefault["Chest"] = NiPoint3(9.956283, -0.000000, 0.000000);
			boneLocalDefault["LArm_Collarbone"] = NiPoint3(19.153227, -0.510421, 1.695077);
			boneLocalDefault["LArm_UpperArm"] = NiPoint3(12.536572, 0.000003, 0.000000);
			boneLocalDefault["LArm_ForeArm1"] = NiPoint3(17.968307, 0.000019, 0.000007);
			boneLocalDefault["LArm_ForeArm2"] = NiPoint3(6.151599, -0.000022, -0.000045);
			boneLocalDefault["LArm_ForeArm3"] = NiPoint3(6.151586, -0.000068, 0.000016);
			boneLocalDefault["LArm_Hand"] = NiPoint3(6.151584, 0.000033, -0.000065);
			boneLocalDefault["RArm_Collarbone"] = NiPoint3(19.153219, -0.510418, -1.695124);
			boneLocalDefault["RArm_UpperArm"] = NiPoint3(12.534259, 0.000007, -0.000013);
			boneLocalDefault["RArm_ForeArm1"] = NiPoint3(17.970503, 0.000103, -0.000056);
			boneLocalDefault["RArm_ForeArm2"] = NiPoint3(6.152768, 0.000040, -0.000039);
			boneLocalDefault["RArm_ForeArm3"] = NiPoint3(6.152856, -0.000009, -0.000091);
			boneLocalDefault["RArm_Hand"] = NiPoint3(6.152939, 0.000044, -0.000029);
			boneLocalDefault["Neck"] = NiPoint3(22.084, -3.767, 0.0);
			boneLocalDefault["Head"] = NiPoint3(8.224354, 0.0, 0.0);

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
        auto it = savedStates.find(name);
        if (it != savedStates.end()) {
            node->m_localTransform = it->second;
        }

        for (auto i = 0; i < node->m_children.m_emptyRunStart; ++i) {
            if (auto nextNode = node->m_children.m_data[i] ? node->m_children.m_data[i]->GetAsNiNode() : nullptr) {
                this->restoreLocals(nextNode);
            }
        }
    }

	bool Skeleton::setNodes() {
		QueryPerformanceFrequency(&freqCounter);
		QueryPerformanceCounter(&timer);

		std::srand(static_cast<unsigned int>(time(nullptr)));

		_offHandGripping = false;
		_hasLetGoGripButton = false;
		_prevSpeed = 0.0;

		_playerNodes = reinterpret_cast<PlayerNodes*>(reinterpret_cast<char*>(*g_player) + 0x6E0);

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
		_leftHand  = getNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		
		if (!_rightHand || !_leftHand) {
			return false;
		}

		_rightHandPrevFrame = _rightHand->m_worldTransform;
		_leftHandPrevFrame = _leftHand->m_worldTransform;

		NiNode* screenNode = _playerNodes->ScreenNode;

		if (NiNode* screenNode = _playerNodes->ScreenNode) {
			if (NiAVObject* screen = screenNode->GetObjectByName(&BSFixedString("Screen:0"))) {
				_pipboyScreenPrevFrame = screen->m_worldTransform;
			}
		}

		_spine = this->getNode("SPINE2", _root);
		_chest = this->getNode("Chest", _root);

		_MESSAGE("common node = %016I64X", _common);
		_MESSAGE("righthand node = %016I64X", _rightHand);
		_MESSAGE("lefthand node = %016I64X", _leftHand);

		_weaponEquipped = false;

		// Setup Arms
		const std::vector<std::pair<BSFixedString, NiAVObject**>> armNodes = {
			{"RArm_Collarbone", &rightArm.shoulder},
			{"RArm_UpperArm", &rightArm.upper},
			{"RArm_UpperTwist1", &rightArm.upperT1},
			{"RArm_ForeArm1", &rightArm.forearm1},
			{"RArm_ForeArm2", &rightArm.forearm2},
			{"RArm_ForeArm3", &rightArm.forearm3},
			{"RArm_Hand", &rightArm.hand},
			{"LArm_Collarbone", &leftArm.shoulder},
			{"LArm_UpperArm", &leftArm.upper},
			{"LArm_UpperTwist1", &leftArm.upperT1},
			{"LArm_ForeArm1", &leftArm.forearm1},
			{"LArm_ForeArm2", &leftArm.forearm2},
			{"LArm_ForeArm3", &leftArm.forearm3},
			{"LArm_Hand", &leftArm.hand}
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

		savedStates.clear();
		saveStatesTree(_root->m_parent->GetAsNiNode());
		_MESSAGE("finished saving tree");

		initBoneTreeMap(_root);
		return true;
	}

	void Skeleton::positionDiff() {
		NiPoint3 firstpos = _playerNodes->HmdNode->m_worldTransform.pos;
		NiPoint3 skellypos = _root->m_worldTransform.pos;

		_MESSAGE("difference = %f %f %f", (firstpos.x - skellypos.x), (firstpos.y - skellypos.y), (firstpos.z - skellypos.z));

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


	float Skeleton::getNeckYaw() {

		if (!_playerNodes) {
			_MESSAGE("playernodes not set in neck yaw");
			return 0.0;
		}

		NiPoint3 pos = _playerNodes->UprightHmdNode->m_worldTransform.pos;
		NiPoint3 hmdToLeft  = _playerNodes->SecondaryWandNode->m_worldTransform.pos  - pos;
		NiPoint3 hmdToRight = _playerNodes->primaryWandNode->m_worldTransform.pos - pos;
		float weight = 1.0f;

		if ((vec3_len(hmdToLeft) < 10.0f) || (vec3_len(hmdToRight) < 10.0f)) {
			return 0.0;
		}

		// handle excessive angles when hand is above the head.
		if (hmdToLeft.z > 0) {
			weight = (std::max)((float)(weight - (0.05 * hmdToLeft.z)), 0.0f);
		}

		if (hmdToRight.z > 0) {
			weight = (std::max)((float)(weight - (0.05 * hmdToRight.z)), 0.0f);
		}

		// hands moving across the chest rotate too much.   try to handle with below
		// wp = parWp + parWr * lp =>   lp = (wp - parWp) * parWr'
		NiPoint3 locLeft  = _playerNodes->HmdNode->m_worldTransform.rot.Transpose() * hmdToLeft;
		NiPoint3 locRight = _playerNodes->HmdNode->m_worldTransform.rot.Transpose() * hmdToRight;

		if (locLeft.x > locRight.x) {
			float delta = locRight.x - locLeft.x;
			weight = (std::max)((float)(weight + (0.02 * delta)), 0.0f);
		}

		NiPoint3 sum = hmdToRight + hmdToLeft;

		NiPoint3 forwardDir = vec3_norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * vec3_norm(sum));  // rotate sum to local hmd space to get the proper angle
		NiPoint3 hmdForwardDir = vec3_norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * _playerNodes->HmdNode->m_localTransform.pos);

		float anglePrime = atan2f(forwardDir.x, forwardDir.y);
		float angleSec = atan2f(forwardDir.x, forwardDir.z);
		float angleFinal;

		sum = vec3_norm(sum);

		float pitchDiff = atan2f(hmdForwardDir.y, hmdForwardDir.z) - atan2f(forwardDir.z, forwardDir.y);

        angleFinal = (fabs(pitchDiff) > degrees_to_rads(80.0f)) ? angleSec : anglePrime;
		return std::clamp(-angleFinal * weight, degrees_to_rads(-50.0f), degrees_to_rads(50.0f));
	}

    float Skeleton::getNeckPitch() {
        const NiPoint3& lookDir = vec3_norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * _playerNodes->HmdNode->m_localTransform.pos);
        return atan2f(lookDir.y, lookDir.z);
    }

    float Skeleton::getBodyPitch() {
        constexpr float basePitch = 105.3f;
        constexpr float weight = 0.1f;

        float curHeight = c_playerHeight;
        float heightCalc = std::abs((curHeight - _playerNodes->UprightHmdNode->m_localTransform.pos.z) / curHeight);

        float angle = heightCalc * (basePitch + weight * rads_to_degrees(getNeckPitch()));

        return degrees_to_rads(angle);
    }

    void Skeleton::setUnderHMD(float groundHeight) {
        detectInPowerArmor();

        if (c_disableSmoothMovement) {
            _playerNodes->playerworldnode->m_localTransform.pos.z = _inPowerArmor ? (c_PACameraHeight + c_dynamicCameraHeight) : (c_cameraHeight + c_dynamicCameraHeight);
            updateDown(_playerNodes->playerworldnode, true);
        }

        float z = _root->m_localTransform.pos.z;
        float neckYaw = getNeckYaw();
        float neckPitch = getNeckPitch();

        Quaternion qa;
        qa.setAngleAxis(-neckPitch, NiPoint3(-1, 0, 0));

        Matrix44 mat = qa.getRot();
        NiMatrix43 newRot = mat.multiply43Left(_playerNodes->HmdNode->m_localTransform.rot);

        _forwardDir = rotateXY(NiPoint3(newRot.data[1][0], newRot.data[1][1], 0), neckYaw * 0.7f);
        _sidewaysRDir = NiPoint3(_forwardDir.y, -_forwardDir.x, 0);

        NiNode* body = _root->m_parent->GetAsNiNode();
        body->m_localTransform.pos = NiPoint3(0.0f, 0.0f, 0.0f);
        body->m_worldTransform.pos = this->getPosition();
		body->m_worldTransform.pos.z = 0.0f;
        body->m_worldTransform.pos.z += _playerNodes->playerworldnode->m_localTransform.pos.z;

        NiPoint3 back = vec3_norm(NiPoint3(_forwardDir.x, _forwardDir.y, 0));
        NiPoint3 bodydir = NiPoint3(0, 1, 0);

        mat.rotateVectoVec(back, bodydir);
        _root->m_localTransform.rot = mat.multiply43Left(body->m_worldTransform.rot.Transpose());
        _root->m_localTransform.pos = body->m_worldTransform.pos - getPosition();
        _root->m_localTransform.pos.z = z;
        _root->m_localTransform.scale = c_playerHeight / defaultCameraHeight;  // set scale based on specified user height
    }

    void Skeleton::setBodyPosture() {
        uint64_t dominantHand = c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed;
        const auto UISelectButton = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)33); // Right Trigger

        float neckPitch = getNeckPitch();
        float bodyPitch = _inPowerArmor ? getBodyPitch() : getBodyPitch() / 1.2f;

        NiNode* camera = (*g_playerCamera)->cameraNode;
        NiNode* com = getNode("COM", _root);
        NiNode* neck = getNode("Neck", _root);
        NiNode* spine = getNode("SPINE1", _root);

        _leftKneePos = getNode("LLeg_Calf", com)->m_worldTransform.pos;
        _rightKneePos = getNode("RLeg_Calf", com)->m_worldTransform.pos;

        com->m_localTransform.pos.x = 0.0f;
        com->m_localTransform.pos.y = 0.0f;

        float z_adjust = (_inPowerArmor ? c_powerArmor_up : c_playerOffset_up) - cosf(neckPitch) * (5.0f * _root->m_localTransform.scale);
        NiPoint3 neckAdjust = NiPoint3(-_forwardDir.x * c_playerOffset_forward / 2, -_forwardDir.y * c_playerOffset_forward / 2, z_adjust);
        NiPoint3 neckPos = camera->m_worldTransform.pos + neckAdjust;

        _torsoLen = vec3_len(neck->m_worldTransform.pos - com->m_worldTransform.pos);

        NiPoint3 hmdToHip = neckPos - com->m_worldTransform.pos;
        NiPoint3 dir = NiPoint3(-_forwardDir.x, -_forwardDir.y, 0);

        float dist = tanf(bodyPitch) * vec3_len(hmdToHip);
        NiPoint3 tmpHipPos = com->m_worldTransform.pos + dir * (dist / vec3_len(dir));
        tmpHipPos.z = com->m_worldTransform.pos.z;

        NiPoint3 hmdtoNewHip = tmpHipPos - neckPos;
        NiPoint3 newHipPos = neckPos + hmdtoNewHip * (_torsoLen / vec3_len(hmdtoNewHip));

        NiPoint3 newPos = com->m_localTransform.pos + _root->m_worldTransform.rot.Transpose() * (newHipPos - com->m_worldTransform.pos);
        float offsetFwd = _inPowerArmor ? c_powerArmor_forward : c_playerOffset_forward;
        com->m_localTransform.pos.y += (newPos.y + offsetFwd);
        com->m_localTransform.pos.z = _inPowerArmor ? newPos.z / 1.7f : newPos.z / 1.5f;

        NiNode* body = _root->m_parent->GetAsNiNode();
        body->m_worldTransform.pos.z -= _inPowerArmor ? (c_PACameraHeight + c_PARootOffset) : (c_cameraHeight + c_RootOffset);

        Matrix44 rot;
        rot.rotateVectoVec(neckPos - tmpHipPos, hmdToHip);
        NiMatrix43 mat = rot.multiply43Left(spine->m_parent->m_worldTransform.rot.Transpose());
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

	void Skeleton::fixArmor() {
		NiNode* lPauldron = getNode("L_Pauldron", _root);
		NiNode* rPauldron = getNode("R_Pauldron", _root);

		if (!lPauldron || !rPauldron) {
			return;
		}

		//float delta = getNode("LArm_Collarbone", _root)->m_worldTransform.pos.z - _root->m_worldTransform.pos.z;
		float delta = getNode("LArm_UpperArm", _root)->m_worldTransform.pos.z - _root->m_worldTransform.pos.z;
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

        NiNode* lKnee = getNode("LLeg_Calf", lHip);
        NiNode* rKnee = getNode("RLeg_Calf", rHip);
        NiNode* lFoot = getNode("LLeg_Foot", lHip);
        NiNode* rFoot = getNode("RLeg_Foot", rHip);

        if (!lKnee || !rKnee || !lFoot || !rFoot) {
            return;
        }

        // Move feet closer together
        NiPoint3 leftToRight = (rFoot->m_worldTransform.pos - lFoot->m_worldTransform.pos) * (_inPowerArmor ? -0.15f : 0.3f);
        lFoot->m_worldTransform.pos += leftToRight;
        rFoot->m_worldTransform.pos -= leftToRight;

        // Calculate direction vector
        NiPoint3 dir = _curPos - _lastPos;
        dir.z = 0;
        dir = vec3_norm(dir);

        double curSpeed = std::clamp(abs(vec3_len(dir)) / _frameTime, 0.0, 350.0);
        if (_prevSpeed > 20.0) {
            curSpeed = (curSpeed + _prevSpeed) / 2;
        }

        double stepTime = std::clamp(cos(curSpeed / 140.0), 0.28, 0.50);

        // If decelerating, reset target
        if ((curSpeed - _prevSpeed) < -20.0f) {
            _walkingState = 3;
        }

        _prevSpeed = curSpeed;

        static float spineAngle = 0.0f;

        // Setup current walking state based on velocity and previous state
        if (!c_jumping) {
            switch (_walkingState) {
                case 0:
                    if (curSpeed >= 35.0) {
                        _walkingState = 1;  // Start walking
                        _footStepping = std::rand() % 2 + 1;  // Pick a random foot to take a step
                        _stepDir = dir;
                        _stepTimeinStep = stepTime;
                        delayFrame = 2;

                        if (_footStepping == 1) {
                            _rightFootTarget = rFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 1.5);
                            _rightFootStart = rFoot->m_worldTransform.pos;
                            _leftFootTarget = lFoot->m_worldTransform.pos;
                            _leftFootStart = lFoot->m_worldTransform.pos;
                        } else {
                            _rightFootTarget = rFoot->m_worldTransform.pos;
                            _rightFootStart = rFoot->m_worldTransform.pos;
                            _leftFootTarget = lFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 1.5);
                            _leftFootStart = lFoot->m_worldTransform.pos;
                        }
                        _leftFootPos = _leftFootStart;
                        _rightFootPos = _rightFootStart;
                        _currentStepTime = stepTime / 2;
                    } else {
                        _currentStepTime = 0.0;
                        _footStepping = 0;
                        spineAngle = 0.0;
                    }
                    break;
                case 1:
                    if (curSpeed < 20.0) {
                        _walkingState = 2;  // Begin process to stop walking
                        _currentStepTime = 0.0;
                    }
                    break;
                case 2:
                    if (curSpeed >= 20.0) {
                        _walkingState = 1;  // Resume walking
                        _currentStepTime = 0.0;
                    }
                    break;
                case 3:
                    _stepDir = dir;
                    if (_footStepping == 1) {
                        _rightFootTarget = rFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 0.1);
                    } else {
                        _leftFootTarget = lFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 0.1);
                    }
                    _walkingState = 1;
                    break;
                default:
                    _walkingState = 0;
                    break;
            }
        } else {
            _walkingState = 0;
        }

        if (_walkingState == 0) {
            // We're standing still, so just set foot positions accordingly
            _leftFootPos = lFoot->m_worldTransform.pos;
            _rightFootPos = rFoot->m_worldTransform.pos;
            _leftFootPos.z = _root->m_worldTransform.pos.z;
            _rightFootPos.z = _root->m_worldTransform.pos.z;
            return;
        }

        NiPoint3 dirOffset = dir - _stepDir;
        float dot = vec3_dot(dir, _stepDir);
        double scale = (std::min)(curSpeed * stepTime * 1.5, 140.0);
        dirOffset *= scale;

        int sign = 1;
        _currentStepTime += _frameTime;
        double frameStep = _frameTime / _stepTimeinStep;
        double interp = std::clamp(frameStep * (_currentStepTime / _frameTime), 0.0, 1.0);

        if (_footStepping == 1) {
            sign = -1;
            if (dot < 0.9) {
                if (!delayFrame) {
                    _rightFootTarget += dirOffset;
                    _stepDir = dir;
                    delayFrame = 2;
                } else {
                    delayFrame--;
                }
            } else {
                delayFrame = delayFrame == 2 ? delayFrame : delayFrame + 1;
            }
            _rightFootTarget.z = _root->m_worldTransform.pos.z;
            _rightFootStart.z = _root->m_worldTransform.pos.z;
            _rightFootPos = _rightFootStart + ((_rightFootTarget - _rightFootStart) * interp);
            double stepAmount = std::clamp(vec3_len(_rightFootTarget - _rightFootStart) / 150.0, 0.0, 1.0);
            double stepHeight = (std::max)(stepAmount * 9.0, 1.0);
            float up = sinf(interp * PI) * stepHeight;
            _rightFootPos.z += up;
        } else {
            if (dot < 0.9) {
                if (!delayFrame) {
                    _leftFootTarget += dirOffset;
                    _stepDir = dir;
                    delayFrame = 2;
                } else {
                    delayFrame--;
                }
            } else {
                delayFrame = delayFrame == 2 ? delayFrame : delayFrame + 1;
            }
            _leftFootTarget.z = _root->m_worldTransform.pos.z;
            _leftFootStart.z = _root->m_worldTransform.pos.z;
            _leftFootPos = _leftFootStart + ((_leftFootTarget - _leftFootStart) * interp);
            double stepAmount = std::clamp(vec3_len(_leftFootTarget - _leftFootStart) / 150.0, 0.0, 1.0);
            double stepHeight = (std::max)(stepAmount * 9.0, 1.0);
            float up = sinf(interp * PI) * stepHeight;
            _leftFootPos.z += up;
        }

        spineAngle = sign * sinf(interp * PI) * 3.0;
        Matrix44 rot;
        rot.setEulerAngles(degrees_to_rads(spineAngle), 0.0, 0.0);
        _spine->m_localTransform.rot = rot.multiply43Left(_spine->m_localTransform.rot);

        if (_currentStepTime > stepTime) {
            _currentStepTime = 0.0;
            _stepDir = dir;
            _stepTimeinStep = stepTime;

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
    }


	void Skeleton::setLegs() {
		Matrix44 rotatedM;


		NiNode* lHip =  getNode("LLeg_Thigh", _root);
		NiNode* rHip =  getNode("RLeg_Thigh", _root);
		NiNode* lKnee = getNode("LLeg_Calf", lHip);
		NiNode* rKnee = getNode("RLeg_Calf", rHip);
		NiNode* lFoot = getNode("LLeg_Foot", lHip);
		NiNode* rFoot = getNode("RLeg_Foot", rHip);

		NiPoint3 pos = _leftKneePos - lHip->m_worldTransform.pos;
		NiPoint3 uLocalDir = lHip->m_worldTransform.rot.Transpose() * vec3_norm(pos) / lHip->m_worldTransform.scale;
		rotatedM.rotateVectoVec(uLocalDir, lKnee->m_localTransform.pos);
		lHip->m_localTransform.rot = rotatedM.multiply43Left(lHip->m_localTransform.rot);

		pos = _rightKneePos - rHip->m_worldTransform.pos;
		uLocalDir = rHip->m_worldTransform.rot.Transpose() * vec3_norm(pos) / rHip->m_worldTransform.scale;
		rotatedM.rotateVectoVec(uLocalDir, rKnee->m_localTransform.pos);
		rHip->m_localTransform.rot = rotatedM.multiply43Left(rHip->m_localTransform.rot);

		updateDown(lHip, true);
		updateDown(rHip, true);

		// now calves
		pos = _leftFootPos - lKnee->m_worldTransform.pos;
		uLocalDir = lKnee->m_worldTransform.rot.Transpose() * vec3_norm(pos) / lKnee->m_worldTransform.scale;
		rotatedM.rotateVectoVec(uLocalDir, lFoot->m_localTransform.pos);
		lKnee->m_localTransform.rot = rotatedM.multiply43Left(lKnee->m_localTransform.rot);

		pos = _rightFootPos - rKnee->m_worldTransform.pos;
		uLocalDir = rKnee->m_worldTransform.rot.Transpose() * vec3_norm(pos) / rKnee->m_worldTransform.scale;
		rotatedM.rotateVectoVec(uLocalDir, rFoot->m_localTransform.pos);
		rKnee->m_localTransform.rot = rotatedM.multiply43Left(rKnee->m_localTransform.rot);
	}

	// adapted solver from VRIK.  Thanks prog!
	void Skeleton::setSingleLeg(bool isLeft) {
		Matrix44 rotMat;

		NiNode* footNode = isLeft ? getNode("LLeg_Foot", _root) : getNode("RLeg_Foot", _root);
		NiNode* kneeNode = isLeft ? getNode("LLeg_Calf", _root) : getNode("RLeg_Calf", _root);
		NiNode* hipNode  = isLeft ? getNode("LLeg_Thigh", _root) : getNode("RLeg_Thigh", _root);

		NiPoint3 footPos = isLeft ? _leftFootPos : _rightFootPos;
		NiPoint3 kneePos = isLeft ? _leftKneePos : _rightKneePos;
		NiPoint3 hipPos = hipNode->m_worldTransform.pos;

		NiPoint3 footToHip = hipNode->m_worldTransform.pos - footPos;

		NiPoint3 rotV = NiPoint3(0, 1, 0);
		if (_inPowerArmor) {
			rotV.y = 0;
			rotV.z = isLeft ? 1 : -1;
		}
		NiPoint3 hipDir = hipNode->m_worldTransform.rot * rotV;
		NiPoint3 xDir = vec3_norm(footToHip);
		NiPoint3 yDir = vec3_norm(hipDir - xDir * vec3_dot(hipDir, xDir));

		float thighLenOrig = vec3_len(kneeNode->m_localTransform.pos);
		float calfLenOrig = vec3_len(footNode->m_localTransform.pos);
		float thighLen = thighLenOrig;
		float calfLen = calfLenOrig;


		float ftLen = (std::max)(vec3_len(footToHip), 0.1f);

		if (ftLen > thighLen + calfLen) {
			float diff = ftLen - thighLen - calfLen;
			float ratio = calfLen / (calfLen + thighLen);
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
		float xDist = cosf(footAngle) * calfLen;
		float yDist = sinf(footAngle) * calfLen;
		kneePos = footPos + xDir * xDist + yDir * yDist;

		NiPoint3 pos = kneePos - hipPos;
		NiPoint3 uLocalDir = hipNode->m_worldTransform.rot.Transpose() * vec3_norm(pos) / hipNode->m_worldTransform.scale;
		rotMat.rotateVectoVec(uLocalDir, kneeNode->m_localTransform.pos);
		hipNode->m_localTransform.rot = rotMat.multiply43Left(hipNode->m_localTransform.rot);

		NiMatrix43 hipWR;
		rotMat.makeTransformMatrix(hipNode->m_localTransform.rot, NiPoint3(0, 0, 0));
		hipWR = rotMat.multiply43Left(hipNode->m_parent->m_worldTransform.rot);

		NiMatrix43 calfWR;
		rotMat.makeTransformMatrix(kneeNode->m_localTransform.rot, NiPoint3(0, 0, 0));
		calfWR = rotMat.multiply43Left(hipWR);

		uLocalDir = calfWR.Transpose() * vec3_norm(footPos - kneePos) / kneeNode->m_worldTransform.scale;
		rotMat.rotateVectoVec(uLocalDir, footNode->m_localTransform.pos);
		kneeNode->m_localTransform.rot = rotMat.multiply43Left(kneeNode->m_localTransform.rot);

		rotMat.makeTransformMatrix(kneeNode->m_localTransform.rot, NiPoint3(0, 0, 0));
		calfWR = rotMat.multiply43Left(hipWR);

		// Calculate Clp:  Cwp = Twp + Twr * (Clp * Tws) = kneePos   ===>   Clp = Twr' * (kneePos - Twp) / Tws
		//LPOS(nodeCalf) = Twr.Transpose() * (kneePos - thighPos) / WSCL(nodeThigh);
		kneeNode->m_localTransform.pos = hipWR.Transpose() * (kneePos - hipPos) / hipNode->m_worldTransform.scale;
		if (vec3_len(kneeNode->m_localTransform.pos) > thighLenOrig) {
			kneeNode->m_localTransform.pos = vec3_norm(kneeNode->m_localTransform.pos) * thighLenOrig;
		}

		// Calculate Flp:  Fwp = Cwp + Cwr * (Flp * Cws) = footPos   ===>   Flp = Cwr' * (footPos - Cwp) / Cws
		//LPOS(nodeFoot) = Cwr.Transpose() * (footPos - kneePos) / WSCL(nodeCalf);
		footNode->m_localTransform.pos = calfWR.Transpose() * (footPos - kneePos) / kneeNode->m_worldTransform.scale;
		if (vec3_len(footNode->m_localTransform.pos) > calfLenOrig) {
			footNode->m_localTransform.pos = vec3_norm(footNode->m_localTransform.pos) * calfLenOrig;
		}
	}

    void Skeleton::rotateLeg(uint32_t pos, float angle) {
        BSFlattenedBoneTree* rt = reinterpret_cast<BSFlattenedBoneTree*>(_root);
        Matrix44 rot;
        rot.setEulerAngles(degrees_to_rads(angle), 0, 0);

        auto& transform = rt->transforms[pos];
        transform.local.rot = rot.multiply43Left(transform.local.rot);

        const auto& parentTransform = rt->transforms[transform.parPos];
        NiPoint3 p = parentTransform.world.rot * (transform.local.pos * parentTransform.world.scale);
        transform.world.pos = parentTransform.world.pos + p;

        rot.makeTransformMatrix(transform.local.rot, NiPoint3(0, 0, 0));
        transform.world.rot = rot.multiply43Left(parentTransform.world.rot);
    }

	void Skeleton::fixPAArmor() {

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
		}
		else {
			oneTime = false;
		}
	}

	void Skeleton::setBodyLen() {
		_torsoLen = vec3_len(getNode("Camera", _root)->m_worldTransform.pos - getNode("COM", _root)->m_worldTransform.pos);
		_torsoLen *= c_playerHeight / defaultCameraHeight;

		_legLen = vec3_len(getNode("LLeg_Thigh", _root)->m_worldTransform.pos - getNode("Pelvis", _root)->m_worldTransform.pos);
		_legLen += vec3_len(getNode("LLeg_Calf", _root)->m_worldTransform.pos - getNode("LLeg_Thigh", _root)->m_worldTransform.pos);
		_legLen += vec3_len(getNode("LLeg_Foot", _root)->m_worldTransform.pos - getNode("LLeg_Calf", _root)->m_worldTransform.pos);
		_legLen *= c_playerHeight / defaultCameraHeight;
	}

    void Skeleton::hideWeapon() {
        static BSFixedString nodeName("Weapon");

        if (NiAVObject* weapon = rightArm.hand->GetObjectByName(&nodeName)) {
            weapon->flags |= 0x1;
            weapon->m_localTransform.scale = 0.0;
            if (NiNode* weaponNode = weapon->GetAsNiNode()) {
                for (auto i = 0; i < weaponNode->m_children.m_emptyRunStart; ++i) {
                    if (NiNode* nextNode = weaponNode->m_children.m_data[i]->GetAsNiNode()) {
                        nextNode->flags |= 0x1;
                        nextNode->m_localTransform.scale = 0.0;
                    }
                }
            }
        }
    }

	void Skeleton::swapPipboy() {
		_pipboyStatus = false;
		_pipTimer = 0;
	}

	void Skeleton::positionPipboy() {
		static BSFixedString wandPipName("PipboyRoot_NIF_ONLY");
		NiAVObject* wandPip = _playerNodes->SecondaryWandNode->GetObjectByName(&wandPipName);

		if (wandPip == nullptr) {
			return;
		}

		static BSFixedString nodeName("PipboyBone");
		NiAVObject* pipboyBone;
		if (c_leftHandedPipBoy) {
			pipboyBone = rightArm.forearm1->GetObjectByName(&nodeName);
		}
		else {
			pipboyBone = leftArm.forearm1->GetObjectByName(&nodeName);
		}

		if (pipboyBone == nullptr) {
			return;
		}

		NiPoint3 locpos = NiPoint3(0, 0, 0);

		locpos = (pipboyBone->m_worldTransform.rot * (locpos * pipboyBone->m_worldTransform.scale));

		NiPoint3 wandWP = pipboyBone->m_worldTransform.pos + locpos;

		NiPoint3 delta = wandWP - wandPip->m_parent->m_worldTransform.pos;

		wandPip->m_localTransform.pos = wandPip->m_parent->m_worldTransform.rot.Transpose() * (delta / wandPip->m_parent->m_worldTransform.scale);

		// Slr = LHwr' * RHwr * Slr
		Matrix44 loc;
		loc.setEulerAngles(degrees_to_rads(30), 0, 0);

		NiMatrix43 wandWROT = loc.multiply43Left(pipboyBone->m_worldTransform.rot);

		loc.makeTransformMatrix(wandWROT, NiPoint3(0, 0, 0));
		wandPip->m_localTransform.rot = loc.multiply43Left(wandPip->m_parent->m_worldTransform.rot.Transpose());
	}

	void Skeleton::leftHandedModePipboy() {

		if (c_leftHandedPipBoy) {
			NiNode* pipbone = getNode("PipboyBone", rightArm.forearm1->GetAsNiNode());

			if (!pipbone) {
				pipbone = getNode("PipboyBone", leftArm.forearm1->GetAsNiNode());

				if (!pipbone) {
					return;
				}

				pipbone->m_parent->RemoveChild(pipbone);
				rightArm.forearm3->GetAsNiNode()->AttachChild(pipbone, true);
			}

			Matrix44 rot;
			rot.setEulerAngles(0, degrees_to_rads(180.0), 0);

			pipbone->m_localTransform.rot = rot.multiply43Left(pipbone->m_localTransform.rot);
			pipbone->m_localTransform.pos *= -1.5;
		}
	}

	void Skeleton::makeArmsT(bool isLeft) {
		ArmNodes arm;

		arm = isLeft ? leftArm : rightArm;
		Matrix44 mat;
		mat.makeIdentity();

		arm.forearm1->m_localTransform.rot = mat.make43();
		arm.forearm2->m_localTransform.rot = mat.make43();
		arm.forearm3->m_localTransform.rot = mat.make43();
		arm.hand->m_localTransform.rot = mat.make43();
		arm.upper->m_localTransform.rot = mat.make43();
//		arm.shoulder->m_localTransform.rot = mat.make43();

		return;
	}

	ArmNodes Skeleton::getArm(bool isLeft) {
		return isLeft ? leftArm : rightArm;
	}

	void Skeleton::fixMelee() {

		_weaponEquipped = false;

		BGSInventoryList* inventory = (*g_player)->inventoryList;

		if (!inventory) {
			return;
		}
		
		bool isMelee = Offsets::CombatUtilities_IsActorUsingMelee(*g_player);

		for (int i = 0; i < inventory->items.count; i++) {
			BGSInventoryItem item;

			inventory->items.GetNthItem(i, item);

			if (!item.form) {
				continue;
			}

			if ((item.form->formType == FormType::kFormType_WEAP) && (item.stack->flags & 0x3))   {
				// check if weapon is sheathed or not
				if ((*g_player)->actorState.IsWeaponDrawn()) {
					_weaponEquipped = true;
				}

				TESObjectWEAP* weap = static_cast<TESObjectWEAP*>(item.form);

				uint8_t type = weap->weapData.unk137; // unk137 is the weapon type that maps to WeaponType enum

				if (isMelee) {

					NiAVObject* wNode = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());

					Matrix44 rot;
					rot.setEulerAngles(degrees_to_rads(85.0f), degrees_to_rads(-70.0f), degrees_to_rads(0.0f));
					wNode->m_localTransform.rot = rot.multiply43Right(wNode->m_localTransform.rot);

					updateDown(wNode->GetAsNiNode(), true);

					(*g_player)->Update(0.0f);
					break;

				}
			}
		}
	}


		// Thanks Shizof and SmoothtMovementVR for below code
    bool HasKeyword(TESObjectARMO* armor, UInt32 keywordFormId)
    {
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

		// Thanks Shizof and SmoothtMovementVR for below code
		if ((*g_player)->equipData) {
			if ((*g_player)->equipData->slots[0x03].item != nullptr)
			{
				TESForm* equippedForm = (*g_player)->equipData->slots[0x03].item;
				if (equippedForm)
				{
					if (equippedForm->formType == TESObjectARMO::kTypeID)
					{
						TESObjectARMO* armor = DYNAMIC_CAST(equippedForm, TESForm, TESObjectARMO);

						if (armor)
						{
							if (HasKeyword(armor, KeywordPowerArmor) || HasKeyword(armor, KeywordPowerArmorFrame))
							{
								_inPowerArmor = true;
								return true;
							}
							else
							{
								_inPowerArmor = false;
								return false;
							}
						}
					}
				}
			}
		}
		return false;

	}

	void Skeleton::setWandsVisibility(bool a_show, wandMode a_mode) {
		auto setVisibilityForNode = [this, a_show](NiNode* node) {
			for (int i = 0; i < node->m_children.m_emptyRunStart; ++i) {
				if (auto child = node->m_children.m_data[i]) {
					if (auto triShape = child->GetAsBSTriShape()) {
						setVisibility(triShape, a_show);
						break;
					} else if (!_stricmp(child->m_name.c_str(), "")) {
						setVisibility(child, a_show);
						if (auto grandChild = child->GetAsNiNode()->m_children.m_data[0]) {
							setVisibility(grandChild, a_show);
						}
						break;
					}
				}
			}
		};

		if (a_mode == both || (a_mode == mainhandWand && !c_leftHandedMode) || (a_mode == offhandWand && c_leftHandedMode)) {
			setVisibilityForNode(_playerNodes->primaryWandNode);
		}

		if (a_mode == both || (a_mode == mainhandWand && c_leftHandedMode) || (a_mode == offhandWand && !c_leftHandedMode)) {
			setVisibilityForNode(_playerNodes->SecondaryWandNode);
		}
	}

	void Skeleton::showWands(wandMode a_mode) {
		setWandsVisibility(true, a_mode);
	}

	void Skeleton::hideWands(wandMode a_mode) {
		setWandsVisibility(false, a_mode);
	}

	void Skeleton::hideFistHelpers() {
		if (!c_leftHandedMode) {
			NiAVObject* node = getNode("fist_M_Right_HELPER", _playerNodes->primaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;   // first bit sets the cull flag so it will be hidden;
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
				node->flags |= 0x1;   // first bit sets the cull flag so it will be hidden;
			}

			node = getNode("fist_F_Left_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}

			node = getNode("PA_fist_L_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;
			}

		}
		else {
			NiAVObject* node = getNode("fist_M_Right_HELPER", _playerNodes->SecondaryWandNode);
			if (node != nullptr) {
				node->flags |= 0x1;   // first bit sets the cull flag so it will be hidden;
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
				node->flags |= 0x1;   // first bit sets the cull flag so it will be hidden;
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

		NiAVObject* uiNode = getNode("Point002", _playerNodes->SecondaryWandNode);

		if (uiNode) {
			uiNode->m_localTransform.scale = 0.0;
		}
	}

	bool Skeleton::isLookingAtPipBoy() {
		BSFixedString wandPipName("PipboyRoot_NIF_ONLY");
		NiAVObject* pipboy = _playerNodes->SecondaryWandNode->GetObjectByName(&wandPipName);

		if (pipboy == nullptr) {
			return false;
		}

		BSFixedString screenName("Screen:0");
		NiAVObject* screen = pipboy->GetObjectByName(&screenName);

		NiPoint3 pipBoyOut = screen->m_worldTransform.rot * NiPoint3(0, -1, 0);
		NiPoint3 lookDir = (*g_playerCamera)->cameraNode->m_worldTransform.rot * NiPoint3(0, 1, 0);

		float dot = vec3_dot(vec3_norm(pipBoyOut), vec3_norm(lookDir));

		return dot < -(c_pipBoyLookAtGate);

	}

    void Skeleton::hidePipboy() {
        BSFixedString pipName("PipboyBone");
        NiAVObject* pipboy = nullptr;

        if (!c_leftHandedPipBoy) {
            if (leftArm.forearm3) {
                pipboy = leftArm.forearm3->GetObjectByName(&pipName);
            }
        } else {
            pipboy = rightArm.forearm3->GetObjectByName(&pipName);
        }

        if (!pipboy) {
            return;
        }

        // Changed to allow scaling of third person Pipboy --->
        if (!c_hidePipboy) {
            if (pipboy->m_localTransform.scale != c_pipBoyScale) {
                pipboy->m_localTransform.scale = c_pipBoyScale;
                toggleVis(pipboy->GetAsNiNode(), false, true);
            }
        } else {
            if (pipboy->m_localTransform.scale != 0.0) {
                pipboy->m_localTransform.scale = 0.0;
                toggleVis(pipboy->GetAsNiNode(), true, true);
            }
        }
    }

	void Skeleton::operatePipBoy() {

		if ((*g_player)->firstPersonSkeleton == nullptr) {
			return;
		}

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;

		NiPoint3 finger;
		NiAVObject* pipboy = nullptr;
		NiAVObject* pipboyTrans = nullptr;
		c_leftHandedPipBoy ? finger = rt->transforms[boneTreeMap["LArm_Finger23"]].world.pos : finger = rt->transforms[boneTreeMap["RArm_Finger23"]].world.pos;
		c_leftHandedPipBoy ? pipboy = getNode("PipboyRoot", rightArm.shoulder->GetAsNiNode()) : pipboy = getNode("PipboyRoot", leftArm.shoulder->GetAsNiNode());
		if (pipboy == nullptr) {
			return;
		}

		const auto pipOnButtonPressed = (c_pipBoyButtonArm ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed :
			VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed) & vr::ButtonMaskFromId((vr::EVRButtonId)c_pipBoyButtonID);
		const auto pipOffButtonPressed = (c_pipBoyButtonOffArm ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed :
			VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed) & vr::ButtonMaskFromId((vr::EVRButtonId)c_pipBoyButtonOffID);

		// check off button
		if (pipOffButtonPressed && !_stickyoffpip) {
			if (_pipboyStatus) {
				_pipboyStatus = false;
				turnPipBoyOff();
				exitPBConfig();
				drawWeapon(); // draw weapon as we no longer need primary trigger as an input.
				disablePipboyHandPose();
				_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				_MESSAGE("Disabling Pipboy with button");
				_stickyoffpip = true;
			}
		}
		else if (!pipOffButtonPressed) {
			// _stickyoffpip is a guard so we don't constantly toggle the pip boy off every frame
			_stickyoffpip = false;
		}

		/* Refactored this part of the code so that turning on the wrist based Pipboy works the same way as the 'Projected Pipboy'. It works on button release rather than press,
		this enables us to determine if the button was held for a short or long press by the status of the '_controlSleepStickyT' bool. If it is still set to true on button release
		then we know the button was a short press, if it is set to false we know it was a long press. Long press = torch on / off, Short Press = Pipboy enable.
		*/

		if (pipOnButtonPressed && !_stickybpip) {
			_stickybpip = true;
			_controlSleepStickyT = true;
			std::thread t5(SecondaryTriggerSleep, 300); // switches a bool to false after 150ms
			t5.detach();
		}
		else if (!pipOnButtonPressed) {
			if (_controlSleepStickyT && _stickybpip) {  // if bool is still set to true on control release we know it was a short press.
				_pipboyStatus = true;
				_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
				turnPipBoyOn();
				holsterWeapon(); // holster weapon so we can use primary trigger as an input.
				setPipboyHandPose();
				c_IsOperatingPipboy = true;
				_MESSAGE("Enabling Pipboy with button");
				_stickybpip = false;
			}
			else {
				// stickypip is a guard so we don't constantly toggle the pip boy every frame
				_stickybpip = false;
			}
		}
		if (!isLookingAtPipBoy()) {
			vr::VRControllerAxis_t axis_state = (c_pipBoyButtonArm > 0) ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0];
			const auto timeElapsed = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _lastLookingAtPip;
			if (_pipboyStatus && timeElapsed > c_pipBoyOffDelay && !_isPBConfigModeActive) {
				_pipboyStatus = false;
				turnPipBoyOff();
				_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				drawWeapon(); // draw weapon as we no longer need primary trigger as an input.
				disablePipboyHandPose();
				c_IsOperatingPipboy = false;
				_MESSAGE("Disabling PipBoy due to inactivity for %d more than %d ms", timeElapsed, c_pipBoyOffDelay);
			}
			else if (c_pipBoyAllowMovementNotLooking && _pipboyStatus && (axis_state.x != 0 || axis_state.y != 0) && !_isPBConfigModeActive) {
				turnPipBoyOff();
				_pipboyStatus = false;
				_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				drawWeapon(); // draw weapon as we no longer need primary trigger as an input.
				disablePipboyHandPose();
				c_IsOperatingPipboy = false;
				_MESSAGE("Disabling PipBoy due to movement when not looking at pipboy. input: (%f, %f)", axis_state.x, axis_state.y);
			}
			return;
		}
		else if (_pipboyStatus)
		{
			_lastLookingAtPip = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		}

		//Why not enable both? So I commented out....

		//if (c_pipBoyButtonMode) // If c_pipBoyButtonMode, don't check touch
			//return;

		float distance;
		//Virtual Power Button Code
		static BSFixedString pwrButtonTrans("PowerTranslate");
		c_leftHandedPipBoy ? pipboy = getNode("PowerDetect", rightArm.shoulder->GetAsNiNode()) : pipboy = getNode("PowerDetect", leftArm.shoulder->GetAsNiNode());
		c_leftHandedPipBoy ? pipboyTrans = rightArm.forearm3->GetObjectByName(&pwrButtonTrans) : pipboyTrans = leftArm.forearm3->GetObjectByName(&pwrButtonTrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		distance = vec3_len(finger - pipboy->m_worldTransform.pos);
		if (distance > 2.0) {
			_pipTimer = 0;
			_stickypip = false;
			pipboyTrans->m_localTransform.pos.z = 0.0;
		}
		else {
			if (_pipTimer < 2) {
				_stickypip = false;
				_pipTimer++;
			}
			else {
				float fz = 0 - (2.0 - distance);
				if (fz >= -0.14 && fz <= 0.0) {
					pipboyTrans->m_localTransform.pos.z = (fz);
				}
				if ((pipboyTrans->m_localTransform.pos.z < -0.10) && (!_stickypip)) {
					_stickypip = true;
					if (vrhook != nullptr) {
						c_leftHandedPipBoy ? vrhook->StartHaptics(1, 0.05, 0.3) : vrhook->StartHaptics(2, 0.05, 0.3);
					}
					if (_pipboyStatus) {
						_pipboyStatus = false;
						turnPipBoyOff();
						exitPBConfig();
						_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
					}
					else {
						_pipboyStatus = true;
						_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
						turnPipBoyOn();
					}
				}
			}
		}
		//Virtual Light Button Code
		static BSFixedString lhtButtontrans("LightTranslate");
		c_leftHandedPipBoy ? pipboy = getNode("LightDetect", rightArm.shoulder->GetAsNiNode()) : pipboy = getNode("LightDetect", leftArm.shoulder->GetAsNiNode());
		c_leftHandedPipBoy ? pipboyTrans = rightArm.forearm3->GetObjectByName(&lhtButtontrans) : pipboyTrans = leftArm.forearm3->GetObjectByName(&lhtButtontrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		distance = vec3_len(finger - pipboy->m_worldTransform.pos);
		if (distance > 2.0) {
			stickyPBlight = false;
			pipboyTrans->m_localTransform.pos.z = 0.0;
		}
		else if (distance <= 2.0) {
			float fz = 0 - (2.0 - distance);
			if (fz >= -0.2 && fz <= 0.0) {
				pipboyTrans->m_localTransform.pos.z = (fz);
			}
			if ((pipboyTrans->m_localTransform.pos.z < -0.14) && (!stickyPBlight)) {
				stickyPBlight = true;
				if (vrhook != nullptr) {
					c_leftHandedPipBoy ? vrhook->StartHaptics(1, 0.05, 0.3) : vrhook->StartHaptics(2, 0.05, 0.3);
				}
				if (!_pipboyStatus) {
					Offsets::togglePipboyLight(*g_player);
				}
			}
		}
		//Virtual Radio Button Code
		static BSFixedString radioButtontrans("RadioTranslate");
		c_leftHandedPipBoy ? pipboy = getNode("RadioDetect", rightArm.shoulder->GetAsNiNode()) : pipboy = getNode("RadioDetect", leftArm.shoulder->GetAsNiNode());
		c_leftHandedPipBoy ? pipboyTrans = rightArm.forearm3->GetObjectByName(&radioButtontrans) : pipboyTrans = leftArm.forearm3->GetObjectByName(&radioButtontrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		distance = vec3_len(finger - pipboy->m_worldTransform.pos);
		if (distance > 2.0) {
			stickyPBRadio = false;
			pipboyTrans->m_localTransform.pos.y = 0.0;
		}
		else if (distance <= 2.0) {
			float fz = 0 - (2.0 - distance);
			if (fz >= -0.15 && fz <= 0.0) {
				pipboyTrans->m_localTransform.pos.y = (fz);
			}
			if ((pipboyTrans->m_localTransform.pos.y < -0.12) && (!stickyPBRadio)) {
				stickyPBRadio = true;
				if (vrhook != nullptr) {
					c_leftHandedPipBoy ? vrhook->StartHaptics(1, 0.05, 0.3) : vrhook->StartHaptics(2, 0.05, 0.3);
				}
				if (!_pipboyStatus) {
					if (Offsets::isPlayerRadioEnabled()) {
						TurnPlayerRadioOn(false);
					}
					else {
						TurnPlayerRadioOn(true);

					}
				}

			}
		}
	}

	// Cylons Code Start >>>>

	/* ==============================================PIPBOY CONTOLS================================================================================
	//
	// UNIVERSIAL CONTROLS
	//
	// root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0); // changes sub tabs
	// root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0); // changes sub tabs
	// root->Invoke("root.Menu_mc.gotoNextPage", nullptr, nullptr, 0); // changes main tabs
	// root->Invoke("root.Menu_mc.gotoPrevPage", nullptr, nullptr, 0); // changes main tabs
	//
	// INV + RADIO TABS CONTROLS
	//
	// root->Invoke("root.Menu_mc.CurrentPage.List_mc.moveSelectionUp", nullptr, nullptr, 0);  // scrolls up page list
	// root->Invoke("root.Menu_mc.CurrentPage.List_mc.moveSelectionDown", nullptr, nullptr, 0); // scrolls down page list
	//
	// DATA TAB CONTROLS
	//
	// root->Invoke("root.Menu_mc.CurrentPage.StatsTab_mc.CategoryList_mc.moveSelectionUp", nullptr, nullptr, 0) // Scrolls stats page list
	// root->Invoke("root.Menu_mc.CurrentPage.StatsTab_mc.CategoryList_mc.moveSelectionDown", nullptr, nullptr, 0) // Scrolls stats page list
	// root->Invoke("root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc.moveSelectionUp", nullptr, nullptr, 0) // Scrolls Quest page List
	// root->Invoke("root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc.moveSelectionDown", nullptr, nullptr, 0) // Scrolls Quest page List
	// root->Invoke("root.Menu_mc.CurrentPage.WorkshopsTab_mc.List_mc.moveSelectionUp", nullptr, nullptr, 0) // Scolls Workshop page list
	// root->Invoke("root.Menu_mc.CurrentPage.WorkshopsTab_mc.List_mc.moveSelectionDown", nullptr, nullptr, 0) // Scolls Workshop page list
	//
	// STATS TAB CONTROLS
	//
	// root->Invoke("root.Menu_mc.CurrentPage.PerksTab_mc.List_mc.moveSelectionUp", nullptr, nullptr, 0) // Scrolls perks page list
	// root->Invoke("root.Menu_mc.CurrentPage.PerksTab_mc.List_mc.moveSelectionDown", nullptr, nullptr, 0) // Scrolls perks page list
	// root->Invoke("root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc.moveSelectionUp", nullptr, nullptr, 0) // Scrolls SPECIAL page list
	// root->Invoke("root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc.moveSelectionDown", nullptr, nullptr, 0) // Scrolls SPECIAL page list
	//
	// MAP TAB CONTROLS
	//
	// GFxValue akArgs[2];       // Move Map
	// akArgs[0]   <- X Value
	// akArgs[1]   <- Y Value
	// root->Invoke("root.Menu_mc.CurrentPage.WorldMapHolder_mc.PanMap", nullptr, akArgs, 2)
	//
	// INFORMATION
	//
	// 	if (root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage")) {   // Returns Current Page Number (0 = STAT, 1 = INV, 2 = DATA, 3 = MAP, 4 = RADIO)
	//	   X = PBCurrentPage.GetUInt();
	//  }
	// ===============================================================================================================================================*/

	void Skeleton::pipboyManagement() {  //Manages all aspects of Virtual Pipboy usage outside of turning the device / radio / torch on or off. Additionally swaps left hand controls to the right hand.  
		bool isInPA = detectInPowerArmor();
		if (!isInPA) {
			static BSFixedString orbNames[7] = { "TabChangeUpOrb", "TabChangeDownOrb", "PageChangeUpOrb", "PageChangeDownOrb", "ScrollItemsUpOrb", "ScrollItemsDownOrb", "SelectItemsOrb" };
			BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;
			bool helmetHeadLamp = armorHasHeadLamp();
			bool lightOn = Offsets::isPipboyLightOn(*g_player);
			bool RadioOn = Offsets::isPlayerRadioEnabled();
			Matrix44 rot;
			float radFreq = (Offsets::getPlayerRadioFreq() - 23);
			NiTransform Needle;
			static BSFixedString pwrButtonOn("PowerButton_mesh:2");
			static BSFixedString pwrButtonOff("PowerButton_mesh:off");
			static BSFixedString lhtButtonOn("LightButton_mesh:2");
			static BSFixedString lhtButtonOff("LightButton_mesh:off");
			static BSFixedString radButtonOn("RadioOn");
			static BSFixedString radButtonOff("RadioOff");
			static BSFixedString radioNeedle("RadioNeedle_mesh");
			static BSFixedString NewModeKnob("ModeKnobDuplicate");
			static BSFixedString OriginalModeKnob("ModeKnob02");
			NiAVObject* pipbone = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&pwrButtonOn) : leftArm.forearm3->GetObjectByName(&pwrButtonOn);
			NiAVObject* pipbone2 = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&pwrButtonOff) : leftArm.forearm3->GetObjectByName(&pwrButtonOff);
			NiAVObject* pipbone3 = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&lhtButtonOn) : leftArm.forearm3->GetObjectByName(&lhtButtonOn);
			NiAVObject* pipbone4 = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&lhtButtonOff) : leftArm.forearm3->GetObjectByName(&lhtButtonOff);
			NiAVObject* pipbone5 = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&radButtonOn) : leftArm.forearm3->GetObjectByName(&radButtonOn);
			NiAVObject* pipbone6 = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&radButtonOff) : leftArm.forearm3->GetObjectByName(&radButtonOff);
			NiAVObject* pipbone7 = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&radioNeedle) : leftArm.forearm3->GetObjectByName(&radioNeedle);
			NiAVObject* pipbone8 = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&NewModeKnob) : leftArm.forearm3->GetObjectByName(&NewModeKnob);
			NiAVObject* pipbone9 = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&OriginalModeKnob) : leftArm.forearm3->GetObjectByName(&OriginalModeKnob);
			if (!pipbone || !pipbone2 || !pipbone3 || !pipbone4 || !pipbone5 || !pipbone6 || !pipbone7 || !pipbone8) {
				return;
			}
			if (isLookingAtPipBoy()) {
				BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;
				NiPoint3 finger;
				NiAVObject* pipboy = nullptr;
				NiAVObject* pipboyTrans = nullptr;
				c_leftHandedPipBoy ? finger = rt->transforms[boneTreeMap["LArm_Finger23"]].world.pos : finger = rt->transforms[boneTreeMap["RArm_Finger23"]].world.pos;
				c_leftHandedPipBoy ? pipboy = getNode("PipboyRoot", rightArm.shoulder->GetAsNiNode()) : pipboy = getNode("PipboyRoot", leftArm.shoulder->GetAsNiNode());
				float distance;
				distance = vec3_len(finger - pipboy->m_worldTransform.pos);
				if ((distance < c_pipboyDetectionRange) && !c_IsOperatingPipboy && !_pipboyStatus) { // Hides Weapon and poses hand for pointing
					c_IsOperatingPipboy = true;
					holsterWeapon();
					setPipboyHandPose();
				}
				if ((distance > c_pipboyDetectionRange) && c_IsOperatingPipboy && !_pipboyStatus) { // Restores Weapon and releases hand pose
					c_IsOperatingPipboy = false;					
					disablePipboyHandPose();
					drawWeapon();
				}
			}
			else if (!isLookingAtPipBoy() && c_IsOperatingPipboy && !_pipboyStatus) { // Catches if you're not looking at the pipboy when your hand moves outside of the control area and restores weapon / releases hand pose							
				disablePipboyHandPose();
				for (int i = 0; i < 7; i++) {  // Remove any stuck helper orbs if Pipboy times out for any reason.
					NiAVObject* orb = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&orbNames[i]) : leftArm.forearm3->GetObjectByName(&orbNames[i]);
					if (orb != nullptr) {
						if (orb->m_localTransform.scale > 0) {
							orb->m_localTransform.scale = 0;
						}
					}
				}
				drawWeapon();
				c_IsOperatingPipboy = false;
			}
			if (LastPipboyPage == 4) {  // fixes broken 'Mode Knob' position when radio tab is selected
				rot.makeTransformMatrix(pipbone8->m_localTransform.rot, NiPoint3(0, 0, 0));
				float rotx;
				float roty;
				float rotz;
				rot.getEulerAngles(&rotx, &roty, &rotz);
				if (rotx < 0.57) {
					Matrix44 kRot;
					kRot.setEulerAngles(-0.05, degrees_to_rads(0), degrees_to_rads(0));
					pipbone8->m_localTransform.rot = kRot.multiply43Right(pipbone8->m_localTransform.rot);
				}
			}
			else { // restores control of the 'Mode Knob' to the Pipboy behaviour file
				pipbone8->m_localTransform.rot = pipbone9->m_localTransform.rot;
			}
			// Controls Pipboy power light glow (on or off depending on Pipboy state)
			_pipboyStatus ? pipbone->flags &= 0xfffffffffffffffe : pipbone2->flags &= 0xfffffffffffffffe;
			_pipboyStatus ? pipbone->m_localTransform.scale = 1 : pipbone2->m_localTransform.scale = 1;
			_pipboyStatus ? pipbone2->flags |= 0x1 : pipbone->flags |= 0x1;
			_pipboyStatus ? pipbone2->m_localTransform.scale = 0 : pipbone->m_localTransform.scale = 0;
			// Control switching between hand and head based Pipboy light 
			if (lightOn && !helmetHeadLamp) {
				NiPoint3 hand;
				NiNode* head = getNode("Head", (*g_player)->GetActorRootNode(false)->GetAsNiNode());
				if (!head) {
					return;
				}
				c_leftHandedPipBoy ? hand = rt->transforms[boneTreeMap["RArm_Hand"]].world.pos : hand = rt->transforms[boneTreeMap["LArm_Hand"]].world.pos;
				float distance = vec3_len(hand - head->m_worldTransform.pos);
				if (distance < 15.0) {
					uint64_t _PipboyHand = (c_leftHandedPipBoy ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed);
					const auto SwitchLightButton = _PipboyHand & vr::ButtonMaskFromId((vr::EVRButtonId)c_SwitchTorchButton);
					if (vrhook != nullptr && _SwitchLightHaptics) {
						c_leftHandedPipBoy ? vrhook->StartHaptics(2, 0.1, 0.1) : vrhook->StartHaptics(1, 0.1, 0.1);
						_isSaveButtonPressed = true;
					}
					// Control switching between hand and head based Pipboy light
					if (SwitchLightButton && !_SwithLightButtonSticky) {
						_SwithLightButtonSticky = true;
						_SwitchLightHaptics = false;
						if (vrhook != nullptr) {
							c_leftHandedPipBoy ? vrhook->StartHaptics(2, 0.05, 0.3) : vrhook->StartHaptics(1, 0.05, 0.3);
							_isSaveButtonPressed = true;
						}
						NiNode* LGHT_ATTACH = c_leftHandedPipBoy ? getNode("RArm_Hand", rightArm.shoulder->GetAsNiNode()) : getNode("LArm_Hand", leftArm.shoulder->GetAsNiNode());
						NiNode* lght = c_IsPipBoyTorchOnArm ? get1stChildNode("HeadLightParent", LGHT_ATTACH) : _playerNodes->HeadLightParentNode->GetAsNiNode();
						if (lght) {
							BSFixedString parentnode = c_IsPipBoyTorchOnArm ? lght->m_parent->m_name : _playerNodes->HeadLightParentNode->m_parent->m_name;
							Matrix44 LightRot;
							int Rotz = c_IsPipBoyTorchOnArm ? -90 : 90;
							LightRot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0), degrees_to_rads(Rotz));
							lght->m_localTransform.rot = LightRot.multiply43Right(lght->m_localTransform.rot);
							int PosY = c_IsPipBoyTorchOnArm ? 0 : 4;
							lght->m_localTransform.pos.y = PosY;
							c_IsPipBoyTorchOnArm ? lght->m_parent->RemoveChild(lght) : _playerNodes->HeadLightParentNode->m_parent->RemoveChild(lght);
							c_IsPipBoyTorchOnArm ? _playerNodes->HmdNode->AttachChild(lght, true) : LGHT_ATTACH->AttachChild(lght, true);
							c_IsPipBoyTorchOnArm ? c_IsPipBoyTorchOnArm = false : c_IsPipBoyTorchOnArm = true;
						}
						CSimpleIniA ini;
						SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\FRIK.ini");
						rc = ini.SetBoolValue("Fallout4VRBody", "PipBoyTorchOnArm", c_IsPipBoyTorchOnArm);
						rc = ini.SaveFile(".\\Data\\F4SE\\plugins\\FRIK.ini");
					}
					if (!SwitchLightButton) {
						_SwithLightButtonSticky = false;
					}

				}
				else if (distance > 10) {
					_SwitchLightHaptics = true;
					_SwithLightButtonSticky = false;
				}
			}
			//Attach light to hand 
			if (c_IsPipBoyTorchOnArm) {
				NiNode* LGHT_ATTACH = c_leftHandedPipBoy ? getNode("RArm_Hand", rightArm.shoulder->GetAsNiNode()) : getNode("LArm_Hand", leftArm.shoulder->GetAsNiNode());
				if (LGHT_ATTACH) {
					if (lightOn && !helmetHeadLamp) {
						NiNode* lght = _playerNodes->HeadLightParentNode->GetAsNiNode();
						BSFixedString parentnode = _playerNodes->HeadLightParentNode->m_parent->m_name;
						if (parentnode == "HMDNode") {
							_playerNodes->HeadLightParentNode->m_parent->RemoveChild(lght);
							Matrix44 LightRot;
							LightRot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0), degrees_to_rads(90));
							lght->m_localTransform.rot = LightRot.multiply43Right(lght->m_localTransform.rot);
							lght->m_localTransform.pos.y = 4;
							LGHT_ATTACH->AttachChild(lght, true);
						}
					}
					//Restore HeadLight to correct node when light is powered off (to avoid any crashes)
					else if (!lightOn || helmetHeadLamp) {
						NiNode* lght = get1stChildNode("HeadLightParent", LGHT_ATTACH);
						if (lght) {
							BSFixedString parentnode = lght->m_parent->m_name;
							if (parentnode != "HMDNode") {
								Matrix44 LightRot;
								LightRot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0), degrees_to_rads(-90));
								lght->m_localTransform.rot = LightRot.multiply43Right(lght->m_localTransform.rot);
								lght->m_localTransform.pos.y = 0;
								lght->m_parent->RemoveChild(lght);
								_playerNodes->HmdNode->AttachChild(lght, true);
							}
						}
					}
				}
			}
			// Controls Radio / Light on & off indicators
			lightOn ? pipbone3->flags &= 0xfffffffffffffffe : pipbone4->flags &= 0xfffffffffffffffe;
			lightOn ? pipbone3->m_localTransform.scale = 1 : pipbone4->m_localTransform.scale = 1;
			lightOn ? pipbone4->flags |= 0x1 : pipbone3->flags |= 0x1;
			lightOn ? pipbone4->m_localTransform.scale = 0 : pipbone3->m_localTransform.scale = 0;
			RadioOn ? pipbone5->flags &= 0xfffffffffffffffe : pipbone6->flags &= 0xfffffffffffffffe;
			RadioOn ? pipbone5->m_localTransform.scale = 1 : pipbone6->m_localTransform.scale = 1;
			RadioOn ? pipbone6->flags |= 0x1 : pipbone5->flags |= 0x1;
			RadioOn ? pipbone6->m_localTransform.scale = 0 : pipbone5->m_localTransform.scale = 0;
			// Controls Radio Needle Position.
			if (RadioOn && (radFreq != lastRadioFreq)) {
				float x = -1 * (radFreq - lastRadioFreq);
				rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(x), degrees_to_rads(0));
				pipbone7->m_localTransform.rot = rot.multiply43Right(pipbone7->m_localTransform.rot);
				lastRadioFreq = radFreq;
			}
			else if (!RadioOn && lastRadioFreq > 0) {
				float x = lastRadioFreq;
				rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(x), degrees_to_rads(0));
				pipbone7->m_localTransform.rot = rot.multiply43Right(pipbone7->m_localTransform.rot);
				lastRadioFreq = 0.0;
			}
			// Scaleform code for managing Pipboy menu controls (Virtual and Physical)
			if (_pipboyStatus) {
				BSFixedString pipboyMenu("PipboyMenu");
				IMenu* menu = (*g_ui)->GetMenu(pipboyMenu);
				if (menu != nullptr) {
					GFxMovieRoot* root = menu->movie->movieRoot;
					if (root != nullptr) {
						GFxValue IsProjected;
						GFxValue PBCurrentPage;
						if (root->GetVariable(&IsProjected, "root.Menu_mc.projectedBorder_mc.visible")) { //check if Pipboy is projected and disable right stick rotation if it isn't
							bool UIProjected = IsProjected.GetBool();
							if (!UIProjected && c_switchUIControltoPrimary) {
								SetINIFloat("fDirectionalDeadzone:Controls", 1.0);  //prevents player movement controls so we can switch controls to the right stick (or left if the Pipboy is on the right arm)
							}
						}
						if (root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage")) {  // Get Current Pipboy Tab and store it.
							if (PBCurrentPage.GetType() != GFxValue::kType_Undefined) {
								LastPipboyPage = PBCurrentPage.GetUInt();
							}
						}
						static BSFixedString boneNames[7] = { "TabChangeUp", "TabChangeDown", "PageChangeUp", "PageChangeDown", "ScrollItemsUp", "ScrollItemsDown", "SelectButton02" };
						static BSFixedString transNames[7] = { "TabChangeUpTrans", "TabChangeDownTrans", "PageChangeUpTrans", "PageChangeDownTrans", "ScrollItemsUpTrans", "ScrollItemsDownTrans", "SelectButtonTrans" };
						float boneDistance[7] = { 2.0, 2.0, 2.0, 2.0, 1.5, 1.5, 2.0 };
						float transDistance[7] = { 0.6, 0.6, 0.6, 0.6, 0.1, 0.1, 0.4 };
						float maxDistance[7] = { 1.2,  1.2,  1.2,  1.2, 1.2,  1.2, 0.6 };
						NiPoint3 finger;
						NiAVObject* pipboy = nullptr;
						NiAVObject* pipboyTrans = nullptr;
						// Virtual Controls Code Starts here: 
						c_leftHandedPipBoy ? finger = rt->transforms[boneTreeMap["LArm_Finger23"]].world.pos : finger = rt->transforms[boneTreeMap["RArm_Finger23"]].world.pos;
						for (int i = 0; i < 7; i++) {
							NiAVObject* bone = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&boneNames[i]) : leftArm.forearm3->GetObjectByName(&boneNames[i]);
							NiNode* trans = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&transNames[i])->GetAsNiNode() : leftArm.forearm3->GetObjectByName(&transNames[i])->GetAsNiNode();
							if (bone && trans) {
								float distance = vec3_len(finger - bone->m_worldTransform.pos);
								if (distance > boneDistance[i]) {
									trans->m_localTransform.pos.z = 0.0;
									_PBControlsSticky[i] = false;
									NiAVObject* orb = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&orbNames[i]) : leftArm.forearm3->GetObjectByName(&orbNames[i]); //Hide helper Orbs when not near a control surface
									if (orb != nullptr) {
										if (orb->m_localTransform.scale > 0) {
											orb->m_localTransform.scale = 0;
										}
									}
								}
								else if (distance <= boneDistance[i]) {
									float fz = (boneDistance[i] - distance);
									NiAVObject* orb = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&orbNames[i]) : leftArm.forearm3->GetObjectByName(&orbNames[i]); //Show helper Orbs when not near a control surface
									if (orb != nullptr) {
										if (orb->m_localTransform.scale < 1) {
											orb->m_localTransform.scale = 1;
										}
									}
									if (fz > 0.0 && fz < maxDistance[i]) {
										trans->m_localTransform.pos.z = (fz);
										if (i == 4) {  // Move Scroll Knob Anti-Clockwise when near control surface
											static BSFixedString KnobNode = "ScrollItemsKnobRot";
											NiAVObject* ScrollKnob = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&KnobNode) : leftArm.forearm3->GetObjectByName(&KnobNode);
											Matrix44 rot;
											rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(fz), degrees_to_rads(0));
											ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
										}
										if (i == 5) { // Move Scroll Knob Clockwise when near control surface
											float roty = (fz * -1);
											static BSFixedString KnobNode = "ScrollItemsKnobRot";
											NiAVObject* ScrollKnob = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&KnobNode) : leftArm.forearm3->GetObjectByName(&KnobNode);
											Matrix44 rot;
											rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(roty), degrees_to_rads(0));
											ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
										}
									}
									if ((trans->m_localTransform.pos.z > transDistance[i]) && !_PBControlsSticky[i]) {
										if (vrhook != nullptr) {
											_PBControlsSticky[i] = true;
											c_leftHandedMode ? vrhook->StartHaptics(1, 0.05, 0.3) : vrhook->StartHaptics(2, 0.05, 0.3);
											if (i == 0) {
												root->Invoke("root.Menu_mc.gotoPrevPage", nullptr, nullptr, 0); // Previous Page											
											}
											if (i == 1) {
												root->Invoke("root.Menu_mc.gotoNextPage", nullptr, nullptr, 0);  // Next Page				
											}
											if (i == 2) {
												root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0); // Previous Sub Page
											}
											if (i == 3) {
												root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0); // Next Sub Page
											}
											if (i == 4) {
												std::thread t1(SimulateExtendedButtonPress, VK_UP); // Scroll up list
												t1.detach();
											}
											if (i == 5) {
												std::thread t1(SimulateExtendedButtonPress, VK_DOWN); // Scroll down list
												t1.detach();
											}
											if (i == 6) {
												std::thread t1(SimulateExtendedButtonPress, VK_RETURN); // Select Current Item
												t1.detach();
											}

										}
									}
								}
							}
						}
						// Mirror Left Stick Controls on Right Stick.
						if (!_isPBConfigModeActive && c_switchUIControltoPrimary) {
							BSFixedString selectnodename = "SelectRotate";
							NiNode* trans = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&selectnodename)->GetAsNiNode() : leftArm.forearm3->GetObjectByName(&selectnodename)->GetAsNiNode();
							vr::VRControllerAxis_t doinantHandStick = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0]);
							vr::VRControllerAxis_t doinantTrigger = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[1] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[1]);
							vr::VRControllerAxis_t secondaryTrigger = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[1] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[1]);
							uint64_t dominantHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed);
							const auto UISelectButton = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)33); // Right Trigger
							const auto UIAltSelectButton = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)32); // Right Touchpad
							GFxValue GetSWFVar;
							bool isPBMessageBoxVisible = false;
							// Move Pipboy trigger mesh with controller trigger position.
							if (trans != nullptr) {
								if (doinantTrigger.x > 0.00 && secondaryTrigger.x == 0.0) {
									trans->m_localTransform.pos.z = (doinantTrigger.x / 3) * -1;
								}
								else if (secondaryTrigger.x > 0.00 && doinantTrigger.x == 0.0) {
									trans->m_localTransform.pos.z = (secondaryTrigger.x / 3) * -1;
								}
								else {
									trans->m_localTransform.pos.z = 0.00;
								}
							}
							if (root->GetVariable(&GetSWFVar, "root.Menu_mc.CurrentPage.MessageHolder_mc.visible")) {
								isPBMessageBoxVisible = GetSWFVar.GetBool();
							}
							if (LastPipboyPage != 3 || isPBMessageBoxVisible) {
								static BSFixedString KnobNode = "ScrollItemsKnobRot";
								NiAVObject* ScrollKnob = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&KnobNode) : leftArm.forearm3->GetObjectByName(&KnobNode);
								Matrix44 rot;
								if (doinantHandStick.y > 0.85) {
									rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0.4), degrees_to_rads(0));
									ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
								}
								if (doinantHandStick.y < -0.85) {
									rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(-0.4), degrees_to_rads(0));
									ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
								}
							}
							if (LastPipboyPage == 3 && !isPBMessageBoxVisible) { // Map Tab
								GFxValue akArgs[2];
								akArgs[0].SetNumber(doinantHandStick.x * -1);
								akArgs[1].SetNumber(doinantHandStick.y);
								if (root->Invoke("root.Menu_mc.CurrentPage.WorldMapHolder_mc.PanMap", nullptr, akArgs, 2)) {}	// Move Map	
								if (root->Invoke("root.Menu_mc.CurrentPage.LocalMapHolder_mc.PanMap", nullptr, akArgs, 2)) {}
							}
							else {
								if (doinantHandStick.y > 0.85) {
									if (!_controlSleepStickyY) {
										_controlSleepStickyY = true;
										std::thread t1(SimulateExtendedButtonPress, VK_UP); // Scroll up list
										t1.detach();
										std::thread t2(RightStickYSleep, 155);
										t2.detach();
									}
								}
								if (doinantHandStick.y < -0.85) {
									if (!_controlSleepStickyY) {
										_controlSleepStickyY = true;
										std::thread t1(SimulateExtendedButtonPress, VK_DOWN); // Scroll down list
										t1.detach();
										std::thread t2(RightStickYSleep, 155);
										t2.detach();
									}
								}
								if (doinantHandStick.x < -0.85) {
									if (!_controlSleepStickyX) {
										_controlSleepStickyX = true;
										root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0); // Next Sub Page
										std::thread t3(RightStickXSleep, 170);
										t3.detach();
									}
								}
								if (doinantHandStick.x > 0.85) {
									if (!_controlSleepStickyX) {
										_controlSleepStickyX = true;
										root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0); // Previous Sub Page
										std::thread t3(RightStickXSleep, 170);
										t3.detach();
									}
								}
							}
							if (UISelectButton && !_UISelectSticky) {
								_UISelectSticky = true;
								std::thread t1(SimulateExtendedButtonPress, VK_RETURN); // Select Current Item
								t1.detach();
							}
							else if (!UISelectButton) {
								_UISelectSticky = false;
							}
							if (UIAltSelectButton && !_UIAltSelectSticky) {
								_UIAltSelectSticky = true;
								if (root->Invoke("root.Menu_mc.CurrentPage.onMessageButtonPress()", nullptr, nullptr, 0)) {}
							}
							else if (!UIAltSelectButton) {
								_UIAltSelectSticky = false;
							}
						}
						else if (!_isPBConfigModeActive && !c_switchUIControltoPrimary) {
							//still move Pipboy trigger mesh even if controls havent been swapped.
							vr::VRControllerAxis_t secondaryTrigger = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[1] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[1]);
							vr::VRControllerAxis_t offHandStick = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0]);
							BSFixedString selectnodename = "SelectRotate";
							NiNode* trans = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&selectnodename)->GetAsNiNode() : leftArm.forearm3->GetObjectByName(&selectnodename)->GetAsNiNode();
							if (trans != nullptr) {
								if (secondaryTrigger.x > 0.00) {
									trans->m_localTransform.pos.z = (secondaryTrigger.x / 3) * -1;
								}
								else {
									trans->m_localTransform.pos.z = 0.00;
								}
							}
							//still move Pipboy scroll knob even if controls havent been swapped.
							bool isPBMessageBoxVisible = false;
							GFxValue GetSWFVar;
							if (root->GetVariable(&GetSWFVar, "root.Menu_mc.CurrentPage.MessageHolder_mc.visible")) {
								isPBMessageBoxVisible = GetSWFVar.GetBool();
							}
							if (LastPipboyPage != 3 || isPBMessageBoxVisible) {
								static BSFixedString KnobNode = "ScrollItemsKnobRot";
								NiAVObject* ScrollKnob = c_leftHandedPipBoy ? rightArm.forearm3->GetObjectByName(&KnobNode) : leftArm.forearm3->GetObjectByName(&KnobNode);
								Matrix44 rot;
								if (offHandStick.x > 0.85) {
									rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0.4), degrees_to_rads(0));
									ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
								}
								if (offHandStick.x < -0.85) {
									rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(-0.4), degrees_to_rads(0));
									ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
								}
							}
						}
					}
				}
			}
		}
		else if (isInPA) {
			lastRadioFreq = 0.0;  // Ensures Radio needle doesn't get messed up when entering and then exiting Power Armor.
			// Continue to update Pipboy page info when in Power Armor.
			BSFixedString pipboyMenu("PipboyMenu");
			IMenu* menu = (*g_ui)->GetMenu(pipboyMenu);
			if (menu != nullptr) {
				GFxMovieRoot* root = menu->movie->movieRoot;
				if (root != nullptr) {
					GFxValue PBCurrentPage;
					if (root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage")) {
						if (PBCurrentPage.GetType() != GFxValue::kType_Undefined) {
							LastPipboyPage = PBCurrentPage.GetUInt();
						}
					}
				}
			}
		}
	}

	void Skeleton::exitPBConfig() {  // Exit Pipboy Config Mode / remove UI.
		if (_isPBConfigModeActive) {
			for (int i = 0; i <= 9; i++) {
				_PBTouchbuttons[i] = false;
			}
			static BSFixedString hudname("PBCONFIGHUD");
			NiAVObject* PBConfigUI = _playerNodes->primaryUIAttachNode->GetObjectByName(&hudname);
			if (PBConfigUI) {
				PBConfigUI->flags |= 0x1;
				PBConfigUI->m_localTransform.scale = 0;
				PBConfigUI->m_parent->RemoveChild(PBConfigUI);
			}
			disableConfigModePose();
			_isPBConfigModeActive = false;
		}
	}

    void Skeleton::configModeExit() { // Exit Main FRIK Config Mode
        c_CalibrationModeUIActive = false;
        if (NiNode* c_MBox = getNode("messageBoxMenuWider", _playerNodes->playerworldnode)) {
            c_MBox->flags &= ~0x1;
            c_MBox->m_localTransform.scale = 1.0;
        }
        if (c_CalibrateModeActive) {
            std::fill(std::begin(_MCTouchbuttons), std::end(_MCTouchbuttons), false);
            static BSFixedString hudname("MCCONFIGHUD");
            if (NiAVObject* MCConfigUI = _playerNodes->primaryUIAttachNode->GetObjectByName(&hudname)) {
                MCConfigUI->flags |= 0x1;
                MCConfigUI->m_localTransform.scale = 0;
                MCConfigUI->m_parent->RemoveChild(MCConfigUI);
            }
            disableConfigModePose();
            SetINIFloat("fDirectionalDeadzone:Controls", c_repositionMasterMode ? 1.0 : c_DirectionalDeadzone);
            c_CalibrateModeActive = false;
        }
    }

	void Skeleton::mainConfigurationMode() {
		if (c_CalibrateModeActive) {
			float rAxisOffsetY;
			char* meshName[10] = { "MC-MainTitleTrans", "MC-Tile01Trans", "MC-Tile02Trans", "MC-Tile03Trans", "MC-Tile04Trans", "MC-Tile05Trans", "MC-Tile06Trans", "MC-Tile07Trans", "MC-Tile08Trans", "MC-Tile09Trans" };
			char* meshName2[10] = { "MC-MainTitle", "MC-Tile01", "MC-Tile02", "MC-Tile03", "MC-Tile04", "MC-Tile05", "MC-Tile06", "MC-Tile07", "MC-Tile08", "MC-Tile09" };
			char* meshName3[10] = { "","","","","","","", "MC-Tile07On", "MC-Tile08On", "MC-Tile09On" };
			char* meshName4[4] = { "MC-ModeA", "MC-ModeB", "MC-ModeC", "MC-ModeD" };
			if (!c_CalibrationModeUIActive) { // Create Config UI
				ShowMessagebox("FRIK Config Mode");
				NiNode* c_MBox = getNode("messageBoxMenuWider", _playerNodes->playerworldnode);
				if (c_MBox) {
					c_MBox->flags |= 0x1;
					c_MBox->m_localTransform.scale = 0;
				}
				BSFixedString menuName("FavoritesMenu"); // close favorites menu if open.
				if ((*g_ui)->IsMenuOpen(menuName)) {
					if ((*g_ui)->IsMenuRegistered(menuName)) {
						CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
					}
					if (vrhook != nullptr) {
						c_leftHandedMode ? vrhook->StartHaptics(1, 0.55, 0.5) : vrhook->StartHaptics(2, 0.55, 0.5);
					}
				}
				NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigHUD.nif");
				NiCloneProcess proc;
				proc.unk18 = Offsets::cloneAddr1;
				proc.unk48 = Offsets::cloneAddr2;
				NiNode* HUD = Offsets::cloneNode(retNode, &proc);
				HUD->m_name = BSFixedString("MCCONFIGHUD");
				NiNode* UIATTACH = getNode("world_primaryWand.nif", _playerNodes->primaryUIAttachNode);
				UIATTACH->AttachChild((NiAVObject*)HUD, true);
				char* MainHud[10] = { "Data/Meshes/FRIK/UI-MainTitle.nif", "Data/Meshes/FRIK/UI-Tile01.nif", "Data/Meshes/FRIK/UI-Tile02.nif", "Data/Meshes/FRIK/UI-Tile03.nif", "Data/Meshes/FRIK/UI-Tile04.nif", "Data/Meshes/FRIK/UI-Tile05.nif", "Data/Meshes/FRIK/UI-Tile06.nif", "Data/Meshes/FRIK/UI-Tile07.nif", "Data/Meshes/FRIK/UI-Tile08.nif", "Data/Meshes/FRIK/UI-Tile09.nif" };
				char* MainHud2[10] = { "Data/Meshes/FRIK/MC-MainTitle.nif", "Data/Meshes/FRIK/MC-Tile01.nif", "Data/Meshes/FRIK/MC-Tile02.nif", "Data/Meshes/FRIK/MC-Tile03.nif", "Data/Meshes/FRIK/MC-Tile04.nif", "Data/Meshes/FRIK/MC-Tile05.nif", "Data/Meshes/FRIK/MC-Tile06.nif", "Data/Meshes/FRIK/MC-Tile07.nif", "Data/Meshes/FRIK/MC-Tile08.nif", "Data/Meshes/FRIK/MC-Tile09.nif" };
				char* MainHud3[4] = { "Data/Meshes/FRIK/MC-Tile09a.nif", "Data/Meshes/FRIK/MC-Tile09b.nif", "Data/Meshes/FRIK/MC-Tile09c.nif", "Data/Meshes/FRIK/MC-Tile09d.nif" };
				for (int i = 0; i <= 9; i++) {
					NiNode* retNode = loadNifFromFile(MainHud[i]);
					NiCloneProcess proc;
					proc.unk18 = Offsets::cloneAddr1;
					proc.unk48 = Offsets::cloneAddr2;
					NiNode* UI = Offsets::cloneNode(retNode, &proc);
					UI->m_name = BSFixedString(meshName2[i]);
					HUD->AttachChild((NiAVObject*)UI, true);
					retNode = loadNifFromFile(MainHud2[i]);
					NiNode* UI2 = Offsets::cloneNode(retNode, &proc);
					UI2->m_name = BSFixedString(meshName[i]);
					UI->AttachChild((NiAVObject*)UI2, true);
					if (i == 7 || i == 8) {
						retNode = loadNifFromFile("Data/Meshes/FRIK/UI-StickyMarker.nif");
						NiNode* UI3 = Offsets::cloneNode(retNode, &proc);
						UI3->m_name = BSFixedString(meshName3[i]);
						UI2->AttachChild((NiAVObject*)UI3, true);
					}
					if (i == 9) {
						for (int x = 0; x < 4; x++) {
							retNode = loadNifFromFile(MainHud3[x]);
							NiNode* UI3 = Offsets::cloneNode(retNode, &proc);
							UI3->m_name = BSFixedString(meshName4[x]);
							UI2->AttachChild((NiAVObject*)UI3, true);
						}
					}
				}
				setConfigModeHandPose();
				c_CalibrationModeUIActive = true;
				c_armLengthbkup = c_armLength;
				c_powerArmor_upbkup = c_powerArmor_up;
				c_playerOffset_upbkup = c_playerOffset_up;
				c_RootOffsetbkup = c_RootOffset;
				c_PARootOffsetbkup = c_PARootOffset;
				c_fVrScalebkup = c_fVrScale;
				c_playerOffset_forwardbkup = c_playerOffset_forward;
				c_powerArmor_forwardbkup = c_powerArmor_forward;
				c_cameraHeightbkup = c_cameraHeight;
				c_PACameraHeightbkup = c_PACameraHeight;
			}
			else {
				NiNode* UIElement = nullptr;
				// Dampen Hands
				UIElement = getNode("MC-Tile07On", _playerNodes->primaryUIAttachNode);
				c_dampenHands ? UIElement->m_localTransform.scale = 1 : UIElement->m_localTransform.scale = 0;
				// Weapon Reposition Mode
				UIElement = getNode("MC-Tile08On", _playerNodes->primaryUIAttachNode);
				c_repositionMasterMode ? UIElement->m_localTransform.scale = 1 : UIElement->m_localTransform.scale = 0;
				// Grip Mode
				if (!c_enableGripButtonToGrap && !c_onePressGripButton && !c_enableGripButtonToLetGo) { // Standard Sticky Grip on / off
					for (int i = 0; i < 4; i++) {
						if (i == 0) {
							UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
							UIElement->m_localTransform.scale = 1;
						}
						else {
							UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
							UIElement->m_localTransform.scale = 0;
						}
					}
				}
				else if (!c_enableGripButtonToGrap && !c_onePressGripButton && c_enableGripButtonToLetGo) { // Sticky Grip with button to release
					for (int i = 0; i < 4; i++) {
						if (i == 1) {
							UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
							UIElement->m_localTransform.scale = 1;
						}
						else {
							UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
							UIElement->m_localTransform.scale = 0;
						}
					}
				}
				else if (c_enableGripButtonToGrap && c_onePressGripButton && !c_enableGripButtonToLetGo) { // Button held to Grip
					for (int i = 0; i < 4; i++) {
						if (i == 2) {
							UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
							UIElement->m_localTransform.scale = 1;
						}
						else {
							UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
							UIElement->m_localTransform.scale = 0;
						}
					}
				}
				else if (c_enableGripButtonToGrap && !c_onePressGripButton && c_enableGripButtonToLetGo) { // button press to toggle Grip on or off
					for (int i = 0; i < 4; i++) {
						if (i == 3) {
							UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
							UIElement->m_localTransform.scale = 1;
						}
						else {
							UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
							UIElement->m_localTransform.scale = 0;
						}
					}
				}
				else {  //Not exepected - show no mode lable until button pressed 
					for (int i = 0; i < 4; i++) {
						UIElement = getNode(meshName4[i], _playerNodes->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 0;
					}
				}
				BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;
				NiPoint3 finger;
				c_leftHandedMode ? finger = rt->transforms[boneTreeMap["RArm_Finger23"]].world.pos : finger = rt->transforms[boneTreeMap["LArm_Finger23"]].world.pos;
				for (int i = 1; i <= 9; i++) {
					BSFixedString TouchName = meshName2[i];
					BSFixedString TransName = meshName[i];
					NiNode* TouchMesh = (NiNode*)_playerNodes->primaryUIAttachNode->GetObjectByName(&TouchName);
					NiNode* TransMesh = (NiNode*)_playerNodes->primaryUIAttachNode->GetObjectByName(&TransName);
					if (TouchMesh && TransMesh) {
						float distance = vec3_len(finger - TouchMesh->m_worldTransform.pos);
						if (distance > 2.0) {
							TransMesh->m_localTransform.pos.y = 0.0;
							if (i == 7 || i == 8 || i == 9) {
								_MCTouchbuttons[i] = false;
							}
						}
						else if (distance <= 2.0) {
							float fz = (2.0 - distance);
							if (fz > 0.0 && fz < 1.2) {
								TransMesh->m_localTransform.pos.y = (fz);
							}
							if ((TransMesh->m_localTransform.pos.y > 1.0) && !_MCTouchbuttons[i]) {
								if (vrhook != nullptr) {
									//_PBConfigSticky = true;
									c_leftHandedMode ? vrhook->StartHaptics(2, 0.05, 0.3) : vrhook->StartHaptics(1, 0.05, 0.3);
									for (int i = 1; i <= 7; i++) {
										_MCTouchbuttons[i] = false;
									}
									BSFixedString bname = "MCCONFIGMarker";
									NiNode* UIMarker = (NiNode*)_playerNodes->primaryUIAttachNode->GetObjectByName(&bname);
									if (UIMarker) {
										UIMarker->m_parent->RemoveChild(UIMarker);
									}
									if (i < 7) {
										NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigMarker.nif");
										NiCloneProcess proc;
										proc.unk18 = Offsets::cloneAddr1;
										proc.unk48 = Offsets::cloneAddr2;
										NiNode* UI = Offsets::cloneNode(retNode, &proc);
										UI->m_name = BSFixedString("MCCONFIGMarker");
										TouchMesh->AttachChild((NiAVObject*)UI, true);
									}
									_MCTouchbuttons[i] = true;
								}
							}
						}
					}
				}
				vr::VRControllerAxis_t doinantHandStick = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0]);
				uint64_t dominantHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed);
				uint64_t offHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed);
				bool CamZButtonPressed = _MCTouchbuttons[1];
				bool CamYButtonPressed = _MCTouchbuttons[2];
				bool ScaleButtonPressed = _MCTouchbuttons[3];
				bool BodyZButtonPressed = _MCTouchbuttons[4];
				bool BodyPoseButtonPressed = _MCTouchbuttons[5];
				bool ArmsButtonPressed = _MCTouchbuttons[6];
				bool HandsButtonPressed = _MCTouchbuttons[7];
				bool WeaponButtonPressed = _MCTouchbuttons[8];
				bool GripButtonPressed = _MCTouchbuttons[9];
				bool isInPA = detectInPowerArmor();
				if (HandsButtonPressed && !_isHandsButtonPressed) {
					_isHandsButtonPressed = true;
					c_dampenHands = !c_dampenHands;
				}
				else if (!HandsButtonPressed) {
					_isHandsButtonPressed = false;
				}
				if (WeaponButtonPressed && !_isWeaponButtonPressed) {
					_isWeaponButtonPressed = true;
					c_repositionMasterMode = !c_repositionMasterMode;
				}
				else if (!WeaponButtonPressed) {
					_isWeaponButtonPressed = false;
				}
				if (GripButtonPressed && !_isGripButtonPressed) {
					_isGripButtonPressed = true;
					if (!c_enableGripButtonToGrap && !c_onePressGripButton && !c_enableGripButtonToLetGo) { 
						c_enableGripButtonToGrap = false;
						c_onePressGripButton = false;
						c_enableGripButtonToLetGo = true;
					}
					else if (!c_enableGripButtonToGrap && !c_onePressGripButton && c_enableGripButtonToLetGo) { 
						c_enableGripButtonToGrap = true;
						c_onePressGripButton = true;
						c_enableGripButtonToLetGo = false;
					}
					else if (c_enableGripButtonToGrap && c_onePressGripButton && !c_enableGripButtonToLetGo) { 
						c_enableGripButtonToGrap = true;
						c_onePressGripButton = false;
						c_enableGripButtonToLetGo = true;
					}
					else if (c_enableGripButtonToGrap && !c_onePressGripButton && c_enableGripButtonToLetGo) { 
						c_enableGripButtonToGrap = false;
						c_onePressGripButton = false;
						c_enableGripButtonToLetGo = false;
					}
					else {  //Not exepected - reset to standard sticky grip
						c_enableGripButtonToGrap = false;
						c_onePressGripButton = false;
						c_enableGripButtonToLetGo = false;
					}
				}
				else if (!GripButtonPressed) {
					_isGripButtonPressed = false;
				}
				if ((doinantHandStick.y > 0.10) && (CamZButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					isInPA ? c_PACameraHeight += rAxisOffsetY : c_cameraHeight += rAxisOffsetY;
				}
				if ((doinantHandStick.y < -0.10) && (CamZButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					isInPA ? c_PACameraHeight += rAxisOffsetY : c_cameraHeight += rAxisOffsetY;
				}
				if ((doinantHandStick.y > 0.10) && (CamYButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 10;
					isInPA ? c_powerArmor_forward += rAxisOffsetY : c_playerOffset_forward -= rAxisOffsetY;
				}
				if ((doinantHandStick.y < -0.10) && (CamYButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 10;
					isInPA ? c_powerArmor_forward += rAxisOffsetY : c_playerOffset_forward -= rAxisOffsetY;
				}
				if ((doinantHandStick.y > 0.10) && (ScaleButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					c_fVrScale -= rAxisOffsetY;
					Setting* set = GetINISetting("fVrScale:VR");
					set->SetDouble(c_fVrScale);
				}
				if ((doinantHandStick.y < -0.10) && (ScaleButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					c_fVrScale -= rAxisOffsetY;
					Setting* set = GetINISetting("fVrScale:VR");
					set->SetDouble(c_fVrScale);
				}
				if ((doinantHandStick.y > 0.10) && (BodyZButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					isInPA ? c_PARootOffset += rAxisOffsetY : c_RootOffset += rAxisOffsetY;
				}
				if ((doinantHandStick.y < -0.10) && (BodyZButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					isInPA ? c_PARootOffset += rAxisOffsetY : c_RootOffset += rAxisOffsetY;
				}
				if ((doinantHandStick.y > 0.10) && (BodyPoseButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					isInPA ? c_powerArmor_up +=rAxisOffsetY : c_playerOffset_up += rAxisOffsetY;
				}
				if ((doinantHandStick.y < -0.10) && (BodyPoseButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					isInPA ? c_powerArmor_up += rAxisOffsetY : c_playerOffset_up += rAxisOffsetY;
				}
				if ((doinantHandStick.y > 0.10) && (ArmsButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					c_armLength += rAxisOffsetY;
				}
				if ((doinantHandStick.y < -0.10) && (ArmsButtonPressed)) {
					rAxisOffsetY = doinantHandStick.y / 4;
					c_armLength += rAxisOffsetY;
				}
			}
		}
	}

	void Skeleton::pipboyConfigurationMode() { // The Pipboy Configuration Mode function. 
		if (_pipboyStatus) {
			float rAxisOffsetX;
			vr::VRControllerAxis_t doinantHandStick = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0]);
			uint64_t dominantHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed);
			uint64_t offHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed);
			const auto PBConfigButtonPressed = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)32);
			bool ModelSwapButtonPressed = _PBTouchbuttons[1];
			bool RotateButtonPressed = _PBTouchbuttons[2];
			bool SaveButtonPressed = _PBTouchbuttons[3];
			bool ModelScaleButtonPressed = _PBTouchbuttons[4];
			bool ScaleButtonPressed = _PBTouchbuttons[5];
			bool MoveXButtonPressed = _PBTouchbuttons[6];
			bool MoveYButtonPressed = _PBTouchbuttons[7];
			bool MoveZButtonPressed = _PBTouchbuttons[8];
			bool ExitButtonPressed = _PBTouchbuttons[9];
			char* meshName[10] = { "PB-MainTitleTrans", "PB-Tile07Trans", "PB-Tile03Trans", "PB-Tile08Trans", "PB-Tile02Trans", "PB-Tile01Trans", "PB-Tile04Trans", "PB-Tile05Trans", "PB-Tile06Trans", "PB-Tile09Trans" };
			char* meshName2[10] = { "PB-MainTitle", "PB-Tile07", "PB-Tile03", "PB-Tile08", "PB-Tile02", "PB-Tile01", "PB-Tile04", "PB-Tile05", "PB-Tile06", "PB-Tile09" };
			static BSFixedString wandPipName("PipboyRoot");
			NiAVObject* pbRoot = _playerNodes->SecondaryWandNode->GetObjectByName(&wandPipName);
			if (!pbRoot) {
				return;
			}
			BSFixedString pipName("PipboyBone");
			NiAVObject* _3rdPipboy = nullptr;
			if (!c_leftHandedPipBoy) {
				if (leftArm.forearm3) {
					_3rdPipboy = leftArm.forearm3->GetObjectByName(&pipName);
				}
			}
			else {
				_3rdPipboy = rightArm.forearm3->GetObjectByName(&pipName);

			}
			if (PBConfigButtonPressed && !_isPBConfigModeActive) { // Enter Pipboy Config Mode by holding down favorites button.
				_PBConfigModeEnterCounter += 1;
				if (_PBConfigModeEnterCounter > 200) {
					BSFixedString menuName("FavoritesMenu");
					if ((*g_ui)->IsMenuOpen(menuName)) {
						if ((*g_ui)->IsMenuRegistered(menuName)) {
							CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
						}
					}
					if (vrhook != nullptr) {
						c_leftHandedMode ? vrhook->StartHaptics(1, 0.55, 0.5) : vrhook->StartHaptics(2, 0.55, 0.5);
					}
					NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigHUD.nif");
					NiCloneProcess proc;
					proc.unk18 = Offsets::cloneAddr1;
					proc.unk48 = Offsets::cloneAddr2;
					NiNode* HUD = Offsets::cloneNode(retNode, &proc);
					HUD->m_name = BSFixedString("PBCONFIGHUD");
					NiNode* UIATTACH = getNode("world_primaryWand.nif", _playerNodes->primaryUIAttachNode);
					UIATTACH->AttachChild((NiAVObject*)HUD, true);
					char* MainHud[10] = { "Data/Meshes/FRIK/UI-MainTitle.nif", "Data/Meshes/FRIK/UI-Tile07.nif", "Data/Meshes/FRIK/UI-Tile03.nif", "Data/Meshes/FRIK/UI-Tile08.nif", "Data/Meshes/FRIK/UI-Tile02.nif", "Data/Meshes/FRIK/UI-Tile01.nif", "Data/Meshes/FRIK/UI-Tile04.nif", "Data/Meshes/FRIK/UI-Tile05.nif", "Data/Meshes/FRIK/UI-Tile06.nif", "Data/Meshes/FRIK/UI-Tile09.nif" };
					char* MainHud2[10] = { "Data/Meshes/FRIK/PB-MainTitle.nif", "Data/Meshes/FRIK/PB-Tile07.nif", "Data/Meshes/FRIK/PB-Tile03.nif", "Data/Meshes/FRIK/PB-Tile08.nif", "Data/Meshes/FRIK/PB-Tile02.nif", "Data/Meshes/FRIK/PB-Tile01.nif", "Data/Meshes/FRIK/PB-Tile04.nif", "Data/Meshes/FRIK/PB-Tile05.nif", "Data/Meshes/FRIK/PB-Tile06.nif", "Data/Meshes/FRIK/PB-Tile09.nif" };
					for (int i = 0; i <= 9; i++) {
						NiNode* retNode = loadNifFromFile(MainHud[i]);
						NiCloneProcess proc;
						proc.unk18 = Offsets::cloneAddr1;
						proc.unk48 = Offsets::cloneAddr2;
						NiNode* UI = Offsets::cloneNode(retNode, &proc);
						UI->m_name = BSFixedString(meshName2[i]);
						HUD->AttachChild((NiAVObject*)UI, true);
						retNode = loadNifFromFile(MainHud2[i]);
						NiNode* UI2 = Offsets::cloneNode(retNode, &proc);
						UI2->m_name = BSFixedString(meshName[i]);
						UI->AttachChild((NiAVObject*)UI2, true);
					}
					setConfigModeHandPose();
					_isPBConfigModeActive = true;
					_PBConfigModeEnterCounter = 0;
				}
			}
			else if (!PBConfigButtonPressed && !_isPBConfigModeActive) {
				_PBConfigModeEnterCounter = 0;
			}
			if (_isPBConfigModeActive) {
				BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;
				NiPoint3 finger;
				c_leftHandedMode ? finger = rt->transforms[boneTreeMap["RArm_Finger23"]].world.pos : finger = rt->transforms[boneTreeMap["LArm_Finger23"]].world.pos;
				for (int i = 1; i <= 9; i++) {
					BSFixedString TouchName = meshName2[i];
					BSFixedString TransName = meshName[i];
					NiNode* TouchMesh = (NiNode*)_playerNodes->primaryUIAttachNode->GetObjectByName(&TouchName);
					NiNode* TransMesh = (NiNode*)_playerNodes->primaryUIAttachNode->GetObjectByName(&TransName);
					if (TouchMesh && TransMesh) {
						float distance = vec3_len(finger - TouchMesh->m_worldTransform.pos);
						if (distance > 2.0) {
							TransMesh->m_localTransform.pos.y = 0.0;
							if (i == 1 || i == 3) {
								_PBTouchbuttons[i] = false;
							}
						}
						else if (distance <= 2.0) {
							float fz = (2.0 - distance);
							if (fz > 0.0 && fz < 1.2) {
								TransMesh->m_localTransform.pos.y = (fz);
							}
							if ((TransMesh->m_localTransform.pos.y > 1.0) && !_PBTouchbuttons[i]) {
								if (vrhook != nullptr) {
									//_PBConfigSticky = true;
									c_leftHandedMode ? vrhook->StartHaptics(2, 0.05, 0.3) : vrhook->StartHaptics(1, 0.05, 0.3);
									for (int i = 1; i <= 9; i++) {
										if ((i != 1) && (i != 3))
										_PBTouchbuttons[i] = false;
									}
									BSFixedString bname = "PBCONFIGMarker";
									NiNode* UIMarker = (NiNode*)_playerNodes->primaryUIAttachNode->GetObjectByName(&bname);
									if (UIMarker) {
										UIMarker->m_parent->RemoveChild(UIMarker);
									}
									if ((i != 1) && (i != 3)) {
										NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigMarker.nif");
										NiCloneProcess proc;
										proc.unk18 = Offsets::cloneAddr1;
										proc.unk48 = Offsets::cloneAddr2;
										NiNode* UI = Offsets::cloneNode(retNode, &proc);
										UI->m_name = BSFixedString("PBCONFIGMarker");
										TouchMesh->AttachChild((NiAVObject*)UI, true);
									}
									_PBTouchbuttons[i] = true;
								}
							}
						}
					}
				}
				if (SaveButtonPressed && !_isSaveButtonPressed) {
					_isSaveButtonPressed = true;
					c_IsHoloPipboy ? g_weaponOffsets->addOffset("HoloPipboyPosition", pbRoot->m_localTransform, Mode::normal) : g_weaponOffsets->addOffset("PipboyPosition", pbRoot->m_localTransform, Mode::normal);
					writeOffsetJson();
				}
				else if (!SaveButtonPressed) {
					_isSaveButtonPressed = false;
				}
				if (ExitButtonPressed) {
					exitPBConfig();
				}
				if (ModelSwapButtonPressed && !_isModelSwapButtonPressed) {
					_isModelSwapButtonPressed = true;
					c_IsHoloPipboy ? c_IsHoloPipboy = false : c_IsHoloPipboy = true;
					_pipboyStatus = false;
					_stickypip = false;
					turnPipBoyOff();
					swapPB();
					_pipboyStatus = true;
					_stickypip = true;
					_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
					turnPipBoyOn();
					CSimpleIniA ini;
					SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\FRIK.ini");
					rc = ini.SetBoolValue("Fallout4VRBody", "HoloPipBoyEnabled", c_IsHoloPipboy);
					rc = ini.SaveFile(".\\Data\\F4SE\\plugins\\FRIK.ini");
				}
				else if (!ModelSwapButtonPressed) {
					_isModelSwapButtonPressed = false;
				}
				if ((doinantHandStick.y > 0.10 || doinantHandStick.y < -0.10) && (RotateButtonPressed)) {
					Matrix44 rot;
					rAxisOffsetX = doinantHandStick.y / 10;
					if (rAxisOffsetX < 0) {
						rAxisOffsetX = rAxisOffsetX * -1;
					}
					else {
						rAxisOffsetX = 0 - rAxisOffsetX;
					}
					rot.setEulerAngles((degrees_to_rads(rAxisOffsetX)), 0, 0);
					pbRoot->m_localTransform.rot = rot.multiply43Left(pbRoot->m_localTransform.rot);
					rot.multiply43Left(pbRoot->m_localTransform.rot);
				}
				if ((doinantHandStick.y > 0.10) && (ScaleButtonPressed)) {
					pbRoot->m_localTransform.scale = (pbRoot->m_localTransform.scale + 0.001);
				}
				if ((doinantHandStick.y < -0.10) && (ScaleButtonPressed)) {
					pbRoot->m_localTransform.scale = (pbRoot->m_localTransform.scale - 0.001);
				}
				if ((doinantHandStick.y > 0.10) && (MoveXButtonPressed)) {
					rAxisOffsetX = doinantHandStick.y / 50;
					pbRoot->m_localTransform.pos.x = (pbRoot->m_localTransform.pos.x + rAxisOffsetX);
				}
				if ((doinantHandStick.y < -0.10) && (MoveXButtonPressed)) {
					rAxisOffsetX = doinantHandStick.y / 50;
					pbRoot->m_localTransform.pos.x = (pbRoot->m_localTransform.pos.x + rAxisOffsetX);
				}
				if ((doinantHandStick.y > 0.10) && (MoveYButtonPressed)) {
					rAxisOffsetX = doinantHandStick.y / 20;
					pbRoot->m_localTransform.pos.y = (pbRoot->m_localTransform.pos.y + rAxisOffsetX);
				}
				if ((doinantHandStick.y < -0.10) && (MoveYButtonPressed)) {
					rAxisOffsetX = doinantHandStick.y / 20;
					pbRoot->m_localTransform.pos.y = (pbRoot->m_localTransform.pos.y + rAxisOffsetX);
				}
				if ((doinantHandStick.y > 0.10) && (MoveZButtonPressed)) {
					rAxisOffsetX = doinantHandStick.y / 20;
					pbRoot->m_localTransform.pos.z = (pbRoot->m_localTransform.pos.z - rAxisOffsetX);
				}
				if ((doinantHandStick.y < -0.10) && (MoveZButtonPressed)) {
					rAxisOffsetX = doinantHandStick.y / 20;
					pbRoot->m_localTransform.pos.z = (pbRoot->m_localTransform.pos.z - rAxisOffsetX);
				}

				if ((doinantHandStick.y > 0.10) && (ModelScaleButtonPressed) && (_3rdPipboy)) {
					rAxisOffsetX = doinantHandStick.y / 65;
					_3rdPipboy->m_localTransform.scale += rAxisOffsetX;
					c_pipBoyScale = _3rdPipboy->m_localTransform.scale;
				}
				if ((doinantHandStick.y < -0.10) && (ModelScaleButtonPressed) && (_3rdPipboy)) {
					rAxisOffsetX = doinantHandStick.y / 65;
					_3rdPipboy->m_localTransform.scale += rAxisOffsetX;
					c_pipBoyScale = _3rdPipboy->m_localTransform.scale;
				}
			}
		}
	}

	void Skeleton::setPipboyHandPose() {
		float position[15] = { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
		if (c_leftHandedPipBoy) {
			std::string finger [15] = {"LArm_Finger11", "LArm_Finger12", "LArm_Finger13", "LArm_Finger21", "LArm_Finger22", "LArm_Finger23", "LArm_Finger31", "LArm_Finger32", "LArm_Finger33", "LArm_Finger41", "LArm_Finger42", "LArm_Finger43", "LArm_Finger51", "LArm_Finger52", "LArm_Finger53" };
			for (int x = 0; x < 15; x++) {
				std::string f = finger[x];
				handPapyrusHasControl[f.c_str()] = true;
				handPapyrusPose[f.c_str()] = position[x];		
			}
		}
		else {
			std::string finger[15] = { "RArm_Finger11", "RArm_Finger12", "RArm_Finger13", "RArm_Finger21", "RArm_Finger22", "RArm_Finger23", "RArm_Finger31", "RArm_Finger32", "RArm_Finger33", "RArm_Finger41", "RArm_Finger42", "RArm_Finger43", "RArm_Finger51", "RArm_Finger52", "RArm_Finger53" };
			for (int x = 0; x < 15; x++) {
				std::string f = finger[x];
				handPapyrusHasControl[f.c_str()] = true;
				handPapyrusPose[f.c_str()] = position[x];
			}
		}
	}

	void Skeleton::disablePipboyHandPose() {
		if (c_leftHandedPipBoy) {
			std::string finger[15] = { "LArm_Finger11", "LArm_Finger12", "LArm_Finger13", "LArm_Finger21", "LArm_Finger22", "LArm_Finger23", "LArm_Finger31", "LArm_Finger32", "LArm_Finger33", "LArm_Finger41", "LArm_Finger42", "LArm_Finger43", "LArm_Finger51", "LArm_Finger52", "LArm_Finger53" };
			for (int x = 0; x < 15; x++) {
				std::string f = finger[x];
				handPapyrusHasControl[f.c_str()] = false;
			}
		}
		else {
			std::string finger[15] = { "RArm_Finger11", "RArm_Finger12", "RArm_Finger13", "RArm_Finger21", "RArm_Finger22", "RArm_Finger23", "RArm_Finger31", "RArm_Finger32", "RArm_Finger33", "RArm_Finger41", "RArm_Finger42", "RArm_Finger43", "RArm_Finger51", "RArm_Finger52", "RArm_Finger53" };
			for (int x = 0; x < 15; x++) {
				std::string f = finger[x];
				handPapyrusHasControl[f.c_str()] = false;
			}
		}
	}

	void Skeleton::setConfigModeHandPose() {
		float position[15] = { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
		if (c_leftHandedMode) {
			std::string finger[15] = { "RArm_Finger11", "RArm_Finger12", "RArm_Finger13", "RArm_Finger21", "RArm_Finger22", "RArm_Finger23", "RArm_Finger31", "RArm_Finger32", "RArm_Finger33", "RArm_Finger41", "RArm_Finger42", "RArm_Finger43", "RArm_Finger51", "RArm_Finger52", "RArm_Finger53" };
			for (int x = 0; x < 15; x++) {
				std::string f = finger[x];
				handPapyrusHasControl[f.c_str()] = true;
				handPapyrusPose[f.c_str()] = position[x];
			}
		}
		else {
			std::string finger[15] = { "LArm_Finger11", "LArm_Finger12", "LArm_Finger13", "LArm_Finger21", "LArm_Finger22", "LArm_Finger23", "LArm_Finger31", "LArm_Finger32", "LArm_Finger33", "LArm_Finger41", "LArm_Finger42", "LArm_Finger43", "LArm_Finger51", "LArm_Finger52", "LArm_Finger53" };
			for (int x = 0; x < 15; x++) {
				std::string f = finger[x];
				handPapyrusHasControl[f.c_str()] = true;
				handPapyrusPose[f.c_str()] = position[x];
			}
		}
	}

	void Skeleton::disableConfigModePose() {
		if (c_leftHandedMode) {
			std::string finger[15] = { "RArm_Finger11", "RArm_Finger12", "RArm_Finger13", "RArm_Finger21", "RArm_Finger22", "RArm_Finger23", "RArm_Finger31", "RArm_Finger32", "RArm_Finger33", "RArm_Finger41", "RArm_Finger42", "RArm_Finger43", "RArm_Finger51", "RArm_Finger52", "RArm_Finger53" };
			for (int x = 0; x < 15; x++) {
				std::string f = finger[x];
				handPapyrusHasControl[f.c_str()] = false;
			}
		}
		else {
			std::string finger[15] = { "LArm_Finger11", "LArm_Finger12", "LArm_Finger13", "LArm_Finger21", "LArm_Finger22", "LArm_Finger23", "LArm_Finger31", "LArm_Finger32", "LArm_Finger33", "LArm_Finger41", "LArm_Finger42", "LArm_Finger43", "LArm_Finger51", "LArm_Finger52", "LArm_Finger53" };
			for (int x = 0; x < 15; x++) {
				std::string f = finger[x];
				handPapyrusHasControl[f.c_str()] = false;
			}
		}
	}

    bool Skeleton::armorHasHeadLamp() { // detect if the player has an armor item which uses the headlamp equipped as not to overwrite it
        TESForm* equippedItem = (*g_player)->equipData->slots[0].item;
        if (equippedItem) {
            if (TESObjectARMO* tourchEnabledArmor = DYNAMIC_CAST(equippedItem, TESForm, TESObjectARMO)) {
                return HasKeyword(tourchEnabledArmor, 0xB34A6);
            }
        }
        return false;
    }
	// <<<< Cylons Code End

	void Skeleton::set1stPersonArm(NiNode* weapon, NiNode* offsetNode) {

		NiNode** wp = &weapon;
		NiNode** op = &offsetNode;

		update1stPersonArm(*g_player, wp, op);
	}

    void Skeleton::showHidePAHUD() {
        NiNode* wand = this->getPlayerNodes()->primaryUIAttachNode;
        BSFixedString bname("BackOfHand");
        NiNode* node = static_cast<NiNode*>(wand->GetObjectByName(&bname));

        if (node && _inPowerArmor) {
            node->m_worldTransform.pos += NiPoint3(-5.0, -7.0, 2.0);
            updateTransformsDown(node, false);
        }

        NiNode* hud = getNode("PowerArmorHelmetRoot", _playerNodes->roomnode);
        if (hud) {
            hud->m_localTransform.scale = c_showPAHUD ? 1.0f : 0.0f;
        }
    }

	void Skeleton::setLeftHandedSticky() {
		_leftHandedSticky = !c_leftHandedMode;
	}


    void Skeleton::handleWeaponNodes() {
        if (_leftHandedSticky == c_leftHandedMode) {
            return;
        }

        NiNode* rightWeapon = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
        NiNode* leftWeapon = _playerNodes->WeaponLeftNode;
        NiNode* rHand = getNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
        NiNode* lHand = getNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());

        if (!rightWeapon || !rHand || !leftWeapon || !lHand) {
            if (c_verbose) {
                _MESSAGE("Cannot set up weapon nodes");
            }
            _leftHandedSticky = c_leftHandedMode;
            return;
        }

        rHand->RemoveChild(rightWeapon);
        rHand->RemoveChild(leftWeapon);
        lHand->RemoveChild(rightWeapon);
        lHand->RemoveChild(leftWeapon);

        if (c_leftHandedMode) {
            rHand->AttachChild(leftWeapon, true);
            lHand->AttachChild(rightWeapon, true);
        } else {
            rHand->AttachChild(rightWeapon, true);
            lHand->AttachChild(leftWeapon, true);
        }

        if (c_IsOperatingPipboy) {
            rightWeapon->m_localTransform.scale = 0.0;
        }

        fixBackOfHand();
        _leftHandedSticky = c_leftHandedMode;
    }

	// This is the main arm IK solver function - Algo credit to prog from SkyrimVR VRIK mod - what a beast!
	void Skeleton::setArms(bool isLeft) {
		ArmNodes arm;

		arm = isLeft ? leftArm : rightArm;

		// This first part is to handle the game calculating the first person hand based off two offset nodes
		// PrimaryWeaponOffset and PrimaryMeleeoffset
		// Unfortunately neither of these two nodes are that close to each other so when you equip a melee or ranged weapon
		// the hand will jump which compeltely messes up the solver and looks bad to boot.
		// So this code below does a similar operation as the in game function that solves the first person arm by forcing
		// everything to go to the PrimaryWeaponNode.  I have hardcoded a rotation below based off one of the guns that
		// matches my real life hand pose with an index controller very well.   I use this as the baseline for everything

        if ((*g_player)->firstPersonSkeleton == nullptr) {
            return;
        }

        NiNode* rightWeapon = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
        NiNode* leftWeapon = getNode("WeaponLeft", (*g_player)->firstPersonSkeleton->GetAsNiNode());

        bool handleLeftMode = c_leftHandedMode ^ isLeft;

        NiNode* weaponNode = handleLeftMode ? leftWeapon : rightWeapon;
        NiNode* offsetNode = handleLeftMode ? _playerNodes->SecondaryMeleeWeaponOffsetNode2 : _playerNodes->primaryWeaponOffsetNOde;

        if (handleLeftMode) {
            _playerNodes->SecondaryMeleeWeaponOffsetNode2->m_localTransform = _playerNodes->primaryWeaponOffsetNOde->m_localTransform;
            Matrix44 lr;
            lr.setEulerAngles(0, degrees_to_rads(180), 0);
            _playerNodes->SecondaryMeleeWeaponOffsetNode2->m_localTransform.rot = lr.multiply43Right(_playerNodes->SecondaryMeleeWeaponOffsetNode2->m_localTransform.rot);
            _playerNodes->SecondaryMeleeWeaponOffsetNode2->m_localTransform.pos = NiPoint3(-2, -9, 2);
            updateTransforms(_playerNodes->SecondaryMeleeWeaponOffsetNode2);
        }

        Matrix44 w;
        if (!c_leftHandedMode) {
            w.data[0][0] = -0.122;
            w.data[1][0] = 0.987;
            w.data[2][0] = 0.100;
            w.data[0][1] = 0.990;
            w.data[1][1] = 0.114;
            w.data[2][1] = 0.081;
            w.data[0][2] = 0.069;
            w.data[1][2] = 0.109;
            w.data[2][2] = -0.992;
        } else {
            w.data[0][0] = -0.122;
            w.data[1][0] = 0.987;
            w.data[2][0] = 0.100;
            w.data[0][1] = -0.990;
            w.data[1][1] = -0.114;
            w.data[2][1] = -0.081;
            w.data[0][2] = -0.069;
            w.data[1][2] = -0.109;
            w.data[2][2] = 0.992;
        }

        _weapSave = c_leftHandedMode ? leftWeapon->m_localTransform : rightWeapon->m_localTransform;

        weaponNode->m_localTransform.rot = w.make43();

        if (handleLeftMode) {
            w.setEulerAngles(degrees_to_rads(0), degrees_to_rads(isLeft ? 45 : -45), degrees_to_rads(0));
            weaponNode->m_localTransform.rot = w.multiply43Right(weaponNode->m_localTransform.rot);
        }

        if (c_leftHandedMode) {
            w.setEulerAngles(degrees_to_rads(0), degrees_to_rads(45), degrees_to_rads(0));
            _weapSave.rot = w.multiply43Right(_weapSave.rot);
        }

        weaponNode->m_localTransform.pos = c_leftHandedMode ? (isLeft ? NiPoint3(3.389, -2.099, 3.133) : NiPoint3(0, -4.8, 0)) : (isLeft ? NiPoint3(0, 0, 0) : NiPoint3(6.389, -2.099, -3.133));

        dampenHand(offsetNode, isLeft);

        weaponNode->IncRef();
        set1stPersonArm(weaponNode, offsetNode);

        NiPoint3 handPos = isLeft ? _leftHand->m_worldTransform.pos : _rightHand->m_worldTransform.pos;
        NiMatrix43 handRot = isLeft ? _leftHand->m_worldTransform.rot : _rightHand->m_worldTransform.rot;

        // Detect if the 1st person hand position is invalid. This can happen when a controller loses tracking.
        // If it is, do not handle IK and let Fallout use its normal animations for that arm instead.
        if (isnan(handPos.x) || isnan(handPos.y) || isnan(handPos.z) ||
            isinf(handPos.x) || isinf(handPos.y) || isinf(handPos.z) ||
            vec3_len(arm.upper->m_worldTransform.pos - handPos) > 200.0) {
            return;
        }


		double adjustedArmLength = c_armLength / 36.74;

		// Shoulder IK is done in a very simple way

		NiPoint3 shoulderToHand = handPos - arm.upper->m_worldTransform.pos;
		float armLength = c_armLength;
		float adjustAmount = (std::clamp)(vec3_len(shoulderToHand) - armLength * 0.5f, 0.0f, armLength * 0.85f) / (armLength * 0.85f);
		NiPoint3 shoulderOffset = vec3_norm(shoulderToHand) * (adjustAmount * armLength * 0.08f);

		NiPoint3 clavicalToNewShoulder = arm.upper->m_worldTransform.pos + shoulderOffset - arm.shoulder->m_worldTransform.pos;

		NiPoint3 sLocalDir = (arm.shoulder->m_worldTransform.rot.Transpose() * clavicalToNewShoulder) / arm.shoulder->m_worldTransform.scale;

		Matrix44 rotatedM;
		rotatedM = 0.0;
		rotatedM.rotateVectoVec(sLocalDir, NiPoint3(1, 0, 0));

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

		float originalUpperLen = vec3_len(arm.forearm1->m_localTransform.pos);
		float originalForearmLen;

		if (_inPowerArmor) {
			originalForearmLen = vec3_len(arm.hand->m_localTransform.pos);
		}
		else {
			originalForearmLen = vec3_len(arm.hand->m_localTransform.pos) + vec3_len(arm.forearm2->m_localTransform.pos) + vec3_len(arm.forearm3->m_localTransform.pos);
		}
		float upperLen = originalUpperLen * adjustedArmLength;
		float forearmLen = originalForearmLen * adjustedArmLength;

		NiPoint3 Uwp = arm.upper->m_worldTransform.pos;
		NiPoint3 handToShoulder = Uwp - handPos;
		float hsLen = (std::max)(vec3_len(handToShoulder), 0.1f);

		if (hsLen > (upperLen + forearmLen) * 2.25) {
			return;
		}


		// Stretch the upper arm and forearm proportionally when the hand distance exceeds the arm length
		if (hsLen > upperLen + forearmLen) {
			float diff = hsLen - upperLen - forearmLen;
			float ratio = forearmLen / (forearmLen + upperLen);
			forearmLen += ratio * diff + 0.1;
			upperLen += (1.0 - ratio) * diff + 0.1;
		}

		NiPoint3 forwardDir = vec3_norm(_forwardDir);
		NiPoint3 sidewaysDir = vec3_norm(_sidewaysRDir * negLeft);

		// The primary twist angle comes from the direction the wrist is pointing into the forearm
		NiPoint3 handBack = handRot * NiPoint3(-1, 0, 0);
		float twistAngle = asinf((std::clamp)(handBack.z, -0.999f, 0.999f));

		// The second twist angle comes from a side vector pointing "outward" from the side of the wrist
		NiPoint3 handSide = handRot * NiPoint3(0, -1, 0);
		NiPoint3 handinSide = handSide * negLeft;
		float twistAngle2 = -1 * asinf((std::clamp)(handSide.z, -0.599f, 0.999f));

		// Blend the two twist angles together, using the primary angle more when the wrist is pointing downward
		//float interpTwist = (std::clamp)((handBack.z + 0.866f) * 1.155f, 0.25f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
		float interpTwist = (std::clamp)((handBack.z + 0.866f) * 1.155f, 0.45f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
//		_MESSAGE("%2f %2f %2f", rads_to_degrees(twistAngle), rads_to_degrees(twistAngle2), interpTwist);
		twistAngle = twistAngle + interpTwist * (twistAngle2 - twistAngle);
		// Wonkiness is bad.  Interpolate twist angle towards zero to correct it when the angles are pointed a certain way.
	/*	float fixWonkiness1 = (std::clamp)(vec3_dot(handSide, vec3_norm(-sidewaysDir - forwardDir * 0.25f + NiPoint3(0, 0, -0.25))), 0.0f, 1.0f);
		float fixWonkiness2 = 1.0f - (std::clamp)(vec3_dot(handBack, vec3_norm(forwardDir + sidewaysDir)), 0.0f, 1.0f);
		twistAngle = twistAngle + fixWonkiness1 * fixWonkiness2 * (-PI / 2.0f - twistAngle);*/

//		_MESSAGE("final angle %2f", rads_to_degrees(twistAngle));

		// Smooth out sudden changes in the twist angle over time to reduce elbow shake
		static std::array<float, 2> prevAngle = { 0, 0 };
		twistAngle = prevAngle[isLeft ? 0 : 1] + (twistAngle - prevAngle[isLeft ? 0 : 1]) * 0.25f;
		prevAngle[isLeft ? 0 : 1] = twistAngle;

		// Calculate the hand's distance behind the body - It will increase the minimum elbow rotation angle
		float size = 1.0;
		float behindD = -(forwardDir.x * arm.shoulder->m_worldTransform.pos.x + forwardDir.y * arm.shoulder->m_worldTransform.pos.y) - 10.0f;
		float handBehindDist = -(handPos.x * forwardDir.x + handPos.y * forwardDir.y + behindD);
		float behindAmount = (std::clamp)(handBehindDist / (40.0f * size), 0.0f, 1.0f);

		// Holding hands in front of chest increases the minimum elbow rotation angle (elbows lift) and decreases the maximum angle
		NiPoint3 planeDir = rotateXY(forwardDir, negLeft * degrees_to_rads(135));
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
		float twistMinAngle = degrees_to_rads(-85.0) + degrees_to_rads(50) * adjustMinAmount;
		float twistMaxAngle = degrees_to_rads(55.0) - (std::max)(degrees_to_rads(90) * armCrossAmount, degrees_to_rads(70) * upLimit);

		// Twist angle ranges from -PI/2 to +PI/2; map that range to go from the minimum to the maximum instead
		float twistLimitAngle = twistMinAngle + ((twistAngle + PI / 2.0f) / PI) * (twistMaxAngle - twistMinAngle);

		//_MESSAGE("%f %f %f %f", rads_to_degrees(twistAngle), rads_to_degrees(twistAngle2), rads_to_degrees(twistAngle), rads_to_degrees(twistLimitAngle));
		// The bendDownDir vector points in the direction the player faces, and bends up/down with the final elbow angle
		NiMatrix43 rot = getRotationAxisAngle(sidewaysDir * negLeft, twistLimitAngle);
		NiPoint3 bendDownDir = rot * forwardDir;

		// Get the "X" direction vectors pointing to the shoulder
		NiPoint3 xDir = vec3_norm(handToShoulder);

		// Get the final "Y" vector, perpendicular to "X", and pointing in elbow direction (as in the diagram above)
		float sideD = -(sidewaysDir.x * arm.shoulder->m_worldTransform.pos.x + sidewaysDir.y * arm.shoulder->m_worldTransform.pos.y) - 1.0 * 8.0f;
		float acrossAmount = -(handPos.x * sidewaysDir.x + handPos.y * sidewaysDir.y + sideD) / (16.0f * 1.0);
		float handSideTwistOutward = vec3_dot(handSide, vec3_norm(sidewaysDir + (forwardDir * 0.5f)));
		float armTwist = (std::clamp)(handSideTwistOutward - (std::max)(0.0f, acrossAmount + 0.25f), 0.0f, 1.0f);

		if (acrossAmount < 0) {
			acrossAmount *= 0.2f;
		}

		float handBehindHead = (std::clamp)((handBehindDist + 0.0f * size) / (15.0f * size), 0.0f, 1.0f) * (std::clamp)(upLimit * 1.2f, 0.0f, 1.0f);
		float elbowsTwistForward = (std::max)(acrossAmount * degrees_to_rads(90), handBehindHead * degrees_to_rads(120));
		NiPoint3 elbowDir = rotateXY(bendDownDir, -negLeft * (degrees_to_rads(150) - armTwist * degrees_to_rads(25) - elbowsTwistForward));
		NiPoint3 yDir = elbowDir - xDir * vec3_dot(elbowDir, xDir);
		yDir = vec3_norm(yDir);

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
		NiPoint3 uLocalDir = Uwr.Transpose() * vec3_norm(pos) / arm.upper->m_worldTransform.scale;

		rotatedM.rotateVectoVec(uLocalDir, arm.forearm1->m_localTransform.pos);
		arm.upper->m_localTransform.rot = rotatedM.multiply43Left(arm.upper->m_localTransform.rot);

		rotatedM.makeTransformMatrix(arm.upper->m_localTransform.rot, arm.upper->m_localTransform.pos);

		Uwr = rotatedM.multiply43Left(arm.shoulder->m_worldTransform.rot);

		// Find the angle of the forearm twisted around the upper arm and twist the upper arm to align it
		//    Uwr * twist = Cwr * Ulr   ===>   Ulr = Cwr' * Uwr * twist
		pos = handPos - elbowWorld;
		NiPoint3 uLocalTwist = Uwr.Transpose() * vec3_norm(pos);
		uLocalTwist.x = 0;
		NiPoint3 upperSide = arm.upper->m_worldTransform.rot * NiPoint3(0, 1, 0);
		NiPoint3 uloc = arm.shoulder->m_worldTransform.rot.Transpose() * upperSide;
		uloc.x = 0;
		float upperAngle = acosf(vec3_dot(vec3_norm(uLocalTwist), vec3_norm(uloc))) * (uLocalTwist.z > 0 ? 1 : -1);

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
		NiPoint3 fLocalDir = Fwr.Transpose() * vec3_norm(elbowHand);

		rotatedM.rotateVectoVec(fLocalDir, NiPoint3(1, 0, 0));
		arm.forearm1->m_localTransform.rot = rotatedM.multiply43Left(arm.forearm1->m_localTransform.rot);
		rotatedM.makeTransformMatrix(arm.forearm1->m_localTransform.rot, arm.forearm1->m_localTransform.pos);
		Fwr = rotatedM.multiply43Left(Uwr);

		NiMatrix43 Fwr2, Fwr3;

		if (!_inPowerArmor && (arm.forearm2 != nullptr) && (arm.forearm3 != nullptr)) {
			rotatedM.makeTransformMatrix(arm.forearm2->m_localTransform.rot, arm.forearm2->m_localTransform.pos);
			Fwr2 = rotatedM.multiply43Left(Fwr);
			rotatedM.makeTransformMatrix(arm.forearm3->m_localTransform.rot, arm.forearm3->m_localTransform.pos);
			Fwr3 = rotatedM.multiply43Left(Fwr2);




			// Find the angle the wrist is pointing and twist forearm3 appropriately
			//    Fwr * twist = Uwr * Flr   ===>   Flr = (Uwr' * Fwr) * twist = (Flr) * twist

			NiPoint3 wLocalDir = Fwr3.Transpose() * vec3_norm(handinSide);
			wLocalDir.x = 0;
			NiPoint3 forearm3Side = Fwr3 * NiPoint3(0, 0, -1);   // forearm is rotated 90 degrees already from hand so need this vector instead of 0,-1,0
			NiPoint3 floc = Fwr2.Transpose() * vec3_norm(forearm3Side);
			floc.x = 0;
			float fcos = vec3_dot(vec3_norm(wLocalDir), vec3_norm(floc));
			float fsin = vec3_det(vec3_norm(wLocalDir), vec3_norm(floc), NiPoint3(-1, 0, 0));
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
		}
		else {
			arm.hand->m_localTransform.rot = rotatedM.multiply43Left(Fwr.Transpose());
		}

		// Calculate Flp:  Fwp = Uwp + Uwr * (Flp * Uws) = elbowWorld   ===>   Flp = Uwr' * (elbowWorld - Uwp) / Uws
		arm.forearm1->m_localTransform.pos = Uwr.Transpose() * ((elbowWorld - Uwp) / arm.upper->m_worldTransform.scale);

		float origEHLen = vec3_len(arm.hand->m_worldTransform.pos - arm.forearm1->m_worldTransform.pos);
		float forearmRatio = (forearmLen / origEHLen) * _root->m_localTransform.scale;

		if (arm.forearm2 && !_inPowerArmor) {
			arm.forearm2->m_localTransform.pos *= forearmRatio;
			arm.forearm3->m_localTransform.pos *= forearmRatio;
		}
		arm.hand->m_localTransform.pos     *= forearmRatio;

		return;
	}

	void Skeleton::showOnlyArms() {
		NiPoint3 rwp = rightArm.shoulder->m_worldTransform.pos;
		NiPoint3 lwp = leftArm.shoulder->m_worldTransform.pos;
		_root->m_localTransform.scale = 0.00001;
		updateTransforms(_root);
		_root->m_worldTransform.pos += _forwardDir * -10.0f;
		_root->m_worldTransform.pos.z = rwp.z;
		updateDown(_root, false);

		rightArm.shoulder->m_localTransform.scale = 100000;
		leftArm.shoulder->m_localTransform.scale = 100000;

		updateTransforms((NiNode*)rightArm.shoulder);
		updateTransforms((NiNode*)leftArm.shoulder);

		rightArm.shoulder->m_worldTransform.pos = rwp;
		leftArm.shoulder->m_worldTransform.pos = lwp;

		updateDown((NiNode*)rightArm.shoulder, false);
		updateDown((NiNode*)leftArm.shoulder, false);
	}

	void Skeleton::hideHands() {
		NiPoint3 rwp = rightArm.shoulder->m_worldTransform.pos;
		_root->m_localTransform.scale = 0.00001;
		updateTransforms(_root);
		_root->m_worldTransform.pos += _forwardDir * -10.0f;
		_root->m_worldTransform.pos.z = rwp.z;
		updateDown(_root, false);

	}

    void Skeleton::calculateHandPose(std::string bone, float gripProx, bool thumbUp, bool isLeft) {
        Quaternion qc, qt;
        int sign = isLeft ? -1 : 1;

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
                rot.setEulerAngles(sign * 0.5, sign * 0.4, -0.3);
            } else if (bone.find("Finger13") != std::string::npos) {
                rot.setEulerAngles(0, 0, degrees_to_rads(-35.0));
            }
            NiMatrix43 wr = handOpen[bone].rot;
            wr = rot.multiply43Left(wr);
            qt.fromRot(wr);
        }
        else if (_closedHand[bone]) {
            qt.fromRot(handClosed[bone].rot);
        }
        else {
            qt.fromRot(handOpen[bone].rot);
            if (_handBonesButton[bone] == vr::k_EButton_Grip) {
                Quaternion qo;
                qo.fromRot(handClosed[bone].rot);
                qo.slerp(1.0 - gripProx, qt);
                qt = qo;
            }
        }

        qc.fromRot(_handBones[bone].rot);
        float blend = std::clamp(_frameTime * 7, 0.0, 1.0);
        qc.slerp(blend, qt);
        _handBones[bone].rot = qc.getRot().make43();
    }

	void Skeleton::copy1stPerson(std::string bone) {
		BSFlattenedBoneTree* fpTree = (BSFlattenedBoneTree*)(*g_player)->firstPersonSkeleton->m_children.m_data[0]->GetAsNiNode();

		int pos = fpTree->GetBoneIndex(bone);


		if (pos >= 0) {
			if (fpTree->transforms[pos].refNode) {
				_handBones[bone] = fpTree->transforms[pos].refNode->m_localTransform;
			}
			else {
				_handBones[bone] = fpTree->transforms[pos].local;
			}
		}
	}

    void Skeleton::fixBoneTree() {
        BSFlattenedBoneTree* rt = reinterpret_cast<BSFlattenedBoneTree*>(_root);

        if (rt->numTransforms > 145) {
            return;
        }

        for (auto pos = 0; pos < rt->numTransforms; ++pos) {
            const auto& name = rt->transforms[pos].name;
            auto found = fingerRelations.find(name.c_str());
            if (found != fingerRelations.end()) {
                const auto& relation = found->second;
                int parentPos = boneTreeMap[relation.first];
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

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;
		bool isLeft = false;

	//	fixBoneTree();

		//if (rt->numTransforms > 145) {
		//	return;
		//}

		for (auto pos = 0; pos < rt->numTransforms; pos++) {

			std::string name = boneTreeVec[pos];
			auto found = fingerRelations.find(name.c_str());
			if (found != fingerRelations.end()) {
				isLeft = name[0] == 'L';
				uint64_t reg = isLeft ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonTouched : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonTouched;
				float gripProx = isLeft ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[2].x : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[2].x;
				bool thumbUp = (reg & vr::ButtonMaskFromId(vr::k_EButton_Grip)) && (reg & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) && (!(reg & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad)));
				_closedHand[name] = reg & vr::ButtonMaskFromId(_handBonesButton[name]);

				if ((*g_player)->actorState.IsWeaponDrawn() && !_pipboyStatus && !c_IsOperatingPipboy && !(isLeft ^ c_leftHandedMode)) { // CylonSurfer Updated conditions to cater for Virtual Pipboy usage (Ensures Index Finger is extended when weapon is drawn)
					this->copy1stPerson(name);
				}
				else {
					this->calculateHandPose(name, gripProx, thumbUp, isLeft);
				}
				
				NiTransform trans = _handBones[name];

				rt->transforms[pos].local.rot = trans.rot;
				rt->transforms[pos].local.pos = handOpen[name.c_str()].pos;

				if (rt->transforms[pos].refNode) {
					rt->transforms[pos].refNode->m_localTransform = rt->transforms[pos].local;

				}
			}

			if (rt->transforms[pos].refNode) {
				rt->transforms[pos].world = rt->transforms[pos].refNode->m_worldTransform;
			}
			else {
				short parent = rt->transforms[pos].parPos;
				NiPoint3 p = rt->transforms[pos].local.pos;
				p = (rt->transforms[parent].world.rot * (p * rt->transforms[parent].world.scale));

				rt->transforms[pos].world.pos = rt->transforms[parent].world.pos + p;

				Matrix44 rot;
				rot.makeTransformMatrix(rt->transforms[pos].local.rot, NiPoint3(0, 0, 0));

				rt->transforms[pos].world.rot = rot.multiply43Left(rt->transforms[parent].world.rot);
			}
		}

		if ((*g_player)->actorState.IsWeaponDrawn()) {
			NiNode* weap = getNode("Weapon", (*g_player)->firstPersonSkeleton);

			std::string weapname("");
			if ((*g_player)->middleProcess->unk08->equipData) {
				weapname = (*g_player)->middleProcess->unk08->equipData->item->GetFullName();
			}

			if (weap) {
				
				Matrix44 ident = Matrix44();

				ident.data[2][0] = 1.0;
				ident.data[0][1] = 1.0;
				ident.data[1][2] = 1.0;
				ident.data[3][3] = 1.0;
				_playerNodes->primaryWeaponScopeCamera->m_localTransform.rot = ident.make43();
				updateTransforms(dynamic_cast<NiNode*>(_playerNodes->primaryWeaponScopeCamera));


				if (!c_staticGripping) {
					float oldVecX = weap->m_localTransform.rot.data[0][0];
					float oldVecY = weap->m_localTransform.rot.data[1][0];
					float oldVecZ = weap->m_localTransform.rot.data[2][0];
					float newVecX = _weapSave.rot.data[0][0];
					float newVecY = _weapSave.rot.data[1][0];
					float newVecZ = _weapSave.rot.data[2][0];
					float dotProd = oldVecX * newVecX + oldVecY * newVecY + oldVecZ * newVecZ; //dot product is equal to the cosine of the angle between multiplied by the maginitude of the vectors. Maths!
					float magnitude = sqrtf(((oldVecX * oldVecX) + (oldVecY * oldVecY) + (oldVecZ * oldVecZ)) * ((newVecX * newVecX) + (newVecY * newVecY) + (newVecZ * newVecZ))); //sqrt(A)*sqrt(B)=sqrt(A*B)
					//_MESSAGE(std::to_string(dotProd / magnitude).c_str());
					//prevent dynamic grip if the rotation will be at or above 69 degrees, dirty melee fix. TODO replace with INI option? and more proper way of detecting melee weapons?
					if (dotProd >= (magnitude * 0.35836794954530027348413778941347)) { //cos(69) = 0.35836794954530027348413778941347
						weap->m_localTransform = _weapSave;

						// rotate scope parent so it matches the dynamic grip

						NiPoint3 staticVec = NiPoint3(-0.120, 0.987, 0.108);
						NiPoint3 dynamicVec = NiPoint3(_weapSave.rot.data[0][0], _weapSave.rot.data[1][0], _weapSave.rot.data[2][0]);

						Quaternion dynRotQ;

						dynRotQ.vec2vec(staticVec, dynamicVec);
						Matrix44 rot = dynRotQ.getRot();

						_playerNodes->primaryWeaponScopeCamera->m_localTransform.rot = rot.multiply43Left(_playerNodes->primaryWeaponScopeCamera->m_localTransform.rot);
						//	updateTransforms(dynamic_cast<NiNode*>(_playerNodes->primaryWeaponScopeCamera));
					}
				}
				auto newWeapon = (_inPowerArmor ? weapname + powerArmorSuffix : weapname) != _lastWeapon;
				if (newWeapon) {
					_lastWeapon = (_inPowerArmor ? weapname + powerArmorSuffix : weapname);
					_useCustomWeaponOffset = false;
					_useCustomOffHandOffset = false;
					auto lookup = g_weaponOffsets->getOffset(weapname, _inPowerArmor ? Mode::offHandwithPowerArmor : Mode::offHand);
					if (lookup.has_value()) {
						_useCustomOffHandOffset = true;
						_offhandOffset = lookup.value();
						_MESSAGE("Found offHandOffset for %s pos (%f, %f, %f) scale %f: powerArmor: %d",
							weapname, _offhandOffset.pos.x, _offhandOffset.pos.y, _offhandOffset.pos.z, _offhandOffset.scale, _inPowerArmor);
					}
					lookup = g_weaponOffsets->getOffset(weapname, _inPowerArmor ? Mode::powerArmor : Mode::normal);
					if (lookup.has_value()) {
						_useCustomWeaponOffset = true;
						_customTransform = lookup.value();
						_MESSAGE("Found weaponOffset for %s pos (%f, %f, %f) scale %f: powerArmor: %d",
							weapname, _customTransform.pos.x, _customTransform.pos.y, _customTransform.pos.z, _customTransform.scale, _inPowerArmor);
					}
					else { // offsets should already be applied if not already saved
						NiPoint3 offset = NiPoint3(-0.94, 0, 0); // apply static VR offset
						NiNode* weapOffset = getNode("WeaponOffset", weap);

						if (weapOffset) {
							offset.x -= weapOffset->m_localTransform.pos.y;
							offset.y -= -2.099;
							_MESSAGE("%s: WeaponOffset pos (%f, %f, %f) scale %f", weapname, weapOffset->m_localTransform.pos.x, weapOffset->m_localTransform.pos.y, weapOffset->m_localTransform.pos.z,
								weapOffset->m_localTransform.scale);
						}
						weap->m_localTransform.pos += offset;
					}
				}

				if (c_leftHandedMode) {
					weap->m_localTransform.pos.x += 6.25f;
					weap->m_localTransform.pos.y += 2.5f;
					weap->m_localTransform.pos.z += 2.75f;
				}
				if (_useCustomWeaponOffset) { // load custom transform
					weap->m_localTransform = _customTransform;
				}
				else // save transform to manipulate
					_customTransform = weap->m_localTransform;
				updateDown(weap, true);

				// handle offhand gripping

				static NiPoint3 _offhandFingerBonePos = NiPoint3(0, 0, 0);
				static NiPoint3 bodyPos = NiPoint3(0, 0, 0);
				static float avgHandV[3] = { 0.0f, 0.0f, 0.0f };
				static int fc = 0;
				float handV = 0.0f;

				auto offHandBone = c_leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";
				auto onHandBone = !c_leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";
				if (_offHandGripping && c_enableOffHandGripping) {

					float handFrameMovement;

					handFrameMovement = vec3_len(rt->transforms[boneTreeMap[offHandBone]].world.pos - _offhandFingerBonePos);

					float bodyFrameMovement = vec3_len(_curPos - bodyPos);
					avgHandV[fc] = abs(handFrameMovement - bodyFrameMovement);

					fc = fc == 2 ? 0 : fc + 1;

					float sum = 0;

					for (int i = 0; i < 3; i++) {
						sum += avgHandV[i];
					}

					handV = sum / 3;

					uint64_t reg = c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed;
					if (c_onePressGripButton && _hasLetGoGripButton) {
						_offHandGripping = false;
					}
					else if (c_enableGripButtonToLetGo && _hasLetGoGripButton) {
						if (reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_gripButtonID)) {
							_offHandGripping = false;
							_hasLetGoGripButton = false;
						}
					}
					else if ((handV > c_gripLetGoThreshold) && !c_isLookingThroughScope) {
						_offHandGripping = false;
					}
					uint64_t _pressLength = 0;
					if (!_repositionButtonHolding) {
						if (reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_repositionButtonID)) {
							_repositionButtonHolding = true;
							_hasLetGoRepositionButton = false;
							_repositionButtonHoldStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
							_startFingerBonePos = rt->transforms[boneTreeMap[offHandBone]].world.pos - _curPos;
							_offsetPreview = weap->m_localTransform.pos;
							_MESSAGE("Reposition Button Hold start: weapon %s mode: %d", weapname, _repositionMode);
						}
					}
					else {
						if (!_repositionModeSwitched && reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_offHandActivateButtonID)) {
							_repositionMode = static_cast<repositionMode>((_repositionMode + 1) % (repositionMode::total + 1));
							if (vrhook)
								vrhook->StartHaptics(c_leftHandedMode ? 0 : 1, 0.1 * (_repositionMode + 1), 0.3);
							_repositionModeSwitched = true;
							_MESSAGE("Reposition Mode Switch: weapon %s %d ms mode: %d", weapname, _pressLength, _repositionMode);
						}
						else if (_repositionModeSwitched && !(reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_offHandActivateButtonID))) {
							_repositionModeSwitched = false;
						}
						_pressLength = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _repositionButtonHoldStart;
						if (!_inRepositionMode && reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_repositionButtonID) && _pressLength > c_holdDelay) {
							if (vrhook && c_repositionMasterMode)
								vrhook->StartHaptics(c_leftHandedMode ? 0 : 1, 0.1 * (_repositionMode + 1), 0.3);
							_inRepositionMode = c_repositionMasterMode;
						}
						else if (!(reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_repositionButtonID))) {
							_repositionButtonHolding = false;
							_hasLetGoRepositionButton = true;
							_inRepositionMode = false;
							_endFingerBonePos = rt->transforms[boneTreeMap[offHandBone]].world.pos - _curPos;
							_MESSAGE("Reposition Button Hold stop: weapon %s %d ms mode: %d", weapname, _pressLength, _repositionMode);
						}
					}

					if (_offHandGripping) {

						NiPoint3 oH2Bar; // off hand to barrel

						oH2Bar = rt->transforms[boneTreeMap[offHandBone]].world.pos - weap->m_worldTransform.pos;

						if (_useCustomOffHandOffset) {
							// TODO: figure out y offset (left-right)
							//NiPoint3 zZeroedOffhandOffset = _offhandOffset.pos;
							//zZeroedOffhandOffset.z = 0;
							//auto fcos = vec3_dot(vec3_norm(zZeroedOffhandOffset), vec3_norm(_offhandOffset.pos));
							//auto fsin = vec3_det(vec3_norm(zZeroedOffhandOffset), vec3_norm(_offhandOffset.pos), NiPoint3(-1, 0, 0));
							//oH2Bar.y -= _offhandOffset.pos.y;
							oH2Bar.z -= _offhandOffset.pos.z;
						}							
						oH2Bar.z += 3.5f;

						NiPoint3 barrelVec = NiPoint3(0, 1, 0);

						NiPoint3 scopeVecLoc = oH2Bar;
						oH2Bar = weap->m_worldTransform.rot.Transpose() * vec3_norm(oH2Bar) / weap->m_worldTransform.scale;
						scopeVecLoc = _playerNodes->primaryWeaponScopeCamera->m_worldTransform.rot.Transpose() * vec3_norm(scopeVecLoc) / _playerNodes->primaryWeaponScopeCamera->m_worldTransform.scale;

						Matrix44 rot;
						//rot.rotateVectoVec(oH2Bar, barrelVec);

						// use Quaternion to rotate the vector onto the other vector to avoid issues with the poles
						_aimAdjust.vec2vec(vec3_norm(oH2Bar), vec3_norm(barrelVec));
						rot = _aimAdjust.getRot();
						_originalWeaponRot = weap->m_localTransform.rot; // save unrotated vector
						weap->m_localTransform.rot = rot.multiply43Left(weap->m_localTransform.rot);

						// rotate scopeParent so scope widget works
						Quaternion scopeQ;
						barrelVec = NiPoint3(1, 0, 0);
						_aimAdjust.vec2vec(vec3_norm(scopeVecLoc), vec3_norm(barrelVec));
						rot = _aimAdjust.getRot();
						
						_playerNodes->primaryWeaponScopeCamera->m_localTransform.rot = rot.multiply43Left(_playerNodes->primaryWeaponScopeCamera->m_localTransform.rot);
						//updateTransforms(dynamic_cast<NiNode*>(_playerNodes->primaryWeaponScopeCamera));

						_offhandFingerBonePos = rt->transforms[boneTreeMap[offHandBone]].world.pos;
						_offhandPos = _offhandFingerBonePos;
						bodyPos = _curPos;
						vr::VRControllerAxis_t axis_state = !(c_pipBoyButtonArm > 0) ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0];
						if (_repositionButtonHolding && c_repositionMasterMode) {
							// this is for a preview of the move. The preview happens one frame before we detect the release so must be processed separately.
							auto end = _offhandPos - _curPos;
							auto change = vec3_len(end) > vec3_len(_startFingerBonePos) ? vec3_len(end - _startFingerBonePos) : -vec3_len(end - _startFingerBonePos);
							switch (_repositionMode) {
							case weapon:
								showWands(wandMode::offhandWand);
								_offsetPreview.x = change;
								if (axis_state.x != 0.f || axis_state.y != 0.f) { // axis_state y is up and down, which corresponds to reticle z axis
									_offsetPreview.y += axis_state.x;
									_offsetPreview.z -= axis_state.y;
									_DMESSAGE("Updating weapon to (%f,%f) with analog input: (%f, %f)", _offsetPreview.y, _offsetPreview.z, axis_state.x, axis_state.y);
								}
								if (change != 0.f)
									_DMESSAGE("Previewing translation for %s by %f to %f", weapname, change, weap->m_localTransform.pos.x + _offsetPreview.x);
								weap->m_localTransform.pos.x += _offsetPreview.x; // x is a distance delta
								weap->m_localTransform.pos.y = _offsetPreview.y; // y, z are cumulative
								weap->m_localTransform.pos.z = _offsetPreview.z;
								break;
							case offhand:
								showWands(wandMode::mainhandWand);
								weap->m_localTransform.rot = _originalWeaponRot;
								break;
							case resetToDefault:
								showWands(wandMode::both);
								break;
							}
						}
						//_hasLetGoRepositionButton is always one frame after _repositionButtonHolding
						if (_hasLetGoRepositionButton && _pressLength > 0 && _pressLength < c_holdDelay && c_repositionMasterMode) {
							_MESSAGE("Updating grip rotation for %s: powerArmor: %d", weapname, _inPowerArmor);
							_customTransform.rot = weap->m_localTransform.rot;
							_hasLetGoRepositionButton = false;
							_useCustomWeaponOffset = true;
							g_weaponOffsets->addOffset(weapname, _customTransform, _inPowerArmor ? Mode::powerArmor : Mode::normal);
							writeOffsetJson();
						}
						else if (_hasLetGoRepositionButton && _pressLength > 0 && _pressLength > c_holdDelay && c_repositionMasterMode) {
							switch (_repositionMode) {
							case weapon:
								_MESSAGE("Saving position translation for %s from (%f, %f, %f) -> (%f, %f, %f): powerArmor: %d", weapname, weap->m_localTransform.pos.x, weap->m_localTransform.pos.y, weap->m_localTransform.pos.z, _offsetPreview.x, _offsetPreview.y, _offsetPreview.z, _inPowerArmor);
								weap->m_localTransform.pos.x += _offsetPreview.x; // x is a distance delta
								weap->m_localTransform.pos.y = _offsetPreview.y; // y, z are cumulative
								weap->m_localTransform.pos.z = _offsetPreview.z;
								_customTransform.pos = weap->m_localTransform.pos;
								_useCustomWeaponOffset = true;
								g_weaponOffsets->addOffset(weapname, _customTransform, _inPowerArmor ? Mode::powerArmor : Mode::normal);
								break;
							case offhand:
								_offhandOffset.rot = weap->m_localTransform.rot;
								_offhandOffset.pos = rt->transforms[boneTreeMap[offHandBone]].world.pos - rt->transforms[boneTreeMap[onHandBone]].world.pos;
								_useCustomOffHandOffset = true;
								g_weaponOffsets->addOffset(weapname, _offhandOffset, _inPowerArmor ? Mode::offHandwithPowerArmor : Mode::offHand);
								_MESSAGE("Saving offHandOffset (%f, %f, %f,) for %s: powerArmor: %d", _offhandOffset.pos.x, _offhandOffset.pos.y, _offhandOffset.pos.z, weapname, _inPowerArmor);
								break;
							case resetToDefault:
								_MESSAGE("Resetting grip to defaults for %s: powerArmor: %d", weapname, _inPowerArmor);
								_useCustomWeaponOffset = false;
								_useCustomOffHandOffset = false;
								g_weaponOffsets->deleteOffset(weapname, _inPowerArmor ? Mode::powerArmor : Mode::normal);
								g_weaponOffsets->deleteOffset(weapname, _inPowerArmor ? Mode::offHandwithPowerArmor : Mode::offHand);
								_repositionMode = weapon;
								break;
							}
							hideWands();
							_hasLetGoRepositionButton = false;
							writeOffsetJson();
						}
					}
				}
				else {
					_offhandFingerBonePos = rt->transforms[boneTreeMap[offHandBone]].world.pos;
					_offhandPos = _offhandFingerBonePos;
					bodyPos = _curPos;

					if (fc != 0) {
						for (int i = 0; i < 3; i++) {
							avgHandV[i] = 0.0f;
						}
					}
					fc = 0;

				}


				updateDown(weap, true);
			}
		}

	}

    void Skeleton::offHandToBarrel() {
        NiNode* weap = getNode("Weapon", (*g_player)->firstPersonSkeleton);
        if (!weap || !(*g_player)->actorState.IsWeaponDrawn()) {
            _offHandGripping = false;
            return;
        }

        BSFlattenedBoneTree* rt = reinterpret_cast<BSFlattenedBoneTree*>(_root);
        NiPoint3 barrelVec(0, 1, 0);
        NiPoint3 oH2Bar = c_leftHandedMode ? rt->transforms[boneTreeMap["RArm_Finger31"]].world.pos - weap->m_worldTransform.pos : rt->transforms[boneTreeMap["LArm_Finger31"]].world.pos - weap->m_worldTransform.pos;
        float len = vec3_len(oH2Bar);
        oH2Bar = weap->m_worldTransform.rot.Transpose() * vec3_norm(oH2Bar) / weap->m_worldTransform.scale;
        float dotP = vec3_dot(vec3_norm(oH2Bar), barrelVec);
        uint64_t reg = c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed;

        if (!(reg & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_gripButtonID)))) {
            _hasLetGoGripButton = true;
        }

        if (dotP > 0.955 && len > 10.0) {
            if (!c_enableGripButtonToGrap) {
                _offHandGripping = true;
            } else if (!_pipboyStatus && (reg & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_gripButtonID)))) {
                if (_offHandGripping || !_hasLetGoGripButton) {
                    return;
                }
                _offHandGripping = true;
                _hasLetGoGripButton = false;
            }
        }
    }

	/* Handle off-hand scope*/
    void Skeleton::offHandToScope() {
        NiNode* weap = getNode("Weapon", (*g_player)->firstPersonSkeleton);
        if (!weap || !(*g_player)->actorState.IsWeaponDrawn() || !c_isLookingThroughScope) {
            _zoomModeButtonHeld = false;
            return;
        }

        static BSFixedString reticleNodeName = "ReticleNode";
        NiAVObject* scopeRet = weap->GetObjectByName(&reticleNodeName);
        if (!scopeRet) {
            return;
        }

        const std::string scopeName = scopeRet->m_name;
        auto reticlePos = scopeRet->GetAsNiNode()->m_worldTransform.pos;
        auto offset = vec3_len(reticlePos - _offhandPos);
        uint64_t handInput = c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed;
        const auto handNearScope = (offset < c_scopeAdjustDistance); // hand is close to scope, enable scope specific commands

        // Zoom toggling
        if (handNearScope && !_inRepositionMode) {
            if (!_zoomModeButtonHeld && (handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_offHandActivateButtonID)))) {
                _zoomModeButtonHeld = true;
                _MESSAGE("Zoom Toggle started");
            } else if (_zoomModeButtonHeld && !(handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_offHandActivateButtonID)))) {
                _zoomModeButtonHeld = false;
                _MESSAGE("Zoom Toggle pressed; sending message to switch zoom state");
                g_messaging->Dispatch(g_pluginHandle, 16, nullptr, 0, "FO4VRBETTERSCOPES");
                if (vrhook) {
                    vrhook->StartHaptics(c_leftHandedMode ? 0 : 1, 0.1, 0.3);
                }
            }
        }

        if (c_repositionMasterMode) {
            uint64_t _pressLength = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _repositionButtonHoldStart;

            // Detect scope reposition button being held near scope. Only has to start near scope.
            if (handNearScope && !_repositionButtonHolding && (handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_repositionButtonID)))) {
                _repositionButtonHolding = true;
                _hasLetGoRepositionButton = false;
                _repositionButtonHoldStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                _MESSAGE("Reposition Button Hold start: scope %s", scopeName.c_str());
            } else if (_repositionButtonHolding && !(handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_repositionButtonID)))) {
                // Was in scope reposition mode and just released button, time to save
                _repositionButtonHolding = false;
                _hasLetGoRepositionButton = true;
                _inRepositionMode = false;
                msgData.x = 0.f;
                msgData.y = 1; // Pass save command
                msgData.z = 0.f;
                g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
                _MESSAGE("Reposition Button Hold stop: scope %s %d ms", scopeName.c_str(), _pressLength);
            }

            // Repositioning does not require hand near scope
            if (!_inRepositionMode && (handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_repositionButtonID))) && _pressLength > c_holdDelay) {
                // Enter reposition mode
                if (vrhook && c_repositionMasterMode) {
                    vrhook->StartHaptics(c_leftHandedMode ? 0 : 1, 0.1, 0.3);
                }
                _inRepositionMode = c_repositionMasterMode;
            } else if (_inRepositionMode) { // In reposition mode for better scopes
                vr::VRControllerAxis_t axis_state = !(c_pipBoyButtonArm > 0) ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0];
                if (!_repositionModeSwitched && (handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_offHandActivateButtonID)))) {
                    if (vrhook) {
                        vrhook->StartHaptics(c_leftHandedMode ? 0 : 1, 0.1, 0.3);
                    }
                    _repositionModeSwitched = true;
                    msgData.x = 0.f;
                    msgData.y = 0.f;
                    msgData.z = 0.f;
                    g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
                    _MESSAGE("Reposition Mode Reset: scope %s %d ms", scopeName.c_str(), _pressLength);
                } else if (_repositionModeSwitched && !(handInput & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(c_offHandActivateButtonID)))) {
                    _repositionModeSwitched = false;
                } else if (axis_state.x != 0 || axis_state.y != 0) { // Axis_state y is up and down, which corresponds to reticle z axis
                    msgData.x = axis_state.x;
                    msgData.y = 0.f;
                    msgData.z = axis_state.y;
                    g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
                    _MESSAGE("Moving scope reticle. input: (%f, %f)", axis_state.x, axis_state.y);
                }
            }
        }
    }

	void Skeleton::moveBack() {
		NiNode* body = _root->m_parent->GetAsNiNode();

		_root->m_localTransform.pos = body->m_worldTransform.pos - this->getPosition();
		_root->m_localTransform.pos.y -= 50.0;
	}

    void Skeleton::dampenHand(NiNode* node, bool isLeft) {
        if (!c_dampenHands) {
            return;
        }

        // Get the previous frame transform
        NiTransform& prevFrame = isLeft ? _leftHandPrevFrame : _rightHandPrevFrame;

        // Spherical interpolation between previous frame and current frame for the world rotation matrix
        Quaternion rq, rt;
        rq.fromRot(prevFrame.rot);
        rt.fromRot(node->m_worldTransform.rot);
        rq.slerp(1 - c_dampenHandsRotation, rt);
        node->m_worldTransform.rot = rq.getRot().make43();

        // Linear interpolation between the position from the previous frame to current frame
        NiPoint3 dir = _curPos - _lastPos;  // Offset the player movement from this interpolation
        NiPoint3 deltaPos = node->m_worldTransform.pos - prevFrame.pos - dir;  // Add in player velocity
        deltaPos *= c_dampenHandsTranslation;
        node->m_worldTransform.pos -= deltaPos;

        // Update the previous frame transform
        if (isLeft) {
            _leftHandPrevFrame = node->m_worldTransform;
        } else {
            _rightHandPrevFrame = node->m_worldTransform;
        }

        updateDown(node, false);
    }

	void Skeleton::dampenPipboyScreen() {
		if (!c_dampenPipboyScreen) {
			return;
		}
		NiNode* pipboyScreen = _playerNodes->ScreenNode;

		if (pipboyScreen && _pipboyStatus) {
			Quaternion rq, rt;
			// do a spherical interpolation between previous frame and current frame for the world rotation matrix
			NiTransform prevFrame = _pipboyScreenPrevFrame;
			rq.fromRot(prevFrame.rot);
			rt.fromRot(pipboyScreen->m_worldTransform.rot);
			rq.slerp(1 - c_dampenPipboyRotation, rt);
			pipboyScreen->m_worldTransform.rot = rq.getRot().make43();
			// do a linear interprolation  between the position from the previous frame to current frame
			NiPoint3 deltaPos = pipboyScreen->m_worldTransform.pos - prevFrame.pos;
			deltaPos *= c_dampenPipboyTranslation;  // just use hands dampening value for now
			pipboyScreen->m_worldTransform.pos -= deltaPos;
			_pipboyScreenPrevFrame = pipboyScreen->m_worldTransform;
			updateDown(pipboyScreen->GetAsNiNode(), false);
		}
	}

	void Skeleton::fixBackOfHand() {
		if (c_leftHandedMode) {
			NiNode* backOfHand = getNode("world_primaryWand.nif", _playerNodes->primaryUIAttachNode);

			if (backOfHand) {
				Matrix44 mat;

				mat.setEulerAngles(degrees_to_rads(180), degrees_to_rads(0), degrees_to_rads(180));
				backOfHand->m_localTransform.rot = mat.make43();
				backOfHand->m_localTransform.pos = NiPoint3(7.0, 0.0, -13.0);
			}
		}
	}



	void Skeleton::debug() {

		static std::uint64_t fc = 0;

		//Offsets::ForceGamePause(*g_menuControls);

		BSFadeNode* rn = static_cast<BSFadeNode*>(_root->m_parent);
		//_MESSAGE("newrun");

		//for (int i = 0; i < 44; i++) {
		//	if ((*g_player)->equipData->slots[i].item != nullptr) {
		//		std::string name = (*g_player)->equipData->slots[i].item->GetFullName();
		//		auto form_type = (*g_player)->equipData->slots[i].item->GetFormType();
		//		_MESSAGE("%s formType = %d", name.c_str(), form_type);
		//		if (form_type == FormType::kFormType_ARMO) {
		//			auto form = reinterpret_cast<TESObjectARMO*>((*g_player)->equipData->slots[i].item);
		//			auto bipedslot = form->bipedObject.data.parts;
		//			_MESSAGE("biped slot = %d", bipedslot);
		//		}
		//	}
		//}



		//static bool runTimer = false;
		//static auto startTime = std::chrono::high_resolution_clock::now();

		//if (fc > 400) {
		//	fc = 0;
		//	BSFixedString event("reloadStart");
		//	IAnimationGraphManagerHolder_NotifyAnimationGraph(&(*g_player)->animGraphHolder, event);
		//	runTimer = true;
		//	startTime = std::chrono::high_resolution_clock::now();
		//}

		//if (runTimer) {
		//	auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
		//	if (300 < std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()) {

		//		BSFixedString event("reloadComplete");
		//		IAnimationGraphManagerHolder_NotifyAnimationGraph(&(*g_player)->animGraphHolder, event);
		//		TESObjectREFR_UpdateAnimation((*g_player), 5.0f);

		//		runTimer = false;
		//	}
		//}

		//_MESSAGE("throwing-> %d", Actor_CanThrow(*g_player, g_equipIndex));

		//for (auto i = 0; i < rn->kGeomArray.capacity-1; ++i) {
		//	BSFadeNode::FlattenedGeometryData data = rn->kGeomArray[i];
		//	_MESSAGE("%s", data.spGeometry->m_name.c_str());
		//}

		//_playerNodes->ScopeParentNode->flags &= 0xfffffffffffffffe;
		//updateDown(dynamic_cast<NiNode*>(_playerNodes->ScopeParentNode), true);


	//	BSFixedString name("LArm_Hand");
	////	NiAVObject* node = (*g_player)->firstPersonSkeleton->GetObjectByName(&name);
	//	NiAVObject* node = _root->GetObjectByName(&name);
	//	if (!node) { return; }
	//	_MESSAGE("%d %f %f %f %f %f %f", node->flags & 0xF, node->m_localTransform.pos.x,
	//									 node->m_localTransform.pos.y,
	//									 node->m_localTransform.pos.z,
	//									 node->m_worldTransform.pos.x,
	//									 node->m_worldTransform.pos.y,
	//									 node->m_worldTransform.pos.z
	//									);

	//	for (auto i = 0; i < (*g_player)->inventoryList->items.count; i++) {
		//	_MESSAGE("%d,%d,%x,%x,%s", fc, i, (*g_player)->inventoryList->items[i].form->formID, (*g_player)->inventoryList->items[i].form->formType, (*g_player)->inventoryList->items[i].form->GetFullName());
		//}

		//if (fc < 1) {
		//	tHashSet<ObjectModMiscPair, BGSMod::Attachment::Mod*> *map = g_modAttachmentMap.GetPtr();
		//	map->Dump();
		//}

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;

		//for (auto i = 0; i < rt->numTransforms; i++) {

		//	//if (rt->transforms[i].refNode) {
		//	//	_MESSAGE("%d,%s,%d,%d", fc, rt->transforms[i].refNode->m_name.c_str(), rt->transforms[i].childPos, rt->transforms[i].parPos);
		//	//}
		//	//else {
		//	//	_MESSAGE("%d,%s,%d,%d", fc, "", rt->transforms[i].childPos, rt->transforms[i].parPos);
		//	//}
		//		_MESSAGE("%d,%d,%s", fc, i, rt->transforms[i].name.c_str());
		//}
		//
		//for (auto i = 0; i < rt->numTransforms; i++) {
		//	int pos = rt->bonePositions[i].position;
		//	if (rt->bonePositions[i].name && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
		//		_MESSAGE("%d,%d,%s", fc, pos, rt->bonePositions[i].name->data);
		//	}
		//	else {
		//		_MESSAGE("%d,%d", fc, pos);
		//	}
		//}

		//	if (rt->bonePositions[i].name && (rt->bonePositions[i].position != 0) && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
		//		int pos = rt->bonePositions[i].position;
		//		if (pos > rt->numTransforms) {
		//			continue;
		//		}
		//		_MESSAGE("%d,%s,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", fc, rt->bonePositions[i].name->data,
		//							rt->bonePositions[i].position,
		//							rt->transforms[pos].local.rot.arr[0],
		//							rt->transforms[pos].local.rot.arr[1],
		//							rt->transforms[pos].local.rot.arr[2],
		//							rt->transforms[pos].local.rot.arr[3],
		//							rt->transforms[pos].local.rot.arr[4],
		//							rt->transforms[pos].local.rot.arr[5],
		//							rt->transforms[pos].local.rot.arr[6],
		//							rt->transforms[pos].local.rot.arr[7],
		//							rt->transforms[pos].local.rot.arr[8],
		//							rt->transforms[pos].local.rot.arr[9],
		//							rt->transforms[pos].local.rot.arr[10],
		//							rt->transforms[pos].local.rot.arr[11],
		//							rt->transforms[pos].local.pos.x,
		//							rt->transforms[pos].local.pos.y,
		//							rt->transforms[pos].local.pos.z,
		//							rt->transforms[pos].world.rot.arr[0],
		//							rt->transforms[pos].world.rot.arr[1],
		//							rt->transforms[pos].world.rot.arr[2],
		//							rt->transforms[pos].world.rot.arr[3],
		//							rt->transforms[pos].world.rot.arr[4],
		//							rt->transforms[pos].world.rot.arr[5],
		//							rt->transforms[pos].world.rot.arr[6],
		//							rt->transforms[pos].world.rot.arr[7],
		//							rt->transforms[pos].world.rot.arr[8],
		//							rt->transforms[pos].world.rot.arr[9],
		//							rt->transforms[pos].world.rot.arr[10],
		//							rt->transforms[pos].world.rot.arr[11],
		//							rt->transforms[pos].world.pos.x,
		//							rt->transforms[pos].world.pos.y,
		//							rt->transforms[pos].world.pos.z
		//		);


		//	//	if(strstr(rt->bonePositions[i].name->data, "Finger")) {
		//	//		Matrix44 rot;
		//	//		rot.makeIdentity();
		//	//		rt->transforms[pos].local.rot = rot.make43();

		//	//		if (rt->transforms[pos].refNode) {
		//	//			rt->transforms[pos].refNode->m_localTransform.rot = rot.make43();
		//	//		}

		//	//		rot.makeTransformMatrix(rt->transforms[pos].local.rot, NiPoint3(0, 0, 0));

		//	//		short parent = rt->transforms[pos].parPos;
		//	//		rt->transforms[pos].world.rot = rot.multiply43Left(rt->transforms[parent].world.rot);

		//	//		if (rt->transforms[pos].refNode) {
		//	//			rt->transforms[pos].refNode->m_worldTransform.rot = rt->transforms[pos].world.rot;
		//	//		}

		//	//	}
		//	}

		//	//rt->UpdateWorldBound();
		//}

		fc++;
	}

}
