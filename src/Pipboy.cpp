#include "Pipboy.h"

#include <chrono>
#include <thread>

#include "Config.h"
#include "FRIK.h"
#include "HandPose.h"
#include "utils.h"
#include "common/Logger.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/scaleformUtils.h"
#include "f4vr/VRControllersManager.h"

using namespace RE::Scaleform;
using namespace std::chrono;
using namespace common;

namespace
{
    bool isPrimaryTriggerPressed()
    {
        return f4vr::VRControllers.isPressed(vr::k_EButton_SteamVR_Trigger, f4vr::Hand::Primary);
    }

    bool isAButtonPressed()
    {
        return f4vr::VRControllers.isPressed(vr::k_EButton_A, f4vr::Hand::Primary);
    }

    bool isBButtonPressed()
    {
        return f4vr::VRControllers.isPressed(vr::k_EButton_ApplicationMenu, f4vr::Hand::Primary);
    }

    bool isPrimaryGripPressHeldDown()
    {
        return f4vr::VRControllers.isPressHeldDown(vr::k_EButton_Grip, f4vr::Hand::Primary);
    }

    bool isWorldMapVisible(const GFx::Movie* root)
    {
        return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.WorldMapHolder_mc");
    }

    std::string getCurrentMapPath(const GFx::Movie* root, const std::string& suffix = "")
    {
        return (isWorldMapVisible(root) ? "root.Menu_mc.CurrentPage.WorldMapHolder_mc" : "root.Menu_mc.CurrentPage.LocalMapHolder_mc") + suffix;
    }

    /**
     * Is context menu message box popup is visible or not. Only one can be visible at a time.
     */
    bool isMessageHolderVisible(const GFx::Movie* root)
    {
        return root &&
        (f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.MessageHolder_mc")
            || f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc"));
    }

    bool isQuestTabVisibleOnDataPage(const GFx::Movie* root)
    {
        return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.QuestsTab_mc");
    }

    bool isQuestTabObjectiveListEnabledOnDataPage(const GFx::Movie* root)
    {
        GFx::Value var;
        return root->GetVariable(&var, "root.Menu_mc.CurrentPage.QuestsTab_mc.ObjectivesList_mc.selectedIndex") && var.GetType() == GFx::Value::ValueType::kInt && var.GetInt() > -
            1;
    }

    bool isWorkshopsTabVisibleOnDataPage(const GFx::Movie* root)
    {
        return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc");
    }

    GFx::Movie* getPipboyMenuRootMovie()
    {
        const auto menu = RE::UI::GetSingleton()->GetMenu("PipboyMenu");
        return menu ? menu->uiMovie.get() : nullptr;
    }
}

namespace frik
{
    /**
     * Turn Pipboy On/Off.
     */
    void Pipboy::setOnOff(const bool setOn)
    {
        if (_isOnStatus == setOn) {
            return;
        }
        _isOnStatus = setOn;
        _stickyTurnOff = false;
        if (setOn) {
            f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = 1.0;
            turnPipBoyOn();
        } else {
            turnPipBoyOff();
            g_frik.closePipboyConfigurationModeActive();
            f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = 0.0;
        }
    }

    /**
     * Is the player currently looking at the Pipboy screen?
     * Handle different thresholds if Pipboy is on or off as looking away is more relaxed threshold.
     */
    bool Pipboy::isLookingAtPipBoy() const
    {
        const auto pipboy = f4vr::findAVObject(f4vr::getPlayerNodes()->SecondaryWandNode, "PipboyRoot_NIF_ONLY");
        const auto screen = pipboy ? f4vr::findAVObject(pipboy, "Screen:0") : nullptr;
        if (screen == nullptr) {
            return false;
        }

        const float threshhold = _isOnStatus ? g_config.pipboyLookAwayThreshold : g_config.pipboyLookAtThreshold;
        return isCameraLookingAtObject(f4vr::getPlayerCamera()->cameraNode, screen, threshhold);
    }

    /**
     * Run Pipboy mesh replacement if not already done (or forced) to the configured meshes either holo or screen.
     * @param force true - run mesh replace, false - only if not previously replaced
     */
    void Pipboy::replaceMeshes(const bool force)
    {
        if (force || !_meshesReplaced) {
            if (g_config.isHoloPipboy == 0) {
                replaceMeshes("HoloEmitter", "Screen");
            } else if (g_config.isHoloPipboy == 1) {
                replaceMeshes("Screen", "HoloEmitter");
            }
        }
    }

    /**
     * Executed every frame to update the Pipboy location and handle interaction with pipboy config UX.
     * TODO: refactor into separate functions for each functionality
     */
    void Pipboy::onFrameUpdate()
    {
        replaceMeshes(false);

        checkTurningOnByButton();
        checkTurningOffByButton();

        checkTurningOnByLookingAt();
        checkTurningOffByLookingAway();

        checkSwitchingFlashlightHeadHand();

        storeLastPipboyPage();

        const auto pipboyRoot = getPipboyMenuRootMovie();
        _physicalHandler.operate(_lastPipboyPage, isMessageHolderVisible(pipboyRoot));

        if (f4vr::isInPowerArmor()) {
            lastRadioFreq = 0.0; // Ensures radio needle doesn't get messed up when entering and then exiting Power Armor.
            return;
        }

        if (_isOnStatus && pipboyRoot) {
            if (g_config.enablePrimaryControllerPipboyUse && !g_frik.isPipboyConfigurationModeActive()) {
                // Use primary hand controller to operate Pipboy
                handlePrimaryControllerThumbstickOperation(pipboyRoot);
                handlePrimaryControllerButtonsOperation(pipboyRoot, false);
            }

            dampenPipboyScreen();
        }

        // Hide some Pipboy related meshes on exit of Power Armor if they're not hidden
        if (const auto hideNode = f4vr::findNode(f4vr::getWorldRootNode(), g_config.isHoloPipboy ? "Screen" : "HoloEmitter")) {
            if (fNotEqual(hideNode->local.scale, 0)) {
                hideNode->flags.flags |= 0x1;
                hideNode->local.scale = 0;
            }
        }

        // sets 3rd Person Pipboy Scale
        if (const auto pipboy3Rd = f4vr::findNode(f4vr::getWorldRootNode(), "PipboyBone")) {
            pipboy3Rd->local.scale = g_config.pipBoyScale;
        }
    }

    /**
     * Execute the given operation on the current Pipboy page.
     */
    void Pipboy::execOperation(const PipboyOperation operation)
    {
        const auto root = getPipboyMenuRootMovie();
        if (!root) {
            logger::warn<>("Pipboy operation failed, root is null");
            return;
        }

        switch (operation) {
        case PipboyOperation::GOTO_PREV_PAGE:
            gotoPrevPage(root);
            break;
        case PipboyOperation::GOTO_NEXT_PAGE:
            gotoNextPage(root);
            break;
        case PipboyOperation::GOTO_PREV_TAB:
            gotoPrevTab(root);
            break;
        case PipboyOperation::GOTO_NEXT_TAB:
            gotoNextTab(root);
            break;
        case PipboyOperation::MOVE_LIST_SELECTION_UP:
            moveListSelectionUpDown(root, true);
            break;
        case PipboyOperation::MOVE_LIST_SELECTION_DOWN:
            moveListSelectionUpDown(root, false);
            break;
        case PipboyOperation::PRIMARY_PRESS:
            handlePrimaryControllerButtonsOperation(root, true);
            break;
        }
    }

    /**
     * Handle replacing of Pipboy meshes on the arm with either screen or holo emitter.
     */
    void Pipboy::replaceMeshes(const std::string& itemHide, const std::string& itemShow)
    {
        const auto pn = f4vr::getPlayerNodes();
        RE::NiNode* pipParent = f4vr::find1StChildNode(pn->SecondaryWandNode, "PipboyParent");
        if (!pipParent) {
            _meshesReplaced = false;
            return;
        }

        const auto pipboyRoot = f4vr::find1StChildNode(pipParent, "PipboyRoot_NIF_ONLY");
        const auto pipboyReplacetNode = vrui::loadNifFromFile(g_config.isHoloPipboy ? "Data/Meshes/FRIK/HoloPipboyVR.nif" : "Data/Meshes/FRIK/PipboyVR.nif");
        if (pipboyReplacetNode && pipboyRoot) {
            const auto newScreen = f4vr::findAVObject(pipboyReplacetNode, "Screen:0")->parent;
            if (!newScreen) {
                _meshesReplaced = false;
                return;
            }

            pipParent->DetachChild(pipboyRoot);
            pipParent->AttachChild(pipboyReplacetNode, true);

            pn->ScreenNode->DetachChildAt(0);
            // using native function here to attach the new screen as too lazy to fully reverse what it's doing and it works fine.
            f4vr::addNode(reinterpret_cast<uint64_t>(&pn->ScreenNode), newScreen);
            pn->PipboyRoot_nif_only_node = pipboyReplacetNode;
        }

        _meshesReplaced = true;

        static std::string wandPipName("PipboyRoot");
        if (const auto pbRoot = f4vr::findAVObject(pn->SecondaryWandNode, wandPipName)) {
            pbRoot->local = g_config.getPipboyOffset();
        }

        pn->PipboyRoot_nif_only_node->local.scale = 0.0; //prevents the VRPipboy screen from being displayed on first load whilst PB is off.
        if (const auto hideNode = f4vr::findNode(f4vr::getWorldRootNode(), itemHide.c_str())) {
            hideNode->flags.flags |= 0x1;
            hideNode->local.scale = 0;
        }
        if (const auto showNode = f4vr::findNode(f4vr::getWorldRootNode(), itemShow.c_str())) {
            showNode->flags.flags &= 0xfffffffffffffffe;
            showNode->local.scale = 1;
        }

        logger::info("Pipboy Meshes replaced! Hide: {}, Show: {}", itemHide.c_str(), itemShow.c_str());
    }

    /**
     * Turn Pipboy off if "on" button was pressed (short press).
     */
    void Pipboy::checkTurningOnByButton()
    {
        // TODO: create nice function to detect release that is not after long press
        const auto pipOnButtonPressed = (g_config.pipBoyButtonArm
            ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed
            : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed) & vr::ButtonMaskFromId(
            static_cast<vr::EVRButtonId>(g_config.pipBoyButtonID));

        if (pipOnButtonPressed && !_stickyTurnOff /*&& !_physicalHandler.isOperating()*/) {
            _stickyTurnOff = true;
            _controlSleepStickyT = true;
            std::thread t5(&Pipboy::secondaryTriggerSleep, this, 300); // switches a bool to false after 150ms
            t5.detach();
        } else if (!pipOnButtonPressed) {
            if (_controlSleepStickyT && _stickyTurnOff && (!g_config.pipboyOpenWhenLookAt || isLookingAtPipBoy())) {
                // if bool is still set to true on control release we know it was a short press.
                _isOnStatus = true;
                f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = 1.0;
                turnPipBoyOn();
                setPipboyHandPose();
                logger::info("Enabling Pipboy with button");
                _stickyTurnOff = false;
            } else {
                // guard so we don't constantly toggle the pip boy every frame
                _stickyTurnOff = false;
            }
        }
    }

    /**
     * Turn Pipboy off if "off" button was pressed (short press).
     */
    void Pipboy::checkTurningOffByButton()
    {
        // TODO: create nice function to detect release that is not after long press
        const auto pipOffButtonPressed = (g_config.pipBoyButtonOffArm
            ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed
            : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed) & vr::ButtonMaskFromId(
            static_cast<vr::EVRButtonId>(g_config.pipBoyButtonOffID));

        // check off button
        if (pipOffButtonPressed && !_stickyTurnOn) {
            if (_isOnStatus) {
                _isOnStatus = false;
                turnPipBoyOff();
                g_frik.closePipboyConfigurationModeActive();
                disablePipboyHandPose();
                f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = 0.0;
                logger::info("Disabling Pipboy with button");
                _stickyTurnOn = true;
            }
        } else if (!pipOffButtonPressed) {
            // guard so we don't constantly toggle the pip boy off every frame
            _stickyTurnOn = false;
        }
    }

    /**
     * Turn Pipboy on if player is looking at it for a certain amount of time.
     */
    void Pipboy::checkTurningOnByLookingAt()
    {
        if (_isOnStatus || !g_config.pipboyOpenWhenLookAt) {
            return;
        }
        if (_startedLookingAtPip == 0) {
            _startedLookingAtPip = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        } else {
            const auto timeElapsed = static_cast<int>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _startedLookingAtPip);
            if (timeElapsed > g_config.pipBoyOnDelay) {
                _isOnStatus = true;
                f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = 1.0;
                turnPipBoyOn();
                setPipboyHandPose();
                _startedLookingAtPip = 0;
            }
        }
    }

    /**
     * Turn Pipboy off if player is looking away for a certain amount of time or moving the controller.
     */
    void Pipboy::checkTurningOffByLookingAway()
    {
        if (_isOnStatus) {
            _lastLookingAtPip = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        }
        if (isLookingAtPipBoy()) {
            return;
        }

        _startedLookingAtPip = 0;
        const vr::VRControllerAxis_t movingStick = g_config.pipBoyButtonArm > 0
            ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0]
            : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0];
        const vr::VRControllerAxis_t lookingStick = f4vr::isLeftHandedMode()
            ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0]
            : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0];
        const auto timeElapsed = static_cast<int>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _lastLookingAtPip);
        const bool closeLookingWayWithDelay = g_config.pipboyCloseWhenLookAway && _isOnStatus
            && !g_frik.isPipboyConfigurationModeActive()
            && timeElapsed > g_config.pipBoyOffDelay;
        const bool closeLookingWayWithMovement = g_config.pipboyCloseWhenMovingWhileLookingAway && _isOnStatus
            && !g_frik.isPipboyConfigurationModeActive()
            && (fNotEqual(movingStick.x, 0, 0.3f) || fNotEqual(movingStick.y, 0, 0.3f)
                || fNotEqual(lookingStick.x, 0, 0.3f) || fNotEqual(lookingStick.y, 0, 0.3f));

        if (closeLookingWayWithDelay || closeLookingWayWithMovement) {
            _isOnStatus = false;
            turnPipBoyOff();
            f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = 0.0;
            disablePipboyHandPose();
        }
    }

    /**
     * Switch between Pipboy flashlight on head or hand based if player switches using button press near head.
     */
    void Pipboy::checkSwitchingFlashlightHeadHand()
    {
        const bool lightOn = f4vr::isPipboyLightOn(f4vr::getPlayer());
        if (!lightOn) {
            return;
        }

        const bool helmetHeadLamp = isArmorHasHeadLamp();
        if (!helmetHeadLamp) {
            const auto head = f4vr::findNode(f4vr::getPlayer()->GetActorRootNode(false), "Head");
            if (!head) {
                return;
            }
            const bool useRightHand = g_config.leftHandedPipBoy || g_config.isPipBoyTorchRightArmMode;
            const auto hand = _skelly->getBoneWorldTransform(useRightHand ? "RArm_Hand" : "LArm_Hand").translate;
            const float distance = vec3Len(hand - head->world.translate);
            if (distance < 15.0) {
                uint64_t pipboyHand = f4vr::VRControllers.getControllerState_DEPRECATED(useRightHand ? f4vr::TrackerType::Right : f4vr::TrackerType::Left).ulButtonPressed;
                const auto SwitchLightButton = pipboyHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config.switchTorchButton));
                f4vr::VRControllers.triggerHaptic(useRightHand ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand, 0.1f, 0.1f);
                // Control switching between hand and head based Pipboy light
                if (SwitchLightButton && !_stickySwithLight) {
                    _stickySwithLight = true;
                    f4vr::VRControllers.triggerHaptic(useRightHand ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand, 0.1f);
                    auto LGHT_ATTACH = useRightHand
                        ? f4vr::findNode(_skelly->getRightArm().shoulder, "RArm_Hand")
                        : f4vr::findNode(_skelly->getLeftArm().shoulder, "LArm_Hand");
                    RE::NiNode* lght = g_config.isPipBoyTorchOnArm
                        ? f4vr::find1StChildNode(LGHT_ATTACH, "HeadLightParent")
                        : f4vr::getPlayerNodes()->HeadLightParentNode;
                    if (lght) {
                        auto parentnode = g_config.isPipBoyTorchOnArm ? lght->parent->name : f4vr::getPlayerNodes()->HeadLightParentNode->parent->name;
                        float rotz = g_config.isPipBoyTorchOnArm ? -90 : 90;
                        lght->local.rotate = lght->local.rotate * getMatrixFromEulerAngles(0, 0, degreesToRads(rotz));
                        lght->local.translate.y = g_config.isPipBoyTorchOnArm ? 0 : 4;
                        g_config.isPipBoyTorchOnArm
                            ? lght->parent->DetachChild(lght)
                            : f4vr::getPlayerNodes()->HeadLightParentNode->parent->DetachChild(lght);
                        g_config.isPipBoyTorchOnArm
                            ? f4vr::getPlayerNodes()->HmdNode->AttachChild(lght, true)
                            : LGHT_ATTACH->AttachChild(lght, true);
                        g_config.togglePipBoyTorchOnArm();
                    }
                }
                if (!SwitchLightButton) {
                    _stickySwithLight = false;
                }
            } else if (distance > 10) {
                _stickySwithLight = false;
            }
        }

        //Attach light to hand
        if (g_config.isPipBoyTorchOnArm) {
            auto LGHT_ATTACH = g_config.leftHandedPipBoy || g_config.isPipBoyTorchRightArmMode
                ? f4vr::findNode(_skelly->getRightArm().shoulder, "RArm_Hand")
                : f4vr::findNode(_skelly->getLeftArm().shoulder, "LArm_Hand");
            if (LGHT_ATTACH) {
                if (lightOn && !helmetHeadLamp) {
                    RE::NiNode* lght = f4vr::getPlayerNodes()->HeadLightParentNode;
                    auto parentnode = f4vr::getPlayerNodes()->HeadLightParentNode->parent->name;
                    if (parentnode == "HMDNode") {
                        f4vr::getPlayerNodes()->HeadLightParentNode->parent->DetachChild(lght);
                        lght->local.rotate = lght->local.rotate * getMatrixFromEulerAngles(0, 0, degreesToRads(90));
                        lght->local.translate.y = 4;
                        LGHT_ATTACH->AttachChild(lght, true);
                    }
                }
                //Restore HeadLight to correct node when light is powered off (to avoid any crashes)
                else if (!lightOn || helmetHeadLamp) {
                    if (auto lght = f4vr::find1StChildNode(LGHT_ATTACH, "HeadLightParent")) {
                        auto parentnode = lght->parent->name;
                        if (parentnode != "HMDNode") {
                            lght->local.rotate = lght->local.rotate * getMatrixFromEulerAngles(0, 0, degreesToRads(-90));
                            lght->local.translate.y = 0;
                            lght->parent->DetachChild(lght);
                            f4vr::getPlayerNodes()->HmdNode->AttachChild(lght, true);
                        }
                    }
                }
            }
        }
    }

    /**
     * Execute operation on the currently active Pipboy UI by primary thumbstick input.
     * On map page the map is moved.
     * On other pages the list selection is moved up/down or tabs are changed left/right.
     */
    void Pipboy::handlePrimaryControllerThumbstickOperation(GFx::Movie* const root)
    {
        if (!isPrimaryGripPressHeldDown()) {
            const auto doinantHandStick = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary, 0);
            if (_lastPipboyPage == PipboyPage::MAP && !isMessageHolderVisible(root)) {
                // Map Tab
                GFx::Value akArgs[2];
                akArgs[0] = doinantHandStick.x * -1;
                akArgs[1] = doinantHandStick.y;
                // Move Map
                root->Invoke("root.Menu_mc.CurrentPage.WorldMapHolder_mc.PanMap", nullptr, akArgs, 2);
                root->Invoke("root.Menu_mc.CurrentPage.LocalMapHolder_mc.PanMap", nullptr, akArgs, 2);
            } else {
                if (doinantHandStick.y > 0.85) {
                    if (!_controlSleepStickyY) {
                        _controlSleepStickyY = true;
                        moveListSelectionUpDown(root, true);
                        std::thread t2(&Pipboy::rightStickYSleep, this, 155);
                        t2.detach();
                    }
                }
                if (doinantHandStick.y < -0.85) {
                    if (!_controlSleepStickyY) {
                        _controlSleepStickyY = true;
                        moveListSelectionUpDown(root, false);
                        std::thread t2(&Pipboy::rightStickYSleep, this, 155);
                        t2.detach();
                    }
                }
                if (doinantHandStick.x < -0.85) {
                    if (!_controlSleepStickyX) {
                        _controlSleepStickyX = true;
                        gotoPrevTab(root);
                        std::thread t3(&Pipboy::rightStickXSleep, this, 170);
                        t3.detach();
                    }
                }
                if (doinantHandStick.x > 0.85) {
                    if (!_controlSleepStickyX) {
                        _controlSleepStickyX = true;
                        gotoNextTab(root);
                        std::thread t3(&Pipboy::rightStickXSleep, this, 170);
                        t3.detach();
                    }
                }
            }
        }
    }

    /**
     * Execute operation on the currently active Pipboy UI by primary controller buttons.
     * First thing is to detect is a message box is visible (context menu options) as message box and main lists are active at the
     * same time. So, if the message box is visible we will only operate on the message box list and not the main list.
     * See documentation: https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Development-%E2%80%90-Pipboy-Controls
     * @param root - Pipboy Scaleform UI root
     * @param triggerPressed - true if operate as trigger is pressed regardless of controller input (used for physical handler)
     */
    void Pipboy::handlePrimaryControllerButtonsOperation(GFx::Movie* root, bool triggerPressed)
    {
        // page level navigation
        const bool gripPressHeldDown = isPrimaryGripPressHeldDown();
        if (!gripPressHeldDown && isAButtonPressed()) {
            gotoPrevPage(root);
            return;
        }
        if (!gripPressHeldDown && isBButtonPressed()) {
            gotoNextPage(root);
            return;
        }

        // include actual controller check
        triggerPressed = triggerPressed || isPrimaryTriggerPressed();

        // Context menu message box handling
        if (triggerPressed && isMessageHolderVisible(root)) {
            triggerShortHeptic();
            f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.MessageHolder_mc", f4vr::ScaleformListOp::Select);
            f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc", f4vr::ScaleformListOp::Select);
            // prevent affecting the main list if message box is visible
            return;
        }

        // Specific handling by current page
        switch (f4vr::getScaleformInt(root, "root.Menu_mc.DataObj._CurrentPage").value_or(-1)) {
        case 0:
            handlePrimaryControllerOperationOnStatusPage(root, triggerPressed);
            break;
        case 1:
            handlePrimaryControllerOperationOnInventoryPage(root, triggerPressed);
            break;
        case 2:
            handlePrimaryControllerOperationOnDataPage(root, triggerPressed);
            break;
        case 3:
            handlePrimaryControllerOperationOnMapPage(root, triggerPressed);
            break;
        case 4:
            handlePrimaryControllerOperationOnRadioPage(root, triggerPressed);
            break;
        default: ;
        }

        if (f4vr::VRControllers.isPressed(vr::EVRButtonId::k_EButton_Axis0, f4vr::Hand::Primary)) {
            root->Invoke("root.Menu_mc.CurrentPage.onMessageButtonPress()", nullptr, nullptr, 0);
        }
    }

    void Pipboy::gotoPrevPage(GFx::Movie* root)
    {
        root->Invoke("root.Menu_mc.gotoPrevPage", nullptr, nullptr, 0);
    }

    void Pipboy::gotoNextPage(GFx::Movie* root)
    {
        root->Invoke("root.Menu_mc.gotoNextPage", nullptr, nullptr, 0);
    }

    void Pipboy::gotoPrevTab(GFx::Movie* root)
    {
        triggerShortHeptic();
        root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0);
    }

    void Pipboy::gotoNextTab(GFx::Movie* root)
    {
        triggerShortHeptic();
        root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0);
    }

    /**
     * Execute moving selection on the currently active list up or down.
     * Whatever the list found to exist it will be moved, mostly we can "try" to move any of the lists and only the one that exists will be moved.
     * On DATA page all 3 tabs can exist at the same time, so we need to check which one is visible to prevent working on a hidden one.
     * First thing is to detect is a message box is visible (context menu options) as message box and main lists are active at the
     * same time. So, if the message box is visible we will only operate on the message box list and not the main list.
     */
    void Pipboy::moveListSelectionUpDown(GFx::Movie* root, const bool moveUp)
    {
        triggerShortHeptic();

        const auto listOp = moveUp ? f4vr::ScaleformListOp::MoveUp : f4vr::ScaleformListOp::MoveDown;

        if (isMessageHolderVisible(root)) {
            f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.MessageHolder_mc", listOp);
            f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc", listOp);
            // prevent affecting the main list if message box is visible
            return;
        }

        // Inventory, Radio, Special, and Perks tabs
        f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", listOp);
        f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc", listOp);
        f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.PerksTab_mc.List_mc", listOp);

        // Quest, Workshop, and Stats tabs exist at the same time, need to check which one is visible
        if (isQuestTabVisibleOnDataPage(root)) {
            // Quests tab has 2 lists for the main quests and quest objectives
            const char* listPath = isQuestTabObjectiveListEnabledOnDataPage(root)
                ? "root.Menu_mc.CurrentPage.QuestsTab_mc.ObjectivesList_mc"
                : "root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc";
            f4vr::doOperationOnScaleformList(root, listPath, listOp);
        } else if (isWorkshopsTabVisibleOnDataPage(root)) {
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc.List_mc", listOp);
        } else {
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.StatsTab_mc.CategoryList_mc", listOp);
        }
    }

    void Pipboy::handlePrimaryControllerOperationOnStatusPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (triggerPressed) {
            triggerShortHeptic();
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc", f4vr::ScaleformListOp::Select);
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.PerksTab_mc.List_mc", f4vr::ScaleformListOp::Select);
        }
    }

    void Pipboy::handlePrimaryControllerOperationOnInventoryPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (triggerPressed) {
            triggerShortHeptic();
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", f4vr::ScaleformListOp::Select);
        }
    }

    /**
     * On DATA page all 3 tabs can exist at the same time, so we need to check which one is visible to prevent working on a hidden one.
     */
    void Pipboy::handlePrimaryControllerOperationOnDataPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (triggerPressed) {
            triggerShortHeptic();
            if (isQuestTabVisibleOnDataPage(root)) {
                f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc", f4vr::ScaleformListOp::Select);
            } else {
                // open quest in map
                f4vr::invokeScaleformProcessUserEvent(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc", "XButton");
            }
        }
    }

    /**
     * For handling map faster travel or marker setting we need to know if fast travel can be done using "bCanFastTravel"
     * and then let the Pipboy code handle the rest by sending the right event to the currently visible map (world/local).
     */
    void Pipboy::handlePrimaryControllerOperationOnMapPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (isPrimaryGripPressHeldDown()) {
            if (triggerPressed) {
                // switch world/local maps
                triggerShortHeptic();
                f4vr::invokeScaleformProcessUserEvent(root, "root.Menu_mc.CurrentPage", "XButton");
            } else {
                // zoom map
                const auto [_, primAxisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary);
                if (fNotEqual(primAxisY, 0, 0.5f)) {
                    GFx::Value args[1];
                    args[0] = primAxisY / 100.f;
                    root->Invoke(getCurrentMapPath(root, ".ZoomMap").c_str(), nullptr, args, 1);
                }
            }
        } else if (triggerPressed) {
            triggerShortHeptic();

            // handle fast travel, custom marker
            const char* eventName = f4vr::getScaleformBool(root, getCurrentMapPath(root, ".bCanFastTravel").c_str()) ? "MapHolder:activate_marker" : "MapHolder:set_custom_marker";
            f4vr::invokeScaleformDispatchEvent(root, getCurrentMapPath(root), eventName);
        }
    }

    void Pipboy::handlePrimaryControllerOperationOnRadioPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (triggerPressed) {
            triggerShortHeptic();
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", f4vr::ScaleformListOp::Select);
        }
    }

    /**
     * Get Current Pipboy Tab and store it.
     */
    void Pipboy::storeLastPipboyPage()
    {
        const auto root = getPipboyMenuRootMovie();
        GFx::Value PBCurrentPage;
        if (root && root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage") && PBCurrentPage.GetType() != GFx::Value::ValueType::kUndefined) {
            _lastPipboyPage = static_cast<PipboyPage>(PBCurrentPage.GetUInt());
        }
    }

    /**
     * Reduce shaking of Pipboy screen by "dampening" its movement.
     */
    void Pipboy::dampenPipboyScreen()
    {
        if (!g_config.dampenPipboyScreen) {
            return;
        }
        if (!_isOnStatus) {
            _pipboyScreenPrevFrame = f4vr::getPlayerNodes()->ScreenNode->world;
            return;
        }
        RE::NiNode* pipboyScreen = f4vr::getPlayerNodes()->ScreenNode;

        if (pipboyScreen && _isOnStatus) {
            Quaternion rq, rt;
            // do a spherical interpolation between previous frame and current frame for the world rotation matrix
            const auto prevFrame = _pipboyScreenPrevFrame;
            rq.fromMatrix(prevFrame.rotate);
            rt.fromMatrix(pipboyScreen->world.rotate);
            rq.slerp(1 - g_config.dampenPipboyRotation, rt);
            pipboyScreen->world.rotate = rq.getMatrix();
            // do a linear interpolation between the position from the previous frame to current frame
            RE::NiPoint3 deltaPos = pipboyScreen->world.translate - prevFrame.translate;
            deltaPos *= g_config.dampenPipboyTranslation; // just use hands dampening value for now
            pipboyScreen->world.translate -= deltaPos;
            _pipboyScreenPrevFrame = pipboyScreen->world;
            f4vr::updateDown(pipboyScreen, false);
        }
    }

    /**
     * Prevents continuous Input from Right Stick X Axis
     */
    void Pipboy::rightStickXSleep(const int time)
    {
        Sleep(time);
        _controlSleepStickyX = false;
    }

    /**
     * Prevents continuous Input from Right Stick Y Axis
     */
    void Pipboy::rightStickYSleep(const int time)
    {
        Sleep(time);
        _controlSleepStickyY = false;
    }

    /**
     * Used to determine if secondary trigger received a long or short press
     */
    void Pipboy::secondaryTriggerSleep(const int time)
    {
        Sleep(time);
        _controlSleepStickyT = false;
    }
}
