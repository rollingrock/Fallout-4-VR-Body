#include "PipboyPhysicalHandler.h"

#include "Config.h"
#include "FRIK.h"
#include "HandPose.h"
#include "f4vr/VRControllersManager.h"

using namespace common;

namespace frik
{
    /**
     * Manages all aspects of virtual-physical Pipboy usage outside of turning the device / radio / torch on or off.
     * See documentation: https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Development-%E2%80%90-Pipboy-Controls
     */
    void PipboyPhysicalHandler::operate(const PipboyPage lastPipboyPage)
    {
        if (f4vr::isWeaponEquipped()) {
            // no physical Pipboy operation if the player has a weapon equipped
            updatePipboyPhysicalElements(lastPipboyPage);
            return;
        }

        const auto fingerPos = _skelly->getBoneWorldTransform(g_config.leftHandedPipBoy ? "LArm_Finger23" : "RArm_Finger23").translate;

        const auto arm = getPipboyArmNode();
        const auto powerButton = arm ? f4vr::findNode(arm, "PowerDetect") : nullptr;
        const auto lightButton = arm ? f4vr::findNode(arm, "LightDetect") : nullptr;
        const auto radioButton = arm ? f4vr::findNode(arm, "RadioDetect") : nullptr;
        if (!powerButton || !lightButton || !radioButton) {
            return;
        }

        checkHandStateToOperatePipboy(fingerPos, powerButton, lightButton, radioButton);

        // operate buttons not requiring Pipboy to be on
        if (f4vr::isPipboyOnWrist()) {
            operatePowerButton(fingerPos, powerButton);
        }
        operateLightButton(fingerPos, lightButton);
        operateRadioButton(fingerPos, radioButton);

        updatePipboyPhysicalElements(lastPipboyPage);

        if (!_pipboy->isOpen()) {
            return;
        }

        // Operate the Pipboy 7 control points when it is on
        for (int i = 0; i < 7; i++) {
            operatePipboyPhysicalElement(fingerPos, static_cast<PipboyOperation>(i));
        }
    }

    /**
     * Check if a hand is in the correct position to operate the Pipboy.
     * Enabling operation state by hiding the weapon and setting the hand pose for pointing.
     * Disabling operation state if the hand moves outside the control area or if the Pipboy is not looking at it.
     */
    void PipboyPhysicalHandler::checkHandStateToOperatePipboy(
        const RE::NiPoint3 fingerPos, const RE::NiNode* powerButton, const RE::NiNode* lightButton, const RE::NiNode* radioButton)
    {
        const bool wasOperating = _isOperatingPipboy;

        // have a buffer zone for finger detection not to trash the hand pose
        const float pipboyDetectionRange = g_config.pipboyOperationFingerDetectionRange + (_isOperatingPipboy ? 1.0f : -1.0f);
        _isOperatingPipboy = vec3Len(fingerPos - powerButton->world.translate) < pipboyDetectionRange
            || vec3Len(fingerPos - lightButton->world.translate) < pipboyDetectionRange
            || vec3Len(fingerPos - radioButton->world.translate) < pipboyDetectionRange;

        if (wasOperating == _isOperatingPipboy) {
            return;
        }

        if (_isOperatingPipboy) {
            setPipboyHandPose();
        } else {
            disablePipboyHandPose();
            // Remove any stuck helper orbs if Pipboy times out for any reason.
            const auto arm = getPipboyArmNode();
            for (const auto& orbIdx : ORBS_NAMES) {
                if (const auto orb = f4vr::findAVObject(arm, orbIdx)) {
                    orb->local.scale = std::min<float>(orb->local.scale, 0);
                }
            }
        }
    }

    /**
     * Handle the virtual power button interaction by detecting if the player's finger is near the button.
     * Handle sticky button press and toggle the Pipboy state.
     */
    void PipboyPhysicalHandler::operatePowerButton(const RE::NiPoint3 fingerPos, const RE::NiNode* powerButton)
    {
        const auto powerTranslate = f4vr::findAVObject(getPipboyArmNode(), "PowerTranslate");

        const float distance = vec3Len(fingerPos - powerButton->world.translate);
        if (distance > 3) {
            _stickyPower = false;
            powerTranslate->local.translate.z = 0.0;
            return;
        }

        const float fz = 0 - (2.0f - distance);
        if (fz >= -0.14 && fz <= 0.0) {
            powerTranslate->local.translate.z = fz;
        }
        if (powerTranslate->local.translate.z < -0.10 && !_stickyPower) {
            _stickyPower = true;
            triggerShortHaptic();
            _pipboy->openClose(!_pipboy->isOpen());
        }
    }

    /**
     * Handle the virtual light button interaction by detecting if the player's finger is near the button.
     * Handle sticky button press and toggle the Pipboy state.
     */
    void PipboyPhysicalHandler::operateLightButton(const RE::NiPoint3 fingerPos, const RE::NiNode* lightButton)
    {
        const auto lightTranslate = f4vr::findAVObject(getPipboyArmNode(), "LightTranslate");

        const float distance = vec3Len(fingerPos - lightButton->world.translate);
        if (distance > 2.0) {
            _stickyLight = false;
            lightTranslate->local.translate.z = 0.0;
            return;
        }

        const float fz = 0 - (2.0f - distance);
        if (fz >= -0.2 && fz <= 0.0) {
            lightTranslate->local.translate.z = fz;
        }
        if (lightTranslate->local.translate.z < -0.14 && !_stickyLight) {
            _stickyLight = true;
            triggerShortHaptic();
            if (!_pipboy->isOpen()) {
                f4vr::togglePipboyLight(f4vr::getPlayer());
            }
        }
    }

    /**
     * Handle the virtual radio button interaction by detecting if the player's finger is near the button.
     * Handle sticky button press and toggle the Pipboy state.
     */
    void PipboyPhysicalHandler::operateRadioButton(const RE::NiPoint3 fingerPos, const RE::NiNode* radioButton)
    {
        const auto arm = getPipboyArmNode();
        const auto lightTranslate = f4vr::findAVObject(arm, "RadioTranslate");

        const float distance = vec3Len(fingerPos - radioButton->world.translate);
        if (distance > 2.0) {
            _stickyRadio = false;
            lightTranslate->local.translate.y = 0.0;
            return;
        }
        const float fz = 0 - (2.0f - distance);
        if (fz >= -0.15 && fz <= 0.0) {
            lightTranslate->local.translate.y = fz;
        }
        if (lightTranslate->local.translate.y < -0.12 && !_stickyRadio) {
            _stickyRadio = true;
            triggerShortHaptic();
            if (f4vr::isPlayerRadioEnabled()) {
                turnPlayerRadioOn(false);
            } else {
                turnPlayerRadioOn(true);
            }
        }
    }

    /**
     * Update the Pipboy elements (power button, light button, radio) based on the current state of the Pipboy.
     */
    void PipboyPhysicalHandler::updatePipboyPhysicalElements(const PipboyPage lastPipboyPage)
    {
        const auto arm = getPipboyArmNode();
        const auto pageKnob = f4vr::findAVObject(arm, "ModeKnobDuplicate");
        const auto pageKnob2 = f4vr::findAVObject(arm, "ModeKnob02");
        const auto powerOn = f4vr::findAVObject(arm, "PowerButton_mesh:2");
        const auto powerOff = f4vr::findAVObject(arm, "PowerButton_mesh:off");
        const auto lightOn = f4vr::findAVObject(arm, "LightButton_mesh:2");
        const auto lightOff = f4vr::findAVObject(arm, "LightButton_mesh:off");
        const auto radioOn = f4vr::findAVObject(arm, "RadioOn");
        const auto radioOff = f4vr::findAVObject(arm, "RadioOff");
        const auto radioNeedle = f4vr::findAVObject(arm, "RadioNeedle_mesh");
        if (!powerOn || !powerOff || !lightOn || !lightOff || !radioOn || !radioOff || !radioNeedle || !pageKnob) {
            return;
        }

        if (f4vr::isPipboyOnWrist()) {
            if (lastPipboyPage == PipboyPage::RADIO) {
                // fixes broken 'Mode Knob' position when radio tab is selected
                float rotx;
                float roty;
                float rotz;
                getEulerAnglesFromMatrix(pageKnob->local.rotate, &rotx, &roty, &rotz);
                if (rotx < 0.57) {
                    pageKnob->local.rotate = pageKnob->local.rotate * getMatrixFromEulerAngles(-0.05f, 0, 0);
                }
            } else {
                // restores control of the 'Mode Knob' to the Pipboy behaviour file
                pageKnob->local.rotate = pageKnob2->local.rotate;
            }

            // Controls Pipboy power light glow (on or off depending on Pipboy state)
            _pipboy->isOpen() ? powerOn->flags.flags &= 0xfffffffffffffffe : powerOff->flags.flags &= 0xfffffffffffffffe;
            _pipboy->isOpen() ? powerOn->local.scale = 1 : powerOff->local.scale = 1;
            _pipboy->isOpen() ? powerOff->flags.flags |= 0x1 : powerOn->flags.flags |= 0x1;
            _pipboy->isOpen() ? powerOff->local.scale = 0 : powerOn->local.scale = 0;
        }

        // Controls light on & off indicators
        const bool isLightOn = f4vr::isPipboyLightOn(f4vr::getPlayer());
        isLightOn ? lightOn->flags.flags &= 0xfffffffffffffffe : lightOff->flags.flags &= 0xfffffffffffffffe;
        isLightOn ? lightOn->local.scale = 1 : lightOff->local.scale = 1;
        isLightOn ? lightOff->flags.flags |= 0x1 : lightOn->flags.flags |= 0x1;
        isLightOn ? lightOff->local.scale = 0 : lightOn->local.scale = 0;

        // Controls radio on & off indicators
        const bool isRadioOn = f4vr::isPlayerRadioEnabled();
        isRadioOn ? radioOn->flags.flags &= 0xfffffffffffffffe : radioOff->flags.flags &= 0xfffffffffffffffe;
        isRadioOn ? radioOn->local.scale = 1 : radioOff->local.scale = 1;
        isRadioOn ? radioOff->flags.flags |= 0x1 : radioOn->flags.flags |= 0x1;
        isRadioOn ? radioOff->local.scale = 0 : radioOn->local.scale = 0;

        // Controls Radio Needle Position.
        const float radioFreq = f4vr::getPlayerRadioFreq() - 23;
        if (isRadioOn && fNotEqual(radioFreq, _lastRadioFreq)) {
            const float x = -1 * (radioFreq - _lastRadioFreq);
            radioNeedle->local.rotate = radioNeedle->local.rotate * getMatrixFromEulerAngles(0, degreesToRads(x), 0);
            _lastRadioFreq = radioFreq;
        } else if (!isRadioOn && _lastRadioFreq > 0) {
            const float x = _lastRadioFreq;
            radioNeedle->local.rotate = radioNeedle->local.rotate * getMatrixFromEulerAngles(0, degreesToRads(x), 0);
            _lastRadioFreq = 0.0;
        }

        if (g_frik.isPipboyConfigurationModeActive() || !g_config.enablePrimaryControllerPipboyUse || !f4vr::isPipboyOnWrist()) {
            return;
        }

        const auto doinantHandStick = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary, f4vr::Axis::Thumbstick);
        const auto doinantTrigger = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary, f4vr::Axis::Trigger);
        const auto secondaryTrigger = f4vr::VRControllers.getAxisValue(f4vr::Hand::Offhand, f4vr::Axis::Trigger);

        // Move Pipboy trigger mesh with controller trigger position.
        if (const auto trans = f4vr::findAVObject(arm, "SelectRotate")) {
            if (doinantTrigger.x > 0.00 && secondaryTrigger.x == 0.0) {
                trans->local.translate.z = doinantTrigger.x / 3 * -1;
            } else if (secondaryTrigger.x > 0.00 && doinantTrigger.x == 0.0) {
                trans->local.translate.z = secondaryTrigger.x / 3 * -1;
            } else {
                trans->local.translate.z = 0.00;
            }
        }

        const bool isPBMessageBoxVisible = PipboyOperationHandler::isMessageHolderVisible(PipboyOperationHandler::getPipboyMenuRoot());
        if (lastPipboyPage != PipboyPage::MAP || isPBMessageBoxVisible) {
            const auto scrollKnob = f4vr::findAVObject(arm, "ScrollItemsKnobRot");
            if (doinantHandStick.y > 0.85) {
                scrollKnob->local.rotate = scrollKnob->local.rotate * getMatrixFromEulerAngles(0, degreesToRads(0.4f), 0);
            }
            if (doinantHandStick.y < -0.85) {
                scrollKnob->local.rotate = scrollKnob->local.rotate * getMatrixFromEulerAngles(0, degreesToRads(-0.4f), 0);
            }
        }
    }

    /**
     * 
     */
    void PipboyPhysicalHandler::operatePipboyPhysicalElement(const RE::NiPoint3 fingerPos, const PipboyOperation operation)
    {
        const int opIdx = static_cast<int>(operation);
        const auto armBone = (g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm()).forearm3;
        const auto bone = f4vr::findAVObject(armBone, BONES_NAMES[opIdx]);
        const auto trans = f4vr::findAVObject(armBone, TRANS_NAMES[opIdx]);
        const auto orbName = ORBS_NAMES[opIdx];
        const float boneDistance = BONES_DISTANCES[opIdx];
        const float transDistance = TRANS_DISTANCES[opIdx];
        const float maxDistance = MAX_DISTANCES[opIdx];
        bool& controlSticky = _controlsSticky[opIdx];

        if (bone && trans) {
            const float distance = vec3Len(fingerPos - bone->world.translate);
            if (distance > boneDistance) {
                trans->local.translate.z = 0.0;
                controlSticky = false;
                RE::NiAVObject* orb = g_config.leftHandedPipBoy
                    ? f4vr::findAVObject(_skelly->getRightArm().forearm3, orbName)
                    : f4vr::findAVObject(_skelly->getLeftArm().forearm3, orbName); //Hide helper Orbs when not near a control surface
                if (orb != nullptr) {
                    orb->local.scale = std::min<float>(orb->local.scale, 0);
                }
            } else if (distance <= boneDistance) {
                const float fz = boneDistance - distance;
                RE::NiAVObject* orb = g_config.leftHandedPipBoy
                    ? f4vr::findAVObject(_skelly->getRightArm().forearm3, orbName)
                    : f4vr::findAVObject(_skelly->getLeftArm().forearm3, orbName); //Show helper Orbs when not near a control surface
                if (orb != nullptr) {
                    orb->local.scale = std::max<float>(orb->local.scale, 1);
                }
                if (fz > 0.0 && fz < maxDistance) {
                    trans->local.translate.z = fz;
                    if (operation == PipboyOperation::MOVE_LIST_SELECTION_UP) {
                        // Move Scroll Knob Anti-Clockwise when near control surface
                        static std::string KnobNode = "ScrollItemsKnobRot";
                        RE::NiAVObject* ScrollKnob = g_config.leftHandedPipBoy
                            ? f4vr::findAVObject(_skelly->getRightArm().forearm3, KnobNode)
                            : f4vr::findAVObject(_skelly->getLeftArm().forearm3, KnobNode);
                        ScrollKnob->local.rotate = ScrollKnob->local.rotate * getMatrixFromEulerAngles(0, degreesToRads(fz), 0);
                    } else if (operation == PipboyOperation::MOVE_LIST_SELECTION_DOWN) {
                        // Move Scroll Knob Clockwise when near control surface
                        const float roty = fz * -1;
                        static std::string KnobNode = "ScrollItemsKnobRot";
                        RE::NiAVObject* ScrollKnob = g_config.leftHandedPipBoy
                            ? f4vr::findAVObject(_skelly->getRightArm().forearm3, KnobNode)
                            : f4vr::findAVObject(_skelly->getLeftArm().forearm3, KnobNode);
                        ScrollKnob->local.rotate = ScrollKnob->local.rotate * getMatrixFromEulerAngles(0, degreesToRads(roty), 0);
                    }
                }
                if (trans->local.translate.z > transDistance && !controlSticky) {
                    controlSticky = true;
                    PipboyOperationHandler::exec(operation);
                }
            }
        }
    }

    RE::NiAVObject* PipboyPhysicalHandler::getPipboyArmNode() const
    {
        return (g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm()).forearm3;
    }
}
