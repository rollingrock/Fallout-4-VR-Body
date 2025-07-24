#include "Pipboy.h"

#include <chrono>
#include <thread>

#include "Config.h"
#include "FRIK.h"
#include "HandPose.h"
#include "utils.h"
#include "common/Logger.h"
#include "f4vr/VRControllersManager.h"

using namespace std::chrono;
using namespace common;

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

        _physicalHandler.operate(_lastPipboyPage);

        if (f4vr::isInPowerArmor()) {
            lastRadioFreq = 0.0; // Ensures radio needle doesn't get messed up when entering and then exiting Power Armor.
            return;
        }

        if (_isOnStatus) {
            _operationHandler.operate();

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
     * Get Current Pipboy Tab and store it.
     * For handling if Pipboy was opened in PA where most of this code is not executed.
     */
    void Pipboy::storeLastPipboyPage()
    {
        const auto pipboyPage = PipboyOperationHandler::getCurrentPipboyPage(PipboyOperationHandler::getPipboyMenuRoot());
        if (pipboyPage.has_value()) {
            _lastPipboyPage = pipboyPage.value();
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
     * Used to determine if secondary trigger received a long or short press
     */
    void Pipboy::secondaryTriggerSleep(const int time)
    {
        Sleep(time);
        _controlSleepStickyT = false;
    }
}
