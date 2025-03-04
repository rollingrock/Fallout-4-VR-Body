#include "Config.h"
#include "weaponOffset.h"
#include "include/SimpleIni.h"
#include "utils.h"
#include "resource.h"

namespace F4VRBody {

	Config* g_config = nullptr;

	constexpr const char* FRIK_INI_PATH = ".\\Data\\F4SE\\plugins\\FRIK.ini";
	constexpr const char* INI_SECTION_MAIN = "Fallout4VRBody";
	constexpr const char* INI_SECTION_SMOOTH_MOVEMENT = "SmoothMovementVR";

	void Config::load() {
		// load main config
		loadFrikINI();

		// update the log level after reading it from FRIK ini
		updateLoggerLogLevel();

		// load weapon offset JSON
		loadWeaponOffsetsJsons();

		// load hide meshes
		loadHideFace();
		loadHideSkins();
		loadHideSlots();
	}

	void Config::save() const {
		// save main config
		saveFrikINI();

		// save off any weapon offsets
		saveWeaponOffsetsJsons();
	}

	void Config::loadFrikINI() {

		if (!std::filesystem::exists(FRIK_INI_PATH)) {
			_MESSAGE("No existing FRIK.ini file found, creating default...");
			createDefaultFrikINI();
		}

		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH);
		if (rc < 0) {
			throw std::runtime_error("Failed to load FRIK.ini file! Error: " + rc);
		}

		playerHeight = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "PlayerHeight", 120.4828f);
		setScale = ini.GetBoolValue(INI_SECTION_MAIN, "setScale", false);
		fVrScale = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "fVrScale", 70.0);
		playerOffset_forward = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "playerOffset_forward", -4.0);
		playerOffset_up = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "playerOffset_up", -2.0);
		powerArmor_forward = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_forward", 0.0);
		powerArmor_up = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_up", 0.0);
		pipboyDetectionRange = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "pipboyDetectionRange", 15.0);
		armLength = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "armLength", 36.74);
		cameraHeight = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "cameraHeightOffset", 0.0);
		PACameraHeight = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_cameraHeightOffset", 0.0);
		rootOffset = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "RootOffset", 0.0);
		PARootOffset = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_RootOffset", 0.0);
		showPAHUD = ini.GetBoolValue(INI_SECTION_MAIN, "showPAHUD");
		hidePipboy = ini.GetBoolValue(INI_SECTION_MAIN, "hidePipboy");
		leftHandedPipBoy = ini.GetBoolValue(INI_SECTION_MAIN, "PipboyRightArmLeftHandedMode");
		verbose = ini.GetBoolValue(INI_SECTION_MAIN, "VerboseLogging");
		armsOnly = ini.GetBoolValue(INI_SECTION_MAIN, "EnableArmsOnlyMode");
		staticGripping = ini.GetBoolValue(INI_SECTION_MAIN, "EnableStaticGripping");
		handUI_X = ini.GetDoubleValue(INI_SECTION_MAIN, "handUI_X", 0.0);
		handUI_Y = ini.GetDoubleValue(INI_SECTION_MAIN, "handUI_Y", 0.0);
		handUI_Z = ini.GetDoubleValue(INI_SECTION_MAIN, "handUI_Z", 0.0);
		hideHead = ini.GetBoolValue(INI_SECTION_MAIN, "HideHead");
		c_loadedHideHead = hideHead;
		hideEquipment = ini.GetBoolValue(INI_SECTION_MAIN, "HideEquipment");
		c_loadedHideEquipment = hideEquipment;
		hideSkin = ini.GetBoolValue(INI_SECTION_MAIN, "HideSkin");
		c_loadedHideSkin = hideSkin;
		pipBoyLookAtGate = ini.GetDoubleValue(INI_SECTION_MAIN, "PipBoyLookAtThreshold", 0.7);
		pipBoyOffDelay = (int)ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOffDelay", 5000);
		pipBoyOnDelay = (int)ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOnDelay", 5000);
		gripLetGoThreshold = ini.GetDoubleValue(INI_SECTION_MAIN, "GripLetGoThreshold", 15.0f);
		pipBoyButtonMode = ini.GetBoolValue(INI_SECTION_MAIN, "OperatePipboyWithButton", false);
		pipBoyOpenWhenLookAt = ini.GetBoolValue(INI_SECTION_MAIN, "PipBoyOpenWhenLookAt", false);
		pipBoyAllowMovementNotLooking = ini.GetBoolValue(INI_SECTION_MAIN, "AllowMovementWhenNotLookingAtPipboy", true);
		pipBoyButtonArm = (int)ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonArm", 0);
		pipBoyButtonID = (int)ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonID", vr::EVRButtonId::k_EButton_Grip); //2
		pipBoyButtonOffArm = (int)ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonOffArm", 0);
		pipBoyButtonOffID = (int)ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonOffID", vr::EVRButtonId::k_EButton_Grip); //2		
		isHoloPipboy = (bool)ini.GetBoolValue(INI_SECTION_MAIN, "HoloPipBoyEnabled", true);
		isPipBoyTorchOnArm = (bool)ini.GetBoolValue(INI_SECTION_MAIN, "PipBoyTorchOnArm", true);
		switchTorchButton = (int)ini.GetLongValue(INI_SECTION_MAIN, "SwitchTorchButton", 2);
		gripButtonID = (int)ini.GetLongValue(INI_SECTION_MAIN, "GripButtonID", vr::EVRButtonId::k_EButton_Grip); // 2
		enableOffHandGripping = ini.GetBoolValue(INI_SECTION_MAIN, "EnableOffHandGripping", true);
		enableGripButtonToGrap = ini.GetBoolValue(INI_SECTION_MAIN, "EnableGripButton", true);
		enableGripButtonToLetGo = ini.GetBoolValue(INI_SECTION_MAIN, "EnableGripButtonToLetGo", true);
		onePressGripButton = ini.GetBoolValue(INI_SECTION_MAIN, "EnableGripButtonOnePress", true);
		dampenHands = ini.GetBoolValue(INI_SECTION_MAIN, "DampenHands", true);
		dampenHandsInVanillaScope = ini.GetBoolValue(INI_SECTION_MAIN, "DampenHandsInVanillaScope", true);
		dampenPipboyScreen = ini.GetBoolValue(INI_SECTION_MAIN, "DampenPipboyScreen", true);
		dampenHandsRotation = ini.GetDoubleValue(INI_SECTION_MAIN, "DampenHandsRotation", 0.7);
		dampenHandsTranslation = ini.GetDoubleValue(INI_SECTION_MAIN, "DampenHandsTranslation", 0.7);
		dampenHandsRotationInVanillaScope = ini.GetDoubleValue(INI_SECTION_MAIN, "DampenHandsRotationInVanillaScope", 0.2);
		dampenHandsTranslationInVanillaScope = ini.GetDoubleValue(INI_SECTION_MAIN, "DampenHandsTranslationInVanillaScope", 0.2);
		dampenPipboyRotation = ini.GetDoubleValue(INI_SECTION_MAIN, "DampenPipboyRotation", 0.7);
		dampenPipboyTranslation = ini.GetDoubleValue(INI_SECTION_MAIN, "DampenPipboyTranslation", 0.7);
		directionalDeadzone = ini.GetDoubleValue(INI_SECTION_MAIN, "fDirectionalDeadzone", 0.5);
		playerHMDHeight = ini.GetDoubleValue(INI_SECTION_MAIN, "fHMDHeight", 109.0);
		shoulderToHMD = ini.GetDoubleValue(INI_SECTION_MAIN, "fShouldertoHMD", 109.0);

		//Pipboy & Main Config Mode Buttons
		pipBoyScale = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "PipboyScale", 1.0);
		switchUIControltoPrimary = (bool)ini.GetBoolValue(INI_SECTION_MAIN, "PipboyUIPrimaryController", true);
		autoFocusWindow = (bool)ini.GetBoolValue(INI_SECTION_MAIN, "AutoFocusWindow", false);
		UISelfieButton = (int)ini.GetLongValue("ConfigModeUIButtons", "ToggleSelfieModeButton", 2);

		// weaponPositioning
		//c_weaponRepositionMasterMode = ini.GetBoolValue(INI_SECTION_MAIN, "EnableRepositionMode", false);       Enabled / Disabled via config mode
		holdDelay = (int)ini.GetLongValue(INI_SECTION_MAIN, "HoldDelay", 1000);
		repositionButtonID = (int)ini.GetLongValue(INI_SECTION_MAIN, "RepositionButtonID", vr::EVRButtonId::k_EButton_SteamVR_Trigger); // 33
		offHandActivateButtonID = (int)ini.GetLongValue(INI_SECTION_MAIN, "OffHandActivateButtonID", vr::EVRButtonId::k_EButton_A); // 7
		scopeAdjustDistance = ini.GetDoubleValue(INI_SECTION_MAIN, "ScopeAdjustDistance", 15.f);

		//Smooth Movement
		disableSmoothMovement = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableSmoothMovement");
		smoothingAmount = (float)ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "SmoothAmount", 15.0);
		smoothingAmountHorizontal = (float)ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "SmoothAmountHorizontal", 5.0);
		dampingMultiplier = (float)ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "Damping", 1.0);
		dampingMultiplierHorizontal = (float)ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "DampingHorizontal", 1.0);
		stoppingMultiplier = (float)ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "StoppingMultiplier", 0.6);
		stoppingMultiplierHorizontal = (float)ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "StoppingMultiplierHorizontal", 0.6);
		disableInteriorSmoothing = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableInteriorSmoothing", 1);
		disableInteriorSmoothingHorizontal = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableInteriorSmoothingHorizontal", 1);
	}

	/// <summary>
	/// Get default FRIK.ini as dll resource and write it to the FRIK.ini file.
	/// </summary>
	void Config::createDefaultFrikINI() {

		auto data = getEmbededResourceAsString(IDR_FRIK_INI);

		std::ofstream outFile(FRIK_INI_PATH);
		if (!outFile) {
			throw std::runtime_error("Failed to create FRIK.ini file");
		}
		if (!outFile.write(data.data(), data.size())) {
			outFile.close();
			std::remove(FRIK_INI_PATH);
			throw std::runtime_error("Failed to write to FRIK.ini file");
		}
		outFile.close();

		_MESSAGE("Default FRIK.ini file created successfully (size: %d)", data.size());
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

	/// <summary>
	/// Update the global logger log level based on the config setting.
	/// </summary>
	void Config::updateLoggerLogLevel() const {
		auto logLevel = verbose ? IDebugLog::kLevel_DebugMessage : IDebugLog::kLevel_Message;
		_MESSAGE("Set log level = %d", logLevel);
		gLog.SetPrintLevel(logLevel);
		gLog.SetLogLevel(logLevel);
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

	void Config::saveFrikINI() const
	{
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH);

		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "PlayerHeight", (double)playerHeight);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "fVrScale", (double)fVrScale);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "playerOffset_forward", (double)playerOffset_forward);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "playerOffset_up", (double)playerOffset_up);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_forward", (double)powerArmor_forward);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_up", (double)powerArmor_up);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "armLength", (double)armLength);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "cameraHeightOffset", (double)cameraHeight);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_cameraHeightOffset", (double)PACameraHeight);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, "showPAHUD", showPAHUD);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, "hidePipboy", hidePipboy);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "PipboyScale", (double)pipBoyScale);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, "HoloPipBoyEnabled", isHoloPipboy);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, "PipBoyTorchOnArm", isPipBoyTorchOnArm);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, "EnableArmsOnlyMode", armsOnly);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, "EnableStaticGripping", staticGripping);
		//rc = ini.SetBoolValue(INI_SECTION_MAIN, "EnableRepositionMode", c_weaponRepositionMasterMode);  Handled by Config Mode. 
		rc = ini.SetBoolValue(INI_SECTION_MAIN, "HideTheHead", hideHead);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "handUI_X", handUI_X);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "handUI_Y", handUI_Y);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "handUI_Z", handUI_Z);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, "DampenHands", dampenHands);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "DampenHandsRotation", dampenHandsRotation);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "DampenHandsTranslation", dampenHandsTranslation);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_RootOffset", (double)PARootOffset);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "RootOffset", (double)rootOffset);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "fHMDHeight", (double)playerHMDHeight);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, "fShouldertoHMD", (double)shoulderToHMD);
		
		rc = ini.SaveFile(FRIK_INI_PATH);

		if (rc < 0) {
			_ERROR("Config: Failed to save FRIK.ini. Error: %d", rc);
		}
		else {
			_MESSAGE("Config: Saving FRIK.ini successful");
		}
	}

	/// <summary>
	/// Save specific key and bool value into FRIK.ini file.
	/// </summary>
	void Config::saveBoolValue(const char* key, bool value) {
		_MESSAGE("Config: Saving \"%s = %s\" to FRIK.ini", key, value ? "true" : "false");
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, key, value);
		rc = ini.SaveFile(FRIK_INI_PATH);
	}
}