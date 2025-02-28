#include "Config.h"
#include "weaponOffset.h"
#include "include/SimpleIni.h"
#include "utils.h"

namespace F4VRBody {

	Config* g_config = nullptr;

	bool Config::loadConfig() {
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\FRIK.ini");

		if (rc < 0) {
			_MESSAGE("ERROR: cannot read FRIK.ini");
			return false;
		}

		playerHeight = (float)ini.GetDoubleValue("Fallout4VRBody", "PlayerHeight", 120.4828f);
		setScale = ini.GetBoolValue("Fallout4VRBody", "setScale", false);
		fVrScale = (float)ini.GetDoubleValue("Fallout4VRBody", "fVrScale", 70.0);
		playerOffset_forward = (float)ini.GetDoubleValue("Fallout4VRBody", "playerOffset_forward", -4.0);
		playerOffset_up = (float)ini.GetDoubleValue("Fallout4VRBody", "playerOffset_up", -2.0);
		powerArmor_forward = (float)ini.GetDoubleValue("Fallout4VRBody", "powerArmor_forward", 0.0);
		powerArmor_up = (float)ini.GetDoubleValue("Fallout4VRBody", "powerArmor_up", 0.0);
		pipboyDetectionRange = (float)ini.GetDoubleValue("Fallout4VRBody", "pipboyDetectionRange", 15.0);
		armLength = (float)ini.GetDoubleValue("Fallout4VRBody", "armLength", 36.74);
		cameraHeight = (float)ini.GetDoubleValue("Fallout4VRBody", "cameraHeightOffset", 0.0);
		PACameraHeight = (float)ini.GetDoubleValue("Fallout4VRBody", "powerArmor_cameraHeightOffset", 0.0);
		rootOffset = (float)ini.GetDoubleValue("Fallout4VRBody", "RootOffset", 0.0);
		PARootOffset = (float)ini.GetDoubleValue("Fallout4VRBody", "powerArmor_RootOffset", 0.0);
		showPAHUD = ini.GetBoolValue("Fallout4VRBody", "showPAHUD");
		hidePipboy = ini.GetBoolValue("Fallout4VRBody", "hidePipboy");
		leftHandedPipBoy = ini.GetBoolValue("Fallout4VRBody", "PipboyRightArmLeftHandedMode");
		verbose = ini.GetBoolValue("Fallout4VRBody", "VerboseLogging");
		armsOnly = ini.GetBoolValue("Fallout4VRBody", "EnableArmsOnlyMode");
		staticGripping = ini.GetBoolValue("Fallout4VRBody", "EnableStaticGripping");
		handUI_X = ini.GetDoubleValue("Fallout4VRBody", "handUI_X", 0.0);
		handUI_Y = ini.GetDoubleValue("Fallout4VRBody", "handUI_Y", 0.0);
		handUI_Z = ini.GetDoubleValue("Fallout4VRBody", "handUI_Z", 0.0);
		hideHead = ini.GetBoolValue("Fallout4VRBody", "HideHead");
		c_loadedHideHead = hideHead;
		hideEquipment = ini.GetBoolValue("Fallout4VRBody", "HideEquipment");
		c_loadedHideEquipment = hideEquipment;
		hideSkin = ini.GetBoolValue("Fallout4VRBody", "HideSkin");
		c_loadedHideSkin = hideSkin;
		pipBoyLookAtGate = ini.GetDoubleValue("Fallout4VRBody", "PipBoyLookAtThreshold", 0.7);
		pipBoyOffDelay = (int)ini.GetLongValue("Fallout4VRBody", "PipBoyOffDelay", 5000);
		pipBoyOnDelay = (int)ini.GetLongValue("Fallout4VRBody", "PipBoyOnDelay", 5000);
		gripLetGoThreshold = ini.GetDoubleValue("Fallout4VRBody", "GripLetGoThreshold", 15.0f);
		pipBoyButtonMode = ini.GetBoolValue("Fallout4VRBody", "OperatePipboyWithButton", false);
		pipBoyOpenWhenLookAt = ini.GetBoolValue("Fallout4VRBody", "PipBoyOpenWhenLookAt", false);
		pipBoyAllowMovementNotLooking = ini.GetBoolValue("Fallout4VRBody", "AllowMovementWhenNotLookingAtPipboy", true);
		pipBoyButtonArm = (int)ini.GetLongValue("Fallout4VRBody", "OperatePipboyWithButtonArm", 0);
		pipBoyButtonID = (int)ini.GetLongValue("Fallout4VRBody", "OperatePipboyWithButtonID", vr::EVRButtonId::k_EButton_Grip); //2
		pipBoyButtonOffArm = (int)ini.GetLongValue("Fallout4VRBody", "OperatePipboyWithButtonOffArm", 0);
		pipBoyButtonOffID = (int)ini.GetLongValue("Fallout4VRBody", "OperatePipboyWithButtonOffID", vr::EVRButtonId::k_EButton_Grip); //2		
		isHoloPipboy = (bool)ini.GetBoolValue("Fallout4VRBody", "HoloPipBoyEnabled", true);
		isPipBoyTorchOnArm = (bool)ini.GetBoolValue("Fallout4VRBody", "PipBoyTorchOnArm", true);
		switchTorchButton = (int)ini.GetLongValue("Fallout4VRBody", "SwitchTorchButton", 2);
		gripButtonID = (int)ini.GetLongValue("Fallout4VRBody", "GripButtonID", vr::EVRButtonId::k_EButton_Grip); // 2
		enableOffHandGripping = ini.GetBoolValue("Fallout4VRBody", "EnableOffHandGripping", true);
		enableGripButtonToGrap = ini.GetBoolValue("Fallout4VRBody", "EnableGripButton", true);
		enableGripButtonToLetGo = ini.GetBoolValue("Fallout4VRBody", "EnableGripButtonToLetGo", true);
		onePressGripButton = ini.GetBoolValue("Fallout4VRBody", "EnableGripButtonOnePress", true);
		dampenHands = ini.GetBoolValue("Fallout4VRBody", "DampenHands", true);
		dampenHandsInVanillaScope = ini.GetBoolValue("Fallout4VRBody", "DampenHandsInVanillaScope", true);
		dampenPipboyScreen = ini.GetBoolValue("Fallout4VRBody", "DampenPipboyScreen", true);
		dampenHandsRotation = ini.GetDoubleValue("Fallout4VRBody", "DampenHandsRotation", 0.7);
		dampenHandsTranslation = ini.GetDoubleValue("Fallout4VRBody", "DampenHandsTranslation", 0.7);
		dampenHandsRotationInVanillaScope = ini.GetDoubleValue("Fallout4VRBody", "DampenHandsRotationInVanillaScope", 0.2);
		dampenHandsTranslationInVanillaScope = ini.GetDoubleValue("Fallout4VRBody", "DampenHandsTranslationInVanillaScope", 0.2);
		dampenPipboyRotation = ini.GetDoubleValue("Fallout4VRBody", "DampenPipboyRotation", 0.7);
		dampenPipboyTranslation = ini.GetDoubleValue("Fallout4VRBody", "DampenPipboyTranslation", 0.7);
		directionalDeadzone = ini.GetDoubleValue("Fallout4VRBody", "fDirectionalDeadzone", 0.5);
		playerHMDHeight = ini.GetDoubleValue("Fallout4VRBody", "fHMDHeight", 109.0);
		shoulderToHMD = ini.GetDoubleValue("Fallout4VRBody", "fShouldertoHMD", 109.0);
		
		//Pipboy & Main Config Mode Buttons
		pipBoyScale = (float)ini.GetDoubleValue("Fallout4VRBody", "PipboyScale", 1.0);
		UISelfieButton = (int)ini.GetLongValue("ConfigModeUIButtons", "ToggleSelfieModeButton", 2);
		switchUIControltoPrimary = (bool)ini.GetBoolValue("Fallout4VRBody", "PipboyUIPrimaryController", true);
		autoFocusWindow = (bool)ini.GetBoolValue("Fallout4VRBody", "AutoFocusWindow", false);

		//Smooth Movement
		disableSmoothMovement = ini.GetBoolValue("SmoothMovementVR", "DisableSmoothMovement");
		smoothingAmount = (float)ini.GetDoubleValue("SmoothMovementVR", "SmoothAmount", 15.0);
		smoothingAmountHorizontal = (float)ini.GetDoubleValue("SmoothMovementVR", "SmoothAmountHorizontal", 5.0);
		dampingMultiplier = (float)ini.GetDoubleValue("SmoothMovementVR", "Damping", 1.0);
		dampingMultiplierHorizontal = (float)ini.GetDoubleValue("SmoothMovementVR", "DampingHorizontal", 1.0);
		stoppingMultiplier = (float)ini.GetDoubleValue("SmoothMovementVR", "StoppingMultiplier", 0.6);
		stoppingMultiplierHorizontal = (float)ini.GetDoubleValue("SmoothMovementVR", "StoppingMultiplierHorizontal", 0.6);
		disableInteriorSmoothing = ini.GetBoolValue("SmoothMovementVR", "DisableInteriorSmoothing", 1);
		disableInteriorSmoothingHorizontal = ini.GetBoolValue("SmoothMovementVR", "DisableInteriorSmoothingHorizontal", 1);

		// weaponPositioning
		//c_repositionMasterMode = ini.GetBoolValue("Fallout4VRBody", "EnableRepositionMode", false);       Enabled / Disabled via config mode
		holdDelay = (int)ini.GetLongValue("Fallout4VRBody", "HoldDelay", 1000);
		repositionButtonID = (int)ini.GetLongValue("Fallout4VRBody", "RepositionButtonID", vr::EVRButtonId::k_EButton_SteamVR_Trigger); // 33
		offHandActivateButtonID = (int)ini.GetLongValue("Fallout4VRBody", "OffHandActivateButtonID", vr::EVRButtonId::k_EButton_A); // 7
		scopeAdjustDistance = ini.GetDoubleValue("Fallout4VRBody", "ScopeAdjustDistance", 15.f);
		
		// now load weapon offset JSON
		readOffsetJson();

		loadHideFace();

		loadHideSkins();

		loadHideSlots();

		return true;
	}

	void Config::loadHideFace() {
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
	}

	void Config::loadHideSkins() {
		std::ifstream cullList;
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
	}

	void Config::loadHideSlots() {
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
	}

	void Config::saveSettings() {
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\FRIK.ini");

		rc = ini.SetDoubleValue("Fallout4VRBody", "PlayerHeight", (double)playerHeight);
		rc = ini.SetDoubleValue("Fallout4VRBody", "fVrScale", (double)fVrScale);
		rc = ini.SetDoubleValue("Fallout4VRBody", "playerOffset_forward", (double)playerOffset_forward);
		rc = ini.SetDoubleValue("Fallout4VRBody", "playerOffset_up", (double)playerOffset_up);
		rc = ini.SetDoubleValue("Fallout4VRBody", "powerArmor_forward", (double)powerArmor_forward);
		rc = ini.SetDoubleValue("Fallout4VRBody", "powerArmor_up", (double)powerArmor_up);
		rc = ini.SetDoubleValue("Fallout4VRBody", "armLength", (double)armLength);
		rc = ini.SetDoubleValue("Fallout4VRBody", "cameraHeightOffset", (double)cameraHeight);
		rc = ini.SetDoubleValue("Fallout4VRBody", "powerArmor_cameraHeightOffset", (double)PACameraHeight);
		rc = ini.SetBoolValue("Fallout4VRBody", "showPAHUD", showPAHUD);
		rc = ini.SetBoolValue("Fallout4VRBody", "hidePipboy", hidePipboy);
		rc = ini.SetDoubleValue("Fallout4VRBody", "PipboyScale", (double)pipBoyScale);
		rc = ini.SetBoolValue("Fallout4VRBody", "HoloPipBoyEnabled", isHoloPipboy);
		rc = ini.SetBoolValue("Fallout4VRBody", "PipBoyTorchOnArm", isPipBoyTorchOnArm);
		rc = ini.SetBoolValue("Fallout4VRBody", "EnableArmsOnlyMode", armsOnly);
		rc = ini.SetBoolValue("Fallout4VRBody", "EnableStaticGripping", staticGripping);
		//rc = ini.SetBoolValue("Fallout4VRBody", "EnableRepositionMode", c_repositionMasterMode);  Handled by Config Mode. 
		rc = ini.SetBoolValue("Fallout4VRBody", "HideTheHead", hideHead);
		rc = ini.SetDoubleValue("Fallout4VRBody", "handUI_X", handUI_X);
		rc = ini.SetDoubleValue("Fallout4VRBody", "handUI_Y", handUI_Y);
		rc = ini.SetDoubleValue("Fallout4VRBody", "handUI_Z", handUI_Z);
		rc = ini.SetBoolValue("Fallout4VRBody", "DampenHands", dampenHands);
		rc = ini.SetDoubleValue("Fallout4VRBody", "DampenHandsRotation", dampenHandsRotation);
		rc = ini.SetDoubleValue("Fallout4VRBody", "DampenHandsTranslation", dampenHandsTranslation);
		rc = ini.SetDoubleValue("Fallout4VRBody", "powerArmor_RootOffset", (double)PARootOffset);
		rc = ini.SetDoubleValue("Fallout4VRBody", "RootOffset", (double)rootOffset);
		rc = ini.SetDoubleValue("Fallout4VRBody", "fHMDHeight", (double)playerHMDHeight);
		rc = ini.SetDoubleValue("Fallout4VRBody", "fShouldertoHMD", (double)shoulderToHMD);
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

	/// <summary>
	/// Save specific key and bool value into FRIK.ini file.
	/// </summary>
	void Config::saveBoolValue(const char* key, bool value) {
		_MESSAGE("Config: Saving \"%s = %s\" to FRIK.ini", key, value ? "true" : "false");
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(".\\Data\\F4SE\\plugins\\FRIK.ini");
		rc = ini.SetBoolValue("Fallout4VRBody", key, value);
		rc = ini.SaveFile(".\\Data\\F4SE\\plugins\\FRIK.ini");
	}
}