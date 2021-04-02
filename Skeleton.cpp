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

	void Matrix44::getEulerAngles(float *heading, float *roll, float *attitude) {

		if (data[2][0] < 1.0) {
			if (data[2][0] > -1.0) {
				*heading = atan2(-data[2][1], data[2][2]);
				*attitude = asin(data[2][0]);
				*roll = atan2(-data[1][0], data[0][0]);
			}
			else {
				*heading = -atan2(-data[0][1], data[1][1]);
				*attitude = -PI / 2;
				*roll = 0.0;
			}
		}
		else {
			*heading = atan2(data[0][1], data[1][1]);
			*attitude = PI / 2;
			*roll = 0.0;
		}
	}

	void Matrix44::setEulerAngles(float heading, float roll, float attitude) {
		float sinX = sin(heading);
		float cosX = cos(heading);
		float sinY = sin(attitude);
		float cosY = cos(attitude);
		float sinZ = sin(roll);
		float cosZ = cos(roll);

		data[0][0] = cosY * cosZ;
		data[0][1] = sinX * sinY * cosZ + sinZ * cosX;
		data[0][2] = sinX * sinZ - cosX * sinY * cosZ;
		data[1][0] = -cosY * sinZ;
		data[1][1] = cosX * cosZ - sinX * sinY * sinZ;
		data[1][2] = cosX * sinY * sinZ + sinX * cosZ;
		data[2][0] = sinY;
		data[2][1] = -sinX * cosY;
		data[2][2] = cosX * cosY;
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

		result = matrixMultiply(local, result, &mat);
		
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
		}

		for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
			auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
			if (nextNode) {
				this->updateDown(nextNode, true);
			}
		}


	}

	void Skeleton::projectSkelly(float offsetOutFront) {    // Projects the 3rd person body out in front of the player by offset amount


//		this->updateDown(_root);

		NiPoint3 playerLook;
		playerLook.x = _root->m_worldTransform.rot.data[1][0];   // get unit vector pointing straight out in front of the player in player space.   y is forward axis  x is horizontal   since 2x2 rotation z is not used
		playerLook.y = _root->m_worldTransform.rot.data[1][1];
		playerLook.z = 0;

		playerLook *= offsetOutFront;

		this->rotateWorld(_root);
		this->updatePos(_root, playerLook);   // offset all positions in the skeleton
		
		// offset up the z-axis some;

		playerLook.x = 0.0;    // offset vector pointing up.
		playerLook.y = 0.0;
		playerLook.z = 15.0;

		this->updatePos(_root, playerLook);   // offset all positions in the skeleton

//		this->updateDown(_root);

	}

	Matrix44* Skeleton::matrixMultiply(Matrix44* worldMat, Matrix44* retMat, Matrix44* localMat) {
		using func_t = decltype(&Skeleton::matrixMultiply);
		RelocAddr<func_t> func(0x1a8d60);

		Matrix44* mat = func(worldMat, retMat, localMat);
		return mat;
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

	void Skeleton::setNodes() {
		_playerNodes = (PlayerNodes*)((char*)(*g_player) + 0x6E0);

		setCommonNode();
		_rightHand = this->getNode("RArm_Hand", _root);
		_leftHand = this->getNode("LArm_Hand", _root);

		_wandRight = getNode("fist_M_Right", _playerNodes->primaryWandNode);
		_wandLeft = getNode("LeftHand", _playerNodes->SecondaryWandNode);

		_wandRight = _playerNodes->primaryWandNode->m_children.m_data[8]->GetAsNiNode();
		_wandLeft = _playerNodes->SecondaryWandNode->m_children.m_data[5]->GetAsNiNode();
		
		_MESSAGE("common node = %016I64X", _common);
		_MESSAGE("righthand node = %016I64X", _rightHand);
		_MESSAGE("lefthand node = %016I64X", _leftHand);
	}

	void Skeleton::positionDiff() {
		NiPoint3 firstpos = _playerNodes->HmdNode->m_worldTransform.pos;
		NiPoint3 skellypos = _root->m_worldTransform.pos;

		_MESSAGE("difference = %f %f %f", (firstpos.x - skellypos.x), (firstpos.y - skellypos.y), (firstpos.z - skellypos.z));

	}

	NiPoint3 Skeleton::getPosition() {
		NiPoint3 curPos = _playerNodes->HmdNode->m_worldTransform.pos;
		
		float dist = sqrt(pow((curPos.x - _lastPos.x),2) + pow((curPos.y - _lastPos.y),2) + pow((curPos.z - _lastPos.z),2));

		//if (dist > 50.0) {
			_lastPos = curPos;
		//}

		return _lastPos;
	}

	void Skeleton::setUnderHMD() {
		Matrix44 mat;
		Matrix44* result = new Matrix44();
		
		mat = 0.0;

		float y = _playerNodes->UprightHmdNode->m_worldTransform.rot.data[1][1];  // Middle column is y vector.   Grab just x and y portions and make a unit vector.    This can be used to rotate body to always be orientated with the hmd.
		float x = _playerNodes->UprightHmdNode->m_worldTransform.rot.data[1][0];  //  Later will use this vector as the basis for the rest of the IK

		float mag = sqrt(y * y + x * x);

		x /= mag;
		y /= mag;

		float dot = (_curx * x) + (_cury * y);

		if (dot < 0.5) {
			_curx = x;
			_cury = y;
		}
		else {
			x = _curx;
			y = _cury;
		}

		mat.data[0][0] = y;
		mat.data[0][1] = -x;
		mat.data[1][0] = x;
		mat.data[1][1] = y;
		mat.data[2][2] = 1.0;

		mat.setPosition(this->getPosition());
		mat.data[3][3] = 1.0;

		_common->m_localTransform.pos.y = -10.0;
		_common->m_localTransform.pos.z = 0.0;

		Matrix44* loc = (Matrix44*)&(_common->m_localTransform.rot);

		matrixMultiply(&mat, result, loc);


		for (auto i = 0; i < 3; i++) {
			for (auto j = 0; j < 3; j++) {
				_common->m_worldTransform.rot.data[i][j] = result->data[i][j];

			}
		}

		_common->m_worldTransform.pos.x = result->data[3][0];
		_common->m_worldTransform.pos.y = result->data[3][1];
		_common->m_worldTransform.pos.z = _root->m_worldTransform.pos.z + DEFAULT_HEIGHT;

		updateDown(_common, false);

		delete result;
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

		matrixMultiply((Matrix44*)&(_rightHand->m_worldTransform.rot), result, &mat);

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
}
