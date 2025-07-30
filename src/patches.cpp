#include "patches.h"

#include "common/Logger.h"

#include "xbyak/xbyak.h"

using namespace common;

namespace patches
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

    static void patchInventoryInfBug()
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

    static void patchLockForReadMask()
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

    static void patchPipeGunScopeCrash()
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

    REL::Offset DropAddOnPatch1(0x03e9caf);
    REL::Offset DropAddOnPatch2(0x03e9cd3);
    REL::Offset DropAddOnPatch3(0x3e9df5);

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

    static void patchBody()
    {
        // For new game
        const auto patchAddress = REL::Offset(0xF08D5B).address();
        REL::safe_write(patchAddress, static_cast<uint8_t>(0x74));

        // now for existing games to update
        const auto patchAddress2 = REL::Offset(0xf29ac8).address();
        REL::safe_write(patchAddress2, 0x9090D231); // This was movzx EDX,R14B.   Want to just zero out EDX with an xor instead

        logger::info("Patched Body at 0x{:X} and 0x{:X}", patchAddress, patchAddress2);
    }

    void patchAll()
    {
        patchBody();
        patchInventoryInfBug();
        patchLockForReadMask();
        patchPipeGunScopeCrash();
        //patchDropAddOn3DReplacement();
    }
}
