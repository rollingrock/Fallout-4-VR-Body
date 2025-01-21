#pragma once

#include "api/PapyrusVRAPI.h"
#include "api/VRManagerAPI.h"
#include "f4se/NiTypes.h"
#include "f4se/NiNodes.h"
#include <memory>
#include <string>
#include <map>

namespace VRHook {

	class VRSystem {
	public:
		enum TrackerType {
			HMD,
			Left,
			Right,
			Vive
		};

		VRSystem() : leftPacket(0), rightPacket(0), vrHook(RequestOpenVRHookManagerObject()), roomNode(nullptr) {
			initializeDevices();
		}

		void setRoomNode(NiNode* a_node) {
			roomNode = a_node;
		}

		OpenVRHookManagerAPI* getHook() const {
			return vrHook;
		}

		void updatePoses() {
			vrHook->GetVRCompositor()->GetLastPoses(renderPoses, vr::k_unMaxTrackedDeviceCount, gamePoses, vr::k_unMaxTrackedDeviceCount);
		}

		bool viveTrackersPresent() const {
			return !viveTrackers.empty();
		}

		void setVRControllerState() {
			if (vrHook) {
				auto vrSystem = vrHook->GetVRSystem();
				auto lefthand = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
				auto righthand = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

				vrSystem->GetControllerState(lefthand, &leftControllerState, sizeof(vr::VRControllerState_t));
				vrSystem->GetControllerState(righthand, &rightControllerState, sizeof(vr::VRControllerState_t));

				if (rightControllerState.unPacketNum != rightPacket) {
					rightPacket = rightControllerState.unPacketNum;
				}
				if (leftControllerState.unPacketNum != leftPacket) {
					leftPacket = leftControllerState.unPacketNum;
				}
			}
		}

		vr::VRControllerState_t getControllerState(TrackerType a_tracker) const {
			switch (a_tracker) {
			case Left:
				return leftControllerState;
			case Right:
				return rightControllerState;
			default:
				return rightControllerState; // Default to right controller state
			}
		}

		void getTrackerNiTransformByName(const std::string& trackerName, NiTransform* transform);
		void getTrackerNiTransformByIndex(vr::TrackedDeviceIndex_t idx, NiTransform* transform);
		void getControllerNiTransformByName(const std::string& trackerName, NiTransform* transform);
		void debugPrint();

	private:
		void applyRoomTransform(NiTransform* transform);
		void initializeDevices();
		std::string getProperty(vr::ETrackedDeviceProperty property, vr::TrackedDeviceIndex_t idx) const;
		uint32_t leftPacket;
		uint32_t rightPacket;
		OpenVRHookManagerAPI* vrHook;
		vr::VRControllerState_t rightControllerState;
		vr::VRControllerState_t leftControllerState;
		vr::TrackedDevicePose_t renderPoses[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t gamePoses[vr::k_unMaxTrackedDeviceCount];
		std::map<std::string, vr::TrackedDeviceIndex_t> viveTrackers;
		std::map<std::string, vr::TrackedDeviceIndex_t> controllers;
		NiNode* roomNode;
	};

	extern VRSystem* g_vrHook;

	inline void InitVRSystem() {
		g_vrHook = new VRSystem();
	}

}
