#include "F4VRBody.h"
#include "Config.h"
#include "ConfigurationMode.h"
#include "Skeleton.h"
#include "Pipboy.h"
#include "HandPose.h"
#include "utils.h"
#include "MuzzleFlash.h"
#include "BSFlattenedBoneTree.h"
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
	bool _controlSleepStickyX = false;
	bool _controlSleepStickyY = false;
	bool _controlSleepStickyT = false;
	bool c_weaponRepositionMasterMode = false;

	std::map<std::string, NiTransform, CaseInsensitiveComparator> handClosed;
	std::map<std::string, NiTransform, CaseInsensitiveComparator> handOpen;

	std::map<std::string, float> handPapyrusPose;
	std::map<std::string, bool> handPapyrusHasControl;

	UInt32 PipboyAA = 0x0001ED3D;
	UInt32 PipboyArmor = 0x00021B3B;
	UInt32 MiningHelmet = 0x0022DD1F;
	UInt32 PALightKW = 0x000B34A6;

	// Papyrus

	const char* boneSphereEventName = "OnBoneSphereEvent";
	RegistrationSetHolder<NullParameters>      g_boneSphereEventRegs;

	std::map<UInt32, BoneSphere*> boneSphereRegisteredObjects;
	UInt32 nextBoneSphereHandle;
	UInt32 curDevice;

	bool isHideSlotLoaded = false;

	bool bDumpArray = false;

	static void adjustSlotVisibility(int slotId, bool isHidden) {
		auto& slot = (*g_player)->equipData->slots[slotId];

		if (slot.item == nullptr)
			return;

		auto form_type = slot.item->GetFormType();
		if (form_type != FormType::kFormType_ARMO)
			return;

		if (slot.node != nullptr) {
			if (isHidden)
				slot.node->flags |= 0x1;
			else
				slot.node->flags &= 0xfffffffffffffffe; // show
		}
	}

	void restoreGeometry() {
		//Face and Skin
		BSFadeNode* rn = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode);

		if (!rn) {
			return;
		}

		for (auto i = 0; i < rn->kGeomArray.count; ++i) {
			rn->kGeomArray[i].spGeometry->flags &= 0xfffffffffffffffe; // show
		}

		//Equipment
		adjustSlotVisibility(0, false);
		adjustSlotVisibility(1, false);
		adjustSlotVisibility(2, false);
		adjustSlotVisibility(16, false);
		adjustSlotVisibility(17, false);
		adjustSlotVisibility(18, false);
		adjustSlotVisibility(19, false);
		adjustSlotVisibility(20, false);
		adjustSlotVisibility(22, false);
	}

	// cull items in skins/faces cull list

	static void cullGeometry() {
		if (c_selfieMode && g_config->selfieIgnoreHideFlags) {
			restoreGeometry();
			return;
		}

		BSFadeNode* rn = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode);
		if (!rn) {
			return;
		}

		if (g_config->hideHead || g_config->hideSkin || c_selfieMode) {
			for (auto i = 0; i < rn->kGeomArray.count; ++i) {
				bool hide = false;
				auto& geometry = rn->kGeomArray[i].spGeometry;
				auto geometryName = geometry->m_name.c_str();
				auto geomStr = trim(str_tolower(std::string(geometryName)));

				if (g_config->hideHead) {
					for (auto& faceGeom : g_config->faceGeometry) {
						if (geomStr.find(faceGeom) != std::string::npos) {
							//_MESSAGE("Found %s in %s", faceGeom, geomStr.c_str());
							hide = true;
							break;
						}
					}
				}

				if (g_config->hideSkin && !hide) {
					for (auto& skinGeom : g_config->skinGeometry) {
						if (geomStr.find(skinGeom) != std::string::npos) {
							hide = true;
							break;
						}
					}
				}

				if (hide)
					geometry->flags |= 0x1; // hide
				else
					geometry->flags &= 0xfffffffffffffffe; // show
			}
		}

		if (g_config->hideEquipment) {
			for (auto slot : g_config->hideEquipSlotIndexes) {
				adjustSlotVisibility(slot, true);
			}
		}
	}

	static void dumpGeometryArrayInUpdate() {

		if (!bDumpArray) {
			return;
		}

		BSFadeNode* rn = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode);

		if (!rn) {
			return;
		}

		for (auto i = 0; i < rn->kGeomArray.count; ++i) {
			auto& geometry = rn->kGeomArray[i].spGeometry;
			auto geometryName = geometry->m_name.c_str();
			_MESSAGE("%s hidden: %d", geometryName, geometry->flags &= 0x1);
		}

		bDumpArray = false;

	}



	// Bone sphere detection

	void detectBoneSphere() {

		if ((*g_player)->firstPersonSkeleton == nullptr) {
			return;
		}

		// prefer to use fingers but these aren't always rendered.    so default to hand if nothing else

		NiAVObject* rFinger = getChildNode("RArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		NiAVObject* lFinger = getChildNode("LArm_Finger22", (*g_player)->firstPersonSkeleton->GetAsNiNode());

		if (rFinger == nullptr) {
			rFinger = getChildNode("RArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		}

		if (lFinger == nullptr) {
			lFinger = getChildNode("LArm_Hand", (*g_player)->firstPersonSkeleton->GetAsNiNode());
		}

		if ((lFinger == nullptr) || (rFinger == nullptr)) {
			return;
		}

		NiPoint3 offset;

		for (auto const& element : boneSphereRegisteredObjects) {
			offset = element.second->bone->m_worldTransform.rot * element.second->offset;
			offset = element.second->bone->m_worldTransform.pos + offset;

			double dist = (double)vec3_len(rFinger->m_worldTransform.pos - offset);

			if (dist <= ((double)element.second->radius - 0.1)) {
				if (!element.second->stickyRight) {
					element.second->stickyRight = true;

					SInt32 evt = BoneSphereEvent_Enter;
					UInt32 handle = element.first;
					UInt32 device = 1;
					curDevice = device;

					if (g_boneSphereEventRegs.m_data.size() > 0) {
						g_boneSphereEventRegs.ForEach(
							[&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
								SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, boneSphereEventName, evt, handle, device);
							}
						);
					}
				}
			}
			else if (dist >= ((double)element.second->radius + 0.1)) {
				if (element.second->stickyRight) {
					element.second->stickyRight = false;

					SInt32 evt = BoneSphereEvent_Exit;
					UInt32 handle = element.first;
					UInt32 device = 1;
					curDevice = 0;

					if (g_boneSphereEventRegs.m_data.size() > 0) {
						g_boneSphereEventRegs.ForEach(
							[&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
								SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, boneSphereEventName, evt, handle, device);
							}
						);
					}
				}
			}

			dist = (double)vec3_len(lFinger->m_worldTransform.pos - offset);

			if (dist <= ((double)element.second->radius - 0.1)) {
				if (!element.second->stickyLeft) {
					element.second->stickyLeft = true;

					SInt32 evt = BoneSphereEvent_Enter;
					UInt32 handle = element.first;
					UInt32 device = 2;
					curDevice = device;

					if (g_boneSphereEventRegs.m_data.size() > 0) {
						g_boneSphereEventRegs.ForEach(
							[&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
								SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, boneSphereEventName, evt, handle, device);
							}
						);
					}
				}
			}
			else if (dist >= ((double)element.second->radius + 0.1)) {
				if (element.second->stickyLeft) {
					element.second->stickyLeft = false;

					SInt32 evt = BoneSphereEvent_Exit;
					UInt32 handle = element.first;
					UInt32 device = 2;
					curDevice = 0;

					if (g_boneSphereEventRegs.m_data.size() > 0) {
						g_boneSphereEventRegs.ForEach(
							[&evt, &handle, &device](const EventRegistration<NullParameters>& reg) {
								SendPapyrusEvent3<SInt32, UInt32, UInt32>(reg.handle, reg.scriptName, boneSphereEventName, evt, handle, device);
							}
						);
					}
				}
			}
		}
	}

	void handleDebugBoneSpheres() {

		for (auto const& element : boneSphereRegisteredObjects) {
			NiNode* bone = element.second->bone;
			NiNode* sphere = element.second->debugSphere;

			if (element.second->turnOnDebugSpheres && !element.second->debugSphere) {
				NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/1x1Sphere.nif");
				NiCloneProcess proc;
				proc.unk18 = Offsets::cloneAddr1;
				proc.unk48 = Offsets::cloneAddr2;

				sphere = Offsets::cloneNode(retNode, &proc);
				if (sphere) {
					sphere->m_name = BSFixedString("Sphere01");

					bone->AttachChild((NiAVObject*)sphere, true);
					sphere->flags &= 0xfffffffffffffffe;
					sphere->m_localTransform.scale = (element.second->radius * 2);
					element.second->debugSphere = sphere;
				}
			}
			else if (sphere && !element.second->turnOnDebugSpheres) {
				sphere->flags |= 0x1;
				sphere->m_localTransform.scale = 0;
			}
			else if (sphere && element.second->turnOnDebugSpheres) {
				sphere->flags &= 0xfffffffffffffffe;
				sphere->m_localTransform.scale = (element.second->radius * 2);
			}

			if (sphere) {
				NiPoint3 offset;

				offset = bone->m_worldTransform.rot * element.second->offset;
				offset = bone->m_worldTransform.pos + offset;

				// wp = parWp + parWr * lp =>   lp = (wp - parWp) * parWr'
				sphere->m_localTransform.pos = bone->m_worldTransform.rot.Transpose() * (offset - bone->m_worldTransform.pos);
			}
		}

	}

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

	bool setSkelly(bool inPowerArmor) {

		_DMESSAGE("setSkelly Start");
		if (!(*g_player)->unkF0) {
			_DMESSAGE("loaded Data Not Set Yet");
			return false;
		}

		_MESSAGE("loadedData = %016I64X", (*g_player)->unkF0);
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

			initPipboy(_skelly, _vrhook);

			turnPipBoyOff();

			initConfigurationMode(_skelly, _vrhook);

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

		if (!inPowerArmorSticky) {
			inPowerArmorSticky = detectInPowerArmor();

			if (inPowerArmorSticky) {
				delete _skelly;
				firstTime = true;
				return;
			}
		}
		else {
			inPowerArmorSticky = detectInPowerArmor();

			if (!inPowerArmorSticky) {
				delete _skelly;
				firstTime = true;
				return;
			}
		}


		if (!_skelly || firstTime) {
			if (!setSkelly(inPowerArmorSticky)) {
				return;
			}

			//	StackPtr<BSAnimationManager*> manager;

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

		cullGeometry();

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
		detectBoneSphere();
		handleDebugBoneSpheres();
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

		dumpGeometryArrayInUpdate();

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
	}

	void startUp() {
		_MESSAGE("Starting up F4Body");
		isLoaded = true;
		nextBoneSphereHandle = 1;
		curDevice = 0;
		scopeMenuEvent.Register();
		return;
	}


	// Papyrus Native Funcs

	void holsterWeapon() { // Sends Papyrus Event to holster weapon when inside of Pipboy usage zone
		SInt32 evt = BoneSphereEvent_Holster;
		if (g_boneSphereEventRegs.m_data.size() > 0) {
			g_boneSphereEventRegs.ForEach(
				[&evt](const EventRegistration<NullParameters>& reg) {
					SendPapyrusEvent1<SInt32>(reg.handle, reg.scriptName, boneSphereEventName, evt);
				}
			);
		}
	}

	void drawWeapon() {  // Sends Papyrus to draw weapon when outside of Pipboy usage zone
		SInt32 evt = BoneSphereEvent_Draw;
		if (g_boneSphereEventRegs.m_data.size() > 0) {
			g_boneSphereEventRegs.ForEach(
				[&evt](const EventRegistration<NullParameters>& reg) {
					SendPapyrusEvent1<SInt32>(reg.handle, reg.scriptName, boneSphereEventName, evt);
				}
			);
		}
	}

	void saveStates(StaticFunctionTag* base) {
		g_config->save();
	}

	void setFingerPositionScalar(StaticFunctionTag* base, bool isLeft, float thumb, float index, float middle, float ring, float pinky) {
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

	void restoreFingerPoseControl(StaticFunctionTag* base, bool isLeft) {
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

	void calibrate(StaticFunctionTag* base) {

		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		Sleep(2000);
		PlayerNodes* pn = (PlayerNodes*)((char*)(*g_player) + 0x6E0);

		 g_config->playerHeight = pn->UprightHmdNode->m_localTransform.pos.z;

		_MESSAGE("Calibrated Height: %f  arm length: %f %f", g_config->playerHeight, g_config->armLength);
		ShowMessagebox("FRIK Config Mode");
	}

	void togglePAHUD(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->showPAHUD = !g_config->showPAHUD;
	}

	void toggleHeadVis(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}
		 g_config->hideHead = !g_config->hideHead;
	}

	void togglePipboyVis(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->hidePipboy = !g_config->hidePipboy;

	}

	void toggleSelfieMode(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 c_selfieMode = !c_selfieMode;
	}

	static void setSelfieMode(StaticFunctionTag* base, bool isSelfieMode) {
		if (c_selfieMode == isSelfieMode)
			return;

		toggleSelfieMode(base);
	}

	void toggleArmsOnlyMode(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->armsOnly = !g_config->armsOnly;
	}

	void toggleStaticGripping(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->staticGripping = !g_config->staticGripping;

		bool gripConfig = !g_config->staticGripping;
		g_messaging->Dispatch(g_pluginHandle, 15, (void*)gripConfig, sizeof(bool), "FO4VRBETTERSCOPES");
	}

	bool isLeftHandedMode(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		return *Offsets::iniLeftHandedMode;
	}


	void moveCameraUp(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->cameraHeight += 2.0f;
	}

	void moveCameraDown(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->cameraHeight -= 2.0f;
	}

	void setDynamicCameraHeight(StaticFunctionTag* base, float dynamicCameraHeight) {
		 c_dynamicCameraHeight = dynamicCameraHeight;
	}

	void makeTaller(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->playerHeight += 2.0f;
	}

	void makeShorter(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->playerHeight -= 2.0f;
	}

	void moveUp(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->playerOffset_up += 1.0f;
	}

	void moveDown(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->playerOffset_up -= 1.0f;
	}

	void moveForward(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->playerOffset_forward += 1.0f;
	}

	void moveBackward(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->playerOffset_forward -= 1.0f;
	}

	void increaseScale(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->fVrScale += 1.0f;
		Setting* set = GetINISetting("fVrScale:VR");
		set->SetDouble(g_config->fVrScale);
	}

	void decreaseScale(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->fVrScale -= 1.0f;
		Setting* set = GetINISetting("fVrScale:VR");
		set->SetDouble(g_config->fVrScale);
	}

	void handUiXUp(StaticFunctionTag* base) {
		 g_config->handUI_X += 1.0f;
	}

	void handUiXDown(StaticFunctionTag* base) {
		 g_config->handUI_X -= 1.0f;
	}

	void handUiYUp(StaticFunctionTag* base) {
		 g_config->handUI_Y += 1.0f;
	}

	void handUiYDown(StaticFunctionTag* base) {
		 g_config->handUI_Y -= 1.0f;
	}

	void handUiZUp(StaticFunctionTag* base) {
		 g_config->handUI_Z += 1.0f;
	}

	void handUiZDown(StaticFunctionTag* base) {
		 g_config->handUI_Z -= 1.0f;
	}

	// Sphere bone detection funcs

	UInt32 RegisterBoneSphere(StaticFunctionTag* base, float radius, BSFixedString bone) {
		if (radius == 0.0) {
			return 0;
		}

		NiNode* boneNode = getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode)->GetAsNiNode();

		if (!boneNode) {
			_MESSAGE("RegisterBoneSphere: BONE DOES NOT EXIST!!");
			return 0;
		}

		BoneSphere* sphere = new BoneSphere(radius, boneNode, NiPoint3(0,0,0));
		UInt32 handle = nextBoneSphereHandle++;

		boneSphereRegisteredObjects[handle] = sphere;

		return handle;
	}

	UInt32 RegisterBoneSphereOffset(StaticFunctionTag* base, float radius, BSFixedString bone, VMArray<float> pos) {
		if (radius == 0.0) {
			return 0;
		}

		if (pos.Length() != 3) {
			return 0;
		}

		if (!(*g_player)->unkF0) {
			_MESSAGE("can't register yet as new game");
			return 0;
		}

		NiNode* boneNode = (NiNode*)getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode);

		if (!boneNode) {

			auto n = (*g_player)->unkF0->rootNode->GetAsNiNode();

			while (n->m_parent) {
				n = n->m_parent->GetAsNiNode();
			}

			boneNode = getChildNode(bone.c_str(), n);  // ObjectLODRoot

			if (!boneNode) {
				_MESSAGE("RegisterBoneSphere: BONE DOES NOT EXIST!!");
				return 0;
			}
		}

		NiPoint3 offsetVec;

		pos.Get(&(offsetVec.x), 0);
		pos.Get(&(offsetVec.y), 1);
		pos.Get(&(offsetVec.z), 2);


		BoneSphere* sphere = new BoneSphere(radius, boneNode, offsetVec);
		UInt32 handle = nextBoneSphereHandle++;

		boneSphereRegisteredObjects[handle] = sphere;

		return handle;
	}

	void DestroyBoneSphere(StaticFunctionTag* base, UInt32 handle) {
		if (boneSphereRegisteredObjects.count(handle)) {
			NiNode* sphere = boneSphereRegisteredObjects[handle]->debugSphere;

			if (sphere) {
				sphere->flags |= 0x1;
				sphere->m_localTransform.scale = 0;
				sphere->m_parent->RemoveChild(sphere);
			}

			delete boneSphereRegisteredObjects[handle];
			boneSphereRegisteredObjects.erase(handle);
		}
	}

	void RegisterForBoneSphereEvents(StaticFunctionTag* base, VMObject* thisObject) {
		_MESSAGE("RegisterForBoneSphereEvents");
		if (!thisObject) {
			return;
		}

		g_boneSphereEventRegs.Register(thisObject->GetHandle(), thisObject->GetObjectType());
	}

	void UnRegisterForBoneSphereEvents(StaticFunctionTag* base, VMObject* thisObject) {
		if (!thisObject) {
			return;
		}

		_MESSAGE("UnRegisterForBoneSphereEvents");
		g_boneSphereEventRegs.Unregister(thisObject->GetHandle(), thisObject->GetObjectType());
	}

	void toggleDebugBoneSpheres(StaticFunctionTag* base, bool turnOn) {
		for (auto const& element : boneSphereRegisteredObjects) {
			element.second->turnOnDebugSpheres = turnOn;
		}
	}

	void toggleDebugBoneSpheresAtBone(StaticFunctionTag* base, UInt32 handle, bool turnOn) {
		if (boneSphereRegisteredObjects.count(handle)) {
			boneSphereRegisteredObjects[handle]->turnOnDebugSpheres = turnOn;
		}
	}

	void toggleDampenHands(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->dampenHands = !g_config->dampenHands;
	}

	void increaseDampenRotation(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->dampenHandsRotation += 0.05f;
		 g_config->dampenHandsRotation = g_config->dampenHandsRotation >= 1.0f ? 0.95f : g_config->dampenHandsRotation;
	}

	void decreaseDampenRotation(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->dampenHandsRotation -= 0.05f;
		 g_config->dampenHandsRotation = g_config->dampenHandsRotation <= 0.0f ? 0.05f : g_config->dampenHandsRotation;
	}

	void increaseDampenTranslation(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->dampenHandsTranslation += 0.05f;

		 g_config->dampenHandsTranslation = g_config->dampenHandsTranslation >= 1.0f ? 0.95f : g_config->dampenHandsTranslation;
	}

	void decreaseDampenTranslation(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 g_config->dampenHandsTranslation -= 0.05f;
		 g_config->dampenHandsTranslation = g_config->dampenHandsTranslation <= 0.0f ? 0.5f : g_config->dampenHandsTranslation;
	}

	void toggleRepositionMasterMode(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		 c_weaponRepositionMasterMode = !c_weaponRepositionMasterMode;
	}

	void dumpGeometryArray(StaticFunctionTag* base) {

		bDumpArray = true;
	}


	bool RegisterFuncs(VirtualMachine* vm) {

		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("saveStates", "FRIK:FRIK", F4VRBody::saveStates, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("Calibrate", "FRIK:FRIK", F4VRBody::calibrate, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("togglePAHud", "FRIK:FRIK", F4VRBody::togglePAHUD, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("toggleHeadVis", "FRIK:FRIK", F4VRBody::toggleHeadVis, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("togglePipboyVis", "FRIK:FRIK", F4VRBody::togglePipboyVis, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("toggleSelfieMode", "FRIK:FRIK", F4VRBody::toggleSelfieMode, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("setSelfieMode", "FRIK:FRIK", F4VRBody::setSelfieMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("toggleArmsOnlyMode", "FRIK:FRIK", F4VRBody::toggleArmsOnlyMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("toggleStaticGripping", "FRIK:FRIK", F4VRBody::toggleStaticGripping, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>("isLeftHandedMode", "FRIK:FRIK", F4VRBody::isLeftHandedMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveCameraUp", "FRIK:FRIK", F4VRBody::moveCameraUp, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveCameraDown", "FRIK:FRIK", F4VRBody::moveCameraDown, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, float>("setDynamicCameraHeight", "FRIK:FRIK", F4VRBody::setDynamicCameraHeight, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("makeTaller", "FRIK:FRIK", F4VRBody::makeTaller, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("makeShorter", "FRIK:FRIK", F4VRBody::makeShorter, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveUp", "FRIK:FRIK", F4VRBody::moveUp, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveDown", "FRIK:FRIK", F4VRBody::moveDown, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveForward", "FRIK:FRIK", F4VRBody::moveForward, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveBackward", "FRIK:FRIK", F4VRBody::moveBackward, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("increaseScale", "FRIK:FRIK", F4VRBody::increaseScale, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("decreaseScale", "FRIK:FRIK", F4VRBody::decreaseScale, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("toggleDampenHands", "FRIK:FRIK", F4VRBody::toggleDampenHands, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("increaseDampenRotation", "FRIK:FRIK", F4VRBody::increaseDampenRotation, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("decreaseDampenRotation", "FRIK:FRIK", F4VRBody::decreaseDampenRotation, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("increaseDampenTranslation", "FRIK:FRIK", F4VRBody::increaseDampenTranslation, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("decreaseDampenTranslation", "FRIK:FRIK", F4VRBody::decreaseDampenTranslation, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("toggleRepositionMasterMode", "FRIK:FRIK", F4VRBody::toggleRepositionMasterMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("handUiXUp", "FRIK:FRIK", F4VRBody::handUiXUp, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("handUiXDown", "FRIK:FRIK", F4VRBody::handUiXDown, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("handUiYUp", "FRIK:FRIK", F4VRBody::handUiYUp, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("handUiYDown", "FRIK:FRIK", F4VRBody::handUiYDown, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("handUiZUp", "FRIK:FRIK", F4VRBody::handUiZUp, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("handUiZDown", "FRIK:FRIK", F4VRBody::handUiZDown, vm));
		vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, float, BSFixedString>("RegisterBoneSphere", "FRIK:FRIK", F4VRBody::RegisterBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, float, BSFixedString, VMArray<float> >("RegisterBoneSphereOffset", "FRIK:FRIK", F4VRBody::RegisterBoneSphereOffset, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, UInt32>("DestroyBoneSphere", "FRIK:FRIK", F4VRBody::DestroyBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMObject*>("RegisterForBoneSphereEvents", "FRIK:FRIK", F4VRBody::RegisterForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMObject*>("UnRegisterForBoneSphereEvents", "FRIK:FRIK", F4VRBody::UnRegisterForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("toggleDebugBoneSpheres", "FRIK:FRIK", F4VRBody::toggleDebugBoneSpheres, vm));
		vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, UInt32, bool>("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", F4VRBody::toggleDebugBoneSpheresAtBone, vm));
		vm->RegisterFunction(new NativeFunction6<StaticFunctionTag, void, bool, float, float, float, float, float>("setFingerPositionScalar", "FRIK:FRIK", F4VRBody::setFingerPositionScalar, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("restoreFingerPoseControl", "FRIK:FRIK", F4VRBody::restoreFingerPoseControl, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("dumpGeometryArray", "FRIK:FRIK", F4VRBody::dumpGeometryArray, vm));

		return true;
	}

}