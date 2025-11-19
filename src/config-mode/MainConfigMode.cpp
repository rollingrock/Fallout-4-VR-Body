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
        openMainConfigUI();
    }

    void MainConfigMode::onFrameUpdate()
    {
        if (!isOpen()
            && f4vr::VRControllers.isLongPressed(f4vr::Hand::Primary, vr::k_EButton_Axis0)
            && f4vr::VRControllers.isPressHeldDown(f4vr::Hand::Offhand, vr::k_EButton_Axis0)) {
            logger::info("Open main config by shortcut...");
            if (g_frik.isFavoritesMenuOpen()) {
                f4vr::closeFavoriteMenu();
            }
            openMainConfigUI();
        }
    }

    void MainConfigMode::openMainConfigUI()
    {
        const auto openBodyConfigBtn = std::make_shared<UIButton>("FRIK/ui_main_conf_btn_body_pos.nif");
        openBodyConfigBtn->setOnPressHandler([this](UIWidget*) { openBodyConfigUI(); });

        const auto dampenHandsBtn = std::make_shared<UIToggleButton>("FRIK/ui_main_conf_btn_dampen_hands.nif");
        dampenHandsBtn->setToggleState(g_config.dampenHands);
        dampenHandsBtn->setOnToggleHandler([this](UIWidget*, const bool enabled) { setDampenHands(enabled); });

        const auto gripModesMap = std::map<TwoHandedGripMode, std::string>{
            { TwoHandedGripMode::Mode1, "FRIK/ui_main_conf_btn_grip_mode_1.nif" },
            { TwoHandedGripMode::Mode2, "FRIK/ui_main_conf_btn_grip_mode_2.nif" },
            { TwoHandedGripMode::Mode3, "FRIK/ui_main_conf_btn_grip_mode_3.nif" },
            { TwoHandedGripMode::Mode4, "FRIK/ui_main_conf_btn_grip_mode_4.nif" }
        };
        const auto twoHandedGripModeBtn = std::make_shared<UIMultiStateToggleButton<TwoHandedGripMode>>(gripModesMap);
        twoHandedGripModeBtn->setState(getTwoHandedGripMode());
        twoHandedGripModeBtn->setOnStateChangedHandler([this](UIWidget*, const TwoHandedGripMode mode) { updateTwoHandedGripMode(mode); });

        const auto row1Container = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.3f);
        row1Container->addElement(openBodyConfigBtn);
        row1Container->addElement(dampenHandsBtn);
        row1Container->addElement(twoHandedGripModeBtn);

        const auto openPipboyConfigBtn = std::make_shared<UIButton>("FRIK/ui_main_conf_btn_pipboy_config.nif");
        openPipboyConfigBtn->setOnPressHandler([this](UIWidget*) { openPipboyConfigUI(); });

        const auto openWeaponAdjustConfigBtn = std::make_shared<UIButton>("FRIK/ui_main_conf_btn_weapon_adjust.nif");
        openWeaponAdjustConfigBtn->setOnPressHandler([this](UIWidget*) { openWeaponAdjustConfigUI(); });

        const auto exitBtn = std::make_shared<UIButton>("FRIK/ui_common_btn_exit.nif");
        exitBtn->setOnPressHandler([this](UIWidget*) { closeMainConfigMode(); });

        const auto row2Container = std::make_shared<UIContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);
        row2Container->addElement(openPipboyConfigBtn);
        row2Container->addElement(openWeaponAdjustConfigBtn);
        row2Container->addElement(exitBtn);

        // _primaryHandModeButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_primary_hand.nif");
        // _primaryHandModeButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::PrimaryHand; });
        //
        // _offhandModeButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_offhand.nif");
        // _offhandModeButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::Offhand; });
        //
        // _throwableUIButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_throwable.nif");
        // _throwableUIButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::Throwable; });
        //
        // const auto backOfHandUIButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_back_of_hand_ui.nif");
        // backOfHandUIButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::BackOfHandUI; });
        //

        // firstRowContainerInner->addElement(_primaryHandModeButton);
        // firstRowContainerInner->addElement(_offhandModeButton);
        // firstRowContainerInner->addElement(_throwableUIButton);
        // firstRowContainerInner->addElement(backOfHandUIButton);
        //
        // if (isBetterScopesVRModLoaded()) {
        //     _betterScopesModeButton = std::make_shared<UIToggleButton>("FRIK/ui_weapconf_btn_better_scopes_vr.nif");
        //     _betterScopesModeButton->setOnToggleHandler([this](UIWidget*, bool) { _repositionTarget = RepositionTarget::BetterScopes; });
        //     firstRowContainerInner->addElement(_betterScopesModeButton);
        // }
        //
        // _emptyHandsMessageBox = std::make_shared<UIWidget>("FRIK/ui_weapconf_msg_empty_hands.nif");
        //
        // const auto firstRowContainer = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.3f);
        // firstRowContainer->addElement(_emptyHandsMessageBox);
        // firstRowContainer->addElement(firstRowContainerInner);
        //
        // _saveButton = std::make_shared<UIButton>("FRIK/ui_common_btn_save.nif");
        // _saveButton->setOnPressHandler([this](UIWidget*) { saveConfig(); });
        //
        // _resetButton = std::make_shared<UIButton>("FRIK/ui_common_btn_reset.nif");
        // _resetButton->setOnPressHandler([this](UIWidget*) { resetConfig(); });
        //

        //
        // _throwableNotEquippedMessageBox = std::make_shared<UIWidget>("FRIK/ui_weapconf_msg_throwable_empty_hands.nif");
        //
        // const auto secondRowContainer = std::make_shared<UIContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);
        // secondRowContainer->addElement(_saveButton);
        // secondRowContainer->addElement(_resetButton);
        // secondRowContainer->addElement(_throwableNotEquippedMessageBox);
        // secondRowContainer->addElement(exitButton);
        //
        const auto header = std::make_shared<UIWidget>("FRIK/ui_main_conf_title.nif", 0.4f);
        // _complexAdjustFooter = std::make_shared<UIWidget>("FRIK/ui_weapconf_msg_footer.nif", 0.7f);
        // _throwableAdjustFooter = std::make_shared<UIWidget>("FRIK/ui_weapconf_msg_footer_throwable.nif", 0.7f);
        // _simpleAdjustFooter = std::make_shared<UIWidget>("FRIK/ui_weapconf_msg_footer_simple.nif", 0.7f);
        // _simpleAdjustFooter->setVisibility(false);
        //
        // const auto mainContainer = std::make_shared<UIContainer>("Main", UIContainerLayout::VerticalCenter, 0.3f);
        // mainContainer->addElement(firstRowContainer);
        // mainContainer->addElement(secondRowContainer);
        // mainContainer->addElement(_complexAdjustFooter);
        // mainContainer->addElement(_throwableAdjustFooter);
        // mainContainer->addElement(_simpleAdjustFooter);

        _configUI = std::make_shared<UIContainer>("MainConfig", UIContainerLayout::VerticalDown, 0.4f, 1.8f);
        _configUI->addElement(header);
        _configUI->addElement(row1Container);
        _configUI->addElement(row2Container);

        // start hidden by default (will be set visible in frame update if it should be)
        // _configUI->setVisibility(false);

        g_uiManager->attachPresetToPrimaryWandLeft(_configUI, { -1, 4, 0 });
    }

    void MainConfigMode::openBodyConfigUI() {}

    /**
     * Set the dampen hands value and save the config.
     */
    void MainConfigMode::setDampenHands(const bool enabled)
    {
        g_config.dampenHands = enabled;
        g_config.save();
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
