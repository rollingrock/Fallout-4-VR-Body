// ReSharper disable CppClangTidyBugproneReservedIdentifier
// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier

#include <F4SE_common/BranchTrampoline.h>
#include <F4SE_common/SafeWrite.h>

#include "hook.h"
#include "F4VRBody.h"
#include "GunReload.h"
#include "f4se/GameCamera.h"
#include "f4se/GameReferences.h"

#include "xbyak/xbyak.h"

//RelocAddr<uintptr_t> hookBeforeRenderer(0xd844bc);   // this hook didn't work as only a few nodes get moved
RelocAddr<uintptr_t> hookBeforeRenderer(0x1C21156);
// This hook is in member function 0x33 for BSFlattenedBoneTree right before it updates it's own data buffer of all the skeleton world transforms.   I think that buffer is what actually gets rendered

//RelocAddr<uintptr_t> hookMainLoopFunc(0xd8187e);   // now using in fo4vr better scopes

RelocAddr<uintptr_t> hookAnimationVFunc(0xf2f0a8); // This is PostUpdateAnimationGraphManager virtual function that updates the player skeleton below the hmd. 

RelocAddr<uintptr_t> hookPlayerUpdate(0xf1004c);

RelocAddr<uintptr_t> hookBoneTreeUpdate(0xd84ee4);

RelocAddr<uintptr_t> hookEndUpdate(0xd84f2c);
RelocAddr<uintptr_t> hookMainDrawCandidate(0xd844bc);
RelocAddr<uintptr_t> hookMainDrawandUi(0xd87ace);

using _hookedFunc = void(*)(uint64_t param1, uint64_t param2, uint64_t param3);
RelocAddr<_hookedFunc> hookedFunc(0x1C18620);

using _hookedPosPlayerFunc = void(*)(double param1, double param2, double param3);
RelocAddr<_hookedPosPlayerFunc> hookedPosPlayerFunc(0x2841530);

using _hookedMainDrawCandidateFunc = void(*)(uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4);
RelocAddr<_hookedMainDrawCandidateFunc> hookedMainDrawCandidateFunc(0xd831f0);

//typedef void(*_hookedMainLoop)();
//RelocAddr<_hookedMainLoop> hookedMainLoop(0xd83ac0);

using _hookedf10ed0 = void(*)(uint64_t pc);
RelocAddr<_hookedf10ed0> hookedf10ed0(0xf10ed0);

using _hookedda09a0 = void(*)(uint64_t parm);
RelocAddr<_hookedda09a0> hookedda09a0(0xda09a0);

using _hooked1c22fb0 = void(*)(uint64_t a, uint64_t b);
RelocAddr<_hooked1c22fb0> hooked1c22fb0(0x1c22fb0);

using _main_update_player = void(*)(uint64_t rcx, uint64_t rdx);
RelocAddr<_main_update_player> main_update_player(0x1c22fb0);
RelocAddr<uintptr_t> hookMainUpdatePlayer(0x0f0ff6a);

using _hookMultiBoundCullingFunc = void(*)();
RelocAddr<_hookMultiBoundCullingFunc> hookMultiBoundCullingFunc(0x0d84930);
RelocAddr<uintptr_t> hookMultiBoundCulling(0x0d8445d);

using _smoothMovementHook = void(*)(uint64_t rcx);
RelocAddr<_smoothMovementHook> smoothMovementHook(0x1ba7ba0);
RelocAddr<uintptr_t> hook_smoothMovementHook(0xd83ec4);

using _someRandomFunc = void(*)(uint64_t rcx);
RelocAddr<_someRandomFunc> someRandomFunc(0xd3c820);
RelocAddr<uintptr_t> hookSomeRandomFunc(0xd8405e);

using _Actor_ReEquipAll = void(*)(Actor* a_actor);
RelocAddr<_Actor_ReEquipAll> Actor_ReEquipAll(0xddf050);
RelocAddr<uintptr_t> hookActor_ReEquipAllExit(0xf01528);

using _ExtraData_SetMultiBoundRef = void(*)(std::uint64_t rcx, std::uint64_t rdx);
RelocAddr<_ExtraData_SetMultiBoundRef> ExtraData_SetMultiBoundRef(0x91320);
RelocAddr<uintptr_t> hookExtraData_SetMultiBoundRef(0xf00dc6);

using _Actor_GetCurrentWeapon = uint64_t(*)(uint64_t rcx, uint64_t rdx, uint64_t r8);
RelocAddr<_Actor_GetCurrentWeapon> Actor_GetCurrentWeapon(0xe50da0);
RelocAddr<uintptr_t> hookActor_GetCurrentWeaponForGunReload(0xf3027c);

using _TESObjectREFR_SetupAnimationUpdateDataForRefernce = uint64_t(*)(uint64_t rcx, float* rdx);
RelocAddr<_TESObjectREFR_SetupAnimationUpdateDataForRefernce> TESObjectREFR_SetupAnimationUpdateDataForRefernce(0x4189c0);
RelocAddr<uintptr_t> hookActor_SetupAnimationUpdateDataForRefernce(0xf0fbdf);

using _AIProcess_Set3DUpdateFlags = void(*)(Actor::MiddleProcess* rcx, int rdx);
RelocAddr<_AIProcess_Set3DUpdateFlags> AIProcess_Set3DUpdateFlags(0xec8ce0);

// Gun Reload Init
uint64_t gunReloadInit(const uint64_t rcx, const uint64_t rdx, const uint64_t r8) {
	// frik::g_gunReloadSystem->startAnimationCapture();
	return Actor_GetCurrentWeapon(rcx, rdx, r8);
}

uint64_t updatePlayerAnimationHook(const uint64_t rcx, float* rdx) {
	if (frik::g_animDeltaTime >= 0.0f) {
		rdx[0] = frik::g_animDeltaTime;
	}
	return TESObjectREFR_SetupAnimationUpdateDataForRefernce(rcx, rdx);
}

// fix powerarmor 3d mesh hooks

void fixPA3D() {
	Actor_ReEquipAll(*g_player);
	AIProcess_Set3DUpdateFlags((*g_player)->middleProcess, 0x520);
}

void fixPA3DEnter(const std::uint64_t rcx, const std::uint64_t rdx) {
	ExtraData_SetMultiBoundRef(rcx, rdx);
	AIProcess_Set3DUpdateFlags((*g_player)->middleProcess, 0x520);
}

// renderer stuff

void RendererEnable(const std::uint64_t a_ptr, const bool a_bool) {
	using func_t = decltype(&RendererEnable);
	RelocAddr<func_t> func(0x0b00150);
	return func(a_ptr, a_bool);
}

std::uint64_t RendererGetByName(const BSFixedString& a_name) {
	using func_t = decltype(&RendererGetByName);
	RelocAddr<func_t> func(0x0b00270);
	return func(a_name);
}

RelocAddr<uint64_t> wandMesh(0x2d686d8);

void hookIt(const uint64_t rcx) {
	const uint64_t parm = rcx;
	frik::update();
	//hookedf10ed0((uint64_t)(*g_player));    // this function does the final body updates and does some stuff with the world bound to reporting up the parent tree.   

	// so all of this below is an attempt to bypass the functionality in game around my hook at resets the root parent node's world pos which screws up armor
	// we still need to call the function i hooked below to get some things ready for the renderer however starting with the named "Root" node instead of it's parent preseves locations
	if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
		if ((*g_player)->unkF0->rootNode->m_children.m_emptyRunStart > 0) {
			if ((*g_player)->unkF0->rootNode->m_children.m_data[0]) {
				uint64_t arr[5] = {0, 0, 0, 0, 0};
				const uint64_t body = (uint64_t)(*g_player)->unkF0->rootNode->m_children.m_data[0];
				arr[1] = body + 0x180;
				arr[2] = 0x800;
				arr[3] = 2;
				arr[4] = 0x3c0c1400;
				hooked1c22fb0(body, (uint64_t)&arr);
			}
		}
	}

	hookedda09a0(parm);
}

void hook2(const uint64_t rcx, const uint64_t rdx, const uint64_t r8, const uint64_t r9) {
	frik::update();

	hookedMainDrawCandidateFunc(rcx, rdx, r8, r9);

	const BSFixedString name("ScopeMenu");

	const std::uint64_t renderer = RendererGetByName(name);

	if (renderer) {
		//		RendererEnable(renderer, false);
	}
}

void hook5(const uint64_t rcx) {
	frik::update();

	someRandomFunc(rcx);

	const BSFixedString name("ScopeMenu");

	const std::uint64_t renderer = RendererGetByName(name);

	if (renderer) {
		//		RendererEnable(renderer, false);
	}
}

void hook3(const double param1, const double param2, const double param3) {
	hookedPosPlayerFunc(param1, param2, param3);
	frik::update();
}

void hook4() {
	frik::update();
	hookMultiBoundCullingFunc();
}

void hookSmoothMovement(const uint64_t rcx) {
	if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
		frik::smoothMovement();
	}
	smoothMovementHook(rcx);
}

void hook_main_update_player(const uint64_t rcx, const uint64_t rdx) {
	if (*g_player && (*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
		NiNode* body = (*g_player)->unkF0->rootNode->GetAsNiNode();
		body->m_localTransform.pos.x = (*g_playerCamera)->cameraNode->m_worldTransform.pos.x;
		body->m_localTransform.pos.y = (*g_playerCamera)->cameraNode->m_worldTransform.pos.y;
		body->m_worldTransform.pos.x = (*g_playerCamera)->cameraNode->m_worldTransform.pos.x;
		body->m_worldTransform.pos.y = (*g_playerCamera)->cameraNode->m_worldTransform.pos.y;

		//static BSFixedString pwn("PlayerWorldNode");
		//NiNode* pwn_node = (*g_player)->unkF0->rootNode->m_parent->GetObjectByName(&pwn)->GetAsNiNode();
		//body->m_localTransform.pos.z += pwn_node->m_localTransform.pos.z;
		//body->m_worldTransform.pos.z += pwn_node->m_localTransform.pos.z;
	}

	main_update_player(rcx, rdx);
}

void updateCounter() {
	//g_mainLoopCounter++;
	//hookedMainLoop();
}

void hookMain() {
	//Log::info("Hooking before main renderer");
	//	g_branchTrampoline.Write5Call(hookBeforeRenderer.GetUIntPtr(), (uintptr_t)hookIt);
	//Log::info("Successfully hooked before main renderer");

	// replace mesh pointer string
	const auto mesh = "Data\\Meshes\\FRIK\\_primaryWand.nif";

	for (int i = 0; i < strlen(mesh); ++i) {
		SafeWrite8(wandMesh.GetUIntPtr() + i, mesh[i]);
	}

	const int bytesToNOP = 0x1FF;

	for (int i = 0; i < bytesToNOP; ++i) {
		SafeWrite8(hookAnimationVFunc.GetUIntPtr() + i, 0x90); // this block resets the body pose to hang off the camera.    Blocking this off so body height is correct.
	}

	//	g_branchTrampoline.Write5Call(hookAnimationVFunc.GetUIntPtr(), (uintptr_t)&frik::update);

	//	g_branchTrampoline.Write5Call(hookEndUpdate.GetUIntPtr(), (uintptr_t)&hookIt);
	//g_branchTrampoline.Write5Call(hookMainDrawCandidate.GetUIntPtr(), (uintptr_t)&hook2);
	//	g_branchTrampoline.Write5Call(hookMultiBoundCulling.GetUIntPtr(), (uintptr_t)&hook4);
	g_branchTrampoline.Write5Call(hookSomeRandomFunc.GetUIntPtr(), (uintptr_t)&hook5);

	g_branchTrampoline.Write5Call(hookMainUpdatePlayer.GetUIntPtr(), (uintptr_t)&hook_main_update_player);
	g_branchTrampoline.Write5Call(hook_smoothMovementHook.GetUIntPtr(), (uintptr_t)&hookSmoothMovement);

	g_branchTrampoline.Write5Call(hookActor_ReEquipAllExit.GetUIntPtr(), (uintptr_t)&fixPA3D);
	g_branchTrampoline.Write5Call(hookExtraData_SetMultiBoundRef.GetUIntPtr(), (uintptr_t)&fixPA3DEnter);
	g_branchTrampoline.Write5Call(hookActor_GetCurrentWeaponForGunReload.GetUIntPtr(), (uintptr_t)&gunReloadInit);
	g_branchTrampoline.Write5Call(hookActor_SetupAnimationUpdateDataForRefernce.GetUIntPtr(), (uintptr_t)&updatePlayerAnimationHook);

	//	Log::info("hooking main loop function");
	//	g_branchTrampoline.Write5Call(hookMainLoopFunc.GetUIntPtr(), (uintptr_t)updateCounter);
	//	Log::info("successfully hooked main loop");
}
