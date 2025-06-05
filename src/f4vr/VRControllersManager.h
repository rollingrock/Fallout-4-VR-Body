#pragma once

#include <chrono>
#include <unordered_map>

#include "../api/openvr.h"

namespace f4vr {
	// TODO: remove this after migrating all code from getControllerState_DEPRECATED
	enum class TrackerType : std::uint8_t {
		HMD,
		Left,
		Right,
		Vive
	};

	enum class Hand : std::uint8_t {
		Primary,
		Offhand
	};

	/**
	 * Manages VR controller input states and button interaction logic
	 */
	class VRControllersManager {
	public:
		/**
		 * Update controller states; must be called each frame
		 */
		void update() {
			if (!vr::VRSystem()) {
				return;
			}

			_leftHanded = isLeftHandedMode();

			_left.index = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
			_right.index = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);

			const auto now = getCurrentTimeSeconds();

			_left.update(_left.index, now);
			_right.update(_right.index, now);

			_currentTime = now;
		}

		/**
		 * Sets the debounce cooldown time (in seconds) between consecutive presses
		 */
		void setDebounceCooldown(const float seconds) {
			_debounceCooldown = seconds;
		}

		vr::VRControllerState_t getControllerState_DEPRECATED(const TrackerType a_tracker) const {
			return a_tracker == TrackerType::Left ? _left.current : _right.current;
		}

		/**
		 * Returns true if the specified button is being touched on the given hand
		 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
		 */
		bool isTouching(const vr::EVRButtonId button, const Hand primaryHand) const {
			return isTouching(button, getHand(primaryHand));
		}

		bool isTouching(const int button, const Hand primaryHand) const {
			return isTouching(static_cast<vr::EVRButtonId>(button), getHand(primaryHand));
		}

		bool isTouching(const vr::EVRButtonId button, const vr::ETrackedControllerRole hand) const {
			return get(hand).isTouching(button);
		}

		/**
		 * Returns true if the button was just pressed, factoring in debounce
		 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
		 */
		bool isPressed(const vr::EVRButtonId button, const Hand primaryHand) {
			return isPressed(button, getHand(primaryHand));
		}

		bool isPressed(const int button, const Hand primaryHand) {
			return isPressed(static_cast<vr::EVRButtonId>(button), getHand(primaryHand));
		}

		bool isPressed(const vr::EVRButtonId button, const vr::ETrackedControllerRole hand) {
			return get(hand).justPressed(button, _currentTime, _debounceCooldown);
		}

		/**
		 * Returns true if the button is currently held down
		 * This will return true for EVERY frame while the button is pressed.
		 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
		 */
		bool isPressHeldDown(const vr::EVRButtonId button, const Hand primaryHand) const {
			return isPressHeldDown(button, getHand(primaryHand));
		}

		bool isPressHeldDown(const int button, const Hand primaryHand) const {
			return isPressHeldDown(static_cast<vr::EVRButtonId>(button), getHand(primaryHand));
		}

		bool isPressHeldDown(const vr::EVRButtonId button, const vr::ETrackedControllerRole hand) const {
			return get(hand).isPressed(button);
		}

		/**
		 * Returns true if the specified button was just released
		 * This will return true for ONE frame only when the button is first released.
		 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
		 */
		bool isReleased(const vr::EVRButtonId button, const Hand primaryHand) {
			return isReleased(button, getHand(primaryHand));
		}

		bool isReleased(const int button, const Hand primaryHand) {
			return isReleased(static_cast<vr::EVRButtonId>(button), getHand(primaryHand));
		}

		bool isReleased(const vr::EVRButtonId button, const vr::ETrackedControllerRole hand) {
			return get(hand).justReleased(button, _currentTime, _debounceCooldown);
		}

		/**
		 * Returns true if the button has been held longer than a threshold.
		 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
		 * If <clear> false: will return true for EVERY frame when the button is pressed for longer then <durationSeconds>.
		 * If <clear> true: will return true for ONE frame when the button is pressed for longer then <durationSeconds>.
		 */
		bool isLongPressed(const vr::EVRButtonId button, const Hand primaryHand, const float durationSeconds = 0.6f, const bool clear = true) {
			return isLongPressed(button, getHand(primaryHand), durationSeconds, clear);
		}

		bool isLongPressed(const vr::EVRButtonId button, const vr::ETrackedControllerRole hand, const float durationSeconds = 0.6f, const bool clear = true) {
			auto& state = get(hand);
			const bool isLongPressed = state.isPressed(button) && state.getHeldDuration(button, _currentTime) >= durationSeconds;
			if (clear && isLongPressed) {
				state.clearHeldDuration(button);
			}
			return isLongPressed;
		}

		/**
		 * Retrieves analog axis value for the specified controller.
		 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
		 */
		vr::VRControllerAxis_t getAxisValue(const Hand primaryHand, const uint32_t axisIndex = 0) const {
			return getAxisValue(getHand(primaryHand), axisIndex);
		}

		vr::VRControllerAxis_t getAxisValue(const vr::ETrackedControllerRole hand, const uint32_t axisIndex = 0) const {
			return get(hand).getAxis(axisIndex);
		}

		/**
		 * Trigger a haptic pulse on the specified controller for specific duration and intensity.
		 * Regular primary is right hand, but if left hand mode is on then primary is left hand.
		 * Duration in seconds and intensity between 0.0 and 1.0.
		 */
		void triggerHaptic(const Hand primaryHand, const float durationSeconds = 0.1f, const float intensity = 0.3f) {
			return triggerHaptic(getHand(primaryHand), durationSeconds, intensity);
		}

		void triggerHaptic(const vr::ETrackedControllerRole hand, const float durationSeconds = 0.2f, const float intensity = 0.3f) {
			get(hand).startHaptic(_currentTime + durationSeconds, intensity);
		}

	private:
		// Internal state for one controller (left or right)
		struct ControllerState {
			vr::TrackedDeviceIndex_t index = vr::k_unTrackedDeviceIndexInvalid;
			vr::VRControllerState_t current{};
			vr::VRControllerState_t previous{};
			bool valid = false;
			std::unordered_map<vr::EVRButtonId, float> pressStartTimes; // Track how long each button has been held
			std::unordered_map<vr::EVRButtonId, float> lastPressTime;
			std::unordered_map<vr::EVRButtonId, float> lastReleaseTime;
			std::unordered_map<vr::EVRButtonId, bool> longPressHandled; // Track if long press was handled
			float hapticEndTime = 0;
			float hapticIntensity = 0;

			// Updates the controller state and tracks press transitions
			void update(const vr::TrackedDeviceIndex_t newIndex, const float now) {
				if (newIndex == vr::k_unTrackedDeviceIndexInvalid || !vr::VRSystem()) {
					valid = false;
					return;
				}

				previous = current;
				valid = vr::VRSystem()->GetControllerState(newIndex, &current, sizeof(current));
				if (!valid) {
					return;
				}

				// Update press start times for all button transitions
				for (int b = vr::k_EButton_System; b <= vr::k_EButton_Max; ++b) {
					auto button = static_cast<vr::EVRButtonId>(b);
					const bool isNowPressed = (current.ulButtonPressed & vr::ButtonMaskFromId(button)) != 0;
					const bool wasPressed = (previous.ulButtonPressed & vr::ButtonMaskFromId(button)) != 0;

					if (isNowPressed && !wasPressed) {
						pressStartTimes[button] = now;
					} else if (!isNowPressed && wasPressed) {
						pressStartTimes.erase(button);
					}
				}

				// handle haptic
				if (now < hapticEndTime) {
					vr::VRSystem()->TriggerHapticPulse(index, 0, static_cast<uint16_t>(hapticIntensity * 3000));
				}
			}

			bool isPressed(const vr::EVRButtonId button) const {
				return valid && current.ulButtonPressed & vr::ButtonMaskFromId(button);
			}

			bool isTouching(const vr::EVRButtonId button) const {
				return valid && current.ulButtonTouched & vr::ButtonMaskFromId(button);
			}

			bool justPressed(const vr::EVRButtonId button, const float now, const float debounceCooldown) {
				if (!valid) {
					return false;
				}
				const auto mask = vr::ButtonMaskFromId(button);
				const bool wasJustPressed = !(previous.ulButtonPressed & mask) && current.ulButtonPressed & mask;
				return wasJustPressed && checkDebounce(lastPressTime[button], now, debounceCooldown);
			}

			bool justReleased(const vr::EVRButtonId button, const float now, const float debounceCooldown) {
				if (!valid || longPressHandled[button]) {
					longPressHandled[button] = false;
					return false;
				}
				const auto mask = vr::ButtonMaskFromId(button);
				const bool wasJustReleased = previous.ulButtonPressed & mask && !(current.ulButtonPressed & mask);
				return wasJustReleased && checkDebounce(lastReleaseTime[button], now, debounceCooldown);
			}

			static bool checkDebounce(float& lastTime, const float now, const float cooldown) {
				if (now - lastTime < cooldown) {
					return false;
				}
				lastTime = now;
				return true;
			}

			vr::VRControllerAxis_t getAxis(const uint32_t axisIndex) const {
				if (!valid || axisIndex >= vr::k_unControllerStateAxisCount) {
					return {};
				}
				return current.rAxis[axisIndex];
			}

			float getHeldDuration(const vr::EVRButtonId button, const float now) const {
				const auto it = pressStartTimes.find(button);
				if (it == pressStartTimes.end()) {
					return 0.0f;
				}
				return now - it->second;
			}

			void clearHeldDuration(const vr::EVRButtonId button) {
				pressStartTimes.erase(button);
				// Mark long press as handled so isReleased won't fire
				longPressHandled[button] = true;
			}

			void startHaptic(const float endTime, const float intensity) {
				if (!vr::VRSystem()) {
					return;
				}
				hapticEndTime = endTime;
				hapticIntensity = intensity;
				if (valid) {
					triggerHapticPulse();
				}
			}

			/**
			 * OpenVR "TriggerHapticPulse" API is incorrect.
			 * The <usDurationMicroSec> argument is actually "intensity" with values somewhere between 0 and 3999. (I didn't feel any change above 3000)
			 * The only way to actually handle duration is to repeat the pulse every frame until the duration is over.
			 */
			void triggerHapticPulse() const {
				vr::VRSystem()->TriggerHapticPulse(index, 0, max(0, min(3000, static_cast<uint16_t>( hapticIntensity * 3000))));
			}
		};

		// Returns a time value in seconds since static program start
		static float getCurrentTimeSeconds() {
			static const auto START = std::chrono::steady_clock::now();
			const auto now = std::chrono::steady_clock::now();
			const std::chrono::duration<float> elapsed = now - START;
			return elapsed.count();
		}

		// Helpers to access left or right controller states
		const ControllerState& get(const vr::ETrackedControllerRole hand) const {
			return hand == vr::TrackedControllerRole_LeftHand ? _left : _right;
		}

		ControllerState& get(const vr::ETrackedControllerRole hand) {
			return hand == vr::TrackedControllerRole_LeftHand ? _left : _right;
		}

		// Resolves controller hand based on primary hand and left-handed setting
		vr::ETrackedControllerRole getHand(const Hand primaryHand) const {
			return _leftHanded
				? primaryHand == Hand::Primary
				? vr::TrackedControllerRole_LeftHand
				: vr::TrackedControllerRole_RightHand
				: primaryHand == Hand::Primary
				? vr::TrackedControllerRole_RightHand
				: vr::TrackedControllerRole_LeftHand;
		}

		ControllerState _left;
		ControllerState _right;
		bool _leftHanded = false;
		float _currentTime = 0.0f;
		float _debounceCooldown = 0.1f; // default 100 ms
	};

	// Global singleton instance for convenient access
	inline VRControllersManager VRControllers;
}
