#include "WeaponPositionConfigMode.h"
#include "Config.h"
#include "utils.h"
#include "Skeleton.h"
#include "F4VRBody.h"
#include "ui/UIManager.h"
#include "ui/UIButton.h"
#include "ui/UIToggleButton.h"
#include "ui/UIContainer.h"
#include "ui/UIToggleGroupContainer.h"


namespace F4VRBody {
	/**
	 * On release, we need to remove the UI from global manager.
	 */
	WeaponPositionConfigMode::~WeaponPositionConfigMode() {
		if (_configUI) {
			// remove the UI
			ui::g_uiManager->detachElement(_configUI, true);
		}
	}

	/**
	 * Handle configuration UI interaction.
	 */
	void WeaponPositionConfigMode::onFrameUpdate(NiNode* weapon) const {
		if (g_configurationMode->isCalibrateModeActive()) {
			// don't show this config UI if main config UI is shown
			_configUI->setVisibility(false);
			return;
		}
		_configUI->setVisibility(true);

		if (!weapon) {
			// don't handle repositioning if no weapon is drawn
			_mainContainer->setVisibility(false);
			_noEquippedWeaponContainer->setVisibility(true);
			return;
		}
		_mainContainer->setVisibility(true);
		_noEquippedWeaponContainer->setVisibility(false);

		// show the right footer
		const bool weaponAdjust = _repositionTarget == RepositionTarget::Weapon;
		_footerForWeaponAdjust->setVisibility(weaponAdjust);
		_footerForOtherAdjust->setVisibility(!weaponAdjust);

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

		const float leftHandedMult = g_config->leftHandedMode ? -1.f : 1.f;

		// Update the weapon transform by player thumbstick and buttons input.
		// Depending on buttons pressed can horizontal/vertical position or rotation.
		if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_Grip)) {
			Matrix44 rot;
			// pitch and yaw rotation by primary stick, roll rotation by secondary stick
			rot.setEulerAngles(-degrees_to_rads(primAxisY / 5), -degrees_to_rads(secAxisX / 3), degrees_to_rads(primAxisX / 5));
			_adjuster->_weaponOffsetTransform.rot = rot.multiply43Left(_adjuster->_weaponOffsetTransform.rot);
		} else {
			// adjust horizontal (y - right/left, x - forward/backward) by primary stick
			_adjuster->_weaponOffsetTransform.pos.y += leftHandedMult * primAxisX / 10;
			_adjuster->_weaponOffsetTransform.pos.x += primAxisY / 10;
			// adjust vertical (z - up/down) by secondary stick
			_adjuster->_weaponOffsetTransform.pos.z -= leftHandedMult * secAxisY / 10;
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
			Matrix44 rot;
			rot.setEulerAngles(-degrees_to_rads(axisY / 5), 0, degrees_to_rads(axisX / 5));
			_adjuster->_offhandOffsetRot = rot.multiply43Left(_adjuster->_offhandOffsetRot);
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
		_MESSAGE("Reset Reposition Config for target: %d, Weapon: %s", _repositionTarget, _adjuster->_lastWeapon.c_str());
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
		_MESSAGE("Save Reposition Config for target: %d, Weapon: %s", _repositionTarget, _adjuster->_lastWeapon.c_str());
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
		g_config->removeWeaponOffsets(_adjuster->_lastWeapon, _adjuster->_lastWeaponInPA ? WeaponOffsetsMode::powerArmor : WeaponOffsetsMode::normal);
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
		_adjuster->_offhandOffsetRot = Matrix44::getIdentity43();
		g_config->removeWeaponOffsets(_adjuster->_lastWeapon, _adjuster->_lastWeaponInPA ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
	}

	void WeaponPositionConfigMode::saveOffhandConfig() const {
		ShowNotification("Saving Offhand Position");
		_adjuster->_vrHook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4f);
		NiTransform transform;
		transform.scale = 1;
		transform.pos = NiPoint3(0, 0, 0);
		transform.rot = _adjuster->_offhandOffsetRot;
		g_config->saveWeaponOffsets(_adjuster->_lastWeapon, transform, _adjuster->_lastWeaponInPA ? WeaponOffsetsMode::offHandwithPowerArmor : WeaponOffsetsMode::offHand);
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

	/**
	 * Create the configuration UI
	 */
	void WeaponPositionConfigMode::createConfigUI() {
		const auto weaponModeButton = std::make_shared<ui::UIToggleButton>("FRIK/ui_weapconf_btn_weapon.nif");
		weaponModeButton->setToggleState(true);
		weaponModeButton->setOnToggleHandler([this](ui::UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::Weapon; });

		const auto offhandModeButton = std::make_shared<ui::UIToggleButton>("FRIK/ui_weapconf_btn_offhand.nif");
		offhandModeButton->setOnToggleHandler([this](ui::UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::Offhand; });

		const auto firstRowContainer = std::make_shared<ui::UIToggleGroupContainer>(ui::UIContainerLayout::HorizontalCenter, 0.3f);
		firstRowContainer->addElement(weaponModeButton);
		firstRowContainer->addElement(offhandModeButton);

		if (isBetterScopesVRModLoaded()) {
			const auto betterScopesModeButton = std::make_shared<ui::UIToggleButton>("FRIK/ui_weapconf_btn_better_scopes_vr.nif");
			betterScopesModeButton->setOnToggleHandler([this](ui::UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::BetterScopes; });
			firstRowContainer->addElement(betterScopesModeButton);
		}

		const auto saveButton = std::make_shared<ui::UIButton>("FRIK/ui_common_btn_save.nif");
		saveButton->setOnPressHandler([this](ui::UIWidget* widget) { saveConfig(); });

		const auto resetButton = std::make_shared<ui::UIButton>("FRIK/ui_common_btn_reset.nif");
		resetButton->setOnPressHandler([this](ui::UIWidget* widget) { resetConfig(); });

		const auto exitButton = std::make_shared<ui::UIButton>("FRIK/ui_common_btn_exit.nif");
		exitButton->setOnPressHandler([this](ui::UIWidget* widget) { _adjuster->toggleWeaponRepositionMode(); });

		const auto secondRowContainer = std::make_shared<ui::UIContainer>(ui::UIContainerLayout::HorizontalCenter, 0.3f);
		secondRowContainer->addElement(saveButton);
		secondRowContainer->addElement(resetButton);
		secondRowContainer->addElement(exitButton);

		const auto header = std::make_shared<ui::UIWidget>("FRIK/ui_weapconf_header.nif");
		header->setSize(14, 2);
		_footerForWeaponAdjust = std::make_shared<ui::UIWidget>("FRIK/ui_weapconf_footer.nif");
		_footerForWeaponAdjust->setSize(14, 4.5);
		_footerForOtherAdjust = std::make_shared<ui::UIWidget>("FRIK/ui_weapconf_footer_2.nif");
		_footerForOtherAdjust->setSize(10, 4.5);

		_mainContainer = std::make_shared<ui::UIContainer>(ui::UIContainerLayout::VerticalCenter, 0.4f);
		_mainContainer->addElement(firstRowContainer);
		_mainContainer->addElement(secondRowContainer);
		_mainContainer->addElement(_footerForWeaponAdjust);
		_mainContainer->addElement(_footerForOtherAdjust);

		const auto footerEmpty = std::make_shared<ui::UIWidget>("FRIK/ui_weapconf_footer_empty.nif");
		footerEmpty->setSize(7.2, 4.5);
		const auto exitButtonOnFooter = std::make_shared<ui::UIButton>("FRIK/ui_common_btn_exit.nif");
		exitButtonOnFooter->setOnPressHandler([this](ui::UIWidget* widget) { _adjuster->toggleWeaponRepositionMode(); });

		_noEquippedWeaponContainer = std::make_shared<ui::UIContainer>(ui::UIContainerLayout::HorizontalCenter, 0.4f);
		_noEquippedWeaponContainer->addElement(footerEmpty);
		_noEquippedWeaponContainer->addElement(exitButtonOnFooter);

		_configUI = std::make_shared<ui::UIContainer>(ui::UIContainerLayout::VerticalCenter, 0.5f);
		_configUI->setPosition(-14, 8, -7);
		_configUI->addElement(header);
		_configUI->addElement(_mainContainer);
		_configUI->addElement(_noEquippedWeaponContainer);

		ui::g_uiManager->attachElementToPrimaryWand(_configUI);
	}
}
