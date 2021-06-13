#pragma once

/*
Copyright (c) 2015, Valve Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.
*/

namespace PapyrusVR
{
	static const uint32_t k_unTrackedDeviceIndex_Hmd = 0;
	static const uint32_t k_unMaxTrackedDeviceCount = 64;
	static const uint32_t k_unTrackedDeviceIndexOther = 0xFFFFFFFE;
	static const uint32_t k_unTrackedDeviceIndexInvalid = 0xFFFFFFFF;

	enum ETrackingResult //OpenVR
	{
		TrackingResult_Uninitialized = 1,

		TrackingResult_Calibrating_InProgress = 100,
		TrackingResult_Calibrating_OutOfRange = 101,

		TrackingResult_Running_OK = 200,
		TrackingResult_Running_OutOfRange = 201,
	};

	enum EVRButtonId //OpenVR
	{
		k_EButton_System = 0,
		k_EButton_ApplicationMenu = 1,
		k_EButton_Grip = 2,
		k_EButton_DPad_Left = 3,
		k_EButton_DPad_Up = 4,
		k_EButton_DPad_Right = 5,
		k_EButton_DPad_Down = 6,
		k_EButton_A = 7,

		k_EButton_ProximitySensor = 31,

		k_EButton_Axis0 = 32,
		k_EButton_Axis1 = 33,
		k_EButton_Axis2 = 34,
		k_EButton_Axis3 = 35,
		k_EButton_Axis4 = 36,

		// aliases for well known controllers
		k_EButton_SteamVR_Touchpad = k_EButton_Axis0,
		k_EButton_SteamVR_Trigger = k_EButton_Axis1,

		k_EButton_Dashboard_Back = k_EButton_Grip,

		k_EButton_Max = 64
	};

	typedef struct TrackedDevicePose //Based on OpenVR vr::TrackedDevicePose_t
	{
		Matrix34 mDeviceToAbsoluteTracking;
		Vector3 vVelocity;				// velocity in tracker space in m/s
		Vector3 vAngularVelocity;		// angular velocity in radians/s (?)
		ETrackingResult eTrackingResult;
		bool bPoseIsValid; 

		// This indicates that there is a device connected for this spot in the pose array.
		// It could go from true to false if the user unplugs the device.
		bool bDeviceIsConnected;
	} TrackedDevicePose;
}