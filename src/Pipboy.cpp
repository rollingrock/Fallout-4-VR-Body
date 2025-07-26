#include "Pipboy.h"

#include "Config.h"
#include "FRIK.h"
#include "utils.h"
#include "common/Logger.h"
#include "f4vr/VRControllersManager.h"

using namespace common;

namespace
{
    /**
     * This is the actual thing that causes the game to turn Pipboy functionality on/off.
     * Not sure what it does exactly...
     */
    void turnPipBoyOnOff(const bool open)
    {
        RE::GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy")->SetFloat(open ? 0.0 : 20);
        RE::GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy")->SetFloat(open ? 0.0 : 5);
        RE::GetINISetting("fPipboyScaleOuterAngle:VRPipboy")->SetFloat(open ? 0.0 : 20);
        RE::GetINISetting("fPipboyScaleInnerAngle:VRPipboy")->SetFloat(open ? 0.0 : 5);
    }
}

namespace frik
{
    /**
     * First frame PA fix:
     *
     */
    Pipboy::Pipboy(Skeleton* skelly):
        _skelly(skelly), _physicalHandler(skelly, this)
    {
        exitPowerArmorBugFixHack(true);
    }

    /**
     * Turn Pipboy On/Off.
     */
    void Pipboy::openClose(const bool open)
    {
        if (_isOpen == open) {
            return;
        }
        _isOpen = open;
        turnPipBoyOnOff(open);
        f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = open ? 1 : 0;
        if (!open) {
            _pipboyScreenStableFrame = std::nullopt;
            g_frik.closePipboyConfigurationModeActive();
        }
    }

    /**
     * Swap the Pipboy model between screen and holo models.
     */
    void Pipboy::swapModel() const
    {
        g_config.toggleIsHoloPipboy();
        turnPipBoyOnOff(false);
        replaceMeshes(true);
        f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = 1;
        turnPipBoyOnOff(true);
    }

    /**
     * Executed every frame to update the Pipboy location and handle interaction with pipboy config UX.
     */
    void Pipboy::onFrameUpdate()
    {
        exitPowerArmorBugFixHack(false);

        if (!g_config.removeFlashlight && f4vr::isPipboyLightOn(f4vr::getPlayer())) {
            checkSwitchingFlashlightHeadHand();
            adjustFlashlightTransformToHandOrHead();
        }

        hideShowPipboyOnArm();
        if (g_config.hidePipboy) {
            // nothing to do if Pipboy is hidden
            return;
        }

        // store the last Pipboy page even if in PA to have the correct dail after existing PA.
        storeLastPipboyPage();

        if (f4vr::isInPowerArmor()) {
            return;
        }

        _physicalHandler.operate(_lastPipboyPage);

        if (!f4vr::isPipboyOnWrist()) {
            restoreDefaultPipboyModelIfNeeded();
            return;
        }

        replaceMeshes(false);

        // check by looking should be first to handle closing by button not opening it again by looking at Pipboy.
        checkTurningOnByLookingAt();
        checkTurningOffByLookingAway();

        checkTurningOnByButton();
        checkTurningOffByButton();

        if (_isOpen) {
            positionPipboyModelToPipboyOnArm();

            PipboyOperationHandler::operate();

            dampenPipboyScreen();
        }
    }

    /**
     * There is a bug that when existing Power Armor you can't open the main menu.
     * But it only happens if the Pipboy is on-wrist setting (others are fine).
     * A fix is to open and close the Pipboy.
     * The hack here is to open the Pipboy in the ctor and close it ASAP in the first frame.
     */
    void Pipboy::exitPowerArmorBugFixHack(const bool set)
    {
        if (set) {
            // set Pipboy to open
            if (f4vr::isPipboyOnWrist()) {
                turnPipBoyOnOff(true);
                _exitPowerArmorFixFirstFrame = true;
            }
        } else {
            // Close the Pipboy on first frame if existed PA
            if (_exitPowerArmorFixFirstFrame) {
                if (f4vr::isPipboyOnWrist() && !f4vr::isInPowerArmor())
                    turnPipBoyOnOff(false);
                _exitPowerArmorFixFirstFrame = false;
            }
        }
    }

    /**
     * Hide the Pipboy on arm model if configured to do so.
     * Check every frame in case it was changed to show.
     */
    void Pipboy::hideShowPipboyOnArm() const
    {
        const auto pipboy = getPipboyNode();
        if (!pipboy) {
            return;
        }

        if (g_config.hidePipboy) {
            if (pipboy->local.scale != 0.0) {
                pipboy->local.scale = 0.0;
                f4vr::setNodeVisibilityDeep(pipboy, false);
            }
        } else if (fEqual(pipboy->local.scale, 0)) {
            pipboy->local.scale = g_config.pipBoyScale;
            f4vr::setNodeVisibilityDeep(pipboy, true);
        }
    }

    /**
     * Restore the default Pipboy nif model to be used by in-front Pipboy.
     * Only need to restore if original was replaced, only happens once.
     */
    void Pipboy::restoreDefaultPipboyModelIfNeeded()
    {
        if (!_originalPipboyRootNifOnlyNode || !f4vr::getPlayerNodes()->PipboyRoot_nif_only_node) {
            return;
        }

        const auto pn = f4vr::getPlayerNodes();
        pn->PipboyParentNode->DetachChild(pn->PipboyRoot_nif_only_node);
        pn->PipboyParentNode->AttachChild(_originalPipboyRootNifOnlyNode, true);
        pn->PipboyRoot_nif_only_node = _originalPipboyRootNifOnlyNode;
        _originalPipboyRootNifOnlyNode = nullptr;
    }

    /**
     * Position the Pipboy model to match the Pipboy on arm position.
     */
    void Pipboy::positionPipboyModelToPipboyOnArm() const
    {
        const auto pipboyBone = getPipboyNode();
        const auto wandPip = f4vr::getPlayerNodes()->PipboyRoot_nif_only_node;
        if (!pipboyBone || !wandPip) {
            return;
        }

        const auto delta = pipboyBone->world.translate - wandPip->parent->world.translate;
        wandPip->local.translate = wandPip->parent->world.rotate * ((delta / wandPip->parent->world.scale));

        const auto wandWROT = getMatrixFromEulerAngles(degreesToRads(30), 0, 0) * pipboyBone->world.rotate;
        wandPip->local.rotate = wandWROT * wandPip->parent->world.rotate.Transpose();
    }

    /**
     * Run Pipboy mesh replacement if not already done (or forced) to the configured meshes either holo or screen.
     * @param force true - run mesh replace, false - only if not previously replaced
     */
    void Pipboy::replaceMeshes(const bool force) const
    {
        if (force || !_originalPipboyRootNifOnlyNode || _lastMeshReplaceIsHoloPipboy != g_config.isHoloPipboy) {
            const bool success = g_config.isHoloPipboy
                ? replaceMeshes("Screen", "HoloEmitter")
                : replaceMeshes("HoloEmitter", "Screen");
            if (success) {
                _lastMeshReplaceIsHoloPipboy = g_config.isHoloPipboy;
            }
        }
    }

    /**
     * Handle replacing of Pipboy meshes on the arm with either screen or holo emitter.
     */
    bool Pipboy::replaceMeshes(const std::string& itemHide, const std::string& itemShow) const
    {
        const auto pn = f4vr::getPlayerNodes();
        if (!pn->PipboyParentNode || !pn->PipboyRoot_nif_only_node) {
            return false;
        }

        if (!_originalPipboyRootNifOnlyNode) {
            // save the original to restore it later if needed.
            _originalPipboyRootNifOnlyNode = pn->PipboyRoot_nif_only_node;
        }

        const auto pipboyReplacetNode = f4vr::loadNifFromFile(g_config.isHoloPipboy ? "FRIK/HoloPipboyVR.nif" : "FRIK/PipboyVR.nif");
        const auto newScreen = f4vr::findAVObject(pipboyReplacetNode, "Screen:0")->parent;
        if (!newScreen) {
            return false;
        }

        // replace the game Pipboy nif with ours
        pn->PipboyParentNode->DetachChild(pn->PipboyRoot_nif_only_node);
        pn->PipboyParentNode->AttachChild(pipboyReplacetNode, true);
        pn->PipboyRoot_nif_only_node = pipboyReplacetNode;
        pn->PipboyRoot_nif_only_node->local.scale = 0.0; // hide until opened

        pn->ScreenNode = newScreen;

        // No fucking idea why this was done, but I removed it and nothing is broken. Leaving as comment if I'm wrong.
        // pn->ScreenNode->DetachChildAt(0);
        // // using native function here to attach the new screen as too lazy to fully reverse what it's doing and it works fine.
        // f4vr::addNode(reinterpret_cast<uint64_t>(&pn->ScreenNode), newScreen);

        // load Pipboy screen adjusted position config
        if (const auto pbRoot = f4vr::getFirstChild(pipboyReplacetNode)) {
            pbRoot->local = g_config.getPipboyOffset();
        }

        // sets 3rd Person Pipboy (on the arm) scale and correct UI
        if (const auto pipboy3Rd = getPipboyNode()) {
            pipboy3Rd->local.scale = g_config.pipBoyScale;

            if (const auto hideNode = f4vr::findNode(pipboy3Rd, itemHide.c_str())) {
                f4vr::setNodeVisibility(hideNode, false);
                hideNode->local.scale = 0;
            }
            if (const auto showNode = f4vr::findNode(pipboy3Rd, itemShow.c_str())) {
                f4vr::setNodeVisibility(showNode, true);
                showNode->local.scale = 1;
            }
        }

        logger::info("Pipboy Meshes replaced! Hide: {}, Show: {}", itemHide.c_str(), itemShow.c_str());
        return true;
    }

    /**
     * Turn Pipboy off if "on" button was pressed (short press).
     */
    void Pipboy::checkTurningOnByButton()
    {
        if (!f4vr::VRControllers.isReleasedShort(f4vr::Hand::Offhand, g_config.pipBoyButtonID)) {
            return;
        }

        logger::info("Open Pipboy with button");
        openClose(true);
    }

    /**
     * Turn Pipboy off if "off" button was pressed (short press).
     */
    void Pipboy::checkTurningOffByButton()
    {
        if (!_isOpen || !f4vr::VRControllers.isReleasedShort(f4vr::Hand::Offhand, g_config.pipBoyButtonOffID)) {
            return;
        }

        logger::info("Close Pipboy with button");
        openClose(false);

        // Prevents Pipboy from being turned on again immediately after closing it.
        _startedLookingAtPip = nowMillis() + 10000;
    }

    /**
     * Turn Pipboy on if player is looking at it for a certain amount of time.
     */
    void Pipboy::checkTurningOnByLookingAt()
    {
        if (_isOpen || !g_config.pipboyOpenWhenLookAt || !isPlayerLookingAt()) {
            _startedLookingAtPip = 0;
            return;
        }
        if (_startedLookingAtPip == 0) {
            _startedLookingAtPip = nowMillis();
        } else {
            if (isNowTimePassed(_startedLookingAtPip, g_config.pipBoyOnDelay)) {
                logger::info("Open Pipboy by looking at");
                openClose(true);
            }
        }
    }

    /**
     * Turn Pipboy off if player is looking away for a certain amount of time or moving the controller.
     */
    void Pipboy::checkTurningOffByLookingAway()
    {
        if (!_isOpen) {
            return;
        }

        if (isPlayerLookingAt()) {
            _lastLookingAtPip = nowMillis();
            return;
        }

        const auto movingStick = f4vr::VRControllers.getThumbstickValue(g_config.pipBoyButtonArm > 0 ? f4vr::Hand::Right : f4vr::Hand::Left);
        const auto lookingStick = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Primary);
        const bool closeLookingWayWithDelay = g_config.pipboyCloseWhenLookAway && _isOpen
            && !g_frik.isPipboyConfigurationModeActive()
            && isNowTimePassed(_lastLookingAtPip, g_config.pipBoyOffDelay);
        const bool closeLookingWayWithMovement = g_config.pipboyCloseWhenMovingWhileLookingAway
            && _isOpen
            && !g_frik.isPipboyConfigurationModeActive()
            && (fNotEqual(movingStick.x, 0, 0.3f) || fNotEqual(movingStick.y, 0, 0.3f)
                || fNotEqual(lookingStick.x, 0, 0.3f) || fNotEqual(lookingStick.y, 0, 0.3f));

        if (closeLookingWayWithDelay || closeLookingWayWithMovement) {
            logger::info("Close Pipboy when looking away: byDelay({}), byMovement({})", closeLookingWayWithDelay, closeLookingWayWithMovement);
            openClose(false);
        }
    }

    /**
     * Switch between Pipboy flashlight on head or right/left hand based if player switches using button press of the hand near head.
     */
    void Pipboy::checkSwitchingFlashlightHeadHand()
    {
        const auto hmdPos = f4vr::getPlayerNodes()->HmdNode->world.translate;
        const auto isLeftHandCloseToHMD = vec3Len(_skelly->getBoneWorldTransform("LArm_Hand").translate - hmdPos) < 15;
        const auto isRightHandCloseToHMD = vec3Len(_skelly->getBoneWorldTransform("RArm_Hand").translate - hmdPos) < 15;

        const auto now = nowMillis();
        if (isLeftHandCloseToHMD && (g_config.flashlightLocation == FlashlightLocation::Head || g_config.flashlightLocation == FlashlightLocation::LeftArm)) {
            if (_flashlightHapticCooldown < now)
                triggerShortHaptic(f4vr::Hand::Left);
        } else if (isRightHandCloseToHMD && (g_config.flashlightLocation == FlashlightLocation::Head || g_config.flashlightLocation == FlashlightLocation::RightArm)) {
            if (_flashlightHapticCooldown < now)
                triggerShortHaptic(f4vr::Hand::Right);
        } else {
            _flashlightHapticCooldown = 0;
            return;
        }

        const bool isLeftHandGrab = isLeftHandCloseToHMD && f4vr::VRControllers.isReleasedShort(f4vr::Hand::Left, g_config.switchTorchButton);
        const bool isRightHandGrab = isRightHandCloseToHMD && f4vr::VRControllers.isReleasedShort(f4vr::Hand::Right, g_config.switchTorchButton);
        if (!isLeftHandGrab && !isRightHandGrab) {
            return;
        }

        if (g_config.flashlightLocation == FlashlightLocation::Head) {
            _flashlightHapticCooldown = now + 5000;
            g_config.setFlashlightLocation(isLeftHandGrab ? FlashlightLocation::LeftArm : FlashlightLocation::RightArm);
        } else if ((g_config.flashlightLocation == FlashlightLocation::LeftArm && isLeftHandGrab)
            || (g_config.flashlightLocation == FlashlightLocation::RightArm && isRightHandGrab)) {
            _flashlightHapticCooldown = now + 5000;
            g_config.setFlashlightLocation(FlashlightLocation::Head);
        }
    }

    /**
     * Adjust the position of the light node to the hand that is holding it or revert to head position.
     * It is safer than moving the node as that can result in game crash.
     */
    void Pipboy::adjustFlashlightTransformToHandOrHead() const
    {
        const auto lightNode = f4vr::getFirstChild(f4vr::getPlayerNodes()->HeadLightParentNode);
        if (!lightNode) {
            return;
        }

        // revert to original transform
        lightNode->local.rotate = getIdentityMatrix();
        lightNode->local.translate = RE::NiPoint3(0, 0, 0);

        if (g_config.flashlightLocation != FlashlightLocation::Head) {
            // update world transforms after reverting to original
            f4vr::updateTransforms(lightNode);

            // use the right arm node
            const auto armNode = g_config.flashlightLocation == FlashlightLocation::LeftArm
                ? f4vr::findNode(_skelly->getLeftArm().shoulder, "LArm_Hand")
                : f4vr::findNode(_skelly->getRightArm().shoulder, "RArm_Hand");

            // calculate relocation transform and set to local
            lightNode->local = calculateRelocation(lightNode, armNode);

            // small adjustment to prevent light on the fingers
            lightNode->local.translate += RE::NiPoint3(4, 0, 2);
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
        if (!_isOpen || !g_config.dampenPipboyScreen) {
            return;
        }

        const auto pipboyScreen = f4vr::getPlayerNodes()->ScreenNode;
        if (!pipboyScreen) {
            return;
        }

        auto lastPos = pipboyScreen->world.translate;
        _pipboyScreenPrevFrame.emplace_back(lastPos);
        if (_pipboyScreenPrevFrame.size() < 4) {
            return;
        }
        if (_pipboyScreenPrevFrame.size() > 5) {
            _pipboyScreenPrevFrame.pop_front();
        }

        const auto deltaPos = lastPos - _pipboyScreenPrevFrame[0];
        const auto threshold2 = g_config.dampenPipboyLargeThreshold;
        if (std::abs(deltaPos.x) > threshold2 || std::abs(deltaPos.y) > threshold2 || std::abs(deltaPos.z) > threshold2) {
            // too much movement, don't dampen at all so the screen will not lag behind the pipboy
            _pipboyScreenStableFrame = std::nullopt;
            return;
        }

        if (!_pipboyScreenStableFrame.has_value()) {
            _pipboyScreenStableFrame = lastPos;
        }

        if (g_config.isHoloPipboy) {
            dampenPipboyHoloScreen(pipboyScreen);
        } else {
            dampenPipboyRegularScreen(pipboyScreen);
        }
    }

    /**
     * Small dampening of the screen movement but still always move the screen to be in the pipboy.
     */
    void Pipboy::dampenPipboyRegularScreen(RE::NiNode* pipboyScreen)
    {
        const auto prevFrame = _pipboyScreenStableFrame.value();
        _pipboyScreenStableFrame = pipboyScreen->world.translate;

        // do a linear interpolation between the position from the previous frame to current frame
        const auto deltaPos = (pipboyScreen->world.translate - prevFrame) * g_config.dampenRegularPipboyMultiplier;
        pipboyScreen->world.translate -= deltaPos;
        f4vr::updateDown(pipboyScreen, false);
    }

    /**
     * Allow the holo screen to remain stable in place while the arm moves a little.
     * But if the arm moves more than a certain threshold, move the screen to the new position smoothly.
     */
    void Pipboy::dampenPipboyHoloScreen(RE::NiNode* pipboyScreen)
    {
        const auto prevFrame = _pipboyScreenStableFrame.value();

        const auto deltaPos = pipboyScreen->world.translate - prevFrame;
        const auto threshold = g_config.dampenHoloPipboySmallThreshold;
        if (_dampenScreenAdjustCounter == 0 && std::abs(deltaPos.x) < threshold && std::abs(deltaPos.y) < threshold && std::abs(deltaPos.z) < threshold) {
            // prevent any screen movement for small movements
            pipboyScreen->world.translate = prevFrame;
        } else {
            // if arm moved more than threshold, move the screen to pipboy position smoothly
            _pipboyScreenStableFrame = prevFrame + deltaPos / static_cast<float>(g_config.dampenHoloPipboyAdjustmentFrames);
            _dampenScreenAdjustCounter = _dampenScreenAdjustCounter > 0 ? _dampenScreenAdjustCounter - 1 : g_config.dampenHoloPipboyAdjustmentFrames;
            pipboyScreen->world.translate = _pipboyScreenStableFrame.value();
        }
        f4vr::updateDown(pipboyScreen, false);
    }

    /**
     * Is the player currently looking at the Pipboy screen?
     * Handle different thresholds if Pipboy is on or off as looking away is more relaxed threshold.
     */
    bool Pipboy::isPlayerLookingAt() const
    {
        const auto pipboy = f4vr::getPlayerNodes()->PipboyRoot_nif_only_node;
        const auto screen = pipboy ? f4vr::findAVObject(pipboy, "Screen:0") : nullptr;
        if (screen == nullptr) {
            return false;
        }

        const float threshhold = _isOpen ? g_config.pipboyLookAwayThreshold : g_config.pipboyLookAtThreshold;
        return isCameraLookingAtObject(f4vr::getPlayerCamera()->cameraNode, screen, threshhold);
    }

    /**
     * The left-handed Pipboy is currently broken.
     * Needs a lot of work to handle it everywhere.
     */
    void Pipboy::leftHandedModePipboy() const
    {
        if (g_config.leftHandedPipBoy) {
            auto pipbone = f4vr::findNode(_skelly->getRightArm().forearm1, "PipboyBone");

            if (!pipbone) {
                pipbone = f4vr::findNode(_skelly->getLeftArm().forearm1, "PipboyBone");

                if (!pipbone) {
                    return;
                }

                pipbone->parent->DetachChild(pipbone);
                _skelly->getRightArm().forearm3->IsNode()->AttachChild(pipbone, true);
            }

            pipbone->local.rotate = getMatrixFromEulerAngles(0, degreesToRads(180.0), 0) * pipbone->local.rotate;
            pipbone->local.translate *= -1.5;
        }
    }

    RE::NiAVObject* Pipboy::getPipboyNode() const
    {
        const auto arm = (g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm()).forearm3;
        return arm ? f4vr::findAVObject(arm, "PipboyBone") : nullptr;
    }
}
