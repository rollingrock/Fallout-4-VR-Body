#pragma once

#include <windows.h>
#include "common/IDebugLog.h"
#include "openvr.h"

// VR input callbacks
// last argument is ptr to VRControllerState that the mod authors can modify and use to block inputs
typedef bool (*GetControllerState_CB)(vr::TrackedDeviceIndex_t unControllerDeviceIndex, const vr::VRControllerState_t *pControllerState, uint32_t unControllerStateSize, vr::VRControllerState_t* pOutputControllerState);
typedef vr::EVRCompositorError (*WaitGetPoses_CB)(VR_ARRAY_COUNT(unRenderPoseArrayCount) vr::TrackedDevicePose_t* pRenderPoseArray, uint32_t unRenderPoseArrayCount,
	VR_ARRAY_COUNT(unGamePoseArrayCount) vr::TrackedDevicePose_t* pGamePoseArray, uint32_t unGamePoseArrayCount);

class OpenVRHookManagerAPI
{
public:
	virtual unsigned int GetVersion() = 0;
    virtual bool IsInitialized() = 0;

    virtual void RegisterControllerStateCB(GetControllerState_CB cbfunc) = 0;
	virtual void RegisterGetPosesCB(WaitGetPoses_CB cbfunc) = 0;
	virtual void UnregisterControllerStateCB(GetControllerState_CB cbfunc) = 0;
	virtual void UnregisterGetPosesCB(WaitGetPoses_CB cbfunc) = 0;

	virtual vr::IVRSystem* GetVRSystem() const = 0;
	virtual vr::IVRCompositor* GetVRCompositor() const = 0;

	virtual void StartHaptics(unsigned int trackedControllerId, float hapticTime, float hapticIntensity) = 0;
};


// Request OpenVRHookManagerAPI object from dll if it is available, otherwise return null.  Use to initialize raw OpenVR hooking
inline OpenVRHookManagerAPI* RequestOpenVRHookManagerObject()
{
	typedef OpenVRHookManagerAPI* (*GetVRHookMgrFuncPtr_t)();
	HMODULE FO4VRToolsModule = LoadLibraryA("FO4VRTools.dll");
	if (FO4VRToolsModule != nullptr)
	{
		GetVRHookMgrFuncPtr_t vrHookGetFunc = (GetVRHookMgrFuncPtr_t)GetProcAddress(FO4VRToolsModule, "GetVRHookManager");
		if (vrHookGetFunc)
		{
			return vrHookGetFunc();
		}
		else
		{
			_MESSAGE("Failed to get address of function GetVRHookmanager from FO4VRTools.dll in RequestOpenVRHookManagerObject().  Is your skyrimvrtools.dll out of date?");
		}
	}
	else
	{
		_MESSAGE("Failed to load FO4VRTools.dll in RequestOpenVRHookManagerObject()");
	}
	
	return nullptr;
}
