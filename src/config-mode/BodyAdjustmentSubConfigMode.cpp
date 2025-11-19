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
        handleAdjustment();

        // save
        if (f4vr::VRControllers.isLongPressed(f4vr::Hand::Primary, vr::EVRButtonId::k_EButton_A)) {
            f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, .6f, .5f);
            saveConfig();
        }
    }

    /**
     * Create all the config elements.
     */
    void BodyAdjustmentSubConfigMode::createConfigUI()
    {
        const auto heightToggleBtn = std::make_shared<UIToggleButton>("FRIK/ui_main_conf_btn_body_vertical.nif");
        heightToggleBtn->setToggleState(true);
        heightToggleBtn->setOnToggleHandler([this](UIWidget*, bool) { _configTarget = BodyAdjustmentConfigTarget::BodyHeight; });

        const auto forwardToggleBtn = std::make_shared<UIToggleButton>("FRIK/ui_main_conf_btn_body_forward.nif");
        forwardToggleBtn->setOnToggleHandler([this](UIWidget*, bool) { _configTarget = BodyAdjustmentConfigTarget::BodyForwardOffset; });

        const auto armsLengthToggleBtn = std::make_shared<UIToggleButton>("FRIK/ui_main_conf_btn_arms_length.nif");
        armsLengthToggleBtn->setOnToggleHandler([this](UIWidget*, bool) { _configTarget = BodyAdjustmentConfigTarget::BodyArmsLength; });

        const auto vrScaleToggleBtn = std::make_shared<UIToggleButton>("FRIK/ui_main_conf_btn_vr_scale.nif");
        vrScaleToggleBtn->setOnToggleHandler([this](UIWidget*, bool) { _configTarget = BodyAdjustmentConfigTarget::VRScale; });

        const auto row1Container = std::make_shared<UIToggleGroupContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.3f);
        row1Container->addElement(heightToggleBtn);
        row1Container->addElement(forwardToggleBtn);
        row1Container->addElement(armsLengthToggleBtn);
        row1Container->addElement(vrScaleToggleBtn);

        const auto saveBtn = std::make_shared<UIButton>("FRIK/ui_common_btn_save.nif");
        saveBtn->setOnPressHandler([this](UIWidget*) { saveConfig(); });

        const auto resetBtn = std::make_shared<UIButton>("FRIK/ui_common_btn_reset.nif");
        resetBtn->setOnPressHandler([this](UIWidget*) { resetConfig(); });

        const auto exitBtn = std::make_shared<UIButton>("FRIK/ui_common_btn_back.nif");
        exitBtn->setOnPressHandler([this](UIWidget*) { closeConfig(); });

        const auto row2Container = std::make_shared<UIContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);
        row2Container->addElement(saveBtn);
        row2Container->addElement(resetBtn);
        row2Container->addElement(exitBtn);

        const auto header = std::make_shared<UIWidget>("FRIK/ui_main_conf_title_body_adjust.nif", 0.4f);

        _configUI = std::make_shared<UIContainer>("BodyAdjustConfig", UIContainerLayout::VerticalDown, 0.4f, 1.8f);
        _configUI->addElement(header);
        _configUI->addElement(row1Container);
        _configUI->addElement(row2Container);

        g_uiManager->attachPresetToPrimaryWandLeft(_configUI, { -1, 4, 0 });
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
        }
    }

    void BodyAdjustmentSubConfigMode::handleHeightAdjustment()
    {
        const auto primAxisY = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Primary).y;
        g_config.setPlayerHMDOffsetUp(g_config.getPlayerHMDOffsetUp() + correctAdjustmentValue(primAxisY, 4));

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
            g_config.fVrScale += defaultConfig.fVrScale;
            updateVRScaleGameConfig();
            break;
        }
    }
}
