#include "Pipboy.h"
#include "ConfigurationMode.h"
#include "Config.h"
#include "VR.h"

#include <chrono>
#include <time.h>
#include <string.h>

using namespace std::chrono;

namespace F4VRBody {

	Pipboy* g_pipboy = nullptr;

	void Pipboy::swapPipboy() {
		_pipboyStatus = false;
		_pipTimer = 0;
	}

	void Pipboy::onSetNodes() {
		if (!_skelly) {
			return;
		}
		if (NiNode* screenNode = _skelly->getPlayerNodes()->ScreenNode) {
			if (NiAVObject* screen = screenNode->GetObjectByName(&BSFixedString("Screen:0"))) {
				_pipboyScreenPrevFrame = screen->m_worldTransform;
			}
		}
	}

	/// <summary>
	/// Run Pipboy mesh replacement if not already done (or forced) to the configured meshes either holo or screen.
	/// </summary>
	/// <param name="force">true - run mesh replace, false - only if not previously replaced</param>
	void Pipboy::replaceMeshes(bool force) {
		if (force || !meshesReplaced) {
			if (g_config->isHoloPipboy == 0) {
				replaceMeshes("HoloEmitter", "Screen");
			}
			else if (g_config->isHoloPipboy == 1) {
				replaceMeshes("Screen", "HoloEmitter");
			}
		}
	}

	/// <summary>
	/// Executed every frame to update the Pipboy location and handle interaction with pipboy config UX.
	/// TODO: refactor into seperate functions for each functionality
	/// </summary>
	void Pipboy::onUpdate() {
		
		pipboyManagement();
		dampenPipboyScreen();

		//Hide some Pipboy related meshes on exit of Power Armor if they're not hidden
		if (!_skelly->detectInPowerArmor()) {
			NiNode* _HideNode = nullptr;
			g_config->isHoloPipboy ? _HideNode = getChildNode("Screen", (*g_player)->unkF0->rootNode) : _HideNode = getChildNode("HoloEmitter", (*g_player)->unkF0->rootNode);
			if (_HideNode) {
				if (_HideNode->m_localTransform.scale != 0) {
					_HideNode->flags |= 0x1;
					_HideNode->m_localTransform.scale = 0;
				}
			}
		}
	}

	/// <summary>
	/// Hnalde replacing of Pipboy meshes on the arm with either screen or holo emitter.
	/// </summary>
	void Pipboy::replaceMeshes(std::string itemHide, std::string itemShow) {

		auto pn = _skelly->getPlayerNodes();
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
		g_config->isHoloPipboy ? retNode = loadNifFromFile("Data/Meshes/FRIK/HoloPipboyVR.nif") : retNode = loadNifFromFile("Data/Meshes/FRIK/PipboyVR.nif");
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
		if (g_config->isHoloPipboy == true) {
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

		_MESSAGE("Pipboy Meshes replaced! Hide: %s, Show: %s", itemHide.c_str(), itemShow.c_str());
	}

	void Pipboy::operatePipBoy() {

		if ((*g_player)->firstPersonSkeleton == nullptr) {
			return;
		}

		BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_skelly->getRoot();

		NiPoint3 finger;
		NiAVObject* pipboy = nullptr;
		NiAVObject* pipboyTrans = nullptr;
		g_config->leftHandedPipBoy ? finger = rt->transforms[_skelly->getBoneInMap("LArm_Finger23")].world.pos : finger = rt->transforms[_skelly->getBoneInMap("RArm_Finger23")].world.pos;
		g_config->leftHandedPipBoy ? pipboy = _skelly->getNode("PipboyRoot", _skelly->getRightArm().shoulder->GetAsNiNode()) : pipboy = _skelly->getNode("PipboyRoot", _skelly->getLeftArm().shoulder->GetAsNiNode());
		if (pipboy == nullptr) {
			return;
		}

		const auto pipOnButtonPressed = (g_config->pipBoyButtonArm ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed :
			VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed) & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->pipBoyButtonID);
		const auto pipOffButtonPressed = (g_config->pipBoyButtonOffArm ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed :
			VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed) & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->pipBoyButtonOffID);

		// check off button
		if (pipOffButtonPressed && !_stickyoffpip) {
			if (_pipboyStatus) {
				_pipboyStatus = false;
				turnPipBoyOff();
				g_configurationMode->exitPBConfig();
				if (_isWeaponinHand) {
					drawWeapon(); // draw weapon as we no longer need primary trigger as an input.
					_weaponStateDetected = false;
				}
				_skelly->disablePipboyHandPose();
				_skelly->getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				_MESSAGE("Disabling Pipboy with button");
				_stickyoffpip = true;
			}
		}
		else if (!pipOffButtonPressed) {
			// _stickyoffpip is a guard so we don't constantly toggle the pip boy off every frame
			_stickyoffpip = false;
		}

		/* Refactored this part of the code so that turning on the wrist based Pipboy works the same way as the 'Projected Pipboy'. It works on button release rather than press,
		this enables us to determine if the button was held for a short or long press by the status of the '_controlSleepStickyT' bool. If it is still set to true on button release
		then we know the button was a short press, if it is set to false we know it was a long press. Long press = torch on / off, Short Press = Pipboy enable.
		*/

		if (pipOnButtonPressed && !_stickybpip && !_isOperatingPipboy) {
			_stickybpip = true;
			_controlSleepStickyT = true;
			std::thread t5(SecondaryTriggerSleep, 300); // switches a bool to false after 150ms
			t5.detach();
		}
		else if (!pipOnButtonPressed) {
			if (_controlSleepStickyT && _stickybpip && isLookingAtPipBoy()) {  // if bool is still set to true on control release we know it was a short press.
				_pipboyStatus = true;
				_skelly->getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
				if (!_weaponStateDetected) {
					_isWeaponinHand = (*g_player)->actorState.IsWeaponDrawn();
					if (_isWeaponinHand) {
						holsterWeapon(); // holster weapon so we can use primary trigger as an input.
					}
				}
				turnPipBoyOn();
				_skelly->setPipboyHandPose();
				_isOperatingPipboy = true;
				_MESSAGE("Enabling Pipboy with button");
				_stickybpip = false;
			}
			else {
				// stickypip is a guard so we don't constantly toggle the pip boy every frame
				_stickybpip = false;
			}
		}

		if (!isLookingAtPipBoy()) {
			_startedLookingAtPip = 0;
			vr::VRControllerAxis_t axis_state = (g_config->pipBoyButtonArm > 0) ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0];
			const auto timeElapsed = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _lastLookingAtPip;
			if (_pipboyStatus && timeElapsed > g_config->pipBoyOffDelay && !g_configurationMode->isPipBoyConfigModeActive()) {
				_pipboyStatus = false;
				turnPipBoyOff();
				_skelly->getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				if (_isWeaponinHand) {
					drawWeapon(); // draw weapon as we no longer need primary trigger as an input.
					_weaponStateDetected = false;
				}
				_skelly->disablePipboyHandPose();
				_isOperatingPipboy = false;
				//		_MESSAGE("Disabling PipBoy due to inactivity for %d more than %d ms", timeElapsed, g_config->pipBoyOffDelay);
			}
			else if (g_config->pipBoyAllowMovementNotLooking && _pipboyStatus && (axis_state.x != 0 || axis_state.y != 0) && !g_configurationMode->isPipBoyConfigModeActive()) {
				turnPipBoyOff();
				_pipboyStatus = false;
				_skelly->getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				if (_isWeaponinHand) {
					drawWeapon(); // draw weapon as we no longer need primary trigger as an input.
					_weaponStateDetected = false;
				}
				_skelly->disablePipboyHandPose();
				_isOperatingPipboy = false;
				//		_MESSAGE("Disabling PipBoy due to movement when not looking at pipboy. input: (%f, %f)", axis_state.x, axis_state.y);
			}
			return;
		}
		else if (_pipboyStatus)
		{
			_lastLookingAtPip = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		}
		else if (g_config->pipBoyOpenWhenLookAt) {
			if (_startedLookingAtPip == 0) {
				_startedLookingAtPip = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
			}
			else {
				const auto timeElapsed = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _startedLookingAtPip;
				if (timeElapsed > g_config->pipBoyOnDelay) {
					_pipboyStatus = true;
					_skelly->getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
					if (!_weaponStateDetected) {
						_isWeaponinHand = (*g_player)->actorState.IsWeaponDrawn();
						if (_isWeaponinHand) {
							holsterWeapon(); // holster weapon so we can use primary trigger as an input.
						}
					}
					turnPipBoyOn();
					_skelly->setPipboyHandPose();
					_isOperatingPipboy = true;
					_startedLookingAtPip = 0;
				}
			}
		}

		//Why not enable both? So I commented out....

		//if (g_config->pipBoyButtonMode) // If g_config->pipBoyButtonMode, don't check touch
			//return;

		float distance;
		//Virtual Power Button Code
		static BSFixedString pwrButtonTrans("PowerTranslate");
		g_config->leftHandedPipBoy ? pipboy = _skelly->getNode("PowerDetect", _skelly->getRightArm().shoulder->GetAsNiNode()) : pipboy = _skelly->getNode("PowerDetect", _skelly->getLeftArm().shoulder->GetAsNiNode());
		g_config->leftHandedPipBoy ? pipboyTrans = _skelly->getRightArm().forearm3->GetObjectByName(&pwrButtonTrans) : pipboyTrans = _skelly->getLeftArm().forearm3->GetObjectByName(&pwrButtonTrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		distance = vec3_len(finger - pipboy->m_worldTransform.pos);
		if (distance > 2.0) {
			_pipTimer = 0;
			_stickypip = false;
			pipboyTrans->m_localTransform.pos.z = 0.0;
		}
		else {
			if (_pipTimer < 2) {
				_stickypip = false;
				_pipTimer++;
			}
			else {
				float fz = 0 - (2.0 - distance);
				if (fz >= -0.14 && fz <= 0.0) {
					pipboyTrans->m_localTransform.pos.z = (fz);
				}
				if ((pipboyTrans->m_localTransform.pos.z < -0.10) && (!_stickypip)) {
					_stickypip = true;
					if (_vrhook != nullptr) {
						g_config->leftHandedPipBoy ? _vrhook->StartHaptics(1, 0.05, 0.3) : _vrhook->StartHaptics(2, 0.05, 0.3);
					}
					if (_pipboyStatus) {
						_pipboyStatus = false;
						turnPipBoyOff();
						g_configurationMode->exitPBConfig();
						_skelly->getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
					}
					else {
						_pipboyStatus = true;
						_skelly->getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
						turnPipBoyOn();
					}
				}
			}
		}
		//Virtual Light Button Code
		static BSFixedString lhtButtontrans("LightTranslate");
		g_config->leftHandedPipBoy ? pipboy = _skelly->getNode("LightDetect", _skelly->getRightArm().shoulder->GetAsNiNode()) : pipboy = _skelly->getNode("LightDetect", _skelly->getLeftArm().shoulder->GetAsNiNode());
		g_config->leftHandedPipBoy ? pipboyTrans = _skelly->getRightArm().forearm3->GetObjectByName(&lhtButtontrans) : pipboyTrans = _skelly->getLeftArm().forearm3->GetObjectByName(&lhtButtontrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		distance = vec3_len(finger - pipboy->m_worldTransform.pos);
		if (distance > 2.0) {
			stickyPBlight = false;
			pipboyTrans->m_localTransform.pos.z = 0.0;
		}
		else if (distance <= 2.0) {
			float fz = 0 - (2.0 - distance);
			if (fz >= -0.2 && fz <= 0.0) {
				pipboyTrans->m_localTransform.pos.z = (fz);
			}
			if ((pipboyTrans->m_localTransform.pos.z < -0.14) && (!stickyPBlight)) {
				stickyPBlight = true;
				if (_vrhook != nullptr) {
					g_config->leftHandedPipBoy ? _vrhook->StartHaptics(1, 0.05, 0.3) : _vrhook->StartHaptics(2, 0.05, 0.3);
				}
				if (!_pipboyStatus) {
					Offsets::togglePipboyLight(*g_player);
				}
			}
		}
		//Virtual Radio Button Code
		static BSFixedString radioButtontrans("RadioTranslate");
		g_config->leftHandedPipBoy ? pipboy = _skelly->getNode("RadioDetect", _skelly->getRightArm().shoulder->GetAsNiNode()) : pipboy = _skelly->getNode("RadioDetect", _skelly->getLeftArm().shoulder->GetAsNiNode());
		g_config->leftHandedPipBoy ? pipboyTrans = _skelly->getRightArm().forearm3->GetObjectByName(&radioButtontrans) : pipboyTrans = _skelly->getLeftArm().forearm3->GetObjectByName(&radioButtontrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		distance = vec3_len(finger - pipboy->m_worldTransform.pos);
		if (distance > 2.0) {
			stickyPBRadio = false;
			pipboyTrans->m_localTransform.pos.y = 0.0;
		}
		else if (distance <= 2.0) {
			float fz = 0 - (2.0 - distance);
			if (fz >= -0.15 && fz <= 0.0) {
				pipboyTrans->m_localTransform.pos.y = (fz);
			}
			if ((pipboyTrans->m_localTransform.pos.y < -0.12) && (!stickyPBRadio)) {
				stickyPBRadio = true;
				if (_vrhook != nullptr) {
					g_config->leftHandedPipBoy ? _vrhook->StartHaptics(1, 0.05, 0.3) : _vrhook->StartHaptics(2, 0.05, 0.3);
				}
				if (!_pipboyStatus) {
					if (Offsets::isPlayerRadioEnabled()) {
						TurnPlayerRadioOn(false);
					}
					else {
						TurnPlayerRadioOn(true);

					}
				}

			}
		}
	}


	/* ==============================================PIPBOY CONTOLS================================================================================
	//
	// UNIVERSIAL CONTROLS
	//
	// root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0); // changes sub tabs
	// root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0); // changes sub tabs
	// root->Invoke("root.Menu_mc.gotoNextPage", nullptr, nullptr, 0); // changes main tabs
	// root->Invoke("root.Menu_mc.gotoPrevPage", nullptr, nullptr, 0); // changes main tabs
	//
	// INV + RADIO TABS CONTROLS
	//
	// root->Invoke("root.Menu_mc.CurrentPage.List_mc.moveSelectionUp", nullptr, nullptr, 0);  // scrolls up page list
	// root->Invoke("root.Menu_mc.CurrentPage.List_mc.moveSelectionDown", nullptr, nullptr, 0); // scrolls down page list
	//
	// DATA TAB CONTROLS
	//
	// root->Invoke("root.Menu_mc.CurrentPage.StatsTab_mc.CategoryList_mc.moveSelectionUp", nullptr, nullptr, 0) // Scrolls stats page list
	// root->Invoke("root.Menu_mc.CurrentPage.StatsTab_mc.CategoryList_mc.moveSelectionDown", nullptr, nullptr, 0) // Scrolls stats page list
	// root->Invoke("root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc.moveSelectionUp", nullptr, nullptr, 0) // Scrolls Quest page List
	// root->Invoke("root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc.moveSelectionDown", nullptr, nullptr, 0) // Scrolls Quest page List
	// root->Invoke("root.Menu_mc.CurrentPage.WorkshopsTab_mc.List_mc.moveSelectionUp", nullptr, nullptr, 0) // Scolls Workshop page list
	// root->Invoke("root.Menu_mc.CurrentPage.WorkshopsTab_mc.List_mc.moveSelectionDown", nullptr, nullptr, 0) // Scolls Workshop page list
	//
	// STATS TAB CONTROLS
	//
	// root->Invoke("root.Menu_mc.CurrentPage.PerksTab_mc.List_mc.moveSelectionUp", nullptr, nullptr, 0) // Scrolls perks page list
	// root->Invoke("root.Menu_mc.CurrentPage.PerksTab_mc.List_mc.moveSelectionDown", nullptr, nullptr, 0) // Scrolls perks page list
	// root->Invoke("root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc.moveSelectionUp", nullptr, nullptr, 0) // Scrolls SPECIAL page list
	// root->Invoke("root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc.moveSelectionDown", nullptr, nullptr, 0) // Scrolls SPECIAL page list
	//
	// MAP TAB CONTROLS
	//
	// GFxValue akArgs[2];       // Move Map
	// akArgs[0]   <- X Value
	// akArgs[1]   <- Y Value
	// root->Invoke("root.Menu_mc.CurrentPage.WorldMapHolder_mc.PanMap", nullptr, akArgs, 2)
	//
	// INFORMATION
	//
	// 	if (root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage")) {   // Returns Current Page Number (0 = STAT, 1 = INV, 2 = DATA, 3 = MAP, 4 = RADIO)
	//	   X = PBCurrentPage.GetUInt();
	//  }
	// ===============================================================================================================================================*/

	void Pipboy::pipboyManagement() {  //Manages all aspects of Virtual Pipboy usage outside of turning the device / radio / torch on or off. Additionally swaps left hand controls to the right hand.  
		bool isInPA = _skelly->detectInPowerArmor();
		if (!isInPA) {
			static BSFixedString orbNames[7] = { "TabChangeUpOrb", "TabChangeDownOrb", "PageChangeUpOrb", "PageChangeDownOrb", "ScrollItemsUpOrb", "ScrollItemsDownOrb", "SelectItemsOrb" };
			BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_skelly->getRoot();
			bool helmetHeadLamp = _skelly->armorHasHeadLamp();
			bool lightOn = Offsets::isPipboyLightOn(*g_player);
			bool RadioOn = Offsets::isPlayerRadioEnabled();
			Matrix44 rot;
			float radFreq = (Offsets::getPlayerRadioFreq() - 23);
			NiTransform Needle;
			static BSFixedString pwrButtonOn("PowerButton_mesh:2");
			static BSFixedString pwrButtonOff("PowerButton_mesh:off");
			static BSFixedString lhtButtonOn("LightButton_mesh:2");
			static BSFixedString lhtButtonOff("LightButton_mesh:off");
			static BSFixedString radButtonOn("RadioOn");
			static BSFixedString radButtonOff("RadioOff");
			static BSFixedString radioNeedle("RadioNeedle_mesh");
			static BSFixedString NewModeKnob("ModeKnobDuplicate");
			static BSFixedString OriginalModeKnob("ModeKnob02");
			NiAVObject* pipbone = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&pwrButtonOn) : _skelly->getLeftArm().forearm3->GetObjectByName(&pwrButtonOn);
			NiAVObject* pipbone2 = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&pwrButtonOff) : _skelly->getLeftArm().forearm3->GetObjectByName(&pwrButtonOff);
			NiAVObject* pipbone3 = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&lhtButtonOn) : _skelly->getLeftArm().forearm3->GetObjectByName(&lhtButtonOn);
			NiAVObject* pipbone4 = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&lhtButtonOff) : _skelly->getLeftArm().forearm3->GetObjectByName(&lhtButtonOff);
			NiAVObject* pipbone5 = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&radButtonOn) : _skelly->getLeftArm().forearm3->GetObjectByName(&radButtonOn);
			NiAVObject* pipbone6 = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&radButtonOff) : _skelly->getLeftArm().forearm3->GetObjectByName(&radButtonOff);
			NiAVObject* pipbone7 = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&radioNeedle) : _skelly->getLeftArm().forearm3->GetObjectByName(&radioNeedle);
			NiAVObject* pipbone8 = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&NewModeKnob) : _skelly->getLeftArm().forearm3->GetObjectByName(&NewModeKnob);
			NiAVObject* pipbone9 = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&OriginalModeKnob) : _skelly->getLeftArm().forearm3->GetObjectByName(&OriginalModeKnob);
			if (!pipbone || !pipbone2 || !pipbone3 || !pipbone4 || !pipbone5 || !pipbone6 || !pipbone7 || !pipbone8) {
				return;
			}
			if (isLookingAtPipBoy()) {
				BSFlattenedBoneTree* rt = (BSFlattenedBoneTree*)_skelly->getRoot();
				NiPoint3 finger;
				NiAVObject* pipboy = nullptr;
				NiAVObject* pipboyTrans = nullptr;
				g_config->leftHandedPipBoy ? finger = rt->transforms[_skelly->getBoneInMap("LArm_Finger23")].world.pos : finger = rt->transforms[_skelly->getBoneInMap("RArm_Finger23")].world.pos;
				g_config->leftHandedPipBoy ? pipboy = _skelly->getNode("PipboyRoot", _skelly->getRightArm().shoulder->GetAsNiNode()) : pipboy = _skelly->getNode("PipboyRoot", _skelly->getLeftArm().shoulder->GetAsNiNode());
				float distance;
				distance = vec3_len(finger - pipboy->m_worldTransform.pos);
				if ((distance < g_config->pipboyDetectionRange) && !_isOperatingPipboy && !_pipboyStatus) { // Hides Weapon and poses hand for pointing
					_isOperatingPipboy = true;
					_isWeaponinHand = (*g_player)->actorState.IsWeaponDrawn();
					if (_isWeaponinHand) {
						_weaponStateDetected = true;
						holsterWeapon();
					}
					_skelly->setPipboyHandPose();
				}
				if ((distance > g_config->pipboyDetectionRange) && _isOperatingPipboy && !_pipboyStatus) { // Restores Weapon and releases hand pose
					_isOperatingPipboy = false;
					_skelly->disablePipboyHandPose();
					if (_isWeaponinHand) {
						_weaponStateDetected = false;
						drawWeapon();
					}
				}
			}
			else if (!isLookingAtPipBoy() && _isOperatingPipboy && !_pipboyStatus) { // Catches if you're not looking at the pipboy when your hand moves outside of the control area and restores weapon / releases hand pose							
				_skelly->disablePipboyHandPose();
				for (int i = 0; i < 7; i++) {  // Remove any stuck helper orbs if Pipboy times out for any reason.
					NiAVObject* orb = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&orbNames[i]) : _skelly->getLeftArm().forearm3->GetObjectByName(&orbNames[i]);
					if (orb != nullptr) {
						if (orb->m_localTransform.scale > 0) {
							orb->m_localTransform.scale = 0;
						}
					}
				}
				if (_isWeaponinHand) {
					_weaponStateDetected = false;
					drawWeapon();
				}
				_isOperatingPipboy = false;
			}
			if (_lastPipboyPage == 4) {  // fixes broken 'Mode Knob' position when radio tab is selected
				rot.makeTransformMatrix(pipbone8->m_localTransform.rot, NiPoint3(0, 0, 0));
				float rotx;
				float roty;
				float rotz;
				rot.getEulerAngles(&rotx, &roty, &rotz);
				if (rotx < 0.57) {
					Matrix44 kRot;
					kRot.setEulerAngles(-0.05, degrees_to_rads(0), degrees_to_rads(0));
					pipbone8->m_localTransform.rot = kRot.multiply43Right(pipbone8->m_localTransform.rot);
				}
			}
			else { // restores control of the 'Mode Knob' to the Pipboy behaviour file
				pipbone8->m_localTransform.rot = pipbone9->m_localTransform.rot;
			}
			// Controls Pipboy power light glow (on or off depending on Pipboy state)
			_pipboyStatus ? pipbone->flags &= 0xfffffffffffffffe : pipbone2->flags &= 0xfffffffffffffffe;
			_pipboyStatus ? pipbone->m_localTransform.scale = 1 : pipbone2->m_localTransform.scale = 1;
			_pipboyStatus ? pipbone2->flags |= 0x1 : pipbone->flags |= 0x1;
			_pipboyStatus ? pipbone2->m_localTransform.scale = 0 : pipbone->m_localTransform.scale = 0;
			// Control switching between hand and head based Pipboy light 
			if (lightOn && !helmetHeadLamp) {
				NiPoint3 hand;
				NiNode* head = _skelly->getNode("Head", (*g_player)->GetActorRootNode(false)->GetAsNiNode());
				if (!head) {
					return;
				}
				g_config->leftHandedPipBoy ? hand = rt->transforms[_skelly->getBoneInMap("RArm_Hand")].world.pos : hand = rt->transforms[_skelly->getBoneInMap("LArm_Hand")].world.pos;
				float distance = vec3_len(hand - head->m_worldTransform.pos);
				if (distance < 15.0) {
					uint64_t _PipboyHand = (g_config->leftHandedPipBoy ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed);
					const auto SwitchLightButton = _PipboyHand & vr::ButtonMaskFromId((vr::EVRButtonId)g_config->switchTorchButton);
					if (_vrhook != nullptr && _SwitchLightHaptics) {
						g_config->leftHandedPipBoy ? _vrhook->StartHaptics(2, 0.1, 0.1) : _vrhook->StartHaptics(1, 0.1, 0.1);
					}
					// Control switching between hand and head based Pipboy light
					if (SwitchLightButton && !_SwithLightButtonSticky) {
						_SwithLightButtonSticky = true;
						_SwitchLightHaptics = false;
						if (_vrhook != nullptr) {
							g_config->leftHandedPipBoy ? _vrhook->StartHaptics(2, 0.05, 0.3) : _vrhook->StartHaptics(1, 0.05, 0.3);
						}
						NiNode* LGHT_ATTACH = g_config->leftHandedPipBoy ? _skelly->getNode("RArm_Hand", _skelly->getRightArm().shoulder->GetAsNiNode()) : _skelly->getNode("LArm_Hand", _skelly->getLeftArm().shoulder->GetAsNiNode());
						NiNode* lght = g_config->isPipBoyTorchOnArm ? get1stChildNode("HeadLightParent", LGHT_ATTACH) : _skelly->getPlayerNodes()->HeadLightParentNode->GetAsNiNode();
						if (lght) {
							BSFixedString parentnode = g_config->isPipBoyTorchOnArm ? lght->m_parent->m_name : _skelly->getPlayerNodes()->HeadLightParentNode->m_parent->m_name;
							Matrix44 LightRot;
							int Rotz = g_config->isPipBoyTorchOnArm ? -90 : 90;
							LightRot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0), degrees_to_rads(Rotz));
							lght->m_localTransform.rot = LightRot.multiply43Right(lght->m_localTransform.rot);
							int PosY = g_config->isPipBoyTorchOnArm ? 0 : 4;
							lght->m_localTransform.pos.y = PosY;
							g_config->isPipBoyTorchOnArm ? lght->m_parent->RemoveChild(lght) : _skelly->getPlayerNodes()->HeadLightParentNode->m_parent->RemoveChild(lght);
							g_config->isPipBoyTorchOnArm ? _skelly->getPlayerNodes()->HmdNode->AttachChild(lght, true) : LGHT_ATTACH->AttachChild(lght, true);
							g_config->togglePipBoyTorchOnArm();
						}
					}
					if (!SwitchLightButton) {
						_SwithLightButtonSticky = false;
					}

				}
				else if (distance > 10) {
					_SwitchLightHaptics = true;
					_SwithLightButtonSticky = false;
				}
			}
			//Attach light to hand 
			if (g_config->isPipBoyTorchOnArm) {
				NiNode* LGHT_ATTACH = g_config->leftHandedPipBoy ? _skelly->getNode("RArm_Hand", _skelly->getRightArm().shoulder->GetAsNiNode()) : _skelly->getNode("LArm_Hand", _skelly->getLeftArm().shoulder->GetAsNiNode());
				if (LGHT_ATTACH) {
					if (lightOn && !helmetHeadLamp) {
						NiNode* lght = _skelly->getPlayerNodes()->HeadLightParentNode->GetAsNiNode();
						BSFixedString parentnode = _skelly->getPlayerNodes()->HeadLightParentNode->m_parent->m_name;
						if (parentnode == "HMDNode") {
							_skelly->getPlayerNodes()->HeadLightParentNode->m_parent->RemoveChild(lght);
							Matrix44 LightRot;
							LightRot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0), degrees_to_rads(90));
							lght->m_localTransform.rot = LightRot.multiply43Right(lght->m_localTransform.rot);
							lght->m_localTransform.pos.y = 4;
							LGHT_ATTACH->AttachChild(lght, true);
						}
					}
					//Restore HeadLight to correct node when light is powered off (to avoid any crashes)
					else if (!lightOn || helmetHeadLamp) {
						NiNode* lght = get1stChildNode("HeadLightParent", LGHT_ATTACH);
						if (lght) {
							BSFixedString parentnode = lght->m_parent->m_name;
							if (parentnode != "HMDNode") {
								Matrix44 LightRot;
								LightRot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0), degrees_to_rads(-90));
								lght->m_localTransform.rot = LightRot.multiply43Right(lght->m_localTransform.rot);
								lght->m_localTransform.pos.y = 0;
								lght->m_parent->RemoveChild(lght);
								_skelly->getPlayerNodes()->HmdNode->AttachChild(lght, true);
							}
						}
					}
				}
			}
			// Controls Radio / Light on & off indicators
			lightOn ? pipbone3->flags &= 0xfffffffffffffffe : pipbone4->flags &= 0xfffffffffffffffe;
			lightOn ? pipbone3->m_localTransform.scale = 1 : pipbone4->m_localTransform.scale = 1;
			lightOn ? pipbone4->flags |= 0x1 : pipbone3->flags |= 0x1;
			lightOn ? pipbone4->m_localTransform.scale = 0 : pipbone3->m_localTransform.scale = 0;
			RadioOn ? pipbone5->flags &= 0xfffffffffffffffe : pipbone6->flags &= 0xfffffffffffffffe;
			RadioOn ? pipbone5->m_localTransform.scale = 1 : pipbone6->m_localTransform.scale = 1;
			RadioOn ? pipbone6->flags |= 0x1 : pipbone5->flags |= 0x1;
			RadioOn ? pipbone6->m_localTransform.scale = 0 : pipbone5->m_localTransform.scale = 0;
			// Controls Radio Needle Position.
			if (RadioOn && (radFreq != lastRadioFreq)) {
				float x = -1 * (radFreq - lastRadioFreq);
				rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(x), degrees_to_rads(0));
				pipbone7->m_localTransform.rot = rot.multiply43Right(pipbone7->m_localTransform.rot);
				lastRadioFreq = radFreq;
			}
			else if (!RadioOn && lastRadioFreq > 0) {
				float x = lastRadioFreq;
				rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(x), degrees_to_rads(0));
				pipbone7->m_localTransform.rot = rot.multiply43Right(pipbone7->m_localTransform.rot);
				lastRadioFreq = 0.0;
			}
			// Scaleform code for managing Pipboy menu controls (Virtual and Physical)
			if (_pipboyStatus) {
				BSFixedString pipboyMenu("PipboyMenu");
				IMenu* menu = (*g_ui)->GetMenu(pipboyMenu);
				if (menu != nullptr) {
					GFxMovieRoot* root = menu->movie->movieRoot;
					if (root != nullptr) {
						GFxValue IsProjected;
						GFxValue PBCurrentPage;
						if (root->GetVariable(&IsProjected, "root.Menu_mc.projectedBorder_mc.visible")) { //check if Pipboy is projected and disable right stick rotation if it isn't
							bool UIProjected = IsProjected.GetBool();
							if (!UIProjected && g_config->switchUIControltoPrimary) {
								//prevents player rotation controls so we can switch controls to the right stick (or left if the Pipboy is on the right arm)
								rotationStickEnabledToggle(false); 
							}
						}
						if (root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage")) {  // Get Current Pipboy Tab and store it.
							if (PBCurrentPage.GetType() != GFxValue::kType_Undefined) {
								_lastPipboyPage = PBCurrentPage.GetUInt();
							}
						}
						static BSFixedString boneNames[7] = { "TabChangeUp", "TabChangeDown", "PageChangeUp", "PageChangeDown", "ScrollItemsUp", "ScrollItemsDown", "SelectButton02" };
						static BSFixedString transNames[7] = { "TabChangeUpTrans", "TabChangeDownTrans", "PageChangeUpTrans", "PageChangeDownTrans", "ScrollItemsUpTrans", "ScrollItemsDownTrans", "SelectButtonTrans" };
						float boneDistance[7] = { 2.0, 2.0, 2.0, 2.0, 1.5, 1.5, 2.0 };
						float transDistance[7] = { 0.6, 0.6, 0.6, 0.6, 0.1, 0.1, 0.4 };
						float maxDistance[7] = { 1.2,  1.2,  1.2,  1.2, 1.2,  1.2, 0.6 };
						NiPoint3 finger;
						NiAVObject* pipboy = nullptr;
						NiAVObject* pipboyTrans = nullptr;
						// Virtual Controls Code Starts here: 
						g_config->leftHandedPipBoy ? finger = rt->transforms[_skelly->getBoneInMap("LArm_Finger23")].world.pos : finger = rt->transforms[_skelly->getBoneInMap("RArm_Finger23")].world.pos;
						for (int i = 0; i < 7; i++) {
							NiAVObject* bone = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&boneNames[i]) : _skelly->getLeftArm().forearm3->GetObjectByName(&boneNames[i]);
							NiNode* trans = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&transNames[i])->GetAsNiNode() : _skelly->getLeftArm().forearm3->GetObjectByName(&transNames[i])->GetAsNiNode();
							if (bone && trans) {
								float distance = vec3_len(finger - bone->m_worldTransform.pos);
								if (distance > boneDistance[i]) {
									trans->m_localTransform.pos.z = 0.0;
									_PBControlsSticky[i] = false;
									NiAVObject* orb = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&orbNames[i]) : _skelly->getLeftArm().forearm3->GetObjectByName(&orbNames[i]); //Hide helper Orbs when not near a control surface
									if (orb != nullptr) {
										if (orb->m_localTransform.scale > 0) {
											orb->m_localTransform.scale = 0;
										}
									}
								}
								else if (distance <= boneDistance[i]) {
									float fz = (boneDistance[i] - distance);
									NiAVObject* orb = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&orbNames[i]) : _skelly->getLeftArm().forearm3->GetObjectByName(&orbNames[i]); //Show helper Orbs when not near a control surface
									if (orb != nullptr) {
										if (orb->m_localTransform.scale < 1) {
											orb->m_localTransform.scale = 1;
										}
									}
									if (fz > 0.0 && fz < maxDistance[i]) {
										trans->m_localTransform.pos.z = (fz);
										if (i == 4) {  // Move Scroll Knob Anti-Clockwise when near control surface
											static BSFixedString KnobNode = "ScrollItemsKnobRot";
											NiAVObject* ScrollKnob = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&KnobNode) : _skelly->getLeftArm().forearm3->GetObjectByName(&KnobNode);
											Matrix44 rot;
											rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(fz), degrees_to_rads(0));
											ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
										}
										if (i == 5) { // Move Scroll Knob Clockwise when near control surface
											float roty = (fz * -1);
											static BSFixedString KnobNode = "ScrollItemsKnobRot";
											NiAVObject* ScrollKnob = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&KnobNode) : _skelly->getLeftArm().forearm3->GetObjectByName(&KnobNode);
											Matrix44 rot;
											rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(roty), degrees_to_rads(0));
											ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
										}
									}
									if ((trans->m_localTransform.pos.z > transDistance[i]) && !_PBControlsSticky[i]) {
										if (_vrhook != nullptr) {
											_PBControlsSticky[i] = true;
											g_config->leftHandedMode ? _vrhook->StartHaptics(1, 0.05, 0.3) : _vrhook->StartHaptics(2, 0.05, 0.3);
											if (i == 0) {
												root->Invoke("root.Menu_mc.gotoPrevPage", nullptr, nullptr, 0); // Previous Page											
											}
											if (i == 1) {
												root->Invoke("root.Menu_mc.gotoNextPage", nullptr, nullptr, 0);  // Next Page				
											}
											if (i == 2) {
												root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0); // Previous Sub Page
											}
											if (i == 3) {
												root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0); // Next Sub Page
											}
											if (i == 4) {
												std::thread t1(SimulateExtendedButtonPress, VK_UP); // Scroll up list
												t1.detach();
											}
											if (i == 5) {
												std::thread t1(SimulateExtendedButtonPress, VK_DOWN); // Scroll down list
												t1.detach();
											}
											if (i == 6) {
												std::thread t1(SimulateExtendedButtonPress, VK_RETURN); // Select Current Item
												t1.detach();
											}

										}
									}
								}
							}
						}
						// Mirror Left Stick Controls on Right Stick.
						if (!g_configurationMode->isPipBoyConfigModeActive() && g_config->switchUIControltoPrimary) {
							BSFixedString selectnodename = "SelectRotate";
							NiNode* trans = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&selectnodename)->GetAsNiNode() : _skelly->getLeftArm().forearm3->GetObjectByName(&selectnodename)->GetAsNiNode();
							vr::VRControllerAxis_t doinantHandStick = (g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0]);
							vr::VRControllerAxis_t doinantTrigger = (g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[1] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[1]);
							vr::VRControllerAxis_t secondaryTrigger = (g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[1] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[1]);
							uint64_t dominantHand = (g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed);
							const auto UISelectButton = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)33); // Right Trigger
							const auto UIAltSelectButton = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)32); // Right Touchpad
							GFxValue GetSWFVar;
							bool isPBMessageBoxVisible = false;
							// Move Pipboy trigger mesh with controller trigger position.
							if (trans != nullptr) {
								if (doinantTrigger.x > 0.00 && secondaryTrigger.x == 0.0) {
									trans->m_localTransform.pos.z = (doinantTrigger.x / 3) * -1;
								}
								else if (secondaryTrigger.x > 0.00 && doinantTrigger.x == 0.0) {
									trans->m_localTransform.pos.z = (secondaryTrigger.x / 3) * -1;
								}
								else {
									trans->m_localTransform.pos.z = 0.00;
								}
							}
							if (root->GetVariable(&GetSWFVar, "root.Menu_mc.CurrentPage.MessageHolder_mc.visible")) {
								isPBMessageBoxVisible = GetSWFVar.GetBool();
							}
							if (_lastPipboyPage != 3 || isPBMessageBoxVisible) {
								static BSFixedString KnobNode = "ScrollItemsKnobRot";
								NiAVObject* ScrollKnob = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&KnobNode) : _skelly->getLeftArm().forearm3->GetObjectByName(&KnobNode);
								Matrix44 rot;
								if (doinantHandStick.y > 0.85) {
									rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0.4), degrees_to_rads(0));
									ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
								}
								if (doinantHandStick.y < -0.85) {
									rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(-0.4), degrees_to_rads(0));
									ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
								}
							}
							if (_lastPipboyPage == 3 && !isPBMessageBoxVisible) { // Map Tab
								GFxValue akArgs[2];
								akArgs[0].SetNumber(doinantHandStick.x * -1);
								akArgs[1].SetNumber(doinantHandStick.y);
								if (root->Invoke("root.Menu_mc.CurrentPage.WorldMapHolder_mc.PanMap", nullptr, akArgs, 2)) {}	// Move Map	
								if (root->Invoke("root.Menu_mc.CurrentPage.LocalMapHolder_mc.PanMap", nullptr, akArgs, 2)) {}
							}
							else {
								if (doinantHandStick.y > 0.85) {
									if (!_controlSleepStickyY) {
										_controlSleepStickyY = true;
										std::thread t1(SimulateExtendedButtonPress, VK_UP); // Scroll up list
										t1.detach();
										std::thread t2(RightStickYSleep, 155);
										t2.detach();
									}
								}
								if (doinantHandStick.y < -0.85) {
									if (!_controlSleepStickyY) {
										_controlSleepStickyY = true;
										std::thread t1(SimulateExtendedButtonPress, VK_DOWN); // Scroll down list
										t1.detach();
										std::thread t2(RightStickYSleep, 155);
										t2.detach();
									}
								}
								if (doinantHandStick.x < -0.85) {
									if (!_controlSleepStickyX) {
										_controlSleepStickyX = true;
										root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0); // Next Sub Page
										std::thread t3(RightStickXSleep, 170);
										t3.detach();
									}
								}
								if (doinantHandStick.x > 0.85) {
									if (!_controlSleepStickyX) {
										_controlSleepStickyX = true;
										root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0); // Previous Sub Page
										std::thread t3(RightStickXSleep, 170);
										t3.detach();
									}
								}
							}
							if (UISelectButton && !_UISelectSticky) {
								_UISelectSticky = true;
								std::thread t1(SimulateExtendedButtonPress, VK_RETURN); // Select Current Item
								t1.detach();
							}
							else if (!UISelectButton) {
								_UISelectSticky = false;
							}
							if (UIAltSelectButton && !_UIAltSelectSticky) {
								_UIAltSelectSticky = true;
								if (root->Invoke("root.Menu_mc.CurrentPage.onMessageButtonPress()", nullptr, nullptr, 0)) {}
							}
							else if (!UIAltSelectButton) {
								_UIAltSelectSticky = false;
							}
						}
						else if (!g_configurationMode->isPipBoyConfigModeActive() && !g_config->switchUIControltoPrimary) {
							//still move Pipboy trigger mesh even if controls havent been swapped.
							vr::VRControllerAxis_t secondaryTrigger = (g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[1] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[1]);
							vr::VRControllerAxis_t offHandStick = (g_config->leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0]);
							BSFixedString selectnodename = "SelectRotate";
							NiNode* trans = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&selectnodename)->GetAsNiNode() : _skelly->getLeftArm().forearm3->GetObjectByName(&selectnodename)->GetAsNiNode();
							if (trans != nullptr) {
								if (secondaryTrigger.x > 0.00) {
									trans->m_localTransform.pos.z = (secondaryTrigger.x / 3) * -1;
								}
								else {
									trans->m_localTransform.pos.z = 0.00;
								}
							}
							//still move Pipboy scroll knob even if controls havent been swapped.
							bool isPBMessageBoxVisible = false;
							GFxValue GetSWFVar;
							if (root->GetVariable(&GetSWFVar, "root.Menu_mc.CurrentPage.MessageHolder_mc.visible")) {
								isPBMessageBoxVisible = GetSWFVar.GetBool();
							}
							if (_lastPipboyPage != 3 || isPBMessageBoxVisible) {
								static BSFixedString KnobNode = "ScrollItemsKnobRot";
								NiAVObject* ScrollKnob = g_config->leftHandedPipBoy ? _skelly->getRightArm().forearm3->GetObjectByName(&KnobNode) : _skelly->getLeftArm().forearm3->GetObjectByName(&KnobNode);
								Matrix44 rot;
								if (offHandStick.x > 0.85) {
									rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(0.4), degrees_to_rads(0));
									ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
								}
								if (offHandStick.x < -0.85) {
									rot.setEulerAngles(degrees_to_rads(0), degrees_to_rads(-0.4), degrees_to_rads(0));
									ScrollKnob->m_localTransform.rot = rot.multiply43Right(ScrollKnob->m_localTransform.rot);
								}
							}
						}
					}
				}
			}
		}
		else if (isInPA) {
			lastRadioFreq = 0.0;  // Ensures Radio needle doesn't get messed up when entering and then exiting Power Armor.
			// Continue to update Pipboy page info when in Power Armor.
			BSFixedString pipboyMenu("PipboyMenu");
			IMenu* menu = (*g_ui)->GetMenu(pipboyMenu);
			if (menu != nullptr) {
				GFxMovieRoot* root = menu->movie->movieRoot;
				if (root != nullptr) {
					GFxValue PBCurrentPage;
					if (root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage")) {
						if (PBCurrentPage.GetType() != GFxValue::kType_Undefined) {
							_lastPipboyPage = PBCurrentPage.GetUInt();
						}
					}
				}
			}
		}
	}

	void Pipboy::dampenPipboyScreen() {
		if (!g_config->dampenPipboyScreen) {
			return;
		}
		else if (!_pipboyStatus) {
			_pipboyScreenPrevFrame = _skelly->getPlayerNodes()->ScreenNode->m_worldTransform;
			return;
		}
		NiNode* pipboyScreen = _skelly->getPlayerNodes()->ScreenNode;

		if (pipboyScreen && _pipboyStatus) {
			Quaternion rq, rt;
			// do a spherical interpolation between previous frame and current frame for the world rotation matrix
			NiTransform prevFrame = _pipboyScreenPrevFrame;
			rq.fromRot(prevFrame.rot);
			rt.fromRot(pipboyScreen->m_worldTransform.rot);
			rq.slerp(1 - g_config->dampenPipboyRotation, rt);
			pipboyScreen->m_worldTransform.rot = rq.getRot().make43();
			// do a linear interprolation  between the position from the previous frame to current frame
			NiPoint3 deltaPos = pipboyScreen->m_worldTransform.pos - prevFrame.pos;
			deltaPos *= g_config->dampenPipboyTranslation;  // just use hands dampening value for now
			pipboyScreen->m_worldTransform.pos -= deltaPos;
			_pipboyScreenPrevFrame = pipboyScreen->m_worldTransform;
			_skelly->updateDown(pipboyScreen->GetAsNiNode(), false);
		}
	}

	bool Pipboy::isLookingAtPipBoy() {
		BSFixedString wandPipName("PipboyRoot_NIF_ONLY");
		NiAVObject* pipboy = _skelly->getPlayerNodes()->SecondaryWandNode->GetObjectByName(&wandPipName);

		if (pipboy == nullptr) {
			return false;
		}

		BSFixedString screenName("Screen:0");
		NiAVObject* screen = pipboy->GetObjectByName(&screenName);
		if (screen == nullptr) {
			return false;
		}

		return  isCameraLookingAtObject((*g_playerCamera)->cameraNode, screen, g_config->pipBoyLookAtGate);

		//NiPoint3 pipBoyOut = screen->m_worldTransform.rot * NiPoint3(0, -1, 0);
		//NiPoint3 lookDir = (*g_playerCamera)->cameraNode->m_worldTransform.rot * NiPoint3(0, 1, 0);

		//float dot = vec3_dot(vec3_norm(pipBoyOut), vec3_norm(lookDir));

		//return dot < -(pipBoyLookAtGate);
	}
}