#include "Pipboy.h"

#include "Config.h"
#include "FRIK.h"
#include "utils.h"
#include "vrcf/VRControllersManager.h"
#include "skeleton/HandPose.h"

using namespace common;

namespace
{
    /**
     * This is the actual thing that causes the game to turn Pipboy functionality on/off.
     * Not sure what it does exactly...
     */
    void turnPipBoyOnOff(const bool open)
    {
        RE::GetINISetting("fHMDToPipboyScaleOuterAngle:VRPipboy")->SetFloat(open ? 0.0f : 20.0f);
        RE::GetINISetting("fHMDToPipboyScaleInnerAngle:VRPipboy")->SetFloat(open ? 0.0f : 5.0f);
        RE::GetINISetting("fPipboyScaleOuterAngle:VRPipboy")->SetFloat(open ? 0.0f : 20.0f);
        RE::GetINISetting("fPipboyScaleInnerAngle:VRPipboy")->SetFloat(open ? 0.0f : 5.0f);
    }

    /**
     * Get the correct Pipboy root nif to use as a replacement for the on-wrist Pipboy.
     * Special handling for Fallout London VR mod.
     */
    const char* getPipboyReplacementNifPath()
    {
        if (frik::g_config.isFalloutLondonVR) {
            return "FRIK/AttaboyVR.nif";
        }
        return frik::g_config.isHoloPipboy ? "FRIK/HoloPipboyVR.nif" : "FRIK/PipboyVR.nif";
    }
}

namespace frik
{
    /**
     * First frame PA fix:
     *
     */
    Pipboy::Pipboy(Skeleton* skelly) :
        _skelly(skelly), _flashlight(skelly), _physicalHandler(skelly, this)
    {
        // force hide if was open before like when fast traveling (force show if not wrist to allow changing mid-game)
        f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->local.scale = f4vr::isPipboyOnWrist() ? 0.0f : 1.0f;

        // if skeleton changed we need to re-attach the pipboy root nif
        detachReplacedPipboyRootNif();

        exitPowerArmorBugFixHack(true);
    }

    /**
     * Check if the player looking at the Pipboy on arm and the Pipboy is facing the player.
     * @param isPipboyOpen True to check look away threshold, false to check look at threshold.
     */
    bool Pipboy::isPlayerLookingAtPipboy(const bool isPipboyOpen)
    {
        const auto screen = f4vr::getPlayerNodes()->ScreenNode;
        if (screen == nullptr) {
            return false;
        }

        const float threshhold = isPipboyOpen ? g_config.pipboyLookAwayThreshold : g_config.pipboyLookAtThreshold;
        return isCameraLookingAtObject(f4vr::getPlayerCamera()->cameraNode, screen, threshhold);
    }

    /**
     * Turn Pipboy On/Off.
     */
    void Pipboy::openClose(const bool open)
    {
        // ignore if Pipboy is not on wrist game config
        if (!f4vr::isPipboyOnWrist()) {
            return;
        }

        if (_isOpen == open) {
            return;
        }

        if (open && g_config.isFalloutLondonVR && !_attaboyOnBeltNode) {
            // beginning of the game, player doesn't have the attaboy yet (but allow closing if something happens)
            logger::info("Attaboy open canceled, no attaboy on belt found");
            return;
        }

        logger::info("Turning Pipboy {}", open ? "ON" : "OFF");
        _isOpen = open;

        const auto pn = f4vr::getPlayerNodes();
        f4vr::setNodeVisibility(pn->ScreenNode, open);
        pn->PipboyRoot_nif_only_node->local.scale = open ? 1.0f : 0.0f;
        turnPipBoyOnOff(open);

        if (open) {
            // prevent immediate closing of Pipboy
            _lastLookingAtPip = nowMillis();
            if (g_frik.isFavoritesMenuOpen()) {
                f4vr::closeFavoriteMenu();
            }
        } else {
            // Prevents Pipboy from being turned on again immediately after closing it.
            _startedLookingAtPip = nowMillis() + 10000;
            _pipboyScreenPrevFrame.clear();
            g_frik.closePipboyConfigurationModeActive();
        }

        if (g_config.isFalloutLondonVR) {
            setAttaboyHandPose(open);
            if (_attaboyOnBeltNode && _attaboyOnBeltNode->parent && _attaboyOnBeltNode->parent->parent) {
                // show/hide the Attaboy on belt model depending if it's on as it's grabbed by the player
                f4vr::setNodeVisibility(_attaboyOnBeltNode->parent->parent, !open);
            }
        }
    }

    /**
     * Swap the Pipboy model between screen and holo models.
     */
    void Pipboy::swapModel()
    {
        g_config.toggleIsHoloPipboy();
        turnPipBoyOnOff(false);
        detachReplacedPipboyRootNif();
        updateSetupPipboyNodes();
        const auto pn = f4vr::getPlayerNodes();
        pn->PipboyRoot_nif_only_node->local.scale = 1;
        pn->ScreenNode->local = g_config.getPipboyOffset();
        _pipboyScreenPrevFrame.clear();
        turnPipBoyOnOff(true);
    }

    /**
     * Executed every frame to update the Pipboy location and handle interaction with pipboy config UX.
     */
    void Pipboy::onFrameUpdate()
    {
        exitPowerArmorBugFixHack(false);

        _flashlight.onFrameUpdate();

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

        updateSetupPipboyNodes();

        if (g_frik.isPauseMenuOpen() || g_frik.isInScopeMenu()) {
            // prevent interacting with Pipboy when we shouldn't
            return;
        }

        // check by looking should be first to handle closing by button not opening it again by looking at Pipboy.
        checkTurningOnByLookingAt();

        checkTurningOnByButton();
        checkTurningOffByButton();

        if (_isOpen) {
            PipboyOperationHandler::operate();

            dampenPipboyScreen();

            // post screen position adjustments
            checkTurningOffByLookingAway();
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
        const auto pipboy = getPipboyModelOnArmNode();
        if (!pipboy) {
            return;
        }

        if (g_config.hidePipboy || g_config.isFalloutLondonVR) {
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
        detachReplacedPipboyRootNif();

        const auto pn = f4vr::getPlayerNodes();
        if (!_originalPipboyRootNifOnlyNode) {
            return;
        }

        logger::info("Restoring original Pipboy model...");
        _originalPipboyRootNifOnlyNode->local.scale = 1;
        pn->PipboyRoot_nif_only_node = _originalPipboyRootNifOnlyNode;
        if (const auto screenNode = f4vr::findNode(_originalPipboyRootNifOnlyNode, "Screen")) {
            pn->ScreenNode = screenNode;
        } else {
            logger::error("Failed to find Pipboy screen node in original nif!");
        }
        _originalPipboyRootNifOnlyNode = nullptr;
    }

    /**
     * Set up the replaced Pipboy root nif if run first time.
     * Or update Pipboy screen location to make sure it's in the correct place.
     */
    void Pipboy::updateSetupPipboyNodes()
    {
        if (f4vr::isInPowerArmor()) {
            return;
        }

        if (!_newPipboyRootNifOnlyNode) {
            setupPipboyRootNif();
            if (g_config.isHoloPipboy) {
                showHideCorrectPipboyMesh("Screen", "HoloEmitter");
            } else {
                showHideCorrectPipboyMesh("HoloEmitter", "Screen");
            }
        }

        if (!g_frik.isPipboyConfigurationModeActive()) {
            // load Pipboy screen adjusted position config
            f4vr::getPlayerNodes()->ScreenNode->local = g_config.getPipboyOffset();
        }

        updateSetupAttaboyNodes();
    }

    /**
     * Attaboy is not on the player at the beginning of the game but picked up later
     */
    void Pipboy::updateSetupAttaboyNodes()
    {
        if (!g_config.isFalloutLondonVR) {
            return;
        }
        if (_attaboyOnBeltNode) {
            if (!_attaboyOnBeltNode->parent || !_attaboyOnBeltNode->parent->parent) {
                logger::warn("Attaboy on belt node detached!");
                _attaboyOnBeltNode = nullptr;
            }
        } else {
            const auto node = f4vr::findNode(f4vr::getCommonNode(), "PipboyBody", 5);
            _attaboyOnBeltNode = node ? node->IsNode() : nullptr;
            if (_attaboyOnBeltNode) {
                logger::info("Attaboy on belt node found");
            }
        }
    }

    /**
     * Setup FRIK special root nif node to control the screen location and hide Pipboy frame
     */
    void Pipboy::setupPipboyRootNif() const
    {
        const auto pn = f4vr::getPlayerNodes();
        if (!pn->PipboyParentNode || !pn->PipboyRoot_nif_only_node) {
            logger::warn("No pipboy parent or root nif nodes found!");
            return;
        }

        if (!_originalPipboyRootNifOnlyNode) {
            // save the original to restore it later if needed.
            logger::info("Store original pipboy root nif node");
            _originalPipboyRootNifOnlyNode = pn->PipboyRoot_nif_only_node;
            _originalPipboyRootNifOnlyNode->local.scale = 0;
        }

        const auto pipboyReplacementNifPath = getPipboyReplacementNifPath();
        logger::info("Loading pipboy replacement nif '{}'", pipboyReplacementNifPath);
        const auto newPipboyRootNifOnlyNode = f4vr::loadNifFromFile(pipboyReplacementNifPath);
        const auto newScreen = f4vr::findNode(newPipboyRootNifOnlyNode, "Screen");
        if (!newScreen) {
            logger::error("Failed to find Pipboy screen node in the loaded nif!");
            return;
        }

        // replace the game Pipboy nif with ours
        pn->PipboyRoot_nif_only_node = newPipboyRootNifOnlyNode;
        pn->PipboyRoot_nif_only_node->local.scale = 0.0; // hide until opened
        pn->ScreenNode = newScreen;

        // attach where the 3rd-person Pipboy is on the arm
        const auto pipboyAttachNode = g_config.isFalloutLondonVR ? _skelly->getLeftArm().hand->IsNode() : getPipboyModelOnArmNode();
        pipboyAttachNode->AttachChild(newPipboyRootNifOnlyNode, false);

        _newPipboyRootNifOnlyNode = newPipboyRootNifOnlyNode;

        logger::info("Pipboy root nif replaced!");
    }

    /**
     * Show and hide the correct 3rd-person Pipboy on the arm mesh for either regular or holo screen type.
     */
    void Pipboy::showHideCorrectPipboyMesh(const std::string& itemHide, const std::string& itemShow) const
    {
        if (g_config.isFalloutLondonVR) {
            return;
        }
        if (const auto pipboy3Rd = getPipboyModelOnArmNode()) {
            pipboy3Rd->local.scale = g_config.pipBoyScale;

            if (const auto hideNode = f4vr::findNode(pipboy3Rd, itemHide.c_str())) {
                f4vr::setNodeVisibility(hideNode, false);
                hideNode->local.scale = 0;
            }
            if (const auto showNode = f4vr::findNode(pipboy3Rd, itemShow.c_str())) {
                f4vr::setNodeVisibility(showNode, true);
                showNode->local.scale = 1;
            }
            logger::info("Pipboy Meshes replaced! Hide: {}, Show: {}", itemHide.c_str(), itemShow.c_str());
        }
    }

    /**
     * If we created a Pipboy root nif we should detach and release it if no longer needed.
     * Like if changing out of wrist Pipboy mode or if switching the Pipboy model (regular/holo)
     */
    void Pipboy::detachReplacedPipboyRootNif()
    {
        if (_newPipboyRootNifOnlyNode) {
            if (_newPipboyRootNifOnlyNode->parent != nullptr) {
                logger::info("Detach current replaced pipboy root nif node");
                _newPipboyRootNifOnlyNode->parent->DetachChild(_newPipboyRootNifOnlyNode);
            } else {
                logger::warn("Replaced pipboy root nif node is already detached from parent!");
            }
            _newPipboyRootNifOnlyNode = nullptr;
        }
    }

    /**
     * Turn Pipboy off if "on" button was pressed (short press).
     */
    void Pipboy::checkTurningOnByButton()
    {
        if (_isOpen || g_frik.isMainConfigurationModeActive()) {
            return;
        }

        const bool open = _attaboyOnBeltNode && g_config.attaboyGrabActivationDistance > 0
            ? checkAttaboyActivation()
            : vrcf::VRControllers.isReleasedShort(vrcf::Hand::Offhand, g_config.pipBoyButtonID);
        if (open) {
            logger::info("Open Pipboy with button");
            openClose(open);
        }
    }

    /**
     * Turn Pipboy off if "off" button was pressed (short press).
     */
    void Pipboy::checkTurningOffByButton()
    {
        if (!_isOpen) {
            return;
        }

        const bool close = _attaboyOnBeltNode && g_config.attaboyGrabActivationDistance > 0
            ? checkAttaboyActivation()
            : vrcf::VRControllers.isReleasedShort(vrcf::Hand::Offhand, g_config.pipBoyButtonOffID);
        if (close) {
            logger::info("Close Pipboy with button");
            openClose(false);
        }
    }

    /**
     * Check if Fallout London Attaboy activation is triggered.
     * If configured, check that the left hand is close enough to the Attaboy on belt.
     */
    bool Pipboy::checkAttaboyActivation()
    {
        const float dist = MatrixUtils::vec3Len(_skelly->getLeftArm().hand->world.translate - _attaboyOnBeltNode->world.translate);
        if (dist < g_config.attaboyGrabActivationDistance) {
            if (!_attaboyGrabHapticActivated) {
                _attaboyGrabHapticActivated = true;
                triggerStrongHaptic(vrcf::Hand::Left);
                logger::debug("Attaboy activation area triggered");
            }
            if (vrcf::VRControllers.isReleasedShort(vrcf::Hand::Left, g_config.attaboyGrabButtonId)) {
                triggerShortHaptic(vrcf::Hand::Left);
                return true;
            }
        } else {
            _attaboyGrabHapticActivated = false; // move hand away for activation area
        }
        return false;
    }

    /**
     * Turn Pipboy on if player is looking at it for a certain amount of time.
     */
    void Pipboy::checkTurningOnByLookingAt()
    {
        if (_isOpen || !g_config.pipboyOpenWhenLookAt || !isPlayerLookingAtPipboy() || g_config.isFalloutLondonVR) {
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
        if (!_isOpen || g_config.isFalloutLondonVR) {
            return;
        }

        if (!g_config.pipboyCloseWhenLookAway && !g_config.pipboyCloseWhenMovingWhileLookingAway) {
            return;
        }

        if (isPlayerLookingAtPipboy()) {
            _lastLookingAtPip = nowMillis();
            return;
        }

        const auto movingStick = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Offhand);
        const auto lookingStick = vrcf::VRControllers.getThumbstickValue(vrcf::Hand::Primary);
        const bool isPlayerActing =
            fNotEqual(movingStick.x, 0, 0.3f)
            || fNotEqual(movingStick.y, 0, 0.3f)
            || fNotEqual(lookingStick.x, 0, 0.3f)
            || fNotEqual(lookingStick.y, 0, 0.3f)
            || vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Primary, vr::k_EButton_SteamVR_Trigger);

        const bool closeLookingWayWithDelay = g_config.pipboyCloseWhenLookAway
            && !g_frik.isPipboyConfigurationModeActive()
            && isNowTimePassed(_lastLookingAtPip, g_config.pipBoyOffDelay);

        const bool closeLookingWayWithMovement = isPlayerActing && g_config.pipboyCloseWhenMovingWhileLookingAway && !g_frik.isPipboyConfigurationModeActive();

        if (closeLookingWayWithDelay || closeLookingWayWithMovement) {
            logger::info("Close Pipboy when looking away: byDelay({}), byMovement({})", closeLookingWayWithDelay, closeLookingWayWithMovement);
            openClose(false);
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
        if (!_isOpen || g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::None) {
            return;
        }

        const auto pipboyScreen = f4vr::getPlayerNodes()->ScreenNode;
        if (!pipboyScreen) {
            return;
        }

        // for HoldInPlace the frame count is used to stabilize the Pipboy opening, I saw Pipboy not becoming visible if only 4 frames is used, use 12 to be safe
        const unsigned maxFrames = g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::HoldInPlace ? 12 : 4;

        _pipboyScreenPrevFrame.emplace_back(pipboyScreen->world.translate);
        if (_pipboyScreenPrevFrame.size() < maxFrames) {
            _pipboyScreenStableFrame = pipboyScreen->world;
            return;
        }
        if (_pipboyScreenPrevFrame.size() > maxFrames) {
            _pipboyScreenPrevFrame.pop_front();
        }

        if (g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::HoldInPlace) {
            holdPipboyScreenInPlace(pipboyScreen);
        } else {
            dampenPipboyScreenMovement(pipboyScreen);
        }
    }

    /**
     * Keep the Pipboy screen in place where it was opened unless offhand grip button is pressed to move it.
     */
    void Pipboy::holdPipboyScreenInPlace(RE::NiAVObject* const pipboyScreen)
    {
        if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Offhand, g_config.pipBoyButtonOffID, 0.3f)) {
            _pipboyScreenStableFrame = pipboyScreen->world;
        } else {
            pipboyScreen->world = _pipboyScreenStableFrame;
            f4vr::updateDown(pipboyScreen, false);
        }
    }

    /**
     * Small dampening of the screen movement but still always move the screen to be in the pipboy.
     */
    void Pipboy::dampenPipboyScreenMovement(RE::NiAVObject* pipboyScreen)
    {
        const auto threshold = g_config.dampenPipboyThreshold;
        const auto deltaPos = _pipboyScreenStableFrame.translate - _pipboyScreenPrevFrame[0];
        if (std::abs(deltaPos.x) > threshold || std::abs(deltaPos.y) > threshold || std::abs(deltaPos.z) > threshold) {
            // too much movement, don't dampen at all so the screen will not lag behind the pipboy
            _pipboyScreenStableFrame = pipboyScreen->world;
            return;
        }

        const auto prevFrame = _pipboyScreenStableFrame;

        // do a spherical interpolation between previous frame and current frame for the world rotation matrix
        Quaternion rq, rt;
        rq.fromMatrix(prevFrame.rotate);
        rt.fromMatrix(pipboyScreen->world.rotate);
        rq.slerp(1 - g_config.dampenPipboyMultiplier, rt);
        pipboyScreen->world.rotate = rq.getMatrix();

        // do a linear interpolation between the position from the previous frame to current frame
        const auto dampeningDelta = (pipboyScreen->world.translate - prevFrame.translate) * g_config.dampenPipboyMultiplier;
        pipboyScreen->world.translate -= dampeningDelta;

        _pipboyScreenStableFrame = pipboyScreen->world;
        _pipboyScreenStableFrame.translate += dampeningDelta * (1 - g_config.dampenPipboyMultiplier);

        f4vr::updateDown(pipboyScreen, false);
    }

    /**
     * Is the player currently looking at the Pipboy screen?
     * Handle different thresholds if Pipboy is on or off as looking away is more relaxed threshold.
     */
    bool Pipboy::isPlayerLookingAtPipboy() const
    {
        return isPlayerLookingAtPipboy(_isOpen);
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

            pipbone->local.rotate = MatrixUtils::getMatrixFromEulerAngles(0, MatrixUtils::degreesToRads(180.0), 0) * pipbone->local.rotate;
            pipbone->local.translate *= -1.5;
        }
    }

    RE::NiNode* Pipboy::getPipboyModelOnArmNode() const
    {
        if (f4vr::isInPowerArmor()) {
            return nullptr;
        }
        const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
        const auto boneNode = arm.forearm3 ? f4vr::findAVObject(arm.forearm3, "PipboyBone") : nullptr;
        return boneNode ? boneNode->IsNode() : arm.forearm3->IsNode();
    }
}
