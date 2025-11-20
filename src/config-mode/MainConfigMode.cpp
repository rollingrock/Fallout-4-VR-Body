#include "MainConfigMode.h"

#include "FRIK.h"
#include "ui/UIManager.h"
#include "ui/UIMultiStateToggleButton.h"
#include "ui/UIToggleGroupContainer.h"

using namespace vrui;
using namespace common;

namespace frik
{
    int MainConfigMode::isOpen() const
    {
        return _configUI != nullptr;
    }

    void MainConfigMode::openConfigMode()
    {
        logger::info("Open main config by call...");
        createMainConfigUI();
    }

    /**
     * Handle main config on every frame update.
     */
    void MainConfigMode::onFrameUpdate()
    {
        // open main config on both thumbsticks long-pressed shortcut
        if (!isOpen()
            && f4vr::VRControllers.isLongPressed(f4vr::Hand::Primary, vr::k_EButton_Axis0, 1.0f, false)
            && f4vr::VRControllers.isLongPressed(f4vr::Hand::Offhand, vr::k_EButton_Axis0, 1.0f, false)) {
            logger::info("Open main config by shortcut...");
            if (g_frik.isFavoritesMenuOpen()) {
                f4vr::closeFavoriteMenu();
            }
            f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, .6f, .5f);
            createMainConfigUI();
        }

        if (!isOpen()) {
            return;
        }

        // toggle selfie if Pipboy is not open as it uses the same button
        if (!g_frik.isPipboyOn() && f4vr::VRControllers.isReleasedShort(f4vr::Hand::Primary, vr::k_EButton_A)) {
            toggleSelfieMode();
        }

        // if body adjustment sub-config is open pass control to it
        if (isBodyAdjustOpen()) {
            _configUI->setVisibility(false);
            _bodyAdjustmentSubConfig->onFrameUpdate();
            return;
        }

        // hide main config if Pipboy or weapon adjustment config is open
        if (g_frik.isPipboyConfigurationModeActive() || g_frik.inWeaponRepositionMode()) {
            _configUI->setVisibility(false);
            return;
        }

        _configUI->setVisibility(true);
    }

    /**
     * Check if the body adjustment sub-configuration mode is open.
     */
    bool MainConfigMode::isBodyAdjustOpen() const
    {
        return _bodyAdjustmentSubConfig != nullptr;
    }

    /**
     * Create all the main config UI elements.
     */
    void MainConfigMode::createMainConfigUI()
    {
        const auto openBodyConfigBtn = std::make_shared<UIButton>("FRIK\\UI_Main_Config\\btn_body_config.nif");
        openBodyConfigBtn->setOnPressHandler([this](UIWidget*) { openBodyAdjustmentSubConfigUI(); });

        const auto dampenHandsBtn = std::make_shared<UIToggleButton>("FRIK\\UI_Main_Config\\btn_dampen_hands.nif");
        dampenHandsBtn->setToggleState(g_config.dampenHands);
        dampenHandsBtn->setOnToggleHandler([this](UIWidget*, const bool enabled) { g_config.saveDampenHands(enabled); });

        const auto gripModesMap = std::map<TwoHandedGripMode, std::string>{
            { TwoHandedGripMode::Mode1, "FRIK\\UI_Main_Config\\btn_grip_mode_1.nif" },
            { TwoHandedGripMode::Mode2, "FRIK\\UI_Main_Config\\btn_grip_mode_2.nif" },
            { TwoHandedGripMode::Mode3, "FRIK\\UI_Main_Config\\btn_grip_mode_3.nif" },
            { TwoHandedGripMode::Mode4, "FRIK\\UI_Main_Config\\btn_grip_mode_4.nif" }
        };
        const auto twoHandedGripModeBtn = std::make_shared<UIMultiStateToggleButton<TwoHandedGripMode>>(gripModesMap);
        twoHandedGripModeBtn->setState(getTwoHandedGripMode());
        twoHandedGripModeBtn->setOnStateChangedHandler([this](UIWidget*, const TwoHandedGripMode mode) { updateTwoHandedGripMode(mode); });

        const auto row1Container = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.3f);
        row1Container->addElement(openBodyConfigBtn);
        row1Container->addElement(dampenHandsBtn);
        row1Container->addElement(twoHandedGripModeBtn);

        const auto openPipboyConfigBtn = std::make_shared<UIButton>("FRIK\\UI_Main_Config\\btn_pipboy_config.nif");
        openPipboyConfigBtn->setOnPressHandler([this](UIWidget*) { openPipboyConfigUI(); });

        const auto openWeaponAdjustConfigBtn = std::make_shared<UIButton>("FRIK\\UI_Main_Config\\btn_weapon_adjust.nif");
        openWeaponAdjustConfigBtn->setOnPressHandler([this](UIWidget*) { openWeaponAdjustConfigUI(); });

        const auto exitBtn = std::make_shared<UIButton>("FRIK\\UI_Common\\btn_exit.nif");
        exitBtn->setOnPressHandler([this](UIWidget*) { closeMainConfigMode(); });

        const auto row2Container = std::make_shared<UIContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);
        row2Container->addElement(openPipboyConfigBtn);
        row2Container->addElement(openWeaponAdjustConfigBtn);
        row2Container->addElement(exitBtn);

        const auto mainMsg = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\msg_main.nif");
        const auto toggleSelfieMsg = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\msg_toggle_selfie.nif");

        const auto row3Container = std::make_shared<UIContainer>("Row3", UIContainerLayout::HorizontalCenter, 0.3f, 0.7f);
        row3Container->addElement(mainMsg);
        row3Container->addElement(toggleSelfieMsg);

        const auto header = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\title_main.nif", 0.45f);

        _configUI = std::make_shared<UIContainer>("MainConfig", UIContainerLayout::VerticalDown, 0.4f, 1.8f);
        _configUI->addElement(header);
        _configUI->addElement(row1Container);
        _configUI->addElement(row2Container);
        _configUI->addElement(row3Container);

        g_uiManager->attachPresetToPrimaryWandTop(_configUI, { 0, 0, 15 });
    }

    /**
     * Open the sub-config. visibility is handled on frame update.
     */
    void MainConfigMode::openBodyAdjustmentSubConfigUI()
    {
        if (_bodyAdjustmentSubConfig != nullptr) {
            logger::error("Body adjust config class is already initialized");
            return;
        }
        _bodyAdjustmentSubConfig = std::make_shared<BodyAdjustmentSubConfigMode>([this]() {
            _bodyAdjustmentSubConfig.reset();
        });
    }

    /**
     * Toggle the selfie mode on\off.
     */
    void MainConfigMode::toggleSelfieMode()
    {
        const bool newSelfieMode = !g_frik.getSelfieMode();
        logger::info("Toggle selfie mode in main config... On:{}", newSelfieMode);
        g_frik.setSelfieMode(newSelfieMode);
    }

    /**
     * Get the mode from the distinct flags.
     * It's legacy code that I don't want to change to just use grip mode everywhere.
     */
    TwoHandedGripMode MainConfigMode::getTwoHandedGripMode()
    {
        if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
            return TwoHandedGripMode::Mode1;
        }
        if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
            return TwoHandedGripMode::Mode2;
        }
        if (g_config.enableGripButtonToGrap && g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
            return TwoHandedGripMode::Mode3;
        }
        if (g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
            return TwoHandedGripMode::Mode4;
        }
        return TwoHandedGripMode::Mode3;
    }

    /**
     * Set the distinct two-handed grip flags depending on the mode.
     * And save the config.
     */
    void MainConfigMode::updateTwoHandedGripMode(const TwoHandedGripMode mode)
    {
        switch (mode) {
        case TwoHandedGripMode::Mode1:
            f4vr::showNotification("Mode 1: Offhand automatically snap to the barrel when in range, move hand quickly to let go");
            g_config.enableGripButtonToGrap = false;
            g_config.onePressGripButton = false;
            g_config.enableGripButtonToLetGo = false;
            break;
        case TwoHandedGripMode::Mode2:
            f4vr::showNotification("Mode 2: Offhand automatically snap to the barrel when in range, press grip button to let go");
            g_config.enableGripButtonToGrap = false;
            g_config.onePressGripButton = false;
            g_config.enableGripButtonToLetGo = true;
            break;
        case TwoHandedGripMode::Mode3:
            f4vr::showNotification("Mode 3: Hold grip button to snap to the barrel, release grib button to let go");
            g_config.enableGripButtonToGrap = true;
            g_config.onePressGripButton = true;
            g_config.enableGripButtonToLetGo = false;
            break;
        case TwoHandedGripMode::Mode4:
            f4vr::showNotification("Mode 4: Press grip button to snap to the barrel, press grib button again to let go");
            g_config.enableGripButtonToGrap = true;
            g_config.onePressGripButton = false;
            g_config.enableGripButtonToLetGo = true;
            break;
        default:
            // Not expected - reset to standard sticky grip
            g_config.enableGripButtonToGrap = false;
            g_config.onePressGripButton = false;
            g_config.enableGripButtonToLetGo = false;
        }
        g_config.save();
    }

    void MainConfigMode::openPipboyConfigUI()
    {
        closeMainConfigMode();
        g_frik.openPipboyConfigurationModeActive();
    }

    void MainConfigMode::openWeaponAdjustConfigUI()
    {
        closeMainConfigMode();
        g_frik.toggleWeaponRepositionMode();
    }

    void MainConfigMode::closeMainConfigMode()
    {
        g_uiManager->detachElement(_configUI, true);
        _configUI.reset();
    }
}
