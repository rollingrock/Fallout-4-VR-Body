#include "GameHooks.h"

#include "xbyak/xbyak.h"

#include "FRIK.h"

namespace
{
    // fix power-armor 3d mesh hooks
    void fixPA3D()
    {
        const auto player = f4vr::getPlayer();
        f4vr::Actor_ReEquipAll(player);
        f4vr::AIProcess_Set3DUpdateFlags(player->middleProcess, 0x520);
    }

    void fixPA3DEnter(const std::uint64_t rcx, const std::uint64_t rdx)
    {
        const auto player = f4vr::getPlayer();
        f4vr::ExtraData_SetMultiBoundRef(rcx, rdx);
        f4vr::AIProcess_Set3DUpdateFlags(player->middleProcess, 0x520);
    }

    // renderer stuff
    void RendererEnable(const std::uint64_t a_ptr, const bool a_bool)
    {
        using func_t = decltype(&RendererEnable);
        const REL::Relocation<func_t> func(REL::Offset(0x0b00150));
        return func(a_ptr, a_bool);
    }

    std::uint64_t RendererGetByName(const RE::BSFixedString& a_name)
    {
        using func_t = decltype(&RendererGetByName);
        const REL::Relocation<func_t> func(REL::Offset(0x0b00270));
        return func(a_name);
    }

    void hookSmoothMovement(const uint64_t rcx)
    {
        frik::g_frik.smoothMovement();
        f4vr::smoothMovementHook(rcx);
    }

    void hookMainUpdatePlayer(const uint64_t rcx, const uint64_t rdx)
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
        }

        f4vr::main_update_player(rcx, rdx);
    }

    /**
     * Replace mesh pointer string (replaces HP,Ammo,etc. UI to use nif that puts it on the back of the hand)
     */
    void replacePrimaryWandNif()
    {
        const auto mesh = R"(Data\Meshes\FRIK\_primaryWand.nif)";
        for (int i = 0; i < strlen(mesh); ++i) {
            REL::safe_write(f4vr::wandMesh.address() + i, mesh[i]);
        }
    }

    /**
     * this block resets the body pose to hang off the camera. Blocking this off so body height is correct.
     */
    void blockResetBodyPose()
    {
        const int bytesToNOP = 0x1FF;
        for (int i = 0; i < bytesToNOP; ++i) {
            REL::safe_write(f4vr::hookAnimationVFunc.address() + i, static_cast<uint8_t>(0x90));
        }
    }
}

namespace
{
    REL::Offset invJumpFrom(0x2567664);
    REL::Offset invJumpTo(0x256766a);
    REL::Offset toJumpFrom(0x1b932ea);
    REL::Offset toJumpTo(0x1b932f2);
    REL::Offset toJumpBreak(0x1b93315);

    REL::Offset lockForRead_branch(0x1b932f8);
    REL::Offset lockForRead_return(0x1b932fd);

    REL::Offset shaderEffectPatch(0x28d323a);
    REL::Offset shaderEffectCall(0x2813560);
    REL::Offset shaderEffectContinue(0x28d323f);
    REL::Offset shaderEffectReturn(0x28d4ec8);

    REL::Offset DropAddOnPatch1(0x03e9caf);
    REL::Offset DropAddOnPatch2(0x03e9cd3);
    REL::Offset DropAddOnPatch3(0x3e9df5);

    void patchInventoryInfBug()
    {
        struct PatchShortVar : Xbyak::CodeGenerator
        {
            PatchShortVar(void* buf) :
                CodeGenerator(32, buf)
            {
                Xbyak::Label retLab;

                and_(edi, 0xffff); // edi is an int but should be treated as a short.  Should allow for loop to exit.
                mov(r12d, 0xffff);
                jmp(ptr[rip + retLab]);

                L(retLab);
                dq(invJumpTo.address());
            }
        };

        void* buf = F4SE::GetTrampoline().allocate(32);
        const PatchShortVar code(buf);

        // Patch original code to jump to our patch
        F4SE::GetTrampoline().write_branch<6>(invJumpFrom.address(), std::uintptr_t(code.getCode()));
        logger::debug("Patched InventoryInfBug at 0x{:X}, size:{}", lockForRead_branch.address(), code.getSize());
    }

    void patchLockForReadMask()
    {
        struct PatchMoreMask : Xbyak::CodeGenerator
        {
            PatchMoreMask(void* buf) :
                CodeGenerator(64, buf)
            {
                Xbyak::Label retLab;

                and_(dword[rdi + 0x4], 0xFFFFFFF);
                mov(rcx, 1);
                jmp(ptr[rip + retLab]);

                L(retLab);
                dq(lockForRead_return.address());
            }
        };

        void* buf = F4SE::GetTrampoline().allocate(32);
        const PatchMoreMask code(buf);

        // Patch original code to jump to our patch
        F4SE::GetTrampoline().write_branch<5>(lockForRead_branch.address(), std::uintptr_t(code.getCode()));
        logger::debug("Patched LockForReadMask at 0x{:X}, size:{}", lockForRead_branch.address(), code.getSize());
    }

    void patchPipeGunScopeCrash()
    {
        struct PatchMissingR15 : Xbyak::CodeGenerator
        {
            PatchMissingR15(void* buf) :
                CodeGenerator(64, buf)
            {
                Xbyak::Label retLab;
                Xbyak::Label contLab;

                mov(r15, ptr[rsi + 0x78]);
                test(r15, r15);
                jz("null_pointer");
                mov(rax, shaderEffectCall.address());
                call(rax);
                jmp(ptr[rip + contLab]);

                L("null_pointer");
                jmp(ptr[rip + retLab]);

                L(retLab);
                dq(shaderEffectReturn.address());

                L(contLab);
                dq(shaderEffectContinue.address());
            }
        };

        void* buf = F4SE::GetTrampoline().allocate(64);
        const PatchMissingR15 code(buf);

        // Patch original code to jump to our patch
        F4SE::GetTrampoline().write_branch<5>(shaderEffectPatch.address(), std::uintptr_t(code.getCode()));
        logger::debug("Patched PipeGunScopeCrash at 0x{:X}, size:{}", lockForRead_branch.address(), code.getSize());
    }

    void patchBody()
    {
        // For new game
        const auto patchAddress = REL::Offset(0xF08D5B).address();
        REL::safe_write(patchAddress, static_cast<uint8_t>(0x74));

        // now for existing games to update
        const auto patchAddress2 = REL::Offset(0xf29ac8).address();
        REL::safe_write(patchAddress2, 0x9090D231); // This was movzx EDX,R14B.   Want to just zero out EDX with an xor instead

        logger::info("Patched Body at 0x{:X} and 0x{:X}", patchAddress, patchAddress2);
    }
}

namespace frik::hook
{
    void hookMain()
    {
        replacePrimaryWandNif();

        blockResetBodyPose();

        auto& trampoline = F4SE::GetTrampoline();
        trampoline.write_call<5>(f4vr::hook_MainUpdatePlayer.address(), &hookMainUpdatePlayer);
        trampoline.write_call<5>(f4vr::hook_smoothMovementHook.address(), &hookSmoothMovement);

        trampoline.write_call<5>(f4vr::hookActor_ReEquipAllExit.address(), &fixPA3D);
        trampoline.write_call<5>(f4vr::hookExtraData_SetMultiBoundRef.address(), &fixPA3DEnter);
    }

    void patchAll()
    {
        patchBody();
        patchInventoryInfBug();
        patchLockForReadMask();
        patchPipeGunScopeCrash();
    }
}

// removed code, left for reference
// --------------------------------

// logger::info("Hooking before main renderer");
// trampoline.write_call<5>(hookBeforeRenderer.address(), (uintptr_t)hookIt);
// logger::info("Successfully hooked before main renderer");

// replace mesh pointer string (replaces HP,Ammo,etc. UI to use nif that puts it on the back of the hand)

// trampoline.write_call<5>(hookAnimationVFunc.address(), (uintptr_t)&frik::update);
// trampoline.write_call<5>(hookEndUpdate.address(), (uintptr_t)&hookIt);
// trampoline.write_call<5>(hookMainDrawCandidate.address(), (uintptr_t)&hook2);
// trampoline.write_call<5>(hookMultiBoundCulling.address(), (uintptr_t)&hook4);
// trampoline.write_call<5>(hookActor_GetCurrentWeaponForGunReload.address(), &gunReloadInit);

// gun reload animation hook
// trampoline.write_call<5>(hookActor_SetupAnimationUpdateDataForRefernce.address(), &updatePlayerAnimationHook);
//  logger::info("hooking main loop function");
//  trampoline.write_call<5>(hookMainLoopFunc.address(), (uintptr_t)updateCounter);
//  logger::info("successfully hooked main loop");

// void hookIt(const uint64_t rcx)
// {
//     const uint64_t parm = rcx;
//     frik::g_frik.onFrameUpdate();
//     //hookedf10ed0((uint64_t)player);    // this function does the final body updates and does some stuff with the world bound to reporting up the parent tree.

//     // so all of this below is an attempt to bypass the functionality in game around my hook at resets the root parent node's world pos which screws up armor
//     // we still need to call the function i hooked below to get some things ready for the renderer however starting with the named "Root" node instead of it's parent preseves locations
//     const auto player = f4vr::getPlayer();
//     if (player->unkF0) {
//         const auto rootNode = player->unkF0->rootNode;
//         if (rootNode && !rootNode->children.empty()) {
//             if (rootNode->children[0]) {
//                 uint64_t arr[5] = { 0, 0, 0, 0, 0 };
//                 const uint64_t body = (uint64_t)rootNode->children[0].get();
//                 arr[1] = body + 0x180;
//                 arr[2] = 0x800;
//                 arr[3] = 2;
//                 arr[4] = 0x3c0c1400;
//                 hooked1c22fb0(body, (uint64_t)&arr);
//             }
//         }
//     }

//     hookedda09a0(parm);
// }

// void hook2(const uint64_t rcx, const uint64_t rdx, const uint64_t r8, const uint64_t r9)
// {
//     frik::g_frik.onFrameUpdate();

//     hookedMainDrawCandidateFunc(rcx, rdx, r8, r9);

//     const RE::BSFixedString name("ScopeMenu");

//     const std::uint64_t renderer = RendererGetByName(name);

//     if (renderer) {
//         //		RendererEnable(renderer, false);
//     }
// }

// void hook5(const uint64_t rcx)
// {
//     frik::g_frik.onFrameUpdate();
//     someRandomFunc(rcx);

//     // const RE::BSFixedString name("ScopeMenu");
//     // const std::uint64_t renderer = RendererGetByName(name);
//     // if (renderer) {
//     //     //		RendererEnable(renderer, false);
//     // }
// }

// void hook3(const double param1, const double param2, const double param3)
// {
//     hookedPosPlayerFunc(param1, param2, param3);
//     frik::g_frik.onFrameUpdate();
// }

// void hook4()
// {
//     frik::g_frik.onFrameUpdate();
//     hookMultiBoundCullingFunc();
// }

// // Gun Reload Init
// uint64_t gunReloadInit(const uint64_t rcx, const uint64_t rdx, const uint64_t r8)
// {
//     // frik::g_gunReloadSystem->startAnimationCapture();
//     return Actor_GetCurrentWeapon(rcx, rdx, r8);
// }

// uint64_t updatePlayerAnimationHook(const uint64_t rcx, float* rdx)
// {
//     // Use in gun reload
//     // if (frik::g_animDeltaTime >= 0.0f) {
//     //     rdx[0] = frik::g_animDeltaTime;
//     // }
//     return TESObjectREFR_SetupAnimationUpdateDataForRefernce(rcx, rdx);
// }

// OLD CODE that was always commented out
// static void patchTimeOut()
// {
//     struct PatchTimeOut : Xbyak::CodeGenerator
//     {
//         PatchTimeOut(void* buf) :
//             Xbyak::CodeGenerator(128, buf)
//         {
//             Xbyak::Label retLab;
//             Xbyak::Label retLab2;
//             Xbyak::Label retLab3;
//
//             cmp(ebx, 0x2710);
//             jnc(retLab);
//             jmp(ptr[rip + retLab3]);
//
//             L(retLab);
//             push(rax);
//             mov(eax, ptr[rdi + 4]);
//             //		and (eax, 0xBFFF);
//             mov(ptr[rdi + 4], eax);
//             pop(rax);
//             jmp(ptr[rip + retLab2]);
//
//             L(retLab2);
//             dq(toJumpBreak.address());
//
//             L(retLab3);
//             dq(toJumpTo.address());
//         }
//     };
//
//     void* buf = F4SE::GetTrampoline().allocate(128);
//     const PatchTimeOut code(buf);
//
//     // Patch original code to jump to our patch
//     F4SE::GetTrampoline().write_branch<6>(toJumpFrom.address(), std::uintptr_t(code.getCode()));
//     logger::debug("Patched InventoryInfBug at 0x{:X}, size:{}", lockForRead_branch.address(), code.getSize());
// }

// static void patchDropAddOn3DReplacement()
// {
//     int bytesToNOP = 0x6;
//
//     for (int i = 0; i < bytesToNOP; ++i) {
//         REL::safe_write(DropAddOnPatch1.address() + i, 0x90);
//     }
//
//     bytesToNOP = 0x5;
//
//     for (int i = 0; i < bytesToNOP; ++i) {
//         REL::safe_write(DropAddOnPatch2.address() + i, 0x90);
//     }
//
//     bytesToNOP = 0x7;
//
//     for (int i = 0; i < bytesToNOP; ++i) {
//         REL::safe_write(DropAddOnPatch3.address() + i, 0x90);
//     }
// }
