#include "ConfigurationMode.h"

#include "Config.h"
#include "FRIK.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/VRControllersManager.h"
#include "skeleton/HandPose.h"
#include "skeleton/Skeleton.h"

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
            f4vr::VRControllers.reset();
        }
    }

    void ConfigurationMode::exitPBConfig()
    {
        // Exit Pipboy Config Mode / remove UI.
        if (_isPBConfigModeActive) {
            for (int i = 0; i <= 11; i++) {
                _PBTouchbuttons[i] = false;
            }
            if (auto PBConfigUI = f4vr::findAVObject(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBCONFIGHUD")) {
                PBConfigUI->flags.flags |= 0x1;
                PBConfigUI->local.scale = 0;
                PBConfigUI->parent->DetachChild(PBConfigUI);
            }
            disableConfigModePose();
            _isPBConfigModeActive = false;

            // restore pipboy scale if it was changed
            const auto arm = g_config.leftHandedPipBoy ? _skelly->getRightArm() : _skelly->getLeftArm();
            if (const auto pipboyScale = f4vr::findAVObject(arm.forearm3, "PipboyBone")) {
                pipboyScale->local.scale = g_config.pipBoyScale;
            }
        }
    }

    void ConfigurationMode::mainConfigurationMode()
    {
        if (!_calibrateModeActive) {
            return;
        }

        setConfigModeHandPose();

        float rAxisOffsetY;
        const char* meshName_local[10] = {
            "MC-MainTitleTrans", "MC-Tile01Trans", "MC-Tile02Trans", "MC-Tile03Trans", "MC-Tile04Trans", "MC-Tile05Trans", "MC-Tile06Trans", "MC-Tile07Trans",
            "MC-Tile08Trans", "MC-Tile09Trans"
        };
        const char* meshName2_local[10] = {
            "MC-MainTitle", "MC-Tile01", "MC-Tile02", "MC-Tile03", "MC-Tile04", "MC-Tile05", "MC-Tile06", "MC-Tile07", "MC-Tile08", "MC-Tile09"
        };
        const char* meshName3_local[10] = { "", "", "", "", "", "", "", "MC-Tile07On", "MC-Tile08On", "MC-Tile09On" };
        const char* meshName4_local[4] = { "MC-ModeA", "MC-ModeB", "MC-ModeC", "MC-ModeD" };
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
            RE::NiNode* HUD = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigHUD.nif", "MCCONFIGHUD");
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
                RE::NiNode* UI = f4vr::getClonedNiNodeForNifFileSetName(MainHud[i], meshName2_local[i]);
                HUD->AttachChild(UI, true);
                RE::NiNode* UI2 = f4vr::getClonedNiNodeForNifFileSetName(MainHud2[i], meshName_local[i]);
                UI->AttachChild(UI2, true);
                if (i == 7 || i == 8) {
                    RE::NiNode* UI3 = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-StickyMarker.nif", meshName3_local[i]);
                    UI2->AttachChild(UI3, true);
                }
                if (i == 9) {
                    for (int x = 0; x < 4; x++) {
                        RE::NiNode* UI3 = f4vr::getClonedNiNodeForNifFileSetName(MainHud3[x], meshName4_local[x]);
                        UI2->AttachChild(UI3, true);
                    }
                }
            }
            _calibrationModeUIActive = true;
            _armLength_bkup = g_config.armLength;
            _fVrScale_bkup = g_config.fVrScale;
            _playerBodyOffsetUpStanding_bkup = g_config.playerBodyOffsetUpStanding;
            _playerBodyOffsetForwardStanding_bkup = g_config.playerBodyOffsetForwardStanding;
            _playerHMDOffsetUpStanding_bkup = g_config.playerHMDOffsetUpStanding;
            _playerBodyOffsetUpSitting_bkup = g_config.playerBodyOffsetUpSitting;
            _playerBodyOffsetForwardSitting_bkup = g_config.playerBodyOffsetForwardSitting;
            _playerHMDOffsetUpSitting_bkup = g_config.playerHMDOffsetUpSitting;
            _playerBodyOffsetUpStandingInPA_bkup = g_config.playerBodyOffsetUpStandingInPA;
            _playerBodyOffsetForwardStandingInPA_bkup = g_config.playerBodyOffsetForwardStandingInPA;
            _playerHMDOffsetUpStandingInPA_bkup = g_config.playerHMDOffsetUpStandingInPA;
            _playerBodyOffsetUpSittingInPA_bkup = g_config.playerBodyOffsetUpSittingInPA;
            _playerBodyOffsetForwardSittingInPA_bkup = g_config.playerBodyOffsetForwardSittingInPA;
            _playerHMDOffsetUpSittingInPA_bkup = g_config.playerHMDOffsetUpSittingInPA;
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
            UIElement->local.scale = g_frik.inWeaponRepositionMode() ? 1.0f : 00.f;
            // Grip Mode
            if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
                // Standard Sticky Grip on / off
                for (int i = 0; i < 4; i++) {
                    if (i == 0) {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                        UIElement->local.scale = 1;
                    } else {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                        UIElement->local.scale = 0;
                    }
                }
            } else if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
                // Sticky Grip with button to release
                for (int i = 0; i < 4; i++) {
                    if (i == 1) {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                        UIElement->local.scale = 1;
                    } else {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                        UIElement->local.scale = 0;
                    }
                }
            } else if (g_config.enableGripButtonToGrap && g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
                // Button held to Grip
                for (int i = 0; i < 4; i++) {
                    if (i == 2) {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                        UIElement->local.scale = 1;
                    } else {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                        UIElement->local.scale = 0;
                    }
                }
            } else if (g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
                // button press to toggle Grip on or off
                for (int i = 0; i < 4; i++) {
                    if (i == 3) {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                        UIElement->local.scale = 1;
                    } else {
                        UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                        UIElement->local.scale = 0;
                    }
                }
            } else {
                //Not exepected - show no mode lable until button pressed
                for (int i = 0; i < 4; i++) {
                    UIElement = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName4_local[i]);
                    UIElement->local.scale = 0;
                }
            }
            RE::NiPoint3 finger;
            f4vr::isLeftHandedMode()
                ? finger = _skelly->getBoneWorldTransform("RArm_Finger23").translate
                : finger = _skelly->getBoneWorldTransform("LArm_Finger23").translate;
            for (int i = 1; i <= 9; i++) {
                auto TouchMesh = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName2_local[i]);
                auto TransMesh = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName_local[i]);
                if (TouchMesh && TransMesh) {
                    float distance = vec3Len(finger - TouchMesh->world.translate);
                    if (distance > 2.0) {
                        TransMesh->local.translate.y = 0.0;
                        if (i == 7 || i == 8 || i == 9) {
                            _MCTouchbuttons[i] = false;
                        }
                    } else if (distance <= 2.0) {
                        float fz = 2.0f - distance;
                        if (fz > 0.0f && fz < 1.2f) {
                            TransMesh->local.translate.y = fz;
                        }
                        if (TransMesh->local.translate.y > 1.0 && !_MCTouchbuttons[i]) {
                            //_PBConfigSticky = true;
                            f4vr::VRControllers.triggerHaptic(f4vr::Hand::Offhand);
                            for (int j = 1; j <= 7; j++) {
                                _MCTouchbuttons[j] = false;
                            }
                            auto UIMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "MCCONFIGMarker");
                            if (UIMarker) {
                                UIMarker->parent->DetachChild(UIMarker);
                            }
                            if (i < 7) {
                                RE::NiNode* UI = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigMarker.nif", "MCCONFIGMarker");
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
            bool CamZButtonPressed = _MCTouchbuttons[1];
            bool CamYButtonPressed = _MCTouchbuttons[2];
            bool ScaleButtonPressed = _MCTouchbuttons[3];
            bool BodyZButtonPressed = _MCTouchbuttons[4];
            bool BodyPoseButtonPressed = _MCTouchbuttons[5];
            bool ArmsButtonPressed = _MCTouchbuttons[6];
            bool HandsButtonPressed = _MCTouchbuttons[7];
            bool WeaponButtonPressed = _MCTouchbuttons[8];
            bool GripButtonPressed = _MCTouchbuttons[9];
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
                    // Not expected - reset to standard sticky grip
                    g_config.enableGripButtonToGrap = false;
                    g_config.onePressGripButton = false;
                    g_config.enableGripButtonToLetGo = false;
                }
            } else if (!GripButtonPressed) {
                _isGripButtonPressed = false;
            }
            if (doinantHandStick.y > 0.10 && BodyZButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                g_config.setPlayerBodyOffsetUp(g_config.getPlayerBodyOffsetUp() + rAxisOffsetY);
            }
            if (doinantHandStick.y < -0.10 && BodyZButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                g_config.setPlayerBodyOffsetUp(g_config.getPlayerBodyOffsetUp() + rAxisOffsetY);
            }
            if (doinantHandStick.y > 0.10 && CamZButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                g_config.setplayerHMDOffsetUp(g_config.getplayerHMDOffsetUp() + rAxisOffsetY);
                // adjust the body offset to move with the camera
                g_config.setPlayerBodyOffsetUp(g_config.getPlayerBodyOffsetUp() - 0.125f * rAxisOffsetY);
            }
            if (doinantHandStick.y < -0.10 && CamZButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                g_config.setplayerHMDOffsetUp(g_config.getplayerHMDOffsetUp() + rAxisOffsetY);
                // adjust the body offset to move with the camera
                g_config.setPlayerBodyOffsetUp(g_config.getPlayerBodyOffsetUp() - 0.125f * rAxisOffsetY);
            }
            if (doinantHandStick.y > 0.10 && CamYButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 10;
                g_config.setPlayerBodyOffsetForward(g_config.getPlayerBodyOffsetForward() + rAxisOffsetY);
            }
            if (doinantHandStick.y < -0.10 && CamYButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 10;
                g_config.setPlayerBodyOffsetForward(g_config.getPlayerBodyOffsetForward() + rAxisOffsetY);
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
            if (doinantHandStick.y > 0.10 && BodyPoseButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                // isInPA ? g_config.playerOffsetUpInPA += rAxisOffsetY : g_config.playerBodyOffsetUpStanding += rAxisOffsetY;
            }
            if (doinantHandStick.y < -0.10 && BodyPoseButtonPressed) {
                rAxisOffsetY = doinantHandStick.y / 4;
                // isInPA ? g_config.playerOffsetUpInPA += rAxisOffsetY : g_config.playerBodyOffsetUpStanding += rAxisOffsetY;
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
            const auto ExitandSave = f4vr::VRControllers.isReleasedShort(f4vr::Hand::Primary, vr::k_EButton_SteamVR_Trigger);
            const auto ExitnoSave = f4vr::VRControllers.isReleasedShort(f4vr::Hand::Offhand, vr::k_EButton_SteamVR_Trigger);
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
                if (g_config.fVrScale != _fVrScale_bkup) {
                    g_config.fVrScale = _fVrScale_bkup;
                    RE::Setting* set = RE::GetINISetting("fVrScale:VR");
                    set->SetFloat(_fVrScale_bkup);
                }
                g_config.playerBodyOffsetUpStanding = _playerBodyOffsetUpStanding_bkup;
                g_config.playerBodyOffsetForwardStanding = _playerBodyOffsetForwardStanding_bkup;
                g_config.playerHMDOffsetUpStanding = _playerHMDOffsetUpStanding_bkup;
                g_config.playerBodyOffsetUpSitting = _playerBodyOffsetUpSitting_bkup;
                g_config.playerBodyOffsetForwardSitting = _playerBodyOffsetForwardSitting_bkup;
                g_config.playerHMDOffsetUpSitting = _playerHMDOffsetUpSitting_bkup;
                g_config.playerBodyOffsetUpStandingInPA = _playerBodyOffsetUpStandingInPA_bkup;
                g_config.playerBodyOffsetForwardStandingInPA = _playerBodyOffsetForwardStandingInPA_bkup;
                g_config.playerHMDOffsetUpStandingInPA = _playerHMDOffsetUpStandingInPA_bkup;
                g_config.playerBodyOffsetUpSittingInPA = _playerBodyOffsetUpSittingInPA_bkup;
                g_config.playerBodyOffsetForwardSittingInPA = _playerBodyOffsetForwardSittingInPA_bkup;
                g_config.playerHMDOffsetUpSittingInPA = _playerHMDOffsetUpSittingInPA_bkup;
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
            const uint64_t dominantHand = f4vr::isLeftHandedMode()
                ? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed
                : f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed;
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
                setConfigModeHandPose();

                const auto finger = _skelly->getBoneWorldTransform(f4vr::isLeftHandedMode() ? "RArm_Finger23" : "LArm_Finger23").translate;
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
                            float fz = 2.0f - distance;
                            if (fz > 0.0f && fz < 1.2f) {
                                TransMesh->local.translate.y = fz;
                            }
                            if (TransMesh->local.translate.y > 1.0 && !_PBTouchbuttons[i]) {
                                //_PBConfigSticky = true;
                                f4vr::VRControllers.triggerHaptic(f4vr::Hand::Offhand);
                                for (int j = 1; j <= 11; j++) {
                                    if (j != 1 && j != 3) {
                                        _PBTouchbuttons[j] = false;
                                    }
                                }
                                if (auto UIMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBCONFIGMarker")) {
                                    UIMarker->parent->DetachChild(UIMarker);
                                }
                                if (i != 1 && i != 3 && i != 10 && i != 11) {
                                    RE::NiNode* UI = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigMarker.nif", "PBCONFIGMarker");
                                    TouchMesh->AttachChild(UI, true);
                                }
                                if (i == 10 || i == 11) {
                                    if (i == 10) {
                                        if (!g_config.pipboyOpenWhenLookAt) {
                                            const auto UIMarkerMark = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBGlanceMarker");
                                            if (!UIMarkerMark) {
                                                RE::NiNode* UI = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigMarker.nif", "PBGlanceMarker");
                                                TouchMesh->AttachChild(UI, true);
                                            }
                                        } else if (g_config.pipboyOpenWhenLookAt) {
                                            if (const auto UIMarkerMark = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBGlanceMarker")) {
                                                UIMarkerMark->parent->DetachChild(UIMarkerMark);
                                            }
                                        }
                                    }
                                    if (i == 11) {
                                        const auto uiMarker = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, "PBDampenMarker");
                                        if (g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::None) {
                                            if (!uiMarker) {
                                                RE::NiNode* UI = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigMarker.nif", "PBDampenMarker");
                                                TouchMesh->AttachChild(UI, true);
                                            }
                                        } else if (g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::HoldInPlace) {
                                            if (uiMarker) {
                                                uiMarker->parent->DetachChild(uiMarker);
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
                    g_config.savePipboyOffset(f4vr::getPlayerNodes()->ScreenNode->local);
                    if (_3rdPipboy) {
                        // 3rd person Pipboy is null for Fallout London as there is no Pipboy on the arm
                        g_config.savePipboyScale(_3rdPipboy->local.scale);
                    }
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
                    if (g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::Movement) {
                        f4vr::showNotification("Dampen Pipboy screen by smoothing the movement");
                    } else if (g_config.dampenPipboyScreenMode == DampenPipboyScreenMode::HoldInPlace) {
                        f4vr::showNotification("Dampen Pipboy screen by holding it in place where opened.\nHold Pipboy hand grip to move the screen with the arm.");
                    }
                } else if (!DampenScreenButtonPressed) {
                    _isDampenScreenButtonPressed = false;
                }
                if (ExitButtonPressed) {
                    exitPBConfig();
                }
                if (ModelSwapButtonPressed && !_isModelSwapButtonPressed) {
                    _isModelSwapButtonPressed = true;
                    if (!g_config.isFalloutLondonVR) {
                        g_frik.swapPipboyModel();
                    }
                } else if (!ModelSwapButtonPressed) {
                    _isModelSwapButtonPressed = false;
                }

                // Handle Pipboy screen location adjustment logic
                const auto rightHandStick = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary, f4vr::Axis::Thumbstick);
                const auto pbScreenNode = f4vr::getPlayerNodes()->ScreenNode;
                if (RotateButtonPressed) {
                    if (rightHandStick.y > 0.10 || rightHandStick.y < -0.10) {
                        auto rAxisOffsetY = rightHandStick.y / 5;
                        if (rAxisOffsetY < 0) {
                            rAxisOffsetY = rAxisOffsetY * -1;
                        } else {
                            rAxisOffsetY = 0 - rAxisOffsetY;
                        }
                        if (f4vr::VRControllers.isPressHeldDown(f4vr::Hand::Primary, vr::k_EButton_Grip)) {
                            pbScreenNode->local.rotate = getMatrixFromEulerAngles(0, degreesToRads(rAxisOffsetY), 0) * pbScreenNode->local.rotate;
                        } else {
                            pbScreenNode->local.rotate = getMatrixFromEulerAngles(degreesToRads(rAxisOffsetY), 0, 0) * pbScreenNode->local.rotate;
                        }
                    }
                    if (rightHandStick.x > 0.10 || rightHandStick.x < -0.10) {
                        auto rAxisOffsetX = rightHandStick.x / 5;
                        if (rAxisOffsetX < 0) {
                            rAxisOffsetX = rAxisOffsetX * -1;
                        } else {
                            rAxisOffsetX = 0 - rAxisOffsetX;
                        }
                        if (!f4vr::VRControllers.isPressHeldDown(f4vr::Hand::Primary, vr::k_EButton_Grip)) {
                            pbScreenNode->local.rotate = getMatrixFromEulerAngles(0, 0, degreesToRads(rAxisOffsetX)) * pbScreenNode->local.rotate;
                        }
                    }
                }
                if (rightHandStick.y > 0.10 && ScaleButtonPressed) {
                    pbScreenNode->local.scale = pbScreenNode->local.scale + 0.01f;
                }
                if (rightHandStick.y < -0.10 && ScaleButtonPressed) {
                    pbScreenNode->local.scale = pbScreenNode->local.scale - 0.01f;
                }
                if (rightHandStick.y > 0.10 && MoveXButtonPressed) {
                    pbScreenNode->local.translate.x = pbScreenNode->local.translate.x + rightHandStick.y / 20;
                }
                if (rightHandStick.y < -0.10 && MoveXButtonPressed) {
                    pbScreenNode->local.translate.x = pbScreenNode->local.translate.x + rightHandStick.y / 20;
                }
                if (rightHandStick.y > 0.10 && MoveYButtonPressed) {
                    pbScreenNode->local.translate.y = pbScreenNode->local.translate.y - rightHandStick.y / 20;
                }
                if (rightHandStick.y < -0.10 && MoveYButtonPressed) {
                    pbScreenNode->local.translate.y = pbScreenNode->local.translate.y - rightHandStick.y / 20;
                }
                if (rightHandStick.y > 0.10 && MoveZButtonPressed) {
                    pbScreenNode->local.translate.z = pbScreenNode->local.translate.z - rightHandStick.y / 20;
                }
                if (rightHandStick.y < -0.10 && MoveZButtonPressed) {
                    pbScreenNode->local.translate.z = pbScreenNode->local.translate.z - rightHandStick.y / 20;
                }

                if (rightHandStick.y > 0.10 && ModelScaleButtonPressed && _3rdPipboy) {
                    _3rdPipboy->local.scale += rightHandStick.y / 65;
                }
                if (rightHandStick.y < -0.10 && ModelScaleButtonPressed && _3rdPipboy) {
                    _3rdPipboy->local.scale += rightHandStick.y / 65;
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
        const auto pipboyConfigUI = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigHUD.nif", "PBCONFIGHUD");
        if (f4vr::isLeftHandedMode() && !g_config.leftHandedPipBoy) {
            // rotate the UI so left-handed with Pipboy on it have the UI in convenient position
            pipboyConfigUI->local.translate = RE::NiPoint3(-12, 10, -3);
            pipboyConfigUI->local.rotate = getMatrixFromEulerAngles(0, degreesToRads(20), degreesToRads(-40)) * getMatrixFromEulerAngles(0, degreesToRads(90), degreesToRads(30));
        } else {
            pipboyConfigUI->local.translate = RE::NiPoint3(0, 0, 10);
        }
        f4vr::getPlayerNodes()->primaryUIAttachNode->AttachChild(pipboyConfigUI, true);
        for (int i = 0; i <= 11; i++) {
            static const char* MainHud[12] = {
                "Data/Meshes/FRIK/UI-MainTitle.nif", "Data/Meshes/FRIK/UI-Tile07.nif", "Data/Meshes/FRIK/UI-Tile03.nif", "Data/Meshes/FRIK/UI-Tile08.nif",
                "Data/Meshes/FRIK/UI-Tile02.nif", "Data/Meshes/FRIK/UI-Tile01.nif", "Data/Meshes/FRIK/UI-Tile04.nif", "Data/Meshes/FRIK/UI-Tile05.nif",
                "Data/Meshes/FRIK/UI-Tile06.nif", "Data/Meshes/FRIK/UI-Tile09.nif", "Data/Meshes/FRIK/UI-Tile10.nif", "Data/Meshes/FRIK/UI-Tile11.nif"
            };
            static const char* MainHud2[12] = {
                "Data/Meshes/FRIK/PB-MainTitle.nif", "Data/Meshes/FRIK/PB-Tile07.nif", "Data/Meshes/FRIK/PB-Tile03.nif", "Data/Meshes/FRIK/PB-Tile08.nif",
                "Data/Meshes/FRIK/PB-Tile02.nif", "Data/Meshes/FRIK/PB-Tile01.nif", "Data/Meshes/FRIK/PB-Tile04.nif", "Data/Meshes/FRIK/PB-Tile05.nif",
                "Data/Meshes/FRIK/PB-Tile06.nif", "Data/Meshes/FRIK/PB-Tile09.nif", "Data/Meshes/FRIK/PB-Tile10.nif", "Data/Meshes/FRIK/PB-Tile11.nif"
            };

            RE::NiNode* UI = f4vr::getClonedNiNodeForNifFileSetName(MainHud[i], meshName2[i]);
            pipboyConfigUI->AttachChild(UI, true);

            RE::NiNode* UI2 = f4vr::getClonedNiNodeForNifFileSetName(MainHud2[i], meshName[i]);
            UI->AttachChild(UI2, true);

            if (i == 10 && g_config.pipboyOpenWhenLookAt) {
                RE::NiNode* UI3 = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigMarker.nif", "PBGlanceMarker");
                UI->AttachChild(UI3, true);
            }
            if (i == 11 && g_config.dampenPipboyScreenMode != DampenPipboyScreenMode::None) {
                RE::NiNode* UI3 = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigMarker.nif", "PBDampenMarker");
                UI->AttachChild(UI3, true);
            }
        }
        _isPBConfigModeActive = true;
        _PBConfigModeEnterCounter = 0;
    }
}
