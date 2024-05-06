#pragma once

#include "api/PapyrusVRAPI.h"
#include "api/VRManagerAPI.h"

#include "f4se/NiTypes.h"
#include "f4se/NiNodes.h"

#include <memory>


namespace VRHook {


	class VRSystem {
	public:
		VRSystem() {
			leftPacket = 0;
			rightPacket = 0;

			vrHook = RequestOpenVRHookManagerObject();

			for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
				auto dc = vrHook->GetVRSystem()->GetTrackedDeviceClass(i);
				if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker) {
					viveTrackers.insert({ getProperty(vr::ETrackedDeviceProperty::Prop_ModelNumber_String, i), i });
				}
				else if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
					std::string prop = getProperty(vr::ETrackedDeviceProperty::Prop_ModelNumber_String, i);
					if (prop.find("Right") != std::string::npos) {
						controllers.insert({ "Right", i });
					}
					else if (prop.find("Left") != std::string::npos) {
						controllers.insert({ "Left", i });
					}
				}
				else  if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_HMD) {
					controllers.insert({ "HMD", i });
				}
			}
			roomNode = nullptr;
		}

		inline void setRoomNode(NiNode* a_node) {
			roomNode = a_node;
		}

		inline OpenVRHookManagerAPI* getHook() {
			return vrHook;
		}

		inline void updatePoses() {
			vr::VRCompositorError error = vrHook->GetVRCompositor()->GetLastPoses((vr::TrackedDevicePose_t*)renderPoses, vr::k_unMaxTrackedDeviceCount, (vr::TrackedDevicePose_t*)gamePoses, vr::k_unMaxTrackedDeviceCount);
		}

		inline bool viveTrackersPresent() const { return !viveTrackers.empty(); }

		inline void setVRControllerState() {
			if (vrHook != nullptr) {

				vr::TrackedDeviceIndex_t lefthand = vrHook->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
				vr::TrackedDeviceIndex_t righthand = vrHook->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

				vrHook->GetVRSystem()->GetControllerState(lefthand, &leftControllerState, sizeof(vr::VRControllerState_t));
				vrHook->GetVRSystem()->GetControllerState(righthand, &rightControllerState, sizeof(vr::VRControllerState_t));

				if (rightControllerState.unPacketNum != rightPacket) {
				}
				if (leftControllerState.unPacketNum != leftPacket) {
				}
			}
		}

		inline vr::VRControllerState_t getControllerState(bool retLeft) {
			return retLeft ? leftControllerState : rightControllerState;
		}

		void getTrackerNiTransformByName(std::string trackerName, NiTransform* transform);
		void getTrackerNiTransformByIndex(vr::TrackedDeviceIndex_t idx, NiTransform* transform);
		void getControllerNiTransformByName(std::string trackerName, NiTransform* transform);
		void debugPrint();

	private:

		inline std::string getProperty(vr::ETrackedDeviceProperty property, vr::TrackedDeviceIndex_t idx) {
			const uint32_t bufSize = vr::k_unMaxPropertyStringSize;
			std::unique_ptr<char*> pchValue = std::make_unique<char*>(new char[bufSize]);
			vr::TrackedPropertyError pError = vr::TrackedPropertyError::TrackedProp_NotYetAvailable;

			vrHook->GetVRSystem()->GetStringTrackedDeviceProperty(idx, property, *pchValue.get(), bufSize, &pError);

			return std::string(*pchValue.get());
		}

		uint32_t leftPacket;
		uint32_t rightPacket;
		OpenVRHookManagerAPI* vrHook;
		vr::VRControllerState_t rightControllerState;
		vr::VRControllerState_t leftControllerState;
		vr::TrackedDevicePose_t renderPoses[vr::k_unMaxTrackedDeviceCount]; //Used to store available poses
		vr::TrackedDevicePose_t gamePoses[vr::k_unMaxTrackedDeviceCount]; //Used to store available poses
		std::map<std::string, vr::TrackedDeviceIndex_t> viveTrackers;
		std::map<std::string, vr::TrackedDeviceIndex_t> controllers;
		NiNode* roomNode;
	};
}
