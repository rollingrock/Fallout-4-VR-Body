#include "Skeleton.h"
#include "F4VRBody.h"

#include "api/PapyrusVRAPI.h"
#include "api/VRManagerAPI.h"

extern PapyrusVRAPI* g_papyrusvr;

namespace F4VRBody
{

	float defaultCameraHeight = 120.4828;

	UInt32 KeywordPowerArmor = 0x4D8A1;
	UInt32 KeywordPowerArmorFrame = 0x15503F;

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

		if (updateSelf) {
//			nde->UpdateWorldData(ud);
			updateTransforms(nde);
		}

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				this->updateDown(nextNode, true);
			}
		}
	}

	void Skeleton::updateDownTo(NiNode* toNode, NiNode* fromNode, bool updateSelf) {
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

	void Skeleton::projectSkelly(float offsetOutFront) {    // Projects the 3rd person body out in front of the player by offset amount
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

	void Skeleton::setupHead(NiNode* headNode) {
		headNode->m_localTransform.rot.data[0][0] =  0.967;
		headNode->m_localTransform.rot.data[0][1] = -0.251;
		headNode->m_localTransform.rot.data[0][2] =  0.047;
		headNode->m_localTransform.rot.data[1][0] =  0.249;
		headNode->m_localTransform.rot.data[1][1] =  0.967;
		headNode->m_localTransform.rot.data[1][2] =  0.051;
		headNode->m_localTransform.rot.data[2][0] = -0.058;
		headNode->m_localTransform.rot.data[2][1] = -0.037;
		headNode->m_localTransform.rot.data[2][2] =  0.998;
		headNode->m_localTransform.pos.y = -4.0;

		NiAVObject::NiUpdateData* ud = nullptr;

		headNode->UpdateWorldData(ud);
	}

	void Skeleton::saveStatesTree(NiNode* node) {

		std::string name(node->m_name.c_str());

		if (savedStates.find(name) == savedStates.end()) {
			savedStates.insert({ std::string(node->m_name.c_str()), node->m_localTransform });
		}
		else {
			savedStates[name] = node->m_localTransform;
		}

		for (auto i = 0; i < node->m_children.m_emptyRunStart; ++i) {
			auto nextNode = node->m_children.m_data[i] ? node->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				this->saveStatesTree(nextNode);
			}
		}
	}

	void Skeleton::restoreLocals(NiNode* node) {

		std::string name(node->m_name.c_str());

		if (savedStates.find(name) == savedStates.end()) {
			return;
		}

		node->m_localTransform = savedStates[name];

		for (auto i = 0; i < node->m_children.m_emptyRunStart; ++i) {
			auto nextNode = node->m_children.m_data[i] ? node->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				this->restoreLocals(nextNode);
			}
		}
	}

	void Skeleton::setNodes() {
		_playerNodes = (PlayerNodes*)((char*)(*g_player) + 0x6E0);

		setCommonNode();

		_rightHand = getNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		_leftHand  = getNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		_wandRight = (NiNode*)_playerNodes->primaryWandNode->m_children.m_data[8];
		_wandLeft  = (NiNode*)_playerNodes->SecondaryWandNode->m_children.m_data[5];

		_spine = this->getNode("SPINE2", _root);
		_chest = this->getNode("Chest", _root);
		
		_MESSAGE("common node = %016I64X", _common);
		_MESSAGE("righthand node = %016I64X", _rightHand);
		_MESSAGE("lefthand node = %016I64X", _leftHand);


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

		saveStatesTree(_root->m_parent->GetAsNiNode());
	}

	void Skeleton::positionDiff() {
		NiPoint3 firstpos = _playerNodes->HmdNode->m_worldTransform.pos;
		NiPoint3 skellypos = _root->m_worldTransform.pos;

		_MESSAGE("difference = %f %f %f", (firstpos.x - skellypos.x), (firstpos.y - skellypos.y), (firstpos.z - skellypos.z));

	}

	NiPoint3 Skeleton::getPosition() {
		NiPoint3 curPos = _playerNodes->UprightHmdNode->m_worldTransform.pos;
		
		float dist = vec3_len(curPos - _lastPos);

	//	if (dist > 40.0) {
			_lastPos = curPos;
		//}

		return _lastPos;
	}

	// below takes the two vectors from hmd to each hand and sums them to determine a center axis in which to see how much the hmd has rotated
	// A secondary angle is also calculated which is 90 degrees on the z axis up to handle when the hands are approaching the z plane of the hmd
	// this helps keep the body stable through a wide range of hand poses
	// this still struggles with hands close to the face and with one hand low and one hand high.    Will need to take progs advice to add weights 
	// to these positions which i'll do at a later date.


	float Skeleton::getNeckYaw() {
		NiPoint3 pos = _playerNodes->UprightHmdNode->m_worldTransform.pos;
		NiPoint3 hmdToLeft  = _playerNodes->SecondaryWandNode->m_worldTransform.pos  - pos;
		NiPoint3 hmdToRight = _playerNodes->primaryWandNode->m_worldTransform.pos - pos;

		if ((vec3_len(hmdToLeft) < 10.0f) || (vec3_len(hmdToRight) < 10.0f)) {
			return 0.0;
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

		return std::clamp(-angleFinal, degrees_to_rads(-90.0f), degrees_to_rads(90.0f));
	}

	float Skeleton::getNeckPitch() {

		NiPoint3 lookDir = vec3_norm(_playerNodes->HmdNode->m_worldTransform.rot.Transpose() * _playerNodes->HmdNode->m_localTransform.pos);
		float pitchAngle = atan2f(lookDir.y, lookDir.z);

		return pitchAngle;
	}

	float Skeleton::getBodyPitch() {
		float basePitch = 115.3;
		float weight = 0.2;

		float offset = inPowerArmor ? (10.0f + c_cameraHeight) : c_cameraHeight;
		float curHeight = c_playerHeight + offset;
		float heightCalc = abs((curHeight - _playerNodes->UprightHmdNode->m_localTransform.pos.z) / curHeight);

		float angle = heightCalc * (basePitch + weight * rads_to_degrees(getNeckPitch()));

		return degrees_to_rads(angle);
	}

	void Skeleton::setUnderHMD() {

		detectInPowerArmor();

		_playerNodes->playerworldnode->m_localTransform.pos.z = inPowerArmor ? (10.0f + c_cameraHeight) : c_cameraHeight;

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

		updateDown(_root, true);

		NiPoint3 back = vec3_norm(NiPoint3(_forwardDir.x, _forwardDir.y, 0));
		NiPoint3 bodydir = NiPoint3(0, 1, 0);

		mat.rotateVectoVec(back, bodydir);
		_root->m_localTransform.rot = mat.multiply43Left(body->m_worldTransform.rot.Transpose());
		_root->m_localTransform.pos = body->m_worldTransform.pos - getPosition();
		_root->m_localTransform.pos.z = z;
		_root->m_localTransform.scale = c_playerHeight / (inPowerArmor ? (defaultCameraHeight + 10.0f) : defaultCameraHeight);    // set scale based off specified user height

		body->m_worldBound.m_kCenter = this->getPosition();
	}

	void Skeleton::setBodyPosture() {
		float neckPitch = getNeckPitch();
		float bodyPitch = getBodyPitch();


		// save leg positions before moving COM node
		_leftFootPos  = getNode("LLeg_Foot", _root)->m_worldTransform.pos;
		_rightFootPos = getNode("RLeg_Foot", _root)->m_worldTransform.pos;
		_leftKneePos  = getNode("LLeg_Calf", _root)->m_worldTransform.pos;
		_rightKneePos = getNode("RLeg_Calf", _root)->m_worldTransform.pos;

		NiNode* camera = (*g_playerCamera)->cameraNode;
		NiNode* com = getNode("COM", _root);
		NiNode* spine = getNode("SPINE1", _root);

		float comZ = com->m_localTransform.pos.z;

		float z_adjust = c_playerOffset_up + sinf(-neckPitch) * 4.5;
		NiPoint3 neckAdjust = NiPoint3(-_forwardDir.x * c_playerOffset_forward / 2, -_forwardDir.y * c_playerOffset_forward / 2, z_adjust);
		NiPoint3 neckPos = camera->m_worldTransform.pos + neckAdjust;

		NiPoint3 hmdToHip = neckPos - com->m_worldTransform.pos;
		NiPoint3 dir = NiPoint3(-_forwardDir.x, -_forwardDir.y, 0);

		float dist = tanf(bodyPitch) * vec3_len(hmdToHip);
		NiPoint3 tmpHipPos = com->m_worldTransform.pos + dir * (dist / vec3_len(dir));
		tmpHipPos.z = com->m_worldTransform.pos.z;

		NiPoint3 hmdtoNewHip = tmpHipPos - neckPos;
		NiPoint3 newHipPos = neckPos + hmdtoNewHip * (_torsoLen / vec3_len(hmdtoNewHip));

		NiPoint3 newPos = com->m_localTransform.pos + _root->m_worldTransform.rot.Transpose() * ((newHipPos - com->m_worldTransform.pos) / _root->m_localTransform.scale);
		com->m_localTransform.pos.y += newPos.y + c_playerOffset_forward;
		com->m_localTransform.pos.z = newPos.z;
		com->m_localTransform.pos.z -= inPowerArmor ? 15.0f : 0.0f;

		Matrix44 rot;
		rot.rotateVectoVec(neckPos - tmpHipPos, hmdToHip);
		NiMatrix43 mat = rot.multiply43Left(spine->m_parent->m_worldTransform.rot.Transpose());
		rot.makeTransformMatrix(mat, NiPoint3(0, 0, 0));
		spine->m_localTransform.rot = rot.multiply43Right(spine->m_worldTransform.rot);
		
	}

	void Skeleton::fixArmor() {
		NiNode* lPauldron = getNode("L_Pauldron", _root);
		NiNode* rPauldron = getNode("R_Pauldron", _root);
		float delta  = getNode("LArm_Collarbone", _root)->m_worldTransform.pos.z - _root->m_worldTransform.pos.z;

		if (lPauldron) {
			lPauldron->m_localTransform.pos.z = delta - 15.0f;
		}
		if (rPauldron) {
			rPauldron->m_localTransform.pos.z = delta - 15.0f;
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

		_leftFootPos  = lFoot->m_worldTransform.pos;
		_rightFootPos = rFoot->m_worldTransform.pos;

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
		static BSFixedString nodeName("PipboyBone");

		NiAVObject* pipboyBone = leftArm.forearm3->GetObjectByName(&nodeName);
		NiAVObject* child = pipboyBone->GetAsNiNode()->m_children.m_data[0];
		pipboyBone->GetAsNiNode()->RemoveChild(child);

		_pipboyStatus = false;
		_pipTimer = 0;
//	    turnPipBoyOff();
		
		//BSFixedString wandPipName("PipboyParent");
		//NiAVObject* wandPip = _playerNodes->SecondaryWandNode->GetObjectByName(&wandPipName);

	//	wandPip->m_parent->GetAsNiNode()->RemoveChild(wandPip);

//		pipboyBone->GetAsNiNode()->AttachChild(wandPip, true);

	}

	void Skeleton::positionPipboy() {
		static BSFixedString wandPipName("PipboyRoot_NIF_ONLY");
		NiAVObject* wandPip = _playerNodes->SecondaryWandNode->GetObjectByName(&wandPipName);

		if (wandPip == nullptr) {
			return;
		}
		
		static BSFixedString nodeName("PipboyBone");
		NiAVObject* pipboyBone = leftArm.forearm3->GetObjectByName(&nodeName);

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
								inPowerArmor = true;
							}
							else
							{
								inPowerArmor = false;
							}
						}
					}
				}
			}
		}

	}

	void Skeleton::hideWands() {
		_wandRight = (NiNode*)_playerNodes->primaryWandNode->m_children.m_data[8];
		_wandLeft  = (NiNode*)_playerNodes->SecondaryWandNode->m_children.m_data[5];
		_wandRight->flags |= 0x1;
		_wandLeft->flags |= 0x1;
	}

	void Skeleton::hideFistHelpers() {
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

	void Skeleton::operatePipBoy() {

		NiAVObject* finger = getNode("RArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		if (finger == nullptr) {
			finger = getNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		}

		NiAVObject* pipboy = getNode("PipboyRoot", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		if ((finger == nullptr) || (pipboy == nullptr)) {
			return;
		}

		float distance = vec3_len(finger->m_worldTransform.pos - pipboy->m_worldTransform.pos);

		if (distance > c_pipboyDetectionRange) {
			_pipTimer = 0;
			_stickypip = false;
			return;
		}
		else {
			if (_stickypip) {
				return;
			}
			else {
				OpenVRHookManagerAPI* vrhook = RequestOpenVRHookManagerObject();
				if (vrhook != nullptr) {
					vrhook->StartHaptics(2, 0.05, 0.3);
				}
			}

			if (_pipTimer < 10) {
				_pipTimer++;
			}
			else {
				_pipTimer = 0;
				_stickypip = true;

				if (_pipboyStatus) {
					_pipboyStatus = false;
					turnPipBoyOff();
				}
				else {
					_pipboyStatus = true;
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

		NiNode* rightWeapon = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		NiNode* leftWeapon = _playerNodes->WeaponLeftNode;

		NiNode* weaponNode = isLeft ? leftWeapon : rightWeapon;
		NiNode* offsetNode = isLeft ? _playerNodes->SecondaryMeleeWeaponOffsetNode2 : _playerNodes->primaryWeaponOffsetNOde;

		if (isLeft) {
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
		weaponNode->m_localTransform.rot = w.make43();

		if (isLeft) {
			w.setEulerAngles(degrees_to_rads(0), degrees_to_rads(45), degrees_to_rads(0));
			weaponNode->m_localTransform.rot = w.multiply43Right(weaponNode->m_localTransform.rot);
			//w.setEulerAngles(0, degrees_to_rads(40.0f), 0);
			//weaponNode->m_localTransform.rot = w.multiply43Right(weaponNode->m_localTransform.rot);
		}
		
	    weaponNode->m_localTransform.pos = isLeft ? NiPoint3(0, 0, 0) : NiPoint3(6.389, -2.099, -3.133);


		weaponNode->IncRef();
		set1stPersonArm(weaponNode, offsetNode);

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
		float originalForearmLen = vec3_len(arm.hand->m_localTransform.pos) + vec3_len(arm.forearm2->m_localTransform.pos) + vec3_len(arm.forearm3->m_localTransform.pos);
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
		rotatedM.makeTransformMatrix(arm.forearm2->m_localTransform.rot, arm.forearm2->m_localTransform.pos);
		NiMatrix43 Fwr2 = rotatedM.multiply43Left(Fwr);
		rotatedM.makeTransformMatrix(arm.forearm3->m_localTransform.rot, arm.forearm3->m_localTransform.pos);
		NiMatrix43 Fwr3 = rotatedM.multiply43Left(Fwr2);



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
		 
		twist.setEulerAngles(negLeft * forearmAngle/2, 0, 0);
		arm.forearm2->m_localTransform.rot = twist.multiply43Left(arm.forearm2->m_localTransform.rot);

		twist.setEulerAngles(negLeft * forearmAngle/2, 0, 0);
		arm.forearm3->m_localTransform.rot = twist.multiply43Left(arm.forearm3->m_localTransform.rot);

		rotatedM.makeTransformMatrix(arm.forearm2->m_localTransform.rot, arm.forearm2->m_localTransform.pos);
		Fwr2 = rotatedM.multiply43Left(Fwr);
		rotatedM.makeTransformMatrix(arm.forearm3->m_localTransform.rot, arm.forearm3->m_localTransform.pos);
		Fwr3 = rotatedM.multiply43Left(Fwr2);

		// Calculate Hlr:  Fwr * Hlr = handRot   ===>   Hlr = Fwr' * handRot
		rotatedM.makeTransformMatrix(handRot, handPos);
		arm.hand->m_localTransform.rot = rotatedM.multiply43Left(Fwr3.Transpose());

		// Calculate Flp:  Fwp = Uwp + Uwr * (Flp * Uws) = elbowWorld   ===>   Flp = Uwr' * (elbowWorld - Uwp) / Uws
		arm.forearm1->m_localTransform.pos = Uwr.Transpose() * ((elbowWorld - Uwp) / arm.upper->m_worldTransform.scale);

		float origEHLen = vec3_len(arm.hand->m_worldTransform.pos - arm.forearm1->m_worldTransform.pos);
		float forearmRatio = forearmLen / origEHLen;
		arm.forearm2->m_localTransform.pos * forearmRatio;
		arm.forearm3->m_localTransform.pos * forearmRatio;
		arm.hand->m_localTransform.pos     * forearmRatio;

		return;
	}


}
