#include "hook.h"
#include "F4VRBody.h"

#include "xbyak/xbyak.h"

//RelocAddr<uintptr_t> hookBeforeRenderer(0xd844bc);
RelocAddr<uintptr_t> hookBeforeRenderer(0x1C21156);


typedef void(*_hookedFunc)(uint64_t param1, uint64_t param2, uint64_t param3);
//RelocAddr<_hookedFunc> hookedFunc(0xd831f0);
RelocAddr<_hookedFunc> hookedFunc(0x1C18620);


void hookIt(uint64_t rcx_, uint64_t rdx_, uint64_t r8d_) {

	F4VRBody::update(rdx_);

	hookedFunc(rcx_,rdx_,r8d_);

}

void hookMain() {
	_MESSAGE("Hooking before main renderer");
	g_branchTrampoline.Write5Call(hookBeforeRenderer.GetUIntPtr(), (uintptr_t)hookIt);
	_MESSAGE("Successfully hooked before main renderer");
}