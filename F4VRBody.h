#pragma once
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/NiNodes.h"
#include "f4se/NiObjects.h"
#include "NiCloneProcess.h"
#include "f4se/BSGeometry.h"
#include "f4se/GameSettings.h"
#include "f4se/GameMenus.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusVM.h"
//#include "f4se/GameForms.h"

#include "include/SimpleIni.h"
#include "SmoothMovementVR.h"
#include "Offsets.h"

#include <windows.h>

extern uint64_t g_mainLoopCounter;
extern PluginHandle g_pluginHandle;
extern F4SEPapyrusInterface* g_papyrus;
extern F4SEMessagingInterface* g_messaging;



namespace F4VRBody {

	extern float c_playerHeight;
	extern float c_playerOffset_forward;
	extern float c_playerOffset_up;
	extern float c_pipboyDetectionRange;
	extern float c_fVrScale;
	extern float c_armLength;
	extern float c_cameraHeight;
	extern float c_dynamicCameraHeight;
	extern bool  c_showPAHUD;
	extern bool  c_hidePipboy;
	extern bool  c_selfieMode;
	extern bool  c_verbose;
	extern bool  c_armsOnly;
	extern bool  c_leftHandedMode;
	extern bool  c_disableSmoothMovement;
	extern bool  c_jumping;
	extern bool  c_leftHandedPipBoy;
	extern float c_powerArmor_forward;
	extern float c_powerArmor_up;
	extern float c_PACameraHeight;
	extern float c_PARootOffset;
	extern float c_RootOffset;
	extern float c_pipBoyScale;
	extern bool  c_staticGripping;
	extern float c_pipBoyLookAtGate;
	extern float c_gripLetGoThreshold;
	extern bool c_isLookingThroughScope;
	extern bool c_pipBoyButtonMode;
	extern bool c_pipBoyAllowMovementNotLooking;
	extern int c_pipBoyButtonArm;
	extern int c_pipBoyButtonID;
	extern int c_pipBoyButtonOffArm;
	extern int c_pipBoyButtonOffID;
	extern int c_gripButtonID;
	extern int c_holdDelay;
	extern int c_pipBoyOffDelay;
	extern bool c_repositionMasterMode;
	extern int c_repositionButtonID;
	extern int c_offHandActivateButtonID;
	extern bool c_enableOffHandGripping;
	extern bool c_enableGripButtonToGrap;
	extern bool c_enableGripButtonToLetGo;
	extern bool c_onePressGripButton;
	extern float c_scopeAdjustDistance;
	extern bool c_dampenHands;
	extern float c_dampenHandsRotation;
	extern float c_dampenHandsTranslation;
	extern bool c_dampenPipboyScreen;
	extern float c_dampenPipboyRotation;
	extern float c_dampenPipboyTranslation;
	extern bool c_IsHoloPipboy;
	extern bool c_IsPipBoyTorchOnArm;
	extern int c_SwitchTorchButton;
	extern bool c_CalibrateModeActive;
	extern float c_DirectionalDeadzone;
	extern bool _controlSleepStickyX;
	extern bool _controlSleepStickyY;
	extern bool _controlSleepStickyT;
	extern bool c_switchUIControltoPrimary;
	extern float c_armLengthbkup;
	extern float c_powerArmor_upbkup;
	extern float c_playerOffset_upbkup;
	extern float c_RootOffsetbkup;
	extern float c_PARootOffsetbkup;
	extern float c_fVrScalebkup;
	extern float c_playerOffset_forwardbkup;
	extern float c_powerArmor_forwardbkup;
	extern float c_cameraHeightbkup;
	extern float c_PACameraHeightbkup;
	extern float c_PlayerHMDHeight;
	extern float lastCamZ;
	extern float c_shouldertoHMD;

	class BoneSphere {
	public:
		BoneSphere() {
			radius = 0;
			bone = nullptr;
			stickyRight = false;
			stickyLeft = false;
			turnOnDebugSpheres = false;
			offset.x = 0;
			offset.y = 0;
			offset.z = 0;
			debugSphere = nullptr;
		}

		BoneSphere(float a_radius, NiNode* a_bone, NiPoint3 a_offset) : radius(a_radius), bone(a_bone), offset(a_offset) {
			stickyRight = false;
			stickyLeft = false;
			turnOnDebugSpheres = false;
			debugSphere = nullptr;
		}

		float radius;
		NiNode* bone;
		NiPoint3 offset;
		bool stickyRight;
		bool stickyLeft;
		bool turnOnDebugSpheres;
		NiNode* debugSphere;
	};

	enum BoneSphereEvent {
		BoneSphereEvent_None = 0,
		BoneSphereEvent_Enter = 1,
		BoneSphereEvent_Exit = 2,
		BoneSphereEvent_Holster = 3,
		BoneSphereEvent_Draw  = 4
	};

	enum BIPED_SLOTS {
		slot_None = 0,
		slot_HairTop = 1 << 0,
		slot_HairLong = 1 << 1,
		slot_Head = 1 << 2,
		slot_Body = 1 << 3,
		slot_LHand = 1 << 4,
		slot_RHand = 1 << 5,
		slot_UTorso = 1 << 6,
		slot_ULArm = 1 << 7,
		slot_URArm = 1 << 8,
		slot_ULLeg = 1 << 9,
		slot_URLeg = 1 << 10,
		slot_ATorso = 1 << 11,
		slot_ALArm = 1 << 12,
		slot_ARArm = 1 << 13,
		slot_ALLeg = 1 << 14,
		slot_ARLeg = 1 << 15,
		slot_Headband = 1 << 16,
		slot_Eyes = 1 << 17,
		slot_Beard = 1 << 18,
		slot_Mouth = 1 << 19,
		slot_Neck = 1 << 20,
		slot_Ring = 1 << 21,
		slot_Scalp = 1 << 22,
		slot_Decapitation = 1 << 23,
		slot_Unnamed1 = 1 << 24,
		slot_Unnamed2 = 1 << 25,
		slot_Unnamed3 = 1 << 26,
		slot_Unnamed4 = 1 << 27,
		slot_Unnamed5 = 1 << 28,
		slot_Shield = 1 << 29,
		slot_Pipboy = 1 << 30,
		slot_FX = 1 << 31
	};


	NiNode* loadNifFromFile(char* path);

	bool loadConfig();

	void smoothMovement();
	void update();
	void startUp();
	void saveSettings();
	void swapPB();
	// Native funcs to expose to papyrus
	bool HasKeyword();
	void saveStates(StaticFunctionTag* base);
	void calibrate(StaticFunctionTag* base);
	void togglePipboyVis(StaticFunctionTag* base);
	void toggleSelfieMode(StaticFunctionTag* base);
	void makeTaller(StaticFunctionTag* base);
	void makeShorter(StaticFunctionTag* base);
	void moveUp(StaticFunctionTag* base);
	void moveDown(StaticFunctionTag* base);
	void moveForward(StaticFunctionTag* base);
	void moveBackward(StaticFunctionTag* base);
	void increaseScale(StaticFunctionTag* base);
	void decreaseScale(StaticFunctionTag* base);
	void holsterWeapon();
	void drawWeapon();
	bool RegisterFuncs(VirtualMachine* vm);

	inline NiNode* loadNifFromFile(char* path) {
		uint64_t flags[2];
		flags[0] = 0x0;
		flags[1] = 0xed | 0x2d;
		uint64_t mem = 0;
		int ret = Offsets::loadNif((uint64_t)&(*path), (uint64_t)&mem, (uint64_t)&flags);

		return (NiNode*)mem;
	}


}