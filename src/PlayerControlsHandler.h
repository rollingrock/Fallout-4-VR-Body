#pragma once

#include "PapyrusGateway.h"
#include "WeaponPositionAdjuster.h"
#include "f4vr/GameMenusHandler.h"

namespace frik {
	class PlayerControlsHandler {
	public:
		/**
		 * It's just easier to have single place that controls if player controls are enabled or not.
		 * The enable/disable functions have to be idempotent to avoid issues and simplify logic on when we enable/disable player controls.
		 */
		void onFrameUpdate(const Pipboy* pipboy, const WeaponPositionAdjuster* weaponPosition, f4vr::GameMenusHandler* gameMenusHandler) {
			if (pipboy->status()) {
				disableFull();
				return;
			}

			if (_state == State::DISABLED_FULL) {
				// safer to re-enable if was fully disabled and then apply whatever other states (handle draw weapon after opening pip-boy during reposition)
				enable();
				return;
			}

			if (weaponPosition->inWeaponRepositionMode()) {
				if (gameMenusHandler->isPauseMenuOpen() || isAnyPipboyOpen()) {
					// In case the pause menu or Pipboy if opened while weapon repositioning, re-enable the thumbstick controllers
					setControlsThumbstickEnableState(true);
				} else {
					disableWithoutWeapon();
					// double tap in-case we only enabled the thumbstick controllers above
					setControlsThumbstickEnableState(false);
				}
				return;
			}

			if (pipboy->isOperatingPipboy()) {
				disableWeaponOnly();
				return;
			}

			// nothing to disable so it should be enabled
			enable();
		}

	private:
		/**
		 * Enable all player controls.
		 * @param drawWeapon if true will draw the equipped weapon (useful if disableWeapon was used)
		 */
		void enable(const bool drawWeapon = true) {
			if (_state != State::ENABLED) {
				_state = State::ENABLED;
				common::Log::info("Player controls - enabled");
				PapyrusGateway::instance()->enablePlayerControls(drawWeapon);
				setControlsThumbstickEnableState(true);
			}
		}

		/**
		 * Disable all player controls. no movement, no rotation, no weapon use, no VATS, no favorite.
		 */
		void disableFull() {
			if (_state != State::DISABLED_FULL) {
				_state = State::DISABLED_FULL;
				common::Log::info("Player controls - disabled full");
				PapyrusGateway::instance()->disablePlayerControls(true, true);
			}
		}

		/**
		 * Disable only the use of weapons.
		 */
		void disableWeaponOnly() {
			if (_state != State::DISABLED_WEAPON_ONLY) {
				_state = State::DISABLED_WEAPON_ONLY;
				common::Log::info("Player controls - disabled weapon");
				PapyrusGateway::instance()->enableDisableFighting(false, false);
			}
		}

		/**
		 * Disable player controls but allowing the use of weapons.
		 */
		void disableWithoutWeapon() {
			if (_state != State::DISABLED_WITHOUT_WEAPON) {
				_state = State::DISABLED_WITHOUT_WEAPON;
				common::Log::info("Player controls - disabled WITHOUT weapon");
				PapyrusGateway::instance()->disablePlayerControls(false, false);

				// Because restrain blocks weapon use (throwable) we don't use it but have to disable primary hand thumbstick in a different way
				setControlsThumbstickEnableState(false);
			}
		}

		/**
		 * If to enable/disable the use of primary controller analog thumbstick.
		 * Optional: add "fDirectionalDeadzone:Controls" to restrict secondary hand thumbstick as well.
		 */
		void setControlsThumbstickEnableState(const bool toEnable) {
			if (_controlsThumbstickEnableState == toEnable) {
				return; // no change
			}
			_controlsThumbstickEnableState = toEnable;
			common::Log::verbose("Set player controls thumbstick '%s'", toEnable ? "enable" : "disable");
			if (toEnable) {
				f4vr::setIniSettingFloat("fLThumbDeadzone:Controls", _controlsThumbstickOriginalDeadzone);
				f4vr::setIniSettingFloat("fLThumbDeadzoneMax:Controls", _controlsThumbstickOriginalDeadzoneMax);
			} else {
				const auto controlsThumbstickOriginalDeadzone = f4vr::getIniSettingFloat("fLThumbDeadzone:Controls");
				if (controlsThumbstickOriginalDeadzone < 1) {
					_controlsThumbstickOriginalDeadzone = controlsThumbstickOriginalDeadzone;
					_controlsThumbstickOriginalDeadzoneMax = f4vr::getIniSettingFloat("fLThumbDeadzoneMax:Controls");
				} else {
					common::Log::warn("Controls thumbstick deadzone is already set to 1.0, not changing it.");
				}
				f4vr::setIniSettingFloat("fLThumbDeadzone:Controls", 1.0);
				f4vr::setIniSettingFloat("fLThumbDeadzoneMax:Controls", 1.0);
			}
		}

		enum class State : UINT8 {
			ENABLED = 0,
			DISABLED_FULL,
			DISABLED_WEAPON_ONLY,
			DISABLED_WITHOUT_WEAPON
		};

		State _state = State::ENABLED;

		bool _controlsThumbstickEnableState = true;
		float _controlsThumbstickOriginalDeadzone = 0.25f;
		float _controlsThumbstickOriginalDeadzoneMax = 0.94f;
	};
}
