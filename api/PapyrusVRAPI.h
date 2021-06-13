#pragma once
#include <vector>
#include <functional>
#include "VRManagerAPI.h"
#include "VRHookAPI.h"
#include "PapyrusVRTypes.h"

static const UInt32 kPapyrusVR_Message_Init = 40008;

struct PapyrusVRAPI
{
	//Functions
	//std::function<void(OnPoseUpdateCallback)> RegisterPoseUpdateListener; Discontinued
	std::function<PapyrusVR::VRManagerAPI*(void)> GetVRManager;
	std::function<OpenVRHookManagerAPI*(void)> GetOpenVRHook;
};