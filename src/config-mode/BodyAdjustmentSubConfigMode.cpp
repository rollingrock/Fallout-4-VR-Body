#include "BodyAdjustmentSubConfigMode.h"

#include "Config.h"
#include "ConfigUIStyle.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/PlayerNodes.h"
#include "utils.h"
#include "vrui/BindingPrompt.h"
#include "vrui/UIManager.h"
#include "vrui/UIToggleButtonPanel.h"
#include "vrui/UIToggleGroupContainer.h"

using namespace vrui;
using namespace common;

namespace
{
    // how long a value the height adjustment just changed stays highlighted in the values panel
    constexpr uint64_t CHANGED_VALUE_HIGHLIGHT_MS = 1500;

    // as tall as the target buttons beside it; the width follows the text
    constexpr float HEIGHT_VALUES_PANEL_HEIGHT = 2.0f;

    constexpr float HEIGHT_VALUES_TEXT_HEIGHT = 0.22f;

    // the rows spread out to fill the height of the box
    constexpr float HEIGHT_VALUES_LINE_SPACING = 2.4f;

    // where a value starts, measured from the start of its row, so the three line up in a column
    constexpr float HEIGHT_VALUES_TAB_WIDTH = 2.5f;

    void updateVRScaleGameConfig()
    {
        const auto set = RE::GetINISetting("fVrScale:VR");
        set->SetFloat(frik::g_config.fVrScale);
    }
}

namespace frik
{
    BodyAdjustmentSubConfigMode::BodyAdjustmentSubConfigMode(const std::function<void()>& onClose)
        : _onClose(onClose)
    {
        createConfigUI();

        // start from the current values, so opening the screen does not highlight all of them
        trackHeightValues();
        _hmdOffsetUp.changedTime = 0;
        _bodyOffsetUp.changedTime = 0;
        _legSlackAdjustOffset.changedTime = 0;
    }

    /**
     * All the cleanup is done on destruction so dropping this sub-config (exit button, game session
     * load, etc.) always reverts unsaved values and removes the UI.
     */
    BodyAdjustmentSubConfigMode::~BodyAdjustmentSubConfigMode()
    {
        // reload config to revert unsaved values
        g_config.loadConfigOnly();

        // redo VR Scale if changed
        updateVRScaleGameConfig();

        if (_configUI) {
            g_uiManager->detachElement(_configUI, true);
        }
    }

    void BodyAdjustmentSubConfigMode::onFrameUpdate()
    {
        _configUI->setPosition(0, 0, f4vr::isNodeVisible(f4vr::getWeaponNode()) ? 6.0f : 0.0f);

        // save/reset only make sense once an adjustment target is selected
        const bool noTarget = _configTarget == BodyAdjustmentConfigTarget::None;
        _saveBtn->setDisabled(noTarget);
        _resetBtn->setDisabled(noTarget);

        handleAdjustment();

        trackHeightValues();
        updateHeightValuesPanelVisibility();
    }

    /**
     * Note which of the three values the height adjustment changes moved since the last frame. Every
     * frame, not only while height is selected, since the leg slack follows the skeleton on its own and
     * a reset changes them too.
     */
    void BodyAdjustmentSubConfigMode::trackHeightValues()
    {
        const auto now = nowMillis();
        const auto track = [now](TrackedValue& tracked, const float value) {
            if (fNotEqual(tracked.value, value)) {
                tracked.value = value;
                tracked.changedTime = now;
            }
        };
        track(_hmdOffsetUp, g_config.getPlayerHMDOffsetUp());
        track(_bodyOffsetUp, g_config.getPlayerBodyOffsetUp());
        track(_legSlackAdjustOffset, g_config.getPlayerLegSlackAdjustOffset());
    }

    /**
     * While height is the target the other three target buttons give their place in the row to the
     * height values panel: the values are what the thumbsticks are changing.
     */
    void BodyAdjustmentSubConfigMode::updateHeightValuesPanelVisibility() const
    {
        const auto adjustingHeight = _configTarget == BodyAdjustmentConfigTarget::BodyHeight;
        _heightValuesPanel->setVisibility(adjustingHeight);
        for (const auto& btn : _nonHeightTargetBtns) {
            btn->setVisibility(!adjustingHeight);
        }
    }

    /**
     * One row of the height values panel: the value's name, then the value itself at the tab stop so the
     * three line up, highlighted while it is changing.
     */
    TextRow BodyAdjustmentSubConfigMode::heightValueRow(const std::string_view label, const TrackedValue& value)
    {
        const auto highlighted = nowMillis() - value.changedTime < CHANGED_VALUE_HIGHLIGHT_MS;
        return { .spans = { { .text = std::format("{}\t", label) },
                     { .text = std::format("{:.1f}", value.value), .color = highlighted ? std::optional(render::colors::Yellow) : std::nullopt } } };
    }

    /**
     * Create all the config elements.
     */
    void BodyAdjustmentSubConfigMode::createConfigUI()
    {
        const auto playSeatedBtn = std::make_shared<UIToggleButtonPanel>("PlaySeated");
        playSeatedBtn->setTopText("PLAY");
        playSeatedBtn->setImage("vrui\\body\\play-seated.dds");
        playSeatedBtn->setBottomText("SEATED");
        playSeatedBtn->setToggleState(g_config.isPlayingSeated);
        playSeatedBtn->setOnToggleHandler([this](UIToggleButtonPanel*, const bool enabled) {
            togglePlayingSeated(enabled);
        });

        const auto hideHeadBtn = std::make_shared<UIToggleButtonPanel>("HideHeadGear");
        hideHeadBtn->setTopText("HIDE");
        hideHeadBtn->setImage("vrui\\body\\hide-head.dds");
        hideHeadBtn->setBottomText("HEAD GEAR");
        hideHeadBtn->setToggleState(g_config.hideHeadEquipment);
        hideHeadBtn->setOnToggleHandler([this](UIToggleButtonPanel*, const bool enabled) {
            toggleHideHeadEquipment(enabled);
        });

        const auto row1Container = std::make_shared<UIContainer>("Row1", UIContainerLayout::HorizontalCenter, 0.5f);
        row1Container->addElement(playSeatedBtn);
        row1Container->addElement(hideHeadBtn);

        const auto heightToggleBtn = std::make_shared<UIToggleButtonPanel>("BodyHeight");
        heightToggleBtn->setTopText("HMD / BODY");
        heightToggleBtn->setImage("vrui\\body\\body-height.dds");
        heightToggleBtn->setBottomText("UP / DOWN");
        heightToggleBtn->setOnToggleHandler([this](UIToggleButtonPanel*, const bool enabled) {
            _configTarget = enabled ? BodyAdjustmentConfigTarget::BodyHeight : BodyAdjustmentConfigTarget::None;
        });

        const auto forwardToggleBtn = std::make_shared<UIToggleButtonPanel>("BodyForward");
        forwardToggleBtn->setTopText("BODY");
        forwardToggleBtn->setImage("vrui\\body\\body-forward.dds");
        forwardToggleBtn->setBottomText("IN / OUT");
        forwardToggleBtn->setOnToggleHandler([this](UIToggleButtonPanel*, const bool enabled) {
            _configTarget = enabled ? BodyAdjustmentConfigTarget::BodyForwardOffset : BodyAdjustmentConfigTarget::None;
        });

        const auto armsLengthToggleBtn = std::make_shared<UIToggleButtonPanel>("ArmsLength");
        armsLengthToggleBtn->setTopText("ARMS");
        armsLengthToggleBtn->setImage("vrui\\body\\arms-length.dds");
        armsLengthToggleBtn->setBottomText("LENGTH");
        armsLengthToggleBtn->setOnToggleHandler([this](UIToggleButtonPanel*, const bool enabled) {
            _configTarget = enabled ? BodyAdjustmentConfigTarget::BodyArmsLength : BodyAdjustmentConfigTarget::None;
        });

        const auto vrScaleToggleBtn = std::make_shared<UIToggleButtonPanel>("VRScale");
        vrScaleToggleBtn->setTopText("VR");
        vrScaleToggleBtn->setImage("vrui\\body\\vr-scale.dds");
        vrScaleToggleBtn->setBottomText("SCALE");
        vrScaleToggleBtn->setOnToggleHandler([this](UIToggleButtonPanel*, const bool enabled) {
            _configTarget = enabled ? BodyAdjustmentConfigTarget::VRScale : BodyAdjustmentConfigTarget::None;
        });

        _row2Container = std::make_shared<UIToggleGroupContainer>("Row2", UIContainerLayout::HorizontalCenter, 0.3f);
        _row2Container->addElement(heightToggleBtn);
        _row2Container->addElement(forwardToggleBtn);
        _row2Container->addElement(armsLengthToggleBtn);
        _row2Container->addElement(vrScaleToggleBtn);

        // the toggle group disables un-toggling by default; re-enable it so the user can clear the selection back to no target
        heightToggleBtn->setUnToggleAllowed(true);
        forwardToggleBtn->setUnToggleAllowed(true);
        armsLengthToggleBtn->setUnToggleAllowed(true);
        vrScaleToggleBtn->setUnToggleAllowed(true);

        _nonHeightTargetBtns = { forwardToggleBtn, armsLengthToggleBtn, vrScaleToggleBtn };

        // no bottom padding: the last row's descender room already keeps it off the bottom edge
        auto valuesStyle = F4VR_PANEL_STYLE;
        valuesStyle.padding = { .top = 0.3f, .right = 0.3f, .bottom = 0.0f, .left = 0.3f };
        _heightValuesPanel = std::make_shared<UITextPanel>("HeightValues", 0.0f, HEIGHT_VALUES_PANEL_HEIGHT);
        _heightValuesPanel->setSizing(UIPanelSizing::FixedHeight);
        _heightValuesPanel->setStyle(valuesStyle);
        _heightValuesPanel->setTextHeight(HEIGHT_VALUES_TEXT_HEIGHT);
        _heightValuesPanel->setLineSpacing(HEIGHT_VALUES_LINE_SPACING);
        _heightValuesPanel->setTabWidth(HEIGHT_VALUES_TAB_WIDTH);
        _heightValuesPanel->setContent([this](std::vector<TextRow>& rows) {
            rows.push_back(heightValueRow("HMD HEIGHT", _hmdOffsetUp));
            rows.push_back(heightValueRow("BODY OFFSET", _bodyOffsetUp));
            rows.push_back(heightValueRow("LEG SLACK", _legSlackAdjustOffset));
        });

        updateHeightValuesPanelVisibility();

        // the target buttons and the values panel share the row, the three non-height buttons or the panel showing
        const auto row2WithValuesContainer = std::make_shared<UIContainer>("Row2WithValues", UIContainerLayout::HorizontalCenter, 0.3f);
        row2WithValuesContainer->addElement(_row2Container);
        row2WithValuesContainer->addElement(_heightValuesPanel);

        _saveBtn = std::make_shared<UIButtonPanel>("Save");
        _saveBtn->setImage("f4cf\\vrui\\save.DDS");
        _saveBtn->setBottomText("SAVE");
        _saveBtn->setOnPressHandler([this](UIButtonPanel*) {
            saveConfig();
        });

        _resetBtn = std::make_shared<UIButtonPanel>("Reset");
        _resetBtn->setImage("f4cf\\vrui\\reset.DDS");
        _resetBtn->setBottomText("RESET");
        _resetBtn->setOnPressHandler([this](UIButtonPanel*) {
            resetConfig();
        });

        const auto backBtn = std::make_shared<UIButtonPanel>("Back");
        backBtn->setImage("f4cf\\vrui\\exit.DDS");
        backBtn->setBottomText("BACK");
        backBtn->setOnPressHandler([this](UIButtonPanel*) {
            closeConfig();
        });

        const auto row3Container = std::make_shared<UIContainer>("Row3", UIContainerLayout::HorizontalCenter, 0.3f);
        row3Container->addElement(_saveBtn);
        row3Container->addElement(_resetBtn);
        row3Container->addElement(backBtn);

        // One footer whose rows follow the current target. Its rows are only asked for while the UI is
        // attached, and the UI is detached before this sub-config goes.
        const auto footer = std::make_shared<UITextPanel>("Footer");
        footer->setStyle(F4VR_PANEL_STYLE);
        footer->setTextHeight(CONFIG_FOOTER_TEXT_HEIGHT);
        footer->setContent([this](std::vector<TextRow>& rows) {
            appendFooterRows(rows);
        });

        // the selfie toggle works on every target, so it has its own box beside the footer
        const auto row4Container = std::make_shared<UIContainer>("Row4", UIContainerLayout::HorizontalCenter, 0.3f);
        row4Container->addElement(footer);
        row4Container->addElement(createToggleSelfieMsg(footer));

        const auto header = std::make_shared<UITextPanel>("Title");
        header->setStyle(UIPanelStyle{ .color = F4VR_PANEL_STYLE.color });
        header->setTextHeight(0.5f);
        header->setContent([](std::vector<TextRow>& rows) {
            rows.emplace_back("FRIK: BODY ADJUSTMENT", std::nullopt, render::TextDecoration::Underline);
        });

        _configUI = std::make_shared<UIContainer>("BodyAdjustConfig", UIContainerLayout::VerticalUp, 0.35f, 1.7f);
        _configUI->addElement(row4Container);
        _configUI->addElement(row3Container);
        _configUI->addElement(row2WithValuesContainer);
        _configUI->addElement(row1Container);
        _configUI->addElement(header);

        g_uiManager->attachPresetToPrimaryWandTop(_configUI, { 0, 0, 0 });
    }

    /**
     * Toggle seated play.
     */
    void BodyAdjustmentSubConfigMode::togglePlayingSeated(const bool seated)
    {
        g_config.saveIsPlayingSeated(seated);
        clearConfigTarget();
    }

    /**
     * Toggle head hiding on\off.
     * Show notification regarding not hiding in selfie mode for player not to be confused.
     */
    void BodyAdjustmentSubConfigMode::toggleHideHeadEquipment(const bool hide)
    {
        g_config.saveHideHeadEquipment(hide);
        clearConfigTarget();
        if (hide) {
            std::string msg = "Player head equipment is now hidden";
            if (g_config.selfieIgnoreHideFlags) {
                msg = msg + "\nNote: The head equipment is NOT hidden in selfie mode!";
            }
            f4vr::showNotification(msg);
        }
    }

    /**
     * On close of the body adjustment UI notify the parent config, which drops this sub-config.
     * The actual cleanup happens in the destructor.
     */
    void BodyAdjustmentSubConfigMode::closeConfig()
    {
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

    /**
     * The footer's rows for the current target: what the adjustment is for and the sticks that drive it
     * (see the handle*Adjustment functions).
     */
    void BodyAdjustmentSubConfigMode::appendFooterRows(std::vector<TextRow>& rows) const
    {
        const auto stick = [](const vrcf::Hand hand) {
            return vrcf::InputBinding{ .hand = hand, .button = vr::k_EButton_SteamVR_Touchpad };
        };

        switch (_configTarget) {
        case BodyAdjustmentConfigTarget::None:
            rows.emplace_back("BODY ADJUSTMENTS ARE SAVED SEPARATELY");
            rows.emplace_back("FOR SEATED / STANDING MODE");
            rows.emplace_back("AND IN / OUT OF POWER ARMOR");
            break;
        case BodyAdjustmentConfigTarget::BodyHeight:
            rows.push_back({ .spans = { { .text = "HMD UP / DOWN:  " } } });
            appendBindingPrompt(rows.back().spans, stick(vrcf::Hand::Primary), CONFIG_FOOTER_PROMPT_STYLE);
            rows.back().spans.push_back({ .text = "  BODY UP / DOWN:  " });
            appendBindingPrompt(rows.back().spans, stick(vrcf::Hand::Offhand), CONFIG_FOOTER_PROMPT_STYLE);
            rows.emplace_back("ADJUST BODY SO FEET ARE ON THE GROUND");
            rows.emplace_back("THEN ADJUST HMD TO BE AT PLAYER EYE LEVEL");
            break;
        case BodyAdjustmentConfigTarget::BodyForwardOffset:
            rows.push_back({ .spans = { { .text = "MOVE BODY FORWARD / BACKWARDS:  " } } });
            appendBindingPrompt(rows.back().spans, stick(vrcf::Hand::Primary), CONFIG_FOOTER_PROMPT_STYLE);
            rows.emplace_back("SET THE BODY JUST UNDER YOU");
            rows.emplace_back("TRY HIDE HEAD GEAR TO REMOVE CLIPPING");
            break;
        case BodyAdjustmentConfigTarget::BodyArmsLength:
            rows.push_back({ .spans = { { .text = "ADJUST ARMS LENGTH:  " } } });
            appendBindingPrompt(rows.back().spans, stick(vrcf::Hand::Primary), CONFIG_FOOTER_PROMPT_STYLE);
            break;
        case BodyAdjustmentConfigTarget::VRScale:
            rows.push_back({ .spans = { { .text = "ADJUST VR SCALE:  " } } });
            appendBindingPrompt(rows.back().spans, stick(vrcf::Hand::Primary), CONFIG_FOOTER_PROMPT_STYLE);
            rows.emplace_back("AFFECTS THE GAME VR SCALE");
            rows.emplace_back("MAKING THE WORLD BIGGER / SMALLER");
            break;
        }
    }

    void BodyAdjustmentSubConfigMode::handleHeightAdjustment()
    {
        const auto primAxisY = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary).y;
        g_config.setPlayerHMDOffsetUp(g_config.getPlayerHMDOffsetUp() + correctAdjustmentValue(primAxisY, 4));
        g_config.setPlayerBodyOffsetUp(g_config.getPlayerBodyOffsetUp() + 0.2f * correctAdjustmentValue(primAxisY, 4));

        const auto offAxisY = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Offhand).y;
        g_config.setPlayerBodyOffsetUp(g_config.getPlayerBodyOffsetUp() - correctAdjustmentValue(offAxisY, 4));
        g_config.setPlayerLegSlackAdjustOffset(g_config.getPlayerLegSlackAdjustOffset() + 3 * correctAdjustmentValue(offAxisY, 4));

        if (_skeletonLegSlack > g_config.skeletonLegSlackTarget + 0.1f || _skeletonLegSlack < g_config.skeletonLegSlackTarget - 0.1f) {
            logger::debug("Update player leg slack adjust offset due to skeleton leg slack value out of optimal: {}, adjust: {}",
                _skeletonLegSlack,
                g_config.getPlayerLegSlackAdjustOffset());
            g_config.setPlayerLegSlackAdjustOffset(g_config.getPlayerLegSlackAdjustOffset() + (_skeletonLegSlack > g_config.skeletonLegSlackTarget + 0.1f ? -0.1f : 0.1f));
        }
    }

    void BodyAdjustmentSubConfigMode::handleForwardAdjustment()
    {
        const auto axisY = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary).y;
        g_config.setPlayerBodyOffsetForward(g_config.getPlayerBodyOffsetForward() + correctAdjustmentValue(axisY, 4));
    }

    void BodyAdjustmentSubConfigMode::handleArmsLengthAdjustment()
    {
        const auto axisY = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary).y;
        g_config.armLength += correctAdjustmentValue(axisY, 5);
    }

    void BodyAdjustmentSubConfigMode::handleVRScaleAdjustment()
    {
        const auto axisY = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary).y;
        g_config.fVrScale += correctAdjustmentValue(axisY, 5);
        updateVRScaleGameConfig();
    }

    void BodyAdjustmentSubConfigMode::saveConfig()
    {
        f4vr::showNotification("Saving all body adjustment configs");
        g_config.save();
        clearConfigTarget();
    }

    /**
     * Reset the current config adjusting value to the default in the embedded config.
     * Use a small hack to load ONLY the embedded config into a temp config object.
     * The value is only applied live, keep the target selected so Save stays enabled to persist it.
     */
    void BodyAdjustmentSubConfigMode::resetConfig()
    {
        Config defaultConfig;
        defaultConfig.loadEmbeddedDefaultOnly();
        defaultConfig.isPlayingSeated = g_config.isPlayingSeated;

        switch (_configTarget) {
        case BodyAdjustmentConfigTarget::BodyHeight:
            f4vr::showNotification("Reset height body adjustment config");
            g_config.setPlayerHMDOffsetUp(defaultConfig.getPlayerHMDOffsetUp());
            g_config.setPlayerLegSlackAdjustOffset(defaultConfig.getPlayerLegSlackAdjustOffset());
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

    void BodyAdjustmentSubConfigMode::clearConfigTarget()
    {
        _configTarget = BodyAdjustmentConfigTarget::None;
        _row2Container->clearToggleState();
    }
}
