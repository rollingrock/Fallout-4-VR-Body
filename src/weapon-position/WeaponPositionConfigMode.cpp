#include "WeaponPositionConfigMode.h"

#include "Config.h"
#include "FRIK.h"
#include "utils.h"
#include "common/MatrixUtils.h"
#include "vrcf/VRControllersManager.h"
#include "skeleton/Skeleton.h"
#include "vrui/UIButton.h"
#include "vrui/UIContainer.h"
#include "vrui/UIManager.h"
#include "vrui/UIToggleButton.h"
#include "vrui/UIToggleGroupContainer.h"

using namespace vrui;
using namespace common;

namespace frik
{
    /**
     * On release, we need to remove the UI from global manager.
     */
    WeaponPositionConfigMode::~WeaponPositionConfigMode()
    {
        if (_configUI) {
            // remove the UI
            g_uiManager->detachElement(_configUI, true);
        }
    }

    /**
     * Get default melee weapon adjustment to match how a human holds it as game default is straight forward.
     */
    RE::NiTransform WeaponPositionConfigMode::getMeleeWeaponDefaultAdjustment(const RE::NiTransform& originalTransform)
    {
        RE::NiMatrix3 rot;
        RE::NiTransform transform;
        transform.scale = originalTransform.scale;
        if (f4vr::isLeftHandedMode()) {
            transform.translate = RE::NiPoint3(5.5f, -2.2f, 1);
            rot = getMatrixFromEulerAngles(degreesToRads(95), degreesToRads(60), 0);
        } else {
            transform.translate = RE::NiPoint3(4.5f, -2.2f, -1);
            rot = getMatrixFromEulerAngles(degreesToRads(85), degreesToRads(-65), 0);
        }
        transform.rotate = originalTransform.rotate * rot;
        return transform;
    }

    /**
     * Get default melee weapon adjustment to match how a human holds it as game default is straight forward.
     */
    RE::NiTransform WeaponPositionConfigMode::getThrowableWeaponDefaultAdjustment(const RE::NiTransform& originalTransform, const bool inPA)
    {
        RE::NiTransform transform;
        transform.scale = originalTransform.scale;
        transform.rotate = originalTransform.rotate;
        transform.translate = f4vr::isLeftHandedMode()
            ? (inPA ? RE::NiPoint3(-2.5f, 7.5f, -1) : RE::NiPoint3(-3, 4, 0))
            : inPA
            ? RE::NiPoint3(-0.5f, 6, 2)
            : RE::NiPoint3(-2, 3, 1);
        return transform;
    }

    /**
     * Get default back of the hand UI (HP,Ammo,etc.) default adjustment for empty hand and use as base to adjust for weapon.
     */
    RE::NiTransform WeaponPositionConfigMode::getBackOfHandUIDefaultAdjustment(const RE::NiTransform& originalTransform, const bool inPA)
    {
        RE::NiTransform transform;
        transform.scale = originalTransform.scale;
        if (f4vr::isLeftHandedMode()) {
            transform.rotate = getMatrixFromEulerAngles(degreesToRads(180), 0, degreesToRads(180));
            transform.translate = inPA ? RE::NiPoint3(5, 6.5f, -11) : RE::NiPoint3(6.2f, 4.8f, -12.2f);
        } else {
            transform.rotate = getIdentityMatrix();
            transform.translate = inPA ? RE::NiPoint3(-5.8f, 5.8f, 1.8f) : RE::NiPoint3(-6.2f, 3.6f, 0.8f);
        }
        return transform;
    }

    /**
     * Handle configuration UI interaction.
     */
    void WeaponPositionConfigMode::onFrameUpdate(RE::NiNode* weapon)
    {
        if (g_frik.isMainConfigurationModeActive() || g_frik.isPipboyConfigurationModeActive()) {
            // don't show this config UI if main config UI is shown
            _configUI->setVisibility(false);
            return;
        }
        _configUI->setVisibility(true);

        const auto throwable = f4vr::getThrowableWeaponNode();
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

        // save
        if (vrcf::VRControllers.isLongPressed(vrcf::Hand::Primary, vr::EVRButtonId::k_EButton_A)) {
            vrcf::VRControllers.triggerHaptic(vrcf::Hand::Primary, .6f, .5f);
            saveConfig();
        }
    }

    /**
     * Show hide the different config UI elements based on the current state of equipped weapon, throwable, and reposition target.
     * Complex number of states to help guide the player through the configuration process.
     */
    void WeaponPositionConfigMode::showHideUIElements(const bool weaponEquipped, const bool throwableEquipped) const
    {
        // Show/Hide UI that should be visible only when weapon is equipped
        _weaponModeButton->setVisibility(weaponEquipped && !throwableEquipped);
        _primaryHandModeButton->setVisibility(weaponEquipped && !throwableEquipped);
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
    void WeaponPositionConfigMode::handleReposition(RE::NiNode* weapon, RE::NiNode* throwable) const
    {
        switch (_repositionTarget) {
        case RepositionTarget::Weapon:
            handleWeaponReposition(weapon);
            break;
        case RepositionTarget::PrimaryHand:
            handlePrimaryHandReposition();
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
    void WeaponPositionConfigMode::handleWeaponReposition(RE::NiNode* weapon) const
    {
        const auto [primAxisX, primAxisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary);
        const auto [secAxisX, secAxisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Offhand);
        if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
            return;
        }

        auto& transform = _adjuster->_weaponOffsetTransform;
        const float leftHandedMult = f4vr::isLeftHandedMode() ? -1.f : 1.f;

        // Update the weapon transform by player thumbstick and buttons input.
        // Depending on buttons pressed can horizontal/vertical position or rotation.

        if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_A)) {
            // adjust the scale of the weapon
            transform.scale = std::fmax(0.1f, transform.scale + correctAdjustmentValue(primAxisY, 100));
        } else if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_Grip)) {
            // pitch and yaw rotation by primary stick, roll rotation by secondary stick
            const auto rot = getMatrixFromEulerAngles(
                -degreesToRads(correctAdjustmentValue(primAxisY, 5)),
                -degreesToRads(correctAdjustmentValue(secAxisX, 3)),
                degreesToRads(correctAdjustmentValue(primAxisX, 5)));
            transform.rotate = rot * transform.rotate;
        } else {
            // adjust horizontal (y - right/left, x - forward/backward) by primary stick
            transform.translate.y += correctAdjustmentValue(leftHandedMult * primAxisX, 12);
            transform.translate.x += correctAdjustmentValue(primAxisY, 12);
            // adjust vertical (z - up/down) by secondary stick
            transform.translate.z -= correctAdjustmentValue(leftHandedMult * secAxisY, 12);
        }

        // update the weapon with the offset change
        weapon->local = _adjuster->_weaponOffsetTransform;
    }

    /**
     * In primary hand reposition mode...
     */
    void WeaponPositionConfigMode::handlePrimaryHandReposition() const
    {
        // Update the offset position by player thumbstick.
        const auto [axisX, axisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary);
        if (axisX != 0.f || axisY != 0.f) {
            const auto rot = vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_Grip)
                ? getMatrixFromEulerAngles(-degreesToRads(correctAdjustmentValue(axisY, 2)), 0, 0)
                : getMatrixFromEulerAngles(0, -degreesToRads(correctAdjustmentValue(axisY, 2)), -degreesToRads(correctAdjustmentValue(axisX, 3)));
            _adjuster->_primaryHandOffsetRot = rot * _adjuster->_primaryHandOffsetRot;
            _adjuster->_hasPrimaryHandOffset = true;
        }
    }

    /**
     * In offhand reposition mode...
     */
    void WeaponPositionConfigMode::handleOffhandReposition() const
    {
        // Update the offset position by player thumbstick.
        const auto [axisX, axisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary);
        if (axisX != 0.f || axisY != 0.f) {
            const auto rot = getMatrixFromEulerAngles(-degreesToRads(correctAdjustmentValue(axisY, 5)), 0, degreesToRads(correctAdjustmentValue(axisX, 5)));
            _adjuster->_offhandOffsetRot = rot * _adjuster->_offhandOffsetRot;
        }
    }

    /**
     * In throwable reposition mode...
     * NOTE: because of minor tweaks on what axis are used for repositioning it doesn't make sense to create common code for it.
     */
    void WeaponPositionConfigMode::handleThrowableReposition(RE::NiNode* throwable) const
    {
        if (!throwable) {
            return;
        }

        const auto [primAxisX, primAxisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary);
        const auto [secAxisX, secAxisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Offhand);
        if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
            return;
        }

        auto& transform = _adjuster->_throwableWeaponOffsetTransform;
        const float leftHandedMult = f4vr::isLeftHandedMode() ? -1.f : 1.f;

        // Update the transform by player thumbstick and buttons input.
        // Depending on buttons pressed can horizontal/vertical position or rotation.
        if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_A)) {
            // adjust the scale of the weapon
            transform.scale = std::fmax(0.1f, transform.scale + correctAdjustmentValue(primAxisY, 100));
        } else if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_Grip)) {
            // pitch and yaw rotation by primary stick, roll rotation by secondary stick
            const auto rot = getMatrixFromEulerAngles(
                degreesToRads(correctAdjustmentValue(secAxisY, 6)),
                degreesToRads(correctAdjustmentValue(secAxisX, 6)),
                degreesToRads(correctAdjustmentValue(primAxisX, 6)));
            transform.rotate = rot * transform.rotate;
        } else {
            // adjust horizontal (x - right/left, z - forward/backward) by primary stick
            transform.translate.z += correctAdjustmentValue(-leftHandedMult * primAxisX, 14) - correctAdjustmentValue(leftHandedMult * secAxisX, 14);
            transform.translate.x += correctAdjustmentValue(-primAxisY, 14);
            // adjust vertical (y - up/down) by secondary stick
            transform.translate.y += correctAdjustmentValue(leftHandedMult * secAxisY, 14);
        }

        throwable->local = transform;
    }

    /**
     * In back of hand UI reposition mode...
     * NOTE: because of minor tweaks on what axis are used for repositioning it doesn't make sense to create common code for it.
     */
    void WeaponPositionConfigMode::handleBackOfHandUIReposition() const
    {
        const auto [primAxisX, primAxisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary);
        const auto [secAxisX, secAxisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Offhand);
        if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
            return;
        }

        auto& transform = _adjuster->_backOfHandUIOffsetTransform;
        const float leftHandedMult = f4vr::isLeftHandedMode() ? -1.f : 1.f;

        // Update the transform by player thumbstick and buttons input.
        // Depending on buttons pressed can horizontal/vertical position or rotation.
        if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_A)) {
            // adjust the scale of the weapon
            transform.scale = std::fmax(0.1f, transform.scale + correctAdjustmentValue(primAxisY, 100));
        } else if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_Grip)) {
            // pitch and yaw rotation by primary stick, roll rotation by secondary stick
            const auto rot = getMatrixFromEulerAngles(
                -degreesToRads(correctAdjustmentValue(secAxisY, 6)),
                -degreesToRads(correctAdjustmentValue(primAxisX, 6)),
                -degreesToRads(correctAdjustmentValue(primAxisY, 6)));
            transform.rotate = rot * transform.rotate;
        } else {
            // adjust horizontal (z - right/left, y - forward/backward) by primary stick
            transform.translate.z -= correctAdjustmentValue(leftHandedMult * primAxisX, 14);
            transform.translate.y += correctAdjustmentValue(primAxisY, 14);
            // adjust vertical (x - up/down) by secondary stick
            transform.translate.x += correctAdjustmentValue(leftHandedMult * secAxisY, 14);
        }

        // update the weapon with the offset change
        WeaponPositionAdjuster::getBackOfHandUINode()->local = transform;
    }

    /**
     * Handle configuration for BetterScopesVR mod.
     */
    void WeaponPositionConfigMode::handleBetterScopesReposition()
    {
        const auto [axisX, axisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary);
        if (axisX != 0.f || axisY != 0.f) {
            // Axis_state y is up and down, which corresponds to reticule z axis
            RE::NiPoint3 msgData(axisX / 10, 0.f, axisY / 10);
            g_frik.dispatchMessageToBetterScopesVR(17, &msgData, sizeof(RE::NiPoint3*));
        }
    }

    void WeaponPositionConfigMode::resetConfig() const
    {
        logger::info("Reset Reposition Config for target: {}, Weapon: {}", static_cast<int>(_repositionTarget), _adjuster->_currentWeapon.c_str());
        switch (_repositionTarget) {
        case RepositionTarget::Weapon:
            resetWeaponConfig();
            break;
        case RepositionTarget::PrimaryHand:
            resetPrimaryHandConfig();
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

    void WeaponPositionConfigMode::saveConfig() const
    {
        logger::info("Save Reposition Config for target: {}, Weapon: {}", static_cast<int>(_repositionTarget), _adjuster->_currentWeapon.c_str());
        switch (_repositionTarget) {
        case RepositionTarget::Weapon:
            saveWeaponConfig();
            break;
        case RepositionTarget::PrimaryHand:
            savePrimaryHandConfig();
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

    void WeaponPositionConfigMode::resetWeaponConfig() const
    {
        f4vr::showNotification("Reset Weapon Position to Default");
        _adjuster->_weaponOffsetTransform = f4vr::isMeleeWeaponEquipped()
            ? getMeleeWeaponDefaultAdjustment(_adjuster->_weaponOriginalTransform)
            : _adjuster->_weaponOriginalTransform;
        g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::Weapon, _adjuster->_currentlyInPA, true);
    }

    void WeaponPositionConfigMode::saveWeaponConfig() const
    {
        f4vr::showNotification("Saving Weapon Position");
        g_config.saveWeaponOffsets(_adjuster->_currentWeapon, _adjuster->_weaponOffsetTransform, WeaponOffsetsMode::Weapon, _adjuster->_currentlyInPA);
    }

    void WeaponPositionConfigMode::resetPrimaryHandConfig() const
    {
        f4vr::showNotification("Reset Primary Hand Position to Default");
        _adjuster->_primaryHandOffsetRot = getIdentityMatrix();
        g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::PrimaryHand, _adjuster->_currentlyInPA, true);
    }

    void WeaponPositionConfigMode::savePrimaryHandConfig() const
    {
        f4vr::showNotification("Saving Primary Hand Position");
        RE::NiTransform transform;
        transform.scale = 1;
        transform.translate = RE::NiPoint3(0, 0, 0);
        transform.rotate = _adjuster->_primaryHandOffsetRot;
        g_config.saveWeaponOffsets(_adjuster->_currentWeapon, transform, WeaponOffsetsMode::PrimaryHand, _adjuster->_currentlyInPA);
    }

    void WeaponPositionConfigMode::resetOffhandConfig() const
    {
        f4vr::showNotification("Reset Offhand Position to Default");
        _adjuster->_offhandOffsetRot = getIdentityMatrix();
        g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::OffHand, _adjuster->_currentlyInPA, true);
    }

    void WeaponPositionConfigMode::saveOffhandConfig() const
    {
        f4vr::showNotification("Saving Offhand Position");
        RE::NiTransform transform;
        transform.scale = 1;
        transform.translate = RE::NiPoint3(0, 0, 0);
        transform.rotate = _adjuster->_offhandOffsetRot;
        g_config.saveWeaponOffsets(_adjuster->_currentWeapon, transform, WeaponOffsetsMode::OffHand, _adjuster->_currentlyInPA);
    }

    void WeaponPositionConfigMode::resetThrowableConfig() const
    {
        f4vr::showNotification("Reset Throwable Weapon Position to Default");
        _adjuster->_throwableWeaponOffsetTransform = _adjuster->_throwableWeaponOriginalTransform;
        g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::Throwable, _adjuster->_currentlyInPA, true);
    }

    void WeaponPositionConfigMode::saveThrowableConfig() const
    {
        f4vr::showNotification("Saving Throwable Weapon Position");
        g_config.saveWeaponOffsets(_adjuster->_currentThrowableWeaponName, _adjuster->_throwableWeaponOffsetTransform, WeaponOffsetsMode::Throwable, _adjuster->_currentlyInPA);
    }

    void WeaponPositionConfigMode::resetBackOfHandUIConfig() const
    {
        f4vr::showNotification("Reset Back of Hand UI Position to Default");
        _adjuster->_backOfHandUIOffsetTransform = getBackOfHandUIDefaultAdjustment(_adjuster->_backOfHandUIOffsetTransform, _adjuster->_currentlyInPA);
        WeaponPositionAdjuster::getBackOfHandUINode()->local = _adjuster->_backOfHandUIOffsetTransform;
        g_config.removeWeaponOffsets(_adjuster->_currentWeapon, WeaponOffsetsMode::BackOfHandUI, _adjuster->_currentlyInPA, true);
    }

    void WeaponPositionConfigMode::saveBackOfHandUIConfig() const
    {
        f4vr::showNotification("Saving Back of Hand UI Position");
        g_config.saveWeaponOffsets(_adjuster->_currentWeapon, _adjuster->_backOfHandUIOffsetTransform, WeaponOffsetsMode::BackOfHandUI, _adjuster->_currentlyInPA);
    }

    void WeaponPositionConfigMode::resetBetterScopesConfig()
    {
        f4vr::showNotification("Reset BetterScopesVR Scope Offset to Default");
        RE::NiPoint3 msgData(0.f, 0.f, 0.f);
        g_frik.dispatchMessageToBetterScopesVR(17, &msgData, sizeof(RE::NiPoint3*));
    }

    void WeaponPositionConfigMode::saveBetterScopesConfig()
    {
        f4vr::showNotification("Saving BetterScopesVR Scopes Offset");
        RE::NiPoint3 msgData(0.f, 1, 0.f);
        g_frik.dispatchMessageToBetterScopesVR(17, &msgData, sizeof(RE::NiPoint3*));
    }

    /**
     * Create the configuration UI
     */
    void WeaponPositionConfigMode::createConfigUI()
    {
        _weaponModeButton = std::make_shared<UIToggleButton>("FRIK\\UI_Weapon_Config\\btn_weapon.nif");
        _weaponModeButton->setToggleState(true);
        _weaponModeButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::Weapon; });

        _primaryHandModeButton = std::make_shared<UIToggleButton>("FRIK\\UI_Weapon_Config\\btn_primary_hand.nif");
        _primaryHandModeButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::PrimaryHand; });

        _offhandModeButton = std::make_shared<UIToggleButton>("FRIK\\UI_Weapon_Config\\btn_offhand.nif");
        _offhandModeButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::Offhand; });

        _throwableUIButton = std::make_shared<UIToggleButton>("FRIK\\UI_Weapon_Config\\btn_throwable.nif");
        _throwableUIButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::Throwable; });

        const auto backOfHandUIButton = std::make_shared<UIToggleButton>("FRIK\\UI_Weapon_Config\\btn_back_of_hand_ui.nif");
        backOfHandUIButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::BackOfHandUI; });

        const auto firstRowContainerInner = std::make_shared<UIToggleGroupContainer>("Row1Inner", UIContainerLayout::HorizontalCenter, 0.3f);
        firstRowContainerInner->addElement(_weaponModeButton);
        firstRowContainerInner->addElement(_primaryHandModeButton);
        firstRowContainerInner->addElement(_offhandModeButton);
        firstRowContainerInner->addElement(_throwableUIButton);
        firstRowContainerInner->addElement(backOfHandUIButton);

        if (isBetterScopesVRModLoaded()) {
            _betterScopesModeButton = std::make_shared<UIToggleButton>("FRIK\\UI_Weapon_Config\\btn_better_scopes_vr.nif");
            _betterScopesModeButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::BetterScopes; });
            firstRowContainerInner->addElement(_betterScopesModeButton);
        }

        _emptyHandsMessageBox = std::make_shared<UIWidget>("FRIK\\UI_Weapon_Config\\msg_empty_hands.nif");

        const auto firstRowContainer = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.3f);
        firstRowContainer->addElement(_emptyHandsMessageBox);
        firstRowContainer->addElement(firstRowContainerInner);

        _saveButton = std::make_shared<UIButton>("FRIK\\UI_Common\\btn_save.nif");
        _saveButton->setOnPressHandler([this](UIWidget*) { saveConfig(); });

        _resetButton = std::make_shared<UIButton>("FRIK\\UI_Common\\btn_reset.nif");
        _resetButton->setOnPressHandler([this](UIWidget*) { resetConfig(); });

        const auto exitButton = std::make_shared<UIButton>("FRIK\\UI_Common\\btn_exit.nif");
        exitButton->setOnPressHandler([this](UIWidget*) { _adjuster->toggleWeaponRepositionMode(); });

        _throwableNotEquippedMessageBox = std::make_shared<UIWidget>("FRIK\\UI_Weapon_Config\\msg_throwable_empty_hands.nif");

        const auto secondRowContainer = std::make_shared<UIContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);
        secondRowContainer->addElement(_saveButton);
        secondRowContainer->addElement(_resetButton);
        secondRowContainer->addElement(_throwableNotEquippedMessageBox);
        secondRowContainer->addElement(exitButton);

        const auto header = std::make_shared<UIWidget>("FRIK\\UI_Weapon_Config\\title.nif", 0.5f);
        _complexAdjustFooter = std::make_shared<UIWidget>("FRIK\\UI_Weapon_Config\\msg_footer.nif", 0.7f);
        _throwableAdjustFooter = std::make_shared<UIWidget>("FRIK\\UI_Weapon_Config\\msg_footer_throwable.nif", 0.7f);
        _simpleAdjustFooter = std::make_shared<UIWidget>("FRIK\\UI_Weapon_Config\\msg_footer_simple.nif", 0.7f);
        _simpleAdjustFooter->setVisibility(false);

        const auto mainContainer = std::make_shared<UIContainer>("Main", UIContainerLayout::VerticalCenter, 0.3f);
        mainContainer->addElement(firstRowContainer);
        mainContainer->addElement(secondRowContainer);
        mainContainer->addElement(_complexAdjustFooter);
        mainContainer->addElement(_throwableAdjustFooter);
        mainContainer->addElement(_simpleAdjustFooter);

        _configUI = std::make_shared<UIContainer>("WeaponConfig", UIContainerLayout::VerticalDown, 0.4f, 1.5f);
        _configUI->addElement(header);
        _configUI->addElement(mainContainer);

        // start hidden by default (will be set visible in frame update if it should be)
        _configUI->setVisibility(false);

        g_uiManager->attachPresetToPrimaryWandLeft(_configUI, { -16, 4, -5 });
    }
}
