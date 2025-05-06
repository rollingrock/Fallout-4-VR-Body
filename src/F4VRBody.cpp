#include "F4VRBody.h"
#include <algorithm>
#include <f4se/GameRTTI.h>
#include "BSFlattenedBoneTree.h"
#include "Config.h"
#include "ConfigurationMode.h"
#include "CullGeometryHandler.h"
#include "Debug.h"
#include "HandPose.h"
#include "Menu.h"
#include "MuzzleFlash.h"
#include "Pipboy.h"
#include "Skeleton.h"
#include "SmoothMovementVR.h"
#include "utils.h"
#include "common/Logger.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4vr/VR.h"
#include "ui/UIManager.h"
#include "ui/UIModAdapter.h"

using namespace common;

bool firstTime = true;
bool printPlayerOnce = true;

PluginHandle g_pluginHandle = kPluginHandle_Invalid;
F4SEPapyrusInterface* g_papyrus = nullptr;
F4SEMessagingInterface* g_messaging = nullptr;

UInt32 KeywordPowerArmor = 0x4D8A1;
UInt32 KeywordPowerArmorFrame = 0x15503F;

OpenVRHookManagerAPI* _vrhook;

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
		FrameUpdateContext(Skeleton* skelly, OpenVRHookManagerAPI* vrhook)
			: _skelly(skelly), _vrhook(vrhook) {}

		virtual NiPoint3 getInteractionBoneWorldPosition() override {
			return _skelly->getOffhandIndexFingerTipWorldPosition();
		}

		virtual void fireInteractionHeptic() override {
			_vrhook->StartHaptics(g_config->leftHandedMode ? 2 : 1, 0.2f, 0.3f);
		}

		virtual void setInteractionHandPointing(const bool primaryHand, const bool toPoint) override {
			setForceHandPointingPose(primaryHand, toPoint);
		}

	private:
		Skeleton* _skelly;
		OpenVRHookManagerAPI* _vrhook;
	};

	static void fixSkeleton() {
		NiNode* pn = (*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode();

		static BSFixedString lHand("LArm_Hand");
		static BSFixedString lArm("LArm_ForeArm1");
		static BSFixedString lfarm("LArm_ForeArm2");
		static BSFixedString rHand("RArm_Hand");
		static BSFixedString rArm("RArm_ForeArm1");
		static BSFixedString rfarm("RArm_ForeArm2");
		static BSFixedString pipboyName("PipboyBone");

		NiNode* hand = pn->GetObjectByName(&lHand)->GetAsNiNode();
		NiNode* arm = pn->GetObjectByName(&lArm)->GetAsNiNode();
		NiNode* forearm = pn->GetObjectByName(&lfarm)->GetAsNiNode();
		auto pipboy = static_cast<NiNode*>(pn->m_children.m_data[0]->GetObjectByName(&pipboyName));

		Skeleton sk;
		const bool inPA = sk.detectInPowerArmor();

		if (!inPA) {
			if (arm->m_children.m_data[0] == hand) {
				arm->RemoveChildAt(0);
				if (pipboy) {
					pipboy->m_parent->RemoveChild(pipboy);
				} else {
					pipboy = static_cast<NiNode*>(pn->GetObjectByName(&pipboyName));
				}
				forearm->m_parent->RemoveChild(forearm);
				arm->AttachChild(forearm, true);
				forearm->m_children.m_data[0]->GetAsNiNode()->AttachChild(hand, true);
				if (pipboy) {
					forearm->m_children.m_data[0]->GetAsNiNode()->AttachChild(pipboy, true);
				}
			}

			hand = pn->GetObjectByName(&rHand)->GetAsNiNode();
			arm = pn->GetObjectByName(&rArm)->GetAsNiNode();
			forearm = pn->GetObjectByName(&rfarm)->GetAsNiNode();

			if (arm->m_children.m_data[0] == hand) {
				arm->RemoveChildAt(0);
				forearm->m_parent->RemoveChild(forearm);
				arm->AttachChild(forearm, true);
				forearm->m_children.m_data[0]->GetAsNiNode()->AttachChild(hand, true);
			}
		}
	}

	static void fixMissingScreen(f4vr::PlayerNodes* pn) {
		if (const auto screenNode = pn->ScreenNode) {
			const BSFixedString screenName("Screen:0");
			const NiAVObject* newScreen = screenNode->GetObjectByName(&screenName);

			if (!newScreen) {
				pn->ScreenNode->RemoveChildAt(0);

				newScreen = pn->PipboyRoot_nif_only_node->GetObjectByName(&screenName)->m_parent;
				NiNode* rn = Offsets::addNode((uint64_t)&pn->ScreenNode, newScreen);
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
			const auto node = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode());
			if (!node) {
				Log::info("root node not found");
				return false;
			}

			initHandPoses(inPowerArmor);

			_skelly = new Skeleton(node);
			Log::info("skeleton = %016I64X", _skelly->getRoot());
			if (!_skelly->setNodes()) {
				return false;
			}
			//replaceMeshes(_skelly->getPlayerNodes());
			//_skelly->setDirection();

			_vrhook = RequestOpenVRHookManagerObject();

			// init global handlers
			g_pipboy = new Pipboy(_skelly, _vrhook);
			g_configurationMode = new ConfigurationMode(_skelly, _vrhook);
			g_cullGeometry = new CullGeometryHandler();
			g_weaponPosition = new WeaponPositionAdjuster(_skelly, _vrhook);

			turnPipBoyOff();

			if (g_config->setScale) {
				Setting* set = GetINISetting("fVrScale:VR");
				set->SetDouble(g_config->fVrScale);
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
		if (!g_config->disableSmoothMovement) {
			SmoothMovementVR::everyFrame();
		}
	}

	bool HasKeywordPA(const TESObjectARMO* armor, const UInt32 keywordFormId) {
		if (armor) {
			for (UInt32 i = 0; i < armor->keywordForm.numKeywords; i++) {
				if (armor->keywordForm.keywords[i]) {
					if (armor->keywordForm.keywords[i]->formID == keywordFormId) {
						return true;
					}
				}
			}
		}
		return false;
	}

	static bool detectInPowerArmor() {
		// Thanks Shizof and SmoothtMovementVR for below code
		if ((*g_player)->equipData) {
			if ((*g_player)->equipData->slots[0x03].item != nullptr) {
				TESForm* equippedForm = (*g_player)->equipData->slots[0x03].item;
				if (equippedForm) {
					if (equippedForm->formType == TESObjectARMO::kTypeID) {
						const auto armor = DYNAMIC_CAST(equippedForm, TESForm, TESObjectARMO);

						if (armor) {
							if (HasKeywordPA(armor, KeywordPowerArmor) || HasKeywordPA(armor, KeywordPowerArmorFrame)) {
								return true;
							}
							return false;
						}
					}
				}
			}
		}
		return false;
	}

	void update() {
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

		g_config->onUpdateFrame();

		const auto wasInPowerArmor = inPowerArmorSticky;
		inPowerArmorSticky = detectInPowerArmor();
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

			//StackPtr<BSAnimationManager*> manager;
			//AIProcess_getAnimationManager((uint64_t)(*g_player)->middleProcess, manager);
			//BSAnimationManager_setActiveGraph(manager.p, 0);
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

		if (_skelly->getRoot() != static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode())) {
			const auto node = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode());
			if (!node) {
				return;
			}

			_skelly->updateRoot(node);
			_skelly->setNodes();
			_skelly->setDirection();
			g_pipboy->swapPipboy();
			_skelly->setBodyLen();
			// TODO: check if this is needed as the same call is done 10 lines below
			g_pipboy->replaceMeshes(false);
			Log::info("initialized for real");
			return;
		}

		// do stuff now
		g_config->leftHandedMode = *Offsets::iniLeftHandedMode;
		_skelly->setLeftHandedSticky();

		Log::debug("Start of Frame");

		if (!GameVarsConfigured) {
			// TODO: move to common single time init code
			configureGameVars();
			GameVarsConfigured = true;
		}

		g_config->leftHandedMode = *Offsets::iniLeftHandedMode;

		g_pipboy->replaceMeshes(false);

		// check if jumping or in air;
		c_jumping = SmoothMovementVR::checkIfJumpingOrInAir();

		_skelly->setTime();

		f4vr::g_vrHook->setVRControllerState();

		Log::debug("Hide Wands");
		_skelly->hideWands();

		//	fixSkeleton();

		const NiPoint3 position = (*g_player)->pos;
		constexpr float groundHeight = 0.0f;

		uint64_t ret = Offsets::TESObjectCell_GetLandHeight((*g_player)->parentCell, &position, &groundHeight);

		// first restore locals to a default state to wipe out any local transform changes the game might have made since last update
		Log::debug("restore locals of skeleton");
		_skelly->restoreLocals(_skelly->getRoot()->m_parent->GetAsNiNode());
		f4vr::updateDown(_skelly->getRoot(), true);

		// moves head up and back out of the player view.   doing this instead of hiding with a small scale setting since it preserves neck shape
		Log::debug("Setup Head");
		NiNode* headNode = f4vr::getNode("Head", _skelly->getRoot());
		_skelly->setupHead(headNode, g_config->hideHead);

		//// set up the body underneath the headset in a proper scale and orientation
		Log::debug("Set body under HMD");
		_skelly->setUnderHMD(groundHeight);
		f4vr::updateDown(_skelly->getRoot(), true); // Do world update now so that IK calculations have proper world reference

		// Now Set up body Posture and hook up the legs
		Log::debug("Set body posture");
		_skelly->setBodyPosture();
		f4vr::updateDown(_skelly->getRoot(), true); // Do world update now so that IK calculations have proper world reference

		Log::debug("Set Knee Posture");
		_skelly->setKneePos();
		Log::debug("Set Walk");

		if (!g_config->armsOnly) {
			_skelly->walk();
		}
		//_skelly->setLegs();
		Log::debug("Set Legs");
		_skelly->setSingleLeg(false);
		_skelly->setSingleLeg(true);

		// Do another update before setting arms
		f4vr::updateDown(_skelly->getRoot(), true); // Do world update now so that IK calculations have proper world reference

		// do arm IK - Right then Left
		Log::debug("Set Arms");
		_skelly->handleWeaponNodes();
		_skelly->setArms(false);
		_skelly->setArms(true);
		_skelly->leftHandedModePipboy();
		f4vr::updateDown(_skelly->getRoot(), true); // Do world update now so that IK calculations have proper world reference

		// Misc stuff to showahide things and also setup the wrist pipboy
		Log::debug("Pipboy and Weapons");
		_skelly->hideWeapon();
		_skelly->positionPipboy();
		_skelly->hidePipboy();
		_skelly->hideFistHelpers();
		_skelly->showHidePAHud();

		g_cullGeometry->cullPlayerGeometry();

		// project body out in front of the camera for debug purposes
		Log::debug("Selfie Time");
		_skelly->selfieSkelly();
		f4vr::updateDown(_skelly->getRoot(), true);

		Log::debug("fix the missing screen");
		fixMissingScreen(_skelly->getPlayerNodes());

		if (g_config->armsOnly) {
			_skelly->showOnlyArms();
		}

		Log::debug("Operate Skelly hands");
		_skelly->setHandPose();

		Log::debug("Operate Pipboy");
		g_pipboy->operatePipBoy();

		Log::debug("bone sphere stuff");
		g_boneSpheres->onFrameUpdate();

		Log::debug("Weapon position");
		g_weaponPosition->onFrameUpdate();

		//g_gunReloadSystem->Update();

		Offsets::BSFadeNode_MergeWorldBounds((*g_player)->unkF0->rootNode->GetAsNiNode());
		BSFlattenedBoneTree_UpdateBoneArray((*g_player)->unkF0->rootNode->m_children.m_data[0]);
		// just in case any transforms missed because they are not in the tree do a full flat bone array update
		Offsets::BSFadeNode_UpdateGeomArray((*g_player)->unkF0->rootNode, 1);

		if ((*g_player)->middleProcess->unk08->equipData && (*g_player)->middleProcess->unk08->equipData->equippedData) {
			const auto obj = (*g_player)->middleProcess->unk08->equipData->equippedData;
			const auto vfunc = (uint64_t*)obj;
			if ((*vfunc & 0xFFFF) == (Offsets::EquippedWeaponData_vfunc & 0xFFFF)) {
				const auto muzzle = reinterpret_cast<MuzzleFlash*>((*g_player)->middleProcess->unk08->equipData->equippedData->unk28);
				if (muzzle && muzzle->fireNode && muzzle->projectileNode) {
					muzzle->fireNode->m_localTransform = muzzle->projectileNode->m_worldTransform;
				}
			}
		}

		if (isInScopeMenu()) {
			_skelly->hideHands();
		}

		g_pipboy->onUpdate();
		g_configurationMode->onUpdate();

		FrameUpdateContext context(_skelly, _vrhook);
		vrui::g_uiManager->onFrameUpdate(&context);

		f4vr::updateDown(_skelly->getRoot(), true); // Last world update before exit.    Probably not necessary.

		debug(_skelly);

		if (!detectInPowerArmor()) {
			// sets 3rd Person Pipboy Scale
			NiNode* _Pipboy3rd = getChildNode("PipboyBone", (*g_player)->unkF0->rootNode);
			if (_Pipboy3rd) {
				_Pipboy3rd->m_localTransform.scale = g_config->pipBoyScale;
			}
		} else {
			_skelly->fixArmor();
		}

		if (g_config->checkDebugDumpDataOnceFor("nodes")) {
			printAllNodes(_skelly);
		}
		if (g_config->checkDebugDumpDataOnceFor("skelly")) {
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
		g_config->openInNotepad();
	}

	static UInt32 getFrikIniAutoReloading(StaticFunctionTag* base) {
		return g_config->getAutoReloadConfigInterval() > 0 ? 1 : 0;
	}

	static UInt32 toggleReloadFrikIniConfig(StaticFunctionTag* base) {
		Log::info("Papyrus: Toggle reload FRIK.ini config file...");
		g_config->toggleAutoReloadConfig();
		return getFrikIniAutoReloading(base);
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
		return *Offsets::iniLeftHandedMode;
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
		g_config->playerOffset_forward += 1.0f;
	}

	static void moveBackward(StaticFunctionTag* base) {
		Log::info("Papyrus: Move Backward");
		g_config->playerOffset_forward -= 1.0f;
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
		vm->RegisterFunction(new NativeFunction0("ToggleReloadFrikIniConfig", "FRIK:FRIK", toggleReloadFrikIniConfig, vm));
		vm->RegisterFunction(new NativeFunction0("GetWeaponRepositionMode", "FRIK:FRIK", getWeaponRepositionMode, vm));
		vm->RegisterFunction(new NativeFunction0("GetFrikIniAutoReloading", "FRIK:FRIK", getFrikIniAutoReloading, vm));

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
