// ReSharper disable CppClangTidyBugproneReservedIdentifier
// ReSharper disable CppClangTidyClangDiagnosticReservedIdentifier

// #include <F4SE_common/BranchTrampoline.h>
// #include <F4SE_common/SafeWrite.h>

#include "hook.h"
#include "FRIK.h"

#include "xbyak/xbyak.h"

//REL::Relocation<uintptr_t> hookBeforeRenderer(REL::Offset(0xd844bc));   // this hook didn't work as only a few nodes get moved
REL::Relocation hookBeforeRenderer(REL::Offset(0x1C21156));
// This hook is in member function REL::Offset(0x33) for BSFlattenedBoneTree right before it updates it's own data buffer of all the skeleton world transforms.   I think that buffer is what actually gets rendered

//REL::Relocation<uintptr_t> hookMainLoopFunc(REL::Offset(0xd8187e));   // now using in fo4vr better scopes

REL::Relocation hookAnimationVFunc(REL::Offset(0xf2f0a8)); // This is PostUpdateAnimationGraphManager virtual function that updates the player skeleton below the hmd.

REL::Relocation hookPlayerUpdate(REL::Offset(0xf1004c));

REL::Relocation hookBoneTreeUpdate(REL::Offset(0xd84ee4));

REL::Relocation hookEndUpdate(REL::Offset(0xd84f2c));
REL::Relocation hookMainDrawCandidate(REL::Offset(0xd844bc));
REL::Relocation hookMainDrawandUi(REL::Offset(0xd87ace));

using _hookedFunc = void(*)(uint64_t param1, uint64_t param2, uint64_t param3);
REL::Relocation<_hookedFunc> hookedFunc(REL::Offset(0x1C18620));

using _hookedPosPlayerFunc = void(*)(double param1, double param2, double param3);
REL::Relocation<_hookedPosPlayerFunc> hookedPosPlayerFunc(REL::Offset(0x2841530));

using _hookedMainDrawCandidateFunc = void(*)(uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4);
REL::Relocation<_hookedMainDrawCandidateFunc> hookedMainDrawCandidateFunc(REL::Offset(0xd831f0));

//typedef void(*_hookedMainLoop)();
//REL::Relocation<_hookedMainLoop> hookedMainLoop(REL::Offset(0xd83ac0));

using _hookedf10ed0 = void(*)(uint64_t pc);
REL::Relocation<_hookedf10ed0> hookedf10ed0(REL::Offset(0xf10ed0));

using _hookedda09a0 = void(*)(uint64_t parm);
REL::Relocation<_hookedda09a0> hookedda09a0(REL::Offset(0xda09a0));

using _hooked1c22fb0 = void(*)(uint64_t a, uint64_t b);
REL::Relocation<_hooked1c22fb0> hooked1c22fb0(REL::Offset(0x1c22fb0));

using _main_update_player = void(*)(uint64_t rcx, uint64_t rdx);
REL::Relocation<_main_update_player> main_update_player(REL::Offset(0x1c22fb0));
REL::Relocation hookMainUpdatePlayer(REL::Offset(0x0f0ff6a));

using _hookMultiBoundCullingFunc = void(*)();
REL::Relocation<_hookMultiBoundCullingFunc> hookMultiBoundCullingFunc(REL::Offset(0x0d84930));
REL::Relocation hookMultiBoundCulling(REL::Offset(0x0d8445d));

using _smoothMovementHook = void(*)(uint64_t rcx);
REL::Relocation<_smoothMovementHook> smoothMovementHook(REL::Offset(0x1ba7ba0));
REL::Relocation hook_smoothMovementHook(REL::Offset(0xd83ec4));

using _someRandomFunc = void(*)(uint64_t rcx);
REL::Relocation<_someRandomFunc> someRandomFunc(REL::Offset(0xd3c820));
REL::Relocation hookSomeRandomFunc(REL::Offset(0xd8405e));

using _Actor_ReEquipAll = void(*)(F4SEVR::Actor* a_actor);
REL::Relocation<_Actor_ReEquipAll> Actor_ReEquipAll(REL::Offset(0xddf050));
REL::Relocation hookActor_ReEquipAllExit(REL::Offset(0xf01528));

using _ExtraData_SetMultiBoundRef = void(*)(std::uint64_t rcx, std::uint64_t rdx);
REL::Relocation<_ExtraData_SetMultiBoundRef> ExtraData_SetMultiBoundRef(REL::Offset(0x91320));
REL::Relocation hookExtraData_SetMultiBoundRef(REL::Offset(0xf00dc6));

using _Actor_GetCurrentWeapon = uint64_t(*)(uint64_t rcx, uint64_t rdx, uint64_t r8);
REL::Relocation<_Actor_GetCurrentWeapon> Actor_GetCurrentWeapon(REL::Offset(0xe50da0));
REL::Relocation hookActor_GetCurrentWeaponForGunReload(REL::Offset(0xf3027c));

using _TESObjectREFR_SetupAnimationUpdateDataForRefernce = uint64_t(*)(uint64_t rcx, float* rdx);
REL::Relocation<_TESObjectREFR_SetupAnimationUpdateDataForRefernce> TESObjectREFR_SetupAnimationUpdateDataForRefernce(REL::Offset(0x4189c0));
REL::Relocation hookActor_SetupAnimationUpdateDataForRefernce(REL::Offset(0xf0fbdf));

using _AIProcess_Set3DUpdateFlags = void(*)(F4SEVR::Actor::MiddleProcess* rcx, int rdx);
REL::Relocation<_AIProcess_Set3DUpdateFlags> AIProcess_Set3DUpdateFlags(REL::Offset(0xec8ce0));

// Gun Reload Init
uint64_t gunReloadInit(const uint64_t rcx, const uint64_t rdx, const uint64_t r8)
{
    // frik::g_gunReloadSystem->startAnimationCapture();
    return Actor_GetCurrentWeapon(rcx, rdx, r8);
}

uint64_t updatePlayerAnimationHook(const uint64_t rcx, float* rdx)
{
    // Use in gun reload
    // if (frik::g_animDeltaTime >= 0.0f) {
    //     rdx[0] = frik::g_animDeltaTime;
    // }
    return TESObjectREFR_SetupAnimationUpdateDataForRefernce(rcx, rdx);
}

// fix powerarmor 3d mesh hooks

void fixPA3D()
{
    const auto player = f4vr::getPlayer();
    Actor_ReEquipAll(player);
    AIProcess_Set3DUpdateFlags(player->middleProcess, 0x520);
}

void fixPA3DEnter(const std::uint64_t rcx, const std::uint64_t rdx)
{
    const auto player = f4vr::getPlayer();
    ExtraData_SetMultiBoundRef(rcx, rdx);
    AIProcess_Set3DUpdateFlags(player->middleProcess, 0x520);
}

// renderer stuff

void RendererEnable(const std::uint64_t a_ptr, const bool a_bool)
{
    using func_t = decltype(&RendererEnable);
    REL::Relocation<func_t> func(REL::Offset(0x0b00150));
    return func(a_ptr, a_bool);
}

std::uint64_t RendererGetByName(const RE::BSFixedString& a_name)
{
    using func_t = decltype(&RendererGetByName);
    REL::Relocation<func_t> func(REL::Offset(0x0b00270));
    return func(a_name);
}

REL::Relocation wandMesh(REL::Offset(0x2d686d8));

void hookIt(const uint64_t rcx)
{
    const uint64_t parm = rcx;
    frik::g_frik.onFrameUpdate();
    //hookedf10ed0((uint64_t)player);    // this function does the final body updates and does some stuff with the world bound to reporting up the parent tree.

    // so all of this below is an attempt to bypass the functionality in game around my hook at resets the root parent node's world pos which screws up armor
    // we still need to call the function i hooked below to get some things ready for the renderer however starting with the named "Root" node instead of it's parent preseves locations
    const auto player = f4vr::getPlayer();
    if (player->unkF0) {
        const auto rootNode = player->unkF0->rootNode;
        if (rootNode && !rootNode->children.empty()) {
            if (rootNode->children[0]) {
                uint64_t arr[5] = { 0, 0, 0, 0, 0 };
                const uint64_t body = (uint64_t)rootNode->children[0].get();
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

void hook2(const uint64_t rcx, const uint64_t rdx, const uint64_t r8, const uint64_t r9)
{
    frik::g_frik.onFrameUpdate();

    hookedMainDrawCandidateFunc(rcx, rdx, r8, r9);

    const RE::BSFixedString name("ScopeMenu");

    const std::uint64_t renderer = RendererGetByName(name);

    if (renderer) {
        //		RendererEnable(renderer, false);
    }
}

void hook5(const uint64_t rcx)
{
    frik::g_frik.onFrameUpdate();
    someRandomFunc(rcx);

    // const RE::BSFixedString name("ScopeMenu");
    // const std::uint64_t renderer = RendererGetByName(name);
    // if (renderer) {
    //     //		RendererEnable(renderer, false);
    // }
}

void hook3(const double param1, const double param2, const double param3)
{
    hookedPosPlayerFunc(param1, param2, param3);
    frik::g_frik.onFrameUpdate();
}

void hook4()
{
    frik::g_frik.onFrameUpdate();
    hookMultiBoundCullingFunc();
}

void hookSmoothMovement(const uint64_t rcx)
{
    frik::g_frik.smoothMovement();
    smoothMovementHook(rcx);
}

void hook_main_update_player(const uint64_t rcx, const uint64_t rdx)
{
    const auto player = f4vr::getPlayer();
    const auto playerCamera = f4vr::getPlayerCamera();
    if (player && playerCamera && playerCamera->cameraNode && player->unkF0 && player->unkF0->rootNode) {
        const auto body = player->unkF0->rootNode;
        const auto& cameraPos = playerCamera->cameraNode->world.translate;
        body->local.translate.x = cameraPos.x;
        body->local.translate.y = cameraPos.y;
        body->world.translate.x = cameraPos.x;
        body->world.translate.y = cameraPos.y;

        //static RE::BSFixedString pwn("PlayerWorldNode");
        //RE::NiNode* pwn_node = player->unkF0->rootNode->parent->GetObjectByName(&pwn)->GetAsRE::NiNode();
        //body->local.translate.z += pwn_node->local.translate.z;
        //body->world.translate.z += pwn_node->local.translate.z;
    }

    main_update_player(rcx, rdx);
}

void updateCounter()
{
    //g_mainLoopCounter++;
    //hookedMainLoop();
}

void hookMain()
{
    //logger::info("Hooking before main renderer");
    //	trampoline.write_call<5>(hookBeforeRenderer.address(), (uintptr_t)hookIt);
    //logger::info("Successfully hooked before main renderer");

    // replace mesh pointer string (replaces HP,Ammo,etc. UI to use nif that puts it on the back of the hand)
    const auto mesh = R"(Data\Meshes\FRIK\_primaryWand.nif)";
    for (int i = 0; i < strlen(mesh); ++i) {
        REL::safe_write(wandMesh.address() + i, mesh[i]);
    }

    const int bytesToNOP = 0x1FF;
    for (int i = 0; i < bytesToNOP; ++i) {
        // this block resets the body pose to hang off the camera. Blocking this off so body height is correct.
        REL::safe_write(hookAnimationVFunc.address() + i, static_cast<uint8_t>(0x90));
    }

    //	trampoline.write_call<5>(hookAnimationVFunc.address(), (uintptr_t)&frik::update);

    //	trampoline.write_call<5>(hookEndUpdate.address(), (uintptr_t)&hookIt);
    //trampoline.write_call<5>(hookMainDrawCandidate.address(), (uintptr_t)&hook2);
    //	trampoline.write_call<5>(hookMultiBoundCulling.address(), (uintptr_t)&hook4);

    auto& trampoline = F4SE::GetTrampoline();

    // trampoline.write_call<5>(hookSomeRandomFunc.address(), &hook5);

    trampoline.write_call<5>(hookMainUpdatePlayer.address(), &hook_main_update_player);
    trampoline.write_call<5>(hook_smoothMovementHook.address(), &hookSmoothMovement);

    trampoline.write_call<5>(hookActor_ReEquipAllExit.address(), &fixPA3D);
    trampoline.write_call<5>(hookExtraData_SetMultiBoundRef.address(), &fixPA3DEnter);

    // trampoline.write_call<5>(hookActor_GetCurrentWeaponForGunReload.address(), &gunReloadInit);

    // gun reload animation hook
    // trampoline.write_call<5>(hookActor_SetupAnimationUpdateDataForRefernce.address(), &updatePlayerAnimationHook);

    //	logger::info("hooking main loop function");
    //	trampoline.write_call<5>(hookMainLoopFunc.address(), (uintptr_t)updateCounter);
    //	logger::info("successfully hooked main loop");
}
