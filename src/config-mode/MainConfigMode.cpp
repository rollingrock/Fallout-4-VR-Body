#include "MainConfigMode.h"

#include "FRIK.h"
#include "vrcf/VRControllersHaptic.h"
#include "vrcf/VRControllersManager.h"
#include "vrui/UIManager.h"
#include "vrui/UIMultiStateToggleButton.h"
#include "vrui/UIToggleGroupContainer.h"

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
        // open main config on the configured shortcut (both thumbsticks long-pressed by default)
        if (!isOpen() && vrcf::VRControllers.check(g_config.openMainConfigBinding)) {
            logger::info("Open main config by shortcut...");
            if (g_frik.isFavoritesMenuOpen()) {
                f4vr::closeFavoriteMenu();
            }
            vrcf::VRHaptics.trigger(vrcf::Hand::Primary, vrcf::HapticPattern::RampUp);
            createMainConfigUI();
        }

        if (!isOpen()) {
            return;
        }

        _configUI->setPosition(0, 0, f4vr::isNodeVisible(f4vr::getWeaponNode()) ? 6.0f : 0.0f);

        // toggle selfie if Pipboy is not open as it uses the same button
        if (!g_frik.isPipboyOn() && vrcf::VRControllers.check(g_config.toggleSelfieBinding)) {
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
     * Add a button to open the main config for another mod that registered via API.
     */
    void MainConfigMode::registerOpenExternalModSettingButton(const OpenExternalModConfigData& data)
    {
        F4SE::log::info("Register external mod config button: '{}', messageType: {}", data.callbackReceiverName, data.callbackMessageType);
        _externalModConfigButtonDataList.push_back(data);
    }

    /**
     * Create all the main config UI elements.
     */
    void MainConfigMode::createMainConfigUI()
    {
        const auto openBodyConfigBtn = std::make_shared<UIButton>("ui-config-main\\btn-body-config.nif");
        openBodyConfigBtn->setOnPressHandler([this](UIWidget*) {
            openBodyAdjustmentSubConfigUI();
        });

        const auto dampenHandsBtn = std::make_shared<UIToggleButton>("ui-config-main\\btn-dampen-hands.nif");
        dampenHandsBtn->setToggleState(g_config.dampenHands);
        dampenHandsBtn->setOnToggleHandler([this](UIWidget*, const bool enabled) {
            g_config.saveDampenHands(enabled);
        });

        const auto row1Container = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.3f);
        row1Container->addElement(openBodyConfigBtn);
        row1Container->addElement(dampenHandsBtn);

        // two-handed grip mode is part of weapon positioning, hide it when that feature is disabled via API
        if (g_frik.isWeaponPositionEnabled()) {
            const auto gripModesMap = std::map<TwoHandedGripMode, std::string>{ { TwoHandedGripMode::Mode1, "ui-config-main\\btn-grip-mode-1.nif" },
                { TwoHandedGripMode::Mode2, "ui-config-main\\btn-grip-mode-2.nif" },
                { TwoHandedGripMode::Mode3, "ui-config-main\\btn-grip-mode-3.nif" },
                { TwoHandedGripMode::Mode4, "ui-config-main\\btn-grip-mode-4.nif" } };
            const auto twoHandedGripModeBtn = std::make_shared<UIMultiStateToggleButton<TwoHandedGripMode>>(gripModesMap);
            twoHandedGripModeBtn->setState(getTwoHandedGripMode());
            twoHandedGripModeBtn->setOnStateChangedHandler([this](UIWidget*, const TwoHandedGripMode mode) {
                updateTwoHandedGripMode(mode);
            });
            row1Container->addElement(twoHandedGripModeBtn);
        }

        const auto row2Container = std::make_shared<UIContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);

        // hide a sub-config button when its feature is disabled via API as its config is not relevant
        if (g_frik.isPipboyEnabled()) {
            const auto openPipboyConfigBtn = std::make_shared<UIButton>("ui-config-main\\btn-pipboy-config.nif");
            openPipboyConfigBtn->setOnPressHandler([this](UIWidget*) {
                openPipboyConfigUI();
            });
            row2Container->addElement(openPipboyConfigBtn);
        }
        if (g_frik.isWeaponPositionEnabled()) {
            const auto openWeaponAdjustConfigBtn = std::make_shared<UIButton>("ui-config-main\\btn-weapon-adjust.nif");
            openWeaponAdjustConfigBtn->setOnPressHandler([this](UIWidget*) {
                openWeaponAdjustConfigUI();
            });
            row2Container->addElement(openWeaponAdjustConfigBtn);
        }

        for (const auto& buttonData : _externalModConfigButtonDataList) {
            const auto openExtModConfigBtn = std::make_shared<UIButton>(buttonData.buttonIconNifPath);
            openExtModConfigBtn->setOnPressHandler([this, buttonData](UIWidget*) {
                openExternalModConfig(buttonData);
            });
            row2Container->addElement(openExtModConfigBtn);
        }

        const auto advancedConfigBtn = std::make_shared<UIButton>("ui-common\\btn-advanced-config.nif");
        advancedConfigBtn->setOnPressHandler([](UIWidget*) {
            openAdvancedConfig();
        });

        const auto helpWikiBtn = std::make_shared<UIButton>("ui-common\\btn-help-wiki.nif");
        helpWikiBtn->setOnPressHandler([](UIWidget*) {
            openHelpWiki();
        });

        const auto exitBtn = std::make_shared<UIButton>("ui-common\\btn-exit.nif");
        exitBtn->setOnPressHandler([this](UIWidget*) {
            closeMainConfigMode();
        });

        const auto row3Container = std::make_shared<UIContainer>("Row3", UIContainerLayout::HorizontalCenter, 0.3f);
        row3Container->addElement(advancedConfigBtn);
        row3Container->addElement(helpWikiBtn);
        row3Container->addElement(exitBtn);

        const auto mainMsg = std::make_shared<UIWidget>("ui-config-main\\msg-main.nif");
        const auto toggleSelfieMsg = std::make_shared<UIWidget>("ui-config-main\\msg-toggle-selfie.nif");

        const auto messagesContainer = std::make_shared<UIContainer>("Messages", UIContainerLayout::HorizontalCenter, 0.3f);
        messagesContainer->addElement(mainMsg);
        messagesContainer->addElement(toggleSelfieMsg);

        const auto header = std::make_shared<UIWidget>("ui-config-main\\title-main.nif", 1.7f);

        _configUI = std::make_shared<UIContainer>("MainConfig", UIContainerLayout::VerticalUp, 0.35f, 1.8f);
        _configUI->addElement(messagesContainer);
        _configUI->addElement(row3Container);
        _configUI->addElement(row2Container);
        _configUI->addElement(row1Container);
        _configUI->addElement(header);

        g_uiManager->attachPresetToPrimaryWandTop(_configUI, { 0, 0, 0 });
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
        const bool newSelfieMode = !g_frik.isSelfieModeOn();
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

    void MainConfigMode::openExternalModConfig(const OpenExternalModConfigData& data)
    {
        logger::info("Open external mod config for mod: '{}', messageType:{}", data.callbackReceiverName, data.callbackMessageType);
        closeMainConfigMode();
        g_frik.dispatchMessageToExternalMod(data.callbackReceiverName, data.callbackMessageType, nullptr, 0);
    }

    /**
     * Open FRIK.ini in Notepad on the PC desktop for advanced settings not exposed in the in-VR menu.
     */
    void MainConfigMode::openAdvancedConfig()
    {
        logger::info("Open advanced config (FRIK.ini) on PC...");
        f4vr::showNotification("Config INI opened in Notepad on your PC.\nSwitch to your monitor to edit advanced settings.\nSaved changes apply live.");
        Config::openInNotepad();
    }

    /**
     * Open the FRIK help wiki page in the default browser on the PC desktop.
     */
    void MainConfigMode::openHelpWiki()
    {
        logger::info("Open help wiki on PC...");
        f4vr::showNotification("FRIK help wiki opened in your browser.\nSwitch to your monitor to read it.");
        // Launch the URL via explorer.exe rather than passing it directly to ShellExecute: in the game process the shell's
        // protocol-association lookup needs COM initialized on this thread and fails silently, but a direct exe launch works.
        ShellExecuteA(nullptr, "open", "explorer.exe", "https://github.com/rollingrock/Fallout-4-VR-Body/blob/main/docs/README.md", nullptr, SW_SHOWNORMAL);
    }

    void MainConfigMode::closeMainConfigMode()
    {
        g_uiManager->detachElement(_configUI, true);
        _configUI.reset();
    }
}
