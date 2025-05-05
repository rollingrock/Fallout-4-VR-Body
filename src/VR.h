#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include "api/PapyrusVRAPI.h"
#include "f4se/NiNodes.h"
#include "f4se/NiTypes.h"

namespace VRHook {
	class VRSystem {
	public:
		enum TrackerType {
			HMD,
			Left,
			Right,
			Vive
		};

		struct ControllerButtonLongPressState {
			// bit flags for each of the buttons. Use ButtonMaskFromId to turn an ID into a mask
			uint64_t ulButtonPressed;
			// Track the time press started to know if it's long press
			uint64_t startTimeMilisec;
		};

		VRSystem()
			: leftPacket(0), rightPacket(0), vrHook(RequestOpenVRHookManagerObject()), roomNode(nullptr) {
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
			if (!vrHook) {
				return;
			}
			rightControllerPrevState = rightControllerState;
			leftControllerPrevState = leftControllerState;

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

			setVRControllerLongPressState(rightControllerButtonLongPressState, rightControllerState);
			setVRControllerLongPressState(leftControllerButtonLongPressState, leftControllerState);
		}

		vr::VRControllerState_t getControllerState(TrackerType a_tracker) const {
			return a_tracker == Left ? leftControllerState : rightControllerState;
		}

		/// <summary>
		/// Get the controller state for the previous frame.
		/// Can be used to detect release of buttons (button up event).
		/// </summary>
		vr::VRControllerState_t getControllerPreviousState(TrackerType a_tracker) const {
			return a_tracker == Left ? leftControllerPrevState : rightControllerPrevState;
		}

		/// <summary>
		/// Simplified long press mechanism. Tracks the whole buttons mask with single time value.
		/// </summary>
		ControllerButtonLongPressState getControllerLongButtonPressedState(TrackerType a_tracker) const {
			return a_tracker == Left ? leftControllerButtonLongPressState : rightControllerButtonLongPressState;
		}

		/// <summary>
		/// Clear the state of the controller long press to mark it was used.
		/// </summary>
		void clearControllerLongButtonPressedState(TrackerType a_tracker) {
			(a_tracker == Left ? leftControllerButtonLongPressState : rightControllerButtonLongPressState).startTimeMilisec = 0;
		}

		void getTrackerNiTransformByName(const std::string& trackerName, NiTransform* transform);
		void getTrackerNiTransformByIndex(vr::TrackedDeviceIndex_t idx, NiTransform* transform);
		void getControllerNiTransformByName(const std::string& trackerName, NiTransform* transform);
		void debugPrint();

	private:
		void setVRControllerLongPressState(ControllerButtonLongPressState& longPressState, const vr::VRControllerState_t& controllerState) {
			if (longPressState.startTimeMilisec == 0) {
				longPressState.ulButtonPressed = controllerState.ulButtonPressed;
				if (longPressState.ulButtonPressed != 0) {
					longPressState.startTimeMilisec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
				}
			} else {
				longPressState.ulButtonPressed &= controllerState.ulButtonPressed;
				if (longPressState.ulButtonPressed == 0) {
					longPressState.startTimeMilisec = 0;
				}
			}
		}

		void applyRoomTransform(NiTransform* transform);
		void initializeDevices();

		std::string getProperty(vr::ETrackedDeviceProperty property, vr::TrackedDeviceIndex_t idx) const;
		uint32_t leftPacket;
		uint32_t rightPacket;
		OpenVRHookManagerAPI* vrHook;

		// current state of right/left controllers
		vr::VRControllerState_t rightControllerState;
		vr::VRControllerState_t leftControllerState;

		// previous state used to detect button up
		vr::VRControllerState_t rightControllerPrevState;
		vr::VRControllerState_t leftControllerPrevState;

		// Long press data
		ControllerButtonLongPressState rightControllerButtonLongPressState{0, 0};
		ControllerButtonLongPressState leftControllerButtonLongPressState{0, 0};

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
