#include "patches.h"

#include "f4se_common/BranchTrampoline.h"
#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"

#include "xbyak/xbyak.h"

namespace patches {
	RelocAddr<std::uint64_t> invJumpFrom(0x2567664);
	RelocAddr<std::uint64_t> invJumpTo(0x256766a);
	RelocAddr<std::uint64_t> toJumpFrom(0x1b932ea);
	RelocAddr<std::uint64_t> toJumpTo(0x1b932f2);
	RelocAddr<std::uint64_t> toJumpBreak(0x1b93315);

	RelocAddr<std::uint64_t> lockForRead_branch(0x1b932f8);
	RelocAddr<std::uint64_t> lockForRead_return(0x1b932fd);

	RelocAddr<std::uint64_t> shaderEffectPatch(0x28d323a);
	RelocAddr<std::uint64_t> shaderEffectCall(0x2813560);
	RelocAddr<std::uint64_t> shaderEffectContinue(0x28d323f);
	RelocAddr<std::uint64_t> shaderEffectReturn(0x28d4ec8);

	static void patchInventoryInfBug() {
		struct PatchShortVar : Xbyak::CodeGenerator {
			PatchShortVar(void* buf)
				: CodeGenerator(2048, buf) {
				Xbyak::Label retLab;

				and
				(edi, 0xffff); // edi is an int but should be treated as a short.  Should allow for loop to exit.

				mov(r12d, 0xffff);
				jmp(ptr[rip + retLab]);

				L(retLab);
				dq(invJumpTo.GetUIntPtr());
			}
		};

		void* buf = g_localTrampoline.StartAlloc();
		PatchShortVar code(buf);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write6Branch(invJumpFrom.GetUIntPtr(), uintptr_t(code.getCode()));

		//struct PatchTimeOut : Xbyak::CodeGenerator {
		//	PatchTimeOut(void* buf) : Xbyak::CodeGenerator(4096, buf) {
		//		Xbyak::Label retLab;
		//		Xbyak::Label retLab2;
		//		Xbyak::Label retLab3;

		//		cmp(ebx, 0x2710);
		//		jnc(retLab);
		//		jmp(ptr[rip + retLab3]);
		//		
		//		L(retLab);
		//		push(rax);
		//		mov(eax, ptr[rdi + 4]);
		////		and (eax, 0xBFFF);
		//		mov(ptr[rdi + 4], eax);
		//		pop(rax);
		//		jmp(ptr[rip + retLab2]);

		//		L(retLab2);
		//		dq(toJumpBreak.GetUIntPtr());

		//		L(retLab3);
		//		dq(toJumpTo.GetUIntPtr());

		//	}
		//};

		//void* buf2 = g_localTrampoline.StartAlloc();
		//PatchTimeOut code2(buf2);
		//g_localTrampoline.EndAlloc(code2.getCurr());

		//g_branchTrampoline.Write6Branch(toJumpFrom.GetUIntPtr(), uintptr_t(code2.getCode()));
	}

	static void patchLockForReadMask() {
		struct PatchMoreMask : Xbyak::CodeGenerator {
			PatchMoreMask(void* buf)
				: CodeGenerator(2048, buf) {
				Xbyak::Label retLab;

				and
				(dword[rdi + 0x4], 0xFFFFFFF);
				mov(rcx, 1);
				jmp(ptr[rip + retLab]);

				L(retLab);
				dq(lockForRead_return.GetUIntPtr());
			}
		};

		void* buf = g_localTrampoline.StartAlloc();
		PatchMoreMask code(buf);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write5Branch(lockForRead_branch.GetUIntPtr(), uintptr_t(code.getCode()));
	}

	static void patchPipeGunScopeCrash() {
		struct PatchMissingR15 : Xbyak::CodeGenerator {
			PatchMissingR15(void* buf)
				: CodeGenerator(4096, buf) {
				Xbyak::Label retLab;
				Xbyak::Label contLab;

				mov(r15, ptr[rsi + 0x78]);
				test(r15, r15);
				jz("null_pointer");
				mov(rax, shaderEffectCall.GetUIntPtr());
				call(rax);
				jmp(ptr[rip + contLab]);

				L("null_pointer");
				jmp(ptr[rip + retLab]);

				L(retLab);
				dq(shaderEffectReturn.GetUIntPtr());

				L(contLab);
				dq(shaderEffectContinue.GetUIntPtr());
			}
		};

		void* buf = g_localTrampoline.StartAlloc();
		PatchMissingR15 code(buf);
		g_localTrampoline.EndAlloc(code.getCurr());

		g_branchTrampoline.Write5Branch(shaderEffectPatch.GetUIntPtr(), uintptr_t(code.getCode()));
	}

	RelocAddr<uint64_t> DropAddOnPatch1(0x03e9caf);
	RelocAddr<uint64_t> DropAddOnPatch2(0x03e9cd3);
	RelocAddr<uint64_t> DropAddOnPatch3(0x3e9df5);

	static void patchDropAddOn3DReplacement() {
		int bytesToNOP = 0x6;

		for (int i = 0; i < bytesToNOP; ++i) {
			SafeWrite8(DropAddOnPatch1.GetUIntPtr() + i, 0x90);
		}

		bytesToNOP = 0x5;

		for (int i = 0; i < bytesToNOP; ++i) {
			SafeWrite8(DropAddOnPatch2.GetUIntPtr() + i, 0x90);
		}

		bytesToNOP = 0x7;

		for (int i = 0; i < bytesToNOP; ++i) {
			SafeWrite8(DropAddOnPatch3.GetUIntPtr() + i, 0x90);
		}
	}

	static void PatchBody() {
		_MESSAGE("Patch Body In");
		_MESSAGE("addr = %016I64X", RelocAddr<uintptr_t>(0xF08D5B).GetUIntPtr());

		// For new game
		SafeWrite8(RelocAddr<uintptr_t>(0xF08D5B).GetUIntPtr(), 0x74);

		// now for existing games to update
		SafeWrite32(RelocAddr<uintptr_t>(0xf29ac8), 0x9090D231); // This was movzx EDX,R14B.   Want to just zero out EDX with an xor instead
		_MESSAGE("Patch Body Succeeded");
	}

	void patchAll() {
		PatchBody();
		patchInventoryInfBug();
		patchLockForReadMask();
		patchPipeGunScopeCrash();
		//patchDropAddOn3DReplacement();
	}
}
