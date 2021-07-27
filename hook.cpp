#include "hook.h"
#include "F4VRBody.h"

#include "xbyak/xbyak.h"


//RelocAddr<uintptr_t> hookBeforeRenderer(0xd844bc);   // this hook didn't work as only a few nodes get moved
RelocAddr<uintptr_t> hookBeforeRenderer(0x1C21156);    // This hook is in member function 0x33 for BSFlattenedBoneTree right before it updates it's own data buffer of all the skeleton world transforms.   I think that buffer is what actually gets rendered

RelocAddr<uintptr_t> hookMainLoopFunc(0xd8187e);

RelocAddr<uintptr_t> hookAnimationVFunc(0xf2f0a8);  // This is PostUpdateAnimationGraphManager virtual function that updates the player skeleton below the hmd. 

RelocAddr<uintptr_t> hookPlayerUpdate(0xf1004c);

RelocAddr<uintptr_t> hookBoneTreeUpdate(0xd84ee4);

RelocAddr<uintptr_t> hookEndUpdate(0xd84f2c);


typedef void(*_hookedFunc)(uint64_t param1, uint64_t param2, uint64_t param3);
RelocAddr<_hookedFunc> hookedFunc(0x1C18620);

typedef void(*_hookedMainLoop)();
RelocAddr<_hookedMainLoop> hookedMainLoop(0xd83ac0);

typedef void(*_hookedf10ed0)(uint64_t pc);
RelocAddr<_hookedf10ed0> hookedf10ed0(0xf10ed0);

typedef void(*_hookedda09a0)(uint64_t parm);
RelocAddr<_hookedda09a0> hookedda09a0(0xda09a0);

typedef void(*_hooked1c22fb0)(uint64_t a, uint64_t b);
RelocAddr<_hooked1c22fb0> hooked1c22fb0(0x1c22fb0);

RelocAddr<uint64_t> wandMesh(0x2d686d8);

void hookIt(uint64_t rcx) {
	uint64_t parm = rcx;
	F4VRBody::update();
	//hookedf10ed0((uint64_t)(*g_player));    // this function does the final body updates and does some stuff with the world bound to reporting up the parent tree.   

	// so all of this below is an attempt to bypass the functionality in game around my hook at resets the root parent node's world pos which screws up armor
	// we still need to call the fucntion i hooked below to get some things ready for the renderer however starting with the named "Root" node instead of it's parent preseves locations
	if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
		uint64_t arr[5] = { 0, 0, 0, 0, 0 };
		uint64_t body = (uint64_t)&(*(*g_player)->unkF0->rootNode->m_children.m_data[0]);
		arr[1] = body + 0x180;
		arr[2] = 0x800;
		arr[3] = 2;
		arr[4] = 0x3c0c1400;
		hooked1c22fb0(body, (uint64_t)&arr);
	}

	hookedda09a0(parm);
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


	// replace mesh pointer string
	const char* mesh = "Data\\Meshes\\FRIK\\_primaryWand.nif";
	
	for (int i = 0; i < strlen(mesh); ++i) {
		SafeWrite8(wandMesh.GetUIntPtr() + i, mesh[i]);
	}

	int bytesToNOP = 0x1FF;

	for (int i = 0; i < bytesToNOP; ++i) {
		SafeWrite8(hookAnimationVFunc.GetUIntPtr() + i, 0x90);    // this block resets the body pose to hang off the camera.    Blocking this off so body height is correct.
	}

//	g_branchTrampoline.Write5Call(hookAnimationVFunc.GetUIntPtr(), (uintptr_t)&F4VRBody::update);

	g_branchTrampoline.Write5Call(hookEndUpdate.GetUIntPtr(), (uintptr_t)&hookIt);

//	_MESSAGE("hooking main loop function");
//	g_branchTrampoline.Write5Call(hookMainLoopFunc.GetUIntPtr(), (uintptr_t)updateCounter);
//	_MESSAGE("successfully hooked main loop");
}


