#include "F4VRBody.h"

#include "Config.h"
#include "ConfigurationMode.h"
#include "CullGeometryHandler.h"
#include "Debug.h"
#include "HandPose.h"
#include "Menu.h"
#include "Pipboy.h"
#include "Skeleton.h"
#include "SmoothMovementVR.h"
#include "utils.h"
#include "common/Logger.h"
#include "f4se/PapyrusNativeFunctions.h"
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
	CullGeometryHandler* g_cullGeometry = nullptr;
	BoneSpheresHandler* g_boneSpheres = nullptr;
	WeaponPositionAdjuster* g_weaponPosition = nullptr;

	Skeleton* _skelly = nullptr;

	bool isLoaded = false;

	bool c_isLookingThroughScope = false;
	bool c_jumping = false;
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

	static void fixMissingScreen(f4vr::PlayerNodes* pn) {
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

	static bool InitSkelly(const bool inPowerArmor) {
		if (!(*g_player)->unkF0) {
			Log::debug("loaded Data Not Set Yet");
			return false;
		}

		Log::info("Init Skelly - %s (Data: %016I64X)", inPowerArmor ? "PowerArmor" : "Regular", (*g_player)->unkF0);
		if (!(*g_player)->unkF0->rootNode) {
			Log::info("root node not set yet!");
			return false;
		}

		Log::info("root node   = %016I64X", (*g_player)->unkF0->rootNode);

		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			Log::info("set root");
			const auto rootNode = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode());
			if (!rootNode) {
				Log::info("root node not found");
				return false;
			}

			initHandPoses(inPowerArmor);

			_skelly = new Skeleton(rootNode);
			Log::info("skeleton = %016I64X", rootNode);
			if (!_skelly->setNodes()) {
				return false;
			}
			//replaceMeshes(f4vr::getPlayerNodes());
			//_skelly->setDirection();

			// init global handlers
			g_pipboy = new Pipboy(_skelly);
			g_configurationMode = new ConfigurationMode(_skelly);
			g_cullGeometry = new CullGeometryHandler();
			g_weaponPosition = new WeaponPositionAdjuster(_skelly);

			turnPipBoyOff();

			if (g_config.setScale) {
				Setting* set = GetINISetting("fVrScale:VR");
				set->SetDouble(g_config.fVrScale);
			}
			Log::info("scale set");

			_skelly->setBodyLen();
			Log::info("initialized");
			return true;
		}
		return false;
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

		delete g_cullGeometry;
		g_cullGeometry = nullptr;

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

		// do stuff now
		g_config.leftHandedMode = *f4vr::iniLeftHandedMode;
		_skelly->setLeftHandedSticky();

		Log::debug("Start of Frame");

		if (!GameVarsConfigured) {
			// TODO: move to common single time init code
			configureGameVars();
			GameVarsConfigured = true;
		}

		g_config.leftHandedMode = *f4vr::iniLeftHandedMode;

		g_pipboy->replaceMeshes(false);

		// check if jumping or in air;
		c_jumping = f4vr::isJumpingOrInAir();

		_skelly->setTime();

		_skelly->onFrameUpdate();

		Log::debug("fix the missing screen");
		fixMissingScreen(f4vr::getPlayerNodes());

		Log::debug("Operate Pipboy");
		g_pipboy->operatePipBoy();

		Log::debug("bone sphere stuff");
		g_boneSpheres->onFrameUpdate();

		Log::debug("Weapon position");
		g_weaponPosition->onFrameUpdate();

		f4vr::BSFadeNode_MergeWorldBounds((*g_player)->unkF0->rootNode->GetAsNiNode());
		f4vr::BSFlattenedBoneTree_UpdateBoneArray((*g_player)->unkF0->rootNode->m_children.m_data[0]);
		// just in case any transforms missed because they are not in the tree do a full flat bone array update
		f4vr::BSFadeNode_UpdateGeomArray((*g_player)->unkF0->rootNode, 1);

		fixMuzzleFlashPosition();

		g_pipboy->onUpdate();
		g_configurationMode->onUpdate();

		FrameUpdateContext context(_skelly);
		vrui::g_uiManager->onFrameUpdate(&context);

		f4vr::updateDownFromRoot(); // Last world update before exit.    Probably not necessary.

		debug(_skelly);

		// TODO: move this inside skelly on frame update
		if (!f4vr::isInPowerArmor()) {
			// sets 3rd Person Pipboy Scale
			NiNode* _Pipboy3rd = f4vr::getChildNode("PipboyBone", (*g_player)->unkF0->rootNode);
			if (_Pipboy3rd) {
				_Pipboy3rd->m_localTransform.scale = g_config.pipBoyScale;
			}
		} else {
			_skelly->fixArmor();
		}

		if (g_config.checkDebugDumpDataOnceFor("nodes")) {
			printAllNodes(_skelly);
		}
		if (g_config.checkDebugDumpDataOnceFor("skelly")) {
			printNodes((*g_player)->firstPersonSkeleton->GetAsNiNode());
		}
	}

	void startUp() {
		Log::info("Starting up F4Body");
		isLoaded = true;
		g_boneSpheres = new BoneSpheresHandler();
		scopeMenuEvent.Register();
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
		vm->RegisterFunction(
			new NativeFunction3("RegisterBoneSphereOffset", "FRIK:FRIK", registerBoneSphereOffset, vm));
		vm->RegisterFunction(new NativeFunction1("DestroyBoneSphere", "FRIK:FRIK", destroyBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction1("RegisterForBoneSphereEvents", "FRIK:FRIK", registerForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1("UnRegisterForBoneSphereEvents", "FRIK:FRIK", unRegisterForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1("toggleDebugBoneSpheres", "FRIK:FRIK", toggleDebugBoneSpheres, vm));
		vm->RegisterFunction(new NativeFunction2("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", toggleDebugBoneSpheresAtBone, vm));

		// Finger pose related APIs
		vm->RegisterFunction(
			new NativeFunction6("setFingerPositionScalar", "FRIK:FRIK", setFingerPositionScalar, vm));
		vm->RegisterFunction(new NativeFunction1("restoreFingerPoseControl", "FRIK:FRIK", restoreFingerPoseControl, vm));

		return true;
	}
}
