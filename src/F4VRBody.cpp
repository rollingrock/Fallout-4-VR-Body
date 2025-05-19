#include "F4VRBody.h"

#include "Config.h"
#include "ConfigurationMode.h"
#include "Debug.h"
#include "HandPose.h"
#include "Menu.h"
#include "MenuChecker.h"
#include "Pipboy.h"
#include "Skeleton.h"
#include "SmoothMovementVR.h"
#include "utils.h"
#include "common/Logger.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/VRControllersManager.h"
#include "ui/UIManager.h"
#include "ui/UIModAdapter.h"

using namespace common;

bool firstTime = true;
bool printPlayerOnce = true;

PluginHandle g_pluginHandle = kPluginHandle_Invalid;
F4SEPapyrusInterface* g_papyrus = nullptr;
F4SEMessagingInterface* g_messaging = nullptr;

namespace frik {
	Pipboy* g_pipboy = nullptr;
	ConfigurationMode* g_configurationMode = nullptr;
	BoneSpheresHandler* g_boneSpheres = nullptr;
	WeaponPositionAdjuster* g_weaponPosition = nullptr;

	Skeleton* _skelly = nullptr;

	bool isLoaded = false;

	bool c_isLookingThroughScope = false;
	float c_dynamicCameraHeight = 0.0;
	bool c_selfieMode = false;
	bool GameVarsConfigured = false;

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

	static bool areAllNodesReady() {
		if (!(*g_player)->unkF0) {
			Log::sample("LoadData", "Loaded Data not set yet!");
			return false;
		}
		if (!(*g_player)->unkF0->rootNode || !f4vr::getRootNode()) {
			Log::info("Root nodes not set yet!");
			return false;
		}
		if (!f4vr::getCommonNode() || !f4vr::getPlayerNodes()) {
			Log::info("Common or Player nodes not set yet!");
			return false;
		}
		if (!f4vr::getNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode())) {
			Log::info("Arm node not set yet!");
			return false;
		}
		return true;
	}

	static bool InitSkelly(const bool inPowerArmor) {
		if (!areAllNodesReady()) {
			return false;
		}

		Log::info("Init Skeleton for %s", inPowerArmor ? "PowerArmor" : "Regular");
		Log::info("Nodes: Data=%016I64X, Root=%016I64X, Skeleton=%016I64X, Common=%016I64X,",
			(*g_player)->unkF0, (*g_player)->unkF0->rootNode, f4vr::getRootNode(), f4vr::getCommonNode());

		_skelly = new Skeleton(f4vr::getRootNode(), inPowerArmor);

		// init global handlers
		g_pipboy = new Pipboy(_skelly);
		g_configurationMode = new ConfigurationMode(_skelly);
		g_weaponPosition = new WeaponPositionAdjuster(_skelly);

		turnPipBoyOff();

		if (g_config.setScale) {
			Log::info("scale set");
			Setting* set = GetINISetting("fVrScale:VR");
			set->SetDouble(g_config.fVrScale);
		}

		Log::info("initialized");
		return true;
	}

	/**
	 * On switch from normal and power armor, reset the skelly and all dependencies with persistent data.
	 */
	static void resetSkellyAndDependencies() {
		delete _skelly;
		_skelly = nullptr;

		delete g_pipboy;
		g_pipboy = nullptr;

		delete g_configurationMode;
		g_configurationMode = nullptr;

		delete g_weaponPosition;
		g_weaponPosition = nullptr;
	}

	void smoothMovement() {
		if (!g_config.disableSmoothMovement) {
			SmoothMovementVR::everyFrame();
		}
	}

	/**
	 * Fixes the position of the muzzle flash to be at the projectile node.
	 * A workaround for two-handed weapon handling.
	 */
	static void fixMuzzleFlashPosition() {
		if (!(*g_player)->middleProcess->unk08->equipData || !(*g_player)->middleProcess->unk08->equipData->equippedData) {
			return;
		}
		const auto obj = (*g_player)->middleProcess->unk08->equipData->equippedData;
		const auto vfunc = (uint64_t*)obj;
		if ((*vfunc & 0xFFFF) == (f4vr::EquippedWeaponData_vfunc & 0xFFFF)) {
			const auto muzzle = reinterpret_cast<f4vr::MuzzleFlash*>((*g_player)->middleProcess->unk08->equipData->equippedData->unk28);
			if (muzzle && muzzle->fireNode && muzzle->projectileNode) {
				muzzle->fireNode->m_localTransform = muzzle->projectileNode->m_worldTransform;
			}
		}
	}

	void update() {
		f4vr::VRControllers.update(g_config.leftHandedMode);

		static bool inPowerArmorSticky = false;

		if (!isLoaded) {
			return;
		}

		if (!*g_player) {
			return;
		}

		if (printPlayerOnce) {
			Log::info("g_player = %016I64X", *g_player);
			printPlayerOnce = false;
		}

		const auto wasInPowerArmor = inPowerArmorSticky;
		inPowerArmorSticky = f4vr::isInPowerArmor();
		if (wasInPowerArmor != inPowerArmorSticky) {
			Log::info("Power Armor State Changed, reset skelly");
			resetSkellyAndDependencies();
			firstTime = true;
			return;
		}

		if (!_skelly || firstTime) {
			if (!InitSkelly(inPowerArmorSticky)) {
				return;
			}
			firstTime = false;
			return;
		}

		if ((*g_player)->firstPersonSkeleton == nullptr) {
			firstTime = true;
			return;
		}
		if (!((*g_player)->unkF0 && (*g_player)->unkF0->rootNode)) {
			firstTime = true;
			return;
		}

		Log::debug("Update Start of Frame...");

		// TODO: this sucks, refactor left-handed mode
		g_config.leftHandedMode = *f4vr::iniLeftHandedMode;

		if (!GameVarsConfigured) {
			// TODO: move to common single time init code
			configureGameVars();
			GameVarsConfigured = true;
		}

		g_pipboy->replaceMeshes(false);

		Log::debug("Update Skeleton...");
		_skelly->onFrameUpdate();

		Log::debug("Update Bone Sphere...");
		g_boneSpheres->onFrameUpdate();

		Log::debug("Update Pipboy...");
		g_pipboy->onFrameUpdate();

		Log::debug("Update Weapon Position...");
		g_weaponPosition->onFrameUpdate();

		f4vr::BSFadeNode_MergeWorldBounds((*g_player)->unkF0->rootNode->GetAsNiNode());
		f4vr::BSFlattenedBoneTree_UpdateBoneArray((*g_player)->unkF0->rootNode->m_children.m_data[0]);
		// just in case any transforms missed because they are not in the tree do a full flat bone array update
		f4vr::BSFadeNode_UpdateGeomArray((*g_player)->unkF0->rootNode, 1);

		fixMuzzleFlashPosition();

		g_configurationMode->onFrameUpdate();

		FrameUpdateContext context(_skelly);
		vrui::g_uiManager->onFrameUpdate(&context);

		f4vr::updateDownFromRoot(); // Last world update before exit.    Probably not necessary.

		debug(_skelly);

		if (g_config.checkDebugDumpDataOnceFor("nodes")) {
			printAllNodes(_skelly);
		}
		if (g_config.checkDebugDumpDataOnceFor("skelly")) {
			printNodes((*g_player)->firstPersonSkeleton->GetAsNiNode());
		}
	}

	void initOnGameLoaded() {
		Log::info("Starting up F4Body");
		isLoaded = true;
		g_boneSpheres = new BoneSpheresHandler();
		scopeMenuEvent.Register();

		std::srand(static_cast<unsigned int>(time(nullptr)));

		SmoothMovementVR::startFunctions();
		SmoothMovementVR::MenuOpenCloseHandler::Register();
	}

	// Papyrus Native Funcs

	// Settings Holotape related funcs

	static void openMainConfigurationMode(StaticFunctionTag* base) {
		Log::info("Open Main Configuration Mode...");
		g_configurationMode->enterConfigurationMode();
	}

	static void openPipboyConfigurationMode(StaticFunctionTag* base) {
		Log::info("Open Pipboy Configuration Mode...");
		g_configurationMode->openPipboyConfigurationMode();
	}

	static void openFrikIniFile(StaticFunctionTag* base) {
		Log::info("Open FRIK.ini file in notepad...");
		g_config.openInNotepad();
	}

	static UInt32 getWeaponRepositionMode(StaticFunctionTag* base) {
		Log::info("Papyrus: Get Weapon Reposition Mode");
		return g_weaponPosition->inWeaponRepositionMode() ? 1 : 0;
	}

	static UInt32 toggleWeaponRepositionMode(StaticFunctionTag* base) {
		Log::info("Papyrus: Toggle Weapon Reposition Mode: %s", !g_weaponPosition->inWeaponRepositionMode() ? "ON" : "OFF");
		g_weaponPosition->toggleWeaponRepositionMode();
		return getWeaponRepositionMode(base);
	}

	static bool isLeftHandedMode(StaticFunctionTag* base) {
		Log::info("Papyrus: Is Left Handed Mode");
		return *f4vr::iniLeftHandedMode;
	}

	static void setSelfieMode(StaticFunctionTag* base, const bool isSelfieMode) {
		Log::info("Papyrus: Set Selfie Mode: %s", isSelfieMode ? "ON" : "OFF");
		c_selfieMode = isSelfieMode;
	}

	static void toggleSelfieMode(StaticFunctionTag* base) {
		Log::info("Papyrus: toggle selfie mode");
		setSelfieMode(base, !c_selfieMode);
	}

	static void moveForward(StaticFunctionTag* base) {
		Log::info("Papyrus: Move Forward");
		g_config.playerOffset_forward += 1.0f;
	}

	static void moveBackward(StaticFunctionTag* base) {
		Log::info("Papyrus: Move Backward");
		g_config.playerOffset_forward -= 1.0f;
	}

	static void setDynamicCameraHeight(StaticFunctionTag* base, const float dynamicCameraHeight) {
		Log::info("Papyrus: Set Dynamic Camera Height: %f", dynamicCameraHeight);
		c_dynamicCameraHeight = dynamicCameraHeight;
	}

	// Sphere bone detection funcs
	static UInt32 registerBoneSphere(StaticFunctionTag* base, const float radius, const BSFixedString bone) {
		return g_boneSpheres->registerBoneSphere(radius, bone);
	}

	static UInt32 registerBoneSphereOffset(StaticFunctionTag* base, const float radius, const BSFixedString bone, VMArray<float> pos) {
		return g_boneSpheres->registerBoneSphereOffset(radius, bone, pos);
	}

	static void destroyBoneSphere(StaticFunctionTag* base, const UInt32 handle) {
		g_boneSpheres->destroyBoneSphere(handle);
	}

	static void registerForBoneSphereEvents(StaticFunctionTag* base, VMObject* thisObject) {
		g_boneSpheres->registerForBoneSphereEvents(thisObject);
	}

	static void unRegisterForBoneSphereEvents(StaticFunctionTag* base, VMObject* thisObject) {
		g_boneSpheres->unRegisterForBoneSphereEvents(thisObject);
	}

	static void toggleDebugBoneSpheres(StaticFunctionTag* base, const bool turnOn) {
		g_boneSpheres->toggleDebugBoneSpheres(turnOn);
	}

	static void toggleDebugBoneSpheresAtBone(StaticFunctionTag* base, const UInt32 handle, const bool turnOn) {
		g_boneSpheres->toggleDebugBoneSpheresAtBone(handle, turnOn);
	}

	// Finger pose related APIs
	static void setFingerPositionScalar(StaticFunctionTag* base, const bool isLeft, const float thumb, const float index, const float middle, const float ring, const float pinky) {
		Log::info("Papyrus: Set Finger Position Scalar '%s' (%.3f, %.3f, %.3f, %.3f, %.3f)", isLeft ? "Left" : "Right", thumb, index, middle, ring, pinky);
		setFingerPositionScalar(isLeft, thumb, index, middle, ring, pinky);
	}

	static void restoreFingerPoseControl(StaticFunctionTag* base, const bool isLeft) {
		Log::info("Papyrus: Restore Finger Pose Control '%s'", isLeft ? "Left" : "Right");
		restoreFingerPoseControl(isLeft);
	}

	/**
	 * Register code for Papyrus scripts.
	 */
	bool registerPapyrusFunctions(VirtualMachine* vm) {
		// Register code to be accessible from Settings Holotape via Papyrus scripts
		vm->RegisterFunction(new NativeFunction0("OpenMainConfigurationMode", "FRIK:FRIK", openMainConfigurationMode, vm));
		vm->RegisterFunction(new NativeFunction0("OpenPipboyConfigurationMode", "FRIK:FRIK", openPipboyConfigurationMode, vm));
		vm->RegisterFunction(new NativeFunction0("ToggleWeaponRepositionMode", "FRIK:FRIK", toggleWeaponRepositionMode, vm));
		vm->RegisterFunction(new NativeFunction0("OpenFrikIniFile", "FRIK:FRIK", openFrikIniFile, vm));
		vm->RegisterFunction(new NativeFunction0("GetWeaponRepositionMode", "FRIK:FRIK", getWeaponRepositionMode, vm));

		/// Register mod public API to be used by other mods via Papyrus scripts
		vm->RegisterFunction(new NativeFunction0("isLeftHandedMode", "FRIK:FRIK", isLeftHandedMode, vm));
		vm->RegisterFunction(new NativeFunction1("setDynamicCameraHeight", "FRIK:FRIK", setDynamicCameraHeight, vm));
		vm->RegisterFunction(new NativeFunction0("toggleSelfieMode", "FRIK:FRIK", toggleSelfieMode, vm));
		vm->RegisterFunction(new NativeFunction1("setSelfieMode", "FRIK:FRIK", setSelfieMode, vm));
		vm->RegisterFunction(new NativeFunction0("moveForward", "FRIK:FRIK", moveForward, vm));
		vm->RegisterFunction(new NativeFunction0("moveBackward", "FRIK:FRIK", moveBackward, vm));

		// Bone Sphere interaction related APIs
		vm->RegisterFunction(new NativeFunction2("RegisterBoneSphere", "FRIK:FRIK", registerBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction3("RegisterBoneSphereOffset", "FRIK:FRIK", registerBoneSphereOffset, vm));
		vm->RegisterFunction(new NativeFunction1("DestroyBoneSphere", "FRIK:FRIK", destroyBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction1("RegisterForBoneSphereEvents", "FRIK:FRIK", registerForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1("UnRegisterForBoneSphereEvents", "FRIK:FRIK", unRegisterForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1("toggleDebugBoneSpheres", "FRIK:FRIK", toggleDebugBoneSpheres, vm));
		vm->RegisterFunction(new NativeFunction2("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", toggleDebugBoneSpheresAtBone, vm));

		// Finger pose related APIs
		vm->RegisterFunction(new NativeFunction6("setFingerPositionScalar", "FRIK:FRIK", setFingerPositionScalar, vm));
		vm->RegisterFunction(new NativeFunction1("restoreFingerPoseControl", "FRIK:FRIK", restoreFingerPoseControl, vm));

		return true;
	}
}
