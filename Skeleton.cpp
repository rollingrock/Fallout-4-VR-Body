#include "Skeleton.h"
#include "F4VRBody.h"
#include "HandPose.h"
#include "weaponOffset.h"
#include "f4se/GameForms.h"

#include <chrono>
#include <time.h>

extern PapyrusVRAPI* g_papyrusvr;
extern OpenVRHookManagerAPI* vrhook;

using namespace std::chrono;
namespace F4VRBody
{

	float defaultCameraHeight = 120.4828;

	UInt32 KeywordPowerArmor = 0x4D8A1;
	UInt32 KeywordPowerArmorFrame = 0x15503F;

	extern vr::VRControllerState_t rightControllerState;
	extern vr::VRControllerState_t leftControllerState;

	std::map<std::string, int> boneTreeMap;
	std::vector<std::string> boneTreeVec;

	void initBoneTreeMap() {
		boneTreeMap.insert({ "COM", 0 });
		boneTreeMap.insert({ "Pelvis", 1 });
		boneTreeMap.insert({ "LLeg_Thigh", 2 });
		boneTreeMap.insert({ "LLeg_Calf", 3 });
		boneTreeMap.insert({ "LLeg_Foot", 4 });
		boneTreeMap.insert({ "LLeg_Toe1", 5 });
		boneTreeMap.insert({ "LLeg_Calf_skin", 6 });
		boneTreeMap.insert({ "LLeg_Calf_Low_skin", 7 });
		boneTreeMap.insert({ "LLeg_Calf_Armor1", 8 });
		boneTreeMap.insert({ "LLeg_Calf_Armor2", 9 });
		boneTreeMap.insert({ "LLeg_Thigh_skin", 10 });
		boneTreeMap.insert({ "LLeg_Thigh_Low_skin", 11 });
		boneTreeMap.insert({ "LLeg_Thigh_Armor1", 12 });
		boneTreeMap.insert({ "LLeg_Thigh_Armor2", 13 });
		boneTreeMap.insert({ "RLeg_Thigh", 14 });
		boneTreeMap.insert({ "RLeg_Calf", 15 });
		boneTreeMap.insert({ "RLeg_Foot", 16 });
		boneTreeMap.insert({ "RLeg_Toe1", 17 });
		boneTreeMap.insert({ "RLeg_Calf_skin", 18 });
		boneTreeMap.insert({ "RLeg_Calf_Low_skin", 19 });
		boneTreeMap.insert({ "RLeg_Calf_Armor1", 20 });
		boneTreeMap.insert({ "RLeg_Calf_Armor2", 21 });
		boneTreeMap.insert({ "RLeg_Thigh_skin", 22 });
		boneTreeMap.insert({ "RLeg_Thigh_Low_skin", 23 });
		boneTreeMap.insert({ "RLeg_Thigh_Fat_skin", 24 });
		boneTreeMap.insert({ "RLeg_Thigh_Armor", 25 });
		boneTreeMap.insert({ "Pelvis_skin", 26 });
		boneTreeMap.insert({ "RButtFat_skin", 27 });
		boneTreeMap.insert({ "LButtFat_skin", 28 });
		boneTreeMap.insert({ "Pelvis_Rear_skin", 29 });
		boneTreeMap.insert({ "Pelvis_Armor", 30 });
		boneTreeMap.insert({ "SPINE1", 31 });
		boneTreeMap.insert({ "SPINE2", 32 });
		boneTreeMap.insert({ "Chest", 33 });
		boneTreeMap.insert({ "LArm_Collarbone", 34 });
		boneTreeMap.insert({ "LArm_UpperArm", 35 });
		boneTreeMap.insert({ "LArm_ForeArm1", 36 });
		boneTreeMap.insert({ "LArm_ForeArm2", 37 });
		boneTreeMap.insert({ "LArm_ForeArm3", 38 });
		boneTreeMap.insert({ "LArm_Hand", 39 });
		boneTreeMap.insert({ "LArm_Finger11", 40 });
		boneTreeMap.insert({ "LArm_Finger12", 41 });
		boneTreeMap.insert({ "LArm_Finger13", 42 });
		boneTreeMap.insert({ "LArm_Finger21", 43 });
		boneTreeMap.insert({ "LArm_Finger22", 44 });
		boneTreeMap.insert({ "LArm_Finger23", 45 });
		boneTreeMap.insert({ "LArm_Finger31", 46 });
		boneTreeMap.insert({ "LArm_Finger32", 47 });
		boneTreeMap.insert({ "LArm_Finger33", 48 });
		boneTreeMap.insert({ "LArm_Finger41", 49 });
		boneTreeMap.insert({ "LArm_Finger42", 50 });
		boneTreeMap.insert({ "LArm_Finger43", 51 });
		boneTreeMap.insert({ "LArm_Finger51", 52 });
		boneTreeMap.insert({ "LArm_Finger52", 53 });
		boneTreeMap.insert({ "LArm_Finger53", 54 });
		boneTreeMap.insert({ "WeaponLeft", 55 });
		boneTreeMap.insert({ "AnimObjectL1", 56 });
		boneTreeMap.insert({ "AnimObjectL3", 57 });
		boneTreeMap.insert({ "AnimObjectL2", 58 });
		boneTreeMap.insert({ "PipboyBone", 59 });
		boneTreeMap.insert({ "LArm_ForeArm3_skin", 60 });
		boneTreeMap.insert({ "LArm_ForeArm2_skin", 61 });
		boneTreeMap.insert({ "LArm_ForeArm1_skin", 62 });
		boneTreeMap.insert({ "LArm_ForeArm_Armor", 63 });
		boneTreeMap.insert({ "LArm_UpperTwist1", 64 });
		boneTreeMap.insert({ "LArm_UpperTwist2", 65 });
		boneTreeMap.insert({ "LArm_UpperTwist2_skin", 66 });
		boneTreeMap.insert({ "LArm_UpperFat_skin", 67 });
		boneTreeMap.insert({ "LArm_UpperTwist1_skin", 68 });
		boneTreeMap.insert({ "LArm_UpperArm_skin", 69 });
		boneTreeMap.insert({ "LArm_UpperArm_Armor", 70 });
		boneTreeMap.insert({ "LArm_Collarbone_skin", 71 });
		boneTreeMap.insert({ "LArm_ShoulderFat_skin", 72 });
		boneTreeMap.insert({ "Neck", 73 });
		boneTreeMap.insert({ "Head", 74 });
		boneTreeMap.insert({ "Head_skin", 75 });
		boneTreeMap.insert({ "Face_skin", 76 });
		boneTreeMap.insert({ "Helmet_Armor", 77 });
		boneTreeMap.insert({ "Neck_skin", 78 });
		boneTreeMap.insert({ "Neck1_skin", 79 });
		boneTreeMap.insert({ "RArm_Collarbone", 80 });
		boneTreeMap.insert({ "RArm_UpperArm", 81 });
		boneTreeMap.insert({ "RArm_ForeArm1", 82 });
		boneTreeMap.insert({ "RArm_ForeArm2", 83 });
		boneTreeMap.insert({ "RArm_ForeArm3", 84 });
		boneTreeMap.insert({ "RArm_Hand", 85 });
		boneTreeMap.insert({ "RArm_Finger11", 86 });
		boneTreeMap.insert({ "RArm_Finger12", 87 });
		boneTreeMap.insert({ "RArm_Finger13", 88 });
		boneTreeMap.insert({ "RArm_Finger21", 89 });
		boneTreeMap.insert({ "RArm_Finger22", 90 });
		boneTreeMap.insert({ "RArm_Finger23", 91 });
		boneTreeMap.insert({ "RArm_Finger31", 92 });
		boneTreeMap.insert({ "RArm_Finger32", 93 });
		boneTreeMap.insert({ "RArm_Finger33", 94 });
		boneTreeMap.insert({ "RArm_Finger41", 95 });
		boneTreeMap.insert({ "RArm_Finger42", 96 });
		boneTreeMap.insert({ "RArm_Finger43", 97 });
		boneTreeMap.insert({ "RArm_Finger51", 98 });
		boneTreeMap.insert({ "RArm_Finger52", 99 });
		boneTreeMap.insert({ "RArm_Finger53", 100 });
		boneTreeMap.insert({ "WEAPON", 101 });
		boneTreeMap.insert({ "AnimObjectR1", 102 });
		boneTreeMap.insert({ "AnimObjectR2", 103 });
		boneTreeMap.insert({ "AnimObjectR3", 104 });
		boneTreeMap.insert({ "RArm_ForeArm3_skin", 105 });
		boneTreeMap.insert({ "RArm_ForeArm2_skin", 106 });
		boneTreeMap.insert({ "RArm_ForeArm1_skin", 107 });
		boneTreeMap.insert({ "RArm_ForeArm_Armor", 108 });
		boneTreeMap.insert({ "RArm_UpperTwist1", 109 });
		boneTreeMap.insert({ "RArm_UpperTwist2", 110 });
		boneTreeMap.insert({ "RArm_UpperTwist2_skin", 111 });
		boneTreeMap.insert({ "RArm_UpperFat_skin", 112 });
		boneTreeMap.insert({ "RArm_UpperTwist1_skin", 113 });
		boneTreeMap.insert({ "RArm_UpperArm_skin", 114 });
		boneTreeMap.insert({ "RArm_UpperArm_Armor", 115 });
		boneTreeMap.insert({ "RArm_Collarbone_skin", 116 });
		boneTreeMap.insert({ "RArm_ShoulderFat_skin", 117 });
		boneTreeMap.insert({ "L_RibHelper", 118 });
		boneTreeMap.insert({ "R_RibHelper", 119 });
		boneTreeMap.insert({ "Chest_skin", 120 });
		boneTreeMap.insert({ "LBreast_skin", 121 });
		boneTreeMap.insert({ "RBreast_skin", 122 });
		boneTreeMap.insert({ "Chest_Rear_Skin", 123 });
		boneTreeMap.insert({ "Chest_Upper_skin", 124 });
		boneTreeMap.insert({ "Neck_Low_skin", 125 });
		boneTreeMap.insert({ "Back_Armor", 126 });
		boneTreeMap.insert({ "Tank_Armor", 127 });
		boneTreeMap.insert({ "Wheel", 128 });
		boneTreeMap.insert({ "ProjectileNode", 129 });
		boneTreeMap.insert({ "Pauldron_Armor", 130 });
		boneTreeMap.insert({ "L_Pauldron", 131 });
		boneTreeMap.insert({ "R_Pauldron", 132 });
		boneTreeMap.insert({ "Spine2_skin", 133 });
		boneTreeMap.insert({ "UpperBelly_skin", 134 });
		boneTreeMap.insert({ "Spine2_Rear_skin", 135 });
		boneTreeMap.insert({ "Spine1_skin", 136 });
		boneTreeMap.insert({ "Belly_skin", 137 });
		boneTreeMap.insert({ "Spine1_Rear_skin", 138 });
		boneTreeMap.insert({ "Camera", 139 });
		boneTreeMap.insert({ "Camera Control", 140 });
		boneTreeMap.insert({ "AnimObjectA", 141 });
		boneTreeMap.insert({ "AnimObjectB", 142 });
		boneTreeMap.insert({ "CamTargetParent", 143 });
		boneTreeMap.insert({ "CamTarget", 144 });
		boneTreeMap.insert({ "OpenArmor", 145 });

		boneTreeVec.push_back("COM");
		boneTreeVec.push_back("Pelvis");
		boneTreeVec.push_back("LLeg_Thigh");
		boneTreeVec.push_back("LLeg_Calf");
		boneTreeVec.push_back("LLeg_Foot");
		boneTreeVec.push_back("LLeg_Toe1");
		boneTreeVec.push_back("LLeg_Calf_skin");
		boneTreeVec.push_back("LLeg_Calf_Low_skin");
		boneTreeVec.push_back("LLeg_Calf_Armor1");
		boneTreeVec.push_back("LLeg_Calf_Armor2");
		boneTreeVec.push_back("LLeg_Thigh_skin");
		boneTreeVec.push_back("LLeg_Thigh_Low_skin");
		boneTreeVec.push_back("LLeg_Thigh_Armor1");
		boneTreeVec.push_back("LLeg_Thigh_Armor2");
		boneTreeVec.push_back("RLeg_Thigh");
		boneTreeVec.push_back("RLeg_Calf");
		boneTreeVec.push_back("RLeg_Foot");
		boneTreeVec.push_back("RLeg_Toe1");
		boneTreeVec.push_back("RLeg_Calf_skin");
		boneTreeVec.push_back("RLeg_Calf_Low_skin");
		boneTreeVec.push_back("RLeg_Calf_Armor1");
		boneTreeVec.push_back("RLeg_Calf_Armor2");
		boneTreeVec.push_back("RLeg_Thigh_skin");
		boneTreeVec.push_back("RLeg_Thigh_Low_skin");
		boneTreeVec.push_back("RLeg_Thigh_Fat_skin");
		boneTreeVec.push_back("RLeg_Thigh_Armor");
		boneTreeVec.push_back("Pelvis_skin");
		boneTreeVec.push_back("RButtFat_skin");
		boneTreeVec.push_back("LButtFat_skin");
		boneTreeVec.push_back("Pelvis_Rear_skin");
		boneTreeVec.push_back("Pelvis_Armor");
		boneTreeVec.push_back("SPINE1");
		boneTreeVec.push_back("SPINE2");
		boneTreeVec.push_back("Chest");
		boneTreeVec.push_back("LArm_Collarbone");
		boneTreeVec.push_back("LArm_UpperArm");
		boneTreeVec.push_back("LArm_ForeArm1");
		boneTreeVec.push_back("LArm_ForeArm2");
		boneTreeVec.push_back("LArm_ForeArm3");
		boneTreeVec.push_back("LArm_Hand");
		boneTreeVec.push_back("LArm_Finger11");
		boneTreeVec.push_back("LArm_Finger12");
		boneTreeVec.push_back("LArm_Finger13");
		boneTreeVec.push_back("LArm_Finger21");
		boneTreeVec.push_back("LArm_Finger22");
		boneTreeVec.push_back("LArm_Finger23");
		boneTreeVec.push_back("LArm_Finger31");
		boneTreeVec.push_back("LArm_Finger32");
		boneTreeVec.push_back("LArm_Finger33");
		boneTreeVec.push_back("LArm_Finger41");
		boneTreeVec.push_back("LArm_Finger42");
		boneTreeVec.push_back("LArm_Finger43");
		boneTreeVec.push_back("LArm_Finger51");
		boneTreeVec.push_back("LArm_Finger52");
		boneTreeVec.push_back("LArm_Finger53");
		boneTreeVec.push_back("WeaponLeft");
		boneTreeVec.push_back("AnimObjectL1");
		boneTreeVec.push_back("AnimObjectL3");
		boneTreeVec.push_back("AnimObjectL2");
		boneTreeVec.push_back("PipboyBone");
		boneTreeVec.push_back("LArm_ForeArm3_skin");
		boneTreeVec.push_back("LArm_ForeArm2_skin");
		boneTreeVec.push_back("LArm_ForeArm1_skin");
		boneTreeVec.push_back("LArm_ForeArm_Armor");
		boneTreeVec.push_back("LArm_UpperTwist1");
		boneTreeVec.push_back("LArm_UpperTwist2");
		boneTreeVec.push_back("LArm_UpperTwist2_skin");
		boneTreeVec.push_back("LArm_UpperFat_skin");
		boneTreeVec.push_back("LArm_UpperTwist1_skin");
		boneTreeVec.push_back("LArm_UpperArm_skin");
		boneTreeVec.push_back("LArm_UpperArm_Armor");
		boneTreeVec.push_back("LArm_Collarbone_skin");
		boneTreeVec.push_back("LArm_ShoulderFat_skin");
		boneTreeVec.push_back("Neck");
		boneTreeVec.push_back("Head");
		boneTreeVec.push_back("Head_skin");
		boneTreeVec.push_back("Face_skin");
		boneTreeVec.push_back("Helmet_Armor");
		boneTreeVec.push_back("Neck_skin");
		boneTreeVec.push_back("Neck1_skin");
		boneTreeVec.push_back("RArm_Collarbone");
		boneTreeVec.push_back("RArm_UpperArm");
		boneTreeVec.push_back("RArm_ForeArm1");
		boneTreeVec.push_back("RArm_ForeArm2");
		boneTreeVec.push_back("RArm_ForeArm3");
		boneTreeVec.push_back("RArm_Hand");
		boneTreeVec.push_back("RArm_Finger11");
		boneTreeVec.push_back("RArm_Finger12");
		boneTreeVec.push_back("RArm_Finger13");
		boneTreeVec.push_back("RArm_Finger21");
		boneTreeVec.push_back("RArm_Finger22");
		boneTreeVec.push_back("RArm_Finger23");
		boneTreeVec.push_back("RArm_Finger31");
		boneTreeVec.push_back("RArm_Finger32");
		boneTreeVec.push_back("RArm_Finger33");
		boneTreeVec.push_back("RArm_Finger41");
		boneTreeVec.push_back("RArm_Finger42");
		boneTreeVec.push_back("RArm_Finger43");
		boneTreeVec.push_back("RArm_Finger51");
		boneTreeVec.push_back("RArm_Finger52");
		boneTreeVec.push_back("RArm_Finger53");
		boneTreeVec.push_back("WEAPON");
		boneTreeVec.push_back("AnimObjectR1");
		boneTreeVec.push_back("AnimObjectR2");
		boneTreeVec.push_back("AnimObjectR3");
		boneTreeVec.push_back("RArm_ForeArm3_skin");
		boneTreeVec.push_back("RArm_ForeArm2_skin");
		boneTreeVec.push_back("RArm_ForeArm1_skin");
		boneTreeVec.push_back("RArm_ForeArm_Armor");
		boneTreeVec.push_back("RArm_UpperTwist1");
		boneTreeVec.push_back("RArm_UpperTwist2");
		boneTreeVec.push_back("RArm_UpperTwist2_skin");
		boneTreeVec.push_back("RArm_UpperFat_skin");
		boneTreeVec.push_back("RArm_UpperTwist1_skin");
		boneTreeVec.push_back("RArm_UpperArm_skin");
		boneTreeVec.push_back("RArm_UpperArm_Armor");
		boneTreeVec.push_back("RArm_Collarbone_skin");
		boneTreeVec.push_back("RArm_ShoulderFat_skin");
		boneTreeVec.push_back("L_RibHelper");
		boneTreeVec.push_back("R_RibHelper");
		boneTreeVec.push_back("Chest_skin");
		boneTreeVec.push_back("LBreast_skin");
		boneTreeVec.push_back("RBreast_skin");
		boneTreeVec.push_back("Chest_Rear_Skin");
		boneTreeVec.push_back("Chest_Upper_skin");
		boneTreeVec.push_back("Neck_Low_skin");
		boneTreeVec.push_back("Back_Armor");
		boneTreeVec.push_back("Tank_Armor");
		boneTreeVec.push_back("Wheel");
		boneTreeVec.push_back("ProjectileNode");
		boneTreeVec.push_back("Pauldron_Armor");
		boneTreeVec.push_back("L_Pauldron");
		boneTreeVec.push_back("R_Pauldron");
		boneTreeVec.push_back("Spine2_skin");
		boneTreeVec.push_back("UpperBelly_skin");
		boneTreeVec.push_back("Spine2_Rear_skin");
		boneTreeVec.push_back("Spine1_skin");
		boneTreeVec.push_back("Belly_skin");
		boneTreeVec.push_back("Spine1_Rear_skin");
		boneTreeVec.push_back("Camera");
		boneTreeVec.push_back("Camera Control");
		boneTreeVec.push_back("AnimObjectA");
		boneTreeVec.push_back("AnimObjectB");
		boneTreeVec.push_back("CamTargetParent");
		boneTreeVec.push_back("CamTarget");
		boneTreeVec.push_back("OpenArmor");
		boneTreeVec.push_back("NULL");
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
		_MESSAGE("%s%s : children = %d : %2f %2f %2f", padding.c_str(), child->m_name.c_str(), child->m_children.m_emptyRunStart,
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
		_MESSAGE("%s : children = %d : worldbound %f %f %f %f", nde->m_name.c_str(), nde->m_children.m_emptyRunStart,
			nde->m_worldBound.m_kCenter.x, nde->m_worldBound.m_kCenter.y, nde->m_worldBound.m_kCenter.z, nde->m_worldBound.m_fRadius);

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
		Matrix44* result = new Matrix44();
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

		Matrix44::matrixMultiply(local, result, &mat);

		for (auto i = 0; i < 3; i++) {
			for (auto j = 0; j < 3; j++) {
				nde->m_worldTransform.rot.data[i][j] = result->data[i][j];
			}
		}

		nde->m_worldTransform.pos.x = result->data[3][0];
		nde->m_worldTransform.pos.y = result->data[3][1];
		nde->m_worldTransform.pos.z = result->data[3][2];
//		printMatrix(result);

		delete result;
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
		NiAVObject::NiUpdateData* ud = nullptr;

		if (!nde) {
			return;
		}

		if (updateSelf) {
//			nde->UpdateWorldData(ud);
			updateTransforms(nde);
		}

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				this->updateDown(nextNode, true);
			}
			auto triNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsBSTriShape() : nullptr;
			if (triNode) {
				updateTransforms((NiNode*)triNode);
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

		if (!strcmp(toNode->m_name.c_str(), fromNode->m_name.c_str())) {
			return;
		}

		for (auto i = 0; i < fromNode->m_children.m_emptyRunStart; ++i) {
			auto nextNode = fromNode->m_children.m_data[i] ? fromNode->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				this->updateDownTo(toNode, nextNode, true);
			}
		}
	}

	void Skeleton::updateUpTo(NiNode* toNode, NiNode* fromNode, bool updateTarget) {

		if (!toNode || !fromNode) {
			return;
		}

		NiAVObject::NiUpdateData* ud = nullptr;


		if (!strcmp(toNode->m_name.c_str(), fromNode->m_name.c_str())) {
			if (updateTarget) {
				fromNode->UpdateWorldData(ud);
			}
			return;
		}

		fromNode->UpdateWorldData(ud);
		NiNode* parent = fromNode->m_parent ? fromNode->m_parent->GetAsNiNode() : 0;
		if (!parent) {
			return;
		}
		updateUpTo(toNode, parent, true);
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

		if (!c_selfieMode) {
			return;
		}

		if (!_root) {
			return;
		}

		float z = _root->m_localTransform.pos.z;
		NiNode* body = _root->m_parent->GetAsNiNode();

		NiPoint3 back = vec3_norm(NiPoint3(-1*_forwardDir.x, -1*_forwardDir.y, 0));
		NiPoint3 bodydir = NiPoint3(0, 1, 0);

		Matrix44 mat;
		mat = 0.0f;
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

		if (!strcmp(nodeName, nde->m_name.c_str())) {
			return nde;
		}

		NiNode* ret = nullptr;

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				ret = this->getNode(nodeName, nextNode);
				if (ret) {
					return ret;
				}
			}
		}

		return nullptr;
	}

	void Skeleton::setupHead(NiNode* headNode, bool hideHead) {

		if (!headNode) {
			return;
		}

		if (hideHead) {
			headNode->m_localTransform.scale = 0.0000001;
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

		if (!c_selfieMode) {
			headNode->m_localTransform.pos.y = -4.0;
		}
		else {
			// always show the head in selfie mode
			headNode->m_localTransform.scale = 1.0;
		}


		NiAVObject::NiUpdateData* ud = nullptr;

		headNode->UpdateWorldData(ud);
	}

	void Skeleton::insertSaveState(std::string name, NiNode* node) {
		NiNode* fn = getNode(name.c_str(), node);

		if (!fn) {
			_MESSAGE("cannot find node %s", name.c_str());
			return;
		}
		if (savedStates.find(fn->m_name.c_str()) == savedStates.end()) {
			savedStates.insert({ std::string(fn->m_name.c_str()), fn->m_localTransform });
		}
		else {
			savedStates[name] = node->m_localTransform;
		}
		_MESSAGE("inserted %s", name.c_str());
	}

	void Skeleton::saveStatesTree(NiNode* node) {

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

		//for (auto i = 0; i < node->m_children.m_emptyRunStart; ++i) {
		//	auto nextNode = node->m_children.m_data[i] ? node->m_children.m_data[i]->GetAsNiNode() : nullptr;
		//	if (nextNode) {
		//		this->saveStatesTree(nextNode);
		//	}
		//}
	}

	void Skeleton::restoreLocals(NiNode* node) {

		if (!node || node->m_name == nullptr) {
			_MESSAGE("cannot restore locals");
			return;
		}

		std::string name(node->m_name.c_str());

		if (savedStates.find(name) == savedStates.end()) {
	//		_MESSAGE("CANNOT FIND NAME %s", name.c_str());
		}
		else {
			node->m_localTransform = savedStates[name];
	//		_MESSAGE("changed %s", name.c_str());
		}

		for (auto i = 0; i < node->m_children.m_emptyRunStart; ++i) {
			auto nextNode = node->m_children.m_data[i] ? node->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				this->restoreLocals(nextNode);
			}
		}


		NiNode* room = _playerNodes->playerworldnode;

		Matrix44 rot;
		rot.makeIdentity();

		room->m_localTransform.rot = rot.make43();
	}

	void Skeleton::setNodes() {
		QueryPerformanceFrequency(&freqCounter);
		QueryPerformanceCounter(&timer);

		std::srand(time(NULL));

		_offHandGripping = false;
		_hasLetGoGripButton = false;

		_prevSpeed = 0.0;

		_playerNodes = (PlayerNodes*)((char*)(*g_player) + 0x6E0);

		if (!_playerNodes) {
			_MESSAGE("player nodes not set");
			return;
		}

		_curPos = _playerNodes->UprightHmdNode->m_worldTransform.pos;

		setCommonNode();

		_rightHand = getNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		_leftHand  = getNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		_spine = this->getNode("SPINE2", _root);
		_chest = this->getNode("Chest", _root);

		_MESSAGE("common node = %016I64X", _common);
		_MESSAGE("righthand node = %016I64X", _rightHand);
		_MESSAGE("lefthand node = %016I64X", _leftHand);

		_weaponEquipped = false;

		// Setup Arms
		BSFixedString rCollar("RArm_Collarbone");
		BSFixedString rUpper("RArm_UpperArm");
		BSFixedString rUpper1("RArm_UpperTwist1");
		BSFixedString rForearm1("RArm_ForeArm1");
		BSFixedString rForearm2("RArm_ForeArm2");
		BSFixedString rForearm3("RArm_ForeArm3");
		BSFixedString rHand("RArm_Hand");
		BSFixedString lCollar("LArm_Collarbone");
		BSFixedString lUpper("LArm_UpperArm");
		BSFixedString lUpper1("LArm_UpperTwist1");
		BSFixedString lForearm1("LArm_ForeArm1");
		BSFixedString lForearm2("LArm_ForeArm2");
		BSFixedString lForearm3("LArm_ForeArm3");
		BSFixedString lHand("LArm_Hand");

		_MESSAGE("set arm nodes");

		rightArm.shoulder  = _common->GetObjectByName(&rCollar);
		rightArm.upper     = _common->GetObjectByName(&rUpper);
		rightArm.upperT1   = _common->GetObjectByName(&rUpper1);
		rightArm.forearm1  = _common->GetObjectByName(&rForearm1);
		rightArm.forearm2  = _common->GetObjectByName(&rForearm2);
		rightArm.forearm3  = _common->GetObjectByName(&rForearm3);
		rightArm.hand      = _common->GetObjectByName(&rHand);

		leftArm.shoulder   = _common->GetObjectByName(&lCollar);
		leftArm.upper      = _common->GetObjectByName(&lUpper);
		leftArm.upperT1    = _common->GetObjectByName(&lUpper1);
		leftArm.forearm1   = _common->GetObjectByName(&lForearm1);
		leftArm.forearm2   = _common->GetObjectByName(&lForearm2);
		leftArm.forearm3   = _common->GetObjectByName(&lForearm3);
		leftArm.hand       = _common->GetObjectByName(&lHand);

		_MESSAGE("finished set arm nodes");

		//BSSubIndexTriShape* handMesh = nullptr;
		//for (int i = 0; i < _root->m_parent->m_children.m_emptyRunStart; i++) {
		//	BSSubIndexTriShape* node = _root->m_parent->m_children.m_data[i]->GetAsBSSubIndexTriShape();
		//	if (node) {
		//		handMesh = node;
		//		break;
		//	}
		//}

		//if (handMesh) {
		//	BSSkin::Instance* handSkin = (BSSkin::Instance*)(handMesh->numIndices);
		//	_boneTransforms = (HandMeshBoneTransforms*)(handSkin->worldTransforms.entries);
		//	_MESSAGE("bonetransforms   = %016I64X", _boneTransforms);
		//}
		//else {
		//	_boneTransforms = nullptr;
		//	_MESSAGE("can't get bonetransforms for the hand");
		//}

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

		initBoneTreeMap();
	}

	void Skeleton::positionDiff() {
		NiPoint3 firstpos = _playerNodes->HmdNode->m_worldTransform.pos;
		NiPoint3 skellypos = _root->m_worldTransform.pos;

		_MESSAGE("difference = %f %f %f", (firstpos.x - skellypos.x), (firstpos.y - skellypos.y), (firstpos.z - skellypos.z));

	}

	NiPoint3 Skeleton::getPosition() {

			_curPos = _playerNodes->UprightHmdNode->m_worldTransform.pos;

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

		hmdToLeft = vec3_norm(hmdToLeft);
		hmdToRight = vec3_norm(hmdToRight);

		NiPoint3 sum = hmdToRight + hmdToLeft;

		NiPoint3 forwardDir = vec3_norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * vec3_norm(sum));  // rotate sum to local hmd space to get the proper angle
		NiPoint3 hmdForwardDir = vec3_norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * _playerNodes->HmdNode->m_localTransform.pos);

		float anglePrime = atan2f(forwardDir.x, forwardDir.y);

		float angleSec = atan2f(forwardDir.x, forwardDir.z);

		float angleFinal;

		sum = vec3_norm(sum);

		float pitchDiff = atan2f(hmdForwardDir.y, hmdForwardDir.z) - atan2f(forwardDir.z, forwardDir.y);

		if (fabs(pitchDiff) > degrees_to_rads(80.0f)){
			angleFinal = angleSec;
		}
		else {
			angleFinal = anglePrime;
		}

		return std::clamp(-angleFinal * weight, degrees_to_rads(-90.0f), degrees_to_rads(90.0f));
	}

	float Skeleton::getNeckPitch() {

		NiPoint3 lookDir = vec3_norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * _playerNodes->HmdNode->m_localTransform.pos);
		float pitchAngle = atan2f(lookDir.y, lookDir.z);

		return pitchAngle;
	}

	float Skeleton::getBodyPitch() {
		float basePitch = 105.3;
		float weight = 0.1;

		float offset = _inPowerArmor ? c_PACameraHeight + c_cameraHeight : c_cameraHeight;
		float curHeight = c_playerHeight;
		float heightCalc = abs((curHeight - (_playerNodes->UprightHmdNode->m_localTransform.pos.z - offset)) / curHeight);

		float angle = heightCalc * (basePitch + weight * rads_to_degrees(getNeckPitch()));

		return degrees_to_rads(angle);
	}

	void Skeleton::setUnderHMD() {

		detectInPowerArmor();

		if (c_disableSmoothMovement) {
			_playerNodes->playerworldnode->m_localTransform.pos.z = _inPowerArmor ? (c_PACameraHeight + c_cameraHeight) : c_cameraHeight;
		}
		else {
			_playerNodes->playerworldnode->m_localTransform.pos.z += _inPowerArmor ? (c_PACameraHeight + c_cameraHeight) : c_cameraHeight;
		}

		updateDown(_playerNodes->playerworldnode, true);

		Matrix44 mat;
		mat = 0.0;

//		float y    = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][1];  // Middle column is y vector.   Grab just x and y portions and make a unit vector.    This can be used to rotate body to always be orientated with the hmd.
	//	float x    = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][0];  //  Later will use this vector as the basis for the rest of the IK
		float z = _root->m_localTransform.pos.z;

		float neckYaw   = getNeckYaw();
		float neckPitch   = getNeckPitch();

		Quaternion qa;
		qa.setAngleAxis(-neckPitch, NiPoint3(-1, 0, 0));

		mat = qa.getRot();
		NiMatrix43 newRot = mat.multiply43Left(_playerNodes->HmdNode->m_localTransform.rot);

		_forwardDir = rotateXY(NiPoint3(newRot.data[1][0], newRot.data[1][1], 0), neckYaw * 0.7);
		_sidewaysRDir = NiPoint3(_forwardDir.y, -_forwardDir.x, 0);

		NiNode* body = _root->m_parent->GetAsNiNode();
		body->m_localTransform.pos *= 0.0f;
		body->m_worldTransform.pos.x = this->getPosition().x;
		body->m_worldTransform.pos.y = this->getPosition().y;
		body->m_worldTransform.pos.z += _playerNodes->playerworldnode->m_localTransform.pos.z;
	//	_MESSAGE("%2f", _playerNodes->playerworldnode->m_localTransform.pos.z);

		updateDown(_root, true);

		NiPoint3 back = vec3_norm(NiPoint3(_forwardDir.x, _forwardDir.y, 0));
		NiPoint3 bodydir = NiPoint3(0, 1, 0);

		mat.rotateVectoVec(back, bodydir);
		_root->m_localTransform.rot = mat.multiply43Left(body->m_worldTransform.rot.Transpose());
		_root->m_localTransform.pos = body->m_worldTransform.pos - getPosition();
		_root->m_localTransform.pos.z = z;
		_root->m_localTransform.scale = c_playerHeight / defaultCameraHeight;    // set scale based off specified user height

		body->m_worldBound.m_kCenter = this->getPosition();
	}

	void Skeleton::setBodyPosture() {
		float neckPitch = getNeckPitch();
		float bodyPitch = getBodyPitch();

		NiNode* camera = (*g_playerCamera)->cameraNode;
		NiNode* com = getNode("COM", _root);
		NiNode* neck = getNode("Neck", _root);
		NiNode* spine = getNode("SPINE1", _root);

		_leftKneePos = getNode("LLeg_Calf", com)->m_worldTransform.pos;
		_rightKneePos = getNode("RLeg_Calf", com)->m_worldTransform.pos;

		float comZ = com->m_localTransform.pos.z;
		com->m_localTransform.pos.x = 0.0;
		com->m_localTransform.pos.y = 0.0;

		float z_adjust = c_playerOffset_up - cosf(neckPitch) * (5.0 * _root->m_localTransform.scale);
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
		float offsetFwd;
		offsetFwd = _inPowerArmor ? -c_powerArmor_forward : c_playerOffset_forward;
		com->m_localTransform.pos.y += newPos.y + offsetFwd;
		com->m_localTransform.pos.z = newPos.z;
		com->m_localTransform.pos.z -= _inPowerArmor ? c_powerArmor_up + c_PACameraHeight : 0.0f;

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

		float delta = getNode("LArm_Collarbone", _root)->m_worldTransform.pos.z - _root->m_worldTransform.pos.z;

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

		// move feet closer togther
		NiPoint3 leftToRight = _inPowerArmor ?  (rFoot->m_worldTransform.pos - lFoot->m_worldTransform.pos) * -0.15 : (rFoot->m_worldTransform.pos - lFoot->m_worldTransform.pos) * 0.3;
		lFoot->m_worldTransform.pos += leftToRight;
		rFoot->m_worldTransform.pos -= leftToRight;

		// want to calculate direction vector first.     Will only concern with x-y vector to start.
		NiPoint3 lastPos = _lastPos;
		NiPoint3 curPos = _curPos;
		curPos.z = 0;
		lastPos.z = 0;

		NiPoint3 dir = curPos - lastPos;

		double curSpeed;

		curSpeed = std::clamp((abs(vec3_len(dir)) / _frameTime), 0.0, 350.0);
		if (_prevSpeed > 20.0) {
			curSpeed = (curSpeed + _prevSpeed) / 2;
		}
		_prevSpeed = curSpeed;

		double stepTime = std::clamp(cos(curSpeed / 140.0), 0.28, 0.50);
		dir = vec3_norm(dir);

		static float spineAngle = 0.0;

		// setup current walking state based on velocity and previous state
		if (!c_jumping) {
			switch (_walkingState) {
			case 0: {
				if (curSpeed >= 35.0) {
					_walkingState = 1;          // start walking
					_footStepping = std::rand() % 2 + 1;    // pick a random foot to take a step
					_stepDir = dir;
					_stepTimeinStep = stepTime;
					delayFrame = 2;

					if (_footStepping == 1) {
						_rightFootTarget = rFoot->m_worldTransform.pos + _stepDir * (curSpeed * stepTime * 1.5);
						_rightFootStart = rFoot->m_worldTransform.pos;
						_leftFootTarget = lFoot->m_worldTransform.pos;
						_leftFootStart = lFoot->m_worldTransform.pos;
						_leftFootPos = _leftFootStart;
						_rightFootPos = _rightFootStart;
					}
					else {
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
					_walkingState = 2;          // begin process to stop walking
					_currentStepTime = 0.0;
				}
				break;
			}
			case 2: {
				if (curSpeed >= 20.0) {
					_walkingState = 1;         // resume walking
					_currentStepTime = 0.0;
				}
				break;
			}
			default: {
				_walkingState = 0;
				break;
			}
			}
		}
		else {
			_walkingState = 0;
		}

		if (_walkingState == 0) {
			// we're standing still so just set foot positions accordingly.
			_leftFootPos = lFoot->m_worldTransform.pos;
			_rightFootPos = rFoot->m_worldTransform.pos;
			return;
		}
		else if (_walkingState == 1) {
			NiPoint3 dirOffset = dir - _stepDir;
			float dot = vec3_dot(dir, _stepDir);
			double scale = (std::min)(curSpeed * stepTime * 1.5, 140.0);
			dirOffset = dirOffset * scale;

			int sign = 1;

			_currentStepTime += _frameTime;

			double frameStep = _frameTime / (_stepTimeinStep);
			double interp = std::clamp(frameStep * (_currentStepTime / _frameTime), 0.0, 1.0);

			if (_footStepping == 1) {
				sign = -1;
				if (dot < 0.9) {
					if (!delayFrame) {
						_rightFootTarget += dirOffset;
						_stepDir = dir;
						delayFrame = 2;
					}
					else {
						delayFrame--;
					}
				}
				else {
					delayFrame = delayFrame == 2 ? delayFrame : delayFrame + 1;
				}
				_rightFootTarget.z = _root->m_worldTransform.pos.z;
				_rightFootStart.z = _root->m_worldTransform.pos.z;
				_rightFootPos = _rightFootStart + ((_rightFootTarget - _rightFootStart) * interp);
				double stepAmount = std::clamp(vec3_len(_rightFootTarget - _rightFootStart) / 150.0, 0.0, 1.0);
				double stepHeight = (std::max)(stepAmount * 9.0, 1.0);
				float up = sinf(interp * PI) * stepHeight;
				_rightFootPos.z += up;
			}
			else {
				if (dot < 0.9) {
					if (!delayFrame) {
						_leftFootTarget += dirOffset;
						_stepDir = dir;
						delayFrame = 2;
					}
					else {
						delayFrame--;
					}
				}
				else {
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
				//_MESSAGE("%2f %2f", curSpeed, stepTime);

				if (_footStepping == 1) {
					_footStepping = 2;
					_leftFootTarget = lFoot->m_worldTransform.pos + _stepDir * scale;
					_leftFootStart = _leftFootPos;
				}
				else {
					_footStepping = 1;
					_rightFootTarget = rFoot->m_worldTransform.pos + _stepDir * scale;
					_rightFootStart = _rightFootPos;
				}
			}
			return;
		}
		else if (_walkingState == 2) {
			_leftFootPos = lFoot->m_worldTransform.pos;
			_rightFootPos = rFoot->m_worldTransform.pos;
			_walkingState = 0;
			return;

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
		NiPoint3 hipDir = hipNode->m_worldTransform.rot * NiPoint3(0, 1, 0);
		NiPoint3 xDir = vec3_norm(footToHip);
		NiPoint3 yDir = vec3_norm(hipDir - xDir * vec3_dot(hipDir, xDir));

		float thighLenOrig = vec3_len(kneeNode->m_localTransform.pos);
		float calfLenOrig = vec3_len(footNode->m_localTransform.pos);
		float thighLen = thighLenOrig;
		float calfLen = calfLenOrig;

		float ftLen = vec3_len(footToHip);
		if (ftLen < 0.1) {
			ftLen = 0.1;
		}

		if (ftLen > thighLen + calfLen) {
			float diff = ftLen - thighLen - calfLen;
			float ratio = calfLen / (calfLen + thighLen);
			calfLen += ratio * diff + 0.1;
			thighLen += (1.0 - ratio) * diff + 0.1;
		}

		// Use the law of cosines to calculate the angle the calf must bend to reach the knee position
		// In cases where this is impossible (foot too close to thigh), then set calfLen = thighLen so
		// there is always a solution
		float footAngle = acosf((calfLen * calfLen + ftLen * ftLen - thighLen * thighLen) / (2 * calfLen * ftLen));
		if (isnan(footAngle) || isinf(footAngle)) {
			calfLen = thighLen = (thighLenOrig + calfLenOrig) / 2.0;
			footAngle = acosf((calfLen * calfLen + ftLen * ftLen - thighLen * thighLen) / (2 * calfLen * ftLen));
		}

		// Get the desired world coordinate of the knee
		float xDist = cosf(footAngle) * calfLen;
		float yDist = sinf(footAngle) * calfLen;
		kneePos = footPos + xDir * xDist + yDir * yDist;

		NiPoint3 pos = kneePos - hipNode->m_worldTransform.pos;
		NiPoint3 uLocalDir = hipNode->m_worldTransform.rot.Transpose() * vec3_norm(pos) / hipNode->m_worldTransform.scale;
		rotMat.rotateVectoVec(uLocalDir, kneeNode->m_localTransform.pos);
		hipNode->m_localTransform.rot = rotMat.multiply43Left(hipNode->m_localTransform.rot);

		if (_inPowerArmor) {
			//power armor for some reason twists the legs.  twist them back!
			Matrix44 twist;
			float angle = isLeft ? 90.0f : -90.0f;
			twist.setEulerAngles(degrees_to_rads(angle), 0, 0);
			hipNode->m_localTransform.rot = twist.multiply43Left(hipNode->m_localTransform.rot);
		}

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

		// Calculate Flr:  Cwr * Flr = footRot   ===>   Flr = Cwr' * footRot
		//LROT(nodeFoot) = Cwr.Transpose() * footRot;

		// Calculate Clp:  Cwp = Twp + Twr * (Clp * Tws) = kneePos   ===>   Clp = Twr' * (kneePos - Twp) / Tws
		//LPOS(nodeCalf) = Twr.Transpose() * (kneePos - thighPos) / WSCL(nodeThigh);
		kneeNode->m_localTransform.pos = hipWR.Transpose() * (kneePos - hipPos) / hipNode->m_worldTransform.scale;

		// Calculate Flp:  Fwp = Cwp + Cwr * (Flp * Cws) = footPos   ===>   Flp = Cwr' * (footPos - Cwp) / Cws
		//LPOS(nodeFoot) = Cwr.Transpose() * (footPos - kneePos) / WSCL(nodeCalf);
		footNode->m_localTransform.pos = calfWR.Transpose() * (footPos - kneePos) / kneeNode->m_worldTransform.scale;

		if (_inPowerArmor) {
			footNode->m_localTransform.pos *= 1.5;
			//power armor for some reason twists the legs.  twist them back!
			Matrix44 twist;

			float angle = isLeft ? 90.0f : -90.0f;
			float angle2 = isLeft ? -45.0f : 45.0f;
			float angle3 = isLeft ? 30.0f : -30.0f;
			twist.setEulerAngles(degrees_to_rads(angle2), degrees_to_rads(angle), degrees_to_rads(angle3));
			footNode->m_localTransform.rot = twist.multiply43Left(footNode->m_localTransform.rot);
		}
	}

	void Skeleton::rotateLeg(uint32_t pos, float angle) {
			BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;
			Matrix44 rot;
			rot.setEulerAngles(degrees_to_rads(angle), 0, 0);

			rt->transforms[pos].local.rot = rot.multiply43Left(rt->transforms[pos].local.rot);

			short parent = rt->transforms[pos].parPos;

			NiPoint3 p = rt->transforms[pos].local.pos;
			p = (rt->transforms[parent].world.rot * (p * rt->transforms[parent].world.scale));

			rt->transforms[pos].world.pos = rt->transforms[parent].world.pos + p;

			rot.makeTransformMatrix(rt->transforms[pos].local.rot, NiPoint3(0, 0, 0));

			rt->transforms[pos].world.rot = rot.multiply43Left(rt->transforms[parent].world.rot);

	}

	void Skeleton::fixPAArmor() {

		static bool oneTime = false;

		return;
		if (_inPowerArmor) {
			if (!oneTime) {
				oneTime = true;
				return;
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

		_legLen =  vec3_len(getNode("LLeg_Thigh", _root)->m_worldTransform.pos -  getNode("Pelvis", _root)->m_worldTransform.pos);
		_legLen += vec3_len(getNode("LLeg_Calf", _root)->m_worldTransform.pos - getNode("LLeg_Thigh", _root)->m_worldTransform.pos);
		_legLen += vec3_len(getNode("LLeg_Foot", _root)->m_worldTransform.pos - getNode("LLeg_Calf", _root)->m_worldTransform.pos);
		_legLen *= c_playerHeight / defaultCameraHeight;
	}

	void Skeleton::hideWeapon() {

		static BSFixedString nodeName("Weapon");

		NiAVObject* weapon = rightArm.hand->GetObjectByName(&nodeName);

		if (weapon != nullptr) {
			weapon->flags |= 0x1;
			weapon->m_localTransform.scale = 0.0;
			for (auto i = 0; i < weapon->GetAsNiNode()->m_children.m_emptyRunStart; ++i) {
				auto nextNode = weapon->GetAsNiNode()->m_children.m_data[i]->GetAsNiNode();
				nextNode->flags |= 0x1;
				nextNode->m_localTransform.scale = 0.0;
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

				if ((type == WeaponType::kWeaponType_One_Hand_Axe) ||
					(type == WeaponType::kWeaponType_One_Hand_Dagger) ||
					(type == WeaponType::kWeaponType_One_Hand_Mace) ||
					(type == WeaponType::kWeaponType_One_Hand_Sword) ||
					(type == WeaponType::kWeaponType_Two_Hand_Axe) ||
					(type == WeaponType::kWeaponType_Two_Hand_Sword)) {

					NiAVObject* wNode = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());

					Matrix44 rot;
					rot.setEulerAngles(degrees_to_rads(85.0f), degrees_to_rads(-70.0f), degrees_to_rads(0.0f));
					wNode->m_localTransform.rot = rot.multiply43Right(wNode->m_localTransform.rot);
					wNode->m_localTransform.pos.z += 1.5f;

					updateDown(wNode->GetAsNiNode(), true);

					(*g_player)->Update(0.0f);

				}
			}
		}
	}


		// Thanks Shizof and SmoothtMovementVR for below code
	bool HasKeyword(TESObjectARMO* armor, UInt32 keywordFormId)
	{
		if (armor)
		{
			for (UInt32 i = 0; i < armor->keywordForm.numKeywords; i++)
			{
				if (armor->keywordForm.keywords[i])
				{
					if (armor->keywordForm.keywords[i]->formID == keywordFormId)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	void Skeleton::detectInPowerArmor() {

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
							}
							else
							{
								_inPowerArmor = false;
							}
						}
					}
				}
			}
		}

	}

	void Skeleton::hideWands() {
		int i;

		for (i = 0; i < _playerNodes->primaryWandNode->m_children.m_emptyRunStart; i++) {
			if (_playerNodes->primaryWandNode->m_children.m_data[i]) {
				NiNode* n = (NiNode*)(_playerNodes->primaryWandNode->m_children.m_data[i]);
				auto right = _playerNodes->primaryWandNode->m_children.m_data[i]->GetAsBSTriShape();
				if (right) {
					NiAVObject* n1 = (NiAVObject*)right;
					n1->flags |= 0x1;
					break;
				}
				else if (!strcmp(n->m_name.c_str(), "")) {
					n->flags |= 0x1;
					NiAVObject* n1 = (NiAVObject*)n->m_children.m_data[0];
					n1->flags |= 0x1;
					break;
				}
			}
		}

		for (i = 0; i < _playerNodes->SecondaryWandNode->m_children.m_emptyRunStart; i++) {
			if (_playerNodes->SecondaryWandNode->m_children.m_data[i]) {
				NiNode* n = (NiNode*)(_playerNodes->SecondaryWandNode->m_children.m_data[i]);
				auto left = _playerNodes->SecondaryWandNode->m_children.m_data[i]->GetAsBSTriShape();
				if (left) {
					NiAVObject* n2 = (NiAVObject*)left;
					n2->flags |= 0x1;
					break;
				}
				else if (!strcmp(n->m_name.c_str(), "")) {
					n->flags |= 0x1;
					NiAVObject* n2 = (NiAVObject*)n->m_children.m_data[0];
					n2->flags |= 0x1;
					break;
				}
			}
		}
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

		return (dot < -(c_pipBoyLookAtGate) ? true : false);

	}

	void Skeleton::hidePipboy() {
		BSFixedString pipName("PipboyBone");
		NiAVObject* pipboy;

		if (!c_leftHandedPipBoy) {
			pipboy = leftArm.forearm3->GetObjectByName(&pipName);
		}
		else {
			pipboy = rightArm.forearm3->GetObjectByName(&pipName);

		}

		if (!pipboy) {
			_MESSAGE("can't find pipboyroot in hidePipBoy function");
			return;
		}

		if (!c_hidePipboy) {
			if (pipboy->m_localTransform.scale = 1.0) {
				return;
			}
			else {
				pipboy->m_localTransform.scale = 1.0;
				toggleVis(pipboy->GetAsNiNode(), false, true);
			}
		}
		else {
			if (pipboy->m_localTransform.scale = 0.0) {
				return;
			}
			else {
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
		NiAVObject* pipboy;

		if (!c_leftHandedPipBoy) {
			finger = rt->transforms[boneTreeMap["RArm_Finger23"]].world.pos;

			pipboy = getNode("PipboyRoot", leftArm.forearm3->GetAsNiNode());
		}
		else {
			finger = rt->transforms[boneTreeMap["LArm_Finger23"]].world.pos;

			pipboy = getNode("PipboyRoot", rightArm.forearm3->GetAsNiNode());

		}

		if (pipboy == nullptr) {
			return;
		}

		uint64_t reg = (c_pipBoyButtonArm > 0) ? rightControllerState.ulButtonPressed : leftControllerState.ulButtonPressed;
		if ((reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_pipBoyButtonID)) && !_stickypip) {
			if (_pipboyStatus) {
				_pipboyStatus = false;
				turnPipBoyOff();
				_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				_MESSAGE("Disabling Pipboy with button");
			}
			else if (c_pipBoyButtonMode) {
				//turning on the pip is gated by buttonmode
				_pipboyStatus = true;
				_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
				turnPipBoyOn();
				_MESSAGE("Enabling Pipboy with button");
			}
			else
				return;
			_stickypip = true;
		}
		else if (c_pipBoyButtonMode && !(reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_pipBoyButtonID))) {
			_stickypip = false;
		}
		else if (c_pipBoyButtonMode)
			return;


		if (!isLookingAtPipBoy()) {
			vr::VRControllerAxis_t axis_state = (c_pipBoyButtonArm > 0) ? rightControllerState.rAxis[0] : leftControllerState.rAxis[0];
			const auto timeElapsed = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _lastLookingAtPip;
			if (_pipboyStatus && timeElapsed > c_pipBoyOffDelay) {
				_pipboyStatus = false;
				turnPipBoyOff();
				_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				_MESSAGE("Disabling PipBoy due to inactivity for %d more than %d ms", timeElapsed, c_pipBoyOffDelay);
			}
			else if (c_pipBoyAllowMovementNotLooking && _pipboyStatus && (axis_state.x != 0 || axis_state.y != 0)) {
				turnPipBoyOff();
				_pipboyStatus = false;
				_playerNodes->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				_MESSAGE("Disabling PipBoy due to movement when not looking at pipboy. input: (%f, %f)", axis_state.x, axis_state.y);
			}
			return;
		}
		else if (_pipboyStatus)
		{
			_lastLookingAtPip = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		}

		float distance = vec3_len(finger - pipboy->m_worldTransform.pos);

		if (distance > c_pipboyDetectionRange) {
			_pipTimer = 0;
			_stickypip = false;
			return;
		}
		else {
			if (!_stickypip) {
				if (vrhook != nullptr) {
					vrhook->StartHaptics(2, 0.05, 0.3);
				}
			}
			else {
				return;
			}

			if (_pipTimer < 2) {
				_stickypip = false;
				_pipTimer++;
			}
			else {
				_stickypip = true;

				if (_pipboyStatus) {
					_pipboyStatus = false;
					turnPipBoyOff();
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

	void Skeleton::set1stPersonArm(NiNode* weapon, NiNode* offsetNode) {

		NiNode** wp = &weapon;
		NiNode** op = &offsetNode;

		update1stPersonArm(*g_player, wp, op);
	}

	void Skeleton::showHidePAHUD() {
		NiNode* wand = this->getPlayerNodes()->primaryUIAttachNode;

		BSFixedString bname = "BackOfHand";
		NiNode* node = (NiNode*)wand->GetObjectByName(&bname);

		if (node && _inPowerArmor) {
			node->m_worldTransform.pos += NiPoint3(-5.0, -7.0, 2.0);

			updateTransformsDown(node, false);
		}



		NiNode* hud = getNode("PowerArmorHelmetRoot", _playerNodes->roomnode);

		if (hud) {
			hud->m_localTransform.scale = c_showPAHUD ? 1.0 : 0.0;
			return;
		}
	}

	void Skeleton::setArms_wp(bool isLeft) {
		ArmNodes arm;

		arm = isLeft ? leftArm : rightArm;

		NiPoint3 handPos = isLeft ? _leftHand->m_worldTransform.pos : _rightHand->m_worldTransform.pos;
		NiMatrix43 handRot = isLeft ? _leftHand->m_worldTransform.rot : _rightHand->m_worldTransform.rot;

		// Detect if the 1st person hand position is invalid.  This can happen when a controller loses tracking.
		// If it is, do not handle IK and let Fallout use its normal animations for that arm instead.

		if (isnan(handPos.x) || isnan(handPos.y) || isnan(handPos.z) ||
			isinf(handPos.x) || isinf(handPos.y) || isinf(handPos.z) ||
			vec3_len(arm.upper->m_worldTransform.pos - handPos) > 200.0)
		{
			return;
		}

		NiPoint3 handSide = arm.hand->m_worldTransform.rot * NiPoint3(0, 1, 0);
		NiPoint3 handinSide = handSide * -1;

		NiPoint3 wLocalDir = arm.forearm3->m_worldTransform.rot.Transpose() * vec3_norm(handinSide);
		wLocalDir.x = 0;
		NiPoint3 forearm3Side = arm.forearm3->m_worldTransform.rot * NiPoint3(0, -1 * 1, 0);
		NiPoint3 floc = arm.forearm2->m_worldTransform.rot.Transpose() * vec3_norm(forearm3Side);
		floc.x = 0;
		float fcos = vec3_dot(vec3_norm(floc), vec3_norm(wLocalDir));
		NiPoint3 cross = vec3_cross(vec3_norm(floc), vec3_norm(wLocalDir));

		float forearmAngle = acosf(fcos);

		_MESSAGE("cross.x         %5f", cross.x);
		_MESSAGE("fcos            %5f", fcos);
		_MESSAGE("forearmangle    %5f", forearmAngle);
		_MESSAGE("forearmangle    %5f", rads_to_degrees(forearmAngle));


		return;

		float upperArmLength = vec3_len(arm.forearm1->m_worldTransform.pos - arm.upper->m_worldTransform.pos);
		float lowerArmLength = vec3_len(arm.hand->m_worldTransform.pos - arm.forearm1->m_worldTransform.pos);
		float armLength = upperArmLength + lowerArmLength;
		float armLengthScale = 0.75;

		float negLeft = isLeft ? -1 : 1;

		NiPoint3 forwardDir = _forwardDir;
		NiPoint3 sidewaysDir = _sidewaysRDir * negLeft;
		NiPoint3 upDir = NiPoint3(0, 0, 1);

		// Shoulder first.    Solve for a yaw and roll if hand position exceeds a threshold given by
		//              (shoudlerToHand dot forwardUnitVec)
		// yaw = c *  -------------------------------------  - d
		//                       armlength
		// roll is the same except upVec instead of forward

		NiPoint3 shoulderToHand = handPos - arm.upper->m_worldTransform.pos;

		float dotF = vec3_dot(shoulderToHand, forwardDir);
		float dotU = vec3_dot(shoulderToHand, upDir);
		float c = degrees_to_rads(30.0);

		float yaw  = c * (dotF / (armLength * armLengthScale)) - 0.5;
		float roll = c * (dotU / (armLength * armLengthScale)) - 0.5;
		yaw = std::clamp(yaw, 0.0f, degrees_to_rads(33.0));
		roll = std::clamp(roll, 0.0f, degrees_to_rads(33.0));


		Matrix44 rotate;
		rotate.setEulerAngles(yaw, roll, 0);

		arm.shoulder->m_localTransform.rot = rotate.multiply43Left(arm.shoulder->m_localTransform.rot);

		// now elbow
		_MESSAGE("");
		_MESSAGE("========== Frame %d ============", g_mainLoopCounter);
		_MESSAGE("armLength       %5f", armLength);
		_MESSAGE("yaw             %5f", rads_to_degrees(yaw));
		_MESSAGE("roll            %5f", rads_to_degrees(roll));
	}

	void Skeleton::setLeftHandedSticky() {
		_leftHandedSticky = !c_leftHandedMode;
	}


	void Skeleton::handleWeaponNodes() {

		if (_leftHandedSticky != c_leftHandedMode) {
			NiNode* rightWeapon = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
			NiNode* leftWeapon = _playerNodes->WeaponLeftNode;

			NiNode* rHand = getNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
			NiNode* lHand = getNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());

			if (rightWeapon && rHand && leftWeapon && lHand) {
				rHand->RemoveChild(rightWeapon);
				rHand->RemoveChild(leftWeapon);
				lHand->RemoveChild(rightWeapon);
				lHand->RemoveChild(leftWeapon);
			}
			else {
				if (c_verbose) { _MESSAGE("Cannot set up weapon nodes"); }
				_leftHandedSticky = c_leftHandedMode;
				return;
			}

			if (c_leftHandedMode) {
				rHand->AttachChild(leftWeapon, true);
				lHand->AttachChild(rightWeapon, true);
			}
			else {
				rHand->AttachChild(rightWeapon, true);
				lHand->AttachChild(leftWeapon, true);
			}
			_leftHandedSticky = c_leftHandedMode;
		}
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
		NiNode* leftWeapon = _playerNodes->WeaponLeftNode;

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
		w.data[0][0] = -0.120;
		w.data[1][0] = 0.987;
		w.data[2][0] = 0.108;
		w.data[0][1] = 0.991;
		w.data[1][1] = 0.112;
		w.data[2][1] = 0.077;
		w.data[0][2] = 0.064;
		w.data[1][2] = 0.116;
		w.data[2][2] = -0.991;

		if (!isLeft) {
			_weapSave = rightWeapon->m_localTransform;
		}
		weaponNode->m_localTransform.rot = w.make43();

		if (c_leftHandedMode) {
			w.setEulerAngles(degrees_to_rads(180), 0, 0);
			weaponNode->m_localTransform.rot = w.multiply43Right(weaponNode->m_localTransform.rot);
		}

		if (handleLeftMode) {
			if (isLeft) {
				w.setEulerAngles(degrees_to_rads(0), degrees_to_rads(45), degrees_to_rads(0));
			}
			else {
				w.setEulerAngles(degrees_to_rads(0), degrees_to_rads(-45), degrees_to_rads(0));
			}
			weaponNode->m_localTransform.rot = w.multiply43Right(weaponNode->m_localTransform.rot);
		}

		if (c_leftHandedMode) {
			weaponNode->m_localTransform.pos = isLeft ? NiPoint3(6.389, -2.099, 3.133) : NiPoint3(0, 0, 0);
		}
		else {
			weaponNode->m_localTransform.pos = isLeft ? NiPoint3(0, 0, 0) : NiPoint3(6.389, -2.099, -3.133);
		}


		weaponNode->IncRef();
		set1stPersonArm(weaponNode, offsetNode);


		NiPoint3 handPos;
		NiMatrix43 handRot;

		handPos = isLeft ? _leftHand->m_worldTransform.pos : _rightHand->m_worldTransform.pos;
		handRot = isLeft ? _leftHand->m_worldTransform.rot : _rightHand->m_worldTransform.rot;

		// Detect if the 1st person hand position is invalid.  This can happen when a controller loses tracking.
		// If it is, do not handle IK and let Fallout use its normal animations for that arm instead.

		if (isnan(handPos.x) || isnan(handPos.y) || isnan(handPos.z) ||
			isinf(handPos.x) || isinf(handPos.y) || isinf(handPos.z) ||
			vec3_len(arm.upper->m_worldTransform.pos - handPos) > 200.0)
		{
			return;
		}


		double adjustedArmLength = c_armLength / 36.74;

		// Shoulder IK is done in a very simple way

		NiPoint3 shoulderToHand = handPos - arm.upper->m_worldTransform.pos;
		float armLength = c_armLength;
		float adjustAmount = (std::clamp)(vec3_len(shoulderToHand) - armLength * 0.5f, 0.0f, armLength * 0.75f) / (armLength * 0.75f);
		NiPoint3 shoulderOffset = vec3_norm(shoulderToHand) * (adjustAmount * armLength * 0.225f);

		NiPoint3 clavicalToNewShoulder = arm.upper->m_worldTransform.pos + shoulderOffset - arm.shoulder->m_worldTransform.pos;

		NiPoint3 sLocalDir = (arm.shoulder->m_worldTransform.rot.Transpose() * clavicalToNewShoulder) / arm.shoulder->m_worldTransform.scale;

		Matrix44 rotatedM;
		rotatedM = 0.0;
		rotatedM.rotateVectoVec(sLocalDir, NiPoint3(1,0,0));

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

		//sometimes powerarmor or other things detach or do not even load all forearm nodes so just keeping this check in to preserve functionality without crashing.
		if ((arm.forearm2 == nullptr) || (arm.forearm3 == nullptr)) {
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

		if ((arm.forearm2 != nullptr) && (arm.forearm3 != nullptr)) {
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
		if (arm.forearm3) {
			arm.hand->m_localTransform.rot = rotatedM.multiply43Left(Fwr3.Transpose());
		}
		else {
			arm.hand->m_localTransform.rot = rotatedM.multiply43Left(Fwr.Transpose());
		}

		// Calculate Flp:  Fwp = Uwp + Uwr * (Flp * Uws) = elbowWorld   ===>   Flp = Uwr' * (elbowWorld - Uwp) / Uws
		arm.forearm1->m_localTransform.pos = Uwr.Transpose() * ((elbowWorld - Uwp) / arm.upper->m_worldTransform.scale);

		float origEHLen = vec3_len(arm.hand->m_worldTransform.pos - arm.forearm1->m_worldTransform.pos);
		float forearmRatio = (forearmLen / origEHLen) * _root->m_localTransform.scale;

		forearmRatio *= _inPowerArmor ? 1.5 : 1.0;
		if (arm.forearm2) {
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

	void Skeleton::calculateHandPose(std::string bone, float gripProx, bool thumbUp, bool isLeft) {
		Quaternion qc;
		Quaternion qt;

		int sign = isLeft ? -1 : 1;

		// if a mod is using the papyrus interface to manually set finger poses
		if (handPapyrusHasControl[bone]) {
			Quaternion qo;
			qt.fromRot(handOpen[bone].rot);
			qo.fromRot(handClosed[bone].rot);
			qo.slerp(std::clamp(handPapyrusPose[bone], 0.0f, 1.0f), qt);
			qt = qo;
		}
		// thumbUp pose
		else if (thumbUp && (bone.find("Finger1") != std::string::npos)) {
			if (bone.find("Finger11") != std::string::npos) {
				Matrix44 rot;
				rot.setEulerAngles(sign * 0.5, sign * 0.4, -0.3);

				NiMatrix43 wr = handOpen[bone].rot;
				wr = rot.multiply43Left(wr);

				qt.fromRot(wr);
			}
			else if (bone.find("Finger13") != std::string::npos) {
				Matrix44 rot;
				rot.setEulerAngles(0, 0, degrees_to_rads(-35.0));

				NiMatrix43 wr = handOpen[bone].rot;
				wr = rot.multiply43Left(wr);

				qt.fromRot(wr);
			}
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

		float blend = std::clamp(_frameTime*7, 0.0, 1.0);

		qc.slerp(blend, qt);

		Matrix44 rot;
		rot = qc.getRot();

		_handBones[bone].rot = rot.make43();
	}

	void Skeleton::copy1stPerson(std::string bone, bool isLeft, int offset) {

		int firstPos = 68;

		BSFlattenedBoneTree* fpTree = (BSFlattenedBoneTree*)(*g_player)->firstPersonSkeleton->m_children.m_data[0]->GetAsNiNode();


		if (firstPos != 0) {
			if (fpTree->transforms[firstPos + offset].refNode) {
				_handBones[bone] = fpTree->transforms[firstPos + offset].refNode->m_localTransform;
			}
			else {
				_handBones[bone] = fpTree->transforms[firstPos + offset].local;
			}
		}
	}

	void Skeleton::setHandPose() {

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;
		bool isLeft = false;

		for (auto pos = 0; pos < rt->numTransforms; pos++) {


			if(strstr(boneTreeVec[pos].c_str(), "Finger")) {

				isLeft = strncmp(boneTreeVec[pos].c_str(), "L", 1) ? false : true;
				uint64_t reg = isLeft ? leftControllerState.ulButtonTouched : rightControllerState.ulButtonTouched;
				float gripProx = isLeft ? leftControllerState.rAxis[2].x : rightControllerState.rAxis[2].x;
				bool thumbUp = (reg & vr::ButtonMaskFromId(vr::k_EButton_Grip)) && (reg & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) && (!(reg & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad)));
				_closedHand[boneTreeVec[pos]] = reg & vr::ButtonMaskFromId(_handBonesButton[boneTreeVec[pos]]);

				if ((*g_player)->actorState.IsWeaponDrawn() && !isLeft) {
					int dig1 = boneTreeVec[pos].c_str()[11] - '0';
					int dig2 = boneTreeVec[pos].c_str()[12] - '0';
					int arrOff = (dig2 - 1) + ((dig1 - 1) * 3);
					this->copy1stPerson(boneTreeVec[pos], isLeft, arrOff);
				}
				else {
					this->calculateHandPose(boneTreeVec[pos], gripProx, thumbUp, isLeft);
				}

				NiTransform trans = _handBones[boneTreeVec[pos]];

				rt->transforms[pos].local.rot = trans.rot;

				if (rt->transforms[pos].refNode) {
					rt->transforms[pos].refNode->m_localTransform = rt->transforms[pos].local;
					updateTransforms(rt->transforms[pos].refNode);
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
			std::string weapname = (*g_player)->middleProcess->unk08->equipData->item->GetFullName();
			if (weap) {
				if (!c_staticGripping) {
					weap->m_localTransform = _weapSave;
				}
				auto newWeapon = (_inPowerArmor ? weapname + powerArmorSuffix : weapname) != _lastWeapon;
				if (newWeapon) {
					_lastWeapon = (_inPowerArmor ? weapname + powerArmorSuffix : weapname);
					_useCustomWeaponOffset = false;
					auto lookup = g_weaponOffsets->getOffset(weapname, _inPowerArmor);
					if (lookup.has_value()) {
						_useCustomWeaponOffset = true;
						_customTransform = lookup.value();
						_MESSAGE("Found weaponOffset for %s pos (%f, %f, %f) scale %f: powerArmor: %d", 
							weapname, _customTransform.pos.x, _customTransform.pos.y, _customTransform.pos.z, _customTransform.scale, _inPowerArmor);
					}else { // offsets should already be applied if not already saved
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
				if (_useCustomWeaponOffset) { // load custom transform
					weap->m_localTransform = _customTransform;
				}
				else // save transform to manipulate
					_customTransform = weap->m_localTransform;
				updateDown(weap, true);

				// handle offhand gripping

				static NiPoint3 fingerBonePos = NiPoint3(0, 0, 0);
				static NiPoint3 bodyPos = NiPoint3(0, 0, 0);
				static float avgHandV[3] = {0.0f, 0.0f, 0.0f};
				static int fc = 0;
				float handV = 0.0f;

				static auto offHandBone = c_leftHandedMode ? "RArm_Finger31" : "LArm_Finger31";
				if (_offHandGripping && c_enableOffHandGripping) {

					float handFrameMovement;

					handFrameMovement = vec3_len(rt->transforms[boneTreeMap[offHandBone]].world.pos - fingerBonePos);

					float bodyFrameMovement = vec3_len(_curPos - bodyPos);
					avgHandV[fc] = abs(handFrameMovement - bodyFrameMovement);

					fc = fc == 2 ? 0 : fc + 1;

					float sum = 0;

					for (int i = 0; i < 3; i++) {
						sum += avgHandV[i];
					}

					handV = sum / 3;

					uint64_t reg = c_leftHandedMode ? rightControllerState.ulButtonPressed : leftControllerState.ulButtonPressed;
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
							_MESSAGE("Reposition Button Hold start: weapon %s", weapname);
						}
					}
					else{
						_pressLength = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _repositionButtonHoldStart;
						if (reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_repositionButtonID) && _pressLength > c_holdDelay) {
							vrhook->StartHaptics(c_leftHandedMode? 0 : 1, 0.05, 0.3);
						} else if (!(reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_repositionButtonID))) {
							_repositionButtonHolding = false;
							_hasLetGoRepositionButton = true;
							_endFingerBonePos = rt->transforms[boneTreeMap[offHandBone]].world.pos - _curPos;
							_MESSAGE("Reposition Button Hold stop: weapon %s %d ms", weapname, _pressLength);
						}
					}

					if (_offHandGripping) {

						NiPoint3 oH2Bar;

						oH2Bar = rt->transforms[boneTreeMap[offHandBone]].world.pos - weap->m_worldTransform.pos;

						NiPoint3 barrelVec = NiPoint3(0, 1, 0);

						oH2Bar = weap->m_worldTransform.rot.Transpose() * vec3_norm(oH2Bar) / weap->m_worldTransform.scale;

						Matrix44 rot;
						//rot.rotateVectoVec(oH2Bar, barrelVec);

						// use Quaternion to rotate the vector onto the other vector to avoid issues with the poles
						_aimAdjust.vec2vec(vec3_norm(oH2Bar), vec3_norm(barrelVec));
						rot = _aimAdjust.getRot();
						weap->m_localTransform.rot = rot.multiply43Left(weap->m_localTransform.rot);

						fingerBonePos = rt->transforms[boneTreeMap[offHandBone]].world.pos;
						bodyPos = _curPos;
						if (_repositionButtonHolding) {
							auto end = fingerBonePos - _curPos;
							auto change = vec3_len(end) > vec3_len(_startFingerBonePos)? vec3_len(end - _startFingerBonePos) : -vec3_len(end - _startFingerBonePos);
							if (change != 0) {
								weap->m_localTransform.pos.x += change;
								_DMESSAGE("Updating x translation for %s by %f to %f", weapname, change, weap->m_localTransform.pos.x);
							}
						}
						//_hasLetGoRepositionButton is always one frame after _repositionButtonHolding
						if (_hasLetGoRepositionButton && _pressLength > 0 && _pressLength < c_holdDelay) {
							_MESSAGE("Updating grip rotation for %s: powerArmor: %d", weapname, _inPowerArmor);
							_customTransform.rot = weap->m_localTransform.rot;
							_hasLetGoRepositionButton = false;
							_useCustomWeaponOffset = true;
							g_weaponOffsets->addOffset(weapname, _customTransform, _inPowerArmor);
							writeOffsetJson();
						}else if (_hasLetGoRepositionButton && _pressLength > 0 && _pressLength > c_holdDelay) {
							auto change = vec3_len(_endFingerBonePos) > vec3_len(_startFingerBonePos) ? vec3_len(_endFingerBonePos - _startFingerBonePos) : -vec3_len(_endFingerBonePos - _startFingerBonePos);
							weap->m_localTransform.pos.x += change;
							_MESSAGE("Saving position translation for %s from %f -> %f", weapname, _customTransform.pos.x, weap->m_localTransform.pos.x);
							_customTransform.pos.x = weap->m_localTransform.pos.x;
							_hasLetGoRepositionButton = false;
							_useCustomWeaponOffset = true;
							g_weaponOffsets->addOffset(weapname, _customTransform, _inPowerArmor);
							writeOffsetJson();
						}
							_MESSAGE("Resetting grip to defaults for %s: powerArmor: %d", weapname, _inPowerArmor);
							_useCustomWeaponOffset = false;
							g_weaponOffsets->deleteOffset(weapname, _inPowerArmor);
							writeOffsetJson();
						}
					}
				}
				else {
					fingerBonePos = rt->transforms[boneTreeMap[offHandBone]].world.pos;
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

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;

		if (weap && (*g_player)->actorState.IsWeaponDrawn()) {
			NiPoint3 barrelVec = NiPoint3(0, 1, 0);

			NiPoint3 oH2Bar = c_leftHandedMode ? rt->transforms[boneTreeMap["RArm_Finger31"]].world.pos - weap->m_worldTransform.pos : rt->transforms[boneTreeMap["LArm_Finger31"]].world.pos - weap->m_worldTransform.pos;

			float len = vec3_len(oH2Bar);

			oH2Bar = weap->m_worldTransform.rot.Transpose() * vec3_norm(oH2Bar) / weap->m_worldTransform.scale;

			float dotP = vec3_dot(vec3_norm(oH2Bar), barrelVec);
			uint64_t reg = c_leftHandedMode ? rightControllerState.ulButtonPressed : leftControllerState.ulButtonPressed;

			if (!(reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_gripButtonID))) {
				_hasLetGoGripButton = true;
			}

			if ((dotP > 0.955) && (len > 10.0)) {

				if (!c_enableGripButtonToGrap) {
					_offHandGripping = true;
				}
				else if (!_pipboyStatus && reg & vr::ButtonMaskFromId((vr::EVRButtonId)c_gripButtonID)) {
					if (_offHandGripping || !_hasLetGoGripButton) {
						return;
					}
					_offHandGripping = true;
					_hasLetGoGripButton = false;
				}
			}
		}
		else {
			_offHandGripping = false;
		}
	}

	void Skeleton::moveBack() {
		NiNode* body = _root->m_parent->GetAsNiNode();

		_root->m_localTransform.pos = body->m_worldTransform.pos - this->getPosition();
		_root->m_localTransform.pos.y -= 50.0;
	}

	void Skeleton::debug() {

		static int fc = 0;

	//	for (auto i = 0; i < (*g_player)->inventoryList->items.count; i++) {
		//	_MESSAGE("%d,%d,%x,%x,%s", fc, i, (*g_player)->inventoryList->items[i].form->formID, (*g_player)->inventoryList->items[i].form->formType, (*g_player)->inventoryList->items[i].form->GetFullName());
		//}

		//if (fc < 1) {
		//	tHashSet<ObjectModMiscPair, BGSMod::Attachment::Mod*> *map = g_modAttachmentMap.GetPtr();
		//	map->Dump();
		//}

		//BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_root;

		//for (auto i = 0; i < rt->numTransforms; i++) {

		//	if (rt->transforms[i].refNode) {
		//		_MESSAGE("%d,%s,%d,%d", fc, rt->transforms[i].refNode->m_name.c_str(), rt->transforms[i].childPos, rt->transforms[i].parPos);
		//	}
		//	else {
		//		_MESSAGE("%d,%s,%d,%d", fc, "", rt->transforms[i].childPos, rt->transforms[i].parPos);
		//	}
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

			//if (rt->bonePositions[i].name && (rt->bonePositions[i].position != 0) && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
			//	int pos = rt->bonePositions[i].position;
			//	if (pos > rt->numTransforms) {
			//		continue;
			//	}
			//	_MESSAGE("%d,%s,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", fc, rt->bonePositions[i].name->data,
			//						rt->bonePositions[i].position,
			//						rt->transforms[pos].local.rot.arr[0],
			//						rt->transforms[pos].local.rot.arr[1],
			//						rt->transforms[pos].local.rot.arr[2],
			//						rt->transforms[pos].local.rot.arr[3],
			//						rt->transforms[pos].local.rot.arr[4],
			//						rt->transforms[pos].local.rot.arr[5],
			//						rt->transforms[pos].local.rot.arr[6],
			//						rt->transforms[pos].local.rot.arr[7],
			//						rt->transforms[pos].local.rot.arr[8],
			//						rt->transforms[pos].local.rot.arr[9],
			//						rt->transforms[pos].local.rot.arr[10],
			//						rt->transforms[pos].local.rot.arr[11],
			//						rt->transforms[pos].local.pos.x,
			//						rt->transforms[pos].local.pos.y,
			//						rt->transforms[pos].local.pos.z,
			//						rt->transforms[pos].world.rot.arr[0],
			//						rt->transforms[pos].world.rot.arr[1],
			//						rt->transforms[pos].world.rot.arr[2],
			//						rt->transforms[pos].world.rot.arr[3],
			//						rt->transforms[pos].world.rot.arr[4],
			//						rt->transforms[pos].world.rot.arr[5],
			//						rt->transforms[pos].world.rot.arr[6],
			//						rt->transforms[pos].world.rot.arr[7],
			//						rt->transforms[pos].world.rot.arr[8],
			//						rt->transforms[pos].world.rot.arr[9],
			//						rt->transforms[pos].world.rot.arr[10],
			//						rt->transforms[pos].world.rot.arr[11],
			//						rt->transforms[pos].world.pos.x,
			//						rt->transforms[pos].world.pos.y,
			//						rt->transforms[pos].world.pos.z
			//	);


			//	if(strstr(rt->bonePositions[i].name->data, "Finger")) {
			//		Matrix44 rot;
			//		rot.makeIdentity();
			//		rt->transforms[pos].local.rot = rot.make43();

			//		if (rt->transforms[pos].refNode) {
			//			rt->transforms[pos].refNode->m_localTransform.rot = rot.make43();
			//		}

			//		rot.makeTransformMatrix(rt->transforms[pos].local.rot, NiPoint3(0, 0, 0));

			//		short parent = rt->transforms[pos].parPos;
			//		rt->transforms[pos].world.rot = rot.multiply43Left(rt->transforms[parent].world.rot);

			//		if (rt->transforms[pos].refNode) {
			//			rt->transforms[pos].refNode->m_worldTransform.rot = rt->transforms[pos].world.rot;
			//		}

			//	}
			//}

			//rt->UpdateWorldBound();
		//}

		fc++;
	}

}
