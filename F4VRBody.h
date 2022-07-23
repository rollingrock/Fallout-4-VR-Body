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

#include "include/SimpleIni.h"
#include "SmoothMovementVR.h"

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
	extern int c_repositionButtonID;
	extern int c_offHandActivateButtonID;
	extern bool c_enableOffHandGripping;
	extern bool c_enableGripButtonToGrap;
	extern bool c_enableGripButtonToLetGo;
	extern bool c_onePressGripButton;

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
		BoneSphereEvent_Exit = 2
	};


	NiNode* loadNifFromFile(char* path);
		
	bool loadConfig();

	void update();
	void startUp();

	// Native funcs to expose to papyrus

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


	bool RegisterFuncs(VirtualMachine* vm);

}