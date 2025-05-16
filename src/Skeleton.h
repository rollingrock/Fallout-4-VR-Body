#pragma once

#include <map>

#include "api/openvr.h"
#include "f4se/GameCamera.h"
#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/PlayerNodes.h"
#include "include/SimpleIni.h"

namespace frik {
	enum class WandMode : std::uint8_t {
		Both = 0,
		PrimaryHandWand,
		OffhandWand,
	};

	enum class WeaponType : std::uint8_t {
		HandToHandMelee,
		OneHandSword,
		OneHandDagger,
		OneHandAxe,
		OneHandMace,
		TwoHandSword,
		TwoHandAxe,
		Bow,
		Staff,
		Gun,
		Grenade,
		Mine,
		Spell,
		Shield,
		Torch
	};

	struct ArmNodes {
		NiAVObject* shoulder;
		NiAVObject* upper;
		NiAVObject* upperT1;
		NiAVObject* forearm1;
		NiAVObject* forearm2;
		NiAVObject* forearm3;
		NiAVObject* hand;
	};

	struct HandMeshBoneTransforms {
		NiTransform* LArm_ForeArm1_skin;
		NiTransform* LArm_ForeArm2_skin;
		NiTransform* LArm_ForeArm3_skin;
		NiTransform* LArm_Hand;
		NiTransform* RArm_ForeArm2_skin;
		NiTransform* RArm_ForeArm3_skin;
		NiTransform* RArm_Hand;
		NiTransform* LArm_Finger11;
		NiTransform* LArm_Finger12;
		NiTransform* LArm_Finger13;
		NiTransform* LArm_Finger21;
		NiTransform* LArm_Finger22;
		NiTransform* LArm_Finger23;
		NiTransform* LArm_Finger31;
		NiTransform* LArm_Finger32;
		NiTransform* LArm_Finger33;
		NiTransform* LArm_Finger41;
		NiTransform* LArm_Finger42;
		NiTransform* LArm_Finger43;
		NiTransform* LArm_Finger51;
		NiTransform* LArm_Finger52;
		NiTransform* LArm_Finger53;
		NiTransform* RArm_Finger11;
		NiTransform* RArm_Finger12;
		NiTransform* RArm_Finger13;
		NiTransform* RArm_Finger21;
		NiTransform* RArm_Finger22;
		NiTransform* RArm_Finger23;
		NiTransform* RArm_Finger31;
		NiTransform* RArm_Finger32;
		NiTransform* RArm_Finger33;
		NiTransform* RArm_Finger41;
		NiTransform* RArm_Finger42;
		NiTransform* RArm_Finger43;
		NiTransform* RArm_Finger51;
		NiTransform* RArm_Finger52;
		NiTransform* RArm_Finger53;
	};

	struct CaseInsensitiveComparator {
		bool operator()(const std::string& a, const std::string& b) const noexcept {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		}
	};

	class Skeleton {
	public:
		Skeleton()
			: _root(nullptr) {
			_curPos = NiPoint3(0, 0, 0);
			_walkingState = 0;
		}

		Skeleton(BSFadeNode* node)
			: _root(node) {
			_curPos = NiPoint3(0, 0, 0);
			_walkingState = 0;
		}

		void setDirection() {
			_cury = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][1];
			// Middle column is y vector.   Grab just x and y portions and make a unit vector.    This can be used to rotate body to always be orientated with the hmd.
			_curx = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][0]; //  Later will use this vector as the basis for the rest of the IK

			const float mag = sqrt(_cury * _cury + _curx * _curx);

			_curx /= mag;
			_cury /= mag;
		}

		BSFadeNode* getRoot() const {
			return _root;
		}

		void updateRoot(BSFadeNode* node) {
			_root = node;
		}

		ArmNodes getLeftArm() const {
			return _leftArm;
		}

		ArmNodes getRightArm() const {
			return _rightArm;
		}

		f4vr::PlayerNodes* getPlayerNodes() const {
			return _playerNodes;
		}

		void setCommonNode() {
			_common = f4vr::getNode("COM", _root);
		}

		bool inPowerArmor() const {
			return _inPowerArmor;
		}

		NiPoint3 getCurrentBodyPos() const {
			return _curPos;
		}

		static int getBoneInMap(const std::string& boneName);

		NiNode* getWeaponNode() const;
		NiNode* getPrimaryWandNode() const;
		NiNode* getThrowableWeaponNode() const;

		NiPoint3 getOffhandIndexFingerTipWorldPosition() const;

		// reposition stuff
		void selfieSkelly();
		static void setupHead(NiNode* headNode, bool hideHead);
		void saveStatesTree(NiNode* node);
		void restoreLocals(NiNode* node);
		void setUnderHMD();
		void setBodyPosture();
		void setLegs();
		void setSingleLeg(bool isLeft) const;
		void setArms(bool isLeft);
		void setHandPose();
		NiPoint3 getPosition();
		void makeArmsT(bool) const;
		void fixArmor() const;
		void showHidePAHud() const;
		void hideHands();

		// movement
		void walk();

		void setWandsVisibility(bool show = true, WandMode mode = WandMode::Both) const;
		void showWands(WandMode mode = WandMode::Both) const;
		void hideWands(WandMode mode = WandMode::Both) const;
		void hideWeapon() const;
		void leftHandedModePipboy() const;
		void positionPipboy() const;
		void hideFistHelpers() const;
		void fixPAArmor() const;
		void dampenHand(NiNode* node, bool isLeft);
		void hidePipboy() const;
		static bool armorHasHeadLamp();

		// utility
		bool setNodes();
		ArmNodes getArm(bool isLeft) const;
		static void set1StPersonArm(NiNode* weapon, NiNode* offsetNode);
		void setBodyLen();
		void setKneePos();
		void showOnlyArms();
		void handleWeaponNodes();
		void setLeftHandedSticky();
		void calculateHandPose(const std::string& bone, float gripProx, bool thumbUp, bool isLeft);
		void copy1StPerson(const std::string& bone);
		void insertSaveState(const std::string& name, NiNode* node);
		void rotateLeg(uint32_t pos, float angle) const;
		void moveBack();
		void initLocalDefaults();
		void fixBoneTree() const;

		void setTime();
		// Body Positioning
		float getNeckYaw() const;
		float getNeckPitch() const;
		float getBodyPitch() const;

	private:
		BSFadeNode* _root;
		NiNode* _common;
		NiPoint3 _lastPos;
		NiPoint3 _curPos;
		NiPoint3 _forwardDir;
		NiPoint3 _sidewaysRDir;
		NiPoint3 _upDir;
		f4vr::PlayerNodes* _playerNodes;
		NiNode* _rightHand;
		NiNode* _leftHand;
		NiNode* _spine;
		NiNode* _chest;
		float _torsoLen;
		float _legLen;
		ArmNodes _rightArm;
		ArmNodes _leftArm;
		float _curx;
		float _cury;
		std::map<std::string, NiTransform, CaseInsensitiveComparator> _savedStates;
		std::map<std::string, NiPoint3, CaseInsensitiveComparator> _boneLocalDefault;

		NiMatrix43 _originalPipboyRotation;

		bool _leftHandedSticky;

		bool _inPowerArmor;

		LARGE_INTEGER _freqCounter;
		LARGE_INTEGER _timer;
		LARGE_INTEGER _prevTime;
		double _frameTime;

		int _walkingState;
		double _currentStepTime;
		NiPoint3 _leftFootPos;
		NiPoint3 _rightFootPos;
		NiPoint3 _rightFootTarget;
		NiPoint3 _leftFootTarget;
		NiPoint3 _rightFootStart;
		NiPoint3 _leftFootStart;
		NiPoint3 _leftKneePosture;
		NiPoint3 _rightKneePosture;
		NiPoint3 _leftKneePos;
		NiPoint3 _rightKneePos;
		int _footStepping;
		NiPoint3 _stepDir;
		double _prevSpeed;
		double _stepTimeinStep;
		int _delayFrame;

		HandMeshBoneTransforms* _boneTransforms;

		std::map<std::string, NiTransform, CaseInsensitiveComparator> _handBones;
		std::map<std::string, bool, CaseInsensitiveComparator> _closedHand;
		std::map<std::string, vr::EVRButtonId, CaseInsensitiveComparator> _handBonesButton;

		NiTransform _rightHandPrevFrame;
		NiTransform _leftHandPrevFrame;
	};
}
