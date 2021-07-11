#pragma once
#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusVM.h"
#include "include/SimpleIni.h"

extern uint64_t g_mainLoopCounter;
namespace F4VRBody {

	extern float c_playerHeight;
	extern float c_playerOffset_forward;
	extern float c_playerOffset_up;
	extern float c_pipboyDetectionRange;
	extern float c_armLength;

	bool loadConfig();

	void update();
	void startUp();

	// Native funcs to expose to papyrus

	void saveStates(StaticFunctionTag* base);
	void calibrate(StaticFunctionTag* base);
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