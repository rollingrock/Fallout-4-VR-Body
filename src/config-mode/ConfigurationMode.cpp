#include "ConfigurationMode.h"

#include "Config.h"
#include "FRIK.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "vrcf/VRControllersManager.h"
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

    void ConfigurationMode::onFrameUpdate()
    {
        pipboyConfigurationMode();
    }

    /**
     * The Pipboy Configuration Mode function.
     */
    void ConfigurationMode::pipboyConfigurationMode()
    {
        if (!g_frik.isPipboyOn() && f4vr::isPipboyOnWrist()) {
            return;
        }

        const uint64_t dominantHand = f4vr::isLeftHandedMode()
            ? vrcf::VRControllers.getControllerState_DEPRECATED(vrcf::TrackerType::Left).ulButtonPressed
            : vrcf::VRControllers.getControllerState_DEPRECATED(vrcf::TrackerType::Right).ulButtonPressed;
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

            const auto finger = f4vr::Skelly::getBoneWorldTransform(f4vr::isLeftHandedMode() ? "RArm_Finger23" : "LArm_Finger23").translate;
            for (int i = 1; i <= 11; i++) {
                auto TouchMesh = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName2[i]);
                auto TransMesh = f4vr::findNode(f4vr::getPlayerNodes()->primaryUIAttachNode, meshName[i]);
                if (TouchMesh && TransMesh) {
                    float distance = MatrixUtils::vec3Len(finger - TouchMesh->world.translate);
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
                            vrcf::VRControllers.triggerHaptic(vrcf::Hand::Offhand);
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
            const auto rightHandStick = vrcf::VRControllers.getAxisValue(vrcf::Hand::Primary, vrcf::Axis::Thumbstick);
            const auto pbScreenNode = f4vr::getPlayerNodes()->ScreenNode;
            if (RotateButtonPressed) {
                if (rightHandStick.y > 0.10 || rightHandStick.y < -0.10) {
                    auto rAxisOffsetY = rightHandStick.y / 5;
                    if (rAxisOffsetY < 0) {
                        rAxisOffsetY = rAxisOffsetY * -1;
                    } else {
                        rAxisOffsetY = 0 - rAxisOffsetY;
                    }
                    if (vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Primary, vr::k_EButton_Grip)) {
                        pbScreenNode->local.rotate = MatrixUtils::getMatrixFromEulerAngles(0, MatrixUtils::degreesToRads(rAxisOffsetY), 0) * pbScreenNode->local.rotate;
                    } else {
                        pbScreenNode->local.rotate = MatrixUtils::getMatrixFromEulerAngles(MatrixUtils::degreesToRads(rAxisOffsetY), 0, 0) * pbScreenNode->local.rotate;
                    }
                }
                if (rightHandStick.x > 0.10 || rightHandStick.x < -0.10) {
                    auto rAxisOffsetX = rightHandStick.x / 5;
                    if (rAxisOffsetX < 0) {
                        rAxisOffsetX = rAxisOffsetX * -1;
                    } else {
                        rAxisOffsetX = 0 - rAxisOffsetX;
                    }
                    if (!vrcf::VRControllers.isPressHeldDown(vrcf::Hand::Primary, vr::k_EButton_Grip)) {
                        pbScreenNode->local.rotate = MatrixUtils::getMatrixFromEulerAngles(0, 0, MatrixUtils::degreesToRads(rAxisOffsetX)) * pbScreenNode->local.rotate;
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

    void ConfigurationMode::enterPipboyConfigMode()
    {
        if (g_frik.isFavoritesMenuOpen()) {
            f4vr::closeFavoriteMenu();
        }
        vrcf::VRControllers.triggerHaptic(vrcf::Hand::Primary, 0.6f, 0.5f);
        const auto pipboyConfigUI = f4vr::getClonedNiNodeForNifFileSetName("FRIK/UI-ConfigHUD.nif", "PBCONFIGHUD");
        if (f4vr::isLeftHandedMode() && !g_config.leftHandedPipBoy) {
            // rotate the UI so left-handed with Pipboy on it have the UI in convenient position
            pipboyConfigUI->local.translate = RE::NiPoint3(-12, 10, -3);
            pipboyConfigUI->local.rotate = MatrixUtils::getMatrixFromEulerAngles(0, MatrixUtils::degreesToRads(20), MatrixUtils::degreesToRads(-40)) *
                MatrixUtils::getMatrixFromEulerAngles(0, MatrixUtils::degreesToRads(90), MatrixUtils::degreesToRads(30));
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
