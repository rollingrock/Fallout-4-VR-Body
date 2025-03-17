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

#include "SmoothMovementVR.h"
#include "Offsets.h"
#include "Pipboy.h"
#include "ConfigurationMode.h"
#include "CullGeometryHandler.h"
#include "BoneSpheresHandler.h"
#include "WeaponPositionHandler.h"

#include <windows.h>

extern PluginHandle g_pluginHandle;
extern F4SEPapyrusInterface* g_papyrus;
extern F4SEMessagingInterface* g_messaging;


namespace F4VRBody {
	
	extern Pipboy* g_pipboy;
	extern ConfigurationMode* g_configurationMode;
	extern CullGeometryHandler* g_cullGeometry;
	extern BoneSpheresHandler* g_boneSpheres;
	extern WeaponPositionHandler* g_weaponPosition;

	// TODO: bad global state variable that should be refactored
	extern bool c_isLookingThroughScope;
	extern bool c_jumping;
	extern float c_dynamicCameraHeight;
	extern bool c_selfieMode;
	extern bool GameVarsConfigured;


	NiNode* loadNifFromFile(char* path);

	void smoothMovement();
	void update();
	void startUp();
	// Native funcs to expose to papyrus
	bool registerPapyrusFuncs(VirtualMachine* vm);

	inline NiNode* loadNifFromFile(char* path) {
		uint64_t flags[2];
		flags[0] = 0x0;
		flags[1] = 0xed | 0x2d;
		uint64_t mem = 0;
		int ret = Offsets::loadNif((uint64_t)&(*path), (uint64_t)&mem, (uint64_t)&flags);

		return (NiNode*)mem;
	}
}