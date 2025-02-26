#include "Pipboy.h"
#include "VR.h"

namespace F4VRBody {

	Pipboy* g_pipboy = nullptr;

	/// <summary>
	/// Run Pipboy mesh replacement if not already done (or forced) to the configured meshes either holo or screen.
	/// </summary>
	/// <param name="force">true - run mesh replace, false - only if not previously replaced</param>
	void Pipboy::replaceMeshes(bool force) {
		if (force || !meshesReplaced) {
			if (c_IsHoloPipboy == 0) {
				replaceMeshes("HoloEmitter", "Screen");
			}
			else if (c_IsHoloPipboy == 1) {
				replaceMeshes("Screen", "HoloEmitter");
			}
		}
	}

	/// <summary>
	/// Executed every frame to update the Pipboy location and handle interaction with pipboy config UX.
	/// TODO: refactor into seperate functions for each functionality
	/// </summary>
	void Pipboy::onUpdate() {
		// Cylons Code Starts Here ---->
		playerSkelly->pipboyConfigurationMode();
		playerSkelly->mainConfigurationMode();
		playerSkelly->pipboyManagement();
		playerSkelly->dampenPipboyScreen();
		
		//Hide some Pipboy related meshes on exit of Power Armor if they're not hidden
		if (!playerSkelly->detectInPowerArmor()) {
			NiNode* _HideNode = nullptr;
			c_IsHoloPipboy ? _HideNode = getChildNode("Screen", (*g_player)->unkF0->rootNode) : _HideNode = getChildNode("HoloEmitter", (*g_player)->unkF0->rootNode);
			if (_HideNode) {
				if (_HideNode->m_localTransform.scale != 0) {
					_HideNode->flags |= 0x1;
					_HideNode->m_localTransform.scale = 0;
				}
			}
		}
		if (c_CalibrateModeActive) {
			vr::VRControllerAxis_t doinantHandStick = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0]);
			uint64_t dominantHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed);
			uint64_t offHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed);
			const auto ExitandSave = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)33);
			const auto ExitnoSave = offHand & vr::ButtonMaskFromId((vr::EVRButtonId)33);
			const auto SelfieButton = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)1);
			const auto HeightButton = offHand & vr::ButtonMaskFromId((vr::EVRButtonId)1);
			if (ExitandSave && !c_ExitandSavePressed) {
				c_ExitandSavePressed = true;
				playerSkelly->configModeExit();
				saveSettings();
				if (vrhook != nullptr) {
					c_leftHandedMode ? vrhook->StartHaptics(1, 0.55, 0.5) : vrhook->StartHaptics(2, 0.55, 0.5);
				}
			}
			else if (!ExitandSave) {
				c_ExitandSavePressed = false;
			}
			if (ExitnoSave && !c_ExitnoSavePressed) {
				c_ExitnoSavePressed = true;
				playerSkelly->configModeExit();
				c_armLength = c_armLengthbkup;
				c_powerArmor_up = c_powerArmor_upbkup;
				c_playerOffset_up = c_playerOffset_upbkup;
				c_RootOffset = c_RootOffsetbkup;
				c_PARootOffset = c_PARootOffsetbkup;
				c_fVrScale = c_fVrScalebkup;
				c_playerOffset_forward = c_playerOffset_forwardbkup;
				c_powerArmor_forward = c_powerArmor_forwardbkup;
				c_cameraHeight = c_cameraHeightbkup;
				c_PACameraHeight = c_PACameraHeightbkup;
			}
			else if (!ExitnoSave) {
				c_ExitnoSavePressed = false;
			}
			if (SelfieButton && !c_SelfieButtonPressed) {
				c_SelfieButtonPressed = true;
				c_selfieMode = !c_selfieMode;
			}
			else if (!SelfieButton) {
				c_SelfieButtonPressed = false;
			}
			if (HeightButton && !c_UIHeightButtonPressed) {
				c_UIHeightButtonPressed = true;
				PlayerNodes* skelly = playerSkelly->getPlayerNodes();
				c_PlayerHMDHeight = skelly->UprightHmdNode->m_localTransform.pos.z;
				float x = playerSkelly->getNode("LArm_Collarbone", playerSkelly->getRoot())->m_worldTransform.pos.z;
				c_shouldertoHMD = c_PlayerHMDHeight - x;
				lastCamZ = 0.0;
			}
			else if (!HeightButton) {
				c_UIHeightButtonPressed = false;
			}
		}
		else {
			uint64_t dominantHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed);
			uint64_t offHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed);
			const auto dHTouch = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)32);
			const auto oHTouch = offHand & vr::ButtonMaskFromId((vr::EVRButtonId)32);
			if (dHTouch && !c_CalibrateModeActive) {
				c_ConfigModeTimer += 1;
				if (c_ConfigModeTimer > 200 && c_ConfigModeTimer2 > 200) {
					c_DampenHandsButtonPressed = true;
					c_CalibrateModeActive = true;
				}
			}
			else if (!dHTouch && !c_CalibrateModeActive && c_ConfigModeTimer > 0) {
				c_ConfigModeTimer = 0;
			}
			if (oHTouch && !c_CalibrateModeActive) {
				c_ConfigModeTimer2 += 1;
			}
			else if (!oHTouch && !c_CalibrateModeActive && c_ConfigModeTimer2 > 0) {
				c_ConfigModeTimer2 = 0;
			}
		}
		// Cylons Code Ends Here
	}

	/// <summary>
	/// Hnalde replacing of Pipboy meshes on the arm with either screen or holo emitter.
	/// </summary>
	void Pipboy::replaceMeshes(std::string itemHide, std::string itemShow) {

		auto pn = playerSkelly->getPlayerNodes();
		NiNode* ui = pn->primaryUIAttachNode;
		NiNode* wand = get1stChildNode("world_primaryWand.nif", ui);
		NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/_primaryWand.nif");
		if (retNode) {
			// ui->RemoveChild(wand);
			// ui->AttachChild(retNode, true);
		}

		wand = pn->SecondaryWandNode;
		NiNode* pipParent = get1stChildNode("PipboyParent", wand);

		if (!pipParent) {
			meshesReplaced = false;
			return;
		}
		wand = get1stChildNode("PipboyRoot_NIF_ONLY", pipParent);
		c_IsHoloPipboy ? retNode = loadNifFromFile("Data/Meshes/FRIK/HoloPipboyVR.nif") : retNode = loadNifFromFile("Data/Meshes/FRIK/PipboyVR.nif");
		if (retNode && wand) {
			BSFixedString screenName("Screen:0");
			NiAVObject* newScreen = retNode->GetObjectByName(&screenName)->m_parent;

			if (!newScreen) {
				meshesReplaced = false;
				return;
			}

			pipParent->RemoveChild(wand);
			pipParent->AttachChild(retNode, true);

			pn->ScreenNode->RemoveChildAt(0);
			// using native function here to attach the new screen as too lazy to fully reverse what it's doing and it works fine.
			NiNode* rn = Offsets::addNode((uint64_t)&pn->ScreenNode, newScreen);
			pn->PipboyRoot_nif_only_node = retNode;
		}

		meshesReplaced = true;

		// Cylons Code Start >>>>
		auto lookup = g_weaponOffsets->getOffset("PipboyPosition", Mode::normal);
		if (c_IsHoloPipboy == true) {
			lookup = g_weaponOffsets->getOffset("HoloPipboyPosition", Mode::normal);
		}
		if (lookup.has_value()) {
			NiTransform pbTransform = lookup.value();
			static BSFixedString wandPipName("PipboyRoot");
			NiAVObject* pbRoot = pn->SecondaryWandNode->GetObjectByName(&wandPipName);
			if (pbRoot) {
				pbRoot->m_localTransform = pbTransform;
			}
		}
		pn->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0; //prevents the VRPipboy screen from being displayed on first load whilst PB is off.
		NiNode* _HideNode = getChildNode(itemHide.c_str(), (*g_player)->unkF0->rootNode);
		if (_HideNode) {
			_HideNode->flags |= 0x1;
			_HideNode->m_localTransform.scale = 0;
		}
		NiNode* _ShowNode = getChildNode(itemShow.c_str(), (*g_player)->unkF0->rootNode);
		if (_ShowNode) {
			_ShowNode->flags &= 0xfffffffffffffffe;
			_ShowNode->m_localTransform.scale = 1;
		}
		// <<<< Cylons Code End

		_MESSAGE("Pipboy Meshes replaced! Hide: %s, Show: %s", itemHide, itemShow);
	}
}