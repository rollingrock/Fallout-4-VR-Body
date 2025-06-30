#pragma once
#include "PapyrusVRTypes.h"
#include "OpenVRTypes.h"

namespace PapyrusVR
{
	typedef void(*OnVRButtonEvent)(VREventType, EVRButtonId, VRDevice);
	typedef void(*OnVROverlapEvent)(VROverlapEvent, std::uint32_t, VRDevice);
	typedef void(*OnVRUpdateEvent)(float);
	typedef void(*OnVRHapticEvent)(std::uint32_t, std::uint32_t, VRDevice);

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

		virtual std::uint32_t CreateLocalOverlapSphere(float radius, Matrix34* transform, VRDevice attachedDevice = VRDevice::VRDevice_Unknown) = 0;
		virtual void DestroyLocalOverlapObject(std::uint32_t overlapObjectHandle) = 0;

		virtual TrackedDevicePose* GetHMDPose(bool renderPose = true) = 0;
		virtual TrackedDevicePose* GetRightHandPose(bool renderPose = true) = 0;
		virtual TrackedDevicePose* GetLeftHandPose(bool renderPose = true) = 0;
		virtual TrackedDevicePose* GetPoseByDeviceEnum(VRDevice device, bool renderPose = true) = 0;
	};
}


