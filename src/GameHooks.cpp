#include "GameHooks.h"

#include "xbyak/xbyak.h"

#include <format>

#include "FRIK.h"
#include "api/ApiCore.h"

namespace
{
    std::string bytesToHex(const std::uint8_t* bytes, const std::size_t count)
    {
        std::string hex;
        for (std::size_t i = 0; i < count; ++i) {
            hex += std::format("{}{:02X}", i ? " " : "", bytes[i]);
        }
        return hex;
    }

    /**
     * Check the game bytes at a patch site before writing, so a different executable is logged and skipped instead of corrupted.
     */
    bool verifyPatchBytes(const std::string_view name, const std::uintptr_t address, const std::uint8_t* expected, const std::size_t count)
    {
        const auto actual = reinterpret_cast<const std::uint8_t*>(address);
        if (std::equal(expected, expected + count, actual)) {
            return true;
        }
        logger::error("Skip patch '{}' at 0x{:X}: found [{}] expected [{}]", name, address, bytesToHex(actual, count), bytesToHex(expected, count));
        return false;
    }

    bool verifyPatchBytes(const std::string_view name, const std::uintptr_t address, const std::initializer_list<std::uint8_t> expected)
    {
        return verifyPatchBytes(name, address, expected.begin(), expected.size());
    }

    bool verifyPatchBytes(const std::string_view name, const std::uintptr_t address, const std::string_view expected)
    {
        return verifyPatchBytes(name, address, reinterpret_cast<const std::uint8_t*>(expected.data()), expected.size());
    }

    /**
     * A call site only needs its CALL opcode intact; a different target means another mod hooked it first and the trampoline chains to it.
     */
    bool verifyCallSite(const std::string_view name, const std::uintptr_t address, const std::uintptr_t expectedTarget)
    {
        const auto bytes = reinterpret_cast<const std::uint8_t*>(address);
        if (bytes[0] != 0xE8) {
            logger::error("Skip hook '{}' at 0x{:X}: found [{}], not a CALL", name, address, bytesToHex(bytes, 5));
            return false;
        }
        const auto target = address + 5 + *reinterpret_cast<const std::int32_t*>(address + 1);
        if (target != expectedTarget) {
            logger::warn("Hook '{}' at 0x{:X} already redirected to 0x{:X} (expected 0x{:X}); hooking it anyway, which REPLACES that redirect rather than chaining to it",
                name,
                address,
                target,
                expectedTarget);
        }
        return true;
    }

    // fix power-armor 3d mesh hooks
    void fixPA3D()
    {
        const auto player = f4vr::getPlayer();
        f4vr::Actor_ReEquipAll(player);
        f4vr::AIProcess_Set3DUpdateFlags(player->currentProcess, 0x520);
    }

    void fixPA3DEnter(const std::uint64_t rcx, const std::uint64_t rdx)
    {
        const auto player = f4vr::getPlayer();
        f4vr::ExtraData_SetMultiBoundRef(rcx, rdx);
        f4vr::AIProcess_Set3DUpdateFlags(player->currentProcess, 0x520);
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
        if (player && playerCamera && playerCamera->cameraRoot && player->loadedData && player->loadedData->data3D) {
            const auto body = player->loadedData->data3D.get();
            const auto& cameraPos = playerCamera->cameraRoot->world.translate;
            body->local.translate.x = cameraPos.x;
            body->local.translate.y = cameraPos.y;
            body->world.translate.x = cameraPos.x;
            body->world.translate.y = cameraPos.y;
        }

        f4vr::main_update_player(rcx, rdx);
    }

    /**
     * Runs in place of the engine's two per-frame first-person arm placements (the Weapon node onto the primary offset, then WeaponLeft
     * onto the secondary), which come after FRIK's frame update. The engine moves whichever arm the node really hangs under, so during an
     * external left carry (Skeleton::handleLeftHandedWeaponNodesSwitch) that pairing puts the left arm on the right controller and the
     * right arm on the left one; the skeleton re-pairs it.
     */
    // The fallthrough is deliberately a tail call: a mod that detours Update1StPersonArm itself and keys on the caller's return
    // address sees the engine's own call site through it. Keep it a tail call if this function grows.
    void* hookUpdate1stPersonArm(const RE::PlayerCharacter* pc, RE::NiNode** weapon, RE::NiNode** offsetNode)
    {
        if (weapon && frik::g_frik.isSkeletonReady() && frik::g_frik.getSkeleton()->repairEngineArmPlacementForCarry(*weapon)) {
            return nullptr;
        }
        return f4vr::Update1StPersonArm(pc, weapon, offsetNode);
    }

    /**
     * Replace mesh pointer string (replaces HP,Ammo,etc. UI to use nif that puts it on the back of the hand)
     */
    void replacePrimaryWandNif()
    {
        const auto mesh = R"(Data\Meshes\FRIK\_primaryWand.nif)";
        if (!verifyPatchBytes("wandMesh", f4vr::wandMesh.address(), std::string_view(R"(Data\Meshes\world_primaryWand.nif)"))) {
            return;
        }
        for (int i = 0; i < strlen(mesh); ++i) {
            REL::safe_write(f4vr::wandMesh.address() + i, mesh[i]);
        }
    }

    /**
     * Runs in place of the body-pose reset, right after the engine's animation graph output for the player.
     * rcx is the PlayerCharacter sub-object the vfunc was called on; nothing here needs it.
     */
    void hookPlayerPostAnimGraph(const uint64_t)
    {
        if (frik::g_frik.isSkeletonReady()) {
            frik::api::core::invokeFramePhase(frik::FramePhase::NativeGraphOutput);
        }
    }

    /**
     * The PlayerCharacter vfunc at 0xF2F0A0 resets the body pose to hang off the camera, which breaks body height.
     * Its whole body (first instruction to epilogue, prologue and epilogue intact) is replaced with a call to
     * hookPlayerPostAnimGraph followed by NOPs, so the reset is gone and the post-animation point is exposed as a frame phase.
     */
    void detourResetBodyPose()
    {
        const int bytesToNOP = 0x1FF;
        const auto address = f4vr::hookAnimationVFunc.address();
        if (!verifyPatchBytes("resetBodyPose", address, { 0xF6, 0x81, 0x56, 0x12, 0x00, 0x00, 0x10 }) ||
            !verifyPatchBytes("resetBodyPose epilogue", address + bytesToNOP, { 0x48, 0x83, 0xC4, 0x60, 0x5D, 0xC3 })) {
            return;
        }
        for (int i = 0; i < bytesToNOP; ++i) {
            REL::safe_write(address + i, static_cast<uint8_t>(0x90));
        }
        F4SE::GetTrampoline().write_call<5>(address, &hookPlayerPostAnimGraph);
        logger::info("Detoured body pose reset at 0x{:X} to the NativeGraphOutput phase", address);
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
        if (!verifyPatchBytes("inventoryInfBug", invJumpFrom.address(), { 0x41, 0xBC, 0xFF, 0xFF, 0x00, 0x00 })) {
            return;
        }

        struct PatchShortVar : Xbyak::CodeGenerator
        {
            PatchShortVar(void* buf)
                : CodeGenerator(32, buf)
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
        logger::debug("Patched InventoryInfBug at 0x{:X}, size:{}", invJumpFrom.address(), code.getSize());
    }

    void patchLockForReadMask()
    {
        if (!verifyPatchBytes("lockForReadMask", lockForRead_branch.address(), { 0xB9, 0x01, 0x00, 0x00, 0x00 })) {
            return;
        }

        struct PatchMoreMask : Xbyak::CodeGenerator
        {
            PatchMoreMask(void* buf)
                : CodeGenerator(64, buf)
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
        if (!verifyCallSite("pipeGunScopeCrash", shaderEffectPatch.address(), shaderEffectCall.address())) {
            return;
        }

        struct PatchMissingR15 : Xbyak::CodeGenerator
        {
            PatchMissingR15(void* buf)
                : CodeGenerator(64, buf)
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
        logger::debug("Patched PipeGunScopeCrash at 0x{:X}, size:{}", shaderEffectPatch.address(), code.getSize());
    }

    void patchBody()
    {
        // For new game
        const auto patchAddress = REL::Offset(0xF08D5B).address();
        if (verifyPatchBytes("body new game", patchAddress, { 0x75 })) {
            REL::safe_write(patchAddress, static_cast<uint8_t>(0x74));
        }

        // now for existing games to update
        const auto patchAddress2 = REL::Offset(0xf29ac8).address();
        if (verifyPatchBytes("body existing game", patchAddress2, { 0x41, 0x0F, 0xB6, 0xD6 })) {
            REL::safe_write(patchAddress2, 0x9090D231); // This was movzx EDX,R14B.   Want to just zero out EDX with an xor instead
        }

        logger::info("Patched Body at 0x{:X} and 0x{:X}", patchAddress, patchAddress2);
    }
}

namespace frik::hook
{
    void hookMain()
    {
        replacePrimaryWandNif();

        detourResetBodyPose();

        auto& trampoline = F4SE::GetTrampoline();
        if (verifyCallSite("mainUpdatePlayer", f4vr::hook_MainUpdatePlayer.address(), f4vr::main_update_player.address())) {
            trampoline.write_call<5>(f4vr::hook_MainUpdatePlayer.address(), &hookMainUpdatePlayer);
        }
        if (verifyCallSite("smoothMovement", f4vr::hook_smoothMovementHook.address(), f4vr::smoothMovementHook.address())) {
            trampoline.write_call<5>(f4vr::hook_smoothMovementHook.address(), &hookSmoothMovement);
        }
        // the engine's two first-person arm placements in its per-frame update (Weapon, then WeaponLeft)
        for (const auto site : { REL::Offset(0xef6108).address(), REL::Offset(0xef614b).address() }) {
            if (verifyCallSite("update1stPersonArm", site, f4vr::Update1StPersonArm.address())) {
                trampoline.write_call<5>(site, &hookUpdate1stPersonArm);
            }
        }

        if (verifyCallSite("reEquipAllExit", f4vr::hookActor_ReEquipAllExit.address(), f4vr::Actor_ReEquipAll.address())) {
            trampoline.write_call<5>(f4vr::hookActor_ReEquipAllExit.address(), &fixPA3D);
        }
        if (verifyCallSite("setMultiBoundRef", f4vr::hookExtraData_SetMultiBoundRef.address(), f4vr::ExtraData_SetMultiBoundRef.address())) {
            trampoline.write_call<5>(f4vr::hookExtraData_SetMultiBoundRef.address(), &fixPA3DEnter);
        }
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
