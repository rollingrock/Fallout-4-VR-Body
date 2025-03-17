#include "F4VRBody.h"
#include "Config.h"
#include "ConfigurationMode.h"
#include "Skeleton.h"
#include "Pipboy.h"
#include "HandPose.h"
#include "utils.h"
#include "MuzzleFlash.h"
#include "BSFlattenedBoneTree.h"
#include "CullGeometryHandler.h"
#include "GunReload.h"
#include "VR.h"
#include "Debug.h"

#include "api/PapyrusVRAPI.h"
#include "api/VRManagerAPI.h"

#include <algorithm>

#include "Menu.h"
#include "MiscStructs.h"


bool firstTime = true;
bool printPlayerOnce = true;

PluginHandle g_pluginHandle = kPluginHandle_Invalid;
F4SEPapyrusInterface* g_papyrus = NULL;
F4SEMessagingInterface* g_messaging = NULL;

UInt32 KeywordPowerArmor = 0x4D8A1;
UInt32 KeywordPowerArmorFrame = 0x15503F;

OpenVRHookManagerAPI* _vrhook;

namespace F4VRBody {

	Pipboy* g_pipboy = nullptr;
	ConfigurationMode* g_configurationMode = nullptr;
	CullGeometryHandler* g_cullGeometry = nullptr;
	BoneSpheresHandler* g_boneSpheres = nullptr;

	Skeleton* _skelly = nullptr;

	bool isLoaded = false;

	uint64_t updateCounter = 0;
	uint64_t prevCounter = 0;
	uint64_t localCounter = 0;

	bool c_isLookingThroughScope = false;
	bool c_jumping = false;
	float c_dynamicCameraHeight = 0.0;
	bool c_selfieMode = false;
	bool GameVarsConfigured = false;
	bool c_weaponRepositionMasterMode = false;

	std::map<std::string, NiTransform, CaseInsensitiveComparator> handClosed;
	std::map<std::string, NiTransform, CaseInsensitiveComparator> handOpen;

	std::map<std::string, float> handPapyrusPose;
	std::map<std::string, bool> handPapyrusHasControl;

	UInt32 PipboyAA = 0x0001ED3D;
	UInt32 PipboyArmor = 0x00021B3B;
	UInt32 MiningHelmet = 0x0022DD1F;
	UInt32 PALightKW = 0x000B34A6;


	void fixSkeleton() {

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
		NiNode* pipboy = (NiNode*)pn->m_children.m_data[0]->GetObjectByName(&pipboyName);

		Skeleton sk;
		bool inPA = sk.detectInPowerArmor();

		if (!inPA) {
			if (arm->m_children.m_data[0] == hand) {
				arm->RemoveChildAt(0);
				if (pipboy) {
					pipboy->m_parent->RemoveChild(pipboy);
				}
				else {
					pipboy = (NiNode*)pn->GetObjectByName(&pipboyName);
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

	void fixMissingScreen(PlayerNodes* pn) {
		NiNode* screenNode = pn->ScreenNode;

		if (screenNode) {
			BSFixedString screenName("Screen:0");
			NiAVObject* newScreen = screenNode->GetObjectByName(&screenName);

			if (!newScreen) {
				pn->ScreenNode->RemoveChildAt(0);

				newScreen = pn->PipboyRoot_nif_only_node->GetObjectByName(&screenName)->m_parent;
				NiNode* rn = Offsets::addNode((uint64_t)&pn->ScreenNode, newScreen);
			}
		}
	}

	void setHandUI(PlayerNodes* pn) {
		static NiPoint3 origLoc(0, 0, 0);

		NiNode* wand = pn->primaryUIAttachNode;
		BSFixedString bname = "BackOfHand";
		NiNode* node = (NiNode*)wand->GetObjectByName(&bname);

		if (!node) {
			return;
		}

		if (vec3_len(origLoc) == 0.0) {
			origLoc = node->m_localTransform.pos;
		}
		node->m_localTransform.pos = origLoc + NiPoint3(g_config->handUI_X, g_config->handUI_Y, g_config->handUI_Z);

		updateTransformsDown(node, true);
	}

	static bool InitSkelly(bool inPowerArmor) {
		
		if (!(*g_player)->unkF0) {
			_DMESSAGE("loaded Data Not Set Yet");
			return false;
		}

		_MESSAGE("Init Skelly - %s (Data: %016I64X)", inPowerArmor ? "PowerArmor" : "Regular", (*g_player)->unkF0);
		if (!(*g_player)->unkF0->rootNode) {
			_MESSAGE("rootnode not set yet!");
			return false;
		}

		_MESSAGE("rootnode   = %016I64X", (*g_player)->unkF0->rootNode);

		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			_MESSAGE("set root");
			auto node = (BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode();
			if (!node) {
				_MESSAGE("root node not found");
				return false;
			}

			initHandPoses(inPowerArmor);

			_skelly = new Skeleton(node);
			_MESSAGE("skeleton = %016I64X", _skelly->getRoot());
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

			turnPipBoyOff();

			if (g_config->setScale) {
				Setting* set = GetINISetting("fVrScale:VR");
				set->SetDouble(g_config->fVrScale);
			}
			_MESSAGE("scale set");

			_skelly->setBodyLen();
			_MESSAGE("initialized");
			return true;
		}
		else {
			return false;
		}
	}

	/// <summary>
	/// On switch from normal and power armor, reset the skelly and all dependencies with persistant data.
	/// </summary>
	static void resetSkellyAndDependencies() {
		delete _skelly;
		_skelly = nullptr;

		delete g_pipboy;
		g_pipboy = nullptr;

		delete g_configurationMode;
		g_configurationMode = nullptr;

		delete g_cullGeometry;
		g_cullGeometry = nullptr;
	}

	void smoothMovement()
	{
		if (!g_config->disableSmoothMovement) {
			SmoothMovementVR::everyFrame();
		}
	}

	bool HasKeywordPA(TESObjectARMO* armor, UInt32 keywordFormId)
	{
		if (armor)
		{
			for (UInt32 i = 0; i < armor->keywordForm.numKeywords; i++)
			{
				if (armor->keywordForm.keywords[i])
				{
					if (armor->keywordForm.keywords[i]->formID == keywordFormId)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool detectInPowerArmor() {

		// Thanks Shizof and SmoothtMovementVR for below code
		if ((*g_player)->equipData) {
			if ((*g_player)->equipData->slots[0x03].item != nullptr)
			{
				TESForm* equippedForm = (*g_player)->equipData->slots[0x03].item;
				if (equippedForm)
				{
					if (equippedForm->formType == TESObjectARMO::kTypeID)
					{
						TESObjectARMO* armor = DYNAMIC_CAST(equippedForm, TESForm, TESObjectARMO);

						if (armor)
						{
							if (HasKeywordPA(armor, KeywordPowerArmor) || HasKeywordPA(armor, KeywordPowerArmorFrame))
							{
								return true;
							}
							else
							{
								return false;
							}
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

		if (!(*g_player)) {
			return;
		}

		if (printPlayerOnce) {
			_MESSAGE("g_player = %016I64X", (*g_player));
			printPlayerOnce = false;
		}

		g_config->onUpdateFrame();

		auto wasInPowerArmor = inPowerArmorSticky;
		inPowerArmorSticky = detectInPowerArmor();
		if (wasInPowerArmor != inPowerArmorSticky) {
			_MESSAGE("Power Armor State Changed, reset skelly");
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

		if (_skelly->getRoot() != (BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode()) {

			auto node = (BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode();
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
			_MESSAGE("initialized for real");
			return;
		}

		// do stuff now
		 g_config->leftHandedMode = *Offsets::iniLeftHandedMode;
		_skelly->setLeftHandedSticky();


		_DMESSAGE("Start of Frame");

		if (!GameVarsConfigured) {
			// TODO: move to common single time init code
			ConfigureGameVars();
			GameVarsConfigured = true;
		}

		 g_config->leftHandedMode = *Offsets::iniLeftHandedMode;
		
		g_pipboy->replaceMeshes(false);

		// check if jumping or in air;
		 c_jumping = SmoothMovementVR::checkIfJumpingOrInAir();

		_skelly->setTime();

		VRHook::g_vrHook->setVRControllerState();

		_DMESSAGE("Hide Wands");
		_skelly->hideWands();

		//	fixSkeleton();

		NiPoint3 position = (*g_player)->pos;
		float groundHeight = 0.0f;

		uint64_t ret = Offsets::TESObjectCell_GetLandHeight((*g_player)->parentCell, &position, &groundHeight);

		// first restore locals to a default state to wipe out any local transform changes the game might have made since last update
		_DMESSAGE("restore locals of skeleton");
		_skelly->restoreLocals(_skelly->getRoot()->m_parent->GetAsNiNode());
		_skelly->updateDown(_skelly->getRoot(), true);


		// moves head up and back out of the player view.   doing this instead of hiding with a small scale setting since it preserves neck shape
		_DMESSAGE("Setup Head");
		NiNode* headNode = _skelly->getNode("Head", _skelly->getRoot());
		_skelly->setupHead(headNode, g_config->hideHead);

		//// set up the body underneath the headset in a proper scale and orientation
		_DMESSAGE("Set body under HMD");
		_skelly->setUnderHMD(groundHeight);
		_skelly->updateDown(_skelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// Now Set up body Posture and hook up the legs
		_DMESSAGE("Set body posture");
		_skelly->setBodyPosture();
		_skelly->updateDown(_skelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		_DMESSAGE("Set Knee Posture");
		_skelly->setKneePos();
		_DMESSAGE("Set Walk");

		if (!g_config->armsOnly) {
			_skelly->walk();
		}
		//_skelly->setLegs();
		_DMESSAGE("Set Legs");
		_skelly->setSingleLeg(false);
		_skelly->setSingleLeg(true);

		// Do another update before setting arms
		_skelly->updateDown(_skelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// do arm IK - Right then Left
		_DMESSAGE("Set Arms");
		_skelly->handleWeaponNodes();
		_skelly->setArms(false);
		_skelly->setArms(true);
		_skelly->leftHandedModePipboy();
		_skelly->updateDown(_skelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// Misc stuff to showahide things and also setup the wrist pipboy
		_DMESSAGE("Pipboy and Weapons");
		_skelly->hideWeapon();
		_skelly->positionPipboy();
		_skelly->hidePipboy();
		_skelly->fixMelee();
		_skelly->hideFistHelpers();
		_skelly->showHidePAHUD();

		g_cullGeometry->cullPlayerGeometry();

		// project body out in front of the camera for debug purposes
		_DMESSAGE("Selfie Time");
		_skelly->selfieSkelly();
		_skelly->updateDown(_skelly->getRoot(), true);

		_DMESSAGE("fix the missing screen");
		fixMissingScreen(_skelly->getPlayerNodes());

		setHandUI(_skelly->getPlayerNodes());

		if (g_config->armsOnly) {
			_skelly->showOnlyArms();
		}

		_skelly->setHandPose();
		_DMESSAGE("Operate Pipboy");
		g_pipboy->operatePipBoy();
		
		_DMESSAGE("bone sphere stuff");
		g_boneSpheres->onFrameUpdate();

		//g_gunReloadSystem->Update();


		_skelly->offHandToBarrel();
		_skelly->offHandToScope();

		Offsets::BSFadeNode_MergeWorldBounds((*g_player)->unkF0->rootNode->GetAsNiNode());
		BSFlattenedBoneTree_UpdateBoneArray((*g_player)->unkF0->rootNode->m_children.m_data[0]); // just in case any transforms missed because they are not in the tree do a full flat bone array update
		Offsets::BSFadeNode_UpdateGeomArray((*g_player)->unkF0->rootNode, 1);

		if ((*g_player)->middleProcess->unk08->equipData && (*g_player)->middleProcess->unk08->equipData->equippedData) {
			auto obj = (*g_player)->middleProcess->unk08->equipData->equippedData;
			uint64_t* vfunc = (uint64_t*)obj;
			if ((*vfunc & 0xFFFF) == (Offsets::EquippedWeaponData_vfunc & 0xFFFF)) {
				MuzzleFlash* muzzle = reinterpret_cast<MuzzleFlash*>((*g_player)->middleProcess->unk08->equipData->equippedData->unk28);
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
		
		_skelly->fixBackOfHand();
		_skelly->updateDown(_skelly->getRoot(), true);  // Last world update before exit.    Probably not necessary.

		debug(_skelly);

		if (!detectInPowerArmor()) { // sets 3rd Person Pipboy Scale
			NiNode* _Pipboy3rd = getChildNode("PipboyBone", (*g_player)->unkF0->rootNode);
			if (_Pipboy3rd) {
				_Pipboy3rd->m_localTransform.scale = g_config->pipBoyScale;
			}
		}
		else {
			_skelly->fixArmor();
		}

		if (g_config->checkDebugDumpDataOnceFor("skelly")) {
			printNodes((*g_player)->firstPersonSkeleton->GetAsNiNode());
		}
	}

	void startUp() {
		_MESSAGE("Starting up F4Body");
		isLoaded = true;
		g_boneSpheres = new BoneSpheresHandler();
		scopeMenuEvent.Register();
		return;
	}


	// Papyrus Native Funcs

	static void setFingerPositionScalar(StaticFunctionTag* base, bool isLeft, float thumb, float index, float middle, float ring, float pinky) {
		if (isLeft) {
			handPapyrusHasControl["LArm_Finger11"] = true;
			handPapyrusHasControl["LArm_Finger12"] = true;
			handPapyrusHasControl["LArm_Finger13"] = true;
			handPapyrusHasControl["LArm_Finger21"] = true;
			handPapyrusHasControl["LArm_Finger22"] = true;
			handPapyrusHasControl["LArm_Finger23"] = true;
			handPapyrusHasControl["LArm_Finger31"] = true;
			handPapyrusHasControl["LArm_Finger32"] = true;
			handPapyrusHasControl["LArm_Finger33"] = true;
			handPapyrusHasControl["LArm_Finger41"] = true;
			handPapyrusHasControl["LArm_Finger42"] = true;
			handPapyrusHasControl["LArm_Finger43"] = true;
			handPapyrusHasControl["LArm_Finger51"] = true;
			handPapyrusHasControl["LArm_Finger52"] = true;
			handPapyrusHasControl["LArm_Finger53"] = true;
			handPapyrusPose["LArm_Finger11"] = thumb;
			handPapyrusPose["LArm_Finger12"] = thumb;
			handPapyrusPose["LArm_Finger13"] = thumb;
			handPapyrusPose["LArm_Finger21"] = index;
			handPapyrusPose["LArm_Finger22"] = index;
			handPapyrusPose["LArm_Finger23"] = index;
			handPapyrusPose["LArm_Finger31"] = middle;
			handPapyrusPose["LArm_Finger32"] = middle;
			handPapyrusPose["LArm_Finger33"] = middle;
			handPapyrusPose["LArm_Finger41"] = ring;
			handPapyrusPose["LArm_Finger42"] = ring;
			handPapyrusPose["LArm_Finger43"] = ring;
			handPapyrusPose["LArm_Finger51"] = pinky;
			handPapyrusPose["LArm_Finger52"] = pinky;
			handPapyrusPose["LArm_Finger53"] = pinky;
		}
		else {
			handPapyrusHasControl["RArm_Finger11"] = true;
			handPapyrusHasControl["RArm_Finger12"] = true;
			handPapyrusHasControl["RArm_Finger13"] = true;
			handPapyrusHasControl["RArm_Finger21"] = true;
			handPapyrusHasControl["RArm_Finger22"] = true;
			handPapyrusHasControl["RArm_Finger23"] = true;
			handPapyrusHasControl["RArm_Finger31"] = true;
			handPapyrusHasControl["RArm_Finger32"] = true;
			handPapyrusHasControl["RArm_Finger33"] = true;
			handPapyrusHasControl["RArm_Finger41"] = true;
			handPapyrusHasControl["RArm_Finger42"] = true;
			handPapyrusHasControl["RArm_Finger43"] = true;
			handPapyrusHasControl["RArm_Finger51"] = true;
			handPapyrusHasControl["RArm_Finger52"] = true;
			handPapyrusHasControl["RArm_Finger53"] = true;
			handPapyrusPose["RArm_Finger11"] = thumb;
			handPapyrusPose["RArm_Finger12"] = thumb;
			handPapyrusPose["RArm_Finger13"] = thumb;
			handPapyrusPose["RArm_Finger21"] = index;
			handPapyrusPose["RArm_Finger22"] = index;
			handPapyrusPose["RArm_Finger23"] = index;
			handPapyrusPose["RArm_Finger31"] = middle;
			handPapyrusPose["RArm_Finger32"] = middle;
			handPapyrusPose["RArm_Finger33"] = middle;
			handPapyrusPose["RArm_Finger41"] = ring;
			handPapyrusPose["RArm_Finger42"] = ring;
			handPapyrusPose["RArm_Finger43"] = ring;
			handPapyrusPose["RArm_Finger51"] = pinky;
			handPapyrusPose["RArm_Finger52"] = pinky;
			handPapyrusPose["RArm_Finger53"] = pinky;
		}
	}

	static void restoreFingerPoseControl(StaticFunctionTag* base, bool isLeft) {
		if (isLeft) {
			handPapyrusHasControl["LArm_Finger11"] = false;
			handPapyrusHasControl["LArm_Finger12"] = false;
			handPapyrusHasControl["LArm_Finger13"] = false;
			handPapyrusHasControl["LArm_Finger21"] = false;
			handPapyrusHasControl["LArm_Finger22"] = false;
			handPapyrusHasControl["LArm_Finger23"] = false;
			handPapyrusHasControl["LArm_Finger31"] = false;
			handPapyrusHasControl["LArm_Finger32"] = false;
			handPapyrusHasControl["LArm_Finger33"] = false;
			handPapyrusHasControl["LArm_Finger41"] = false;
			handPapyrusHasControl["LArm_Finger42"] = false;
			handPapyrusHasControl["LArm_Finger43"] = false;
			handPapyrusHasControl["LArm_Finger51"] = false;
			handPapyrusHasControl["LArm_Finger52"] = false;
			handPapyrusHasControl["LArm_Finger53"] = false;
		}
		else {
			handPapyrusHasControl["RArm_Finger11"] = false;
			handPapyrusHasControl["RArm_Finger12"] = false;
			handPapyrusHasControl["RArm_Finger13"] = false;
			handPapyrusHasControl["RArm_Finger21"] = false;
			handPapyrusHasControl["RArm_Finger22"] = false;
			handPapyrusHasControl["RArm_Finger23"] = false;
			handPapyrusHasControl["RArm_Finger31"] = false;
			handPapyrusHasControl["RArm_Finger32"] = false;
			handPapyrusHasControl["RArm_Finger33"] = false;
			handPapyrusHasControl["RArm_Finger41"] = false;
			handPapyrusHasControl["RArm_Finger42"] = false;
			handPapyrusHasControl["RArm_Finger43"] = false;
			handPapyrusHasControl["RArm_Finger51"] = false;
			handPapyrusHasControl["RArm_Finger52"] = false;
			handPapyrusHasControl["RArm_Finger53"] = false;
		}
	}

	static void calibratePlayerHeightAndArms(StaticFunctionTag* base) {
		_MESSAGE("Calibrate player height...");
		g_configurationMode->calibratePlayerHeightAndArms();
	}

	static void openMainConfigurationMode(StaticFunctionTag* base) {
		_MESSAGE("Open Main Configuration Mode...");
		g_configurationMode->enterConfigurationMode();
	}

	static void openPipboyConfigurationMode(StaticFunctionTag* base) {
		_MESSAGE("Open Pipboy Configuration Mode...");
		g_configurationMode->openPipboyConfigurationMode();
	}

	static void openFrikIniFile(StaticFunctionTag* base) {
		_MESSAGE("Open FRIK.ini file in notepad...");
		g_config->OpenInNotepad();
	}

	static UInt32 getFrikIniAutoReloading(StaticFunctionTag* base) {
		return g_config->getAutoReloadConfigInterval() > 0 ? 1 : 0;
	}

	static UInt32 toggleReloadFrikIniConfig(StaticFunctionTag* base) {
		_MESSAGE("Toggle reload FRIK.ini config file...");
		g_config->toggleAutoReloadConfig();
		return getFrikIniAutoReloading(base);
	}

	static UInt32 getWeaponRepositionMode(StaticFunctionTag* base) {
		return c_weaponRepositionMasterMode ? 1 : 0;
	}

	static UInt32 toggleWeaponRepositionMode(StaticFunctionTag* base) {
		_MESSAGE("Toggle Weapon Reposition Mode: %s", !c_weaponRepositionMasterMode ? "ON" : "OFF");
		c_weaponRepositionMasterMode = !c_weaponRepositionMasterMode;
		return getWeaponRepositionMode(base);
	}

	static bool isLeftHandedMode(StaticFunctionTag* base) {
		return *Offsets::iniLeftHandedMode;
	}

	static void setSelfieMode(StaticFunctionTag* base, bool isSelfieMode) {
		_MESSAGE("Set Selfie Mode: %s", isSelfieMode ? "ON" : "OFF");
		c_selfieMode = isSelfieMode;
	}

	static void toggleSelfieMode(StaticFunctionTag* base) {
		setSelfieMode(base, !c_selfieMode);
	}
	
	static void setDynamicCameraHeight(StaticFunctionTag* base, float dynamicCameraHeight) {
		_MESSAGE("Set Dynamic Camera Height: %f", dynamicCameraHeight);
		c_dynamicCameraHeight = dynamicCameraHeight;
	}

	// Sphere bone detection funcs
	static UInt32 registerBoneSphere(StaticFunctionTag* base, float radius, BSFixedString bone) {
		return g_boneSpheres->registerBoneSphere(radius, bone);
	}

	static UInt32 registerBoneSphereOffset(StaticFunctionTag* base, float radius, BSFixedString bone, VMArray<float> pos) {
		return g_boneSpheres->registerBoneSphereOffset(radius, bone, pos);
	}

	static void destroyBoneSphere(StaticFunctionTag* base, UInt32 handle) {
		g_boneSpheres->destroyBoneSphere(handle);
	}

	static void registerForBoneSphereEvents(StaticFunctionTag* base, VMObject* thisObject) {
		g_boneSpheres->registerForBoneSphereEvents(thisObject);
	}

	static void unRegisterForBoneSphereEvents(StaticFunctionTag* base, VMObject* thisObject) {
		g_boneSpheres->unRegisterForBoneSphereEvents(thisObject);
	}

	static void toggleDebugBoneSpheres(StaticFunctionTag* base, bool turnOn) {
		g_boneSpheres->toggleDebugBoneSpheres(turnOn);
	}

	static void toggleDebugBoneSpheresAtBone(StaticFunctionTag* base, UInt32 handle, bool turnOn) {
		g_boneSpheres->toggleDebugBoneSpheresAtBone(handle, turnOn);
	}

	/// <summary>
	/// Register code for Papyrus scripts.
	/// </summary>
	bool registerPapyrusFuncs(VirtualMachine* vm) {
		// Register code to be accisible from Settings Holotabe via Papyrus scripts
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("Calibrate", "FRIK:FRIK", F4VRBody::calibratePlayerHeightAndArms, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("OpenMainConfigurationMode", "FRIK:FRIK", F4VRBody::openMainConfigurationMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("OpenPipboyConfigurationMode", "FRIK:FRIK", F4VRBody::openPipboyConfigurationMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("ToggleWeaponRepositionMode", "FRIK:FRIK", F4VRBody::toggleWeaponRepositionMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("OpenFrikIniFile", "FRIK:FRIK", F4VRBody::openFrikIniFile, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("ToggleReloadFrikIniConfig", "FRIK:FRIK", F4VRBody::toggleReloadFrikIniConfig, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("GetWeaponRepositionMode", "FRIK:FRIK", F4VRBody::getWeaponRepositionMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, UInt32>("GetFrikIniAutoReloading", "FRIK:FRIK", F4VRBody::getFrikIniAutoReloading, vm));

		/// Register mod public API to be used by other mods via Papyrus scripts
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("isLeftHandedMode", "FRIK:FRIK", F4VRBody::isLeftHandedMode, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, float>("setDynamicCameraHeight", "FRIK:FRIK", F4VRBody::setDynamicCameraHeight, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("toggleSelfieMode", "FRIK:FRIK", F4VRBody::toggleSelfieMode, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("setSelfieMode", "FRIK:FRIK", F4VRBody::setSelfieMode, vm));
		
		// Bone Sphere interaction related APIs
		vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, float, BSFixedString>("RegisterBoneSphere", "FRIK:FRIK", F4VRBody::registerBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, float, BSFixedString, VMArray<float> >("RegisterBoneSphereOffset", "FRIK:FRIK", F4VRBody::registerBoneSphereOffset, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, UInt32>("DestroyBoneSphere", "FRIK:FRIK", F4VRBody::destroyBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMObject*>("RegisterForBoneSphereEvents", "FRIK:FRIK", F4VRBody::registerForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMObject*>("UnRegisterForBoneSphereEvents", "FRIK:FRIK", F4VRBody::unRegisterForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("toggleDebugBoneSpheres", "FRIK:FRIK", F4VRBody::toggleDebugBoneSpheres, vm));
		vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, UInt32, bool>("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", F4VRBody::toggleDebugBoneSpheresAtBone, vm));
		
		// Finger pose related APIs
		vm->RegisterFunction(new NativeFunction6<StaticFunctionTag, void, bool, float, float, float, float, float>("setFingerPositionScalar", "FRIK:FRIK", F4VRBody::setFingerPositionScalar, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("restoreFingerPoseControl", "FRIK:FRIK", F4VRBody::restoreFingerPoseControl, vm));

		return true;
	}
}