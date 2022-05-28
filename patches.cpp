#include "patches.h"


#include "f4sE_common/Relocation.h"
#include "F4SE_common/SafeWrite.h"
#include "f4se_common/BranchTrampoline.h"

#include "xbyak/xbyak.h"


namespace patches {

	RelocAddr<std::uint64_t> invJumpFrom(0x2567664);
	RelocAddr<std::uint64_t> invJumpTo(0x256766a);
	RelocAddr<std::uint64_t> toJumpFrom(0x1b932ea);
	RelocAddr<std::uint64_t> toJumpTo(0x1b932f2);
	RelocAddr<std::uint64_t> toJumpBreak(0x1b93315);

	void patchInventoryInfBug() {

		struct PatchShortVar : Xbyak::CodeGenerator {
			PatchShortVar(void* buf) : Xbyak::CodeGenerator(4096, buf) {
				Xbyak::Label retLab;
				
				and (edi, 0xffff);   // edi is an int but should be treated as a short.  Should allow for loop to exit.

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


		return;
	}



	bool patchAll() {
		patchInventoryInfBug();
		return true;
	}

}