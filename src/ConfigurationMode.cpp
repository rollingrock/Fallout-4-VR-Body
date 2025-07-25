#include "ConfigurationMode.h"

#include "Config.h"
#include "FRIK.h"
#include "HandPose.h"
#include "Skeleton.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/VRControllersManager.h"

using namespace common;

namespace frik
{
    constexpr const char* meshName[12] = {
        "PB-MainTitleTrans", "PB-Tile07Trans", "PB-Tile03Trans", "PB-Tile08Trans", "PB-Tile02Trans", "PB-Tile01Trans", "PB-Tile04Trans", "PB-Tile05Trans",
        "PB-Tile06Trans", "PB-Tile09Trans", "PB-Tile10Trans", "PB-Tile11Trans"
    };
    constexpr const char* meshName2[12] = {
        "PB-MainTitle", "PB-Tile07", "PB-Tile03", "PB-Tile08", "PB-Tile02", "PB-Tile01", "PB-Tile04", "PB-Tile05", "PB-Tile06", "PB-Tile09", "PB-Tile10",
        "PB-Tile11"
    };

    /**
     * Open Pipboy configuration mode which also requires Pipboy to be open.
     */
    void ConfigurationMode::openPipboyConfigurationMode()
    {
        enterPipboyConfigMode();
    }

    /**
     * Exit Main FRIK Config Mode
     */
    void ConfigurationMode::configModeExit()
    {
        _calibrationModeUIActive = false;
        if (auto c_MBox = f4vr::findNode(f4vr::getPlayerNodes()->playerworldnode, "messageBoxMenuWider")) {
            c_MBox->flags.flags &= ~0x1;
            c_MBox->local.scale = 1.0;
        }
        if (_calibrateModeActive) {
            std::fill(std::begin(_MCTouchbuttons), std::end(_MCTouchbuttons), false);
            if (auto MCConfigUI = f4vr::findAVObject(f4vr::getPlayerNodes()->primaryUIAttachNode, "MCCONFIGHUD")) {
                MCConfigUI->flags.flags |= 0x1;
                MCConfigUI->local.scale = 0;
                MCConfigUI->parent->DetachChild(MCConfigUI);
            }
            disableConfigModePose();
            _calibrateModeActive = false;
        }
    }

    void ConfigurationMode::exitPBConfig()
    {
        // Exit Pipboy Config Mode / remove UI.
        if (_isPBConfigModeActive) {
            for (int i = 0; i <= 11; i++) {
                _PBTouchbuttons[i] = false;
            }
            RE::NiAVObject* PBConfigUI = f4vr::findAVObject(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBCONFIGHUD");
            if (PBConfigUI) {
                PBConfigUI->flags.flags |= 0x1;
                PBConfigUI->local.scale = 0;
                PBConfigUI->parent->DetachChild(PBConfigUI);
            }
            disableConfigModePose();
            _isPBConfigModeActive = false;

            // restore pipboy scale if it was changed
            const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
            const auto pipboyScale = f4vr::findAVObject(arm.forearm3, "PipboyBone");
            pipboyScale->local.scale = g_config.pipBoyScale;
        }
    }

    void ConfigurationMode::mainConfigurationMode()
    {
        if (!_calibrateModeActive) {
            return;
        }

        setConfigModeHandPose();

        float rAxisOffsetY;
        const char* meshName[10] = {
            "MC-MainTitleTrans", "MC-Tile01Trans", "MC-Tile02Trans", "MC-Tile03Trans", "MC-Tile04Trans", "MC-Tile05Trans", "MC-Tile06Trans", "MC-Tile07Trans",
            "MC-Tile08Trans", "MC-Tile09Trans"
        };
        const char* meshName2[10] = {
            "MC-MainTitle", "MC-Tile01", "MC-Tile02", "MC-Tile03", "MC-Tile04", "MC-Tile05", "MC-Tile06", "MC-Tile07", "MC-Tile08", "MC-Tile09"
        };
        const char* meshName3[10] = { "", "", "", "", "", "", "", "MC-Tile07On", "MC-Tile08On", "MC-Tile09On" };
        const char* meshName4[4] = { "MC-ModeA", "MC-ModeB", "MC-ModeC", "MC-ModeD" };
        if (!_calibrationModeUIActive) {
            // Create Config UI
            f4vr::showMessagebox("FRIK Config Mode");
            RE::NiAVObject* c_MBox = f4vr::findNode(f4vr::getPlayerNodes()->playerworldnode, "messageBoxMenuWider");
            if (c_MBox) {
                c_MBox->flags.flags |= 0x1;
                c_MBox->local.scale = 0;
            }
            if (g_frik.isFavoritesMenuOpen()) {
                f4vr::closeFavoriteMenu();
                f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, 0.6f, 0.5f);
            }
            RE::NiNode* HUD = f4vr::getClonedNiNodeForNifFile("FRIK/UI-ConfigHUD.nif", "MCCONFIGHUD");
            // TODO: this should just use "primaryUIAttachNode" but it needs offset corrections, better just change to UI framework
            auto UIATTACH = f4vr::isLeftHandedMode()
                ? f4vr::getPlayerNodes()->primaryUIAttachNode
                : f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "world_primaryWand.nif");
            UIATTACH->AttachChild(HUD, true);
            const char* MainHud[10] = {
                "Data/Meshes/FRIK/UI-MainTitle.nif", "Data/Meshes/FRIK/UI-Tile01.nif", "Data/Meshes/FRIK/UI-Tile02.nif", "Data/Meshes/FRIK/UI-Tile03.nif",
                "Data/Meshes/FRIK/UI-Tile04.nif", "Data/Meshes/FRIK/UI-Tile05.nif", "Data/Meshes/FRIK/UI-Tile06.nif", "Data/Meshes/FRIK/UI-Tile07.nif",
                "Data/Meshes/FRIK/UI-Tile08.nif", "Data/Meshes/FRIK/UI-Tile09.nif"
            };
            const char* MainHud2[10] = {
                "Data/Meshes/FRIK/MC-MainTitle.nif", "Data/Meshes/FRIK/MC-Tile01.nif", "Data/Meshes/FRIK/MC-Tile02.nif", "Data/Meshes/FRIK/MC-Tile03.nif",
                "Data/Meshes/FRIK/MC-Tile04.nif", "Data/Meshes/FRIK/MC-Tile05.nif", "Data/Meshes/FRIK/MC-Tile06.nif", "Data/Meshes/FRIK/MC-Tile07.nif",
                "Data/Meshes/FRIK/MC-Tile08.nif", "Data/Meshes/FRIK/MC-Tile09.nif"
            };
            const char* MainHud3[4] = {
                "Data/Meshes/FRIK/MC-Tile09a.nif", "Data/Meshes/FRIK/MC-Tile09b.nif", "Data/Meshes/FRIK/MC-Tile09c.nif", "Data/Meshes/FRIK/MC-Tile09d.nif"
            };
            for (int i = 0; i <= 9; i++) {
                RE::NiNode* UI = f4vr::getClonedNiNodeForNifFile(MainHud[i], meshName2[i]);
                HUD->AttachChild(UI, true);
                RE::NiNode* UI2 = f4vr::getClonedNiNodeForNifFile(MainHud2[i], meshName[i]);
                UI->AttachChild(UI2, true);
                if (i == 7 || i == 8) {
                    RE::NiNode* UI3 = f4vr::getClonedNiNodeForNifFile("FRIK/UI-StickyMarker.nif", meshName3[i]);
                    UI2->AttachChild(UI3, true);
                }
                if (i == 9) {
                    for (int x = 0; x < 4; x++) {
                        RE::NiNode* UI3 = f4vr::getClonedNiNodeForNifFile(MainHud3[x], meshName4[x]);
                        UI2->AttachChild(UI3, true);
                    }
                }
            }
            _calibrationModeUIActive = true;
            _armLength_bkup = g_config.armLength;
            _powerArmor_up_bkup = g_config.powerArmor_up;
            _playerOffset_up_bkup = g_config.playerOffset_up;
            _rootOffset_bkup = g_config.rootOffset;
            _PARootOffset_bkup = g_config.PARootOffset;
            _fVrScale_bkup = g_config.fVrScale;
            _playerOffset_forward_bkup = g_config.playerOffset_forward;
            _powerArmor_forward_bkup = g_config.powerArmor_forward;
            _cameraHeight_bkup = g_config.cameraHeight;
            _PACameraHeight_bkup = g_config.PACameraHeight;
            enableGripButtonToGrap_bkup = g_config.enableGripButtonToGrap;
            onePressGripButton_bkup = g_config.onePressGripButton;
            enableGripButtonToLetGo_bkup = g_config.enableGripButtonToLetGo;
        } else {
            RE::NiAVObject* UIElement = nullptr;
            // Dampen Hands
            UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "MC-Tile07On");
            g_config.dampenHands ? UIElement->local.scale = 1 : UIElement->local.scale = 0;
            // Weapon Reposition Mode
            UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "MC-Tile08On");
            UIElement->local.scale = g_frik.inWeaponRepositionMode() ? 1 : 0;
            // Grip Mode
            if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
                // Standard Sticky Grip on / off
                for (int i = 0; i < 4; i++) {
                    if (i == 0) {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                        UIElement->local.scale = 1;
                    } else {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                        UIElement->local.scale = 0;
                    }
                }
            } else if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
                // Sticky Grip with button to release
                for (int i = 0; i < 4; i++) {
                    if (i == 1) {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                        UIElement->local.scale = 1;
                    } else {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                        UIElement->local.scale = 0;
                    }
                }
            } else if (g_config.enableGripButtonToGrap && g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
                // Button held to Grip
                for (int i = 0; i < 4; i++) {
                    if (i == 2) {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                        UIElement->local.scale = 1;
                    } else {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                        UIElement->local.scale = 0;
                    }
                }
            } else if (g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
                // button press to toggle Grip on or off
                for (int i = 0; i < 4; i++) {
                    if (i == 3) {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                        UIElement->local.scale = 1;
                    } else {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                        UIElement->local.scale = 0;
                    }
                }
            } else {
                //Not exepected - show no mode lable until button pressed
                for (int i = 0; i < 4; i++) {
                    UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4[i]);
                    UIElement->local.scale = 0;
                }
            }
            RE::NiPoint3 finger;
            f4vr::isLeftHandedMode()
                ? finger = _skelly->getBoneWorldTransform("RArm_Finger23").translate
                : finger = _skelly->getBoneWorldTransform("LArm_Finger23").translate;
            for (int i = 1; i <= 9; i++) {
                auto TouchMesh = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName2[i]);
                auto TransMesh = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName[i]);
                if (TouchMesh && TransMesh) {
                    float distance = vec3Len(finger - TouchMesh->world.translate);
                    if (distance > 2.0) {
                        TransMesh->local.translate.y = 0.0;
                        if (i == 7 || i == 8 || i == 9) {
                            _MCTouchbuttons[i] = false;
                        }
                    } else if (distance <= 2.0) {
                        float fz = 2.0 - distance;
                        if (fz > 0.0 && fz < 1.2) {
                            TransMesh->local.translate.y = fz;
                        }
                        if (TransMesh->local.translate.y > 1.0 && !_MCTouchbuttons[i]) {
                            //_PBConfigSticky = true;
                            f4vr::VRControllers.triggerHaptic(f4vr::Hand::Offhand);
                            for (int i = 1; i <= 7; i++) {
                                _MCTouchbuttons[i] = false;
                            }
                            auto UIMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "MCCONFIGMarker");
                            if (UIMarker) {
                                UIMarker->parent->DetachChild(UIMarker);
                            }
                            if (i < 7) {
                                RE::NiNode* UI = f4vr::getClonedNiNodeForNifFile("FRIK/UI-ConfigMarker.nif", "MCCONFIGMarker");
                                TouchMesh->AttachChild(UI, true);
                            }
                            _MCTouchbuttons[i] = true;
                        }
                    }
                }
            }
            vr::VRControllerAxis_t doinantHandStick = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0]
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0];
            uint64_t dominantHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed;
            uint64_t offHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed;
            bool CamZButtonPressed = _MCTouchbuttons[1];
            bool CamYButtonPressed = _MCTouchbuttons[2];
            bool ScaleButtonPressed = _MCTouchbuttons[3];
            bool BodyZButtonPressed = _MCTouchbuttons[4];
            bool BodyPoseButtonPressed = _MCTouchbuttons[5];
            bool ArmsButtonPressed = _MCTouchbuttons[6];
            bool HandsButtonPressed = _MCTouchbuttons[7];
            bool WeaponButtonPressed = _MCTouchbuttons[8];
            bool GripButtonPressed = _MCTouchbuttons[9];
            bool isInPA = f4vr::isInPowerArmor();
            if (HandsButtonPressed && !_isHandsButtonPressed) {
                _isHandsButtonPressed = true;
                g_config.dampenHands = !g_config.dampenHands;
            } else if (!HandsButtonPressed) {
                _isHandsButtonPressed = false;
            }
            if (WeaponButtonPressed && !_isWeaponButtonPressed) {
                _isWeaponButtonPressed = true;
                g_frik.toggleWeaponRepositionMode();
                // TODO: close main config on toggling this on
            } else if (!WeaponButtonPressed) {
                _isWeaponButtonPressed = false;
            }
            if (GripButtonPressed && !_isGripButtonPressed) {
                _isGripButtonPressed = true;
                if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
                    g_config.enableGripButtonToGrap = false;
                    g_config.onePressGripButton = false;
                    g_config.enableGripButtonToLetGo = true;
                } else if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
                    g_config.enableGripButtonToGrap = true;
                    g_config.onePressGripButton = true;
                    g_config.enableGripButtonToLetGo = false;
                } else if (g_config.enableGripButtonToGrap && g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
                    g_config.enableGripButtonToGrap = true;
                    g_config.onePressGripButton = false;
                    g_config.enableGripButtonToLetGo = true;
                } else if (g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
                    g_config.enableGripButtonToGrap = false;
                    g_config.onePressGripButton = false;
                    g_config.enableGripButtonToLetGo = false;
                } else {
                    //Not exepected - reset to standard sticky grip
                    g_config.enableGripButtonToGrap = false;
                    g_config.onePressGripButton = false;
                    g_config.enableGripButtonToLetGo = false;
                }
            } else if (!GripButtonPressed) {
                _isGripButtonPressed = false;
            }
            if (doinantHandStick.y > 0.10 && CamZButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                isInPA ? g_config.PACameraHeight += rAxisOffsetY : g_config.cameraHeight += rAxisOffsetY;
            }
            if (doinantHandStick.y < -0.10 && CamZButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                isInPA ? g_config.PACameraHeight += rAxisOffsetY : g_config.cameraHeight += rAxisOffsetY;
            }
            if (doinantHandStick.y > 0.10 && CamYButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 10;
                isInPA ? g_config.powerArmor_forward += rAxisOffsetY : g_config.playerOffset_forward -= rAxisOffsetY;
            }
            if (doinantHandStick.y < -0.10 && CamYButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 10;
                isInPA ? g_config.powerArmor_forward += rAxisOffsetY : g_config.playerOffset_forward -= rAxisOffsetY;
            }
            if (doinantHandStick.y > 0.10 && ScaleButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                g_config.fVrScale -= rAxisOffsetY;
                RE::Setting* set = RE::GetINISetting("fVrScale:VR");
                set->SetFloat(g_config.fVrScale);
            }
            if (doinantHandStick.y < -0.10 && ScaleButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                g_config.fVrScale -= rAxisOffsetY;
                RE::Setting* set = RE::GetINISetting("fVrScale:VR");
                set->SetFloat(g_config.fVrScale);
            }
            if (doinantHandStick.y > 0.10 && BodyZButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                isInPA ? g_config.PARootOffset += rAxisOffsetY : g_config.rootOffset += rAxisOffsetY;
            }
            if (doinantHandStick.y < -0.10 && BodyZButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                isInPA ? g_config.PARootOffset += rAxisOffsetY : g_config.rootOffset += rAxisOffsetY;
            }
            if (doinantHandStick.y > 0.10 && BodyPoseButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                isInPA ? g_config.powerArmor_up += rAxisOffsetY : g_config.playerOffset_up += rAxisOffsetY;
            }
            if (doinantHandStick.y < -0.10 && BodyPoseButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                isInPA ? g_config.powerArmor_up += rAxisOffsetY : g_config.playerOffset_up += rAxisOffsetY;
            }
            if (doinantHandStick.y > 0.10 && ArmsButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                g_config.armLength += rAxisOffsetY;
            }
            if (doinantHandStick.y < -0.10 && ArmsButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                g_config.armLength += rAxisOffsetY;
            }
        }
    }

    void ConfigurationMode::onFrameUpdate()
    {
        pipboyConfigurationMode();
        mainConfigurationMode();

        if (_calibrateModeActive) {
            vr::VRControllerAxis_t doinantHandStick = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0]
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0];
            const uint64_t dominantHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed;
            const uint64_t offHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed;
            const auto ExitandSave = dominantHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(33));
            const auto ExitnoSave = offHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(33));
            const auto SelfieButton = dominantHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(1));
            const auto HeightButton = offHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(1));
            if (ExitandSave && !_exitAndSavePressed) {
                _exitAndSavePressed = true;
                configModeExit();
                g_config.save();
                f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, 0.6f, 0.5f);
            } else if (!ExitandSave) {
                _exitAndSavePressed = false;
            }
            if (ExitnoSave && !_exitWithoutSavePressed) {
                _exitWithoutSavePressed = true;
                configModeExit();
                g_config.armLength = _armLength_bkup;
                g_config.powerArmor_up = _powerArmor_up_bkup;
                g_config.playerOffset_up = _playerOffset_up_bkup;
                g_config.rootOffset = _rootOffset_bkup;
                g_config.PARootOffset = _PARootOffset_bkup;
                g_config.fVrScale = _fVrScale_bkup;
                g_config.playerOffset_forward = _playerOffset_forward_bkup;
                g_config.powerArmor_forward = _powerArmor_forward_bkup;
                g_config.cameraHeight = _cameraHeight_bkup;
                g_config.PACameraHeight = _PACameraHeight_bkup;
                g_config.enableGripButtonToGrap = enableGripButtonToGrap_bkup;
                g_config.onePressGripButton = onePressGripButton_bkup;
                g_config.enableGripButtonToLetGo = enableGripButtonToLetGo_bkup;
            } else if (!ExitnoSave) {
                _exitWithoutSavePressed = false;
            }
            if (SelfieButton && !_selfieButtonPressed) {
                _selfieButtonPressed = true;
                g_frik.setSelfieMode(!g_frik.getSelfieMode());
            } else if (!SelfieButton) {
                _selfieButtonPressed = false;
            }
            if (HeightButton && !_UIHeightButtonPressed) {
                _UIHeightButtonPressed = true;
                // TODO: remove this height button
            } else if (!HeightButton) {
                _UIHeightButtonPressed = false;
            }
        } else {
            const uint64_t dominantHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed;
            const uint64_t offHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed;
            const auto dHTouch = dominantHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(32));
            const auto oHTouch = offHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(32));
            if (dHTouch && !_calibrateModeActive) {
                _configModeTimer += 1;
                if (_configModeTimer > 200 && _configModeTimer2 > 200) {
                    _dampenHandsButtonPressed = true;
                    _calibrateModeActive = true;
                }
            } else if (!dHTouch && !_calibrateModeActive && _configModeTimer > 0) {
                _configModeTimer = 0;
            }
            if (oHTouch && !_calibrateModeActive) {
                _configModeTimer2 += 1;
            } else if (!oHTouch && !_calibrateModeActive && _configModeTimer2 > 0) {
                _configModeTimer2 = 0;
            }
        }
    }

    /**
     * The Pipboy Configuration Mode function.
     */
    void ConfigurationMode::pipboyConfigurationMode()
    {
        if (g_frik.isPipboyOn()) {
            vr::VRControllerAxis_t doinantHandStick = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0]
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0];
            uint64_t dominantHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed;
            uint64_t offHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed;
            const auto PBConfigButtonPressed = dominantHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(32));
            bool ModelSwapButtonPressed = _PBTouchbuttons[1];
            bool RotateButtonPressed = _PBTouchbuttons[2];
            bool SaveButtonPressed = _PBTouchbuttons[3];
            bool ModelScaleButtonPressed = _PBTouchbuttons[4];
            bool ScaleButtonPressed = _PBTouchbuttons[5];
            bool MoveXButtonPressed = _PBTouchbuttons[6];
            bool MoveYButtonPressed = _PBTouchbuttons[7];
            bool MoveZButtonPressed = _PBTouchbuttons[8];
            bool ExitButtonPressed = _PBTouchbuttons[9];
            bool GlanceButtonPressed = _PBTouchbuttons[10];
            bool DampenScreenButtonPressed = _PBTouchbuttons[11];
            RE::NiAVObject* pbRoot = f4vr::findAVObject(f4vr::getPlayerNodes()->SecondaryWandNode, "PipboyRoot");
            if (!pbRoot) {
                return;
            }
            RE::NiAVObject* _3rdPipboy = nullptr;
            if (!g_config.leftHandedPipBoy) {
                if (_skelly->getLeftArm().forearm3) {
                    _3rdPipboy = f4vr::findAVObject(_skelly->getLeftArm().forearm3, "PipboyBone");
                }
            } else {
                _3rdPipboy = f4vr::findAVObject(_skelly->getRightArm().forearm3, "PipboyBone");
            }
            // Enter Pipboy Config Mode by holding down favorites button.
            if (PBConfigButtonPressed && !_isPBConfigModeActive) {
                // TODO: change from counter to timer so it will be fps independent
                _PBConfigModeEnterCounter += 1;
                if (_PBConfigModeEnterCounter > 200) {
                    enterPipboyConfigMode();
                }
            } else if (!PBConfigButtonPressed && !_isPBConfigModeActive) {
                _PBConfigModeEnterCounter = 0;
            }
            if (_isPBConfigModeActive) {
                float rAxisOffsetX;
                setConfigModeHandPose();

                RE::NiPoint3 finger;
                f4vr::isLeftHandedMode()
                    ? finger = _skelly->getBoneWorldTransform("RArm_Finger23").translate
                    : finger = _skelly->getBoneWorldTransform("LArm_Finger23").translate;
                for (int i = 1; i <= 11; i++) {
                    auto TouchMesh = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName2[i]);
                    auto TransMesh = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName[i]);
                    if (TouchMesh && TransMesh) {
                        float distance = vec3Len(finger - TouchMesh->world.translate);
                        if (distance > 2.0) {
                            TransMesh->local.translate.y = 0.0;
                            if (i == 1 || i == 3 || i == 10 || i == 11) {
                                _PBTouchbuttons[i] = false;
                            }
                        } else if (distance <= 2.0) {
                            float fz = 2.0 - distance;
                            if (fz > 0.0 && fz < 1.2) {
                                TransMesh->local.translate.y = fz;
                            }
                            if (TransMesh->local.translate.y > 1.0 && !_PBTouchbuttons[i]) {
                                //_PBConfigSticky = true;
                                f4vr::VRControllers.triggerHaptic(f4vr::Hand::Offhand);
                                for (int i = 1; i <= 11; i++) {
                                    if (i != 1 && i != 3) {
                                        _PBTouchbuttons[i] = false;
                                    }
                                }
                                auto UIMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBCONFIGMarker");
                                if (UIMarker) {
                                    UIMarker->parent->DetachChild(UIMarker);
                                }
                                if (i != 1 && i != 3 && i != 10 && i != 11) {
                                    RE::NiNode* UI = f4vr::getClonedNiNodeForNifFile("FRIK/UI-ConfigMarker.nif", "PBCONFIGMarker");
                                    TouchMesh->AttachChild(UI, true);
                                }
                                if (i == 10 || i == 11) {
                                    if (i == 10) {
                                        if (!g_config.pipboyOpenWhenLookAt) {
                                            auto UIMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBGlanceMarker");
                                            if (!UIMarker) {
                                                RE::NiNode* UI = f4vr::getClonedNiNodeForNifFile("FRIK/UI-ConfigMarker.nif", "PBGlanceMarker");
                                                TouchMesh->AttachChild(UI, true);
                                            }
                                        } else if (g_config.pipboyOpenWhenLookAt) {
                                            auto UIMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBGlanceMarker");
                                            if (UIMarker) {
                                                UIMarker->parent->DetachChild(UIMarker);
                                            }
                                        }
                                    }
                                    if (i == 11) {
                                        if (!g_config.dampenPipboyScreen) {
                                            auto UIMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBDampenMarker");
                                            if (!UIMarker) {
                                                RE::NiNode* UI = f4vr::getClonedNiNodeForNifFile("FRIK/UI-ConfigMarker.nif", "PBDampenMarker");
                                                TouchMesh->AttachChild(UI, true);
                                            }
                                        } else if (g_config.dampenPipboyScreen) {
                                            auto UIMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBDampenMarker");
                                            if (UIMarker) {
                                                UIMarker->parent->DetachChild(UIMarker);
                                            }
                                        }
                                    }
                                }
                                _PBTouchbuttons[i] = true;
                            }
                        }
                    }
                }
                if (SaveButtonPressed && !_isSaveButtonPressed) {
                    _isSaveButtonPressed = true;
                    g_config.savePipboyOffset(pbRoot->local);
                    g_config.savePipboyScale(_3rdPipboy->local.scale);
                } else if (!SaveButtonPressed) {
                    _isSaveButtonPressed = false;
                }
                if (GlanceButtonPressed && !_isGlanceButtonPressed) {
                    _isGlanceButtonPressed = true;
                    g_config.togglePipBoyOpenWhenLookAt();
                } else if (!GlanceButtonPressed) {
                    _isGlanceButtonPressed = false;
                }
                if (DampenScreenButtonPressed && !_isDampenScreenButtonPressed) {
                    _isDampenScreenButtonPressed = true;
                    g_config.toggleDampenPipboyScreen();
                } else if (!DampenScreenButtonPressed) {
                    _isDampenScreenButtonPressed = false;
                }
                if (ExitButtonPressed) {
                    exitPBConfig();
                }
                if (ModelSwapButtonPressed && !_isModelSwapButtonPressed) {
                    _isModelSwapButtonPressed = true;
                    g_frik.swapPipboyModel();
                } else if (!ModelSwapButtonPressed) {
                    _isModelSwapButtonPressed = false;
                }
                if ((doinantHandStick.y > 0.10 || doinantHandStick.y < -0.10) && RotateButtonPressed) {
                    rAxisOffsetX = doinantHandStick.y / 10;
                    if (rAxisOffsetX < 0) {
                        rAxisOffsetX = rAxisOffsetX * -1;
                    } else {
                        rAxisOffsetX = 0 - rAxisOffsetX;
                    }
                    pbRoot->local.rotate = getMatrixFromEulerAngles(degreesToRads(rAxisOffsetX), 0, 0) * pbRoot->local.rotate;
                }
                if (doinantHandStick.y > 0.10 && ScaleButtonPressed) {
                    pbRoot->local.scale = pbRoot->local.scale + 0.001;
                }
                if (doinantHandStick.y < -0.10 && ScaleButtonPressed) {
                    pbRoot->local.scale = pbRoot->local.scale - 0.001;
                }
                if (doinantHandStick.y > 0.10 && MoveXButtonPressed) {
                    rAxisOffsetX = doinantHandStick.y / 50;
                    pbRoot->local.translate.x = pbRoot->local.translate.x + rAxisOffsetX;
                }
                if (doinantHandStick.y < -0.10 && MoveXButtonPressed) {
                    rAxisOffsetX = doinantHandStick.y / 50;
                    pbRoot->local.translate.x = pbRoot->local.translate.x + rAxisOffsetX;
                }
                if (doinantHandStick.y > 0.10 && MoveYButtonPressed) {
                    rAxisOffsetX = doinantHandStick.y / 20;
                    pbRoot->local.translate.y = pbRoot->local.translate.y + rAxisOffsetX;
                }
                if (doinantHandStick.y < -0.10 && MoveYButtonPressed) {
                    rAxisOffsetX = doinantHandStick.y / 20;
                    pbRoot->local.translate.y = pbRoot->local.translate.y + rAxisOffsetX;
                }
                if (doinantHandStick.y > 0.10 && MoveZButtonPressed) {
                    rAxisOffsetX = doinantHandStick.y / 20;
                    pbRoot->local.translate.z = pbRoot->local.translate.z - rAxisOffsetX;
                }
                if (doinantHandStick.y < -0.10 && MoveZButtonPressed) {
                    rAxisOffsetX = doinantHandStick.y / 20;
                    pbRoot->local.translate.z = pbRoot->local.translate.z - rAxisOffsetX;
                }

                if (doinantHandStick.y > 0.10 && ModelScaleButtonPressed && _3rdPipboy) {
                    rAxisOffsetX = doinantHandStick.y / 65;
                    _3rdPipboy->local.scale += rAxisOffsetX;
                }
                if (doinantHandStick.y < -0.10 && ModelScaleButtonPressed && _3rdPipboy) {
                    rAxisOffsetX = doinantHandStick.y / 65;
                    _3rdPipboy->local.scale += rAxisOffsetX;
                }
            }
        }
    }

    void ConfigurationMode::enterPipboyConfigMode()
    {
        if (g_frik.isFavoritesMenuOpen()) {
            f4vr::closeFavoriteMenu();
        }
        f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, 0.6f, 0.5f);
        RE::NiNode* HUD = f4vr::getClonedNiNodeForNifFile("FRIK/UI-ConfigHUD.nif", "PBCONFIGHUD");
        // TODO: this should just use "primaryUIAttachNode" but it needs offset corrections, better just change to UI framework
        auto UIATTACH = f4vr::isLeftHandedMode()
            ? f4vr::getPlayerNodes()->primaryUIAttachNode
            : f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "world_primaryWand.nif");
        UIATTACH->AttachChild(HUD, true);
        const char* MainHud[12] = {
            "Data/Meshes/FRIK/UI-MainTitle.nif", "Data/Meshes/FRIK/UI-Tile07.nif", "Data/Meshes/FRIK/UI-Tile03.nif", "Data/Meshes/FRIK/UI-Tile08.nif",
            "Data/Meshes/FRIK/UI-Tile02.nif", "Data/Meshes/FRIK/UI-Tile01.nif", "Data/Meshes/FRIK/UI-Tile04.nif", "Data/Meshes/FRIK/UI-Tile05.nif",
            "Data/Meshes/FRIK/UI-Tile06.nif", "Data/Meshes/FRIK/UI-Tile09.nif", "Data/Meshes/FRIK/UI-Tile10.nif", "Data/Meshes/FRIK/UI-Tile11.nif"
        };
        const char* MainHud2[12] = {
            "Data/Meshes/FRIK/PB-MainTitle.nif", "Data/Meshes/FRIK/PB-Tile07.nif", "Data/Meshes/FRIK/PB-Tile03.nif", "Data/Meshes/FRIK/PB-Tile08.nif",
            "Data/Meshes/FRIK/PB-Tile02.nif", "Data/Meshes/FRIK/PB-Tile01.nif", "Data/Meshes/FRIK/PB-Tile04.nif", "Data/Meshes/FRIK/PB-Tile05.nif",
            "Data/Meshes/FRIK/PB-Tile06.nif", "Data/Meshes/FRIK/PB-Tile09.nif", "Data/Meshes/FRIK/PB-Tile10.nif", "Data/Meshes/FRIK/PB-Tile11.nif"
        };
        for (int i = 0; i <= 11; i++) {
            RE::NiNode* UI = f4vr::getClonedNiNodeForNifFile(MainHud[i], meshName2[i]);
            HUD->AttachChild(UI, true);

            RE::NiNode* UI2 = f4vr::getClonedNiNodeForNifFile(MainHud2[i], meshName[i]);
            UI->AttachChild(UI2, true);

            if (i == 10 && g_config.pipboyOpenWhenLookAt) {
                RE::NiNode* UI3 = f4vr::getClonedNiNodeForNifFile("FRIK/UI-ConfigMarker.nif", "PBGlanceMarker");
                UI->AttachChild(UI3, true);
            }
            if (i == 11 && g_config.dampenPipboyScreen) {
                RE::NiNode* UI3 = f4vr::getClonedNiNodeForNifFile("FRIK/UI-ConfigMarker.nif", "PBDampenMarker");
                UI->AttachChild(UI3, true);
            }
        }
        _isPBConfigModeActive = true;
        _PBConfigModeEnterCounter = 0;
    }
}
