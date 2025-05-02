#pragma once

// From Shizof's mod with permission.  Thanks Shizof!!

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se\GameReferences.h"
#include "f4se\GameRTTI.h"
#include "f4se\NiNodes.h"
#include "f4se\NiTypes.h"
#include "f4se\NiObjects.h"
#include "f4se/GameRTTI.h"
#include "f4se/NiRTTI.h"
#include "f4se/GameForms.h"
#include "f4se\PapyrusEvents.h"
#include "f4se\PapyrusVM.h"
#include "f4se\PapyrusForm.h"
#include "f4se/GameData.h"
#include "f4se_common/Utilities.h"
#include "f4se\GameExtraData.h"
#include "f4se/GameAPI.h"

#include "MenuChecker.h"

#include <list>
#include <thread>
#include <iostream>
#include <string>
#include <fstream>

namespace SmoothMovementVR
{
	typedef bool(*_IsInAir)                     (Actor* actor);
	extern RelocAddr    <_IsInAir>                      IsInAir;

	void everyFrame();
	void StartFunctions();
	bool checkIfJumpingOrInAir();
}

