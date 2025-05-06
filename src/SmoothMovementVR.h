#pragma once

// From Shizof mod with permission. Thanks Shizof!!

#include "f4se/GameReferences.h"
#include "f4se/NiRTTI.h"

namespace SmoothMovementVR {
	using _IsInAir = bool(*)(Actor* actor);
	extern RelocAddr<_IsInAir> IsInAir;

	void everyFrame();
	void startFunctions();
	bool checkIfJumpingOrInAir();
}
