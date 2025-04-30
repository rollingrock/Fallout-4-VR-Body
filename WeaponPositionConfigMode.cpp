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
	 * Get default melee weapon adjustment to match how a human holds it as game default is straight forward.
	 */
	NiTransform WeaponPositionConfigMode::getMeleeWeaponDefaultAdjustment(const NiTransform& originalTransform) {
		Matrix44 rot;
		NiTransform transform;
		transform.scale = originalTransform.scale;
		if (g_config->leftHandedMode) {
			transform.pos = NiPoint3(5.5f, -2.2f, 1);
			rot.setEulerAngles(degrees_to_rads(95), degrees_to_rads(60), 0);
		} else {
			transform.pos = NiPoint3(4.5f, -2.2f, -1);
			rot.setEulerAngles(degrees_to_rads(85), degrees_to_rads(-65), 0);
		}
		transform.rot = rot.multiply43Right(originalTransform.rot);
		return transform;
	}

	/**
	 * Get default back of the hand UI (HP,Ammo,etc.) default adjustment for empty hand and use as base to adjust for weapon.
	 */
	NiTransform WeaponPositionConfigMode::getBackOfHandUIDefaultAdjustment(const NiTransform& originalTransform, const bool inPA) {
		NiTransform transform;
		transform.scale = originalTransform.scale;
		if (g_config->leftHandedMode) {
			Matrix44 mat;
			mat.setEulerAngles(degrees_to_rads(180), 0, degrees_to_rads(180));
			transform.rot = mat.make43();
			transform.pos = inPA ? NiPoint3(5, 6.5f, -11) : NiPoint3(6.2f, 4.8f, -12.2f);
		}
		else {
			transform.rot = Matrix44::getIdentity43();
			transform.pos = inPA ? NiPoint3(-5.8f, 5.8f, 1.8f) : NiPoint3(-6.8f, 3.6f, 0.8f);
		}
		return transform;
	}

	/**
	 * Handle configuration UI interaction.
	 */
	void WeaponPositionConfigMode::onFrameUpdate(NiNode* weapon) const {
		if (g_configurationMode->isCalibrateModeActive() || g_configurationMode->isPipBoyConfigModeActive()) {
			// don't show this config UI if main config UI is shown
			_configUI->setVisibility(false);
			return;
		}
		_configUI->setVisibility(true);

		// Show/Hide UI that should be visible only when weapon is equipped
		const bool weaponEquipped = weapon != nullptr;
		_weaponModeButton->setVisibility(weaponEquipped);
		_offhandModeButton->setVisibility(weaponEquipped);
		if (_betterScopesModeButton) 
			_betterScopesModeButton->setVisibility(weaponEquipped);
		_emptyHandsMessageBox->setVisibility(!weaponEquipped);

		const bool showAnything = weaponEquipped || _repositionTarget == RepositionTarget::BackOfHandUI;
		_saveButton->setVisibility(showAnything);
		_resetButton->setVisibility(showAnything);

		// show the right footer
		const bool complexAdjust = _repositionTarget == RepositionTarget::Weapon || _repositionTarget == RepositionTarget::BackOfHandUI;
		_complexAdjustFooter->setVisibility(showAnything && complexAdjust);
		_simpleAdjustFooter->setVisibility(showAnything && !complexAdjust);

		if (!weaponEquipped && _repositionTarget != RepositionTarget::BackOfHandUI) {
			// only back of hand UI can be repositioned without weapon equipped
			return;
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
		case RepositionTarget::BackOfHandUI:
			handleBackOfHandUIReposition();
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
			_adjuster->_weaponOffsetTransform.pos.y += leftHandedMult * primAxisX / 12;
			_adjuster->_weaponOffsetTransform.pos.x += primAxisY / 12;
			// adjust vertical (z - up/down) by secondary stick
			_adjuster->_weaponOffsetTransform.pos.z -= leftHandedMult * secAxisY / 12;
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
	 * In back of hand UI reposition mode...
	 * NOTE: because of minor tweaks on what axis are used for repositioning it doesn't make sense to create common code for it.
	 */
	void WeaponPositionConfigMode::handleBackOfHandUIReposition() const {
		const auto [primAxisX, primAxisY] = getControllerState(true).rAxis[0];
		const auto [secAxisX, secAxisY] = getControllerState(false).rAxis[0];
		if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
			return;
		}

		const float leftHandedMult = g_config->leftHandedMode ? -1.f : 1.f;

		// Update the transform by player thumbstick and buttons input.
		// Depending on buttons pressed can horizontal/vertical position or rotation.
		if (isButtonPressHeldDownOnController(false, vr::EVRButtonId::k_EButton_Grip)) {
			Matrix44 rot;
			// pitch and yaw rotation by primary stick, roll rotation by secondary stick
			rot.setEulerAngles(-degrees_to_rads(secAxisY / 6), -degrees_to_rads(primAxisX / 6), -degrees_to_rads(primAxisY / 6));
			_adjuster->_backOfHandUIOffsetTransform.rot = rot.multiply43Left(_adjuster->_backOfHandUIOffsetTransform.rot);
		}
		else {
			// adjust horizontal (z - right/left, y - forward/backward) by primary stick
			_adjuster->_backOfHandUIOffsetTransform.pos.z -= leftHandedMult * primAxisX / 14;
			_adjuster->_backOfHandUIOffsetTransform.pos.y += primAxisY / 14;
			// adjust vertical (x - up/down) by secondary stick
			_adjuster->_backOfHandUIOffsetTransform.pos.x += leftHandedMult * secAxisY / 14;
		}

		// update the weapon with the offset change
		_adjuster->getBackOfHandUINode()->m_localTransform = _adjuster->_backOfHandUIOffsetTransform;
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
		_MESSAGE("Reset Reposition Config for target: %d, Weapon: %s", _repositionTarget, _adjuster->_currentWeapon.c_str());
		switch (_repositionTarget) {
		case RepositionTarget::Weapon:
			resetWeaponConfig();
			break;
		case RepositionTarget::Offhand:
			resetOffhandConfig();
			break;
		case RepositionTarget::BackOfHandUI:
			resetBackOfHandUIConfig();
			break;
		case RepositionTarget::BetterScopes:
			resetBetterScopesConfig();
			break;
		}
	}

	void WeaponPositionConfigMode::saveConfig() const {
		_MESSAGE("Save Reposition Config for target: %d, Weapon: %s", _repositionTarget, _adjuster->_currentWeapon.c_str());
		switch (_repositionTarget) {
		case RepositionTarget::Weapon:
			saveWeaponConfig();
			break;
		case RepositionTarget::Offhand:
			saveOffhandConfig();
			break;
		case RepositionTarget::BackOfHandUI:
			saveBackOfHandUIConfig();
			break;
		case RepositionTarget::BetterScopes:
			saveBetterScopesConfig();
			break;
		}
	}

	void WeaponPositionConfigMode::resetWeaponConfig() const {
		ShowNotification("Reset Weapon Position to Default");
		doHaptic();
		_adjuster->_weaponOffsetTransform = isMeleeWeaponEquipped()
			? WeaponPositionConfigMode::getMeleeWeaponDefaultAdjustment(_adjuster->_weaponOriginalTransform)
			: _adjuster->_weaponOriginalTransform;
		g_config->removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::Weapon, _adjuster->_currentlyInPA, true);
	}

	void WeaponPositionConfigMode::saveWeaponConfig() const {
		ShowNotification("Saving Weapon Position");
		doHaptic();
		g_config->saveWeaponOffsets(_adjuster->_currentWeapon, _adjuster->_weaponOffsetTransform, WeaponOffsetsMode::Weapon, _adjuster->_currentlyInPA);
	}

	void WeaponPositionConfigMode::resetOffhandConfig() const {
		ShowNotification("Reset Offhand Position to Default");
		doHaptic();
		_adjuster->_offhandOffsetRot = Matrix44::getIdentity43();
		g_config->removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::OffHand, _adjuster->_currentlyInPA, true);
	}

	void WeaponPositionConfigMode::saveOffhandConfig() const {
		ShowNotification("Saving Offhand Position");
		doHaptic();
		NiTransform transform;
		transform.scale = 1;
		transform.pos = NiPoint3(0, 0, 0);
		transform.rot = _adjuster->_offhandOffsetRot;
		g_config->saveWeaponOffsets(_adjuster->_currentWeapon, transform, WeaponOffsetsMode::OffHand, _adjuster->_currentlyInPA);
	}

	void WeaponPositionConfigMode::resetBackOfHandUIConfig() const {
		ShowNotification("Reset Back of Hand UI Position to Default");
		doHaptic();
		_adjuster->_backOfHandUIOffsetTransform = getBackOfHandUIDefaultAdjustment(_adjuster->_backOfHandUIOffsetTransform, _adjuster->_currentlyInPA);
		_adjuster->getBackOfHandUINode()->m_localTransform = _adjuster->_backOfHandUIOffsetTransform;
		g_config->removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::BackOfHandUI, _adjuster->_currentlyInPA, true);
	}

	void WeaponPositionConfigMode::saveBackOfHandUIConfig() const {
		ShowNotification("Saving Back of Hand UI Position");
		doHaptic();
		g_config->saveWeaponOffsets(_adjuster->_currentWeapon, _adjuster->_backOfHandUIOffsetTransform, WeaponOffsetsMode::BackOfHandUI, _adjuster->_currentlyInPA);
	}

	void WeaponPositionConfigMode::resetBetterScopesConfig() const {
		ShowNotification("Reset BetterScopesVR Scope Offset to Default");
		doHaptic();
		NiPoint3 msgData(0.f, 0.f, 0.f);
		g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
	}

	void WeaponPositionConfigMode::saveBetterScopesConfig() const {
		ShowNotification("Saving BetterScopesVR Scopes Offset");
		doHaptic();
		NiPoint3 msgData(0.f, 1, 0.f);
		g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
	}

	/**
	 * Create the configuration UI
	 */
	void WeaponPositionConfigMode::createConfigUI() {
		_weaponModeButton = std::make_shared<ui::UIToggleButton>("FRIK/ui_weapconf_btn_weapon.nif");
		_weaponModeButton->setToggleState(true);
		_weaponModeButton->setOnToggleHandler([this](ui::UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::Weapon; });

		_offhandModeButton = std::make_shared<ui::UIToggleButton>("FRIK/ui_weapconf_btn_offhand.nif");
		_offhandModeButton->setOnToggleHandler([this](ui::UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::Offhand; });

		const auto backOfHandUIButton = std::make_shared<ui::UIToggleButton>("FRIK/ui_weapconf_btn_back_of_hand_ui.nif");
		backOfHandUIButton->setOnToggleHandler([this](ui::UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::BackOfHandUI; });

		const auto firstRowContainerInner = std::make_shared<ui::UIToggleGroupContainer>(ui::UIContainerLayout::HorizontalCenter, 0.15f);
		firstRowContainerInner->addElement(_weaponModeButton);
		firstRowContainerInner->addElement(_offhandModeButton);
		firstRowContainerInner->addElement(backOfHandUIButton);

		if (isBetterScopesVRModLoaded()) {
			_betterScopesModeButton = std::make_shared<ui::UIToggleButton>("FRIK/ui_weapconf_btn_better_scopes_vr.nif");
			_betterScopesModeButton->setOnToggleHandler([this](ui::UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::BetterScopes; });
			firstRowContainerInner->addElement(_betterScopesModeButton);
		}

		_emptyHandsMessageBox = std::make_shared<ui::UIWidget>("FRIK/ui_weapconf_empty_hands_box.nif");
		_emptyHandsMessageBox->setSize(3.6f, 2.2f);

		const auto firstRowContainer = std::make_shared<ui::UIContainer>(ui::UIContainerLayout::HorizontalCenter, 0.15f);
		firstRowContainer->addElement(_emptyHandsMessageBox);
		firstRowContainer->addElement(firstRowContainerInner);

		_saveButton = std::make_shared<ui::UIButton>("FRIK/ui_common_btn_save.nif");
		_saveButton->setOnPressHandler([this](ui::UIWidget* widget) { saveConfig(); });

		_resetButton = std::make_shared<ui::UIButton>("FRIK/ui_common_btn_reset.nif");
		_resetButton->setOnPressHandler([this](ui::UIWidget* widget) { resetConfig(); });

		const auto exitButton = std::make_shared<ui::UIButton>("FRIK/ui_common_btn_exit.nif");
		exitButton->setOnPressHandler([this](ui::UIWidget* widget) { _adjuster->toggleWeaponRepositionMode(); });

		const auto secondRowContainer = std::make_shared<ui::UIContainer>(ui::UIContainerLayout::HorizontalCenter, 0.2f);
		secondRowContainer->addElement(_saveButton);
		secondRowContainer->addElement(_resetButton);
		secondRowContainer->addElement(exitButton);

		const auto header = std::make_shared<ui::UIWidget>("FRIK/ui_weapconf_header.nif", 0.7f);
		header->setSize(8, 1);
		_complexAdjustFooter = std::make_shared<ui::UIWidget>("FRIK/ui_weapconf_footer.nif", 0.7f);
		_complexAdjustFooter->setSize(7, 2.2f);
		_simpleAdjustFooter = std::make_shared<ui::UIWidget>("FRIK/ui_weapconf_footer_2.nif", 0.7f);
		_simpleAdjustFooter->setSize(5, 2.2f);
		_simpleAdjustFooter->setVisibility(false);

		const auto mainContainer = std::make_shared<ui::UIContainer>(ui::UIContainerLayout::VerticalCenter, 0.2f);
		mainContainer->addElement(firstRowContainer);
		mainContainer->addElement(secondRowContainer);
		mainContainer->addElement(_complexAdjustFooter);
		mainContainer->addElement(_simpleAdjustFooter);
		
		_configUI = std::make_shared<ui::UIContainer>(ui::UIContainerLayout::VerticalDown, 0.3f, 1.7f);
		_configUI->addElement(header);
		_configUI->addElement(mainContainer);

		// start hidden by default (will be set visible in frame update if it should be)
		_configUI->setVisibility(false);

		ui::g_uiManager->attachPresetToPrimaryWandLeft(_configUI, g_config->leftHandedMode, {0, -4, 0});
	}

	/**
	 * Run a simple haptic
	 */
	void WeaponPositionConfigMode::doHaptic() const {
		_adjuster->_vrHook->StartHaptics(g_config->leftHandedMode ? 1 : 2, 0.5, 0.4f);
	}

}
