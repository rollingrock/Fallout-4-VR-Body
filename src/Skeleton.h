#pragma once

#include <map>

#include "api/openvr.h"
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
		Skeleton(BSFadeNode* node)
			: _root(node) {
			_curPos = NiPoint3(0, 0, 0);
			_walkingState = 0;
		}

		ArmNodes getLeftArm() const {
			return _leftArm;
		}

		ArmNodes getRightArm() const {
			return _rightArm;
		}

		void setCommonNode() {
			_common = f4vr::getNode("COM", _root);
		}

		NiPoint3 getCurrentBodyPos() const {
			return _curPos;
		}

		NiTransform getBoneWorldTransform(const std::string& boneName);

		NiPoint3 getOffhandIndexFingerTipWorldPosition();

		// reposition stuff
		void selfieSkelly();
		static void setupHead();
		void saveStatesTree(NiNode* node);
		void restoreLocals();
		void setUnderHMD();
		void setBodyPosture();
		void setLegs() const;
		void setSingleLeg(bool isLeft) const;
		void setArms(bool isLeft);
		void setHandPose();
		NiPoint3 getPosition();
		void makeArmsT(bool) const;
		void fixArmor() const;
		void showHidePAHud() const;
		void hideHands() const;

		// movement
		void walk();

		void setWandsVisibility(bool show = true, WandMode mode = WandMode::Both) const;
		void showWands(WandMode mode = WandMode::Both) const;
		void hideWands(WandMode mode = WandMode::Both) const;
		void hideWeapon() const;
		void leftHandedModePipboy() const;
		void positionPipboy() const;
		void hideFistHelpers() const;
		void fixPAArmor();
		void dampenHand(NiNode* node, bool isLeft);
		void hidePipboy() const;
		static bool armorHasHeadLamp();

		// utility
		bool setNodes();
		ArmNodes getArm(bool isLeft) const;
		static void set1StPersonArm(NiNode* weapon, NiNode* offsetNode);
		void setBodyLen();
		void setKneePos();
		void showOnlyArms() const;
		void handleWeaponNodes();
		void setLeftHandedSticky();
		void calculateHandPose(const std::string& bone, float gripProx, bool thumbUp, bool isLeft);
		void copy1StPerson(const std::string& bone);
		void insertSaveState(const std::string& name, NiNode* node);
		void rotateLeg(uint32_t pos, float angle) const;
		void moveBack();
		void initLocalDefaults();
		void fixBoneTree();

		void setTime();
		// Body Positioning
		float getNeckYaw() const;
		float getNeckPitch() const;
		float getBodyPitch() const;

	private:
		void initBoneTreeMap();
		void restoreLocals(NiNode* node);
		static std::map<std::string, std::pair<std::string, std::string>> makeFingerRelations();

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
		std::map<std::string, NiTransform, CaseInsensitiveComparator> _savedStates;
		std::map<std::string, NiPoint3, CaseInsensitiveComparator> _boneLocalDefault;

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

		std::map<std::string, NiTransform, CaseInsensitiveComparator> _handBones;
		std::map<std::string, bool, CaseInsensitiveComparator> _closedHand;
		std::map<std::string, vr::EVRButtonId, CaseInsensitiveComparator> _handBonesButton;

		NiTransform _rightHandPrevFrame;
		NiTransform _leftHandPrevFrame;

		// bones
		std::map<std::string, int> _boneTreeMap;
		std::vector<std::string> _boneTreeVec;
		const std::map<std::string, std::pair<std::string, std::string>> _fingerRelations = makeFingerRelations();
	};
}
