#include "WeaponPositionConfigMode.h"
#include "Config.h"
#include "utils.h"
#include "Skeleton.h"
#include "F4VRBody.h"

namespace F4VRBody {
	/**
	 * Handle configuration UI interaction.
	 */
	void WeaponPositionConfigMode::onFrameUpdate(NiNode* weapon) {
		// TODO: remove after testing (quick change of reposition target)
		if (isButtonPressedOnController(true, vr::EVRButtonId::k_EButton_ApplicationMenu)) {
			_repositionTarget = static_cast<RepositionTarget>((static_cast<int>(_repositionTarget) + 1) % 3);
			_MESSAGE("Change Reposition Config Target to: %d", _repositionTarget);
		}

		// reposition
		handleReposition(weapon);

		// reset
		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_Grip)) {
			resetConfig();
		}
		// save
		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_A)) {
			saveConfig();
		}
	}

	/**
	 * Handle reposition by user input of the target config.
	 */
	void WeaponPositionConfigMode::handleReposition(NiNode* weapon) const {
		switch (_repositionTarget) {
		case RepositionTarget::Weapon:
			handleWeaponReposition(weapon);
			break;
		case RepositionTarget::Offhand:
			handleOffhandReposition(weapon);
			break;
		case RepositionTarget::BetterScopes:
			handleBetterScopesReposition();
			break;
		}
	}

	/**
	 * In weapon reposition mode...
	 */
	void WeaponPositionConfigMode::handleWeaponReposition(NiNode* weapon) const {
		const auto [primAxisX, primAxisY] = getControllerState(true).rAxis[0];
		const auto [secAxisX, secAxisY] = getControllerState(false).rAxis[0];
		if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
			return;
		}

		// Update the weapon transform by player thumbstick and buttons input.
		// Depending on buttons pressed can horizontal/vertical position or rotation.
		if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_Grip)) {
			Matrix44 rot;
			// pitch and yaw rotation by primary stick, roll rotation by secondary stick
			rot.setEulerAngles(-degrees_to_rads(primAxisY / 5), -degrees_to_rads(secAxisX / 3), degrees_to_rads(primAxisX / 5));
			_adjuster->_weaponOffsetTransform.rot = rot.multiply43Left(_adjuster->_weaponOffsetTransform.rot);
		} else {
			// adjust horizontal (y - right/left, x - forward/backward) by primary stick
			_adjuster->_weaponOffsetTransform.pos.y += primAxisX / 10;
			_adjuster->_weaponOffsetTransform.pos.x += primAxisY / 10;
			// adjust vertical (z - up/down) by secondary stick
			_adjuster->_weaponOffsetTransform.pos.z -= secAxisY / 10;
		}

		// update the weapon with the offset change
		weapon->m_localTransform = _adjuster->_weaponOffsetTransform;
	}

	/**
	 * In offhand reposition mode...
	 */
	void WeaponPositionConfigMode::handleOffhandReposition(NiNode* weapon) const {
		// Update the offset position by player thumbstick.
		const auto [axisX, axisY] = getControllerState(true).rAxis[0];
		if (axisX != 0.f || axisY != 0.f) {
			// adjust horizontal (y - up/down, x - right/left)
			_adjuster->_offhandOffsetPos.z += axisY / 10;
			_adjuster->_offhandOffsetPos.x += axisX / 10;
		}
	}

	/**
	 * Handle configuration for BetterScopesVR mod.
	 */
	void WeaponPositionConfigMode::handleBetterScopesReposition() {
		const auto [axisX, axisY] = getControllerState(true).rAxis[0];
		if (axisX != 0.f || axisY != 0.f) {
			// Axis_state y is up and down, which corresponds to reticule z axis
			NiPoint3 msgData(axisX / 10, 0.f, axisY / 10);
			g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
		}
	}

	void WeaponPositionConfigMode::resetConfig() const {
		switch (_repositionTarget) {
		case RepositionTarget::Weapon:
			resetWeaponConfig();
			break;
		case RepositionTarget::Offhand:
			resetOffhandConfig();
			break;
		case RepositionTarget::BetterScopes:
			resetBetterScopesConfig();
			break;
		}
	}

	void WeaponPositionConfigMode::saveConfig() const {
		switch (_repositionTarget) {
		case RepositionTarget::Weapon:
			saveWeaponConfig();
			break;
		case RepositionTarget::Offhand:
			saveOffhandConfig();
			break;
		case RepositionTarget::BetterScopes:
			saveBetterScopesConfig();
			break;
		}
	}

	void WeaponPositionConfigMode::resetWeaponConfig() const {
		ShowNotification("Reset Weapon Position to Default");
		_adjuster->_vrHook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4f);
		_adjuster->_weaponOffsetTransform = _adjuster->_weaponOriginalTransform;
	}

	void WeaponPositionConfigMode::saveWeaponConfig() const {
		ShowNotification("Saving Weapon Position");
		_adjuster->_vrHook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4f);
		g_config->saveWeaponOffsets(_adjuster->_lastWeapon, _adjuster->_weaponOffsetTransform,
			_adjuster->_lastWeaponInPA ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
	}

	void WeaponPositionConfigMode::resetOffhandConfig() const {
		ShowNotification("Reset Offhand Position to Default");
		_adjuster->_vrHook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4f);
		_adjuster->_offhandOffsetPos = getDefaultOffhandTransform();
	}

	void WeaponPositionConfigMode::saveOffhandConfig() const {
		ShowNotification("Saving Offhand Position");
		_adjuster->_vrHook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4f);
		NiTransform transform;
		transform.rot = Matrix44().make43();
		transform.scale = 1;
		transform.pos = _adjuster->_offhandOffsetPos;
		g_config->saveWeaponOffsets(_adjuster->_lastWeapon, transform,
			_adjuster->_lastWeaponInPA ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
	}

	void WeaponPositionConfigMode::resetBetterScopesConfig() const {
		ShowNotification("Reset BetterScopesVR Scope Offset to Default");
		_adjuster->_vrHook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4f);
		NiPoint3 msgData(0.f, 0.f, 0.f);
		g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
	}

	void WeaponPositionConfigMode::saveBetterScopesConfig() const {
		ShowNotification("Saving BetterScopesVR Scopes Offset");
		_adjuster->_vrHook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4f);
		NiPoint3 msgData(0.f, 1, 0.f);
		g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
	}
}
