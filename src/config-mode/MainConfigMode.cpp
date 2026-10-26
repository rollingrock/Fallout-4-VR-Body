#include "MainConfigMode.h"

#include "ConfigUIStyle.h"
#include "FRIK.h"
#include "perf/Perf.h"
#include "vrcf/VRControllersHaptic.h"
#include "vrcf/VRControllersManager.h"
#include "vrui/UIButton.h"
#include "vrui/UIButtonPanel.h"
#include "vrui/UIManager.h"
#include "vrui/UIMultiStateToggleButtonPanel.h"
#include "vrui/UITextPanel.h"
#include "vrui/UIToggleButtonPanel.h"

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
        tryCreateMainConfigUI();
    }

    /**
     * Open the body adjustment on its own: the main config holds it (and runs its frame update) but stays hidden, and
     * closes with it, back to whatever opened it.
     */
    void MainConfigMode::openBodyAdjustmentConfigMode()
    {
        logger::info("Open body adjustment config by call...");
        tryCreateMainConfigUI();
        if (isOpen()) {
            openBodyAdjustmentSubConfigUI(true);
        }
    }

    /**
     * Build the config UI, logging instead of crashing the game when an asset is missing (a papyrus call has no other guard).
     */
    void MainConfigMode::tryCreateMainConfigUI()
    {
        try {
            createMainConfigUI();
        } catch (const std::exception& e) {
            logger::error("Failed to open main config: {}", e.what());
            closeConfigMode();
        }
    }

    /**
     * Close the main config UI and the body adjustment sub-config if it is open.
     * The sub-config reverts its unsaved values and removes its own UI on destruction.
     */
    void MainConfigMode::closeConfigMode()
    {
        _bodyAdjustmentSubConfig.reset();
        if (_configUI) {
            g_uiManager->detachElement(_configUI, true);
            _configUI.reset();
        }
    }

    /**
     * Handle main config on every frame update.
     */
    void MainConfigMode::onFrameUpdate()
    {
        F4CF_PERF_FUNCTION();

        // open main config on the configured shortcut (both thumbsticks long-pressed by default)
        if (!isOpen() && vrcf::VRControllers.check(g_config.openMainConfigBinding)) {
            logger::info("Open main config by shortcut...");
            if (g_frik.isFavoritesMenuOpen()) {
                f4vr::closeFavoriteMenu();
            }
            vrcf::VRHaptics.trigger(vrcf::Hand::Primary, vrcf::HapticPattern::RampUp);
            tryCreateMainConfigUI();
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
        const auto openBodyConfigBtn = std::make_shared<UIButtonPanel>("BodyConfig");
        openBodyConfigBtn->setTopText("BODY");
        openBodyConfigBtn->setImage("vrui\\main\\body-config.dds");
        openBodyConfigBtn->setBottomText("CONFIG");
        openBodyConfigBtn->setOnPressHandler([this](UIButtonPanel*) {
            openBodyAdjustmentSubConfigUI();
        });

        const auto dampenHandsBtn = std::make_shared<UIToggleButtonPanel>("DampenHands");
        dampenHandsBtn->setTopText("DAMPEN");
        dampenHandsBtn->setImage("vrui\\main\\dampen-hands.dds");
        dampenHandsBtn->setBottomText("HANDS");
        dampenHandsBtn->setToggleState(g_config.dampenHands);
        dampenHandsBtn->setOnToggleHandler([this](UIToggleButtonPanel*, const bool enabled) {
            g_config.saveDampenHands(enabled);
        });

        const auto row1Container = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.3f);
        row1Container->addElement(openBodyConfigBtn);
        row1Container->addElement(dampenHandsBtn);

        // two-handed grip mode is part of weapon positioning, hide it when that feature is disabled via API
        if (g_frik.isWeaponPositionEnabled()) {
            // one icon for every mode; the bottom line names it
            const auto gripMode = [](const char* mode) {
                return UIButtonPanelContent{ .topText = "2 HANDED", .bottomText = mode, .image = "vrui\\main\\grip-mode.dds" };
            };
            const auto twoHandedGripModeBtn = std::make_shared<UIMultiStateToggleButtonPanel<TwoHandedGripMode>>("TwoHandedGripMode",
                std::map<TwoHandedGripMode, UIButtonPanelContent>{ { TwoHandedGripMode::Mode1, gripMode("GRIP MODE 1") },
                    { TwoHandedGripMode::Mode2, gripMode("GRIP MODE 2") },
                    { TwoHandedGripMode::Mode3, gripMode("GRIP MODE 3") },
                    { TwoHandedGripMode::Mode4, gripMode("GRIP MODE 4") } });
            twoHandedGripModeBtn->setState(g_config.twoHandedGripMode);
            twoHandedGripModeBtn->setOnStateChangedHandler([this](UIMultiStateToggleButtonPanel<TwoHandedGripMode>*, const TwoHandedGripMode mode) {
                updateTwoHandedGripMode(mode);
            });
            row1Container->addElement(twoHandedGripModeBtn);
        }

        // sub-config + external-mod buttons share the second row; collect them so they can be split
        // into multiple balanced rows when there are too many to fit in one (see createBalancedButtonRows)
        std::vector<std::shared_ptr<UIElement>> secondRowButtons;

        // hide a sub-config button when its feature is disabled via API as its config is not relevant
        if (g_frik.isPipboyEnabled()) {
            const auto openPipboyConfigBtn = std::make_shared<UIButtonPanel>("PipboyConfig");
            openPipboyConfigBtn->setTopText("PIPBOY");
            openPipboyConfigBtn->setImage("vrui\\main\\pipboy-config.dds");
            openPipboyConfigBtn->setBottomText("CONFIG");
            openPipboyConfigBtn->setOnPressHandler([this](UIButtonPanel*) {
                openPipboyConfigUI();
            });
            secondRowButtons.push_back(openPipboyConfigBtn);
        }
        if (g_frik.isWeaponPositionEnabled()) {
            const auto openWeaponAdjustConfigBtn = std::make_shared<UIButtonPanel>("WeaponConfig");
            openWeaponAdjustConfigBtn->setTopText("WEAPON");
            openWeaponAdjustConfigBtn->setImage("vrui\\main\\weapon-config.dds");
            openWeaponAdjustConfigBtn->setBottomText("CONFIG");
            openWeaponAdjustConfigBtn->setOnPressHandler([this](UIButtonPanel*) {
                openWeaponAdjustConfigUI();
            });
            secondRowButtons.push_back(openWeaponAdjustConfigBtn);
        }
        // other mods register their button as a NIF through the API, so it stays a NIF widget beside the panels
        for (const auto& buttonData : _externalModConfigButtonDataList) {
            const auto openExtModConfigBtn = std::make_shared<UIButton>(buttonData.buttonIconNifPath);
            openExtModConfigBtn->setOnPressHandler([this, buttonData](UIWidget*) {
                openExternalModConfig(buttonData);
            });
            secondRowButtons.push_back(openExtModConfigBtn);
        }

        const auto advancedConfigBtn = std::make_shared<UIButtonPanel>("AdvancedConfig");
        advancedConfigBtn->setTopText("OPEN");
        advancedConfigBtn->setImage("f4cf\\vrui\\advanced-config.DDS");
        advancedConfigBtn->setBottomText("CONFIG FILE");
        advancedConfigBtn->setOnPressHandler([](UIButtonPanel*) {
            openAdvancedConfig();
        });

        const auto helpWikiBtn = std::make_shared<UIButtonPanel>("HelpWiki");
        helpWikiBtn->setTopText("OPEN");
        helpWikiBtn->setImage("f4cf\\vrui\\help-wiki.DDS");
        helpWikiBtn->setBottomText("HELP WIKI");
        helpWikiBtn->setOnPressHandler([](UIButtonPanel*) {
            openHelpWiki();
        });

        const auto exitBtn = std::make_shared<UIButtonPanel>("Exit");
        exitBtn->setImage("f4cf\\vrui\\exit.DDS");
        exitBtn->setBottomText("EXIT");
        exitBtn->setOnPressHandler([this](UIButtonPanel*) {
            closeConfigMode();
        });

        const auto row3Container = std::make_shared<UIContainer>("Row3", UIContainerLayout::HorizontalCenter, 0.3f);
        row3Container->addElement(advancedConfigBtn);
        row3Container->addElement(helpWikiBtn);
        row3Container->addElement(exitBtn);

        const auto messagesContainer = std::make_shared<UIContainer>("Messages", UIContainerLayout::HorizontalCenter, 0.3f);

        // the tip names the open-config binding, so there is nothing to tell while it is turned off
        if (g_config.openMainConfigBinding.isEnabled()) {
            const auto footer = std::make_shared<UITextPanel>("Footer");
            footer->setStyle(F4VR_PANEL_STYLE);
            footer->setTextHeight(CONFIG_FOOTER_TEXT_HEIGHT);
            footer->setContent([](std::vector<TextRow>& rows) {
                rows.emplace_back("TIP: OPEN THIS CONFIG");
                rows.push_back({ .spans = { { .text = "WITH  " } } });
                appendBindingPrompt(rows.back().spans, g_config.openMainConfigBinding, CONFIG_FOOTER_PROMPT_STYLE);
            });
            messagesContainer->addElement(footer);
        }
        messagesContainer->addElement(createToggleSelfieMsg());

        const auto header = std::make_shared<UITextPanel>("Title");
        header->setStyle(UIPanelStyle{ .color = F4VR_PANEL_STYLE.color });
        header->setTextHeight(0.5f);
        header->setContent([](std::vector<TextRow>& rows) {
            rows.emplace_back("FRIK: CONFIGURATION", std::nullopt, render::TextDecoration::Underline);
        });

        _configUI = std::make_shared<UIContainer>("MainConfig", UIContainerLayout::VerticalUp, 0.35f, 1.8f);
        _configUI->addElement(messagesContainer);
        _configUI->addElement(row3Container);
        if (const auto secondRowContainer = createBalancedButtonRows("Row2", secondRowButtons)) {
            _configUI->addElement(secondRowContainer);
        }
        _configUI->addElement(row1Container);
        _configUI->addElement(header);

        g_uiManager->attachPresetToPrimaryWandTop(_configUI, { 0, 0, 0 });
    }

    /**
     * Arrange the given buttons into one or more horizontally-centered rows stacked vertically.
     * A single row is capped so it doesn't get too long; when there are more buttons they are split
     * into multiple balanced rows (e.g. 6 buttons become 2 rows of 3, not a row of 5 and a row of 1).
     * Returns null when there are no buttons.
     */
    std::shared_ptr<UIContainer> MainConfigMode::createBalancedButtonRows(const std::string& name, const std::vector<std::shared_ptr<UIElement>>& buttons)
    {
        constexpr std::size_t maxButtonsPerRow = 5;

        const std::size_t buttonCount = buttons.size();
        if (buttonCount == 0) {
            return nullptr;
        }

        const std::size_t rowCount = (buttonCount + maxButtonsPerRow - 1) / maxButtonsPerRow;
        const std::size_t minButtonsPerRow = buttonCount / rowCount;
        const std::size_t rowsWithExtraButton = buttonCount % rowCount;

        const auto rowsContainer = std::make_shared<UIContainer>(name, UIContainerLayout::VerticalCenter, 0.35f);
        std::size_t buttonIndex = 0;
        for (std::size_t row = 0; row < rowCount; row++) {
            const std::size_t buttonsInRow = minButtonsPerRow + (row < rowsWithExtraButton ? 1 : 0);
            const auto rowContainer = std::make_shared<UIContainer>(name + "Row" + std::to_string(row), UIContainerLayout::HorizontalCenter, 0.3f);
            for (std::size_t i = 0; i < buttonsInRow; i++, buttonIndex++) {
                rowContainer->addElement(buttons[buttonIndex]);
            }
            rowsContainer->addElement(rowContainer);
        }
        return rowsContainer;
    }

    /**
     * Open the sub-config. visibility is handled on frame update.
     */
    void MainConfigMode::openBodyAdjustmentSubConfigUI(const bool closeConfigOnClose)
    {
        if (_bodyAdjustmentSubConfig != nullptr) {
            logger::error("Body adjust config class is already initialized");
            return;
        }
        _bodyAdjustmentSubConfig = std::make_shared<BodyAdjustmentSubConfigMode>([this, closeConfigOnClose]() {
            if (closeConfigOnClose) {
                closeConfigMode();
            } else {
                _bodyAdjustmentSubConfig.reset();
            }
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
     * Announce the selected grip mode and save it through the shared config setting.
     */
    void MainConfigMode::updateTwoHandedGripMode(const TwoHandedGripMode mode)
    {
        switch (mode) {
        case TwoHandedGripMode::Mode1:
            f4vr::showNotification("Mode 1: Offhand automatically snap to the barrel when in range, move hand quickly to let go");
            break;
        case TwoHandedGripMode::Mode2:
            f4vr::showNotification("Mode 2: Offhand automatically snap to the barrel when in range, press grip button to let go");
            break;
        case TwoHandedGripMode::Mode3:
            f4vr::showNotification("Mode 3: Hold grip button to snap to the barrel, release grib button to let go");
            break;
        case TwoHandedGripMode::Mode4:
            f4vr::showNotification("Mode 4: Press grip button to snap to the barrel, press grib button again to let go");
            break;
        }
        g_config.setTwoHandedGripMode(mode);
    }

    void MainConfigMode::openPipboyConfigUI()
    {
        closeConfigMode();
        g_frik.openPipboyConfigurationModeActive();
    }

    void MainConfigMode::openWeaponAdjustConfigUI()
    {
        closeConfigMode();
        g_frik.toggleWeaponRepositionMode();
    }

    void MainConfigMode::openExternalModConfig(const OpenExternalModConfigData& data)
    {
        logger::info("Open external mod config for mod: '{}', messageType:{}", data.callbackReceiverName, data.callbackMessageType);
        closeConfigMode();
        g_frik.dispatchMessageToExternalMod(data.callbackReceiverName, data.callbackMessageType, nullptr, 0);
    }

    /**
     * Open FRIK.toml in Notepad on the PC desktop for advanced settings not exposed in the in-VR menu.
     */
    void MainConfigMode::openAdvancedConfig()
    {
        logger::info("Open advanced config (FRIK.toml) on PC...");
        f4vr::showNotification("Config file opened in Notepad on your PC.\nSwitch to your monitor to edit advanced settings.\nSaved changes apply live.");
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
}
