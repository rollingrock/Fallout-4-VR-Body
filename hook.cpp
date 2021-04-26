#include "hook.h"
#include "F4VRBody.h"

#include "xbyak/xbyak.h"


//RelocAddr<uintptr_t> hookBeforeRenderer(0xd844bc);   // this hook didn't work as only a few nodes get moved
RelocAddr<uintptr_t> hookBeforeRenderer(0x1C21156);    // This hook is in member function 0x33 for BSFlattenedBoneTree right before it updates it's own data buffer of all the skeleton world transforms.   I think that buffer is what actually gets rendered

RelocAddr<uintptr_t> hookMainLoopFunc(0xd8187e);

RelocAddr<uintptr_t> hookAnimationVFunc(0xf2f0a8);  // This is PostUpdateAnimationGraphManager virtual function that updates the player skeleton below the hmd. 

RelocAddr<uintptr_t> hookPlayerUpdate(0xf1004c);

RelocAddr<uintptr_t> hookBoneTreeUpdate(0xd84ee4);


typedef void(*_hookedFunc)(uint64_t param1, uint64_t param2, uint64_t param3);
RelocAddr<_hookedFunc> hookedFunc(0x1C18620);

typedef void(*_hookedMainLoop)();
RelocAddr<_hookedMainLoop> hookedMainLoop(0xd83ac0);

typedef void(*_hookedf10ed0)(uint64_t pc);
RelocAddr<_hookedf10ed0> hookedf10ed0(0xf10ed0);


void hookIt() {
	F4VRBody::update();
//	hookedf10ed0((uint64_t)(*g_player));
	return;
}

void updateCounter() {
	g_mainLoopCounter++;

	hookedMainLoop();
}

void hookMain() {
	//_MESSAGE("Hooking before main renderer");
//	g_branchTrampoline.Write5Call(hookBeforeRenderer.GetUIntPtr(), (uintptr_t)hookIt);
	//_MESSAGE("Successfully hooked before main renderer");

	int bytesToNOP = 0x1FF;

	for (int i = 0; i < bytesToNOP; ++i) {
		SafeWrite8(hookAnimationVFunc.GetUIntPtr() + i, 0x90);
	}

//	g_branchTrampoline.Write5Call(hookAnimationVFunc.GetUIntPtr(), (uintptr_t)&F4VRBody::update);

	g_branchTrampoline.Write5Call(hookBoneTreeUpdate.GetUIntPtr(), (uintptr_t)&hookIt);

	_MESSAGE("hooking main loop function");
	g_branchTrampoline.Write5Call(hookMainLoopFunc.GetUIntPtr(), (uintptr_t)updateCounter);
	_MESSAGE("successfully hooked main loop");
}


