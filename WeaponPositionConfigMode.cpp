#include "WeaponPositionConfigMode.h"
#include "Config.h"
#include "utils.h"
#include "Skeleton.h"
#include "F4VRBody.h"

namespace F4VRBody {
	
	/// <summary>
	/// Default small offset to use if no custom transform exist.
	/// </summary>
	NiPoint3 WeaponPositionConfigMode::getDefaultOffhandTransform() {
		return NiPoint3(0, 0, 2);
	}

	/// <summary>
	/// Handle configuration UI interaction.
	/// </summary>
	void WeaponPositionConfigMode::onFrameUpdate() {
		if (isButtonReleasedOnController(true, vr::EVRButtonId::k_EButton_A)) {
			// TODO: remove after testing (quick enter/exit reposition mode)
			toggleWeaponRepositionMode();
			std::string state = _isInRepositionConfigMode ? "Enabled" : "Disabled";
			ShowNotification("Testing: Weapon Reposition Mode - " + state);
		}

		if (!_isInRepositionConfigMode) {
			return;
		}

		if (isButtonPressedOnController(true, vr::EVRButtonId::k_EButton_ApplicationMenu)) {
			// TODO: remove after testing (quick change of reposition target)
			_repositionTarget = static_cast<RepostionTarget>((_repositionTarget + 1) % 3);
			_MESSAGE("Change Reposition Config Target to: %d", _repositionTarget);
		}
	}

	/// <summary>
	/// In weapon reposition mode...
	/// </summary>
	void WeaponPositionConfigMode::handleWeaponReposition(OpenVRHookManagerAPI* vrhook, NiNode* weapon, NiTransform& offsetTransform, const NiTransform& originalTransform, std::string weaponName, bool isInPA) {
		if (!_isInRepositionConfigMode || _repositionTarget != RepostionTarget::Weapon) {
			return;
		}

		// reset
		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_Grip)) {
			ShowNotification("Reset Weapon Position to Default");
			vrhook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4);
			offsetTransform = originalTransform;
		}
		// save
		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_A)) {
			ShowNotification("Saving Weapon Position");
			vrhook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4);
			g_config->saveWeaponOffsets(weaponName, offsetTransform, isInPA ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
		}

		vr::VRControllerAxis_t axis_state = getControllerState(true).rAxis[0];
		if (axis_state.x == 0.f && axis_state.y == 0.f) {
			return;
		}

		// Update the weapon transform by player thumbstick and buttons input.
		// Depending on buttons pressed we can horizontal/vertical position or rotation.
		if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_A)) {
			Matrix44 rot;
			if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_Grip)) {
				// roll rotation
				rot.setEulerAngles(0, -degrees_to_rads(axis_state.x / 3), 0);
			}
			else {
				// pitch and yaw rotation
				rot.setEulerAngles(-degrees_to_rads(axis_state.y / 5), 0, degrees_to_rads(axis_state.x / 5));
			}
			offsetTransform.rot = rot.multiply43Left(offsetTransform.rot);
		}
		else if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_Grip)) {
			// adjust vertical (z - up/down)
			offsetTransform.pos.z -= axis_state.y / 10;
		}
		else {
			// adjust horizontal (y - right/left, x - forward/backward) (default - no buttons pressed)
			offsetTransform.pos.y += axis_state.x / 10;
			offsetTransform.pos.x += axis_state.y / 10;
		}

		// update the weapon with the offset change
		weapon->m_localTransform = offsetTransform;
	}

	/// <summary>
	/// In offhand reposition mode...
	/// </summary>
	void WeaponPositionConfigMode::handleOffhandReposition(OpenVRHookManagerAPI* vrhook, NiNode* weapon, NiPoint3& offsetPos, std::string weaponName, bool isInPA) {
		if (!_isInRepositionConfigMode || _repositionTarget != RepostionTarget::Offhand) {
			return;
		}

		// reset
		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_Grip)) {
			ShowNotification("Reset Offhand Position to Default");
			vrhook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4);
			offsetPos = getDefaultOffhandTransform();
		}
		// save
		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_A)) {
			ShowNotification("Saving Offhand Position");
			vrhook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4);
			NiTransform transform;
			transform.rot = Matrix44().make43();
			transform.scale = 1;
			transform.pos = offsetPos;
			g_config->saveWeaponOffsets(weaponName, transform, isInPA ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
		}

		// Update the offset position by player thumbstick.
		vr::VRControllerAxis_t axis_state = getControllerState(true).rAxis[0];
		if (axis_state.x != 0.f || axis_state.y != 0.f) {
			// adjust horizontal (y - up/down, x - right/left)
			offsetPos.z += axis_state.y / 10;
			offsetPos.x += axis_state.x / 10;
		}
	}

	/// <summary>
	/// Handle configuration for BetterScopesVR mod.
	/// </summary>
	void WeaponPositionConfigMode::handleBetterScopesConfig(OpenVRHookManagerAPI* vrhook) const {
		if (!_isInRepositionConfigMode || _repositionTarget != RepostionTarget::BetterScopes) {
			return;
		}

		// reset
		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_Grip)) {
			ShowNotification("Reset BetterScopesVR Scope Offset to Default");
			vrhook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4);
			NiPoint3 msgData(0.f, 0.f, 0.f);
			g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
		}
		// save
		if (checkAndClearButtonLongPressedOnController(true, vr::EVRButtonId::k_EButton_A)) {
			ShowNotification("Saving BetterScopesVR Scopes Offset");
			vrhook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4);
			NiPoint3 msgData(0.f, 1, 0.f);
			g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
		}

		vr::VRControllerAxis_t axis_state = getControllerState(true).rAxis[0];
		if (axis_state.x != 0 || axis_state.y != 0) {
			// Axis_state y is up and down, which corresponds to reticle z axis
			NiPoint3 msgData(axis_state.x / 10, 0.f, axis_state.y / 10);
			g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
		}
	}
}