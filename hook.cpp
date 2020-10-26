#include "hook.h"
#include "F4VRBody.h"

RelocAddr<uintptr_t> hookBeforeRenderer(0xd844bc);

typedef void(*_hookedFunc)(uintptr_t param1);
RelocAddr<_hookedFunc> hookedFunc(0xd831f0);


void hookIt(uintptr_t p) {

	F4VRBody::update();

	hookedFunc(p);
}

void hookMain() {
	_MESSAGE("Hooking before main renderer");
	g_branchTrampoline.Write5Call(hookBeforeRenderer.GetUIntPtr(), (uintptr_t)hookIt);
	_MESSAGE("Successfully hooked before main renderer");
}