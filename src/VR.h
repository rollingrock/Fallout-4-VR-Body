#pragma once

#include <chrono>
#include <map>
#include <string>
#include "api/PapyrusVRAPI.h"
#include "f4se/NiNodes.h"
#include "f4se/NiTypes.h"

namespace VRHook {
	class VRSystem {
	public:
		enum class TrackerType : std::uint8_t {
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
			: _leftPacket(0), _rightPacket(0), _vrHook(RequestOpenVRHookManagerObject()), _roomNode(nullptr) {
			initializeDevices();
		}

		void setRoomNode(NiNode* node) {
			_roomNode = node;
		}

		OpenVRHookManagerAPI* getHook() const {
			return _vrHook;
		}

		void updatePoses() {
			_vrHook->GetVRCompositor()->GetLastPoses(_renderPoses, vr::k_unMaxTrackedDeviceCount, _gamePoses, vr::k_unMaxTrackedDeviceCount);
		}

		bool viveTrackersPresent() const {
			return !_viveTrackers.empty();
		}

		void setVRControllerState() {
			if (!_vrHook) {
				return;
			}
			_rightControllerPrevState = _rightControllerState;
			_leftControllerPrevState = _leftControllerState;

			const auto vrSystem = _vrHook->GetVRSystem();
			const auto lefthand = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
			const auto righthand = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

			vrSystem->GetControllerState(lefthand, &_leftControllerState, sizeof(vr::VRControllerState_t));
			vrSystem->GetControllerState(righthand, &_rightControllerState, sizeof(vr::VRControllerState_t));

			if (_rightControllerState.unPacketNum != _rightPacket) {
				_rightPacket = _rightControllerState.unPacketNum;
			}
			if (_leftControllerState.unPacketNum != _leftPacket) {
				_leftPacket = _leftControllerState.unPacketNum;
			}

			setVRControllerLongPressState(_rightControllerButtonLongPressState, _rightControllerState);
			setVRControllerLongPressState(_leftControllerButtonLongPressState, _leftControllerState);
		}

		vr::VRControllerState_t getControllerState(const TrackerType a_tracker) const {
			return a_tracker == TrackerType::Left ? _leftControllerState : _rightControllerState;
		}

		/**
		 * Get the controller state for the previous frame.
		 * Can be used to detect release of buttons (button up event).
		 */
		vr::VRControllerState_t getControllerPreviousState(const TrackerType a_tracker) const {
			return a_tracker == TrackerType::Left ? _leftControllerPrevState : _rightControllerPrevState;
		}

		/**
		 * Simplified long press mechanism. Tracks the whole buttons mask with single time value.
		 */
		ControllerButtonLongPressState getControllerLongButtonPressedState(const TrackerType a_tracker) const {
			return a_tracker == TrackerType::Left ? _leftControllerButtonLongPressState : _rightControllerButtonLongPressState;
		}

		/**
		 * Clear the state of the controller long press to mark it was used.
		 */
		void clearControllerLongButtonPressedState(const TrackerType a_tracker) {
			(a_tracker == TrackerType::Left ? _leftControllerButtonLongPressState : _rightControllerButtonLongPressState).startTimeMilisec = 0;
		}

		void getTrackerNiTransformByName(const std::string& trackerName, NiTransform* transform);
		void getTrackerNiTransformByIndex(vr::TrackedDeviceIndex_t idx, NiTransform* transform) const;
		void getControllerNiTransformByName(const std::string& trackerName, NiTransform* transform);
		void debugPrint();

	private:
		static void setVRControllerLongPressState(ControllerButtonLongPressState& longPressState, const vr::VRControllerState_t& controllerState) {
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

		void applyRoomTransform(NiTransform* transform) const;
		void initializeDevices();

		std::string getProperty(vr::ETrackedDeviceProperty property, vr::TrackedDeviceIndex_t idx) const;
		uint32_t _leftPacket;
		uint32_t _rightPacket;
		OpenVRHookManagerAPI* _vrHook;

		// current state of right/left controllers
		vr::VRControllerState_t _rightControllerState;
		vr::VRControllerState_t _leftControllerState;

		// previous state used to detect button up
		vr::VRControllerState_t _rightControllerPrevState;
		vr::VRControllerState_t _leftControllerPrevState;

		// Long press data
		ControllerButtonLongPressState _rightControllerButtonLongPressState{.ulButtonPressed = 0, .startTimeMilisec = 0};
		ControllerButtonLongPressState _leftControllerButtonLongPressState{.ulButtonPressed = 0, .startTimeMilisec = 0};

		vr::TrackedDevicePose_t _renderPoses[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t _gamePoses[vr::k_unMaxTrackedDeviceCount];
		std::map<std::string, vr::TrackedDeviceIndex_t> _viveTrackers;
		std::map<std::string, vr::TrackedDeviceIndex_t> _controllers;
		NiNode* _roomNode;
	};

	extern VRSystem* g_vrHook;

	inline void InitVRSystem() {
		g_vrHook = new VRSystem();
	}
}
