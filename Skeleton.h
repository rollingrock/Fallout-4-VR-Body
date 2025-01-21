#pragma once
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameCamera.h"
#include "f4se/GameRTTI.h"
#include "f4se/NiNodes.h"
#include "f4se/NiNodes.h"
#include "f4se/BSSkin.h"
#include "f4se/GameFormComponents.h"
#include "f4se/GameMenus.h"
#include "api/PapyrusVRAPI.h"
#include "api/VRManagerAPI.h"
#include "include/SimpleIni.h"
#include <algorithm>
#include <array>
#include <map>

#include "utils.h"
#include "matrix.h"
#include "Quaternion.h"
#include "BSFlattenedBoneTree.h"


#define DEFAULT_HEIGHT 56.0;

extern uint64_t g_mainLoopCounter;



namespace F4VRBody
{
	enum WeaponType
	{
		kWeaponType_Hand_To_Hand_Melee,
		kWeaponType_One_Hand_Sword,
		kWeaponType_One_Hand_Dagger,
		kWeaponType_One_Hand_Axe,
		kWeaponType_One_Hand_Mace,
		kWeaponType_Two_Hand_Sword,
		kWeaponType_Two_Hand_Axe,
		kWeaponType_Bow,
		kWeaponType_Staff,
		kWeaponType_Gun,
		kWeaponType_Grenade,
		kWeaponType_Mine,
		kWeaponType_Spell,
		kWeaponType_Shield,
		kWeaponType_Torch
	};

     // part of PlayerCharacter object but making useful struct below since not mapped in F4SE
	struct PlayerNodes {
		NiNode* playerworldnode; //0x06E0
		NiNode* roomnode; //0x06E8
		NiNode* primaryWandNode; //0x06F0
		NiNode* primaryWandandTouchPad; //0x06F8
		NiNode* primaryUIAttachNode; //0x0700
		NiNode* primaryWeapontoWeaponNode; //0x0708
		NiNode* primaryWeaponKickbackRecoilNode; //0x0710
		NiNode* primaryWeaponOffsetNOde; //0x0718
		NiNode* primaryWeaponScopeCamera; //0x0720
		NiNode* primaryVertibirdMinigunOffNOde; //0x0728
		NiNode* primaryMeleeWeaponOffsetNode; //0x0730
		NiNode* primaryUnarmedPowerArmorWeaponOffsetNode; //0x0738
		NiNode* primaryWandLaserPointer; //0x0740
		NiNode* PrimaryWandLaserPointerAdjuster; //0x0748
		NiNode* unk750; //0x0750
		NiNode* PrimaryMeleeWeaponOffsetNode; //0x0758
		NiNode* SecondaryMeleeWeaponOffsetNode; //0x0760
		NiNode* SecondaryWandNode; //0x0768
		NiNode* Point002Node; //0x0770
		NiNode* WorkshopPalletNode; //0x0778
		NiNode* WorkshopPallenSlide; //0x0780
		NiNode* SecondaryUIOffsetNode; //0x0788
		NiNode* SecondaryMeleeWeaponOffsetNode2; //0x0790
		NiNode* SecondaryUnarmedPowerArmorWeaponOffsetNode; //0x0798
		NiNode* SecondaryAimNode; //0x07A0
		NiNode* PipboyParentNode; //0x07A8
		NiNode* PipboyRoot_nif_only_node; //0x07B0
		NiNode* ScreenNode; //0x07B8
		NiNode* PipboyLightParentNode; //0x07C0
		NiNode* unk7c8; //0x07C8
		NiNode* ScopeParentNode; //0x07D0
		NiNode* unk7d8; //0x07D8
		NiNode* HmdNode; //0x07E0
		NiNode* OffscreenHmdNode; //0x07E8
		NiNode* UprightHmdNode; //0x07F0
		NiNode* UprightHmdLagNode; //0x07F8
		NiNode* BlackSphereNode; //0x0800
		NiNode* HeadLightParentNode; //0x0808
		NiNode* unk810; //0x0810
		NiNode* WeaponLeftNode; //0x0818
		NiNode* unk820; //0x0820
		NiNode* LockPickParentNode; //0x0828

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

	struct CaseInsensitiveComparator
	{
		bool operator()(const std::string& a, const std::string& b) const noexcept
		{
			return ::_stricmp(a.c_str(), b.c_str()) < 0;
		}
	};

	class Skeleton {
	public:
		Skeleton() : _root(nullptr)
		{
			_curPos = NiPoint3(0, 0, 0);
			_walkingState = 0;
		}

		Skeleton(BSFadeNode* a_node) : _root(a_node)
		{
			_curPos = NiPoint3(0, 0, 0);
			_walkingState = 0;
		}

		void setDirection() {
			_cury = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][1];  // Middle column is y vector.   Grab just x and y portions and make a unit vector.    This can be used to rotate body to always be orientated with the hmd.
			_curx = (*g_playerCamera)->cameraNode->m_worldTransform.rot.data[1][0];  //  Later will use this vector as the basis for the rest of the IK

			float mag = sqrt(_cury * _cury + _curx * _curx);

			_curx /= mag;
			_cury /= mag;
		}

		BSFadeNode* getRoot() {
			return _root;
		}

		void updateRoot(BSFadeNode* node) {
			_root = node;
		}

		PlayerNodes* getPlayerNodes() {
			return _playerNodes;
		}

		void setCommonNode() {
			_common = this->getNode("COM", _root);
		}

		// info stuff
		void printChildren(NiNode* child, std::string padding);
		void printNodes(NiNode* nde);
		void positionDiff();

		// reposition stuff
		void rotateWorld(NiNode* nde);
		void updatePos(NiNode* nde, NiPoint3 offset);
		void selfieSkelly(float offsetOutFront);
		void setupHead(NiNode* headNode, bool hideHead);
		void saveStatesTree(NiNode* node);
		void restoreLocals(NiNode* node);
		void setUnderHMD(float groundHeight);
		void setBodyPosture();
		void setLegs();
		void setSingleLeg(bool isLeft);
		void setArms(bool isLeft);
		void setArms_wp(bool isLeft);
		void setHandPose();
		NiPoint3 getPosition();
		NiPoint3 getPosition2();
		void makeArmsT(bool);
		void fixArmor();
		void showHidePAHUD();
		void hideHands();
		void fixBackOfHand();

		// movement
		void walk();

		enum wandMode {
			both = 0,
			mainhandWand,
			offhandWand,
		};

		void setWandsVisibility(bool a_show = true, wandMode a_mode = both);
		void showWands(wandMode a_mode = both);
		void hideWands(wandMode a_mode = both);
		void hideWeapon();
		void swapPipboy();
		void leftHandedModePipboy();
		void positionPipboy();
		void fixMelee();
		void hideFistHelpers();
		void fixPAArmor();
		void dampenHand(NiNode* node, bool isLeft);
		void dampenPipboyScreen();
		bool isLookingAtPipBoy();
		void hidePipboy();
		void operatePipBoy();
		bool armorHasHeadLamp();
		void setPipboyHandPose();
		void disablePipboyHandPose();
		void setConfigModeHandPose();
		void disableConfigModePose();

		// two handed
		void offHandToBarrel();

		// utility
		NiNode* getNode(const char* nodeName, NiNode* nde);
		void setVisibility(NiAVObject* nde, bool a_show = true); // Change flags to show or hide a node
		void updateDown(NiNode* nde, bool updateSelf);
		void updateDownTo(NiNode* toNode, NiNode* fromNode, bool updateSelf);
		void updateUpTo(NiNode* toNode, NiNode* fromNode, bool updateSelf);
		bool setNodes();
		ArmNodes getArm(bool isLeft);
		void set1stPersonArm(NiNode* weapon, NiNode* offsetNode);
		void setBodyLen();
		bool detectInPowerArmor();
		void setKneePos();
		void showOnlyArms();
		void handleWeaponNodes();
		void setLeftHandedSticky();
		void calculateHandPose(std::string bone, float gripProx, bool thumbUp, bool isLeft);
		void copy1stPerson(std::string bone);
		void insertSaveState(const std::string& name, NiNode* node);
		void rotateLeg(uint32_t pos, float angle);
		void offHandToScope();
		void moveBack();
		void debug();
		void initLocalDefaults();
		void fixBoneTree();
		void pipboyConfigurationMode();
		void mainConfigurationMode();
		void pipboyManagement();
		void exitPBConfig();
		void configModeExit();
		
		void setTime();
		// Body Positioning
		float getNeckYaw();
		float getNeckPitch();
		float getBodyPitch();

		enum repositionMode {
			weapon = 0, // move weapon
			offhand, // move offhand grip position
			resetToDefault, // reset to default and exit reposition mode
			total = resetToDefault
		};

	private:
		BSFadeNode* _root;
		NiNode* _common;
		NiPoint3   _lastPos;
		NiPoint3   _curPos;
		NiPoint3   _forwardDir;
		NiPoint3   _sidewaysRDir;
		NiPoint3   _upDir;
		PlayerNodes* _playerNodes;
		NiNode* _rightHand;
		NiNode* _leftHand;
		NiNode* _wandRight;
		NiNode* _wandLeft;
		NiNode* _spine;
		NiNode* _chest;
		float _torsoLen;
		float _legLen;
		ArmNodes rightArm;
		ArmNodes leftArm;
		float _curx;
		float _cury;
		std::map<std::string, NiTransform, CaseInsensitiveComparator> savedStates;
		std::map<std::string, NiPoint3, CaseInsensitiveComparator> boneLocalDefault;

		// Cylons Vars
		bool _stickyoffpip = false;
		bool _stickybpip = false;
		bool _isSaveButtonPressed = false;
		bool _isModelSwapButtonPressed = false;
		bool _isHandsButtonPressed = false;
		bool _isWeaponButtonPressed = false;
		bool _isGripButtonPressed = false;
		bool _isPBConfigModeActive = false;
		int _PBConfigModeEnterCounter = 0;
		bool stickyPBlight = false;
		bool stickyPBRadio = false;
		bool _PBConfigSticky = false;
		bool _PBControlsSticky[7] = { false, false, false,  false,  false,  false, false };
		bool _SwithLightButtonSticky = false;
		bool _SwitchLightHaptics = true;
		bool _UISelectSticky = false;
		bool _UIAltSelectSticky = false;
		UInt32 LastPipboyPage = 0;
		float lastRadioFreq = 0.0;
		bool c_IsOperatingPipboy = false;
		bool _PBTouchbuttons[10] = { false, false, false, false, false, false, false, false, false, false };
		bool _MCTouchbuttons[10] = { false, false, false, false, false, false, false, false, false, false };
		bool c_CalibrationModeUIActive = false;
		// End

		NiMatrix43 originalPipboyRotation;
		bool _pipboyStatus;
		int _pipTimer;
		
		bool _stickypip;

		bool _leftHandedSticky;

		bool _inPowerArmor;

		LARGE_INTEGER freqCounter;
		LARGE_INTEGER timer;
		LARGE_INTEGER prevTime;
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
		int delayFrame;

		HandMeshBoneTransforms* _boneTransforms;


		std::map<std::string, NiTransform, CaseInsensitiveComparator> _handBones;
		std::map<std::string, bool, CaseInsensitiveComparator> _closedHand;
		std::map<std::string, vr::EVRButtonId, CaseInsensitiveComparator> _handBonesButton;

		bool _weaponEquipped;
		NiTransform _weapSave;
		NiTransform _customTransform;
		bool _useCustomWeaponOffset = false;
		bool _useCustomOffHandOffset = false;

		bool _offHandGripping;
		bool _repositionButtonHolding = false;
		bool _inRepositionMode = false;
		bool _repositionModeSwitched = false;
		repositionMode _repositionMode = repositionMode::weapon;
		uint64_t _repositionButtonHoldStart = 0;
		NiPoint3 _startFingerBonePos = NiPoint3(0, 0, 0);
		NiPoint3 _endFingerBonePos = NiPoint3(0, 0, 0);
		NiPoint3 _offsetPreview = NiPoint3(0, 0, 0);
		bool _hasLetGoGripButton;
		bool _hasLetGoRepositionButton = false;
		bool _hasLetGoZoomModeButton = false;
		bool _zoomModeButtonHeld = false;
		std::string _lastWeapon = "";
		Quaternion _aimAdjust;
		uint64_t _lastLookingAtPip = 0;

		NiMatrix43 _originalWeaponRot;
		NiPoint3 _offhandPos {0, 0, 0};
		NiTransform _offhandOffset; // Saving as NiTransform in case we need rotation in future
		NiPoint3 msgData{ 0, 0, 0 }; // used for msg passing

		NiTransform _rightHandPrevFrame;
		NiTransform _leftHandPrevFrame;
		NiTransform _pipboyScreenPrevFrame;
	};
}
