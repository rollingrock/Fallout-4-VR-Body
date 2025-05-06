#include "ConfigurationMode.h"

#include <f4se/GameMenus.h>

#include "BSFlattenedBoneTree.h"
#include "Config.h"
#include "F4VRBody.h"
#include "HandPose.h"
#include "Pipboy.h"
#include "Skeleton.h"
#include "common/CommonUtils.h"
#include "f4vr/F4VRUtils.h"

using namespace common;

namespace frik {
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
	void ConfigurationMode::openPipboyConfigurationMode() {
		g_pipboy->turnOn();
		enterPipboyConfigMode();
	}

	/**
	 * Exit Main FRIK Config Mode
	 */
	void ConfigurationMode::configModeExit() {
		_calibrationModeUIActive = false;
		if (NiNode* c_MBox = f4vr::getNode("messageBoxMenuWider", _skelly->getPlayerNodes()->playerworldnode)) {
			c_MBox->flags &= ~0x1;
			c_MBox->m_localTransform.scale = 1.0;
		}
		if (_calibrateModeActive) {
			std::fill(std::begin(_MCTouchbuttons), std::end(_MCTouchbuttons), false);
			static BSFixedString hudname("MCCONFIGHUD");
			if (NiAVObject* MCConfigUI = _skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&hudname)) {
				MCConfigUI->flags |= 0x1;
				MCConfigUI->m_localTransform.scale = 0;
				MCConfigUI->m_parent->RemoveChild(MCConfigUI);
			}
			disableConfigModePose();
			_calibrateModeActive = false;
		}
	}

	void ConfigurationMode::exitPBConfig() {
		// Exit Pipboy Config Mode / remove UI.
		if (_isPBConfigModeActive) {
			for (int i = 0; i <= 11; i++) {
				_PBTouchbuttons[i] = false;
			}
			static BSFixedString hudname("PBCONFIGHUD");
			NiAVObject* PBConfigUI = _skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&hudname);
			if (PBConfigUI) {
				PBConfigUI->flags |= 0x1;
				PBConfigUI->m_localTransform.scale = 0;
				PBConfigUI->m_parent->RemoveChild(PBConfigUI);
			}
			disableConfigModePose();
			_isPBConfigModeActive = false;
		}
	}

	void ConfigurationMode::mainConfigurationMode() {
		if (!_calibrateModeActive) {
			return;
		}

		setConfigModeHandPose();

		float rAxisOffsetY;
		char* meshName[10] = {
			"MC-MainTitleTrans", "MC-Tile01Trans", "MC-Tile02Trans", "MC-Tile03Trans", "MC-Tile04Trans", "MC-Tile05Trans", "MC-Tile06Trans", "MC-Tile07Trans",
			"MC-Tile08Trans", "MC-Tile09Trans"
		};
		char* meshName2[10] = {
			"MC-MainTitle", "MC-Tile01", "MC-Tile02", "MC-Tile03", "MC-Tile04", "MC-Tile05", "MC-Tile06", "MC-Tile07", "MC-Tile08", "MC-Tile09"
		};
		char* meshName3[10] = {"", "", "", "", "", "", "", "MC-Tile07On", "MC-Tile08On", "MC-Tile09On"};
		char* meshName4[4] = {"MC-ModeA", "MC-ModeB", "MC-ModeC", "MC-ModeD"};
		if (!_calibrationModeUIActive) {
			// Create Config UI
			showMessagebox("FRIK Config Mode");
			NiNode* c_MBox = f4vr::getNode("messageBoxMenuWider", _skelly->getPlayerNodes()->playerworldnode);
			if (c_MBox) {
				c_MBox->flags |= 0x1;
				c_MBox->m_localTransform.scale = 0;
			}
			BSFixedString menuName("FavoritesMenu"); // close favorites menu if open.
			if ((*g_ui)->IsMenuOpen(menuName)) {
				if ((*g_ui)->IsMenuRegistered(menuName)) {
					CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
				}
				if (_vrhook != nullptr) {
					g_config.leftHandedMode ? _vrhook->StartHaptics(1, 0.55, 0.5) : _vrhook->StartHaptics(2, 0.55, 0.5);
				}
			}
			NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigHUD.nif");
			f4vr::NiCloneProcess proc;
			proc.unk18 = Offsets::cloneAddr1;
			proc.unk48 = Offsets::cloneAddr2;
			NiNode* HUD = Offsets::cloneNode(retNode, &proc);
			HUD->m_name = BSFixedString("MCCONFIGHUD");
			// TODO: this should just use "primaryUIAttachNode" but it needs offset corrections, better just change to UI framework
			NiNode* UIATTACH = g_config.leftHandedMode
				? _skelly->getPlayerNodes()->primaryUIAttachNode
				: f4vr::getNode("world_primaryWand.nif", _skelly->getPlayerNodes()->primaryUIAttachNode);
			UIATTACH->AttachChild(HUD, true);
			char* MainHud[10] = {
				"Data/Meshes/FRIK/UI-MainTitle.nif", "Data/Meshes/FRIK/UI-Tile01.nif", "Data/Meshes/FRIK/UI-Tile02.nif", "Data/Meshes/FRIK/UI-Tile03.nif",
				"Data/Meshes/FRIK/UI-Tile04.nif", "Data/Meshes/FRIK/UI-Tile05.nif", "Data/Meshes/FRIK/UI-Tile06.nif", "Data/Meshes/FRIK/UI-Tile07.nif",
				"Data/Meshes/FRIK/UI-Tile08.nif", "Data/Meshes/FRIK/UI-Tile09.nif"
			};
			char* MainHud2[10] = {
				"Data/Meshes/FRIK/MC-MainTitle.nif", "Data/Meshes/FRIK/MC-Tile01.nif", "Data/Meshes/FRIK/MC-Tile02.nif", "Data/Meshes/FRIK/MC-Tile03.nif",
				"Data/Meshes/FRIK/MC-Tile04.nif", "Data/Meshes/FRIK/MC-Tile05.nif", "Data/Meshes/FRIK/MC-Tile06.nif", "Data/Meshes/FRIK/MC-Tile07.nif",
				"Data/Meshes/FRIK/MC-Tile08.nif", "Data/Meshes/FRIK/MC-Tile09.nif"
			};
			char* MainHud3[4] = {
				"Data/Meshes/FRIK/MC-Tile09a.nif", "Data/Meshes/FRIK/MC-Tile09b.nif", "Data/Meshes/FRIK/MC-Tile09c.nif", "Data/Meshes/FRIK/MC-Tile09d.nif"
			};
			for (int i = 0; i <= 9; i++) {
				NiNode* retNode = loadNifFromFile(MainHud[i]);
				f4vr::NiCloneProcess proc;
				proc.unk18 = Offsets::cloneAddr1;
				proc.unk48 = Offsets::cloneAddr2;
				NiNode* UI = Offsets::cloneNode(retNode, &proc);
				UI->m_name = BSFixedString(meshName2[i]);
				HUD->AttachChild(UI, true);
				retNode = loadNifFromFile(MainHud2[i]);
				NiNode* UI2 = Offsets::cloneNode(retNode, &proc);
				UI2->m_name = BSFixedString(meshName[i]);
				UI->AttachChild(UI2, true);
				if (i == 7 || i == 8) {
					retNode = loadNifFromFile("Data/Meshes/FRIK/UI-StickyMarker.nif");
					NiNode* UI3 = Offsets::cloneNode(retNode, &proc);
					UI3->m_name = BSFixedString(meshName3[i]);
					UI2->AttachChild(UI3, true);
				}
				if (i == 9) {
					for (int x = 0; x < 4; x++) {
						retNode = loadNifFromFile(MainHud3[x]);
						NiNode* UI3 = Offsets::cloneNode(retNode, &proc);
						UI3->m_name = BSFixedString(meshName4[x]);
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
			NiNode* UIElement = nullptr;
			// Dampen Hands
			UIElement = f4vr::getNode("MC-Tile07On", _skelly->getPlayerNodes()->primaryUIAttachNode);
			g_config.dampenHands ? UIElement->m_localTransform.scale = 1 : UIElement->m_localTransform.scale = 0;
			// Weapon Reposition Mode
			UIElement = f4vr::getNode("MC-Tile08On", _skelly->getPlayerNodes()->primaryUIAttachNode);
			UIElement->m_localTransform.scale = g_weaponPosition->inWeaponRepositionMode() ? 1 : 0;
			// Grip Mode
			if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
				// Standard Sticky Grip on / off
				for (int i = 0; i < 4; i++) {
					if (i == 0) {
						UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 1;
					} else {
						UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 0;
					}
				}
			} else if (!g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
				// Sticky Grip with button to release
				for (int i = 0; i < 4; i++) {
					if (i == 1) {
						UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 1;
					} else {
						UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 0;
					}
				}
			} else if (g_config.enableGripButtonToGrap && g_config.onePressGripButton && !g_config.enableGripButtonToLetGo) {
				// Button held to Grip
				for (int i = 0; i < 4; i++) {
					if (i == 2) {
						UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 1;
					} else {
						UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 0;
					}
				}
			} else if (g_config.enableGripButtonToGrap && !g_config.onePressGripButton && g_config.enableGripButtonToLetGo) {
				// button press to toggle Grip on or off
				for (int i = 0; i < 4; i++) {
					if (i == 3) {
						UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 1;
					} else {
						UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
						UIElement->m_localTransform.scale = 0;
					}
				}
			} else {
				//Not exepected - show no mode lable until button pressed 
				for (int i = 0; i < 4; i++) {
					UIElement = f4vr::getNode(meshName4[i], _skelly->getPlayerNodes()->primaryUIAttachNode);
					UIElement->m_localTransform.scale = 0;
				}
			}
			auto rt = (BSFlattenedBoneTree*)_skelly->getRoot();
			NiPoint3 finger;
			g_config.leftHandedMode
				? finger = rt->transforms[_skelly->getBoneInMap("RArm_Finger23")].world.pos
				: finger = rt->transforms[_skelly->getBoneInMap("LArm_Finger23")].world.pos;
			for (int i = 1; i <= 9; i++) {
				BSFixedString TouchName = meshName2[i];
				BSFixedString TransName = meshName[i];
				auto TouchMesh = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&TouchName));
				auto TransMesh = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&TransName));
				if (TouchMesh && TransMesh) {
					float distance = vec3Len(finger - TouchMesh->m_worldTransform.pos);
					if (distance > 2.0) {
						TransMesh->m_localTransform.pos.y = 0.0;
						if (i == 7 || i == 8 || i == 9) {
							_MCTouchbuttons[i] = false;
						}
					} else if (distance <= 2.0) {
						float fz = 2.0 - distance;
						if (fz > 0.0 && fz < 1.2) {
							TransMesh->m_localTransform.pos.y = fz;
						}
						if (TransMesh->m_localTransform.pos.y > 1.0 && !_MCTouchbuttons[i]) {
							if (_vrhook != nullptr) {
								//_PBConfigSticky = true;
								g_config.leftHandedMode ? _vrhook->StartHaptics(2, 0.05, 0.3) : _vrhook->StartHaptics(1, 0.05, 0.3);
								for (int i = 1; i <= 7; i++) {
									_MCTouchbuttons[i] = false;
								}
								BSFixedString bname = "MCCONFIGMarker";
								auto UIMarker = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&bname));
								if (UIMarker) {
									UIMarker->m_parent->RemoveChild(UIMarker);
								}
								if (i < 7) {
									NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigMarker.nif");
									f4vr::NiCloneProcess proc;
									proc.unk18 = Offsets::cloneAddr1;
									proc.unk48 = Offsets::cloneAddr2;
									NiNode* UI = Offsets::cloneNode(retNode, &proc);
									UI->m_name = BSFixedString("MCCONFIGMarker");
									TouchMesh->AttachChild(UI, true);
								}
								_MCTouchbuttons[i] = true;
							}
						}
					}
				}
			}
			vr::VRControllerAxis_t doinantHandStick = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).rAxis[0]
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).rAxis[0];
			uint64_t dominantHand = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonPressed
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonPressed;
			uint64_t offHand = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonPressed
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonPressed;
			bool CamZButtonPressed = _MCTouchbuttons[1];
			bool CamYButtonPressed = _MCTouchbuttons[2];
			bool ScaleButtonPressed = _MCTouchbuttons[3];
			bool BodyZButtonPressed = _MCTouchbuttons[4];
			bool BodyPoseButtonPressed = _MCTouchbuttons[5];
			bool ArmsButtonPressed = _MCTouchbuttons[6];
			bool HandsButtonPressed = _MCTouchbuttons[7];
			bool WeaponButtonPressed = _MCTouchbuttons[8];
			bool GripButtonPressed = _MCTouchbuttons[9];
			bool isInPA = _skelly->detectInPowerArmor();
			if (HandsButtonPressed && !_isHandsButtonPressed) {
				_isHandsButtonPressed = true;
				g_config.dampenHands = !g_config.dampenHands;
			} else if (!HandsButtonPressed) {
				_isHandsButtonPressed = false;
			}
			if (WeaponButtonPressed && !_isWeaponButtonPressed) {
				_isWeaponButtonPressed = true;
				g_weaponPosition->toggleWeaponRepositionMode();
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
				Setting* set = GetINISetting("fVrScale:VR");
				set->SetDouble(g_config.fVrScale);
			}
			if (doinantHandStick.y < -0.10 && ScaleButtonPressed) {
				rAxisOffsetY = doinantHandStick.y / 4;
				g_config.fVrScale -= rAxisOffsetY;
				Setting* set = GetINISetting("fVrScale:VR");
				set->SetDouble(g_config.fVrScale);
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

	void ConfigurationMode::onUpdate() {
		checkWeaponRepositionPipboyConflict();
		pipboyConfigurationMode();
		mainConfigurationMode();

		if (_calibrateModeActive) {
			vr::VRControllerAxis_t doinantHandStick = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).rAxis[0]
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).rAxis[0];
			const uint64_t dominantHand = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonPressed
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonPressed;
			const uint64_t offHand = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonPressed
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonPressed;
			const auto ExitandSave = dominantHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(33));
			const auto ExitnoSave = offHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(33));
			const auto SelfieButton = dominantHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(1));
			const auto HeightButton = offHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(1));
			if (ExitandSave && !_exitAndSavePressed) {
				_exitAndSavePressed = true;
				g_configurationMode->configModeExit();
				g_config.save();
				if (_vrhook != nullptr) {
					g_config.leftHandedMode ? _vrhook->StartHaptics(1, 0.55, 0.5) : _vrhook->StartHaptics(2, 0.55, 0.5);
				}
			} else if (!ExitandSave) {
				_exitAndSavePressed = false;
			}
			if (ExitnoSave && !_exitWithoutSavePressed) {
				_exitWithoutSavePressed = true;
				g_configurationMode->configModeExit();
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
				c_selfieMode = !c_selfieMode;
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
			const uint64_t dominantHand = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonPressed
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonPressed;
			const uint64_t offHand = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonPressed
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonPressed;
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
	void ConfigurationMode::pipboyConfigurationMode() {
		if (g_pipboy->status()) {
			float rAxisOffsetX;
			vr::VRControllerAxis_t doinantHandStick = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).rAxis[0]
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).rAxis[0];
			uint64_t dominantHand = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonPressed
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonPressed;
			uint64_t offHand = g_config.leftHandedMode
				? f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Right).ulButtonPressed
				: f4vr::g_vrHook->getControllerState(f4vr::TrackerType::Left).ulButtonPressed;
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
			static BSFixedString wandPipName("PipboyRoot");
			NiAVObject* pbRoot = _skelly->getPlayerNodes()->SecondaryWandNode->GetObjectByName(&wandPipName);
			if (!pbRoot) {
				return;
			}
			BSFixedString pipName("PipboyBone");
			NiAVObject* _3rdPipboy = nullptr;
			if (!g_config.leftHandedPipBoy) {
				if (_skelly->getLeftArm().forearm3) {
					_3rdPipboy = _skelly->getLeftArm().forearm3->GetObjectByName(&pipName);
				}
			} else {
				_3rdPipboy = _skelly->getRightArm().forearm3->GetObjectByName(&pipName);
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

				auto rt = (BSFlattenedBoneTree*)_skelly->getRoot();
				NiPoint3 finger;
				g_config.leftHandedMode
					? finger = rt->transforms[_skelly->getBoneInMap("RArm_Finger23")].world.pos
					: finger = rt->transforms[_skelly->getBoneInMap("LArm_Finger23")].world.pos;
				for (int i = 1; i <= 11; i++) {
					BSFixedString TouchName = meshName2[i];
					BSFixedString TransName = meshName[i];
					auto TouchMesh = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&TouchName));
					auto TransMesh = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&TransName));
					if (TouchMesh && TransMesh) {
						float distance = vec3Len(finger - TouchMesh->m_worldTransform.pos);
						if (distance > 2.0) {
							TransMesh->m_localTransform.pos.y = 0.0;
							if (i == 1 || i == 3 || i == 10 || i == 11) {
								_PBTouchbuttons[i] = false;
							}
						} else if (distance <= 2.0) {
							float fz = 2.0 - distance;
							if (fz > 0.0 && fz < 1.2) {
								TransMesh->m_localTransform.pos.y = fz;
							}
							if (TransMesh->m_localTransform.pos.y > 1.0 && !_PBTouchbuttons[i]) {
								if (_vrhook != nullptr) {
									//_PBConfigSticky = true;
									g_config.leftHandedMode ? _vrhook->StartHaptics(2, 0.05, 0.3) : _vrhook->StartHaptics(1, 0.05, 0.3);
									for (int i = 1; i <= 11; i++) {
										if (i != 1 && i != 3) {
											_PBTouchbuttons[i] = false;
										}
									}
									BSFixedString bname = "PBCONFIGMarker";
									auto UIMarker = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&bname));
									if (UIMarker) {
										UIMarker->m_parent->RemoveChild(UIMarker);
									}
									if (i != 1 && i != 3 && i != 10 && i != 11) {
										NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigMarker.nif");
										f4vr::NiCloneProcess proc;
										proc.unk18 = Offsets::cloneAddr1;
										proc.unk48 = Offsets::cloneAddr2;
										NiNode* UI = Offsets::cloneNode(retNode, &proc);
										UI->m_name = BSFixedString("PBCONFIGMarker");
										TouchMesh->AttachChild(UI, true);
									}
									if (i == 10 || i == 11) {
										if (i == 10) {
											if (!g_config.pipBoyOpenWhenLookAt) {
												BSFixedString bname = "PBGlanceMarker";
												auto UIMarker = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&bname));
												if (!UIMarker) {
													NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigMarker.nif");
													f4vr::NiCloneProcess proc;
													proc.unk18 = Offsets::cloneAddr1;
													proc.unk48 = Offsets::cloneAddr2;
													NiNode* UI = Offsets::cloneNode(retNode, &proc);
													UI->m_name = BSFixedString("PBGlanceMarker");
													TouchMesh->AttachChild(UI, true);
												}
											} else if (g_config.pipBoyOpenWhenLookAt) {
												BSFixedString bname = "PBGlanceMarker";
												auto UIMarker = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&bname));
												if (UIMarker) {
													UIMarker->m_parent->RemoveChild(UIMarker);
												}
											}
										}
										if (i == 11) {
											if (!g_config.dampenPipboyScreen) {
												BSFixedString bname = "PBDampenMarker";
												auto UIMarker = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&bname));
												if (!UIMarker) {
													NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigMarker.nif");
													f4vr::NiCloneProcess proc;
													proc.unk18 = Offsets::cloneAddr1;
													proc.unk48 = Offsets::cloneAddr2;
													NiNode* UI = Offsets::cloneNode(retNode, &proc);
													UI->m_name = BSFixedString("PBDampenMarker");
													TouchMesh->AttachChild(UI, true);
												}
											} else if (g_config.dampenPipboyScreen) {
												BSFixedString bname = "PBDampenMarker";
												auto UIMarker = static_cast<NiNode*>(_skelly->getPlayerNodes()->primaryUIAttachNode->GetObjectByName(&bname));
												if (UIMarker) {
													UIMarker->m_parent->RemoveChild(UIMarker);
												}
											}
										}
									}
									_PBTouchbuttons[i] = true;
								}
							}
						}
					}
				}
				if (SaveButtonPressed && !_isSaveButtonPressed) {
					_isSaveButtonPressed = true;
					g_config.savePipboyOffset(pbRoot->m_localTransform);
					g_config.savePipboyScale(_3rdPipboy->m_localTransform.scale);
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
					g_config.dampenPipboyScreen ? g_config.dampenPipboyScreen = false : g_config.dampenPipboyScreen = true;
					g_config.toggleDampenPipboyScreen();
				} else if (!DampenScreenButtonPressed) {
					_isDampenScreenButtonPressed = false;
				}
				if (ExitButtonPressed) {
					exitPBConfig();
				}
				if (ModelSwapButtonPressed && !_isModelSwapButtonPressed) {
					_isModelSwapButtonPressed = true;
					g_config.toggleIsHoloPipboy();
					turnPipBoyOff();
					g_pipboy->replaceMeshes(true);
					_skelly->getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
					turnPipBoyOn();
				} else if (!ModelSwapButtonPressed) {
					_isModelSwapButtonPressed = false;
				}
				if ((doinantHandStick.y > 0.10 || doinantHandStick.y < -0.10) && RotateButtonPressed) {
					Matrix44 rot;
					rAxisOffsetX = doinantHandStick.y / 10;
					if (rAxisOffsetX < 0) {
						rAxisOffsetX = rAxisOffsetX * -1;
					} else {
						rAxisOffsetX = 0 - rAxisOffsetX;
					}
					rot.setEulerAngles(degreesToRads(rAxisOffsetX), 0, 0);
					pbRoot->m_localTransform.rot = rot.multiply43Left(pbRoot->m_localTransform.rot);
					rot.multiply43Left(pbRoot->m_localTransform.rot);
				}
				if (doinantHandStick.y > 0.10 && ScaleButtonPressed) {
					pbRoot->m_localTransform.scale = pbRoot->m_localTransform.scale + 0.001;
				}
				if (doinantHandStick.y < -0.10 && ScaleButtonPressed) {
					pbRoot->m_localTransform.scale = pbRoot->m_localTransform.scale - 0.001;
				}
				if (doinantHandStick.y > 0.10 && MoveXButtonPressed) {
					rAxisOffsetX = doinantHandStick.y / 50;
					pbRoot->m_localTransform.pos.x = pbRoot->m_localTransform.pos.x + rAxisOffsetX;
				}
				if (doinantHandStick.y < -0.10 && MoveXButtonPressed) {
					rAxisOffsetX = doinantHandStick.y / 50;
					pbRoot->m_localTransform.pos.x = pbRoot->m_localTransform.pos.x + rAxisOffsetX;
				}
				if (doinantHandStick.y > 0.10 && MoveYButtonPressed) {
					rAxisOffsetX = doinantHandStick.y / 20;
					pbRoot->m_localTransform.pos.y = pbRoot->m_localTransform.pos.y + rAxisOffsetX;
				}
				if (doinantHandStick.y < -0.10 && MoveYButtonPressed) {
					rAxisOffsetX = doinantHandStick.y / 20;
					pbRoot->m_localTransform.pos.y = pbRoot->m_localTransform.pos.y + rAxisOffsetX;
				}
				if (doinantHandStick.y > 0.10 && MoveZButtonPressed) {
					rAxisOffsetX = doinantHandStick.y / 20;
					pbRoot->m_localTransform.pos.z = pbRoot->m_localTransform.pos.z - rAxisOffsetX;
				}
				if (doinantHandStick.y < -0.10 && MoveZButtonPressed) {
					rAxisOffsetX = doinantHandStick.y / 20;
					pbRoot->m_localTransform.pos.z = pbRoot->m_localTransform.pos.z - rAxisOffsetX;
				}

				if (doinantHandStick.y > 0.10 && ModelScaleButtonPressed && _3rdPipboy) {
					rAxisOffsetX = doinantHandStick.y / 65;
					_3rdPipboy->m_localTransform.scale += rAxisOffsetX;
					g_config.pipBoyScale = _3rdPipboy->m_localTransform.scale;
				}
				if (doinantHandStick.y < -0.10 && ModelScaleButtonPressed && _3rdPipboy) {
					rAxisOffsetX = doinantHandStick.y / 65;
					_3rdPipboy->m_localTransform.scale += rAxisOffsetX;
					g_config.pipBoyScale = _3rdPipboy->m_localTransform.scale;
				}
			}
		}
	}

	void ConfigurationMode::enterPipboyConfigMode() {
		BSFixedString menuName("FavoritesMenu");
		if ((*g_ui)->IsMenuOpen(menuName)) {
			if ((*g_ui)->IsMenuRegistered(menuName)) {
				CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
			}
		}
		if (_vrhook != nullptr) {
			g_config.leftHandedMode ? _vrhook->StartHaptics(1, 0.55, 0.5) : _vrhook->StartHaptics(2, 0.55, 0.5);
		}
		const NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigHUD.nif");
		f4vr::NiCloneProcess proc;
		proc.unk18 = Offsets::cloneAddr1;
		proc.unk48 = Offsets::cloneAddr2;
		NiNode* HUD = Offsets::cloneNode(retNode, &proc);
		HUD->m_name = BSFixedString("PBCONFIGHUD");
		// TODO: this should just use "primaryUIAttachNode" but it needs offset corrections, better just change to UI framework
		NiNode* UIATTACH = g_config.leftHandedMode
			? _skelly->getPlayerNodes()->primaryUIAttachNode
			: f4vr::getNode("world_primaryWand.nif", _skelly->getPlayerNodes()->primaryUIAttachNode);
		UIATTACH->AttachChild(HUD, true);
		char* MainHud[12] = {
			"Data/Meshes/FRIK/UI-MainTitle.nif", "Data/Meshes/FRIK/UI-Tile07.nif", "Data/Meshes/FRIK/UI-Tile03.nif", "Data/Meshes/FRIK/UI-Tile08.nif",
			"Data/Meshes/FRIK/UI-Tile02.nif", "Data/Meshes/FRIK/UI-Tile01.nif", "Data/Meshes/FRIK/UI-Tile04.nif", "Data/Meshes/FRIK/UI-Tile05.nif",
			"Data/Meshes/FRIK/UI-Tile06.nif", "Data/Meshes/FRIK/UI-Tile09.nif", "Data/Meshes/FRIK/UI-Tile10.nif", "Data/Meshes/FRIK/UI-Tile11.nif"
		};
		char* MainHud2[12] = {
			"Data/Meshes/FRIK/PB-MainTitle.nif", "Data/Meshes/FRIK/PB-Tile07.nif", "Data/Meshes/FRIK/PB-Tile03.nif", "Data/Meshes/FRIK/PB-Tile08.nif",
			"Data/Meshes/FRIK/PB-Tile02.nif", "Data/Meshes/FRIK/PB-Tile01.nif", "Data/Meshes/FRIK/PB-Tile04.nif", "Data/Meshes/FRIK/PB-Tile05.nif",
			"Data/Meshes/FRIK/PB-Tile06.nif", "Data/Meshes/FRIK/PB-Tile09.nif", "Data/Meshes/FRIK/PB-Tile10.nif", "Data/Meshes/FRIK/PB-Tile11.nif"
		};
		for (int i = 0; i <= 11; i++) {
			const NiNode* retNode = loadNifFromFile(MainHud[i]);
			f4vr::NiCloneProcess proc;
			proc.unk18 = Offsets::cloneAddr1;
			proc.unk48 = Offsets::cloneAddr2;
			NiNode* UI = Offsets::cloneNode(retNode, &proc);
			UI->m_name = BSFixedString(meshName2[i]);
			HUD->AttachChild(UI, true);
			retNode = loadNifFromFile(MainHud2[i]);
			NiNode* UI2 = Offsets::cloneNode(retNode, &proc);
			UI2->m_name = BSFixedString(meshName[i]);
			UI->AttachChild(UI2, true);
			if (i == 10 && g_config.pipBoyOpenWhenLookAt) {
				retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigMarker.nif");
				NiNode* UI3 = Offsets::cloneNode(retNode, &proc);
				UI3->m_name = BSFixedString("PBGlanceMarker");
				UI->AttachChild(UI3, true);
			}
			if (i == 11 && g_config.dampenPipboyScreen) {
				retNode = loadNifFromFile("Data/Meshes/FRIK/UI-ConfigMarker.nif");
				NiNode* UI3 = Offsets::cloneNode(retNode, &proc);
				UI3->m_name = BSFixedString("PBDampenMarker");
				UI->AttachChild(UI3, true);
			}
		}
		_isPBConfigModeActive = true;
		_PBConfigModeEnterCounter = 0;
	}

	/**
	 * Check if currently in weapon reposition mode to enable or disable the rotation stick depending if pipboy is open.
	 * Needed to operate vanilla in-fron or projected pipboy when also doing weapon repositioning.
	 * On-wrist pipboy needs the rotation stick disabled to override its own UI.
	 */
	void ConfigurationMode::checkWeaponRepositionPipboyConflict() {
		if (!g_weaponPosition->inWeaponRepositionMode()) {
			return;
		}
		f4vr::setControlsThumbstickEnableState(isAnyPipboyOpen() && !g_pipboy->isOperatingPipboy());
	}
}
