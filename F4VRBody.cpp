#include "F4VRBody.h"
#include "Skeleton.h"

#include "f4se/GameSettings.h"
#include "f4se/GameMenus.h"


#define PI 3.14159265358979323846

bool firstTime = true;

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

	// Papyrus

	const char* boneSphereEventName = "OnBoneSphereEvent";
	RegistrationSetHolder<NullParameters>      g_boneSphereEventRegs;
	
	std::map<UInt32, BoneSphere*> boneSphereRegisteredObjects;
	UInt32 nextBoneSphereHandle;
	UInt32 curDevice;

	bool loadConfig() {
		//INIReader ini(".\\Data\\F4SE\\plugins\\Fallout4VR_Body.ini");

		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\Fallout4VR_Body.ini");

		if (rc < 0) {
			_MESSAGE("ERROR: cannot read Fallout4VR_Body.ini");
			return false;
		}

		c_playerHeight =         (float) ini.GetDoubleValue("Fallout4VRBody", "PlayerHeight", 120.4828f);
		c_setScale     =         ini.GetBoolValue("Fallout4VRBody", "setScale", false);
		c_fVrScale     =         (float) ini.GetDoubleValue("Fallout4VRBody", "fVrScale", 70.0);
		c_playerOffset_forward = (float) ini.GetDoubleValue("Fallout4VRBody", "playerOffset_forward", -4.0);
		c_playerOffset_up =      (float) ini.GetDoubleValue("Fallout4VRBody", "playerOffset_up", -2.0);
		c_pipboyDetectionRange = (float) ini.GetDoubleValue("Fallout4VRBody", "pipboyDetectionRange", 15.0);
		c_armLength =            (float) ini.GetDoubleValue("FalloutVRBody", "armLength", 36.74);
		c_cameraHeight =         (float)ini.GetDoubleValue("FalloutVRBody", "cameraHeightOffset", 0.0);
		c_showPAHUD =            ini.GetBoolValue("FalloutVRBody", "showPAHUD");

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
			offset = element.second->bone->m_worldTransform.pos + element.second->offset;

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

	bool setSkelly() {
		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			playerSkelly = new Skeleton((BSFadeNode*)(*g_player)->unkF0->rootNode->m_children.m_data[0]->GetAsNiNode());
			_MESSAGE("skeleton = %016I64X", playerSkelly->getRoot());
			playerSkelly->setNodes();
			playerSkelly->setDirection();
		    playerSkelly->swapPipboy();

			if (c_setScale) {
				Setting* set = GetINISetting("fVrScale:VR");
				set->SetDouble(c_fVrScale);
			}

			playerSkelly->setBodyLen();
			_MESSAGE("initialized");
			return true;
		}
		else {
			return false;
		}
	}

	void update() {


		//if (prevCounter != g_mainLoopCounter) {
		//	prevCounter = g_mainLoopCounter;
		//	localCounter = 0;
		//	return;
		//}
		//else if (localCounter != 3) {
		//	localCounter++;
		//	return;
		//}

	//	localCounter++;
//		updateCounter++;
		//_MESSAGE("mainLoopCounter = %d; UpdateCounter = %d", g_mainLoopCounter, updateCounter);
	//	_MESSAGE("start of update");

		if (!isLoaded) {
			return;
		}

		if (!playerSkelly) {
			if (firstTime) {
				if (!setSkelly()) {
					return;
				}
			}

			firstTime = false;
			return;
		}

		if ((*g_player)->firstPersonSkeleton == nullptr) {
			return;
		}

		// do stuff now

		playerSkelly->hideWands();

		//_MESSAGE("start update");

		// first restore locals to a default state to wipe out any local transform changes the game might have made since last update
		playerSkelly->restoreLocals(playerSkelly->getRoot());
		playerSkelly->updateDown(playerSkelly->getRoot(), true);

		//_MESSAGE("locals saved");

		//playerSkelly->printNodes();

		// moves head up and back out of the player view.   doing this instead of hiding with a small scale setting since it preserves neck shape
		NiNode* headNode = playerSkelly->getNode("Head", playerSkelly->getRoot());
		playerSkelly->setupHead(headNode);

	//	_MESSAGE("head setup");

		//// set up the body underneath the headset in a proper scale and orientation
		playerSkelly->setUnderHMD();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference
		//_MESSAGE("body init");

		// Now Set up body Posture and hook up the legs
		playerSkelly->setBodyPosture();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

	//	_MESSAGE("posture");

		playerSkelly->setLegs();

		// Do another update before setting arms
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

	//	_MESSAGE("legs");
		// do arm IK - Right then Left
		playerSkelly->setArms(false);
		playerSkelly->setArms(true);
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// Misc stuff to show/hide things and also setup the wrist pipboy
		playerSkelly->hideWeapon();
		playerSkelly->positionPipboy();
		playerSkelly->fixMelee();
		playerSkelly->hideFistHelpers();
		playerSkelly->showHidePAHUD();

		playerSkelly->operatePipBoy();

		playerSkelly->fixArmor();

		// project body out in front of the camera for debug purposes
	//	playerSkelly->projectSkelly(120.0f);
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Last world update before exit.    Probably not necessary.

		detectBoneSphere();
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
		rc = ini.SetBoolValue("FalloutVRBody", "showPAHUD", c_showPAHUD);

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

		NiNode* boneNode = getChildNode(bone.c_str(), (*g_player)->unkF0->rootNode)->GetAsNiNode();

		if (!boneNode) {
			_MESSAGE("RegisterBoneSphere: BONE DOES NOT EXIST!!");
			return 0;
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

	bool RegisterFuncs(VirtualMachine* vm) {

		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("saveStates", "FRIK:FRIK", F4VRBody::saveStates, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("Calibrate", "FRIK:FRIK", F4VRBody::calibrate, vm));
		vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("togglePAHud", "FRIK:FRIK", F4VRBody::togglePAHUD, vm));
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

		return true;
	}

}