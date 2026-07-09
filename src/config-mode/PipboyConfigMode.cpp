#include "PipboyConfigMode.h"

#include "Config.h"
#include "FRIK.h"
#include "common/MatrixUtils.h"
#include "f4vr/F4VRUtils.h"
#include "skeleton/Skeleton.h"
#include "utils.h"
#include "vrcf/VRControllersHaptic.h"
#include "vrcf/VRControllersManager.h"
#include "vrcf/VRControllersSuppressor.h"
#include "vrui/UIButton.h"
#include "vrui/UIManager.h"
#include "vrui/UIMultiStateToggleButton.h"
#include "vrui/UIToggleButton.h"
#include "vrui/UIWidget.h"

using namespace vrui;
using namespace common;

namespace frik
{
    // owner key for suppressing the thumbstick from the game while adjusting, so it doesn't also drive the Pipboy menus
    constexpr std::string_view PIPBOY_CONFIG_SUPPRESS_KEY = "frik.pipboy_config";

    /**
     * On release, detach the UI from the global manager if it is still attached.
     */
    PipboyConfigMode::~PipboyConfigMode()
    {
        if (_configUI) {
            g_uiManager->detachElement(_configUI, true);
        }
    }

    /**
     * Open Pipboy configuration mode (used from the main config menu / Papyrus).
     */
    void PipboyConfigMode::openPipboyConfigurationMode()
    {
        enterPipboyConfigMode();
    }

    void PipboyConfigMode::onFrameUpdate()
    {
        if (!g_frik.isPipboyOn() && f4vr::isPipboyOnWrist()) {
            return;
        }

        // Enter Pipboy Config Mode by long-pressing the configured binding (favorites button by default).
        if (!isPipBoyConfigModeActive() && f4vr::isPipboyOnWrist() && g_frik.isPipboyOn() && vrcf::VRControllers.check(g_config.enterPipboyConfigBinding)) {
            enterPipboyConfigMode();
        }

        if (!isPipBoyConfigModeActive()) {
            return;
        }

        // push the panel forward when a weapon is drawn so it doesn't overlap, like the other wand-top configs
        _configUI->setPosition(0, 0, f4vr::isNodeVisible(f4vr::getWeaponNode()) ? 6.0f : 0.0f);

        // while adjusting, hide controllers from the game so it doesn't also navigate the Pipboy menus
        const bool adjusting = _adjustTarget != PipboyAdjustTarget::None;
        vrcf::VRControllersSuppress.setAllSuppressed(PIPBOY_CONFIG_SUPPRESS_KEY, adjusting);

        // save/reset are only relevant while adjusting a target; disable (rather than hide) them otherwise, exit stays always enabled
        _saveBtn->setDisabled(!adjusting);
        _resetBtn->setDisabled(!adjusting);

        // when the Pipboy isn't on the wrist, only model scale is relevant: hide the rest and show the message
        const bool onWrist = f4vr::isPipboyOnWrist();
        _notOnWristMsg->setVisibility(!onWrist);
        _adjustScreenBtn->setVisibility(onWrist);
        _swapModelBtn->setVisibility(onWrist);
        _row2Container->setVisibility(onWrist);

        // show the footer matching the current adjust target
        _footerMain->setVisibility(_adjustTarget == PipboyAdjustTarget::None);
        _footerScreenAdjust->setVisibility(_adjustTarget == PipboyAdjustTarget::ScreenAdjust);
        _footerModelScale->setVisibility(_adjustTarget == PipboyAdjustTarget::ModelScale);

        handleAdjustment();

        // save on long press of the primary A button (same shortcut as weapon reposition mode)
        if (vrcf::VRControllers.isLongPressed(vrcf::Hand::Primary, vr::EVRButtonId::k_EButton_A)) {
            if (saveConfig()) {
                vrcf::VRHaptics.trigger(vrcf::Hand::Primary, vrcf::HapticPattern::Success);
            }
        }
    }

    /**
     * Enter Pipboy config mode: close the favorites menu, give haptic feedback, and build the UI.
     */
    void PipboyConfigMode::enterPipboyConfigMode()
    {
        if (isPipBoyConfigModeActive()) {
            return;
        }
        if (g_frik.isFavoritesMenuOpen()) {
            f4vr::closeFavoriteMenu();
        }
        vrcf::VRHaptics.trigger(vrcf::Hand::Primary, vrcf::HapticPattern::RampUp);
        createConfigUI();
    }

    /**
     * Exit Pipboy config mode: remove the UI, restore the Pipboy model scale if it was changed but not
     * saved, and close the Pipboy if it's still open (config mode is done with it).
     */
    void PipboyConfigMode::exitPBConfig()
    {
        if (!isPipBoyConfigModeActive()) {
            return;
        }

        g_uiManager->detachElement(_configUI, true);
        _configUI.reset();
        _adjustModeGroup.reset();
        _adjustTarget = PipboyAdjustTarget::None;

        // stop suppressing now that we're no longer adjusting
        vrcf::VRControllersSuppress.release(PIPBOY_CONFIG_SUPPRESS_KEY);

        // restore pipboy scale if it was changed
        if (const auto pipboyModel = getPipboyModelNode()) {
            pipboyModel->local.scale = g_config.pipBoyScale;
        }

        // close the Pipboy if it was open; skipped when this exit was triggered by the Pipboy closing
        if (g_frik.isPipboyOn()) {
            g_frik.closePipboy();
        }
    }

    /**
     * Read the player thumbstick and adjust the currently selected target.
     */
    void PipboyConfigMode::handleAdjustment() const
    {
        switch (_adjustTarget) {
        case PipboyAdjustTarget::ScreenAdjust:
            handleScreenAdjust();
            break;
        case PipboyAdjustTarget::ModelScale:
            handleModelScaleAdjust();
            break;
        case PipboyAdjustTarget::None:
            break;
        }
    }

    /**
     * Adjust the holo-screen with a single mode driven by input combinations, like the weapon reposition mode:
     * - default: move (primary stick for two axes, offhand stick for depth)
     * - hold offhand grip: rotate (pitch/roll by primary stick, yaw by offhand stick)
     * - hold offhand A: scale
     */
    void PipboyConfigMode::handleScreenAdjust()
    {
        const auto [primAxisX, primAxisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary);
        const auto [secAxisX, secAxisY] = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Offhand);
        if (primAxisX == 0.f && primAxisY == 0.f && secAxisX == 0.f && secAxisY == 0.f) {
            return;
        }

        auto& transform = f4vr::getPlayerNodes()->ScreenNode->local;
        if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_A)) {
            // adjust the scale of the screen
            transform.scale = std::fmax(0.1f, transform.scale + correctAdjustmentValue(primAxisY, 100));
        } else if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, vr::EVRButtonId::k_EButton_Grip)) {
            // pitch and roll rotation by primary stick, yaw rotation by offhand stick
            const auto rot = MatrixUtils::getMatrixFromEulerAngles(-MatrixUtils::degreesToRads(correctAdjustmentValue(primAxisY, 5)),
                -MatrixUtils::degreesToRads(correctAdjustmentValue(secAxisY, 5)),
                MatrixUtils::degreesToRads(correctAdjustmentValue(primAxisX, 5)));
            transform.rotate = rot * transform.rotate;
        } else {
            // move horizontally by primary stick and in depth by offhand stick
            transform.translate.x += correctAdjustmentValue(primAxisX, 12);
            transform.translate.y -= correctAdjustmentValue(primAxisY, 12);
            transform.translate.z -= correctAdjustmentValue(secAxisY, 12);
        }
    }

    /**
     * Adjust the 3rd-person Pipboy model scale by the primary stick.
     */
    void PipboyConfigMode::handleModelScaleAdjust() const
    {
        const auto primAxisY = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary).y;
        if (primAxisY == 0.f) {
            return;
        }
        if (const auto pipboyModel = getPipboyModelNode()) {
            pipboyModel->local.scale += correctAdjustmentValue(primAxisY, 65);
        }
    }

    /**
     * Save the currently-adjusted target (screen offset or model scale) to config.
     * Save is only enabled while a target is being adjusted; returns true if anything was saved.
     */
    bool PipboyConfigMode::saveConfig() const
    {
        switch (_adjustTarget) {
        case PipboyAdjustTarget::ScreenAdjust:
            g_config.savePipboyOffset(f4vr::getPlayerNodes()->ScreenNode->local);
            f4vr::showNotification("Saved Pipboy screen position");
            return true;
        case PipboyAdjustTarget::ModelScale:
            if (const auto pipboyModel = getPipboyModelNode()) {
                // 3rd person Pipboy is null for Fallout London as there is no Pipboy on the arm
                g_config.savePipboyScale(pipboyModel->local.scale);
            }
            f4vr::showNotification("Saved Pipboy model scale");
            return true;
        case PipboyAdjustTarget::None:
            return false;
        }
        return false;
    }

    /**
     * Reset the currently-adjusted target (screen offset or model scale) to its factory default
     * (applied live; press Save to persist). Reset is only enabled while a target is being adjusted.
     */
    void PipboyConfigMode::resetConfig() const
    {
        switch (_adjustTarget) {
        case PipboyAdjustTarget::ScreenAdjust:
            f4vr::showNotification("Reset Pipboy screen position to default");
            f4vr::getPlayerNodes()->ScreenNode->local = g_config.getDefaultPipboyOffset();
            break;
        case PipboyAdjustTarget::ModelScale:
            f4vr::showNotification("Reset Pipboy model scale to default");
            if (const auto pipboyModel = getPipboyModelNode()) {
                pipboyModel->local.scale = 1.0f;
            }
            break;
        case PipboyAdjustTarget::None:
            break;
        }
    }

    /**
     * Toggle "open Pipboy when looking at it" and, when enabling, notify with the configured look-at delay.
     */
    void PipboyConfigMode::onOpenWhenLookAtToggled(const bool on)
    {
        g_config.togglePipBoyOpenWhenLookAt();
        if (on) {
            f4vr::showNotification(std::format("Pipboy will open automatically after looking at it for {:.1f} seconds", g_config.pipBoyOnDelay / 1000.f));
        }
    }

    /**
     * Toggle "close Pipboy when looking away" and, when enabling, notify with the configured look-away delay.
     */
    void PipboyConfigMode::onCloseWhenLookAwayToggled(const bool on)
    {
        g_config.togglePipBoyCloseWhenLookAway();
        if (on) {
            f4vr::showNotification(std::format("Pipboy will close automatically after looking away for {:.1f} seconds", g_config.pipBoyOffDelay / 1000.f));
        }
    }

    /**
     * Show a notification describing the new dampen-screen mode (only on the non-None modes, as in the original).
     */
    void PipboyConfigMode::onDampenScreenModeChanged()
    {
        if (g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::Movement) {
            f4vr::showNotification("Dampen Pipboy screen by smoothing the movement");
        } else if (g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::HoldInPlace) {
            f4vr::showNotification("Dampen Pipboy screen by holding it in place where opened.\nHold Pipboy hand grip to move the screen with the arm.");
        }
    }

    /**
     * Get the 3rd-person Pipboy model node (on the arm the Pipboy is worn on), or null if not present
     * (e.g. Fallout London has no Pipboy on the arm).
     */
    RE::NiAVObject* PipboyConfigMode::getPipboyModelNode() const
    {
        const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
        return arm.forearm3 ? f4vr::findAVObject(arm.forearm3, "PipboyBone") : nullptr;
    }

    /**
     * Create the configuration UI and attach it above the primary wand.
     */
    void PipboyConfigMode::createConfigUI()
    {
        // Row of mutually-exclusive adjustment-mode toggles; the selected one drives the thumbstick in handleAdjustment().
        // Screen move/rotate/scale are all done from the single "Adjust Screen" mode via stick + button combinations.
        _adjustScreenBtn = std::make_shared<UIToggleButton>("ui-config-pipboy\\btn-adjust-screen.nif");
        _adjustScreenBtn->setOnToggleHandler([this](UIWidget*, const bool on) {
            _adjustTarget = on ? PipboyAdjustTarget::ScreenAdjust : PipboyAdjustTarget::None;
        });

        const auto scaleModelBtn = std::make_shared<UIToggleButton>("ui-config-pipboy\\btn-scale-model.nif");
        scaleModelBtn->setOnToggleHandler([this](UIWidget*, const bool on) {
            _adjustTarget = on ? PipboyAdjustTarget::ModelScale : PipboyAdjustTarget::None;
        });

        _adjustModeGroup = std::make_shared<UIToggleGroupContainer>("AdjustModes", UIContainerLayout::HorizontalCenter, 0.3f);
        _adjustModeGroup->addElement(_adjustScreenBtn);
        _adjustModeGroup->addElement(scaleModelBtn);
        // allow unselecting the active mode (the group disables un-toggling by default for radio behavior)
        _adjustScreenBtn->setUnToggleAllowed(true);
        scaleModelBtn->setUnToggleAllowed(true);

        _swapModelBtn = std::make_shared<UIButton>("ui-config-pipboy\\btn-swap-model.nif");
        _swapModelBtn->setOnPressHandler([this](UIWidget*) {
            if (!g_config.isFalloutLondonVR) {
                g_frik.swapPipboyModel();
                // swapping reloads the screen offset for the new model, so any active adjust selection is now stale
                _adjustModeGroup->clearToggleState();
                _adjustTarget = PipboyAdjustTarget::None;
            }
        });

        // Shown to the left of the mode group when the Pipboy isn't on the wrist (only model scale stays relevant).
        _notOnWristMsg = std::make_shared<UIWidget>("ui-config-pipboy\\msg-not-on-wrist.nif");
        _notOnWristMsg->setVisibility(false);

        // First row: not-on-wrist message, the adjust-mode toggle group, then the swap-model action button.
        const auto row1Container = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.3f);
        row1Container->addElement(_notOnWristMsg);
        row1Container->addElement(_adjustModeGroup);
        row1Container->addElement(_swapModelBtn);

        // Row of independent toggles.
        const auto glanceBtn = std::make_shared<UIToggleButton>("ui-config-pipboy\\btn-open-look-at.nif");
        glanceBtn->setToggleState(g_config.pipboyOpenWhenLookAt);
        glanceBtn->setOnToggleHandler([](UIWidget*, const bool on) {
            onOpenWhenLookAtToggled(on);
        });

        const auto closeLookAwayBtn = std::make_shared<UIToggleButton>("ui-config-pipboy\\btn-close-look-away.nif");
        closeLookAwayBtn->setToggleState(g_config.pipboyCloseWhenLookAway);
        closeLookAwayBtn->setOnToggleHandler([](UIWidget*, const bool on) {
            onCloseWhenLookAwayToggled(on);
        });

        const auto dampenModesMap = std::map<DampenPipboyScreenMode, std::string>{ { DampenPipboyScreenMode::None, "ui-config-pipboy\\btn-dampen-off.nif" },
            { DampenPipboyScreenMode::Movement, "ui-config-pipboy\\btn-dampen-smooth.nif" },
            { DampenPipboyScreenMode::HoldInPlace, "ui-config-pipboy\\btn-dampen-hold.nif" } };
        const auto dampenBtn = std::make_shared<UIMultiStateToggleButton<DampenPipboyScreenMode>>(dampenModesMap);
        dampenBtn->setState(g_config.dampenPipboyScreenMode);
        dampenBtn->setOnStateChangedHandler([this](UIWidget*, const DampenPipboyScreenMode) {
            g_config.toggleDampenPipboyScreen();
            onDampenScreenModeChanged();
        });

        _row2Container = std::make_shared<UIContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);
        _row2Container->addElement(glanceBtn);
        _row2Container->addElement(closeLookAwayBtn);
        _row2Container->addElement(dampenBtn);

        // Row of save / reset / exit. Save and reset are only enabled while actively adjusting a target
        // (toggled each frame in onFrameUpdate); exit is always available.
        _saveBtn = std::make_shared<UIButton>("ui-common\\btn-save.nif");
        _saveBtn->setOnPressHandler([this](UIWidget*) {
            saveConfig();
        });
        _saveBtn->setDisabled(true);

        _resetBtn = std::make_shared<UIButton>("ui-common\\btn-reset.nif");
        _resetBtn->setOnPressHandler([this](UIWidget*) {
            resetConfig();
        });
        _resetBtn->setDisabled(true);

        const auto exitBtn = std::make_shared<UIButton>("ui-common\\btn-exit.nif");
        exitBtn->setOnPressHandler([this](UIWidget*) {
            exitPBConfig();
        });

        const auto row3Container = std::make_shared<UIContainer>("Row3", UIContainerLayout::HorizontalCenter, 0.3f);
        row3Container->addElement(_saveBtn);
        row3Container->addElement(_resetBtn);
        row3Container->addElement(exitBtn);

        const auto header = std::make_shared<UIWidget>("ui-config-pipboy\\title.nif", 1.7f);

        // Footers - only one visible at a time based on the current adjust target (updated each frame in onFrameUpdate).
        _footerMain = std::make_shared<UIWidget>("ui-config-pipboy\\msg-footer-main.nif");
        _footerScreenAdjust = std::make_shared<UIWidget>("ui-config-pipboy\\msg-footer-adjust.nif");
        _footerModelScale = std::make_shared<UIWidget>("ui-config-pipboy\\msg-footer-adjust-simple.nif");
        _footerScreenAdjust->setVisibility(false);
        _footerModelScale->setVisibility(false);

        const auto footerContainer = std::make_shared<UIContainer>("Footer", UIContainerLayout::HorizontalCenter, 0.3f);
        footerContainer->addElement(_footerMain);
        footerContainer->addElement(_footerScreenAdjust);
        footerContainer->addElement(_footerModelScale);

        _configUI = std::make_shared<UIContainer>("PipboyConfig", UIContainerLayout::VerticalUp, 0.35f, 1.7f);
        _configUI->addElement(footerContainer);
        _configUI->addElement(row3Container);
        _configUI->addElement(_row2Container);
        _configUI->addElement(row1Container);
        _configUI->addElement(header);

        g_uiManager->attachPresetToPrimaryWandTop(_configUI, { 0, 0, 0 });
    }
}
