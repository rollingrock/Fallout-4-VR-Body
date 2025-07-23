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
    void PipboyPhysicalHandler::operate(const PipboyPage lastPipboyPage, const bool isPBMessageBoxVisible)
    {
        // const auto fingerPos = _skelly->getIndexFingerTipWorldPosition(!g_config.leftHandedPipBoy);
        const auto fingerPos = _skelly->getBoneWorldTransform(g_config.leftHandedPipBoy ? "LArm_Finger23" : "RArm_Finger23").translate;
        checkHandStateToOperatePipboy(fingerPos);

        // operate buttons not requiring Pipboy to be on
        operatePowerButton(fingerPos);
        operateLightButton(fingerPos);
        operateRadioButton(fingerPos);

        updatePipboyPhysicalElements(lastPipboyPage, isPBMessageBoxVisible);

        if (!_pipboy->isOn()) {
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
    void PipboyPhysicalHandler::checkHandStateToOperatePipboy(const RE::NiPoint3 fingerPos)
    {
        const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
        if (_pipboy->isLookingAtPipBoy()) {
            const auto pipboy = f4vr::findNode(arm.shoulder, "PipboyRoot");
            const float distance = vec3Len(fingerPos - pipboy->world.translate);
            if (distance < g_config.pipboyDetectionRange && !_isOperatingPipboy && !_pipboy->isOn()) {
                // Hides Weapon and poses hand for pointing
                _isOperatingPipboy = true;
                setPipboyHandPose();
            }
            if (distance > g_config.pipboyDetectionRange && _isOperatingPipboy && !_pipboy->isOn()) {
                // Restores Weapon and releases hand pose
                _isOperatingPipboy = false;
                disablePipboyHandPose();
            }
        } else if (_isOperatingPipboy && !_pipboy->isOn()) {
            // Catches if you're not looking at the pipboy when your hand moves outside the control area and restores weapon / releases hand pose
            disablePipboyHandPose();
            // Remove any stuck helper orbs if Pipboy times out for any reason.
            for (const auto& orbIdx : ORBS_NAMES) {
                if (const auto orb = f4vr::findAVObject(arm.forearm3, orbIdx)) {
                    orb->local.scale = std::min<float>(orb->local.scale, 0);
                }
            }
            _isOperatingPipboy = false;
        }
    }

    /**
     * Handle the virtual power button interaction by detecting if the player's finger is near the button.
     * Handle sticky button press and toggle the Pipboy state.
     */
    void PipboyPhysicalHandler::operatePowerButton(const RE::NiPoint3 fingerPos)
    {
        const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
        const auto powerDetect = f4vr::findNode(arm.shoulder, "PowerDetect");
        const auto powerTranslate = f4vr::findAVObject(arm.forearm3, "PowerTranslate");
        if (!powerTranslate || !powerDetect) {
            return;
        }

        const float distance = vec3Len(fingerPos - powerDetect->world.translate);
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
            triggerShortHeptic();
            _pipboy->setOnOff(!_pipboy->isOn());
        }
    }

    /**
     * Handle the virtual light button interaction by detecting if the player's finger is near the button.
     * Handle sticky button press and toggle the Pipboy state.
     */
    void PipboyPhysicalHandler::operateLightButton(const RE::NiPoint3 fingerPos)
    {
        const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
        const auto lightDetect = f4vr::findNode(arm.shoulder, "LightDetect");
        const auto lightTranslate = f4vr::findAVObject(arm.forearm3, "LightTranslate");

        if (!lightTranslate || !lightDetect) {
            return;
        }
        const float distance = vec3Len(fingerPos - lightDetect->world.translate);
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
            triggerShortHeptic();
            if (!_pipboy->isOn()) {
                f4vr::togglePipboyLight(f4vr::getPlayer());
            }
        }
    }

    /**
     * Handle the virtual radio button interaction by detecting if the player's finger is near the button.
     * Handle sticky button press and toggle the Pipboy state.
     */
    void PipboyPhysicalHandler::operateRadioButton(const RE::NiPoint3 fingerPos)
    {
        const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
        const auto lightDetect = f4vr::findNode(arm.shoulder, "RadioDetect");
        const auto lightTranslate = f4vr::findAVObject(arm.forearm3, "RadioTranslate");

        if (!lightTranslate || !lightDetect) {
            return;
        }

        const float distance = vec3Len(fingerPos - lightDetect->world.translate);
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
            triggerShortHeptic();
            if (!_pipboy->isOn()) {
                if (f4vr::isPlayerRadioEnabled()) {
                    turnPlayerRadioOn(false);
                } else {
                    turnPlayerRadioOn(true);
                }
            }
        }
    }

    /**
     * Update the Pipboy elements (power button, light button, radio) based on the current state of the Pipboy.
     */
    void PipboyPhysicalHandler::updatePipboyPhysicalElements(const PipboyPage lastPipboyPage, const bool isPBMessageBoxVisible)
    {
        const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
        const auto powerOn = f4vr::findAVObject(arm.forearm3, "PowerButton_mesh:2");
        const auto powerOff = f4vr::findAVObject(arm.forearm3, "PowerButton_mesh:off");
        const auto lightOn = f4vr::findAVObject(arm.forearm3, "LightButton_mesh:2");
        const auto lightOff = f4vr::findAVObject(arm.forearm3, "LightButton_mesh:off");
        const auto radioOn = f4vr::findAVObject(arm.forearm3, "RadioOn");
        const auto radioOff = f4vr::findAVObject(arm.forearm3, "RadioOff");
        const auto radioNeedle = f4vr::findAVObject(arm.forearm3, "RadioNeedle_mesh");
        const auto radioKnob = f4vr::findAVObject(arm.forearm3, "ModeKnobDuplicate");
        const auto radioKnob2 = f4vr::findAVObject(arm.forearm3, "ModeKnob02");
        if (!powerOn || !powerOff || !lightOn || !lightOff || !radioOn || !radioOff || !radioNeedle || !radioKnob) {
            return;
        }

        if (lastPipboyPage == PipboyPage::RADIO) {
            // fixes broken 'Mode Knob' position when radio tab is selected
            float rotx;
            float roty;
            float rotz;
            getEulerAnglesFromMatrix(radioKnob->local.rotate, &rotx, &roty, &rotz);
            if (rotx < 0.57) {
                radioKnob->local.rotate = radioKnob->local.rotate * getMatrixFromEulerAngles(-0.05f, 0, 0);
            }
        } else {
            // restores control of the 'Mode Knob' to the Pipboy behaviour file
            radioKnob->local.rotate = radioKnob2->local.rotate;
        }

        // Controls Pipboy power light glow (on or off depending on Pipboy state)
        _pipboy->isOn() ? powerOn->flags.flags &= 0xfffffffffffffffe : powerOff->flags.flags &= 0xfffffffffffffffe;
        _pipboy->isOn() ? powerOn->local.scale = 1 : powerOff->local.scale = 1;
        _pipboy->isOn() ? powerOff->flags.flags |= 0x1 : powerOn->flags.flags |= 0x1;
        _pipboy->isOn() ? powerOff->local.scale = 0 : powerOn->local.scale = 0;

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

        if (g_frik.isPipboyConfigurationModeActive() || !g_config.enablePrimaryControllerPipboyUse) {
            return;
        }

        const auto doinantHandStick = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary, 0);
        const auto doinantTrigger = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary, 1);
        const auto secondaryTrigger = f4vr::VRControllers.getAxisValue(f4vr::Hand::Offhand, 1);

        // Move Pipboy trigger mesh with controller trigger position.
        if (const auto trans = f4vr::findAVObject(arm.forearm3, "SelectRotate")) {
            if (doinantTrigger.x > 0.00 && secondaryTrigger.x == 0.0) {
                trans->local.translate.z = doinantTrigger.x / 3 * -1;
            } else if (secondaryTrigger.x > 0.00 && doinantTrigger.x == 0.0) {
                trans->local.translate.z = secondaryTrigger.x / 3 * -1;
            } else {
                trans->local.translate.z = 0.00;
            }
        }

        if (lastPipboyPage != PipboyPage::MAP || isPBMessageBoxVisible) {
            const auto scrollKnob = f4vr::findAVObject(arm.forearm3, "ScrollItemsKnobRot");
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
                    Pipboy::execOperation(operation);
                }
            }
        }
    }

    void PipboyPhysicalHandler::pipboyManagement(const RE::NiPoint3 fingerPos)
    {
        // if (f4vr::isInPowerArmor()) {
        //     _lastRadioFreq = 0.0; // Ensures Radio needle doesn't get messed up when entering and then exiting Power Armor.
        //     // Continue to update Pipboy page info when in Power Armor.
        //     std::string pipboyMenu("PipboyMenu");
        //     auto menu = RE::UI::GetSingleton()->GetMenu(pipboyMenu);
        //     if (menu != nullptr) {
        //         auto root = menu->uiMovie.get();
        //         storeLastPipboyPage(root);
        //     }
        //     return;
        // }
        //
        // // Scale-form code for managing Pipboy menu controls (Virtual and Physical)
        // if (_pipboy->isOn()) {
        //     storeLastPipboyPage(root);
        //
        //     // Move Pipboy trigger mesh even if controls haven't been swapped.
        //     if (!g_frik.isPipboyConfigurationModeActive() && !g_config.enablePrimaryControllerPipboyUse) {
        //         const auto offHandStick = f4vr::VRControllers.getAxisValue(f4vr::Hand::Offhand, 0);
        //         const auto secondaryTrigger = f4vr::VRControllers.getAxisValue(f4vr::Hand::Offhand, 1);
        //
        //         const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
        //         if (const auto trans = f4vr::findAVObject(arm.forearm3, "SelectRotate")) {
        //             if (secondaryTrigger.x > 0.00) {
        //                 trans->local.translate.z = secondaryTrigger.x / 3 * -1;
        //             } else {
        //                 trans->local.translate.z = 0.00;
        //             }
        //         }
        //
        //         //still move Pipboy scroll knob even if controls haven't been swapped.
        //         // const bool isPBMessageBoxVisible = isMessageHolderVisible(root);
        //         // if (_lastPipboyPage != 3 || isPBMessageBoxVisible) {
        //         //     static std::string KnobNode = "ScrollItemsKnobRot";
        //         //     RE::NiAVObject* ScrollKnob = g_config.leftHandedPipBoy
        //         //         ? f4vr::findAVObject(_skelly->getRightArm().forearm3, KnobNode)
        //         //         : f4vr::findAVObject(_skelly->getLeftArm().forearm3, KnobNode);
        //         //     if (offHandStick.x > 0.85) {
        //         //         ScrollKnob->local.rotate = ScrollKnob->local.rotate * getMatrixFromEulerAngles(0, degreesToRads(0.4f), 0);
        //         //     }
        //         //     if (offHandStick.x < -0.85) {
        //         //         ScrollKnob->local.rotate = ScrollKnob->local.rotate * getMatrixFromEulerAngles(0, degreesToRads(-0.4f), 0);
        //         //     }
        //         // }
        //     }
        // }
    }
}
