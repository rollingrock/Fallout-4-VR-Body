#include "FRIK.h"

#include "Config.h"
#include "ConfigurationMode.h"
#include "Debug.h"
#include "HandPose.h"
#include "PapyrusApi.h"
#include "Pipboy.h"
#include "Skeleton.h"
#include "SmoothMovementVR.h"
#include "utils.h"
#include "common/Logger.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/VRControllersManager.h"
#include "ui/UIManager.h"
#include "ui/UIModAdapter.h"

using namespace common;

namespace frik {
	/**
	 * TODO: think about it, is it the best way to handle this dependency indirection.
	 */
	class FrameUpdateContext : public vrui::UIModAdapter {
	public:
		explicit FrameUpdateContext(Skeleton* skelly)
			: _skelly(skelly) {}

		virtual NiPoint3 getInteractionBoneWorldPosition() override {
			return _skelly->getOffhandIndexFingerTipWorldPosition();
		}

		virtual void fireInteractionHeptic() override {
			f4vr::VRControllers.triggerHaptic(f4vr::Hand::Offhand);
		}

		virtual void setInteractionHandPointing(const bool primaryHand, const bool toPoint) override {
			setForceHandPointingPose(primaryHand, toPoint);
		}

	private:
		Skeleton* _skelly;
	};

	/**
	 * On load of FRIK plugin by F4VRSE setup messaging and papyrus handling once.
	 */
	void FRIK::initialize(const F4SEInterface* f4se) {
		_pluginHandle = f4se->GetPluginHandle();
		if (_pluginHandle == kPluginHandle_Invalid) {
			throw std::exception("Invalid plugin handle");
		}

		_messaging = static_cast<F4SEMessagingInterface*>(f4se->QueryInterface(kInterface_Messaging));
		_messaging->RegisterListener(_pluginHandle, "F4SE", onF4VRSEMessage);

		Log::info("Register papyrus functions...");
		initPapyrusApis(f4se);
	}

	/**
	 * F4VRSE messages listener to handle game loaded, new game, and save loaded events.
	 */
	void FRIK::onF4VRSEMessage(F4SEMessagingInterface::Message* msg) {
		if (!msg) {
			return;
		}

		if (msg->type == F4SEMessagingInterface::kMessage_GameLoaded) {
			// One time event fired after all plugins are loaded and game is full in main menu
			Log::info("F4VRSE On Game Loaded Message...");
			g_frik.initOnGameLoaded();
		}

		if (msg->type == F4SEMessagingInterface::kMessage_PostLoadGame || msg->type == F4SEMessagingInterface::kMessage_NewGame) {
			// If a game is loaded or a new game started re-initialize FRIK for clean slate
			Log::info("F4VRSE On Post Load Message...");
			g_frik.initOnGameSessionLoaded();
		}
	}

	/**
	 * On game fully loaded initialize things that should be initialized only once.
	 */
	void FRIK::initOnGameLoaded() {
		Log::info("Initialize FRIK...");
		std::srand(static_cast<unsigned int>(time(nullptr)));

		Log::info("Init config...");
		g_config.loadAllConfig();

		vrui::initUIManager();

		_gameMenusHandler.init();

		if (isBetterScopesVRModLoaded()) {
			Log::info("BetterScopesVR mod detected, registering for messages...");
			_messaging->Dispatch(_pluginHandle, 15, static_cast<void*>(nullptr), sizeof(bool), BETTER_SCOPES_VR_MOD_NAME);
			_messaging->RegisterListener(_pluginHandle, BETTER_SCOPES_VR_MOD_NAME, onBetterScopesMessage);
		}

		configureGameVars();
	}

	/**
	 * Game session can be initialized multiple times as it is fired on new game and save loaded events.
	 * We should clear and reload as much of the game state as we can.
	 */
	void FRIK::initOnGameSessionLoaded() {
		if (_skelly) {
			Log::info("Resetting skeleton for new game session...");
			releaseSkeleton();
			Log::info("Reload config...");
			g_config.loadAllConfig();
		}
	}

	/**
	 * Called on every game frame via hooks into the game engine.
	 * This is where all the magic happens by updating game state and nodes.
	 */
	void FRIK::onFrameUpdate() {
		if (!g_player || !(*g_player)->unkF0) {
			// game not loaded or existing
			return;
		}

		f4vr::VRControllers.update(*f4vr::iniLeftHandedMode);

		if (_skelly && _inPowerArmor != f4vr::isInPowerArmor()) {
			Log::info("Power Armor State Changed, reset skelly...");
			releaseSkeleton();
		}

		if (!_skelly) {
			if (!isGameReadyForFrameUpdate()) {
				return;
			}
			initSkeleton();
		}

		Log::debug("Update Skeleton...");
		_skelly->onFrameUpdate();

		Log::debug("Update Bone Sphere...");
		_boneSpheres.onFrameUpdate();

		Log::debug("Update Pipboy...");
		_pipboy->onFrameUpdate();

		Log::debug("Update Weapon Position...");
		_weaponPosition->onFrameUpdate();

		f4vr::BSFadeNode_MergeWorldBounds((*g_player)->unkF0->rootNode->GetAsNiNode());
		f4vr::BSFlattenedBoneTree_UpdateBoneArray((*g_player)->unkF0->rootNode->m_children.m_data[0]);
		// just in case any transforms missed because they are not in the tree do a full flat bone array update
		f4vr::BSFadeNode_UpdateGeomArray((*g_player)->unkF0->rootNode, 1);

		_configurationMode->onFrameUpdate();

		FrameUpdateContext context(_skelly);
		vrui::g_uiManager->onFrameUpdate(&context);

		f4vr::updateDownFromRoot(); // Last world update before exit.    Probably not necessary.

		checkDebugDump();
	}

	void FRIK::initSkeleton() {
		_inPowerArmor = f4vr::isInPowerArmor();

		Log::info("Initialize Skeleton (%s) ; Nodes: Player=%p, Data=%p, Root=%p, Skeleton=%p, Common=%p",
			_inPowerArmor ? "PowerArmor" : "Regular", *g_player, (*g_player)->unkF0, (*g_player)->unkF0->rootNode, f4vr::getRootNode(), f4vr::getCommonNode());

		// init skeleton
		_skelly = new Skeleton(f4vr::getRootNode(), _inPowerArmor);

		// init handlers depending on skeleton
		_pipboy = new Pipboy(_skelly);
		_configurationMode = new ConfigurationMode(_skelly);
		_weaponPosition = new WeaponPositionAdjuster(_skelly);

		if (g_config.setScale) {
			// TODO: do we need to do it here every time?
			Log::info("scale set");
			Setting* set = GetINISetting("fVrScale:VR");
			set->SetDouble(g_config.fVrScale);
		}
	}

	/**
	 * Check if game all nodes exist and ready for frame update flow.
	 */
	bool FRIK::isGameReadyForFrameUpdate() {
		if (!g_player || !(*g_player)->unkF0) {
			Log::sample(3000, "Player global not set yet!");
			return false;
		}
		if (!(*g_player)->unkF0->rootNode || !f4vr::getRootNode()) {
			Log::info("Player root nodes not set yet!");
			return false;
		}
		if (!f4vr::getCommonNode() || !f4vr::getPlayerNodes()) {
			Log::info("Common or Player nodes not set yet!");
			return false;
		}
		if (!f4vr::getNode("RArm_Hand", f4vr::getFirstPersonSkeleton())) {
			Log::info("Arm node not set yet!");
			return false;
		}
		return true;
	}

	/**
	 * On switch from normal and power armor, reset the skelly and all dependencies with persistent data.
	 */
	void FRIK::releaseSkeleton() {
		delete _skelly;
		_skelly = nullptr;

		delete _pipboy;
		_pipboy = nullptr;

		delete _configurationMode;
		_configurationMode = nullptr;

		delete _weaponPosition;
		_weaponPosition = nullptr;

		_inPowerArmor = false;
		_dynamicCameraHeight = false;
	}

	void FRIK::configureGameVars() {
		f4vr::setIniSettingFloat("fPipboyMaxScale:VRPipboy", 3.0000);
		f4vr::setIniSettingFloat("fPipboyMinScale:VRPipboy", 0.0100f);
		f4vr::setIniSettingFloat("fVrPowerArmorScaleMultiplier:VR", 1.0000);
	}

	/**
	 * Send a message to the BetterScopesVR mod.
	 */
	void FRIK::dispatchMessageToBetterScopesVR(const UInt32 messageType, void* data, const UInt32 dataLen) const {
		_messaging->Dispatch(_pluginHandle, messageType, data, dataLen, BETTER_SCOPES_VR_MOD_NAME);
	}

	void FRIK::onBetterScopesMessage(F4SEMessagingInterface::Message* msg) {
		if (!msg) {
			return;
		}

		if (msg->type == 15) {
			Log::info("BetterScopesVR looking through scopes: %d", msg->dataLen);
			g_frik.setLookingThroughScope(static_cast<bool>(msg->data));
		}
	}

	void FRIK::checkDebugDump() {
		if (g_config.checkDebugDumpDataOnceFor("all_nodes")) {
			printAllNodes();
		}
		if (g_config.checkDebugDumpDataOnceFor("world")) {
			printNodes(f4vr::getPlayerNodes()->primaryWeaponScopeCamera->m_parent->m_parent->m_parent->m_parent->m_parent->m_parent);
		}
		if (g_config.checkDebugDumpDataOnceFor("fp_skelly")) {
			printNodes(f4vr::getFirstPersonSkeleton());
		}
		if (g_config.checkDebugDumpDataOnceFor("skelly")) {
			printNodes(f4vr::getRootNode()->m_parent);
		}
		if (g_config.checkDebugDumpDataOnceFor("menus")) {
			_gameMenusHandler.debugDumpAllMenus();
		}
	}
}
