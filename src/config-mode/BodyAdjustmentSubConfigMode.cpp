#include "BodyAdjustmentSubConfigMode.h"

#include "Config.h"
#include "utils.h"
#include "skeleton/HandPose.h"
#include "ui/UIButton.h"
#include "ui/UIManager.h"
#include "ui/UIToggleGroupContainer.h"
#include "ui/UIWidget.h"

using namespace vrui;
using namespace common;

namespace
{
    void updateVRScaleGameConfig()
    {
        const auto set = RE::GetINISetting("fVrScale:VR");
        set->SetFloat(frik::g_config.fVrScale);
    }
}

namespace frik
{
    BodyAdjustmentSubConfigMode::BodyAdjustmentSubConfigMode(const std::function<void()>& onClose) :
        _onClose(onClose)
    {
        createConfigUI();
    }

    void BodyAdjustmentSubConfigMode::onFrameUpdate() const
    {
        _noneMsg->setVisibility(_configTarget == BodyAdjustmentConfigTarget::None);
        _heightMsg->setVisibility(_configTarget == BodyAdjustmentConfigTarget::BodyHeight);
        _forwardMsg->setVisibility(_configTarget == BodyAdjustmentConfigTarget::BodyForwardOffset);
        _armsLengthMsg->setVisibility(_configTarget == BodyAdjustmentConfigTarget::BodyArmsLength);
        _vrScaleMsg->setVisibility(_configTarget == BodyAdjustmentConfigTarget::VRScale);

        handleAdjustment();
    }

    /**
     * Create all the config elements.
     */
    void BodyAdjustmentSubConfigMode::createConfigUI()
    {
        const auto playSeattedBtn = std::make_shared<UIToggleButton>("FRIK\\UI_Main_Config\\btn_play_seated.nif");
        playSeattedBtn->setToggleState(g_config.isPlayingSeated);
        playSeattedBtn->setOnToggleHandler([this](UIWidget*, const bool enabled) { togglePlayingSeated(enabled); });

        const auto hideHeadBtn = std::make_shared<UIToggleButton>("FRIK\\UI_Main_Config\\btn_hide_head.nif");
        hideHeadBtn->setToggleState(g_config.hideHeadEquipment);
        hideHeadBtn->setOnToggleHandler([this](UIWidget*, const bool enabled) { toggleHideHeadEquipment(enabled); });

        const auto row1Container = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.5f);
        row1Container->addElement(playSeattedBtn);
        row1Container->addElement(hideHeadBtn);

        const auto heightToggleBtn = std::make_shared<UIToggleButton>("FRIK\\UI_Main_Config\\btn_body_vertical.nif");
        heightToggleBtn->setOnToggleHandler([this](UIWidget*, bool) { _configTarget = BodyAdjustmentConfigTarget::BodyHeight; });

        const auto forwardToggleBtn = std::make_shared<UIToggleButton>("FRIK\\UI_Main_Config\\btn_body_forward.nif");
        forwardToggleBtn->setOnToggleHandler([this](UIWidget*, bool) { _configTarget = BodyAdjustmentConfigTarget::BodyForwardOffset; });

        const auto armsLengthToggleBtn = std::make_shared<UIToggleButton>("FRIK\\UI_Main_Config\\btn_arms_length.nif");
        armsLengthToggleBtn->setOnToggleHandler([this](UIWidget*, bool) { _configTarget = BodyAdjustmentConfigTarget::BodyArmsLength; });

        const auto vrScaleToggleBtn = std::make_shared<UIToggleButton>("FRIK\\UI_Main_Config\\btn_vr_scale.nif");
        vrScaleToggleBtn->setOnToggleHandler([this](UIWidget*, bool) { _configTarget = BodyAdjustmentConfigTarget::VRScale; });

        _row2Container = std::make_shared<UIToggleGroupContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);
        _row2Container->addElement(heightToggleBtn);
        _row2Container->addElement(forwardToggleBtn);
        _row2Container->addElement(armsLengthToggleBtn);
        _row2Container->addElement(vrScaleToggleBtn);

        const auto saveBtn = std::make_shared<UIButton>("FRIK\\UI_Common\\btn_save.nif");
        saveBtn->setOnPressHandler([this](UIWidget*) { saveConfig(); });

        const auto resetBtn = std::make_shared<UIButton>("FRIK\\UI_Common\\btn_reset.nif");
        resetBtn->setOnPressHandler([this](UIWidget*) { resetConfig(); });

        const auto exitBtn = std::make_shared<UIButton>("FRIK\\UI_Common\\btn_back.nif");
        exitBtn->setOnPressHandler([this](UIWidget*) { closeConfig(); });

        const auto row3Container = std::make_shared<UIContainer>("Row3", UIContainerLayout::HorizontalCenter, 0.3f);
        row3Container->addElement(saveBtn);
        row3Container->addElement(resetBtn);
        row3Container->addElement(exitBtn);

        _noneMsg = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\msg_node_selected.nif");
        _heightMsg = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\msg_body_vertical.nif");
        _forwardMsg = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\msg_body_forward.nif");
        _armsLengthMsg = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\msg_arms_length.nif");
        _vrScaleMsg = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\msg_vr_scale.nif");
        const auto toggleSelfieMsg = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\msg_toggle_selfie.nif");

        const auto row4Container = std::make_shared<UIContainer>("Row4", UIContainerLayout::HorizontalCenter, 0.3f, 0.7f);
        row4Container->addElement(_noneMsg);
        row4Container->addElement(_heightMsg);
        row4Container->addElement(_forwardMsg);
        row4Container->addElement(_armsLengthMsg);
        row4Container->addElement(_vrScaleMsg);
        row4Container->addElement(toggleSelfieMsg);

        const auto header = std::make_shared<UIWidget>("FRIK\\UI_Main_Config\\title_body_adjust.nif", 0.5f);

        _configUI = std::make_shared<UIContainer>("BodyAdjustConfig", UIContainerLayout::VerticalDown, 0.35f, 1.8f);
        _configUI->addElement(header);
        _configUI->addElement(row1Container);
        _configUI->addElement(_row2Container);
        _configUI->addElement(row3Container);
        _configUI->addElement(row4Container);

        g_uiManager->attachPresetToPrimaryWandTop(_configUI, { 0, 0, 20 });
    }

    /**
     * Toggle seated play.
     */
    void BodyAdjustmentSubConfigMode::togglePlayingSeated(const bool seated)
    {
        g_config.saveIsPlayingSeated(seated);
        _configTarget = BodyAdjustmentConfigTarget::None;
    }

    /**
     * Toggle head hiding on\off.
     * Show notification regarding not hiding in selfie mode for player not to be confused.
     */
    void BodyAdjustmentSubConfigMode::toggleHideHeadEquipment(const bool hide)
    {
        g_config.saveHideHeadEquipment(hide);
        _row2Container->clearToggleState();
        _configTarget = BodyAdjustmentConfigTarget::None;
        if (hide) {
            std::string msg = "Player head equipment is now hidden";
            if (g_config.selfieIgnoreHideFlags) {
                msg = msg + "\nNote: The head equipment is NOT hidden in selfie mode!";
            }
            f4vr::showNotification(msg);
        }
    }

    /**
     * On close of the body adjustment UI we clear unsaved config, cleanup, and close.
     */
    void BodyAdjustmentSubConfigMode::closeConfig()
    {
        // reload config to revert unsaved values
        g_config.loadIniOnly();

        // redo VR Scale if changed
        updateVRScaleGameConfig();

        // close the UI
        g_uiManager->detachElement(_configUI, true);
        _configUI.reset();

        // notify parent
        _onClose();
    }

    /**
     * Delegate adjustment to the right target.
     * Read the thumbstick value and change the taget values accordingly.
     */
    void BodyAdjustmentSubConfigMode::handleAdjustment() const
    {
        switch (_configTarget) {
        case BodyAdjustmentConfigTarget::BodyHeight:
            handleHeightAdjustment();
            break;
        case BodyAdjustmentConfigTarget::BodyForwardOffset:
            handleForwardAdjustment();
            break;
        case BodyAdjustmentConfigTarget::BodyArmsLength:
            handleArmsLengthAdjustment();
            break;
        case BodyAdjustmentConfigTarget::VRScale:
            handleVRScaleAdjustment();
            break;
        case BodyAdjustmentConfigTarget::None:
            break;
        }
    }

    void BodyAdjustmentSubConfigMode::handleHeightAdjustment()
    {
        const auto primAxisY = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Primary).y;
        g_config.setPlayerHMDOffsetUp(g_config.getPlayerHMDOffsetUp() + correctAdjustmentValue(primAxisY, 4));
        g_config.setPlayerBodyOffsetUp(g_config.getPlayerBodyOffsetUp() - 0.125f * correctAdjustmentValue(primAxisY, 4));

        const auto offAxisY = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Offhand).y;
        g_config.setPlayerBodyOffsetUp(g_config.getPlayerBodyOffsetUp() - correctAdjustmentValue(offAxisY, 4));
    }

    void BodyAdjustmentSubConfigMode::handleForwardAdjustment()
    {
        const auto axisY = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Primary).y;
        g_config.setPlayerBodyOffsetForward(g_config.getPlayerBodyOffsetForward() + correctAdjustmentValue(axisY, 4));
    }

    void BodyAdjustmentSubConfigMode::handleArmsLengthAdjustment()
    {
        const auto axisY = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Primary).y;
        g_config.armLength += correctAdjustmentValue(axisY, 5);
    }

    void BodyAdjustmentSubConfigMode::handleVRScaleAdjustment()
    {
        const auto axisY = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Primary).y;
        g_config.fVrScale += correctAdjustmentValue(axisY, 5);
        updateVRScaleGameConfig();
    }

    void BodyAdjustmentSubConfigMode::saveConfig()
    {
        f4vr::showNotification("Saving all body adjustment configs");
        g_config.save();
    }

    /**
     * Reset the current config adjusting value to the default in the embedded config.
     * Use a small hack to load ONLY the embedded config into a temp config object.
     */
    void BodyAdjustmentSubConfigMode::resetConfig() const
    {
        Config defaultConfig;
        defaultConfig.loadEmbeddedDefaultOnly();
        defaultConfig.isPlayingSeated = g_config.isPlayingSeated;

        switch (_configTarget) {
        case BodyAdjustmentConfigTarget::BodyHeight:
            f4vr::showNotification("Reset height body adjustment config");
            g_config.setPlayerHMDOffsetUp(defaultConfig.getPlayerHMDOffsetUp());
            g_config.setPlayerBodyOffsetUp(defaultConfig.getPlayerBodyOffsetUp());
            break;
        case BodyAdjustmentConfigTarget::BodyForwardOffset:
            f4vr::showNotification("Reset forward body adjustment config");
            g_config.setPlayerBodyOffsetForward(defaultConfig.getPlayerBodyOffsetForward());
            break;
        case BodyAdjustmentConfigTarget::BodyArmsLength:
            f4vr::showNotification("Reset arms length body adjustment config");
            g_config.armLength = defaultConfig.armLength;
            break;
        case BodyAdjustmentConfigTarget::VRScale:
            f4vr::showNotification("Reset VR Scale body adjustment config");
            g_config.fVrScale = defaultConfig.fVrScale;
            updateVRScaleGameConfig();
            break;
        case BodyAdjustmentConfigTarget::None:
            f4vr::showNotification("Please select body adjustment to reset");
            break;
        }
    }
}
