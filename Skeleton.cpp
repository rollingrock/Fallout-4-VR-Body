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
			nde->UpdateWorldData(ud);
	//		updateTransforms(nde);
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

		_wandRight = _playerNodes->primaryWandNode->m_children.m_data[8]->GetAsNiNode();
		_wandLeft = _playerNodes->SecondaryWandNode->m_children.m_data[5]->GetAsNiNode();

		_spine = this->getNode("SPINE2", _root);
		
		_MESSAGE("common node = %016I64X", _common);
		_MESSAGE("righthand node = %016I64X", _rightHand);
		_MESSAGE("lefthand node = %016I64X", _leftHand);


		// Setup Arms
		BSFixedString rCollar("RArm_Collarbone");
		BSFixedString rUpper("RArm_UpperArm");
		BSFixedString rUpper1("RArm_UpperTwist1");
		BSFixedString rForearm("RArm_ForeArm1");
		BSFixedString rForearm1("RArm_ForeArm2");
		BSFixedString rForearm2("RArm_ForeArm3");
		BSFixedString rHand("RArm_Hand");
		BSFixedString lCollar("LArm_Collarbone");
		BSFixedString lUpper("LArm_UpperArm");
		BSFixedString lUpper1("LArm_UpperTwist1");
		BSFixedString lForearm("LArm_ForeArm1");
		BSFixedString lForearm1("LArm_ForeArm2");
		BSFixedString lForearm2("LArm_ForeArm3");
		BSFixedString lHand("LArm_Hand");

		rightArm.shoulder  = _common->GetObjectByName(&rCollar);
		rightArm.upper     = _common->GetObjectByName(&rUpper);
		rightArm.upperT1   = _common->GetObjectByName(&rUpper1);
		rightArm.forearm   = _common->GetObjectByName(&rForearm);
		rightArm.forearmT1 = _common->GetObjectByName(&rForearm1);
		rightArm.forearmT2 = _common->GetObjectByName(&rForearm2);
		rightArm.hand      = _common->GetObjectByName(&rHand);

		leftArm.shoulder  = _common->GetObjectByName(&lCollar);
		leftArm.upper     = _common->GetObjectByName(&lUpper);
		leftArm.upperT1   = _common->GetObjectByName(&lUpper1);
		leftArm.forearm   = _common->GetObjectByName(&lForearm);
		leftArm.forearmT1 = _common->GetObjectByName(&lForearm1);
		leftArm.forearmT2 = _common->GetObjectByName(&lForearm2);
		leftArm.hand      = _common->GetObjectByName(&lHand);

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

		if (dist > 40.0) {
			_lastPos = curPos;
		}

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

		if (dot > 0.3) {
			x = _curx;
			y = _cury;
		}
		else {
			_curx = x;
			_cury = y;
		}

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
		_root->m_localTransform.pos.y += -5.0f;
		_root->m_localTransform.pos.z = z;
		_root->m_localTransform.scale = 0.87f;

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

	void Skeleton::makeArmsT(bool isLeft) {
		ArmNodes arm;

		arm = isLeft ? leftArm : rightArm;
		Matrix44 mat;
		mat.makeIdentity();

		arm.forearm->m_localTransform.rot = mat.make43();
		arm.forearmT1->m_localTransform.rot = mat.make43();
		arm.forearmT2->m_localTransform.rot = mat.make43();
		arm.hand->m_localTransform.rot = mat.make43();
		arm.upper->m_localTransform.rot = mat.make43();
//		arm.shoulder->m_localTransform.rot = mat.make43();

		return;
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

		float upperArmLength = vec3_len(arm.forearm->m_worldTransform.pos - arm.upper->m_worldTransform.pos);
		float lowerArmLength = vec3_len(arm.hand->m_worldTransform.pos - arm.forearm->m_worldTransform.pos);
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

		_MESSAGE("");
		_MESSAGE("========== Frame %d ============", g_mainLoopCounter);
		
//		updateTransforms(arm.shoulder->m_parent->GetAsNiNode());
		updateTransforms(arm.shoulder->GetAsNiNode());
		updateTransforms(arm.upper->GetAsNiNode());
		//updateTransforms(arm.forearm->GetAsNiNode());
		//updateTransforms(arm.forearmT1->GetAsNiNode());
		//updateTransforms(arm.forearmT2->GetAsNiNode());
		//updateTransforms(arm.hand->GetAsNiNode());

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

		//_MESSAGE("handPos               {%5f %5f %5f}", handPos.x, handPos.y, handPos.z);
		//_MESSAGE("wandPos               {%5f %5f %5f}", _wandRight->m_worldTransform.pos.x, _wandRight->m_worldTransform.pos.y, _wandRight->m_worldTransform.pos.z);
		//_MESSAGE("shoulderPos           {%5f %5f %5f}", arm.shoulder->m_worldTransform.pos.x, arm.shoulder->m_worldTransform.pos.y, arm.shoulder->m_worldTransform.pos.z);
		//_MESSAGE("upperPos              {%5f %5f %5f}", arm.upper->m_worldTransform.pos.x, arm.upper->m_worldTransform.pos.y, arm.upper->m_worldTransform.pos.z);
		//_MESSAGE("shoulderToHand        {%5f %5f %5f}", shoulderToHand.x, shoulderToHand.y, shoulderToHand.z);
		//_MESSAGE("shoulderToHandLen      %5f", vec3_len(shoulderToHand));
		//_MESSAGE("adjustAmount           %5f", adjustAmount);
		//_MESSAGE("shoulderOffet         {%5f %5f %5f}", shoulderOffset.x, shoulderOffset.y, shoulderOffset.z);
		//_MESSAGE("shoulderOffetLen       %5f", vec3_len(shoulderOffset));



		if (shoulderOffset.z < 0) {
			shoulderOffset *= 0.4;
		}

		NiPoint3 clavicalToNewShoulder = arm.upper->m_worldTransform.pos + shoulderOffset - arm.shoulder->m_worldTransform.pos;
		clavicalToNewShoulder.z *= -1;    // Bethesda is for some reason inverting the z axis in the final world space translation calculation

		NiPoint3 sLocalDir = (arm.shoulder->m_worldTransform.rot.Transpose() * clavicalToNewShoulder) / arm.shoulder->m_worldTransform.scale;

		//_MESSAGE("clavicalToNewShoulder    {%5f %5f %5f}", clavicalToNewShoulder.x, clavicalToNewShoulder.y, clavicalToNewShoulder.z);
		//_MESSAGE("sLocalDir                {%5f %5f %5f}", sLocalDir.x, sLocalDir.y, sLocalDir.z);

		Matrix44 rotatedM;
		rotatedM = 0.0;
		rotatedM.rotateVectoVec(arm.upper->m_localTransform.pos, sLocalDir);

		NiMatrix43 result = rotatedM.multiply43Left(arm.shoulder->m_localTransform.rot);
		arm.shoulder->m_localTransform.rot = result;

		updateTransforms(arm.shoulder->GetAsNiNode());
		updateTransforms(arm.upper->GetAsNiNode());

		//_MESSAGE("rotatedM[0]           {%5f %5f %5f}", rotatedM.data[0][0], rotatedM.data[0][1], rotatedM.data[0][2]);
		//_MESSAGE("rotatedM[1]           {%5f %5f %5f}", rotatedM.data[1][0], rotatedM.data[1][1], rotatedM.data[1][2]);
		//_MESSAGE("rotatedM[2]           {%5f %5f %5f}", rotatedM.data[2][0], rotatedM.data[2][1], rotatedM.data[2][2]);
		//_MESSAGE("result[0]           {%5f %5f %5f}", result.data[0][0], result.data[0][1], result.data[0][2]);
		//_MESSAGE("result[1]           {%5f %5f %5f}", result.data[1][0], result.data[1][1], result.data[1][2]);
		//_MESSAGE("result[2]           {%5f %5f %5f}", result.data[2][0], result.data[2][1], result.data[2][2]);
		//_MESSAGE("arm.shoulder->m_localTransform.rot[0]           {%5f %5f %5f}", arm.shoulder->m_localTransform.rot.data[0][0], arm.shoulder->m_localTransform.rot.data[0][1], arm.shoulder->m_localTransform.rot.data[0][2]);
		//_MESSAGE("arm.shoulder->m_localTransform.rot[1]           {%5f %5f %5f}", arm.shoulder->m_localTransform.rot.data[1][0], arm.shoulder->m_localTransform.rot.data[1][1], arm.shoulder->m_localTransform.rot.data[1][2]);
		//_MESSAGE("arm.shoulder->m_localTransform.rot[2]           {%5f %5f %5f}", arm.shoulder->m_localTransform.rot.data[2][0], arm.shoulder->m_localTransform.rot.data[2][1], arm.shoulder->m_localTransform.rot.data[2][2]);

		return;

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

		float originalUpperLen = vec3_len(arm.forearm->m_localTransform.pos);
		float originalForearmLen = vec3_len(arm.hand->m_localTransform.pos);
		float upperLen = originalUpperLen * adjustedArmLength;
		float forearmLen = originalForearmLen * adjustedArmLength;

		NiPoint3 Uwp = arm.upper->m_worldTransform.pos;
		NiPoint3 handToShoulder = Uwp - handPos;
		float hsLen = (std::max)(vec3_len(handToShoulder), 0.1f);
		
		_MESSAGE("handPos               {%5f %5f %5f}", handPos.x, handPos.y, handPos.z);
		_MESSAGE("handToShoulder        {%5f %5f %5f}", handToShoulder.x, handToShoulder.y, handToShoulder.z);
		_MESSAGE("hslen                  %5f", hsLen);
		if (hsLen > (upperLen + forearmLen) * 2.25) {
		//	return;
		}


		// Stretch the upper arm and forearm proportionally when the hand distance exceeds the arm length
		if (hsLen > upperLen + forearmLen) {
			float diff = hsLen - upperLen - forearmLen;
			float ratio = forearmLen / (forearmLen + upperLen);
			forearmLen += ratio * diff + 0.1;
			upperLen += (1.0 - ratio) * diff + 0.1;
		}

		_MESSAGE("upperLen               %5f", upperLen);
		_MESSAGE("forearmLen             %5f", forearmLen);
		// Use the spine direction as the body facing angle on X, Y --- This vector has spine twist applied already -- ****NOT DONE NOW******
		NiPoint3 forwardDir = _forwardDir;
		NiPoint3 sidewaysDir = _sidewaysRDir * negLeft;

		_MESSAGE("forwardDir            {%5f %5f %5f}", forwardDir.x, forwardDir.y, forwardDir.z);
		_MESSAGE("sidewaysDir           {%5f %5f %5f}", sidewaysDir.x, sidewaysDir.y, sidewaysDir.z);
		// The primary twist angle comes from the direction the wrist is pointing into the forearm
		NiPoint3 handBack = handRot * NiPoint3(-1, 0, 0);
		float twistAngle = asin((std::clamp)(handBack.z, -0.999f, 0.999f));
		_MESSAGE("handback              {%5f %5f %5f}", handBack.x, handBack.y, handBack.z);
		_MESSAGE("twistangle             %5f", rads_to_degrees(twistAngle));

		// The second twist angle comes from a side vector pointing "outward" from the side of the wrist
		NiPoint3 handSide = handRot * NiPoint3(0, 1 * negLeft, 0);
		float twistAngle2 = asin((std::clamp)(handSide.z, -0.999f, 0.999f));
		_MESSAGE("handSide              {%5f %5f %5f}", handSide.x, handSide.y, handSide.z);
		_MESSAGE("twistangle2            %5f", rads_to_degrees(twistAngle2));

		// Blend the two twist angles together, using the primary angle more when the wrist is pointing downward
		//float interpTwist = (std::clamp)((handBack.z + 0.866f) * 1.155f, 0.25f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
		float interpTwist = (std::clamp)((handBack.z + 0.9f), 0.2f, 0.8f); // 0 to 1 as hand points 60 degrees down to horizontal
		twistAngle = twistAngle + interpTwist * (twistAngle2 - twistAngle);
		// Wonkiness is bad.  Interpolate twist angle towards zero to correct it when the angles are pointed a certain way.
		//float fixWonkiness1 = (std::clamp)(vec3_dot(handSide, vec3_norm(-sidewaysDir - forwardDir * 0.25f + NiPoint3(0, -0.25, 0))), 0.0f, 1.0f);
		//float fixWonkiness2 = 1.0f - (std::clamp)(vec3_dot(handBack, vec3_norm(forwardDir + sidewaysDir)), 0.0f, 1.0f);
		twistAngle = twistAngle + /*fixWonkiness1 * fixWonkiness2 * */(-PI / 2.0f - twistAngle);
		
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
		float armLiftLimitZ = _spine->m_worldTransform.pos.z - 5.0f * size;
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
		NiPoint3 uLocalDir = Uwr.Transpose() * vec3_norm(elbowWorld - Uwp);

		rotatedM.rotateVectoVec(NiPoint3(1, 0, 0), uLocalDir);
		arm.upper->m_localTransform.rot = rotatedM.multiply43Left(arm.upper->m_localTransform.rot);

		rotatedM.makeTransformMatrix(arm.upper->m_localTransform.rot, arm.upper->m_localTransform.pos);
		
		Uwr = rotatedM.multiply43Left(arm.shoulder->m_worldTransform.rot);
		
		// Find the angle of the forearm twisted around the upper arm and twist the upper arm to align it
		//    Uwr * twist = Cwr * Ulr   ===>   Ulr = Cwr' * Uwr * twist
		NiPoint3 uLocalTwist = Uwr.Transpose() * vec3_norm(handPos - elbowWorld);
		NiPoint3 forearmProjection = vec3_norm(uLocalTwist - NiPoint3(1, 0, 0) * vec3_dot(uLocalTwist, NiPoint3(1, 0, 0))); //uLocalTwist - NiPoint3(0,0,1) * dot(uLocalTwist, NiPoint3(0,0,1)));
		float upperAngle = acos(vec3_dot(forearmProjection, NiPoint3(0, -1, 0))) * (forearmProjection.z*-1 > 0 ? 1 : -1) - degrees_to_rads(90);
		if (upperAngle < -degrees_to_rads(180)) {
			upperAngle += degrees_to_rads(360);
		}
		Matrix44 twist;
		//twist.setEulerAngles(0, upperAngle * 1.0, 0);
		//arm.upper->m_localTransform.rot = twist.multiply43Left(arm.upper->m_localTransform.rot);
		//twist.setEulerAngles(0, -upperAngle * 0.7, 0);
		//arm.upperT1->m_localTransform.rot = twist.multiply43Right(arm.upperT1->m_localTransform.rot);

		rotatedM.makeTransformMatrix(arm.upper->m_localTransform.rot, arm.upper->m_localTransform.pos);
		Uwr = rotatedM.multiply43Left(arm.shoulder->m_worldTransform.rot);

		// The forearm arm bone must be rotated from its forward vector to its elbow-to-hand vector in its local space
		// Calculate Flr:  Fwr * rotTowardHand = Uwr * Flr   ===>   Flr = Uwr' * Fwr * rotTowardHand
		rotatedM.makeTransformMatrix(arm.forearm->m_localTransform.rot, arm.forearm->m_localTransform.pos);
		NiMatrix43 Fwr = rotatedM.multiply43Left(Uwr);
		NiPoint3 fLocalDir = Fwr.Transpose() * vec3_norm(handPos - elbowWorld);

		rotatedM.rotateVectoVec(NiPoint3(1, 0, 0), fLocalDir);
		arm.forearm->m_localTransform.rot = rotatedM.multiply43Left(arm.forearm->m_localTransform.rot);
		rotatedM.makeTransformMatrix(arm.forearm->m_localTransform.rot, arm.forearm->m_localTransform.pos);
		Fwr = rotatedM.multiply43Left(Uwr);

		// Find the angle of the hand twisted around the forearm and twist the forearm to align it
		//    Fwr * twist = Uwr * Flr   ===>   Flr = (Uwr' * Fwr) * twist = (Flr) * twist
		NiPoint3 fLocalTwist = Fwr.Transpose() * handSide;
		NiPoint3 handProjection = vec3_norm(fLocalTwist - NiPoint3(1, 0, 0) * vec3_dot(fLocalTwist, NiPoint3(1, 0, 0)));
		float forearmAngle = acos(vec3_dot(handProjection, NiPoint3(1, 0, 0))) * (handProjection.y > 0 ? 1 : -1) + (isLeft ? degrees_to_rads(60) : degrees_to_rads(105));

		//if (isLeft == false && vlibClassifyHeldWeapon(true) == WeaponClass::Large) {
		//	forearmAngle -= DEG_TO_RAD(75);
		//}
		//else if (isLeft == true && vlibGetWeaponType((*g_thePlayer)->GetEquippedObject(true)) == 9) {
		//	forearmAngle += DEG_TO_RAD(75);
		//}
		if (forearmAngle > degrees_to_rads(180)) {
			forearmAngle -= degrees_to_rads(360);
		}

		twist.setEulerAngles(0, forearmAngle * 0.1, 0);
		arm.forearm->m_localTransform.rot = twist.multiply43Left(arm.forearm->m_localTransform.rot);

		twist.setEulerAngles(0, forearmAngle * 0.9, 0);
		arm.forearmT1->m_localTransform.rot = twist.multiply43Left(arm.forearmT1->m_localTransform.rot);

		twist.setEulerAngles(0, forearmAngle * 0.5, 0);
		arm.forearmT2->m_localTransform.rot = twist.multiply43Left(arm.forearmT2->m_localTransform.rot);

		rotatedM.makeTransformMatrix(arm.forearm->m_localTransform.rot, arm.forearm->m_localTransform.pos);
		Fwr = rotatedM.multiply43Left(Uwr);

		// Calculate Hlr:  Fwr * Hlr = handRot   ===>   Hlr = Fwr' * handRot
		rotatedM.makeTransformMatrix(handRot, handPos);
		arm.hand->m_localTransform.rot = rotatedM.multiply43Left(Fwr.Transpose());

		// Calculate Flp:  Fwp = Uwp + Uwr * (Flp * Uws) = elbowWorld   ===>   Flp = Uwr' * (elbowWorld - Uwp) / Uws
		arm.forearm->m_localTransform.pos = Uwr.Transpose() * (elbowWorld - arm.upper->m_worldTransform.pos) / 1.0f;

		// Calculate Hlp:  Hwp = Fwp + Fwr * (Hlp * Fws) = handPos   ===>   Hlp = Fwr' * (handPos - Fwp) / Fws
		arm.hand->m_localTransform.pos = Fwr.Transpose() * (handPos - elbowWorld) / 1.0;

		// Also adjust the twist bone local positions to make the arm deform better when stretching
		// multiplier = (point * end2) / (point * end1)
		float lengthF1 = (std::clamp)(vec3_len(arm.forearmT1->m_localTransform.pos), 0.1f, 100.0f);
		float lengthF2 = (std::clamp)(vec3_len(arm.forearmT2->m_localTransform.pos), 0.1f, 100.0f);

		arm.forearmT1->m_localTransform.pos = arm.forearmT1->m_localTransform.pos * ((lengthF1 * forearmLen) / (lengthF1 * originalForearmLen));
		arm.forearmT2->m_localTransform.pos = arm.forearmT2->m_localTransform.pos * ((lengthF2 * forearmLen) / (lengthF2 * originalForearmLen));

		return;
	}




}
