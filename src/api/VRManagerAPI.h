#pragma once
#include "PapyrusVRTypes.h"
#include "OpenVRTypes.h"

namespace PapyrusVR
{
	typedef void(*OnVRButtonEvent)(VREventType, EVRButtonId, VRDevice);
	typedef void(*OnVROverlapEvent)(VROverlapEvent, UInt32, VRDevice);
	typedef void(*OnVRUpdateEvent)(float);
	typedef void(*OnVRHapticEvent)(UInt32, UInt32, VRDevice);

	class VRManagerAPI
	{
	public:
		virtual bool IsInitialized() = 0;

		virtual void UpdatePoses() = 0;

		virtual void RegisterVRButtonListener(OnVRButtonEvent listener) = 0;
		virtual void UnregisterVRButtonListener(OnVRButtonEvent listener) = 0;

		virtual void RegisterVROverlapListener(OnVROverlapEvent listener) = 0;
		virtual void UnregisterVROverlapListener(OnVROverlapEvent listener) = 0;

		virtual void RegisterVRHapticListener(OnVRHapticEvent listener) = 0;
		virtual void UnregisterVRHapticListener(OnVRHapticEvent listener) = 0;

		virtual void RegisterVRUpdateListener(OnVRUpdateEvent listener) = 0;
		virtual void UnregisterVRUpdateListener(OnVRUpdateEvent listener) = 0;

		virtual UInt32 CreateLocalOverlapSphere(float radius, Matrix34* transform, VRDevice attachedDevice = VRDevice::VRDevice_Unknown) = 0;
		virtual void DestroyLocalOverlapObject(UInt32 overlapObjectHandle) = 0;

		virtual TrackedDevicePose* GetHMDPose(bool renderPose = true) = 0;
		virtual TrackedDevicePose* GetRightHandPose(bool renderPose = true) = 0;
		virtual TrackedDevicePose* GetLeftHandPose(bool renderPose = true) = 0;
		virtual TrackedDevicePose* GetPoseByDeviceEnum(VRDevice device, bool renderPose = true) = 0;
	};
}


