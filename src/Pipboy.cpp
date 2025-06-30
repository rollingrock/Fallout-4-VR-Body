#include "Pipboy.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <f4se/GameCamera.h>

#include "Config.h"
#include "ConfigurationMode.h"
#include "FRIK.h"
#include "HandPose.h"
#include "utils.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "common/Matrix.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/scaleformUtils.h"
#include "f4vr/VRControllersManager.h"

using namespace std::chrono;
using namespace common;

namespace {
	bool isPrimaryTriggerPressed() {
		return f4vr::VRControllers.isPressed(vr::k_EButton_SteamVR_Trigger, f4vr::Hand::Primary);
	}

	bool isAButtonPressed() {
		return f4vr::VRControllers.isPressed(vr::k_EButton_A, f4vr::Hand::Primary);
	}

	bool isBButtonPressed() {
		return f4vr::VRControllers.isPressed(vr::k_EButton_ApplicationMenu, f4vr::Hand::Primary);
	}

	bool isPrimaryGripPressHeldDown() {
		return f4vr::VRControllers.isPressHeldDown(vr::k_EButton_Grip, f4vr::Hand::Primary);
	}

	bool isWorldMapVisible(const GFxMovieRoot* root) {
		return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.WorldMapHolder_mc");
	}

	std::string getCurrentMapPath(const GFxMovieRoot* root, const std::string& suffix = "") {
		return (isWorldMapVisible(root) ? "root.Menu_mc.CurrentPage.WorldMapHolder_mc" : "root.Menu_mc.CurrentPage.LocalMapHolder_mc") + suffix;
	}

	/**
	 * Is context menu message box popup is visible or not. Only one can be visible at a time.
	 */
	bool isMessageHolderVisible(const GFxMovieRoot* root) {
		return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.MessageHolder_mc")
			|| f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc");
	}

	bool isQuestTabVisibleOnDataPage(const GFxMovieRoot* root) {
		return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.QuestsTab_mc");
	}

	bool isQuestTabObjectiveListEnabledOnDataPage(const GFxMovieRoot* root) {
		GFxValue var;
		return root->GetVariable(&var, "root.Menu_mc.CurrentPage.QuestsTab_mc.ObjectivesList_mc.selectedIndex") && var.GetType() == GFxValue::kType_Int && var.GetInt() > -1;
	}

	bool isWorkshopsTabVisibleOnDataPage(const GFxMovieRoot* root) {
		return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc");
	}

	void triggerHeptic() {
		f4vr::VRControllers.triggerHaptic(f4vr::Hand::Primary, 0.001f);
	}
}

namespace frik {
	/**
	 * Turn on the Pipboy and set the status flags.
	 */
	void Pipboy::turnOn() {
		_pipboyStatus = true;
		_isOperatingPipboy = true;
		_stickybpip = false;
		// err... without this line the pipboy screen is not visible...
		f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
		turnPipBoyOn();
	}

	void Pipboy::swapPipboy() {
		_pipboyStatus = false;
		_pipTimer = 0;
	}

	void Pipboy::onSetNodes() {
		if (!_skelly) {
			return;
		}
		if (NiNode* screenNode = f4vr::getPlayerNodes()->ScreenNode) {
			if (const NiAVObject* screen = screenNode->GetObjectByName(&BSFixedString("Screen:0"))) {
				_pipboyScreenPrevFrame = screen->m_worldTransform;
			}
		}
	}

	/**
	 * Run Pipboy mesh replacement if not already done (or forced) to the configured meshes either holo or screen.
	 */
	/// <param name="force">true - run mesh replace, false - only if not previously replaced</param>
	void Pipboy::replaceMeshes(const bool force) {
		if (force || !meshesReplaced) {
			if (g_config.isHoloPipboy == 0) {
				replaceMeshes("HoloEmitter", "Screen");
			} else if (g_config.isHoloPipboy == 1) {
				replaceMeshes("Screen", "HoloEmitter");
			}
		}
	}

	/**
	 * Executed every frame to update the Pipboy location and handle interaction with pipboy config UX.
	 * TODO: refactor into separate functions for each functionality
	 */
	void Pipboy::onFrameUpdate() {
		replaceMeshes(false);

		operatePipBoy();

		pipboyManagement();

		dampenPipboyScreen();

		if (f4vr::isInPowerArmor()) {
			return;
		}

		//Hide some Pipboy related meshes on exit of Power Armor if they're not hidden
		NiNode* hideNode;
		g_config.isHoloPipboy
			? hideNode = f4vr::getChildNode("Screen", (*g_player)->unkF0->rootNode)
			: hideNode = f4vr::getChildNode("HoloEmitter", (*g_player)->unkF0->rootNode);
		if (hideNode) {
			if (hideNode->m_localTransform.scale != 0) {
				hideNode->flags |= 0x1;
				hideNode->m_localTransform.scale = 0;
			}
		}

		// TODO: moved from end of F4VRBody, check works well
		// sets 3rd Person Pipboy Scale
		if (const auto pipboy3Rd = f4vr::getChildNode("PipboyBone", (*g_player)->unkF0->rootNode)) {
			pipboy3Rd->m_localTransform.scale = g_config.pipBoyScale;
		}
	}

	void Pipboy::fixMissingScreen() {
		const auto pn = f4vr::getPlayerNodes();
		if (const auto screenNode = pn->ScreenNode) {
			const BSFixedString screenName("Screen:0");
			const NiAVObject* newScreen = screenNode->GetObjectByName(&screenName);

			if (!newScreen) {
				pn->ScreenNode->RemoveChildAt(0);

				newScreen = pn->PipboyRoot_nif_only_node->GetObjectByName(&screenName)->m_parent;
				NiNode* rn = f4vr::addNode((uint64_t)&pn->ScreenNode, newScreen);
			}
		}
	}

	/**
	 * Handle replacing of Pipboy meshes on the arm with either screen or holo emitter.
	 */
	void Pipboy::replaceMeshes(const std::string& itemHide, const std::string& itemShow) {
		const auto pn = f4vr::getPlayerNodes();
		const NiNode* ui = pn->primaryUIAttachNode;
		NiNode* wand = f4vr::get1StChildNode("world_primaryWand.nif", ui);
		NiNode* retNode = vrui::loadNifFromFile("Data/Meshes/FRIK/_primaryWand.nif");
		if (retNode) {
			// ui->RemoveChild(wand);
			// ui->AttachChild(retNode, true);
		}

		wand = pn->SecondaryWandNode;
		NiNode* pipParent = f4vr::get1StChildNode("PipboyParent", wand);

		if (!pipParent) {
			meshesReplaced = false;
			return;
		}
		wand = f4vr::get1StChildNode("PipboyRoot_NIF_ONLY", pipParent);
		g_config.isHoloPipboy ? retNode = vrui::loadNifFromFile("Data/Meshes/FRIK/HoloPipboyVR.nif") : retNode = vrui::loadNifFromFile("Data/Meshes/FRIK/PipboyVR.nif");
		if (retNode && wand) {
			const BSFixedString screenName("Screen:0");
			const NiAVObject* newScreen = retNode->GetObjectByName(&screenName)->m_parent;

			if (!newScreen) {
				meshesReplaced = false;
				return;
			}

			pipParent->RemoveChild(wand);
			pipParent->AttachChild(retNode, true);

			pn->ScreenNode->RemoveChildAt(0);
			// using native function here to attach the new screen as too lazy to fully reverse what it's doing and it works fine.
			NiNode* rn = f4vr::addNode((uint64_t)&pn->ScreenNode, newScreen);
			pn->PipboyRoot_nif_only_node = retNode;
		}

		meshesReplaced = true;

		static BSFixedString wandPipName("PipboyRoot");
		if (const auto pbRoot = pn->SecondaryWandNode->GetObjectByName(&wandPipName)) {
			pbRoot->m_localTransform = g_config.getPipboyOffset();
		}

		pn->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0; //prevents the VRPipboy screen from being displayed on first load whilst PB is off.
		if (const auto hideNode = f4vr::getChildNode(itemHide.c_str(), (*g_player)->unkF0->rootNode)) {
			hideNode->flags |= 0x1;
			hideNode->m_localTransform.scale = 0;
		}
		if (const auto showNode = f4vr::getChildNode(itemShow.c_str(), (*g_player)->unkF0->rootNode)) {
			showNode->flags &= 0xfffffffffffffffe;
			showNode->m_localTransform.scale = 1;
		}

		logger::info("Pipboy Meshes replaced! Hide: {}, Show: {}", itemHide.c_str(), itemShow.c_str());
	}

	/**
	 * See documentation: https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Development-%E2%80%90-Pipboy-Controls
	 */
	void Pipboy::operatePipBoy() {
		if ((*g_player)->firstPersonSkeleton == nullptr) {
			return;
		}

		RE::NiPoint3 finger;
		const NiAVObject* pipboy;
		NiAVObject* pipboyTrans;
		g_config.leftHandedPipBoy
			? finger = _skelly->getBoneWorldTransform("LArm_Finger23").translate
			: finger = _skelly->getBoneWorldTransform("RArm_Finger23").translate;
		g_config.leftHandedPipBoy
			? pipboy = f4vr::getNode("PipboyRoot", _skelly->getRightArm().shoulder->GetAsNiNode())
			: pipboy = f4vr::getNode("PipboyRoot", _skelly->getLeftArm().shoulder->GetAsNiNode());
		if (pipboy == nullptr) {
			return;
		}

		const auto pipOnButtonPressed = (g_config.pipBoyButtonArm
			? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed
			: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed) & vr::ButtonMaskFromId(
			static_cast<vr::EVRButtonId>(g_config.pipBoyButtonID));
		const auto pipOffButtonPressed = (g_config.pipBoyButtonOffArm
			? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed
			: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed) & vr::ButtonMaskFromId(
			static_cast<vr::EVRButtonId>(g_config.pipBoyButtonOffID));

		// check off button
		if (pipOffButtonPressed && !_stickyoffpip) {
			if (_pipboyStatus) {
				_pipboyStatus = false;
				turnPipBoyOff();
				g_frik.closePipboyConfigurationModeActive();
				if (_isWeaponinHand) {
					_weaponStateDetected = false;
				}
				disablePipboyHandPose();
				f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				logger::info("Disabling Pipboy with button");
				_stickyoffpip = true;
			}
		} else if (!pipOffButtonPressed) {
			// guard so we don't constantly toggle the pip boy off every frame
			_stickyoffpip = false;
		}

		/* Refactored this part of the code so that turning on the wrist based Pipboy works the same way as the 'Projected Pipboy'. It works on button release rather than press,
		this enables us to determine if the button was held for a short or long press by the status of the '_controlSleepStickyT' bool. If it is still set to true on button release
		then we know the button was a short press, if it is set to false we know it was a long press. Long press = torch on / off, Short Press = Pipboy enable.
		*/

		if (pipOnButtonPressed && !_stickybpip && !_isOperatingPipboy) {
			_stickybpip = true;
			_controlSleepStickyT = true;
			std::thread t5(&Pipboy::secondaryTriggerSleep, this, 300); // switches a bool to false after 150ms
			t5.detach();
		} else if (!pipOnButtonPressed) {
			if (_controlSleepStickyT && _stickybpip && (!g_config.pipboyOpenWhenLookAt || isLookingAtPipBoy())) {
				// if bool is still set to true on control release we know it was a short press.
				_pipboyStatus = true;
				f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
				turnPipBoyOn();
				setPipboyHandPose();
				_isOperatingPipboy = true;
				logger::info("Enabling Pipboy with button");
				_stickybpip = false;
			} else {
				// guard so we don't constantly toggle the pip boy every frame
				_stickybpip = false;
			}
		}

		if (!isLookingAtPipBoy()) {
			_startedLookingAtPip = 0;
			const vr::VRControllerAxis_t movingStick = g_config.pipBoyButtonArm > 0
				? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0]
				: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0];
			const vr::VRControllerAxis_t lookingStick = f4vr::isLeftHandedMode()
				? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0]
				: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0];
			const auto timeElapsed = static_cast<int>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _lastLookingAtPip);
			const bool closeLookingWayWithDelay = g_config.pipboyCloseWhenLookAway && _pipboyStatus
				&& !g_frik.isPipboyConfigurationModeActive()
				&& timeElapsed > g_config.pipBoyOffDelay;
			const bool closeLookingWayWithMovement = g_config.pipboyCloseWhenMovingWhileLookingAway && _pipboyStatus
				&& !g_frik.isPipboyConfigurationModeActive()
				&& (fNotEqual(movingStick.x, 0, 0.3f) || fNotEqual(movingStick.y, 0, 0.3f)
					|| fNotEqual(lookingStick.x, 0, 0.3f) || fNotEqual(lookingStick.y, 0, 0.3f));

			if (closeLookingWayWithDelay || closeLookingWayWithMovement) {
				_pipboyStatus = false;
				turnPipBoyOff();
				f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
				if (_isWeaponinHand) {
					_weaponStateDetected = false;
				}
				disablePipboyHandPose();
				_isOperatingPipboy = false;
			}
			return;
		}

		if (_pipboyStatus) {
			_lastLookingAtPip = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
		} else if (g_config.pipboyOpenWhenLookAt) {
			if (_startedLookingAtPip == 0) {
				_startedLookingAtPip = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
			} else {
				const auto timeElapsed = static_cast<int>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - _startedLookingAtPip);
				if (timeElapsed > g_config.pipBoyOnDelay) {
					_pipboyStatus = true;
					f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
					turnPipBoyOn();
					setPipboyHandPose();
					_isOperatingPipboy = true;
					_startedLookingAtPip = 0;
				}
			}
		}

		//Why not enable both? So I commented out....

		//if (g_config.pipBoyButtonMode) // If g_config.pipBoyButtonMode, don't check touch
		//return;

		//Virtual Power Button Code
		static BSFixedString pwrButtonTrans("PowerTranslate");
		g_config.leftHandedPipBoy
			? pipboy = f4vr::getNode("PowerDetect", _skelly->getRightArm().shoulder->GetAsNiNode())
			: pipboy = f4vr::getNode("PowerDetect", _skelly->getLeftArm().shoulder->GetAsNiNode());
		g_config.leftHandedPipBoy
			? pipboyTrans = _skelly->getRightArm().forearm3->GetObjectByName(&pwrButtonTrans)
			: pipboyTrans = _skelly->getLeftArm().forearm3->GetObjectByName(&pwrButtonTrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		float distance = vec3Len(finger - pipboy->m_worldTransform.translate);
		if (distance > 2.0) {
			_pipTimer = 0;
			_stickypip = false;
			pipboyTrans->m_localTransform.translate.z = 0.0;
		} else {
			if (_pipTimer < 2) {
				_stickypip = false;
				_pipTimer++;
			} else {
				const float fz = 0 - (2.0f - distance);
				if (fz >= -0.14 && fz <= 0.0) {
					pipboyTrans->m_localTransform.translate.z = fz;
				}
				if (pipboyTrans->m_localTransform.translate.z < -0.10 && !_stickypip) {
					_stickypip = true;
					triggerHeptic();
					if (_pipboyStatus) {
						_pipboyStatus = false;
						turnPipBoyOff();
						g_frik.closePipboyConfigurationModeActive();
						f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 0.0;
					} else {
						_pipboyStatus = true;
						f4vr::getPlayerNodes()->PipboyRoot_nif_only_node->m_localTransform.scale = 1.0;
						turnPipBoyOn();
					}
				}
			}
		}
		//Virtual Light Button Code
		static BSFixedString lhtButtontrans("LightTranslate");
		g_config.leftHandedPipBoy
			? pipboy = f4vr::getNode("LightDetect", _skelly->getRightArm().shoulder->GetAsNiNode())
			: pipboy = f4vr::getNode("LightDetect", _skelly->getLeftArm().shoulder->GetAsNiNode());
		g_config.leftHandedPipBoy
			? pipboyTrans = _skelly->getRightArm().forearm3->GetObjectByName(&lhtButtontrans)
			: pipboyTrans = _skelly->getLeftArm().forearm3->GetObjectByName(&lhtButtontrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		distance = vec3Len(finger - pipboy->m_worldTransform.translate);
		if (distance > 2.0) {
			stickyPBlight = false;
			pipboyTrans->m_localTransform.translate.z = 0.0;
		} else if (distance <= 2.0) {
			const float fz = 0 - (2.0f - distance);
			if (fz >= -0.2 && fz <= 0.0) {
				pipboyTrans->m_localTransform.translate.z = fz;
			}
			if (pipboyTrans->m_localTransform.translate.z < -0.14 && !stickyPBlight) {
				stickyPBlight = true;
				triggerHeptic();
				if (!_pipboyStatus) {
					f4vr::togglePipboyLight(*g_player);
				}
			}
		}
		//Virtual Radio Button Code
		static BSFixedString radioButtontrans("RadioTranslate");
		g_config.leftHandedPipBoy
			? pipboy = f4vr::getNode("RadioDetect", _skelly->getRightArm().shoulder->GetAsNiNode())
			: pipboy = f4vr::getNode("RadioDetect", _skelly->getLeftArm().shoulder->GetAsNiNode());
		g_config.leftHandedPipBoy
			? pipboyTrans = _skelly->getRightArm().forearm3->GetObjectByName(&radioButtontrans)
			: pipboyTrans = _skelly->getLeftArm().forearm3->GetObjectByName(&radioButtontrans);
		if (!pipboyTrans || !pipboy) {
			return;
		}
		distance = vec3Len(finger - pipboy->m_worldTransform.translate);
		if (distance > 2.0) {
			stickyPBRadio = false;
			pipboyTrans->m_localTransform.translate.y = 0.0;
		} else if (distance <= 2.0) {
			const float fz = 0 - (2.0f - distance);
			if (fz >= -0.15 && fz <= 0.0) {
				pipboyTrans->m_localTransform.translate.y = fz;
			}
			if (pipboyTrans->m_localTransform.translate.y < -0.12 && !stickyPBRadio) {
				stickyPBRadio = true;
				triggerHeptic();
				if (!_pipboyStatus) {
					if (f4vr::isPlayerRadioEnabled()) {
						turnPlayerRadioOn(false);
					} else {
						turnPlayerRadioOn(true);
					}
				}
			}
		}
	}

	/**
	 * Execute operation on the currently active Pipboy UI.
	 * First thing is to detect is a message box is visible (context menu options) as message box and main lists are active at the
	 * same time. So, if the message box is visible we will only operate on the message box list and not the main list.
	 * @param root - Pipboy Scaleform UI root
	 * @param triggerPressed - true if operate as trigger is pressed regardless of controller input (used for "skeleton control")
	 */
	void Pipboy::handlePrimaryControllerOperation(GFxMovieRoot* root, bool triggerPressed) {
		// page level navigation
		const bool gripPressHeldDown = isPrimaryGripPressHeldDown();
		if (!gripPressHeldDown && isAButtonPressed()) {
			gotoPrevPage(root);
			return;
		}
		if (!gripPressHeldDown && isBButtonPressed()) {
			gotoNextPage(root);
			return;
		}

		// include actual controller check
		triggerPressed = triggerPressed || isPrimaryTriggerPressed();

		// Context menu message box handling
		if (triggerPressed && isMessageHolderVisible(root)) {
			triggerHeptic();
			f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.MessageHolder_mc", f4vr::ScaleformListOp::Select);
			f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc", f4vr::ScaleformListOp::Select);
			// prevent affecting the main list if message box is visible
			return;
		}

		// Specific handling by current page
		switch (f4vr::getScaleformInt(root, "root.Menu_mc.DataObj._CurrentPage").value_or(-1)) {
		case 0:
			handlePrimaryControllerOperationOnStatusPage(root, triggerPressed);
			break;
		case 1:
			handlePrimaryControllerOperationOnInventoryPage(root, triggerPressed);
			break;
		case 2:
			handlePrimaryControllerOperationOnDataPage(root, triggerPressed);
			break;
		case 3:
			handlePrimaryControllerOperationOnMapPage(root, triggerPressed);
			break;
		case 4:
			handlePrimaryControllerOperationOnRadioPage(root, triggerPressed);
			break;
		default: ;
		}
	}

	void Pipboy::gotoPrevPage(GFxMovieRoot* root) {
		root->Invoke("root.Menu_mc.gotoPrevPage", nullptr, nullptr, 0);
	}

	void Pipboy::gotoNextPage(GFxMovieRoot* root) {
		root->Invoke("root.Menu_mc.gotoNextPage", nullptr, nullptr, 0);
	}

	void Pipboy::gotoPrevTab(GFxMovieRoot* root) {
		triggerHeptic();
		root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0);
	}

	void Pipboy::gotoNextTab(GFxMovieRoot* root) {
		triggerHeptic();
		root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0);
	}

	/**
	 * Execute moving selection on the currently active list up or down.
	 * Whatever the list found to exist it will be moved, mostly we can "try" to move any of the lists and only the one that exists will be moved.
	 * On DATA page all 3 tabs can exist at the same time, so we need to check which one is visible to prevent working on a hidden one.
	 * First thing is to detect is a message box is visible (context menu options) as message box and main lists are active at the
	 * same time. So, if the message box is visible we will only operate on the message box list and not the main list.
	 */
	void Pipboy::moveListSelectionUpDown(GFxMovieRoot* root, const bool moveUp) {
		triggerHeptic();

		const auto listOp = moveUp ? f4vr::ScaleformListOp::MoveUp : f4vr::ScaleformListOp::MoveDown;

		if (isMessageHolderVisible(root)) {
			f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.MessageHolder_mc", listOp);
			f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc", listOp);
			// prevent affecting the main list if message box is visible
			return;
		}

		// Inventory, Radio, Special, and Perks tabs
		f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", listOp);
		f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc", listOp);
		f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.PerksTab_mc.List_mc", listOp);

		// Quest, Workshop, and Stats tabs exist at the same time, need to check which one is visible
		if (isQuestTabVisibleOnDataPage(root)) {
			// Quests tab has 2 lists for the main quests and quest objectives
			const char* listPath = isQuestTabObjectiveListEnabledOnDataPage(root)
				? "root.Menu_mc.CurrentPage.QuestsTab_mc.ObjectivesList_mc"
				: "root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc";
			f4vr::doOperationOnScaleformList(root, listPath, listOp);
		} else if (isWorkshopsTabVisibleOnDataPage(root)) {
			f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc.List_mc", listOp);
		} else {
			f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.StatsTab_mc.CategoryList_mc", listOp);
		}
	}

	void Pipboy::handlePrimaryControllerOperationOnStatusPage(GFxMovieRoot* root, const bool triggerPressed) {
		if (triggerPressed) {
			triggerHeptic();
			f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc", f4vr::ScaleformListOp::Select);
			f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.PerksTab_mc.List_mc", f4vr::ScaleformListOp::Select);
		}
	}

	void Pipboy::handlePrimaryControllerOperationOnInventoryPage(GFxMovieRoot* root, const bool triggerPressed) {
		if (triggerPressed) {
			triggerHeptic();
			f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", f4vr::ScaleformListOp::Select);
		}
	}

	/**
	 * On DATA page all 3 tabs can exist at the same time, so we need to check which one is visible to prevent working on a hidden one.
	 */
	void Pipboy::handlePrimaryControllerOperationOnDataPage(GFxMovieRoot* root, const bool triggerPressed) {
		if (triggerPressed) {
			triggerHeptic();
			if (isQuestTabVisibleOnDataPage(root)) {
				f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc", f4vr::ScaleformListOp::Select);
			} else {
				// open quest in map
				f4vr::invokeScaleformProcessUserEvent(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc", "XButton");
			}
		}
	}

	/**
	 * For handling map faster travel or marker setting we need to know if fast travel can be done using "bCanFastTravel"
	 * and then let the Pipboy code handle the rest by sending the right event to the currently visible map (world/local).
	 */
	void Pipboy::handlePrimaryControllerOperationOnMapPage(GFxMovieRoot* root, const bool triggerPressed) {
		if (isPrimaryGripPressHeldDown()) {
			if (triggerPressed) {
				// switch world/local maps
				triggerHeptic();
				f4vr::invokeScaleformProcessUserEvent(root, "root.Menu_mc.CurrentPage", "XButton");
			} else {
				// zoom map
				const auto [_, primAxisY] = f4vr::VRControllers.getAxisValue(f4vr::Hand::Primary);
				if (fNotEqual(primAxisY, 0, 0.5f)) {
					GFxValue args[1];
					args[0].SetNumber(primAxisY / 100.f);
					root->Invoke(getCurrentMapPath(root, ".ZoomMap").c_str(), nullptr, args, 1);
				}
			}
		} else if (triggerPressed) {
			triggerHeptic();

			// handle fast travel, custom marker
			const char* eventName = f4vr::getScaleformBool(root, getCurrentMapPath(root, ".bCanFastTravel").c_str()) ? "MapHolder:activate_marker" : "MapHolder:set_custom_marker";
			f4vr::invokeScaleformDispatchEvent(root, getCurrentMapPath(root), eventName);
		}
	}

	void Pipboy::handlePrimaryControllerOperationOnRadioPage(GFxMovieRoot* root, const bool triggerPressed) {
		if (triggerPressed) {
			triggerHeptic();
			f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", f4vr::ScaleformListOp::Select);
		}
	}

	/**
	 * Get Current Pipboy Tab and store it.
	 */
	void Pipboy::storeLastPipboyPage(const GFxMovieRoot* root) {
		GFxValue PBCurrentPage;
		if (root && root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage") && PBCurrentPage.GetType() != GFxValue::kType_Undefined) {
			_lastPipboyPage = PBCurrentPage.GetUInt();
		}
	}

	void Pipboy::pipboyManagement() {
		//Manages all aspects of Virtual Pipboy usage outside of turning the device / radio / torch on or off. Additionally, swaps left hand controls to the right hand.
		bool isInPA = f4vr::isInPowerArmor();
		if (!isInPA) {
			static BSFixedString orbNames[7] = {
				"TabChangeUpOrb", "TabChangeDownOrb", "PageChangeUpOrb", "PageChangeDownOrb", "ScrollItemsUpOrb", "ScrollItemsDownOrb", "SelectItemsOrb"
			};
			bool helmetHeadLamp = isArmorHasHeadLamp();
			bool lightOn = f4vr::isPipboyLightOn(*g_player);
			bool radioOn = f4vr::isPlayerRadioEnabled();
			Matrix44 rot;
			float radFreq = f4vr::getPlayerRadioFreq() - 23;
			static BSFixedString pwrButtonOn("PowerButton_mesh:2");
			static BSFixedString pwrButtonOff("PowerButton_mesh:off");
			static BSFixedString lhtButtonOn("LightButton_mesh:2");
			static BSFixedString lhtButtonOff("LightButton_mesh:off");
			static BSFixedString radButtonOn("RadioOn");
			static BSFixedString radButtonOff("RadioOff");
			static BSFixedString radioNeedle("RadioNeedle_mesh");
			static BSFixedString newModeKnob("ModeKnobDuplicate");
			static BSFixedString originalModeKnob("ModeKnob02");
			NiAVObject* pipbone = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&pwrButtonOn)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&pwrButtonOn);
			NiAVObject* pipbone2 = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&pwrButtonOff)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&pwrButtonOff);
			NiAVObject* pipbone3 = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&lhtButtonOn)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&lhtButtonOn);
			NiAVObject* pipbone4 = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&lhtButtonOff)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&lhtButtonOff);
			NiAVObject* pipbone5 = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&radButtonOn)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&radButtonOn);
			NiAVObject* pipbone6 = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&radButtonOff)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&radButtonOff);
			NiAVObject* pipbone7 = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&radioNeedle)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&radioNeedle);
			NiAVObject* pipbone8 = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&newModeKnob)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&newModeKnob);
			NiAVObject* pipbone9 = g_config.leftHandedPipBoy
				? _skelly->getRightArm().forearm3->GetObjectByName(&originalModeKnob)
				: _skelly->getLeftArm().forearm3->GetObjectByName(&originalModeKnob);
			if (!pipbone || !pipbone2 || !pipbone3 || !pipbone4 || !pipbone5 || !pipbone6 || !pipbone7 || !pipbone8) {
				return;
			}
			if (isLookingAtPipBoy()) {
				RE::NiPoint3 finger;
				NiAVObject* pipboy = nullptr;
				g_config.leftHandedPipBoy
					? finger = _skelly->getBoneWorldTransform("LArm_Finger23").translate
					: finger = _skelly->getBoneWorldTransform("RArm_Finger23").translate;
				g_config.leftHandedPipBoy
					? pipboy = f4vr::getNode("PipboyRoot", _skelly->getRightArm().shoulder->GetAsNiNode())
					: pipboy = f4vr::getNode("PipboyRoot", _skelly->getLeftArm().shoulder->GetAsNiNode());
				float distance;
				distance = vec3Len(finger - pipboy->m_worldTransform.translate);
				if (distance < g_config.pipboyDetectionRange && !_isOperatingPipboy && !_pipboyStatus) {
					// Hides Weapon and poses hand for pointing
					_isOperatingPipboy = true;
					_isWeaponinHand = (*g_player)->actorState.IsWeaponDrawn();
					setPipboyHandPose();
				}
				if (distance > g_config.pipboyDetectionRange && _isOperatingPipboy && !_pipboyStatus) {
					// Restores Weapon and releases hand pose
					_isOperatingPipboy = false;
					disablePipboyHandPose();
					if (_isWeaponinHand) {
						_weaponStateDetected = false;
					}
				}
			} else if (!isLookingAtPipBoy() && _isOperatingPipboy && !_pipboyStatus) {
				// Catches if you're not looking at the pipboy when your hand moves outside the control area and restores weapon / releases hand pose
				disablePipboyHandPose();
				for (int i = 0; i < 7; i++) {
					// Remove any stuck helper orbs if Pipboy times out for any reason.
					NiAVObject* orb = g_config.leftHandedPipBoy
						? _skelly->getRightArm().forearm3->GetObjectByName(&orbNames[i])
						: _skelly->getLeftArm().forearm3->GetObjectByName(&orbNames[i]);
					if (orb != nullptr) {
						orb->m_localTransform.scale = std::min<float>(orb->m_localTransform.scale, 0);
					}
				}
				if (_isWeaponinHand) {
					_weaponStateDetected = false;
				}
				_isOperatingPipboy = false;
			}
			if (_lastPipboyPage == 4) {
				// fixes broken 'Mode Knob' position when radio tab is selected
				rot.makeTransformMatrix(pipbone8->m_localTransform.rotate, RE::NiPoint3(0, 0, 0));
				float rotx;
				float roty;
				float rotz;
				rot.getEulerAngles(&rotx, &roty, &rotz);
				if (rotx < 0.57) {
					Matrix44 kRot;
					kRot.setEulerAngles(-0.05f, degreesToRads(0), degreesToRads(0));
					pipbone8->m_localTransform.rotate = kRot.multiply43Right(pipbone8->m_localTransform.rotate);
				}
			} else {
				// restores control of the 'Mode Knob' to the Pipboy behaviour file
				pipbone8->m_localTransform.rotate = pipbone9->m_localTransform.rotate;
			}
			// Controls Pipboy power light glow (on or off depending on Pipboy state)
			_pipboyStatus ? pipbone->flags &= 0xfffffffffffffffe : pipbone2->flags &= 0xfffffffffffffffe;
			_pipboyStatus ? pipbone->m_localTransform.scale = 1 : pipbone2->m_localTransform.scale = 1;
			_pipboyStatus ? pipbone2->flags |= 0x1 : pipbone->flags |= 0x1;
			_pipboyStatus ? pipbone2->m_localTransform.scale = 0 : pipbone->m_localTransform.scale = 0;
			// Control switching between hand and head based Pipboy light
			if (lightOn && !helmetHeadLamp) {
				NiNode* head = f4vr::getNode("Head", (*g_player)->GetActorRootNode(false)->GetAsNiNode());
				if (!head) {
					return;
				}
				const bool useRightHand = g_config.leftHandedPipBoy || g_config.isPipBoyTorchRightArmMode;
				const auto hand = _skelly->getBoneWorldTransform(useRightHand ? "RArm_Hand" : "LArm_Hand").translate;
				float distance = vec3Len(hand - head->m_worldTransform.translate);
				if (distance < 15.0) {
					uint64_t pipboyHand = f4vr::VRControllers.getControllerState_DEPRECATED(useRightHand ? f4vr::TrackerType::Right : f4vr::TrackerType::Left).ulButtonPressed;
					const auto SwitchLightButton = pipboyHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(g_config.switchTorchButton));
					f4vr::VRControllers.triggerHaptic(useRightHand ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand, 0.1f, 0.1f);
					// Control switching between hand and head based Pipboy light
					if (SwitchLightButton && !_SwithLightButtonSticky) {
						_SwithLightButtonSticky = true;
						_SwitchLightHaptics = false;
						f4vr::VRControllers.triggerHaptic(useRightHand ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand, 0.1f);
						NiNode* LGHT_ATTACH = useRightHand
							? f4vr::getNode("RArm_Hand", _skelly->getRightArm().shoulder->GetAsNiNode())
							: f4vr::getNode("LArm_Hand", _skelly->getLeftArm().shoulder->GetAsNiNode());
						NiNode* lght = g_config.isPipBoyTorchOnArm
							? f4vr::get1StChildNode("HeadLightParent", LGHT_ATTACH)
							: f4vr::getPlayerNodes()->HeadLightParentNode->GetAsNiNode();
						if (lght) {
							BSFixedString parentnode = g_config.isPipBoyTorchOnArm ? lght->m_parent->m_name : f4vr::getPlayerNodes()->HeadLightParentNode->m_parent->m_name;
							Matrix44 lightRot;
							float rotz = g_config.isPipBoyTorchOnArm ? -90 : 90;
							lightRot.setEulerAngles(degreesToRads(0), degreesToRads(0), degreesToRads(rotz));
							lght->m_localTransform.rotate = lightRot.multiply43Right(lght->m_localTransform.rotate);
							lght->m_localTransform.translate.y = g_config.isPipBoyTorchOnArm ? 0 : 4;
							g_config.isPipBoyTorchOnArm ? lght->m_parent->RemoveChild(lght) : f4vr::getPlayerNodes()->HeadLightParentNode->m_parent->RemoveChild(lght);
							g_config.isPipBoyTorchOnArm ? f4vr::getPlayerNodes()->HmdNode->AttachChild(lght, true) : LGHT_ATTACH->AttachChild(lght, true);
							g_config.togglePipBoyTorchOnArm();
						}
					}
					if (!SwitchLightButton) {
						_SwithLightButtonSticky = false;
					}
				} else if (distance > 10) {
					_SwitchLightHaptics = true;
					_SwithLightButtonSticky = false;
				}
			}
			//Attach light to hand
			if (g_config.isPipBoyTorchOnArm) {
				NiNode* LGHT_ATTACH = g_config.leftHandedPipBoy || g_config.isPipBoyTorchRightArmMode
					? f4vr::getNode("RArm_Hand", _skelly->getRightArm().shoulder->GetAsNiNode())
					: f4vr::getNode("LArm_Hand", _skelly->getLeftArm().shoulder->GetAsNiNode());
				if (LGHT_ATTACH) {
					if (lightOn && !helmetHeadLamp) {
						NiNode* lght = f4vr::getPlayerNodes()->HeadLightParentNode->GetAsNiNode();
						BSFixedString parentnode = f4vr::getPlayerNodes()->HeadLightParentNode->m_parent->m_name;
						if (parentnode == "HMDNode") {
							f4vr::getPlayerNodes()->HeadLightParentNode->m_parent->RemoveChild(lght);
							Matrix44 LightRot;
							LightRot.setEulerAngles(degreesToRads(0), degreesToRads(0), degreesToRads(90));
							lght->m_localTransform.rotate = LightRot.multiply43Right(lght->m_localTransform.rotate);
							lght->m_localTransform.translate.y = 4;
							LGHT_ATTACH->AttachChild(lght, true);
						}
					}
					//Restore HeadLight to correct node when light is powered off (to avoid any crashes)
					else if (!lightOn || helmetHeadLamp) {
						if (auto lght = f4vr::get1StChildNode("HeadLightParent", LGHT_ATTACH)) {
							BSFixedString parentnode = lght->m_parent->m_name;
							if (parentnode != "HMDNode") {
								Matrix44 LightRot;
								LightRot.setEulerAngles(degreesToRads(0), degreesToRads(0), degreesToRads(-90));
								lght->m_localTransform.rotate = LightRot.multiply43Right(lght->m_localTransform.rotate);
								lght->m_localTransform.translate.y = 0;
								lght->m_parent->RemoveChild(lght);
								f4vr::getPlayerNodes()->HmdNode->AttachChild(lght, true);
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
			radioOn ? pipbone5->flags &= 0xfffffffffffffffe : pipbone6->flags &= 0xfffffffffffffffe;
			radioOn ? pipbone5->m_localTransform.scale = 1 : pipbone6->m_localTransform.scale = 1;
			radioOn ? pipbone6->flags |= 0x1 : pipbone5->flags |= 0x1;
			radioOn ? pipbone6->m_localTransform.scale = 0 : pipbone5->m_localTransform.scale = 0;
			// Controls Radio Needle Position.
			if (radioOn && radFreq != lastRadioFreq) {
				float x = -1 * (radFreq - lastRadioFreq);
				rot.setEulerAngles(degreesToRads(0), degreesToRads(x), degreesToRads(0));
				pipbone7->m_localTransform.rotate = rot.multiply43Right(pipbone7->m_localTransform.rotate);
				lastRadioFreq = radFreq;
			} else if (!radioOn && lastRadioFreq > 0) {
				float x = lastRadioFreq;
				rot.setEulerAngles(degreesToRads(0), degreesToRads(x), degreesToRads(0));
				pipbone7->m_localTransform.rotate = rot.multiply43Right(pipbone7->m_localTransform.rotate);
				lastRadioFreq = 0.0;
			}
			// Scale-form code for managing Pipboy menu controls (Virtual and Physical)
			if (_pipboyStatus) {
				BSFixedString pipboyMenu("PipboyMenu");
				IMenu* menu = (*g_ui)->GetMenu(pipboyMenu);
				if (menu != nullptr) {
					GFxMovieRoot* root = menu->movie->movieRoot;
					if (root != nullptr) {
						storeLastPipboyPage(root);
						static BSFixedString boneNames[7] = {
							"TabChangeUp", "TabChangeDown", "PageChangeUp", "PageChangeDown", "ScrollItemsUp", "ScrollItemsDown", "SelectButton02"
						};
						static BSFixedString transNames[7] = {
							"TabChangeUpTrans", "TabChangeDownTrans", "PageChangeUpTrans", "PageChangeDownTrans", "ScrollItemsUpTrans", "ScrollItemsDownTrans", "SelectButtonTrans"
						};
						float boneDistance[7] = {2.0f, 2.0f, 2.0f, 2.0f, 1.5f, 1.5f, 2.0f};
						float transDistance[7] = {0.6f, 0.6f, 0.6f, 0.6f, 0.1f, 0.1f, 0.4f};
						float maxDistance[7] = {1.2f, 1.2f, 1.2f, 1.2f, 1.2f, 1.2f, 0.6f};
						RE::NiPoint3 finger;
						// Virtual Controls Code Starts here:
						g_config.leftHandedPipBoy
							? finger = _skelly->getBoneWorldTransform("LArm_Finger23").translate
							: finger = _skelly->getBoneWorldTransform("RArm_Finger23").translate;
						for (int i = 0; i < 7; i++) {
							NiAVObject* bone = g_config.leftHandedPipBoy
								? _skelly->getRightArm().forearm3->GetObjectByName(&boneNames[i])
								: _skelly->getLeftArm().forearm3->GetObjectByName(&boneNames[i]);
							NiNode* trans = g_config.leftHandedPipBoy
								? _skelly->getRightArm().forearm3->GetObjectByName(&transNames[i])->GetAsNiNode()
								: _skelly->getLeftArm().forearm3->GetObjectByName(&transNames[i])->GetAsNiNode();
							if (bone && trans) {
								float distance = vec3Len(finger - bone->m_worldTransform.translate);
								if (distance > boneDistance[i]) {
									trans->m_localTransform.translate.z = 0.0;
									_PBControlsSticky[i] = false;
									NiAVObject* orb = g_config.leftHandedPipBoy
										? _skelly->getRightArm().forearm3->GetObjectByName(&orbNames[i])
										: _skelly->getLeftArm().forearm3->GetObjectByName(&orbNames[i]); //Hide helper Orbs when not near a control surface
									if (orb != nullptr) {
										orb->m_localTransform.scale = std::min<float>(orb->m_localTransform.scale, 0);
									}
								} else if (distance <= boneDistance[i]) {
									float fz = boneDistance[i] - distance;
									NiAVObject* orb = g_config.leftHandedPipBoy
										? _skelly->getRightArm().forearm3->GetObjectByName(&orbNames[i])
										: _skelly->getLeftArm().forearm3->GetObjectByName(&orbNames[i]); //Show helper Orbs when not near a control surface
									if (orb != nullptr) {
										orb->m_localTransform.scale = std::max<float>(orb->m_localTransform.scale, 1);
									}
									if (fz > 0.0 && fz < maxDistance[i]) {
										trans->m_localTransform.translate.z = fz;
										if (i == 4) {
											// Move Scroll Knob Anti-Clockwise when near control surface
											static BSFixedString KnobNode = "ScrollItemsKnobRot";
											NiAVObject* ScrollKnob = g_config.leftHandedPipBoy
												? _skelly->getRightArm().forearm3->GetObjectByName(&KnobNode)
												: _skelly->getLeftArm().forearm3->GetObjectByName(&KnobNode);
											Matrix44 rot;
											rot.setEulerAngles(degreesToRads(0), degreesToRads(fz), degreesToRads(0));
											ScrollKnob->m_localTransform.rotate = rot.multiply43Right(ScrollKnob->m_localTransform.rotate);
										}
										if (i == 5) {
											// Move Scroll Knob Clockwise when near control surface
											float roty = fz * -1;
											static BSFixedString KnobNode = "ScrollItemsKnobRot";
											NiAVObject* ScrollKnob = g_config.leftHandedPipBoy
												? _skelly->getRightArm().forearm3->GetObjectByName(&KnobNode)
												: _skelly->getLeftArm().forearm3->GetObjectByName(&KnobNode);
											Matrix44 rot;
											rot.setEulerAngles(degreesToRads(0), degreesToRads(roty), degreesToRads(0));
											ScrollKnob->m_localTransform.rotate = rot.multiply43Right(ScrollKnob->m_localTransform.rotate);
										}
									}
									if (trans->m_localTransform.translate.z > transDistance[i] && !_PBControlsSticky[i]) {
										_PBControlsSticky[i] = true;
										triggerHeptic();
										if (i == 0) {
											gotoPrevPage(root);
										}
										if (i == 1) {
											gotoNextPage(root);
										}
										if (i == 2) {
											gotoPrevTab(root);
										}
										if (i == 3) {
											gotoNextTab(root);
										}
										if (i == 4) {
											moveListSelectionUpDown(root, true);
										}
										if (i == 5) {
											moveListSelectionUpDown(root, false);
										}
										if (i == 6) {
											handlePrimaryControllerOperation(root, true);
										}
									}
								}
							}
						}
						// Mirror Left Stick Controls on Right Stick.
						if (!g_frik.isPipboyConfigurationModeActive() && g_config.enablePrimaryControllerPipboyUse) {
							BSFixedString selectnodename = "SelectRotate";
							NiNode* trans = g_config.leftHandedPipBoy
								? _skelly->getRightArm().forearm3->GetObjectByName(&selectnodename)->GetAsNiNode()
								: _skelly->getLeftArm().forearm3->GetObjectByName(&selectnodename)->GetAsNiNode();
							vr::VRControllerAxis_t doinantHandStick = f4vr::isLeftHandedMode()
								? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0]
								: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0];
							vr::VRControllerAxis_t doinantTrigger = f4vr::isLeftHandedMode()
								? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[1]
								: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[1];
							vr::VRControllerAxis_t secondaryTrigger = f4vr::isLeftHandedMode()
								? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[1]
								: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[1];
							uint64_t dominantHand = f4vr::isLeftHandedMode()
								? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).ulButtonPressed
								: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).ulButtonPressed;
							const auto UIAltSelectButton = dominantHand & vr::ButtonMaskFromId(static_cast<vr::EVRButtonId>(32)); // Right Touchpad
							bool isPBMessageBoxVisible = false;
							// Move Pipboy trigger mesh with controller trigger position.
							if (trans != nullptr) {
								if (doinantTrigger.x > 0.00 && secondaryTrigger.x == 0.0) {
									trans->m_localTransform.translate.z = doinantTrigger.x / 3 * -1;
								} else if (secondaryTrigger.x > 0.00 && doinantTrigger.x == 0.0) {
									trans->m_localTransform.translate.z = secondaryTrigger.x / 3 * -1;
								} else {
									trans->m_localTransform.translate.z = 0.00;
								}
							}
							isPBMessageBoxVisible = isMessageHolderVisible(root);
							if (_lastPipboyPage != 3 || isPBMessageBoxVisible) {
								static BSFixedString KnobNode = "ScrollItemsKnobRot";
								NiAVObject* ScrollKnob = g_config.leftHandedPipBoy
									? _skelly->getRightArm().forearm3->GetObjectByName(&KnobNode)
									: _skelly->getLeftArm().forearm3->GetObjectByName(&KnobNode);
								Matrix44 rot;
								if (doinantHandStick.y > 0.85) {
									rot.setEulerAngles(degreesToRads(0), degreesToRads(0.4f), degreesToRads(0));
									ScrollKnob->m_localTransform.rotate = rot.multiply43Right(ScrollKnob->m_localTransform.rotate);
								}
								if (doinantHandStick.y < -0.85) {
									rot.setEulerAngles(degreesToRads(0), degreesToRads(-0.4f), degreesToRads(0));
									ScrollKnob->m_localTransform.rotate = rot.multiply43Right(ScrollKnob->m_localTransform.rotate);
								}
							}
							if (!isPrimaryGripPressHeldDown()) {
								if (_lastPipboyPage == 3 && !isPBMessageBoxVisible) {
									// Map Tab
									GFxValue akArgs[2];
									akArgs[0].SetNumber(doinantHandStick.x * -1);
									akArgs[1].SetNumber(doinantHandStick.y);
									if (root->Invoke("root.Menu_mc.CurrentPage.WorldMapHolder_mc.PanMap", nullptr, akArgs, 2)) {} // Move Map
									if (root->Invoke("root.Menu_mc.CurrentPage.LocalMapHolder_mc.PanMap", nullptr, akArgs, 2)) {}
								} else {
									if (doinantHandStick.y > 0.85) {
										if (!_controlSleepStickyY) {
											_controlSleepStickyY = true;
											moveListSelectionUpDown(root, true);
											std::thread t2(&Pipboy::rightStickYSleep, this, 155);
											t2.detach();
										}
									}
									if (doinantHandStick.y < -0.85) {
										if (!_controlSleepStickyY) {
											_controlSleepStickyY = true;
											moveListSelectionUpDown(root, false);
											std::thread t2(&Pipboy::rightStickYSleep, this, 155);
											t2.detach();
										}
									}
									if (doinantHandStick.x < -0.85) {
										if (!_controlSleepStickyX) {
											_controlSleepStickyX = true;
											gotoPrevTab(root);
											std::thread t3(&Pipboy::rightStickXSleep, this, 170);
											t3.detach();
										}
									}
									if (doinantHandStick.x > 0.85) {
										if (!_controlSleepStickyX) {
											_controlSleepStickyX = true;
											gotoNextTab(root);
											std::thread t3(&Pipboy::rightStickXSleep, this, 170);
											t3.detach();
										}
									}
								}
							}

							handlePrimaryControllerOperation(root, false);

							if (UIAltSelectButton && !_UIAltSelectSticky) {
								_UIAltSelectSticky = true;
								if (root->Invoke("root.Menu_mc.CurrentPage.onMessageButtonPress()", nullptr, nullptr, 0)) {}
							} else if (!UIAltSelectButton) {
								_UIAltSelectSticky = false;
							}
						} else if (!g_frik.isPipboyConfigurationModeActive() && !g_config.enablePrimaryControllerPipboyUse) {
							//still move Pipboy trigger mesh even if controls havent been swapped.
							vr::VRControllerAxis_t secondaryTrigger = f4vr::isLeftHandedMode()
								? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[1]
								: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[1];
							vr::VRControllerAxis_t offHandStick = f4vr::isLeftHandedMode()
								? f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Right).rAxis[0]
								: f4vr::VRControllers.getControllerState_DEPRECATED(f4vr::TrackerType::Left).rAxis[0];
							BSFixedString selectnodename = "SelectRotate";
							NiNode* trans = g_config.leftHandedPipBoy
								? _skelly->getRightArm().forearm3->GetObjectByName(&selectnodename)->GetAsNiNode()
								: _skelly->getLeftArm().forearm3->GetObjectByName(&selectnodename)->GetAsNiNode();
							if (trans != nullptr) {
								if (secondaryTrigger.x > 0.00) {
									trans->m_localTransform.translate.z = secondaryTrigger.x / 3 * -1;
								} else {
									trans->m_localTransform.translate.z = 0.00;
								}
							}
							//still move Pipboy scroll knob even if controls haven't been swapped.
							const bool isPBMessageBoxVisible = isMessageHolderVisible(root);
							if (_lastPipboyPage != 3 || isPBMessageBoxVisible) {
								static BSFixedString KnobNode = "ScrollItemsKnobRot";
								NiAVObject* ScrollKnob = g_config.leftHandedPipBoy
									? _skelly->getRightArm().forearm3->GetObjectByName(&KnobNode)
									: _skelly->getLeftArm().forearm3->GetObjectByName(&KnobNode);
								Matrix44 rot;
								if (offHandStick.x > 0.85) {
									rot.setEulerAngles(degreesToRads(0), degreesToRads(0.4), degreesToRads(0));
									ScrollKnob->m_localTransform.rotate = rot.multiply43Right(ScrollKnob->m_localTransform.rotate);
								}
								if (offHandStick.x < -0.85) {
									rot.setEulerAngles(degreesToRads(0), degreesToRads(-0.4), degreesToRads(0));
									ScrollKnob->m_localTransform.rotate = rot.multiply43Right(ScrollKnob->m_localTransform.rotate);
								}
							}
						}
					}
				}
			}
		} else if (isInPA) {
			lastRadioFreq = 0.0; // Ensures Radio needle doesn't get messed up when entering and then exiting Power Armor.
			// Continue to update Pipboy page info when in Power Armor.
			BSFixedString pipboyMenu("PipboyMenu");
			IMenu* menu = (*g_ui)->GetMenu(pipboyMenu);
			if (menu != nullptr) {
				GFxMovieRoot* root = menu->movie->movieRoot;
				storeLastPipboyPage(root);
			}
		}
	}

	void Pipboy::dampenPipboyScreen() {
		if (!g_config.dampenPipboyScreen) {
			return;
		}
		if (!_pipboyStatus) {
			_pipboyScreenPrevFrame = f4vr::getPlayerNodes()->ScreenNode->m_worldTransform;
			return;
		}
		NiNode* pipboyScreen = f4vr::getPlayerNodes()->ScreenNode;

		if (pipboyScreen && _pipboyStatus) {
			Quaternion rq, rt;
			// do a spherical interpolation between previous frame and current frame for the world rotation matrix
			const RE::NiTransform prevFrame = _pipboyScreenPrevFrame;
			rq.fromRot(prevFrame.rotate);
			rt.fromRot(pipboyScreen->m_worldTransform.rotate);
			rq.slerp(1 - g_config.dampenPipboyRotation, rt);
			pipboyScreen->m_worldTransform.rotate = rq.getRot().make43();
			// do a linear interpolation between the position from the previous frame to current frame
			RE::NiPoint3 deltaPos = pipboyScreen->m_worldTransform.translate - prevFrame.translate;
			deltaPos *= g_config.dampenPipboyTranslation; // just use hands dampening value for now
			pipboyScreen->m_worldTransform.translate -= deltaPos;
			_pipboyScreenPrevFrame = pipboyScreen->m_worldTransform;
			f4vr::updateDown(pipboyScreen->GetAsNiNode(), false);
		}
	}

	bool Pipboy::isLookingAtPipBoy() const {
		const BSFixedString wandPipName("PipboyRoot_NIF_ONLY");
		NiAVObject* pipboy = f4vr::getPlayerNodes()->SecondaryWandNode->GetObjectByName(&wandPipName);

		if (pipboy == nullptr) {
			return false;
		}

		const BSFixedString screenName("Screen:0");
		const NiAVObject* screen = pipboy->GetObjectByName(&screenName);
		if (screen == nullptr) {
			return false;
		}

		const float threshhold = _pipboyStatus ? g_config.pipboyLookAwayThreshold : g_config.pipboyLookAtThreshold;
		return isCameraLookingAtObject((*g_playerCamera)->cameraNode, screen, threshhold);
	}

	/**
	 * Prevents continuous Input from Right Stick X Axis
	 */
	void Pipboy::rightStickXSleep(const int time) {
		Sleep(time);
		_controlSleepStickyX = false;
	}

	/**
	 * Prevents continuous Input from Right Stick Y Axis
	 */
	void Pipboy::rightStickYSleep(const int time) {
		Sleep(time);
		_controlSleepStickyY = false;
	}

	/**
	 * Used to determine if secondary trigger received a long or short press
	 */
	void Pipboy::secondaryTriggerSleep(const int time) {
		Sleep(time);
		_controlSleepStickyT = false;
	}
}
