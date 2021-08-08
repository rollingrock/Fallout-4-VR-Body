#include "F4VRBody.h"
#include "Skeleton.h"




#define PI 3.14159265358979323846

bool firstTime = true;

//Smooth Movement
float smoothingAmount = 10.0f;
float smoothingAmountHorizontal = 0;
float dampingMultiplier = 1.0f;
float dampingMultiplierHorizontal = 0;
float stoppingMultiplier = 0.2f;
float stoppingMultiplierHorizontal = 0.2f;
int disableInteriorSmoothing = 1;
int disableInteriorSmoothingHorizontal = 1;

namespace F4VRBody {

	Skeleton* playerSkelly = nullptr;

	bool isLoaded = false;

	uint64_t updateCounter = 0;
	uint64_t prevCounter = 0;
	uint64_t localCounter = 0;

	float c_playerHeight = 0.0;
	bool  c_setScale = false;
	float c_fVrScale = 70.0;
	float c_playerOffset_forward = -4.0;
	float c_playerOffset_up = -2.0;
	float c_pipboyDetectionRange = 15.0f;
	float c_armLength = 36.74;
	float c_cameraHeight = 0.0;
	bool  c_showPAHUD = true;
	bool  c_hidePipboy = false;
	bool  c_selfieMode = false;
	bool  c_verbose = false;

	bool meshesReplaced = false;



	// loadNif native func
	//typedef int(*_loadNif)(const char* path, uint64_t parentNode, uint64_t flags);
	typedef int(*_loadNif)(uint64_t path, uint64_t mem, uint64_t flags);
	RelocAddr<_loadNif> loadNif(0x1d0dee0);
	//RelocAddr<_loadNif> loadNif(0x1d0dd80);

	typedef NiNode*(*_cloneNode)(NiNode* node, NiCloneProcess* obj);
	RelocAddr<_cloneNode> cloneNode(0x1c13ff0);

	typedef NiNode* (*_addNode)(uint64_t attachNode, NiAVObject* node);
	RelocAddr<_addNode> addNode(0xada20);

	RelocAddr<UInt64*> cloneAddr1(0x36ff560);
	RelocAddr<UInt64*> cloneAddr2(0x36ff564);

	NiNode* loadNifFromFile(char* path) {
		uint64_t flags[2];
		flags[0] = 0x0;
		flags[1] = 0xed | 0x2d;
		uint64_t mem = 0;
		int ret = loadNif((uint64_t)&(*path), (uint64_t)&mem, (uint64_t)&flags);

		return (NiNode*)mem;
	}

	// Papyrus

	const char* boneSphereEventName = "OnBoneSphereEvent";
	RegistrationSetHolder<NullParameters>      g_boneSphereEventRegs;
	
	std::map<UInt32, BoneSphere*> boneSphereRegisteredObjects;
	UInt32 nextBoneSphereHandle;
	UInt32 curDevice;

	bool loadConfig() {
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\FRIK.ini");

		if (rc < 0) {
			_MESSAGE("ERROR: cannot read FRIK.ini");
			return false;
		}

		c_playerHeight =         (float) ini.GetDoubleValue("Fallout4VRBody", "PlayerHeight", 120.4828f);
		c_setScale     =         ini.GetBoolValue("Fallout4VRBody", "setScale", false);
		c_fVrScale     =         (float) ini.GetDoubleValue("Fallout4VRBody", "fVrScale", 70.0);
		c_playerOffset_forward = (float) ini.GetDoubleValue("Fallout4VRBody", "playerOffset_forward", -4.0);
		c_playerOffset_up =      (float) ini.GetDoubleValue("Fallout4VRBody", "playerOffset_up", -2.0);
		c_pipboyDetectionRange = (float) ini.GetDoubleValue("Fallout4VRBody", "pipboyDetectionRange", 15.0);
		c_armLength =            (float) ini.GetDoubleValue("Fallout4VRBody", "armLength", 36.74);
		c_cameraHeight =         (float) ini.GetDoubleValue("Fallout4VRBody", "cameraHeightOffset", 0.0);
		c_showPAHUD =            ini.GetBoolValue("Fallout4VRBody", "showPAHUD");
		c_hidePipboy =           ini.GetBoolValue("Fallout4VRBody", "hidePipboy");
		c_verbose =              ini.GetBoolValue("Fallout4VRBody", "VerboseLogging");
		
		//Smooth Movement
		smoothingAmount                    = (float) ini.GetDoubleValue("SmoothMovementVR", "SmoothAmount", 15.0);
		smoothingAmountHorizontal          = (float) ini.GetDoubleValue("SmoothMovementVR", "SmoothAmountHorizontal", 5.0);
		dampingMultiplier                  = (float) ini.GetDoubleValue("SmoothMovementVR", "Damping", 1.0);
		dampingMultiplierHorizontal        = (float) ini.GetDoubleValue("SmoothMovementVR", "DampingHorizontal", 1.0);
		stoppingMultiplier                 = (float) ini.GetDoubleValue("SmoothMovementVR", "StoppingMultiplier", 0.6);
		stoppingMultiplierHorizontal       = (float) ini.GetDoubleValue("SmoothMovementVR", "StoppingMultiplierHorizontal", 0.6);
		disableInteriorSmoothing           = ini.GetBoolValue("SmoothMovementVR", "DisableInteriorSmoothing", 1);
		disableInteriorSmoothingHorizontal = ini.GetBoolValue("SmoothMovementVR", "DisableInteriorSmoothingHorizontal", 1);
		return true;
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

			float dist = vec3_len(rFinger->m_worldTransform.pos - offset);

			if (dist < element.second->radius) {
				if (element.second->sticky) {
					continue;
				}
				element.second->sticky = true;

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
				continue;
			}

			dist = vec3_len(lFinger->m_worldTransform.pos - offset);

			if (dist < element.second->radius) {
				if (element.second->sticky) {
					continue;
				}
				element.second->sticky = true;

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
				continue;
			}

			if (element.second->sticky) {
				element.second->sticky = false;

				SInt32 evt = BoneSphereEvent_Exit;
				UInt32 handle = element.first;
				UInt32 device = curDevice;
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

	void handleDebugBoneSpheres() {

		for (auto const& element : boneSphereRegisteredObjects) {
			NiNode* bone = element.second->bone;
			NiNode* sphere = element.second->debugSphere;

			if (element.second->turnOnDebugSpheres && !element.second->debugSphere) {
				NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/1x1Sphere.nif");
				NiCloneProcess proc;
				proc.unk18 = cloneAddr1;
				proc.unk48 = cloneAddr2;

				sphere = cloneNode(retNode, &proc);
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

	void replaceMeshes(PlayerNodes* pn) {
		NiNode* ui = pn->primaryUIAttachNode;
		NiNode* wand = get1stChildNode("world_primaryWand.nif", ui);
		NiNode* retNode = loadNifFromFile("Data/Meshes/FRIK/_primaryWand.nif");

		if (retNode) {
//			ui->RemoveChild(wand);
	//		ui->AttachChild(retNode, true);
		}

		wand = pn->SecondaryWandNode;
		NiNode* pipParent = get1stChildNode("PipboyParent", wand);

		if (!pipParent) {
			meshesReplaced = false;
			return;
		}

		wand = get1stChildNode("PipboyRoot_NIF_ONLY", pipParent);
		retNode = loadNifFromFile("Data/Meshes/FRIK/PipboyVR.nif");


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
			NiNode* rn = addNode((uint64_t)&pn->ScreenNode, newScreen);
			pn->PipboyRoot_nif_only_node = retNode;
		}

			meshesReplaced = true;
			_MESSAGE("Meshes replaced!");

	}

	void fixMissingScreen(PlayerNodes* pn) {
		NiNode* screenNode = pn->ScreenNode;

		if (screenNode) {
			BSFixedString screenName("Screen:0");
			NiAVObject* newScreen = screenNode->GetObjectByName(&screenName);

			if (!newScreen) {
				pn->ScreenNode->RemoveChildAt(0);

				newScreen = pn->PipboyRoot_nif_only_node->GetObjectByName(&screenName)->m_parent;
				NiNode* rn = addNode((uint64_t)&pn->ScreenNode, newScreen);
			}
		}
	}

	bool setSkelly() {
		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			_MESSAGE("set root");
			auto node = (BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode();
			if (!node) {
				_MESSAGE("root node not found");
				return false;
			}

			playerSkelly = new Skeleton(node);
			_MESSAGE("skeleton = %016I64X", playerSkelly->getRoot());
			playerSkelly->setNodes();
			//replaceMeshes(playerSkelly->getPlayerNodes());
			//playerSkelly->setDirection();
		    playerSkelly->swapPipboy();

			_MESSAGE("handle pipboy init");

			turnPipBoyOff();


			if (c_setScale) {
				Setting* set = GetINISetting("fVrScale:VR");
				set->SetDouble(c_fVrScale);
			}
			_MESSAGE("scale set");

			playerSkelly->setBodyLen();
			_MESSAGE("initialized");
			return true;
		}
		else {
			return false;
		}
	}

	void update() {



		if (!isLoaded) {
			return;
		}

		if (!playerSkelly || firstTime) {
			if (!setSkelly()) {
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

		if (playerSkelly->getRoot() != (BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode()) {
			auto node = (BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode();
			if (!node) {
				return;
			}

			playerSkelly->updateRoot(node);
			playerSkelly->setNodes();
			playerSkelly->setDirection();
		    playerSkelly->swapPipboy();
			playerSkelly->setBodyLen();
			replaceMeshes(playerSkelly->getPlayerNodes());
			_MESSAGE("initialized for real");
			return;
		}

		// do stuff now

		if (c_verbose) { _MESSAGE("Start of Frame"); }

		if (!meshesReplaced) {
			replaceMeshes(playerSkelly->getPlayerNodes());
		}

		if (c_verbose) { _MESSAGE("Smooth Movement"); }
		SmoothMovementVR::everyFrame();
		updateTransformsDown(playerSkelly->getPlayerNodes()->playerworldnode, true);

		playerSkelly->setTime();

		if (c_verbose) { _MESSAGE("Hide Wands"); }
		playerSkelly->hideWands();

		// first restore locals to a default state to wipe out any local transform changes the game might have made since last update
		if (c_verbose) { _MESSAGE("restore locals of skeleton"); }
		playerSkelly->restoreLocals(playerSkelly->getRoot());
		playerSkelly->updateDown(playerSkelly->getRoot(), true);

		// moves head up and back out of the player view.   doing this instead of hiding with a small scale setting since it preserves neck shape
		if (c_verbose) { _MESSAGE("Setup Head"); }
		NiNode* headNode = playerSkelly->getNode("Head", playerSkelly->getRoot());
		playerSkelly->setupHead(headNode);

		//// set up the body underneath the headset in a proper scale and orientation
		if (c_verbose) { _MESSAGE("Set body under HMD"); }
		playerSkelly->setUnderHMD();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// Now Set up body Posture and hook up the legs
		if (c_verbose) { _MESSAGE("Set body posture"); }
		playerSkelly->setBodyPosture();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		if (c_verbose) { _MESSAGE("Set Knee Posture"); }
		playerSkelly->setKneePos();
		if (c_verbose) { _MESSAGE("Set Walk"); }
		playerSkelly->walk();
		//playerSkelly->setLegs();
		if (c_verbose) { _MESSAGE("Set Legs"); }
		playerSkelly->setSingleLeg(false);
		playerSkelly->setSingleLeg(true);

		// Do another update before setting arms
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// do arm IK - Right then Left
		if (c_verbose) { _MESSAGE("Set Arms"); }
		playerSkelly->setArms(false);
		playerSkelly->setArms(true);
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// Misc stuff to showahide things and also setup the wrist pipboy
		if (c_verbose) { _MESSAGE("Pipboy and Weapons"); }
		playerSkelly->hideWeapon();
		playerSkelly->positionPipboy();
		playerSkelly->hidePipboy();
		playerSkelly->fixMelee();
		playerSkelly->hideFistHelpers();
		playerSkelly->showHidePAHUD();

		if (c_verbose) { _MESSAGE("Operate Pipboy"); }
		playerSkelly->operatePipBoy();

		if (c_verbose) { _MESSAGE("Fix the Armor"); }
		playerSkelly->fixArmor();

		// project body out in front of the camera for debug purposes
		if (c_verbose) { _MESSAGE("Selfie Time"); }
		playerSkelly->selfieSkelly(70.0f);
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Last world update before exit.    Probably not necessary.

		if (c_verbose) { _MESSAGE("bone sphere stuff"); }
		detectBoneSphere();
		handleDebugBoneSpheres();

		if (c_verbose) { _MESSAGE("fix the missing screen"); }
		fixMissingScreen(playerSkelly->getPlayerNodes());

	}


	void startUp() {
		_MESSAGE("Starting up F4Body");
		isLoaded = true;
		nextBoneSphereHandle = 1;
		curDevice = 0;
		return;
	}

	
	// Papyrus Native Funcs

	void saveStates(StaticFunctionTag* base) {
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\Fallout4VR_Body.ini");

		rc = ini.SetDoubleValue("Fallout4VRBody", "PlayerHeight", (double)c_playerHeight);
		rc = ini.SetDoubleValue("Fallout4VRBody", "fVrScale", (double)c_fVrScale);
		rc = ini.SetDoubleValue("Fallout4VRBody", "playerOffset_forward", (double)c_playerOffset_forward);
		rc = ini.SetDoubleValue("Fallout4VRBody", "playerOffset_up", (double)c_playerOffset_up);
		rc = ini.SetDoubleValue("Fallout4VRBody", "armLength", (double)c_armLength);
		rc = ini.SetDoubleValue("Fallout4VRBody", "cameraHeightOffset", (double)c_cameraHeight);
		rc = ini.SetBoolValue("Fallout4VRBody", "showPAHUD", c_showPAHUD);
		rc = ini.SetBoolValue("Fallout4VRBody", "hidePipboy", c_hidePipboy);

		rc = ini.SaveFile(".\\Data\\F4SE\\plugins\\Fallout4VR_Body.ini");

		if (rc < 0) {
			_MESSAGE("Failed to write out INI config file");
		}
		else {
			_MESSAGE("successfully wrote config file");
		}

	}

	void calibrate(StaticFunctionTag* base) {

		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		Sleep(2000);
		PlayerNodes* pn = (PlayerNodes*)((char*)(*g_player) + 0x6E0);

		c_playerHeight = pn->UprightHmdNode->m_localTransform.pos.z;
	//	c_armLength = (vec3_len(pn->primaryWandNode->m_worldTransform.pos - pn->SecondaryWandNode->m_worldTransform.pos) / 2) - 20.0f;

		_MESSAGE("Calibrated Height: %f  arm length: %f", c_playerHeight, c_armLength);
	}

	void togglePAHUD(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_showPAHUD = !c_showPAHUD;
	}
	
	void togglePipboyVis(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_hidePipboy = !c_hidePipboy;

	}

	void toggleSelfieMode(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_selfieMode = !c_selfieMode;
	}


	void moveCameraUp(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_cameraHeight += 2.0f;
	}

	void moveCameraDown(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_cameraHeight -= 2.0f;
	}

	void makeTaller(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_playerHeight += 2.0f;
	}

	void makeShorter(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_playerHeight -= 2.0f;
	}

	void moveUp(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_playerOffset_up += 1.0f;
	}

	void moveDown(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_playerOffset_up -= 1.0f;
	}

	void moveForward(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_playerOffset_forward += 1.0f;
	}

	void moveBackward(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_playerOffset_forward -= 1.0f;
	}

	void increaseScale(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_fVrScale += 1.0f;
		Setting* set = GetINISetting("fVrScale:VR");
		set->SetDouble(c_fVrScale);
	}

	void decreaseScale(StaticFunctionTag* base){ 
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_fVrScale -= 1.0f;
		Setting* set = GetINISetting("fVrScale:VR");
		set->SetDouble(c_fVrScale);
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

	void toggleDebugBoneSpheresAtBone(StaticFunctionTag* base, BSFixedString bone, bool turnOn) {
		for (auto const& element : boneSphereRegisteredObjects) {
			if (!strcmp(bone.c_str(), element.second->bone->m_name.c_str())) {
				element.second->turnOnDebugSpheres = turnOn;
			}
		}
	}

	bool RegisterFuncs(VirtualMachine* vm) {

		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("saveStates", "FRIK:FRIK", F4VRBody::saveStates, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("Calibrate", "FRIK:FRIK", F4VRBody::calibrate, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("togglePAHud", "FRIK:FRIK", F4VRBody::togglePAHUD, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("togglePipboyVis", "FRIK:FRIK", F4VRBody::togglePipboyVis, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("toggleSelfieMode", "FRIK:FRIK", F4VRBody::toggleSelfieMode, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveCameraUp", "FRIK:FRIK", F4VRBody::moveCameraUp, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveCameraDown", "FRIK:FRIK", F4VRBody::moveCameraDown, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("makeTaller", "FRIK:FRIK", F4VRBody::makeTaller, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("makeShorter", "FRIK:FRIK", F4VRBody::makeShorter, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveUp", "FRIK:FRIK", F4VRBody::moveUp, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveDown", "FRIK:FRIK", F4VRBody::moveDown, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveForward", "FRIK:FRIK", F4VRBody::moveForward, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("moveBackward", "FRIK:FRIK", F4VRBody::moveBackward, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("increaseScale", "FRIK:FRIK", F4VRBody::increaseScale, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("decreaseScale", "FRIK:FRIK", F4VRBody::decreaseScale, vm));
		vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, float, BSFixedString>("RegisterBoneSphere", "FRIK:FRIK", F4VRBody::RegisterBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction3<StaticFunctionTag, UInt32, float, BSFixedString, VMArray<float> >("RegisterBoneSphereOffset", "FRIK:FRIK", F4VRBody::RegisterBoneSphereOffset, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, UInt32>("DestroyBoneSphere", "FRIK:FRIK", F4VRBody::DestroyBoneSphere, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMObject*>("RegisterForBoneSphereEvents", "FRIK:FRIK", F4VRBody::RegisterForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMObject*>("UnRegisterForBoneSphereEvents", "FRIK:FRIK", F4VRBody::UnRegisterForBoneSphereEvents, vm));
		vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>("toggleDebugBoneSpheres", "FRIK:FRIK", F4VRBody::toggleDebugBoneSpheres, vm));
		vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, bool>("toggleDebugBoneSpheresAtBone", "FRIK:FRIK", F4VRBody::toggleDebugBoneSpheresAtBone, vm));

		return true;
	}

}