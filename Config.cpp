#include "Config.h"
#include "include/SimpleIni.h"
#include "utils.h"
#include "resource.h"
#include "include/json.hpp"

using json = nlohmann::json;

namespace F4VRBody {

	Config* g_config = nullptr;

	constexpr const int FRIK_INI_VERSION = 2;
	
	constexpr const char* FRIK_INI_PATH = ".\\Data\\F4SE\\plugins\\FRIK.ini";
	
	constexpr const char* MESH_HIDE_FACE_INI_PATH = ".\\Data\\F4SE\\plugins\\FRIK_Mesh_Hide\\face.ini";
	constexpr const char* MESH_HIDE_SKINS_INI_PATH = ".\\Data\\F4SE\\plugins\\FRIK_Mesh_Hide\\skins.ini";
	constexpr const char* MESH_HIDE_SLOTS_INI_PATH = ".\\Data\\F4SE\\plugins\\FRIK_Mesh_Hide\\slots.ini";

	constexpr const char* PIPBOY_HOLO_OFFSETS_PATH = ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets\\HoloPipboyPosition.json";
	constexpr const char* PIPBOY_SCREEN_OFFSETS_PATH = ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets\\PipboyPosition.json";
	static const std::string WEAPONS_OFFSETS_PATH{ ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets" };
	
	constexpr const char* INI_SECTION_MAIN = "Fallout4VRBody";
	constexpr const char* INI_SECTION_DEBUG = "Debug";
	constexpr const char* INI_SECTION_SMOOTH_MOVEMENT = "SmoothMovementVR";

	/// <summary>
	/// Runs on every game frame.
	/// Used to reload the config file if the reload interval has passed.
	/// </summary>
	void Config::onUpdateFrame() {
		try {
			if (reloadConfigInterval <= 0)
				return;

			auto now = std::time(nullptr);
			if (now - lastReloadTime < reloadConfigInterval)
				return;

			_VMESSAGE("Reloading FRIK.ini file...");
			lastReloadTime = now;
			loadFrikINI();
			loadHideMeshes();
		}
		catch (const std::exception& e) {
			_WARNING("Failed to reload FRIK.ini file: %s", e.what());
		}
	}

	/// <summary>
	/// Load the FRIK.ini config, hide meshes, and weapon offsets.
	/// Handle creating the FRIK.ini file if it doesn't exist.
	/// Handle updating the FRIK.ini file if the version is old.
	/// </summary>
	void Config::load() {
		// create FRIK.ini if it doesn't exist
		createFileFromResourceIfNotExists(FRIK_INI_PATH, IDR_FRIK_INI);

		loadFrikINI();

		updateLoggerLogLevel();

		if (version < FRIK_INI_VERSION) {
			_MESSAGE("Updating FRIK.ini from version %d to %d", version, FRIK_INI_VERSION);
			updateFrikINIVersion();
		}

		_MESSAGE("Load hide meshes...");
		loadHideMeshes();

		_MESSAGE("Load pipboy offsets...");
		loadPipboyOffsets();

		_MESSAGE("Load weapon offsets...");
		loadWeaponsOffsets();
	}

	void Config::loadFrikINI() {
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH);
		if (rc < 0) {
			throw std::runtime_error("Failed to load FRIK.ini file! Error: " + rc);
		}

		version = ini.GetLongValue(INI_SECTION_DEBUG, "Version", 0);
		logLevel = ini.GetLongValue(INI_SECTION_DEBUG, "LogLevel", 3);
		reloadConfigInterval = ini.GetLongValue(INI_SECTION_DEBUG, "ReloadConfigInterval", 3);
		

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
		armsOnly = ini.GetBoolValue(INI_SECTION_MAIN, "EnableArmsOnlyMode");
		staticGripping = ini.GetBoolValue(INI_SECTION_MAIN, "EnableStaticGripping");
		handUI_X = ini.GetDoubleValue(INI_SECTION_MAIN, "handUI_X", 0.0);
		handUI_Y = ini.GetDoubleValue(INI_SECTION_MAIN, "handUI_Y", 0.0);
		handUI_Z = ini.GetDoubleValue(INI_SECTION_MAIN, "handUI_Z", 0.0);
		hideHead = ini.GetBoolValue(INI_SECTION_MAIN, "HideHead");
		hideEquipment = ini.GetBoolValue(INI_SECTION_MAIN, "HideEquipment");
		hideSkin = ini.GetBoolValue(INI_SECTION_MAIN, "HideSkin");
		pipBoyLookAtGate = ini.GetDoubleValue(INI_SECTION_MAIN, "PipBoyLookAtThreshold", 0.7);
		pipBoyOffDelay = (int)ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOffDelay", 5000);
		pipBoyOnDelay = (int)ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOnDelay", 5000);
		gripLetGoThreshold = ini.GetDoubleValue(INI_SECTION_MAIN, "GripLetGoThreshold", 15.0f);
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
		selfieOutFrontDistance = ini.GetDoubleValue(INI_SECTION_MAIN, "selfieOutFrontDistance", 120.0);
		selfieIgnoreHideFlags = ini.GetBoolValue(INI_SECTION_MAIN, "selfieIgnoreHideFlags", false);

		//Pipboy & Main Config Mode Buttons
		pipBoyScale = (float)ini.GetDoubleValue(INI_SECTION_MAIN, "PipboyScale", 1.0);
		switchUIControltoPrimary = (bool)ini.GetBoolValue(INI_SECTION_MAIN, "PipboyUIPrimaryController", true);
		autoFocusWindow = (bool)ini.GetBoolValue(INI_SECTION_MAIN, "AutoFocusWindow", false);

		// weaponPositioning
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
	/// Update the global logger log level based on the config setting.
	/// </summary>
	void Config::updateLoggerLogLevel() const {
		_MESSAGE("Set log level = %d", logLevel);
		auto level = static_cast<IDebugLog::LogLevel>(logLevel);
		gLog.SetPrintLevel(level);
		gLog.SetLogLevel(level);
	}

	/// <summary>
	/// Current FRIK.ini file is older. Need to update it by:
	/// 1. Overriding the file with the default FRIK.ini resource.
	/// 2. Saving the current config values read from previous FRIK.ini to the new FRIK.ini file.
	/// This preserves the user changed values, including new values and comments, and remove old values completly.
	/// A backup of the previous file is created with the version number for safety.
	/// </summary>
	void Config::updateFrikINIVersion() {
		CSimpleIniA oldIni;
		SI_Error rc = oldIni.LoadFile(FRIK_INI_PATH);
		if (rc < 0) 
			throw std::runtime_error("Failed to load old FRIK.ini file! Error: " + std::to_string(rc));

		// override the file with the default FRIK.ini resource.
		auto tmpIniPath = std::string(FRIK_INI_PATH) + ".tmp";
		createFileFromResourceIfNotExists(tmpIniPath.c_str(), IDR_FRIK_INI);

		CSimpleIniA newIni;
		rc = newIni.LoadFile(tmpIniPath.c_str());
		if (rc < 0)
			throw std::runtime_error("Failed to load new FRIK.ini file! Error: " + std::to_string(rc));

		// remove temp ini file
		std::remove(tmpIniPath.c_str());

		// update all values in the new ini with the old ini values but only if they exists in the new
		std::list<CSimpleIniA::Entry> sectionsList;
		oldIni.GetAllSections(sectionsList);
		for (const auto& section : sectionsList) {
			std::list<CSimpleIniA::Entry> keysList;
			oldIni.GetAllKeys(section.pItem, keysList);
			for (const auto& key : keysList) {
				auto oldVal = oldIni.GetValue(section.pItem, key.pItem);
				auto newVal = newIni.GetValue(section.pItem, key.pItem);
				if (newVal != nullptr && std::strcmp(oldVal, newVal) != 0) {
					_MESSAGE("Migrating %s.%s = %s", section.pItem, key.pItem, oldIni.GetValue(section.pItem, key.pItem));
					newIni.SetValue(section.pItem, key.pItem, oldIni.GetValue(section.pItem, key.pItem));
				}
				else {
					_VMESSAGE("Skipping %s.%s (%s)", section.pItem, key.pItem, newVal == nullptr ? "removed" : "unchanged");
				}
			}
		}

		// set the version to latest
		newIni.SetLongValue(INI_SECTION_DEBUG, "Version", FRIK_INI_VERSION);

		// backup the old ini file before overwritting
		auto nameStr = std::string(FRIK_INI_PATH);
		nameStr = nameStr.replace(nameStr.length() - 4, 4, "_bkp_v" + std::to_string(version) + ".ini");
		int res = std::rename(FRIK_INI_PATH, nameStr.c_str());
		if (rc != 0)
			_WARNING("Failed to backup old FRIK.ini file to '%s'. Error: %d", nameStr, rc);
		
		// save the new ini file
		rc = newIni.SaveFile(FRIK_INI_PATH);
		if (rc < 0)
			throw std::runtime_error("Failed to save post update FRIK.ini file! Error: " + std::to_string(rc));
		
		_MESSAGE("FRIK.ini updated successfully");
	}

	/// <summary>
	/// Load hide meshes from config ini files. Creating if don't exists on the disk.
	/// </summary>
	void Config::loadHideMeshes() {
		createFileFromResourceIfNotExists(MESH_HIDE_FACE_INI_PATH, IDR_MESH_HIDE_FACE);
		faceGeometry = loadListFromFile(MESH_HIDE_FACE_INI_PATH);

		createFileFromResourceIfNotExists(MESH_HIDE_SKINS_INI_PATH, IDR_MESH_HIDE_SKINS);
		skinGeometry = loadListFromFile(MESH_HIDE_SKINS_INI_PATH);
		
		createFileFromResourceIfNotExists(MESH_HIDE_SLOTS_INI_PATH, IDR_MESH_HIDE_SLOTS);
		loadHideEquipmentSlots();
	}

	/// <summary>
	/// Slot Ids base on this link: https://falloutck.uesp.net/wiki/Biped_Slots
	/// </summary>
	void Config::loadHideEquipmentSlots() {
		auto slotsGeometry = loadListFromFile(MESH_HIDE_SLOTS_INI_PATH);

		std::unordered_map<std::string, int> slotToIndexMap = {
			{"hairtop", 0}, // i.e. helmet
			{"hairlong", 1},
			{"head", 2},
			{"headband", 16},
			{"eyes", 17}, // i.e. glasses
			{"beard", 18},
			{"mouth", 19},
			{"neck", 20},
			{"scalp", 22}
		};

		hideEquipSlotIndexes.clear();
		for (auto& geometry : slotsGeometry) {
			if (slotToIndexMap.find(geometry) != slotToIndexMap.end()) {
				hideEquipSlotIndexes.push_back(slotToIndexMap[geometry]);
			}
		}
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
	void Config::saveFrikIniValue(const char* key, bool value) {
		_MESSAGE("Config: Saving \"%s = %s\" to FRIK.ini", key, value ? "true" : "false");
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH);
		rc = ini.SetBoolValue(INI_SECTION_MAIN, key, value);
		rc = ini.SaveFile(FRIK_INI_PATH);
	}

	/// <summary>
	/// Save specific key and double value into FRIK.ini file.
	/// </summary>
	void Config::saveFrikIniValue(const char* key, double value) {
		_MESSAGE("Config: Saving \"%s = %f\" to FRIK.ini", key, value);
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH);
		rc = ini.SetDoubleValue(INI_SECTION_MAIN, key, value);
		rc = ini.SaveFile(FRIK_INI_PATH);
	}

	/// <summary>
	/// Get the Pipboy offset of the currently used Pipboy type.
	/// </summary>
	NiTransform Config::getPipboyOffset() {
		return _pipboyOffsets[isHoloPipboy ? "HoloPipboyPosition" : "PipboyPosition"];
	}

	/// <summary>
	/// Save the Pipboy offset to the offsets map.
	/// </summary>
	void Config::savePipboyOffset(const NiTransform& transform) {
		auto type = isHoloPipboy ? "HoloPipboyPosition" : "PipboyPosition";
		_pipboyOffsets[type] = transform;
		saveOffsetsToJsonFile(type, transform, isHoloPipboy ? PIPBOY_HOLO_OFFSETS_PATH : PIPBOY_SCREEN_OFFSETS_PATH);
	}

	/// <summary>
	/// Get the name for the weapon offset to use depending on the mode.
	/// Basically a hack to store multiple modes of the same weapon by adding suffix to the name.
	/// </summary>
	static std::string getWeaponNameWithMode(const std::string& name, const WeaponOffsetsMode& mode) {
		static const std::string powerArmorSuffix{ "-PowerArmor" };
		static const std::string offHandSuffix{ "-offHand" };
		switch (mode) {
		case normal:
			return name;
		case powerArmor:
			return name + powerArmorSuffix;
		case offHand:
			return name + offHandSuffix;
		case offHandwithPowerArmor:
			return name + offHandSuffix + powerArmorSuffix;
		}
	}

	/// <summary>
	/// Get weapon offsets for given weapon name and mode.
	/// Use non-PA mode if PA mode offsets not found.
	/// </summary>
	/// <returns></returns>
	std::optional<NiTransform> Config::getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode) const {
		auto it = _weaponsOffsets.find(getWeaponNameWithMode(name, mode));
		if (it == _weaponsOffsets.end()) {
			return (mode == powerArmor) || (mode == offHandwithPowerArmor)
				? getWeaponOffsets(name, mode == offHandwithPowerArmor ? offHand : normal) // Check without PA
				: std::nullopt;
		}
		return it->second;
	}

	/// <summary>
	/// Save the weapon offset to config and filesystem.
	/// </summary>
	void Config::saveWeaponOffsets(const std::string& name, const NiTransform& transform, const WeaponOffsetsMode& mode) {
		auto fullName = getWeaponNameWithMode(name, mode);
		_weaponsOffsets[fullName] = transform;
		saveOffsetsToJsonFile(fullName, transform, WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json");
	}

	/// <summary>
	/// Remove the weapon offset from the config and filesystem.
	/// </summary>
	void Config::removeWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode) {
		auto fullName = getWeaponNameWithMode(name, mode);
		_weaponsOffsets.erase(getWeaponNameWithMode(name, mode));
		auto path = WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json";
		_MESSAGE("Removing weapon offsets '%s', file: '%s'", fullName.c_str(), path.c_str());
		if (!std::filesystem::remove(WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json")) {
			_WARNING("Failed to remove weapon offset file: %s", fullName.c_str());
		}
	}

	/// <summary>
	/// Load offset data from given json file path and store it in the given map.
	/// Use the entry key in the json file but for everything to work properly the name of the json should match the key.
	/// </summary>
	static void loadOffsetJsonFile(const std::string& file, std::map<std::string, NiTransform>& offsetsMap) {
		std::ifstream inF;
		inF.open(file, std::ios::in);
		if (inF.fail()) {
			_WARNING("cannot open %s", file.c_str());
			inF.close();
			return;
		}

		json weaponJson;
		try {
			inF >> weaponJson;
		}
		catch (json::parse_error& ex) {
			_MESSAGE("cannot open %s: parse error at byte %d", file.c_str(), ex.byte);
			inF.close();
			return;
		}
		inF.close();

		NiTransform data;
		for (auto& [key, value] : weaponJson.items()) {
			for (int i = 0; i < 12; i++) {
				data.rot.arr[i] = value["rotation"][i].get<double>();
			}
			data.pos.x = value["x"].get<double>();
			data.pos.y = value["y"].get<double>();
			data.pos.z = value["z"].get<double>();
			data.scale = value["scale"].get<double>();

			_MESSAGE("Successfully loaded offset '%s' from '%s'", key.c_str(), file.c_str());
			offsetsMap[key] = data;
		}
	}

	/// <summary>
	/// Load the pipboy screen and holo screen offsets from json files.
	/// </summary>
	void Config::loadPipboyOffsets() {
		createFileFromResourceIfNotExists(PIPBOY_HOLO_OFFSETS_PATH, IDR_PIPBOY_HOLO_OFFSETS);
		createFileFromResourceIfNotExists(PIPBOY_SCREEN_OFFSETS_PATH, IDR_PIPBOY_SCREEN_OFFSETS);
		loadOffsetJsonFile(PIPBOY_HOLO_OFFSETS_PATH, _pipboyOffsets);
		loadOffsetJsonFile(PIPBOY_SCREEN_OFFSETS_PATH, _pipboyOffsets);
	}

	/// <summary>
	/// Load all the weapons offsets found in json files into the weapon offsets map.
	/// </summary>
	void Config::loadWeaponsOffsets() {
		if (!(std::filesystem::exists(WEAPONS_OFFSETS_PATH) && std::filesystem::is_directory(WEAPONS_OFFSETS_PATH))) {
			std::filesystem::create_directory(WEAPONS_OFFSETS_PATH);
		}
		for (const auto& file : std::filesystem::directory_iterator(WEAPONS_OFFSETS_PATH)) {
			std::wstring path = L"";
			try {
				path = file.path().wstring();
			}
			catch (std::exception& e) {
				_WARNING("Unable to convert path to string: %s", e.what());
			}
			if (file.exists() && !file.is_directory()) {
				loadOffsetJsonFile(file.path().string(), _weaponsOffsets);
			}
		}
	}

	/// <summary>
	/// Save the given offsets transform to a json file using the given name.
	/// </summary>
	void Config::saveOffsetsToJsonFile(const std::string& name, const NiTransform& transform, const std::string& file) const {
		_MESSAGE("Saving offsets '%s' to '%s'", name.c_str(), file.c_str());
		json weaponJson;
		weaponJson[name]["rotation"] = transform.rot.arr;
		weaponJson[name]["x"] = transform.pos.x;
		weaponJson[name]["y"] = transform.pos.y;
		weaponJson[name]["z"] = transform.pos.z;
		weaponJson[name]["scale"] = transform.scale;
		
		std::ofstream outF;
		outF.open(file, std::ios::out);
		if (outF.fail()) {
			_MESSAGE("cannot open '%s' for writing", file.c_str());
			return;
		}
		try {
			outF << std::setw(4) << weaponJson;
			outF.close();
		}
		catch (std::exception& e) {
			outF.close();
			_WARNING("Unable to save json '%s': %s", file.c_str(), e.what());
		}
	}
}