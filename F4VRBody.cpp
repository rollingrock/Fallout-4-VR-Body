#include "F4VRBody.h"
#include "Skeleton.h"
#include "HandPose.h"
#include "weaponOffset.h"
#include "utils.h"
#include "MuzzleFlash.h"
#include "BSFlattenedBoneTree.h"
#include "GunReload.h"
#include "VR.h"

#include "api/PapyrusVRAPI.h"
#include "api/VRManagerAPI.h"

#include <algorithm>

#include "Menu.h"
#include "MiscStructs.h"




#define PI 3.14159265358979323846

bool firstTime = true;
bool printPlayerOnce = true;

PluginHandle g_pluginHandle = kPluginHandle_Invalid;
F4SEPapyrusInterface* g_papyrus = NULL;
F4SEMessagingInterface* g_messaging = NULL;

//Smooth Movement
float smoothingAmount = 10.0f;
float smoothingAmountHorizontal = 0;
float dampingMultiplier = 1.0f;
float dampingMultiplierHorizontal = 0;
float stoppingMultiplier = 0.2f;
float stoppingMultiplierHorizontal = 0.2f;
int disableInteriorSmoothing = 1;
int disableInteriorSmoothingHorizontal = 1;

UInt32 KeywordPowerArmor = 0x4D8A1;
UInt32 KeywordPowerArmorFrame = 0x15503F;

OpenVRHookManagerAPI* vrhook;

namespace F4VRBody {

	Skeleton* playerSkelly = nullptr;

	bool isLoaded = false;

	uint64_t updateCounter = 0;
	uint64_t prevCounter = 0;
	uint64_t localCounter = 0;

	float c_playerHeight = 0.0;
	bool  c_setScale = false;
	float c_fVrScale = 72.0;
	float c_playerOffset_forward = -4.0;
	float c_playerOffset_up = -2.0;
	float c_selfieOutFrontDistance = 120.0f;
	float c_pipboyDetectionRange = 15.0f;
	float c_armLength = 36.74;
	float c_cameraHeight = 0.0;
	float c_dynamicCameraHeight = 0.0;
	float c_PACameraHeight = 0.0;
	bool  c_showPAHUD = true;
	bool  c_hidePipboy = false;
	bool  c_leftHandedPipBoy = false;
	bool  c_selfieMode = false;
	bool  c_verbose = false;
	bool  c_armsOnly = false;
	bool  c_leftHandedMode = false;
	bool  c_disableSmoothMovement = false;
	bool  c_jumping = false;
	float c_powerArmor_forward = 0.0f;
	float c_powerArmor_up = 0.0f;
	float c_RootOffset = 0.0f;
	float c_PARootOffset = 0.0f;
	int c_ConfigModeTimer = 0;
	int c_ConfigModeTimer2 = 0;
	bool c_CalibrateModeActive = false;
	bool c_ExitnoSavePressed = false;
	bool c_ExitandSavePressed = false;
	bool c_PrimaryShiftButtonPressed = false;
	bool c_OffHandShiftButtonPressed = false;
	bool c_SelfieButtonPressed = false;
	bool c_DampenHandsButtonPressed = false;
	bool c_WeaponConfigButtonPressed = false;
	bool  c_staticGripping = false;
	float c_handUI_X = 0.0;
	float c_handUI_Y = 0.0;
	float c_handUI_Z = 0.0;
	bool  c_hideHead = false;
	bool  c_loadedHideHead = false;
	bool  c_hideEquipment = false;
	bool  c_loadedHideEquipment = false;
	bool c_hideSkin = false;
	bool c_loadedHideSkin = false;
	float c_pipBoyLookAtGate = 0.7;
	float c_gripLetGoThreshold = 15.0f;
	bool c_isLookingThroughScope = false;
	bool c_pipBoyButtonMode = false;
	bool c_pipBoyAllowMovementNotLooking = true;
	int c_pipBoyButtonArm = 0;   // 0 for left 1 for right
	int c_pipBoyButtonID = vr::EVRButtonId::k_EButton_Grip; // grip button is 2
	int c_pipBoyButtonOffArm = 0;   // 0 for left 1 for right
	int c_pipBoyButtonOffID = vr::EVRButtonId::k_EButton_Grip; // grip button is 2
	bool c_IsHoloPipboy = true; // false = Default, true = HoloPipBoy
	bool c_IsPipBoyTorchOnArm = true; // false = Head Based Torch, true = PipBoy Based Torch
	int c_SwitchTorchButton = 2; // button to switch torch from head to hand
	float c_DirectionalDeadzone = 0.5; // Default value of fDirectionalDeadzone, used when turning off Pipboy to restore directionial control to the player.
	bool c_switchUIControltoPrimary = true; // if the player wants to switch controls or not.
	bool _controlSleepStickyX = false;
	bool _controlSleepStickyY = false;
	bool _controlSleepStickyT = false;
	int c_gripButtonID = vr::EVRButtonId::k_EButton_Grip; // 2
	int c_holdDelay = 1000; // 1000 ms
	int c_pipBoyOffDelay = 5000; // 5000 ms
	bool c_repositionMasterMode = false;
	int c_repositionButtonID = vr::EVRButtonId::k_EButton_SteamVR_Trigger; //33
	int c_offHandActivateButtonID = vr::EVRButtonId::k_EButton_A; // 7
	bool c_enableOffHandGripping = true;
	bool c_enableGripButtonToGrap = true;
	bool c_enableGripButtonToLetGo = true;
	bool c_onePressGripButton = false;
	bool c_dampenHands = true;
	bool c_dampenPipboyScreen = true;
	bool c_PACurrentState = false;
	bool c_PALastState = false;
	float c_dampenHandsRotation = 0.7;
	float c_dampenHandsTranslation = 0.7;
	float c_dampenPipboyRotation = 0.7;
	float c_dampenPipboyTranslation = 0.7;
	int c_UISelfieButton = 2;
	float c_pipBoyScale = 1.0;
	float c_armLengthbkup;
	float c_powerArmor_upbkup;
	float c_playerOffset_upbkup;
	float c_RootOffsetbkup;
	float c_PARootOffsetbkup;
	float c_fVrScalebkup;
	float c_playerOffset_forwardbkup;
	float c_powerArmor_forwardbkup;
	float c_cameraHeightbkup;
	float c_PACameraHeightbkup;
	bool c_UIHeightButtonPressed = false;
	float c_PlayerHMDHeight;
	float lastCamZ = 0.0;
	float c_shouldertoHMD = 0.0;

	float c_scopeAdjustDistance = 15.0f;

	bool meshesReplaced = false;

	float headDefaultHeight = 125.0;

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

	// hide meshes
	std::vector<std::string> faceGeometry;
	std::vector<std::string> skinGeometry;

	std::vector<int> hideSlotIndexes;
	bool isHideSlotLoaded = false;

	bool bDumpArray = false;

	static bool setHideSlotIndexes(bool isForceReload = false) {
		if (isHideSlotLoaded && !isForceReload)
			return true;

		hideSlotIndexes.clear();

		std::ifstream cullList;
		cullList.open(".\\Data\\F4SE\\plugins\\FRIK_Mesh_Hide\\slots.ini");

		if (cullList.is_open()) {
			while (cullList) {
				std::string input;
				cullList >> input;

				if (input.empty())
					continue;

				input = trim(str_tolower(input));

				// Slot Ids base on this link: https://falloutck.uesp.net/wiki/Biped_Slots

				if (input.find("hairtop") != std::string::npos) {
					hideSlotIndexes.push_back(0);
				}

				if (input.find("hairlong") != std::string::npos) {
					hideSlotIndexes.push_back(1);
				}

				if (input.find("head") != std::string::npos) {
					hideSlotIndexes.push_back(2);
				}

				if (input.find("headband") != std::string::npos) {
					hideSlotIndexes.push_back(16);
				}

				if (input.find("eyes") != std::string::npos) {
					hideSlotIndexes.push_back(17);
				}

				if (input.find("beard") != std::string::npos) {
					hideSlotIndexes.push_back(18);
				}

				if (input.find("mouth") != std::string::npos) {
					hideSlotIndexes.push_back(19);
				}

				if (input.find("neck") != std::string::npos) {
					hideSlotIndexes.push_back(20);
				}

				if (input.find("scalp") != std::string::npos) {
					hideSlotIndexes.push_back(22);
				}
			}
		}

		cullList.close();

		return true;
	}

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
		c_powerArmor_forward = (float) ini.GetDoubleValue("Fallout4VRBody", "powerArmor_forward", 0.0);
		c_powerArmor_up =      (float) ini.GetDoubleValue("Fallout4VRBody", "powerArmor_up", 0.0);
		c_pipboyDetectionRange = (float) ini.GetDoubleValue("Fallout4VRBody", "pipboyDetectionRange", 15.0);
		c_armLength =            (float) ini.GetDoubleValue("Fallout4VRBody", "armLength", 36.74);
		c_cameraHeight =         (float) ini.GetDoubleValue("Fallout4VRBody", "cameraHeightOffset", 0.0);
		c_PACameraHeight =         (float) ini.GetDoubleValue("Fallout4VRBody", "powerArmor_cameraHeightOffset", 0.0);
		c_RootOffset = (float)ini.GetDoubleValue("Fallout4VRBody", "RootOffset", 0.0);
		c_PARootOffset = (float)ini.GetDoubleValue("Fallout4VRBody", "powerArmor_RootOffset", 0.0);
		c_showPAHUD =            ini.GetBoolValue("Fallout4VRBody", "showPAHUD");
		c_hidePipboy =           ini.GetBoolValue("Fallout4VRBody", "hidePipboy");
		c_leftHandedPipBoy =     ini.GetBoolValue("Fallout4VRBody", "PipboyRightArmLeftHandedMode");
		c_verbose =              ini.GetBoolValue("Fallout4VRBody", "VerboseLogging");
		c_armsOnly =             ini.GetBoolValue("Fallout4VRBody", "EnableArmsOnlyMode");
		c_staticGripping         = ini.GetBoolValue("Fallout4VRBody", "EnableStaticGripping");
		c_handUI_X = ini.GetDoubleValue("Fallout4VRBody", "handUI_X", 0.0);
		c_handUI_Y = ini.GetDoubleValue("Fallout4VRBody", "handUI_Y", 0.0);
		c_handUI_Z = ini.GetDoubleValue("Fallout4VRBody", "handUI_Z", 0.0);
		c_hideHead = ini.GetBoolValue("Fallout4VRBody", "HideHead");
		c_loadedHideHead = c_hideHead;
		c_hideEquipment = ini.GetBoolValue("Fallout4VRBody", "HideEquipment");
		c_loadedHideEquipment = c_hideEquipment;
		c_hideSkin = ini.GetBoolValue("Fallout4VRBody", "HideSkin");
		c_loadedHideSkin = c_hideSkin;
		c_pipBoyLookAtGate = ini.GetDoubleValue("Fallout4VRBody", "PipBoyLookAtThreshold", 0.7);
		c_pipBoyOffDelay = (int)ini.GetLongValue("Fallout4VRBody", "PipBoyOffDelay", 5000);
		c_gripLetGoThreshold = ini.GetDoubleValue("Fallout4VRBody", "GripLetGoThreshold", 15.0f);
		c_pipBoyButtonMode =             ini.GetBoolValue("Fallout4VRBody", "OperatePipboyWithButton", false);
		c_pipBoyAllowMovementNotLooking = ini.GetBoolValue("Fallout4VRBody", "AllowMovementWhenNotLookingAtPipboy", true);
		c_pipBoyButtonArm = (int)ini.GetLongValue("Fallout4VRBody", "OperatePipboyWithButtonArm", 0);
		c_pipBoyButtonID = (int)ini.GetLongValue("Fallout4VRBody", "OperatePipboyWithButtonID", vr::EVRButtonId::k_EButton_Grip); //2
		c_pipBoyButtonOffArm = (int)ini.GetLongValue("Fallout4VRBody", "OperatePipboyWithButtonOffArm", 0);
		c_pipBoyButtonOffID = (int)ini.GetLongValue("Fallout4VRBody", "OperatePipboyWithButtonOffID", vr::EVRButtonId::k_EButton_Grip); //2		
		c_IsHoloPipboy = (bool)ini.GetBoolValue("Fallout4VRBody", "HoloPipBoyEnabled", true);
		c_IsPipBoyTorchOnArm = (bool)ini.GetBoolValue("Fallout4VRBody", "PipBoyTorchOnArm", true);
		c_SwitchTorchButton = (int)ini.GetLongValue("Fallout4VRBody", "SwitchTorchButton", 2);
		c_gripButtonID = (int)ini.GetLongValue("Fallout4VRBody", "GripButtonID", vr::EVRButtonId::k_EButton_Grip); // 2
		c_enableOffHandGripping = ini.GetBoolValue("Fallout4VRBody", "EnableOffHandGripping", true);
		c_enableGripButtonToGrap = ini.GetBoolValue("Fallout4VRBody", "EnableGripButton", true);
		c_enableGripButtonToLetGo = ini.GetBoolValue("Fallout4VRBody", "EnableGripButtonToLetGo", true);
		c_onePressGripButton = ini.GetBoolValue("Fallout4VRBody", "EnableGripButtonOnePress", true);
		c_dampenHands = ini.GetBoolValue("Fallout4VRBody", "DampenHands", true);
		c_dampenPipboyScreen = ini.GetBoolValue("Fallout4VRBody", "DampenPipboyScreen", true);
		c_dampenHandsRotation = ini.GetDoubleValue("Fallout4VRBody", "DampenHandsRotation", 0.7);
		c_dampenHandsTranslation = ini.GetDoubleValue("Fallout4VRBody", "DampenHandsTranslation", 0.7);
		c_dampenPipboyRotation = ini.GetDoubleValue("Fallout4VRBody", "DampenPipboyRotation", 0.7);
		c_dampenPipboyTranslation = ini.GetDoubleValue("Fallout4VRBody", "DampenPipboyTranslation", 0.7);
		c_DirectionalDeadzone = ini.GetDoubleValue("Fallout4VRBody", "fDirectionalDeadzone", 0.5);
		c_PlayerHMDHeight = ini.GetDoubleValue("Fallout4VRBody", "fHMDHeight", 109.0);
		c_shouldertoHMD = ini.GetDoubleValue("Fallout4VRBody", "fShouldertoHMD", 109.0);
		//Pipboy & Main Config Mode Buttons
		c_pipBoyScale = (float)ini.GetDoubleValue("Fallout4VRBody", "PipboyScale", 1.0);
		c_UISelfieButton = (int)ini.GetLongValue("ConfigModeUIButtons", "ToggleSelfieModeButton", 2);
		c_switchUIControltoPrimary = (bool)ini.GetBoolValue("Fallout4VRBody", "PipboyUIPrimaryController", true);
		 


		//Smooth Movement
		c_disableSmoothMovement            = ini.GetBoolValue("SmoothMovementVR", "DisableSmoothMovement");
		smoothingAmount                    = (float) ini.GetDoubleValue("SmoothMovementVR", "SmoothAmount", 15.0);
		smoothingAmountHorizontal          = (float) ini.GetDoubleValue("SmoothMovementVR", "SmoothAmountHorizontal", 5.0);
		dampingMultiplier                  = (float) ini.GetDoubleValue("SmoothMovementVR", "Damping", 1.0);
		dampingMultiplierHorizontal        = (float) ini.GetDoubleValue("SmoothMovementVR", "DampingHorizontal", 1.0);
		stoppingMultiplier                 = (float) ini.GetDoubleValue("SmoothMovementVR", "StoppingMultiplier", 0.6);
		stoppingMultiplierHorizontal       = (float) ini.GetDoubleValue("SmoothMovementVR", "StoppingMultiplierHorizontal", 0.6);
		disableInteriorSmoothing           = ini.GetBoolValue("SmoothMovementVR", "DisableInteriorSmoothing", 1);
		disableInteriorSmoothingHorizontal = ini.GetBoolValue("SmoothMovementVR", "DisableInteriorSmoothingHorizontal", 1);

		// weaponPositioning
		//c_repositionMasterMode = ini.GetBoolValue("Fallout4VRBody", "EnableRepositionMode", false);       Enabled / Disabled via config mode
		c_holdDelay = (int)ini.GetLongValue("Fallout4VRBody", "HoldDelay", 1000);
		c_repositionButtonID = (int)ini.GetLongValue("Fallout4VRBody", "RepositionButtonID", vr::EVRButtonId::k_EButton_SteamVR_Trigger); // 33
		c_offHandActivateButtonID = (int)ini.GetLongValue("Fallout4VRBody", "OffHandActivateButtonID", vr::EVRButtonId::k_EButton_A); // 7
		c_scopeAdjustDistance = ini.GetDoubleValue("Fallout4VRBody", "ScopeAdjustDistance", 15.f);
		// now load weapon offset JSON
		readOffsetJson();

		std::ifstream cullList;

		cullList.open(".\\Data\\F4SE\\plugins\\FRIK_Mesh_Hide\\face.ini");

		if (cullList.is_open()) {
			while (cullList) {
				std::string input;
				cullList >> input;
				if (!input.empty())
					faceGeometry.push_back(trim(str_tolower(input)));
			}
		}

		cullList.close();

		cullList.open(".\\Data\\F4SE\\plugins\\FRIK_Mesh_Hide\\skins.ini");

		if (cullList.is_open()) {
			while (cullList) {
				std::string input;
				cullList >> input;
				if (!input.empty())
					skinGeometry.push_back(trim(str_tolower(input)));
			}
		}

		cullList.close();

		return true;
	}

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

	static void restoreGeometry() {
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
		BSFadeNode* rn = static_cast<BSFadeNode*>((*g_player)->unkF0->rootNode);

		if (!rn) {
			return;
		}

		if (c_hideHead || c_hideSkin) {
			for (auto i = 0; i < rn->kGeomArray.count; ++i) {
				bool hide = false;
				auto& geometry = rn->kGeomArray[i].spGeometry;
				auto geometryName = geometry->m_name.c_str();
				auto geomStr = trim(str_tolower(std::string(geometryName)));

				if (c_hideHead) {
					for (auto& faceGeom : faceGeometry) {
						if (geomStr.find(faceGeom) != std::string::npos) {
							//_MESSAGE("Found %s in %s", faceGeom, geomStr.c_str());
							hide = true;
							break;
						}
					}
				}

				if (c_hideSkin && !hide) {
					for (auto& skinGeom : skinGeometry) {
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

		if (c_hideEquipment) {
			//Ensure data is loaded.
			isHideSlotLoaded = setHideSlotIndexes();

			for (auto slot : hideSlotIndexes) {
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

	void replaceMeshes(PlayerNodes* pn, std::string itemHide, std::string itemShow) {
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
		c_IsHoloPipboy ? retNode = loadNifFromFile("Data/Meshes/FRIK/HoloPipboyVR.nif") : retNode = loadNifFromFile("Data/Meshes/FRIK/PipboyVR.nif");
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
		if (c_IsHoloPipboy == true) {
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
		node->m_localTransform.pos = origLoc + NiPoint3(c_handUI_X, c_handUI_Y, c_handUI_Z);

		updateTransformsDown(node, true);
	}

	bool setSkelly(bool inPowerArmor) {

		if (c_verbose) {
			_MESSAGE("setSkelly Start");
		}

		if (!(*g_player)->unkF0) {
			if (c_verbose) {
				_MESSAGE("loaded Data Not Set Yet");
			}
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

			playerSkelly = new Skeleton(node);
			_MESSAGE("skeleton = %016I64X", playerSkelly->getRoot());
			if (!playerSkelly->setNodes()) {
				return false;
			}
			//replaceMeshes(playerSkelly->getPlayerNodes());
			//playerSkelly->setDirection();
			playerSkelly->swapPipboy();

			_MESSAGE("handle pipboy init");

			turnPipBoyOff();


			vrhook = RequestOpenVRHookManagerObject();

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

	void smoothMovement()
	{
		if (!c_disableSmoothMovement) {
			if (c_verbose) { _MESSAGE("Smooth Movement"); }
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

		if (!inPowerArmorSticky) {
			inPowerArmorSticky = detectInPowerArmor();

			if (inPowerArmorSticky) {
				delete playerSkelly;
				firstTime = true;
				return;
			}
		}
		else {
			inPowerArmorSticky = detectInPowerArmor();

			if (!inPowerArmorSticky) {
				delete playerSkelly;
				firstTime = true;
				return;
			}
		}


		if (!playerSkelly || firstTime) {
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
			if (c_IsHoloPipboy == 0) {
				replaceMeshes(playerSkelly->getPlayerNodes(),"HoloEmitter", "Screen");
			}
			else if (c_IsHoloPipboy == 1) {
				replaceMeshes(playerSkelly->getPlayerNodes(), "Screen", "HoloEmitter");
			}
			_MESSAGE("initialized for real");
			return;
		}

		// do stuff now
		c_leftHandedMode = *Offsets::iniLeftHandedMode;
		playerSkelly->setLeftHandedSticky();


		if (c_verbose) { _MESSAGE("Start of Frame"); }

		c_leftHandedMode = *Offsets::iniLeftHandedMode;

		if (!meshesReplaced) {
			if (c_IsHoloPipboy == 0) {
				replaceMeshes(playerSkelly->getPlayerNodes(), "HoloEmitter", "Screen");
			}
			else if (c_IsHoloPipboy == 1) {
				replaceMeshes(playerSkelly->getPlayerNodes(), "Screen", "HoloEmitter");
			}
		}


		// check if jumping or in air;


		// check if jumping or in air;
		c_jumping = SmoothMovementVR::checkIfJumpingOrInAir();

		playerSkelly->setTime();

		VRHook::g_vrHook->setVRControllerState();

		if (c_verbose) { _MESSAGE("Hide Wands"); }
		playerSkelly->hideWands();

		//	fixSkeleton();

		NiPoint3 position = (*g_player)->pos;
		float groundHeight = 0.0f;

		uint64_t ret = Offsets::TESObjectCell_GetLandHeight((*g_player)->parentCell, &position, &groundHeight);

		// first restore locals to a default state to wipe out any local transform changes the game might have made since last update
		if (c_verbose) { _MESSAGE("restore locals of skeleton"); }
		playerSkelly->restoreLocals(playerSkelly->getRoot()->m_parent->GetAsNiNode());
		playerSkelly->updateDown(playerSkelly->getRoot(), true);


		// moves head up and back out of the player view.   doing this instead of hiding with a small scale setting since it preserves neck shape
		if (c_verbose) { _MESSAGE("Setup Head"); }
		NiNode* headNode = playerSkelly->getNode("Head", playerSkelly->getRoot());
		playerSkelly->setupHead(headNode, c_hideHead);

		//// set up the body underneath the headset in a proper scale and orientation
		if (c_verbose) { _MESSAGE("Set body under HMD"); }
		playerSkelly->setUnderHMD(groundHeight);
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// Now Set up body Posture and hook up the legs
		if (c_verbose) { _MESSAGE("Set body posture"); }
		playerSkelly->setBodyPosture();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		if (c_verbose) { _MESSAGE("Set Knee Posture"); }
		playerSkelly->setKneePos();
		if (c_verbose) { _MESSAGE("Set Walk"); }

		if (!c_armsOnly) {
			playerSkelly->walk();
		}
		//playerSkelly->setLegs();
		if (c_verbose) { _MESSAGE("Set Legs"); }
		playerSkelly->setSingleLeg(false);
		playerSkelly->setSingleLeg(true);

		// Do another update before setting arms
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// do arm IK - Right then Left
		if (c_verbose) { _MESSAGE("Set Arms"); }
		playerSkelly->handleWeaponNodes();
		playerSkelly->setArms(false);
		playerSkelly->setArms(true);
		playerSkelly->leftHandedModePipboy();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Do world update now so that IK calculations have proper world reference

		// Misc stuff to showahide things and also setup the wrist pipboy
		if (c_verbose) { _MESSAGE("Pipboy and Weapons"); }
		playerSkelly->hideWeapon();
		playerSkelly->positionPipboy();
		playerSkelly->hidePipboy();
		playerSkelly->fixMelee();
		playerSkelly->hideFistHelpers();
		playerSkelly->showHidePAHUD();

		cullGeometry();

		// project body out in front of the camera for debug purposes
		if (c_verbose) { _MESSAGE("Selfie Time"); }
		playerSkelly->selfieSkelly(c_selfieOutFrontDistance);
		playerSkelly->updateDown(playerSkelly->getRoot(), true);

		if (c_verbose) { _MESSAGE("fix the missing screen"); }
		fixMissingScreen(playerSkelly->getPlayerNodes());

		setHandUI(playerSkelly->getPlayerNodes());

		if (c_armsOnly) {
			playerSkelly->showOnlyArms();
		}

		playerSkelly->setHandPose();
		if (c_verbose) { _MESSAGE("Operate Pipboy"); }
		playerSkelly->operatePipBoy();
		if (c_verbose) { _MESSAGE("bone sphere stuff"); }
		detectBoneSphere();
		handleDebugBoneSpheres();
		//g_gunReloadSystem->Update();


		playerSkelly->offHandToBarrel();
		playerSkelly->offHandToScope();

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
			playerSkelly->hideHands();
		}
		// Cylons Code Starts Here ---->
		playerSkelly->pipboyConfigurationMode();
		playerSkelly->mainConfigurationMode();
		playerSkelly->pipboyManagement();
		playerSkelly->dampenPipboyScreen();
		//Hide some Pipboy related meshes on exit of Power Armor if they're not hidden
		if (!detectInPowerArmor()) {
			NiNode* _HideNode = nullptr;
			c_IsHoloPipboy ? _HideNode = getChildNode("Screen", (*g_player)->unkF0->rootNode) : _HideNode = getChildNode("HoloEmitter", (*g_player)->unkF0->rootNode);
			if (_HideNode) {
				if (_HideNode->m_localTransform.scale != 0) {
					_HideNode->flags |= 0x1;
					_HideNode->m_localTransform.scale = 0;
				}
			}
		}
		if (c_CalibrateModeActive) {
			vr::VRControllerAxis_t doinantHandStick = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).rAxis[0] : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).rAxis[0]);
			uint64_t dominantHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed);
			uint64_t offHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed);
			const auto ExitandSave = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)33);
			const auto ExitnoSave = offHand & vr::ButtonMaskFromId((vr::EVRButtonId)33);
			const auto SelfieButton = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)1);
			const auto HeightButton = offHand & vr::ButtonMaskFromId((vr::EVRButtonId)1);
			if (ExitandSave && !c_ExitandSavePressed) {
				c_ExitandSavePressed = true;
				playerSkelly->configModeExit();
				saveSettings();
				if (vrhook != nullptr) {
					c_leftHandedMode ? vrhook->StartHaptics(1, 0.55, 0.5) : vrhook->StartHaptics(2, 0.55, 0.5);
				}
			}
			else if (!ExitandSave) {
				c_ExitandSavePressed = false;
			}
			if (ExitnoSave && !c_ExitnoSavePressed) {
				c_ExitnoSavePressed = true;
				playerSkelly->configModeExit();
				c_armLength = c_armLengthbkup;
				c_powerArmor_up = c_powerArmor_upbkup;
				c_playerOffset_up = c_playerOffset_upbkup;
				c_RootOffset = c_RootOffsetbkup;
				c_PARootOffset = c_PARootOffsetbkup;
				c_fVrScale = c_fVrScalebkup;
				c_playerOffset_forward = c_playerOffset_forwardbkup;
				c_powerArmor_forward = c_powerArmor_forwardbkup;
				c_cameraHeight = c_cameraHeightbkup;
				c_PACameraHeight = c_PACameraHeightbkup;
			}
			else if (!ExitnoSave) {
				c_ExitnoSavePressed = false;
			}
			if (SelfieButton && !c_SelfieButtonPressed) {
				c_SelfieButtonPressed = true;
				c_selfieMode = !c_selfieMode;
			}
			else if (!SelfieButton) {
				c_SelfieButtonPressed = false;
			}
			if (HeightButton && !c_UIHeightButtonPressed) {
				c_UIHeightButtonPressed = true;
				PlayerNodes* skelly = playerSkelly->getPlayerNodes();
				c_PlayerHMDHeight = skelly->UprightHmdNode->m_localTransform.pos.z;
				float x = playerSkelly->getNode("LArm_Collarbone", playerSkelly->getRoot())->m_worldTransform.pos.z;
				c_shouldertoHMD = c_PlayerHMDHeight - x;
				lastCamZ = 0.0;
			}
			else if (!HeightButton) {
				c_UIHeightButtonPressed = false;
			}
		}
		else {
			uint64_t dominantHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed);
			uint64_t offHand = (c_leftHandedMode ? VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Right).ulButtonPressed : VRHook::g_vrHook->getControllerState(VRHook::VRSystem::TrackerType::Left).ulButtonPressed);
			const auto dHTouch = dominantHand & vr::ButtonMaskFromId((vr::EVRButtonId)32);
			const auto oHTouch = offHand & vr::ButtonMaskFromId((vr::EVRButtonId)32);
			if (dHTouch && !c_CalibrateModeActive) {
				c_ConfigModeTimer += 1;
				if (c_ConfigModeTimer > 200 && c_ConfigModeTimer2 > 200) {
					c_DampenHandsButtonPressed = true;
					c_CalibrateModeActive = true;
				}
			}
			else if (!dHTouch && !c_CalibrateModeActive && c_ConfigModeTimer > 0) {
				c_ConfigModeTimer = 0;
			}
			if (oHTouch && !c_CalibrateModeActive) {
				c_ConfigModeTimer2 += 1;
			}
			else if (!oHTouch && !c_CalibrateModeActive && c_ConfigModeTimer2 > 0) {
				c_ConfigModeTimer2 = 0;
			}
		}
		// Cylons Code Ends Here
		playerSkelly->fixBackOfHand();
		playerSkelly->updateDown(playerSkelly->getRoot(), true);  // Last world update before exit.    Probably not necessary.

		dumpGeometryArrayInUpdate();

		playerSkelly->debug();

		if (!detectInPowerArmor()) { // sets 3rd Person Pipboy Scale
			NiNode* _Pipboy3rd = getChildNode("PipboyBone", (*g_player)->unkF0->rootNode);
			if (_Pipboy3rd) {
				_Pipboy3rd->m_localTransform.scale = c_pipBoyScale;
			}
		}
		else {
			playerSkelly->fixArmor();
		}
	}

	void swapPB() {
		if (c_IsHoloPipboy == 0) {
			replaceMeshes(playerSkelly->getPlayerNodes(), "HoloEmitter", "Screen");
		}
		else if (c_IsHoloPipboy == 1) {
			replaceMeshes(playerSkelly->getPlayerNodes(), "Screen", "HoloEmitter");
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

	void saveSettings() {
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\FRIK.ini");

		rc = ini.SetDoubleValue("Fallout4VRBody", "PlayerHeight", (double)c_playerHeight);
		rc = ini.SetDoubleValue("Fallout4VRBody", "fVrScale", (double)c_fVrScale);
		rc = ini.SetDoubleValue("Fallout4VRBody", "playerOffset_forward", (double)c_playerOffset_forward);
		rc = ini.SetDoubleValue("Fallout4VRBody", "playerOffset_up", (double)c_playerOffset_up);
		rc = ini.SetDoubleValue("Fallout4VRBody", "powerArmor_forward", (double)c_powerArmor_forward);
		rc = ini.SetDoubleValue("Fallout4VRBody", "powerArmor_up", (double)c_powerArmor_up);
		rc = ini.SetDoubleValue("Fallout4VRBody", "armLength", (double)c_armLength);
		rc = ini.SetDoubleValue("Fallout4VRBody", "cameraHeightOffset", (double)c_cameraHeight);
		rc = ini.SetDoubleValue("Fallout4VRBody", "powerArmor_cameraHeightOffset", (double)c_PACameraHeight);
		rc = ini.SetBoolValue("Fallout4VRBody", "showPAHUD", c_showPAHUD);
		rc = ini.SetBoolValue("Fallout4VRBody", "hidePipboy", c_hidePipboy);
		rc = ini.SetDoubleValue("Fallout4VRBody", "PipboyScale", (double)c_pipBoyScale);
		rc = ini.SetBoolValue("Fallout4VRBody", "HoloPipBoyEnabled", c_IsHoloPipboy);
		rc = ini.SetBoolValue("Fallout4VRBody", "PipBoyTorchOnArm", c_IsPipBoyTorchOnArm);
		rc = ini.SetBoolValue("Fallout4VRBody", "EnableArmsOnlyMode", c_armsOnly);
		rc = ini.SetBoolValue("Fallout4VRBody", "EnableStaticGripping", c_staticGripping);
		//rc = ini.SetBoolValue("Fallout4VRBody", "EnableRepositionMode", c_repositionMasterMode);  Handled by Config Mode. 
		rc = ini.SetBoolValue("Fallout4VRBody", "HideTheHead", c_hideHead);
		rc = ini.SetDoubleValue("Fallout4VRBody", "handUI_X", c_handUI_X);
		rc = ini.SetDoubleValue("Fallout4VRBody", "handUI_Y", c_handUI_Y);
		rc = ini.SetDoubleValue("Fallout4VRBody", "handUI_Z", c_handUI_Z);
		rc = ini.SetBoolValue("Fallout4VRBody", "DampenHands", c_dampenHands);
		rc = ini.SetDoubleValue("Fallout4VRBody", "DampenHandsRotation", c_dampenHandsRotation);
		rc = ini.SetDoubleValue("Fallout4VRBody", "DampenHandsTranslation", c_dampenHandsTranslation);
		rc = ini.SetDoubleValue("Fallout4VRBody", "powerArmor_RootOffset", (double)c_PARootOffset);
		rc = ini.SetDoubleValue("Fallout4VRBody", "RootOffset", (double)c_RootOffset);
		rc = ini.SetDoubleValue("Fallout4VRBody", "fHMDHeight", (double)c_PlayerHMDHeight);
		rc = ini.SetDoubleValue("Fallout4VRBody", "fShouldertoHMD", (double)c_shouldertoHMD);
		rc = ini.SaveFile(".\\Data\\F4SE\\plugins\\FRIK.ini");
		
		// save off any weapon offsets
		writeOffsetJson();

		if (rc < 0) {
			_MESSAGE("Failed to write out INI config file");
		}
		else {
			_MESSAGE("successfully wrote config file");
		}

	}

	void saveStates(StaticFunctionTag* base) {
		saveSettings();
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

		c_playerHeight = pn->UprightHmdNode->m_localTransform.pos.z;

		_MESSAGE("Calibrated Height: %f  arm length: %f %f", c_playerHeight, c_armLength);
		ShowMessagebox("FRIK Config Mode");
	}

	void togglePAHUD(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_showPAHUD = !c_showPAHUD;
	}

	void toggleHeadVis(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_hideHead = !c_hideHead;
		c_loadedHideHead = c_hideHead;

		if (c_hideHead) {
			restoreGeometry();
		}
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

		if (c_selfieMode)
		{
			restoreGeometry();
			c_hideHead = false;
			c_hideEquipment = false;
			c_hideSkin = false;
		}
		else
		{
			c_hideHead = c_loadedHideHead;
			c_hideEquipment = c_loadedHideEquipment;
			c_hideSkin = c_loadedHideSkin;
		}
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

		c_armsOnly = !c_armsOnly;
	}

	void toggleStaticGripping(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_staticGripping = !c_staticGripping;

		bool gripConfig = !F4VRBody::c_staticGripping;
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

		c_cameraHeight += 2.0f;
	}

	void moveCameraDown(StaticFunctionTag* base){
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_cameraHeight -= 2.0f;
	}

	void setDynamicCameraHeight(StaticFunctionTag* base, float dynamicCameraHeight) {
		c_dynamicCameraHeight = dynamicCameraHeight;
	}

	void makeTaller(StaticFunctionTag* base) {
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

	void handUiXUp(StaticFunctionTag* base) {
		c_handUI_X += 1.0f;
	}

	void handUiXDown(StaticFunctionTag* base) {
		c_handUI_X -= 1.0f;
	}

	void handUiYUp(StaticFunctionTag* base) {
		c_handUI_Y += 1.0f;
	}

	void handUiYDown(StaticFunctionTag* base) {
		c_handUI_Y -= 1.0f;
	}

	void handUiZUp(StaticFunctionTag* base) {
		c_handUI_Z += 1.0f;
	}

	void handUiZDown(StaticFunctionTag* base) {
		c_handUI_Z -= 1.0f;
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

		c_dampenHands = !c_dampenHands;
	}

	void increaseDampenRotation(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_dampenHandsRotation += 0.05f;
		c_dampenHandsRotation = c_dampenHandsRotation >= 1.0f ? 0.95f : c_dampenHandsRotation;
	}

	void decreaseDampenRotation(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_dampenHandsRotation -= 0.05f;
		c_dampenHandsRotation = c_dampenHandsRotation <= 0.0f ? 0.05f : c_dampenHandsRotation;
	}

	void increaseDampenTranslation(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_dampenHandsTranslation += 0.05f;

		c_dampenHandsTranslation = c_dampenHandsTranslation >= 1.0f ? 0.95f : c_dampenHandsTranslation;
	}

	void decreaseDampenTranslation(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_dampenHandsTranslation -= 0.05f;
		c_dampenHandsTranslation = c_dampenHandsTranslation <= 0.0f ? 0.5f : c_dampenHandsTranslation;
	}

	void toggleRepositionMasterMode(StaticFunctionTag* base) {
		BSFixedString menuName("BookMenu");
		if ((*g_ui)->IsMenuRegistered(menuName)) {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}

		c_repositionMasterMode = !c_repositionMasterMode;
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