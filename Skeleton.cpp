#include "Skeleton.h"


namespace F4VRBody
{

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
		_MESSAGE("%s%s : children = %d : local x = %f : world x = %f : local y = %f : world y = %f : local z = %f : world z = %f", padding.c_str(), child->m_name.c_str(), child->m_children.m_emptyRunStart,
			child->m_localTransform.pos.x, child->m_worldTransform.pos.x,
			child->m_localTransform.pos.y, child->m_worldTransform.pos.y,
			child->m_localTransform.pos.z, child->m_worldTransform.pos.z);

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
		_MESSAGE("%s : children = %d : local x = %f : world x = %f : local y = %f : world y = %f : local z = %f : world z = %f", nde->m_name.c_str(), nde->m_children.m_emptyRunStart,
			nde->m_localTransform.pos.x, nde->m_worldTransform.pos.x,
			nde->m_localTransform.pos.y, nde->m_worldTransform.pos.y,
			nde->m_localTransform.pos.z, nde->m_worldTransform.pos.z);

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
		headNode->m_localTransform.pos.y = -20.0;
		headNode->m_localTransform.pos.x = 20.0;

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

		_wandRight = getNode("fist_M_Right", _playerNodes->primaryWandNode);
		_wandLeft = getNode("LeftHand", _playerNodes->SecondaryWandNode);

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
		NiPoint3 curPos = (*g_playerCamera)->cameraNode->m_worldTransform.pos;
		
		float dist = vec3_len(curPos - _lastPos);

	//	if (dist > 40.0) {
			_lastPos = curPos;
		//}

		return _lastPos;
	}

	void Skeleton::setUnderHMD() {
		Matrix44 mat;

		mat = 0.0;

		float y = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][1];  // Middle column is y vector.   Grab just x and y portions and make a unit vector.    This can be used to rotate body to always be orientated with the hmd.
		float x = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][0];  //  Later will use this vector as the basis for the rest of the IK
		float z = _root->m_localTransform.pos.z;

		float dot = (_curx * x) + (_cury * y);
		_MESSAGE("dot %5f", dot);

//		if (dot > 0.2) {
			x = _curx;
			y = _cury;
		//}
		//else {
		//	_curx = x;
		//	_cury = y;
		//}

		_forwardDir = NiPoint3(x, y, 0);
		_sidewaysRDir = NiPoint3(y, -x, 0);


		NiNode* body = _root->m_parent->GetAsNiNode();
		body->m_localTransform.pos *= 0.0f;
		body->m_worldTransform.pos.x = this->getPosition().x;
		body->m_worldTransform.pos.y = this->getPosition().y;

		updateDown(_root, true);
		NiNode::NiUpdateData ud;
		ud.flags = 0x1;
	//	_root->UpdateDownwardPass(&ud, 0);

		NiPoint3 back = vec3_norm(NiPoint3(x, y, 0));
		NiPoint3 bodydir = NiPoint3(0, 1, 0);

		mat.rotateVectoVec(back, bodydir);
		_root->m_localTransform.rot = mat.multiply43Left(body->m_worldTransform.rot.Transpose());
		_root->m_localTransform.pos = body->m_worldTransform.pos - this->getPosition();
		_root->m_localTransform.pos.y += -8.0f;
		_root->m_localTransform.pos.z = z;
		_root->m_localTransform.scale = 0.95f;

		body->m_worldBound.m_kCenter = this->getPosition();

	//	_root->UpdateDownwardPass(nullptr, 0);
	//	updateDown(_root, true);

	}

	void Skeleton::setHandPos() {

		_rightHand->m_worldTransform.rot = _wandRight->m_worldTransform.rot;
		_rightHand->m_worldTransform.pos = _wandRight->m_worldTransform.pos;
		_leftHand->m_worldTransform.rot = _wandLeft->m_worldTransform.rot;
		_leftHand->m_worldTransform.pos = _wandLeft->m_worldTransform.pos;

		Matrix44 mat;
		Matrix44* result = new Matrix44();
		mat.makeIdentity();

		mat.data[0][0] = -1.0;
		mat.data[2][2] = -1.0;

		Matrix44::matrixMultiply((Matrix44*)&(_rightHand->m_worldTransform.rot), result, &mat);

		for (auto i = 0; i < 3; i++) {
			for (auto j = 0; j < 3; j++) {
				_rightHand->m_worldTransform.rot.data[i][j] = result->data[i][j];

			}
		}

		updateDown(_rightHand, false);
		updateDown(_leftHand, false);

		//for (auto i = 0; i < 3; i++) {
		//	for (auto j = 0; j < 3; j++) {
		//		mat.data[i][j] = _rightHand->m_localTransform.rot.data[i][j];
		//	}
		//}

		//float yaw, pitch, roll;

		//mat.getEulerAngles(&yaw, &roll, &pitch);


		//mat.setEulerAngles(yaw, roll, pitch);
		//
		//_MESSAGE("%f %f %f", mat.data[0][0], mat.data[0][1], mat.data[0][2]);
		//_MESSAGE("%f %f %f", mat.data[1][0], mat.data[1][1], mat.data[1][2]);
		//_MESSAGE("%f %f %f", mat.data[2][0], mat.data[2][1], mat.data[2][2]);
		//_MESSAGE(" ");


		// Hide wands

		_wandRight->flags |= 0x1;
		_wandLeft->flags |= 0x1;

		delete result;
	}

	void Skeleton::removeHands() {
		//NiNode* node = (*g_player)->GetObjectRootNode();

		_wandRight->flags |= 0x1;
		_wandLeft->flags |= 0x1;

		BSFixedString nodeName("RArm_Hand");

		NiAVObject* node = _common->GetObjectByName(&nodeName);

		if (node) {
			node->m_worldTransform.scale = 0.000001;
		}



		return;

	}

	void Skeleton::hideWeapon() {

		BSFixedString nodeName("Weapon");

		NiAVObject* weapon = rightArm.hand->GetObjectByName(&nodeName);

		if (weapon != nullptr) {
			weapon->flags |= 0x1;
			weapon->m_localTransform.scale = 0.0;
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

	void Skeleton::set1stPersonArm(NiNode* weapon, NiNode* offsetNode) {

		NiNode** wp = &weapon;
		NiNode** op = &offsetNode;

		update1stPersonArm(*g_player, wp, op);
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

		NiNode* rightWeapon = getNode("Weapon", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		NiNode* leftWeapon = getNode("WeaponLeft", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		NiNode* weaponNode = isLeft ? leftWeapon : rightWeapon;
		NiNode* offsetNode = isLeft ? _playerNodes->SecondaryMeleeWeaponOffsetNode : _playerNodes->primaryWeaponOffsetNOde;


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
			w.setEulerAngles(degrees_to_rads(-75.0f), 0, 0);
			weaponNode->m_localTransform.rot = w.multiply43Right(weaponNode->m_localTransform.rot);
			w.setEulerAngles(0, degrees_to_rads(40.0f), 0);
			weaponNode->m_localTransform.rot = w.multiply43Right(weaponNode->m_localTransform.rot);
		}

		NiPoint3 wp = NiPoint3(6.389, -2.099, -3.133);
		weaponNode->m_localTransform.pos = wp;

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


		double adjustedArmLength = 1.0f;

		// Shoulder IK is done in a very simple way

		NiPoint3 shoulderToHand = handPos - arm.upper->m_worldTransform.pos;
		float armLength = 36.74;
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
		float twistAngle = asin((std::clamp)(handBack.z, -0.999f, 0.999f));

		// The second twist angle comes from a side vector pointing "outward" from the side of the wrist
		NiPoint3 handSide = handRot * NiPoint3(0, negLeft, 0);
		NiPoint3 handinSide = handSide * -1;
		float twistAngle2 = asin((std::clamp)(handSide.z, -0.999f, 0.999f));

		// Blend the two twist angles together, using the primary angle more when the wrist is pointing downward
		//float interpTwist = (std::clamp)((handBack.z + 0.866f) * 1.155f, 0.25f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
		float interpTwist = (std::clamp)((handBack.z + 0.9f), 0.2f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
		twistAngle = twistAngle + interpTwist * (twistAngle2 - twistAngle);
		// Wonkiness is bad.  Interpolate twist angle towards zero to correct it when the angles are pointed a certain way.
		float fixWonkiness1 = (std::clamp)(vec3_dot(handSide, vec3_norm(-sidewaysDir - forwardDir * 0.25f + NiPoint3(0, 0, -0.25))), 0.0f, 1.0f);
		float fixWonkiness2 = 1.0f - (std::clamp)(vec3_dot(handBack, vec3_norm(forwardDir + sidewaysDir)), 0.0f, 1.0f);
		twistAngle = twistAngle + fixWonkiness1 * fixWonkiness2 * (-PI / 2.0f - twistAngle);

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
		float armLiftLimitZ = _chest->m_worldTransform.pos.z - 5.0f * size;
		float armLiftThreshold = 60.0f * size;
		float armLiftLimit = (std::clamp)((armLiftLimitZ + armLiftThreshold - handPos.z) / armLiftThreshold, 0.0f, 1.0f); // 1 at bottom, 0 at top
		float upLimit = (std::clamp)((1.0f - armLiftLimit) * 1.4f, 0.0f, 1.0f); // 0 at bottom, 1 at a much lower top

			// Determine overall amount the elbows minimum rotation will be limited
		float adjustMinAmount = (std::max)(behindAmount, (std::min)(armCrossAmount, armLiftLimit));

		// Get the minimum and maximum angles at which the elbow is allowed to twist
		float twistMinAngle = degrees_to_rads(-85.0) + degrees_to_rads(50) * adjustMinAmount;
		float twistMaxAngle = degrees_to_rads(55.0) - (std::max)(degrees_to_rads(90) * armCrossAmount, degrees_to_rads(80) * upLimit);

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
		float upperAngle = -1 * negLeft * acosf(vec3_dot(vec3_norm(uLocalTwist), vec3_norm(uloc)));

		Matrix44 twist;
		twist.setEulerAngles(upperAngle, 0, 0);
		arm.upper->m_localTransform.rot = twist.multiply43Left(arm.upper->m_localTransform.rot);    

		rotatedM.makeTransformMatrix(arm.upper->m_localTransform.rot, arm.upper->m_localTransform.pos);
		Uwr = rotatedM.multiply43Left(arm.shoulder->m_worldTransform.rot);

		twist.setEulerAngles(upperAngle, 0, 0);
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
