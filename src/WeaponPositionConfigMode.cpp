#include "WeaponPositionConfigMode.h"

#include "Config.h"
#include "F4VRBody.h"
#include "Skeleton.h"
#include "utils.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "f4vr/VRControllersManager.h"
#include "ui/UIButton.h"
#include "ui/UIContainer.h"
#include "ui/UIManager.h"
#include "ui/UIToggleButton.h"
#include "ui/UIToggleGroupContainer.h"

using namespace vrui;
using namespace common;

namespace frik {
	/**
	 * On release, we need to remove the UI from global manager.
	 */
	WeaponPositionConfigMode::~WeaponPositionConfigMode() {
		if (_configUI) {
			// remove the UI
			g_uiManager->detachElement(_configUI, true);
		}
	}

	/**
	 * Get default melee weapon adjustment to match how a human holds it as game default is straight forward.
	 */
	NiTransform WeaponPositionConfigMode::getMeleeWeaponDefaultAdjustment(const NiTransform& originalTransform) {
		Matrix44 rot;
		NiTransform transform;
		transform.scale = originalTransform.scale;
		if (g_config.leftHandedMode) {
			transform.pos = NiPoint3(5.5f, -2.2f, 1);
			rot.setEulerAngles(degreesToRads(95), degreesToRads(60), 0);
		} else {
			transform.pos = NiPoint3(4.5f, -2.2f, -1);
			rot.setEulerAngles(degreesToRads(85), degreesToRads(-65), 0);
		}
		transform.rot = rot.multiply43Right(originalTransform.rot);
		return transform;
	}

	/**
	 * Get default melee weapon adjustment to match how a human holds it as game default is straight forward.
	 */
	NiTransform WeaponPositionConfigMode::getThrowableWeaponDefaultAdjustment(const NiTransform& originalTransform, const bool inPA) {
		NiTransform transform;
		transform.scale = originalTransform.scale;
		transform.rot = originalTransform.rot;
		transform.pos = g_config.leftHandedMode
			? (inPA ? NiPoint3(-2.5f, 7.5f, -1) : NiPoint3(-3, 4, 0))
			: inPA
			? NiPoint3(-0.5f, 6, 2)
			: NiPoint3(-2, 3, 1);
		return transform;
	}

	/**
	 * Get default back of the hand UI (HP,Ammo,etc.) default adjustment for empty hand and use as base to adjust for weapon.
	 */
	NiTransform WeaponPositionConfigMode::getBackOfHandUIDefaultAdjustment(const NiTransform& originalTransform, const bool inPA) {
		NiTransform transform;
		transform.scale = originalTransform.scale;
		if (g_config.leftHandedMode) {
			Matrix44 mat;
			mat.setEulerAngles(degreesToRads(180), 0, degreesToRads(180));
			transform.rot = mat.make43();
			transform.pos = inPA ? NiPoint3(5, 6.5f, -11) : NiPoint3(6.2f, 4.8f, -12.2f);
		} else {
			transform.rot = Matrix44::getIdentity43();
			transform.pos = inPA ? NiPoint3(-5.8f, 5.8f, 1.8f) : NiPoint3(-6.2f, 3.6f, 0.8f);
		}
		return transform;
	}

	/**
	 * Handle configuration UI interaction.
	 */
	void WeaponPositionConfigMode::onFrameUpdate(NiNode* weapon) {
		if (g_configurationMode->isCalibrateModeActive() || g_configurationMode->isPipBoyConfigModeActive()) {
			// don't show this config UI if main config UI is shown
			_configUI->setVisibility(false);
			return;
		}
		_configUI->setVisibility(true);

		const auto throwable = _adjuster->_skelly->getThrowableWeaponNode();
		if (throwable) {
			_repositionTarget = RepositionTarget::Throwable;
			_throwableUIButton->setToggleState(true);
		}

		const bool weaponEquipped = weapon != nullptr;
		showHideUIElements(weaponEquipped, throwable != nullptr);

		if (!weaponEquipped && _repositionTarget != RepositionTarget::Throwable && _repositionTarget != RepositionTarget::BackOfHandUI) {
			// only back of hand UI can be repositioned without weapon equipped
			return;
		}

		// reposition
		handleReposition(weapon, throwable);

		// reset
		if (f4vr::VRControllers.isLongPressed(vr::k_EButton_Grip, f4vr::Hand::Primary) && _repositionTarget != RepositionTarget::Throwable) {
			f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, .6f, .5f);
			resetConfig();
		}
		// save
		if (f4vr::VRControllers.isLongPressed(vr::EVRButtonId::k_EButton_A, f4vr::Hand::Primary)) {
			f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, .6f, .5f);
			saveConfig();
		}
	}

	/**
	 * Show hide the different config UI elements based on the current state of equipped weapon, throwable, and reposition target.
	 * Complex number of states to help guide the player through the configuration process.
	 */
	void WeaponPositionConfigMode::showHideUIElements(const bool weaponEquipped, const bool throwableEquipped) const {
		// Show/Hide UI that should be visible only when weapon is equipped
		_weaponModeButton->setVisibility(weaponEquipped && !throwableEquipped);
		_offhandModeButton->setVisibility(weaponEquipped && !throwableEquipped);
		if (_betterScopesModeButton) {
			_betterScopesModeButton->setVisibility(weaponEquipped && !throwableEquipped);
		}
		_emptyHandsMessageBox->setVisibility(!weaponEquipped && !throwableEquipped);

		const bool showAnything = weaponEquipped || _repositionTarget == RepositionTarget::Throwable || _repositionTarget == RepositionTarget::BackOfHandUI;
		const bool showSaveReset = showAnything && (_repositionTarget != RepositionTarget::Throwable || throwableEquipped);
		_saveButton->setVisibility(showSaveReset);
		_resetButton->setVisibility(showSaveReset);
		_throwableNotEquippedMessageBox->setVisibility(!showSaveReset && _repositionTarget == RepositionTarget::Throwable);

		// show the right footer
		const bool complexAdjust = _repositionTarget == RepositionTarget::Weapon || _repositionTarget == RepositionTarget::BackOfHandUI;
		_complexAdjustFooter->setVisibility(showSaveReset && complexAdjust);
		_throwableAdjustFooter->setVisibility(showSaveReset && throwableEquipped);
		_simpleAdjustFooter->setVisibility(showSaveReset && !complexAdjust && !throwableEquipped);
	}

	/**
	 * Handle reposition by user input of the target config.
	 */
	void WeaponPositionConfigMode::handleReposition(NiNode* weapon, NiNode* throwable) const {
		switch (_repositionTarget) {
		case RepositionTarget::Weapon:
			handleWeaponReposition(weapon);
			break;
		case RepositionTarget::Offhand:
			handleOffhandReposition();
			break;
		case RepositionTarget::Throwable:
			handleThrowableReposition(throwable);
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
	 * NOTE: because of minor tweaks on what axis are used for repositioning it doesn't make sense to create common code for it.
	 */
	void WeaponPositionConfigMode::handleWeaponReposition(NiNode* weapon) const {
		const auto [primAxisX, primAxisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary);
		const auto [secAxisX, secAxisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Offhand);
		if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
			return;
		}

		auto& transform = _adjuster->_weaponOffsetTransform;
		const float leftHandedMult = g_config.leftHandedMode ? -1.f : 1.f;

		// Update the weapon transform by player thumbstick and buttons input.
		// Depending on buttons pressed can horizontal/vertical position or rotation.
		if (f4vr::VRControllers.isPressHeldDown(vr::EVRButtonId::k_EButton_Grip, f4vr::Hand::Offhand)) {
			Matrix44 rot;
			// pitch and yaw rotation by primary stick, roll rotation by secondary stick
			rot.setEulerAngles(-degreesToRads(primAxisY / 5), -degreesToRads(secAxisX / 3), degreesToRads(primAxisX / 5));
			transform.rot = rot.multiply43Left(transform.rot);
		} else {
			// adjust horizontal (y - right/left, x - forward/backward) by primary stick
			transform.pos.y += leftHandedMult * primAxisX / 12;
			transform.pos.x += primAxisY / 12;
			// adjust vertical (z - up/down) by secondary stick
			transform.pos.z -= leftHandedMult * secAxisY / 12;
		}

		// update the weapon with the offset change
		weapon->m_localTransform = _adjuster->_weaponOffsetTransform;
	}

	/**
	 * In offhand reposition mode...
	 */
	void WeaponPositionConfigMode::handleOffhandReposition() const {
		// Update the offset position by player thumbstick.
		const auto [axisX, axisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary);
		if (axisX != 0.f || axisY != 0.f) {
			Matrix44 rot;
			rot.setEulerAngles(-degreesToRads(axisY / 5), 0, degreesToRads(axisX / 5));
			_adjuster->_offhandOffsetRot = rot.multiply43Left(_adjuster->_offhandOffsetRot);
		}
	}

	/**
	 * In throwable reposition mode...
	 * NOTE: because of minor tweaks on what axis are used for repositioning it doesn't make sense to create common code for it.
	 */
	void WeaponPositionConfigMode::handleThrowableReposition(NiNode* throwable) const {
		if (!throwable) {
			return;
		}

		const auto [primAxisX, primAxisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary);
		const auto [secAxisX, secAxisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Offhand);
		if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
			return;
		}

		auto& transform = _adjuster->_throwableWeaponOffsetTransform;
		const float leftHandedMult = g_config.leftHandedMode ? -1.f : 1.f;

		// Update the transform by player thumbstick and buttons input.
		// Depending on buttons pressed can horizontal/vertical position or rotation.
		if (f4vr::VRControllers.isPressHeldDown(vr::EVRButtonId::k_EButton_Grip, f4vr::Hand::Offhand)) {
			Matrix44 rot;
			// pitch and yaw rotation by primary stick, roll rotation by secondary stick
			rot.setEulerAngles(degreesToRads(secAxisY / 6), degreesToRads(secAxisX / 6), degreesToRads(primAxisX / 6));
			transform.rot = rot.multiply43Left(transform.rot);
		} else {
			// adjust horizontal (x - right/left, z - forward/backward) by primary stick
			transform.pos.z += -leftHandedMult * primAxisX / 14 - leftHandedMult * secAxisX / 14;
			transform.pos.x += -primAxisY / 14;
			// adjust vertical (y - up/down) by secondary stick
			transform.pos.y += leftHandedMult * secAxisY / 14;
		}

		throwable->m_localTransform = transform;
	}

	/**
	 * In back of hand UI reposition mode...
	 * NOTE: because of minor tweaks on what axis are used for repositioning it doesn't make sense to create common code for it.
	 */
	void WeaponPositionConfigMode::handleBackOfHandUIReposition() const {
		const auto [primAxisX, primAxisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary);
		const auto [secAxisX, secAxisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Offhand);
		if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
			return;
		}

		auto& transform = _adjuster->_backOfHandUIOffsetTransform;
		const float leftHandedMult = g_config.leftHandedMode ? -1.f : 1.f;

		// Update the transform by player thumbstick and buttons input.
		// Depending on buttons pressed can horizontal/vertical position or rotation.
		if (f4vr::VRControllers.isPressHeldDown(vr::EVRButtonId::k_EButton_Grip, f4vr::Hand::Offhand)) {
			Matrix44 rot;
			// pitch and yaw rotation by primary stick, roll rotation by secondary stick
			rot.setEulerAngles(-degreesToRads(secAxisY / 6), -degreesToRads(primAxisX / 6), -degreesToRads(primAxisY / 6));
			transform.rot = rot.multiply43Left(transform.rot);
		} else {
			// adjust horizontal (z - right/left, y - forward/backward) by primary stick
			transform.pos.z -= leftHandedMult * primAxisX / 14;
			transform.pos.y += primAxisY / 14;
			// adjust vertical (x - up/down) by secondary stick
			transform.pos.x += leftHandedMult * secAxisY / 14;
		}

		// update the weapon with the offset change
		_adjuster->getBackOfHandUINode()->m_localTransform = transform;
	}

	/**
	 * Handle configuration for BetterScopesVR mod.
	 */
	void WeaponPositionConfigMode::handleBetterScopesReposition() {
		const auto [axisX, axisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary);
		if (axisX != 0.f || axisY != 0.f) {
			// Axis_state y is up and down, which corresponds to reticule z axis
			NiPoint3 msgData(axisX / 10, 0.f, axisY / 10);
			g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
		}
	}

	void WeaponPositionConfigMode::resetConfig() const {
		Log::info("Reset Reposition Config for target: %d, Weapon: %s", _repositionTarget, _adjuster->_currentWeapon.c_str());
		switch (_repositionTarget) {
		case RepositionTarget::Weapon:
			resetWeaponConfig();
			break;
		case RepositionTarget::Offhand:
			resetOffhandConfig();
			break;
		case RepositionTarget::Throwable:
			resetThrowableConfig();
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
		Log::info("Save Reposition Config for target: %d, Weapon: %s", _repositionTarget, _adjuster->_currentWeapon.c_str());
		switch (_repositionTarget) {
		case RepositionTarget::Weapon:
			saveWeaponConfig();
			break;
		case RepositionTarget::Offhand:
			saveOffhandConfig();
			break;
		case RepositionTarget::Throwable:
			saveThrowableConfig();
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
		f4vr::showNotification("Reset Weapon Position to Default");
		_adjuster->_weaponOffsetTransform = f4vr::isMeleeWeaponEquipped()
			? getMeleeWeaponDefaultAdjustment(_adjuster->_weaponOriginalTransform)
			: _adjuster->_weaponOriginalTransform;
		g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::Weapon, _adjuster->_currentlyInPA, true);
	}

	void WeaponPositionConfigMode::saveWeaponConfig() const {
		f4vr::showNotification("Saving Weapon Position");
		g_config.saveWeaponOffsets(_adjuster->_currentWeapon, _adjuster->_weaponOffsetTransform, WeaponOffsetsMode::Weapon, _adjuster->_currentlyInPA);
	}

	void WeaponPositionConfigMode::resetOffhandConfig() const {
		f4vr::showNotification("Reset Offhand Position to Default");
		_adjuster->_offhandOffsetRot = Matrix44::getIdentity43();
		g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::OffHand, _adjuster->_currentlyInPA, true);
	}

	void WeaponPositionConfigMode::saveOffhandConfig() const {
		f4vr::showNotification("Saving Offhand Position");
		NiTransform transform;
		transform.scale = 1;
		transform.pos = NiPoint3(0, 0, 0);
		transform.rot = _adjuster->_offhandOffsetRot;
		g_config.saveWeaponOffsets(_adjuster->_currentWeapon, transform, WeaponOffsetsMode::OffHand, _adjuster->_currentlyInPA);
	}

	void WeaponPositionConfigMode::resetThrowableConfig() const {
		f4vr::showNotification("Reset Throwable Weapon Position to Default");
		_adjuster->_throwableWeaponOffsetTransform = _adjuster->_throwableWeaponOriginalTransform;
		g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::Throwable, _adjuster->_currentlyInPA, true);
	}

	void WeaponPositionConfigMode::saveThrowableConfig() const {
		f4vr::showNotification("Saving Throwable Weapon Position");
		g_config.saveWeaponOffsets(_adjuster->_currentThrowableWeaponName, _adjuster->_throwableWeaponOffsetTransform, WeaponOffsetsMode::Throwable, _adjuster->_currentlyInPA);
	}

	void WeaponPositionConfigMode::resetBackOfHandUIConfig() const {
		f4vr::showNotification("Reset Back of Hand UI Position to Default");
		_adjuster->_backOfHandUIOffsetTransform = getBackOfHandUIDefaultAdjustment(_adjuster->_backOfHandUIOffsetTransform, _adjuster->_currentlyInPA);
		_adjuster->getBackOfHandUINode()->m_localTransform = _adjuster->_backOfHandUIOffsetTransform;
		g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::BackOfHandUI, _adjuster->_currentlyInPA, true);
	}

	void WeaponPositionConfigMode::saveBackOfHandUIConfig() const {
		f4vr::showNotification("Saving Back of Hand UI Position");
		g_config.saveWeaponOffsets(_adjuster->_currentWeapon, _adjuster->_backOfHandUIOffsetTransform, WeaponOffsetsMode::BackOfHandUI, _adjuster->_currentlyInPA);
	}

	void WeaponPositionConfigMode::resetBetterScopesConfig() {
		f4vr::showNotification("Reset BetterScopesVR Scope Offset to Default");
		NiPoint3 msgData(0.f, 0.f, 0.f);
		g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
	}

	void WeaponPositionConfigMode::saveBetterScopesConfig() {
		f4vr::showNotification("Saving BetterScopesVR Scopes Offset");
		NiPoint3 msgData(0.f, 1, 0.f);
		g_messaging->Dispatch(g_pluginHandle, 17, &msgData, sizeof(NiPoint3*), "FO4VRBETTERSCOPES");
	}

	/**
	 * Create the configuration UI
	 */
	void WeaponPositionConfigMode::createConfigUI() {
		_weaponModeButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_weapon.nif");
		_weaponModeButton->setToggleState(true);
		_weaponModeButton->setOnToggleHandler([this](UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::Weapon; });

		_offhandModeButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_offhand.nif");
		_offhandModeButton->setOnToggleHandler([this](UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::Offhand; });

		_throwableUIButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_throwable.nif");
		_throwableUIButton->setOnToggleHandler([this](UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::Throwable; });

		const auto backOfHandUIButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_back_of_hand_ui.nif");
		backOfHandUIButton->setOnToggleHandler([this](UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::BackOfHandUI; });

		const auto firstRowContainerInner = std::make_shared<UIToggleGroupContainer>(UIContainerLayout::HorizontalCenter, 0.15f);
		firstRowContainerInner->addElement(_weaponModeButton);
		firstRowContainerInner->addElement(_offhandModeButton);
		firstRowContainerInner->addElement(_throwableUIButton);
		firstRowContainerInner->addElement(backOfHandUIButton);

		if (isBetterScopesVRModLoaded()) {
			_betterScopesModeButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_better_scopes_vr.nif");
			_betterScopesModeButton->setOnToggleHandler([this](UIWidget* widget, bool state) { _repositionTarget = RepositionTarget::BetterScopes; });
			firstRowContainerInner->addElement(_betterScopesModeButton);
		}

		_emptyHandsMessageBox = std::make_shared<UIWidget>("FRIK/ui_weapconf_empty_hands_box.nif");
		_emptyHandsMessageBox->setSize(3.6f, 2.2f);

		const auto firstRowContainer = std::make_shared<UIContainer>(UIContainerLayout::HorizontalCenter, 0.15f);
		firstRowContainer->addElement(_emptyHandsMessageBox);
		firstRowContainer->addElement(firstRowContainerInner);

		_saveButton = std::make_shared<UIButton>("FRIK/ui_common_btn_save.nif");
		_saveButton->setOnPressHandler([this](UIWidget* widget) { saveConfig(); });

		_resetButton = std::make_shared<UIButton>("FRIK/ui_common_btn_reset.nif");
		_resetButton->setOnPressHandler([this](UIWidget* widget) { resetConfig(); });

		const auto exitButton = std::make_shared<UIButton>("FRIK/ui_common_btn_exit.nif");
		exitButton->setOnPressHandler([this](UIWidget* widget) { _adjuster->toggleWeaponRepositionMode(); });

		_throwableNotEquippedMessageBox = std::make_shared<UIWidget>("FRIK/ui_weapconf_throwable_box.nif");
		_throwableNotEquippedMessageBox->setSize(3.6f, 2.2f);

		const auto secondRowContainer = std::make_shared<UIContainer>(UIContainerLayout::HorizontalCenter, 0.2f);
		secondRowContainer->addElement(_saveButton);
		secondRowContainer->addElement(_resetButton);
		secondRowContainer->addElement(_throwableNotEquippedMessageBox);
		secondRowContainer->addElement(exitButton);

		const auto header = std::make_shared<UIWidget>("FRIK/ui_weapconf_header.nif", 0.7f);
		header->setSize(8, 1);
		_complexAdjustFooter = std::make_shared<UIWidget>("FRIK/ui_weapconf_footer.nif", 0.7f);
		_complexAdjustFooter->setSize(7, 2.2f);
		_throwableAdjustFooter = std::make_shared<UIWidget>("FRIK/ui_weapconf_footer_throwable.nif", 0.7f);
		_throwableAdjustFooter->setSize(7, 2.2f);
		_simpleAdjustFooter = std::make_shared<UIWidget>("FRIK/ui_weapconf_footer_2.nif", 0.7f);
		_simpleAdjustFooter->setSize(5, 2.2f);
		_simpleAdjustFooter->setVisibility(false);

		const auto mainContainer = std::make_shared<UIContainer>(UIContainerLayout::VerticalCenter, 0.2f);
		mainContainer->addElement(firstRowContainer);
		mainContainer->addElement(secondRowContainer);
		mainContainer->addElement(_complexAdjustFooter);
		mainContainer->addElement(_throwableAdjustFooter);
		mainContainer->addElement(_simpleAdjustFooter);

		_configUI = std::make_shared<UIContainer>(UIContainerLayout::VerticalDown, 0.3f, 1.7f);
		_configUI->addElement(header);
		_configUI->addElement(mainContainer);

		// start hidden by default (will be set visible in frame update if it should be)
		_configUI->setVisibility(false);

		g_uiManager->attachPresetToPrimaryWandLeft(_configUI, g_config.leftHandedMode, {0, -4, 0});
	}
}
