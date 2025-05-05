#include <fstream>
#include "include/json.hpp"
#include "include/SimpleIni.h"

#include "Config.h"

#include <filesystem>
#include <shlobj_core.h>

#include "utils.h"
#include "res/resource.h"

using json = nlohmann::json;

namespace FRIK {
	Config* g_config = nullptr;

	constexpr int FRIK_INI_VERSION = 7;

	static const auto BASE_PATH = getRelativePathInDocuments(R"(\My Games\Fallout4VR\FRIK_Config)");
	static const auto FRIK_INI_PATH = BASE_PATH + R"(\FRIK.ini)";
	static const auto MESH_HIDE_FACE_INI_PATH = BASE_PATH + R"(\Mesh_Hide\face.ini)";
	static const auto MESH_HIDE_SKINS_INI_PATH = BASE_PATH + R"(\Mesh_Hide\skins.ini)";
	static const auto MESH_HIDE_SLOTS_INI_PATH = BASE_PATH + R"(\Mesh_Hide\slots.ini)";
	static const auto PIPBOY_HOLO_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\HoloPipboyPosition.json)";
	static const auto PIPBOY_SCREEN_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\PipboyPosition.json)";
	static const auto WEAPONS_OFFSETS_PATH = BASE_PATH + R"(\Weapons_Offsets)";

	/// <summary>
	/// Open the FRIK.ini file in Notepad for editing.
	/// </summary>
	void Config::openInNotepad() {
		ShellExecute(nullptr, "open", "notepad.exe", FRIK_INI_PATH.c_str(), nullptr, SW_SHOWNORMAL);
	}

	/// <summary>
	/// Check if debug data dump is requested for the given name.
	/// If matched, the name will be removed from the list to prevent multiple dumps.
	/// Also saved into INI to prevent reloading the same dump name on next config reload.
	/// Support specifying multiple names by any separator as only the matched sub-string is removed.
	/// </summary>
	bool Config::checkDebugDumpDataOnceFor(const char* name) {
		const auto idx = _debugDumpDataOnceNames.find(name);
		if (idx == std::string::npos) {
			return false;
		}
		_debugDumpDataOnceNames = _debugDumpDataOnceNames.erase(idx, strlen(name));
		// write to INI for auto-reload not to re-enable it
		saveFrikIniValue(INI_SECTION_DEBUG, "DebugDumpDataOnceNames", _debugDumpDataOnceNames.c_str());

		_MESSAGE("---- Debug Dump Data check passed for '%s' ----", name);
		return true;
	}

	/// <summary>
	/// Runs on every game frame.
	/// Used to reload the config file if the reload interval has passed.
	/// </summary>
	void Config::onUpdateFrame() {
		try {
			if (_reloadConfigInterval <= 0) {
				return;
			}

			const auto now = std::time(nullptr);
			if (now - lastReloadTime < _reloadConfigInterval) {
				return;
			}

			_VMESSAGE("Reloading FRIK.ini file...");
			lastReloadTime = now;
			loadFrikINI();
			loadHideMeshes();
		} catch (const std::exception& e) {
			_WARNING("Failed to reload FRIK.ini file: %s", e.what());
		}
	}

	/// <summary>
	/// Load the FRIK.ini config, hide meshes, and weapon offsets.
	/// Handle creating the FRIK.ini file if it doesn't exist.
	/// Handle updating the FRIK.ini file if the version is old.
	/// </summary>
	void Config::load() {
		setupFolders();
		migrateConfigFilesIfNeeded();

		// create FRIK.ini if it doesn't exist
		createFileFromResourceIfNotExists(FRIK_INI_PATH, IDR_FRIK_INI, true);

		loadFrikINI();

		updateLoggerLogLevel();

		if (version < FRIK_INI_VERSION) {
			_MESSAGE("Updating FRIK.ini version %d -> %d", version, FRIK_INI_VERSION);
			updateFrikINIVersion();
			// reload the config after update
			loadFrikINI();
		}

		_MESSAGE("Load hide meshes...");
		loadHideMeshes();

		_MESSAGE("Load pipboy offsets...");
		loadPipboyOffsets();

		_MESSAGE("Load weapon embedded offsets...");
		loadWeaponsOffsetsFromEmbedded();

		_MESSAGE("Load weapon custom offsets...");
		loadWeaponsOffsetsFromFilesystem();
	}

	void Config::loadFrikINI() {
		CSimpleIniA ini;
		const SI_Error rc = ini.LoadFile(FRIK_INI_PATH.c_str());
		if (rc < 0) {
			throw std::runtime_error("Failed to load FRIK.ini file! Error: " + rc);
		}

		version = ini.GetLongValue(INI_SECTION_DEBUG, "Version", 0);
		logLevel = ini.GetLongValue(INI_SECTION_DEBUG, "LogLevel", 3);
		_reloadConfigInterval = ini.GetLongValue(INI_SECTION_DEBUG, "ReloadConfigInterval", 3);
		debugFlowFlag1 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "DebugFlowFlag1", 0));
		debugFlowFlag2 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "DebugFlowFlag2", 0));
		debugFlowFlag3 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "DebugFlowFlag3", 0));
		_debugDumpDataOnceNames = ini.GetValue(INI_SECTION_DEBUG, "DebugDumpDataOnceNames", "");

		playerHeight = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "PlayerHeight", 120.4828f));
		setScale = ini.GetBoolValue(INI_SECTION_MAIN, "setScale", false);
		fVrScale = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "fVrScale", 70.0));
		playerOffset_forward = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "playerOffset_forward", -4.0));
		playerOffset_up = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "playerOffset_up", -2.0));
		powerArmor_forward = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_forward", 0.0));
		powerArmor_up = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_up", 0.0));
		pipboyDetectionRange = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "pipboyDetectionRange", 15.0));
		armLength = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "armLength", 36.74));
		cameraHeight = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "cameraHeightOffset", 0.0));
		PACameraHeight = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_cameraHeightOffset", 0.0));
		rootOffset = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "RootOffset", 0.0));
		PARootOffset = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_RootOffset", 0.0));
		showPAHUD = ini.GetBoolValue(INI_SECTION_MAIN, "showPAHUD");
		hidePipboy = ini.GetBoolValue(INI_SECTION_MAIN, "hidePipboy");
		leftHandedPipBoy = ini.GetBoolValue(INI_SECTION_MAIN, "PipboyRightArmLeftHandedMode");
		armsOnly = ini.GetBoolValue(INI_SECTION_MAIN, "EnableArmsOnlyMode");
		hideHead = ini.GetBoolValue(INI_SECTION_MAIN, "HideHead");
		hideEquipment = ini.GetBoolValue(INI_SECTION_MAIN, "HideEquipment");
		hideSkin = ini.GetBoolValue(INI_SECTION_MAIN, "HideSkin");
		pipBoyLookAtGate = ini.GetDoubleValue(INI_SECTION_MAIN, "PipBoyLookAtThreshold", 0.7);
		pipBoyOffDelay = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOffDelay", 5000));
		pipBoyOnDelay = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOnDelay", 5000));
		gripLetGoThreshold = ini.GetDoubleValue(INI_SECTION_MAIN, "GripLetGoThreshold", 15.0f);
		pipBoyOpenWhenLookAt = ini.GetBoolValue(INI_SECTION_MAIN, "PipBoyOpenWhenLookAt", false);
		pipBoyAllowMovementNotLooking = ini.GetBoolValue(INI_SECTION_MAIN, "AllowMovementWhenNotLookingAtPipboy", true);
		pipBoyButtonArm = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonArm", 0));
		pipBoyButtonID = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonID", vr::EVRButtonId::k_EButton_Grip)); //2
		pipBoyButtonOffArm = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonOffArm", 0));
		pipBoyButtonOffID = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonOffID", vr::EVRButtonId::k_EButton_Grip)); //2		
		isHoloPipboy = ini.GetBoolValue(INI_SECTION_MAIN, "HoloPipBoyEnabled", true);
		isPipBoyTorchOnArm = ini.GetBoolValue(INI_SECTION_MAIN, "PipBoyTorchOnArm", true);
		isPipBoyTorchRightArmMode = ini.GetBoolValue(INI_SECTION_MAIN, "PipBoyTorchRightArmMode", false);
		switchTorchButton = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "SwitchTorchButton", 2));
		gripButtonID = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "GripButtonID", vr::EVRButtonId::k_EButton_Grip)); // 2
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
		selfieOutFrontDistance = ini.GetDoubleValue(INI_SECTION_MAIN, "selfieOutFrontDistance", 120.0);
		selfieIgnoreHideFlags = ini.GetBoolValue(INI_SECTION_MAIN, "selfieIgnoreHideFlags", false);

		//Pipboy & Main Config Mode Buttons
		pipBoyScale = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "PipboyScale", 1.0));
		switchUIControltoPrimary = ini.GetBoolValue(INI_SECTION_MAIN, "PipboyUIPrimaryController", true);
		autoFocusWindow = ini.GetBoolValue(INI_SECTION_MAIN, "AutoFocusWindow", false);

		// weaponPositioning
		holdDelay = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "HoldDelay", 1000));
		repositionButtonID = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "RepositionButtonID", vr::EVRButtonId::k_EButton_SteamVR_Trigger)); // 33
		offHandActivateButtonID = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OffHandActivateButtonID", vr::EVRButtonId::k_EButton_A)); // 7
		scopeAdjustDistance = ini.GetDoubleValue(INI_SECTION_MAIN, "ScopeAdjustDistance", 15.f);

		//Smooth Movement
		disableSmoothMovement = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableSmoothMovement");
		smoothingAmount = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "SmoothAmount", 15.0));
		smoothingAmountHorizontal = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "SmoothAmountHorizontal", 5.0));
		dampingMultiplier = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "Damping", 1.0));
		dampingMultiplierHorizontal = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "DampingHorizontal", 1.0));
		stoppingMultiplier = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "StoppingMultiplier", 0.6));
		stoppingMultiplierHorizontal = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "StoppingMultiplierHorizontal", 0.6));
		disableInteriorSmoothing = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableInteriorSmoothing", true);
		disableInteriorSmoothingHorizontal = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableInteriorSmoothingHorizontal", true);
	}

	/// <summary>
	/// Update the global logger log level based on the config setting.
	/// </summary>
	void Config::updateLoggerLogLevel() const {
		_MESSAGE("Set log level = %d", logLevel);
		const auto level = static_cast<IDebugLog::LogLevel>(logLevel);
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
	void Config::updateFrikINIVersion() const {
		CSimpleIniA oldIni;
		SI_Error rc = oldIni.LoadFile(FRIK_INI_PATH.c_str());
		if (rc < 0) {
			throw std::runtime_error("Failed to load old FRIK.ini file! Error: " + std::to_string(rc));
		}

		// override the file with the default FRIK.ini resource.
		const auto tmpIniPath = std::string(FRIK_INI_PATH) + ".tmp";
		createFileFromResourceIfNotExists(tmpIniPath, IDR_FRIK_INI, true);

		CSimpleIniA newIni;
		rc = newIni.LoadFile(tmpIniPath.c_str());
		if (rc < 0) {
			throw std::runtime_error("Failed to load new FRIK.ini file! Error: " + std::to_string(rc));
		}

		// remove temp ini file
		std::remove(tmpIniPath.c_str());

		// update all values in the new ini with the old ini values but only if they exist in the new
		std::list<CSimpleIniA::Entry> sectionsList;
		oldIni.GetAllSections(sectionsList);
		for (const auto& section : sectionsList) {
			std::list<CSimpleIniA::Entry> keysList;
			oldIni.GetAllKeys(section.pItem, keysList);
			for (const auto& key : keysList) {
				const auto oldVal = oldIni.GetValue(section.pItem, key.pItem);
				const auto newVal = newIni.GetValue(section.pItem, key.pItem);
				if (newVal != nullptr && std::strcmp(oldVal, newVal) != 0) {
					_MESSAGE("Migrating %s.%s = %s", section.pItem, key.pItem, oldIni.GetValue(section.pItem, key.pItem));
					newIni.SetValue(section.pItem, key.pItem, oldIni.GetValue(section.pItem, key.pItem));
				} else {
					_VMESSAGE("Skipping %s.%s (%s)", section.pItem, key.pItem, newVal == nullptr ? "removed" : "unchanged");
				}
			}
		}

		// set the version to latest
		newIni.SetLongValue(INI_SECTION_DEBUG, "Version", FRIK_INI_VERSION);

		// backup the old ini file before overwriting
		auto nameStr = std::string(FRIK_INI_PATH);
		nameStr = nameStr.replace(nameStr.length() - 4, 4, "_bkp_v" + std::to_string(version) + ".ini");
		int res = std::rename(FRIK_INI_PATH.c_str(), nameStr.c_str());
		if (rc != 0) {
			_WARNING("Failed to backup old FRIK.ini file to '%s'. Error: %d", nameStr, rc);
		}

		// save the new ini file
		rc = newIni.SaveFile(FRIK_INI_PATH.c_str());
		if (rc < 0) {
			throw std::runtime_error("Failed to save post update FRIK.ini file! Error: " + std::to_string(rc));
		}

		_MESSAGE("FRIK.ini updated successfully");
	}

	/// <summary>
	/// Load hide meshes from config ini files. Creating if don't exists on the disk.
	/// </summary>
	void Config::loadHideMeshes() {
		createFileFromResourceIfNotExists(MESH_HIDE_FACE_INI_PATH, IDR_MESH_HIDE_FACE, true);
		faceGeometry = loadListFromFile(MESH_HIDE_FACE_INI_PATH);

		createFileFromResourceIfNotExists(MESH_HIDE_SKINS_INI_PATH, IDR_MESH_HIDE_SKINS, true);
		skinGeometry = loadListFromFile(MESH_HIDE_SKINS_INI_PATH);

		createFileFromResourceIfNotExists(MESH_HIDE_SLOTS_INI_PATH, IDR_MESH_HIDE_SLOTS, true);
		loadHideEquipmentSlots();
	}

	/// <summary>
	/// Slot Ids base on this link: https://falloutck.uesp.net/wiki/Biped_Slots
	/// </summary>
	void Config::loadHideEquipmentSlots() {
		const auto slotsGeometry = loadListFromFile(MESH_HIDE_SLOTS_INI_PATH);

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
			if (slotToIndexMap.contains(geometry)) {
				hideEquipSlotIndexes.push_back(slotToIndexMap[geometry]);
			}
		}
	}

	void Config::saveFrikINI() const {
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH.c_str());

		ini.SetDoubleValue(INI_SECTION_MAIN, "fVrScale", fVrScale);
		ini.SetDoubleValue(INI_SECTION_MAIN, "playerOffset_forward", playerOffset_forward);
		ini.SetDoubleValue(INI_SECTION_MAIN, "playerOffset_up", playerOffset_up);
		ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_forward", powerArmor_forward);
		ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_up", powerArmor_up);
		ini.SetDoubleValue(INI_SECTION_MAIN, "armLength", armLength);
		ini.SetDoubleValue(INI_SECTION_MAIN, "cameraHeightOffset", cameraHeight);
		ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_cameraHeightOffset", PACameraHeight);
		ini.SetBoolValue(INI_SECTION_MAIN, "showPAHUD", showPAHUD);
		ini.SetBoolValue(INI_SECTION_MAIN, "hidePipboy", hidePipboy);
		ini.SetDoubleValue(INI_SECTION_MAIN, "PipboyScale", pipBoyScale);
		ini.SetBoolValue(INI_SECTION_MAIN, "HoloPipBoyEnabled", isHoloPipboy);
		ini.SetBoolValue(INI_SECTION_MAIN, "PipBoyTorchOnArm", isPipBoyTorchOnArm);
		ini.SetBoolValue(INI_SECTION_MAIN, "DampenPipboyScreen", dampenPipboyScreen);
		ini.SetBoolValue(INI_SECTION_MAIN, "PipBoyOpenWhenLookAt", pipBoyOpenWhenLookAt);
		ini.SetBoolValue(INI_SECTION_MAIN, "DampenHands", dampenHands);
		ini.SetDoubleValue(INI_SECTION_MAIN, "DampenHandsRotation", dampenHandsRotation);
		ini.SetDoubleValue(INI_SECTION_MAIN, "DampenHandsTranslation", dampenHandsTranslation);
		ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_RootOffset", PARootOffset);
		ini.SetDoubleValue(INI_SECTION_MAIN, "RootOffset", rootOffset);
		ini.SetBoolValue(INI_SECTION_MAIN, "EnableGripButton", enableGripButtonToGrap);
		ini.SetBoolValue(INI_SECTION_MAIN, "EnableGripButtonToLetGo", enableGripButtonToLetGo);
		ini.SetBoolValue(INI_SECTION_MAIN, "EnableGripButtonOnePress", onePressGripButton);

		rc = ini.SaveFile(FRIK_INI_PATH.c_str());
		if (rc < 0) {
			_ERROR("Config: Failed to save FRIK.ini. Error: %d", rc);
		} else {
			_MESSAGE("Config: Saving FRIK.ini successful");
		}
	}

	/// <summary>
	/// Save specific key and bool value into FRIK.ini file.
	/// </summary>
	void Config::saveFrikIniValue(const char* section, const char* key, const bool value) {
		_MESSAGE("Config: Saving \"%s = %s\" to FRIK.ini", key, value ? "true" : "false");
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH.c_str());
		rc = ini.SetBoolValue(section, key, value);
		rc = ini.SaveFile(FRIK_INI_PATH.c_str());
	}

	/// <summary>
	/// Save specific key and double value into FRIK.ini file.
	/// </summary>
	void Config::saveFrikIniValue(const char* section, const char* key, const double value) {
		_MESSAGE("Config: Saving \"%s = %f\" to FRIK.ini", key, value);
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH.c_str());
		rc = ini.SetDoubleValue(section, key, value);
		rc = ini.SaveFile(FRIK_INI_PATH.c_str());
	}

	/// <summary>
	/// Save specific key and string value into FRIK.ini file.
	/// </summary>
	void Config::saveFrikIniValue(const char* section, const char* key, const char* value) {
		_MESSAGE("Config: Saving \"%s = %s\" to FRIK.ini", key, value);
		CSimpleIniA ini;
		SI_Error rc = ini.LoadFile(FRIK_INI_PATH.c_str());
		rc = ini.SetValue(section, key, value);
		rc = ini.SaveFile(FRIK_INI_PATH.c_str());
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
		const auto type = isHoloPipboy ? "HoloPipboyPosition" : "PipboyPosition";
		_pipboyOffsets[type] = transform;
		saveOffsetsToJsonFile(type, transform, isHoloPipboy ? PIPBOY_HOLO_OFFSETS_PATH : PIPBOY_SCREEN_OFFSETS_PATH);
	}

	/// <summary>
	/// Get the name for the weapon offset to use depending on the mode.
	/// Basically a hack to store multiple modes of the same weapon by adding suffix to the name.
	/// </summary>
	static std::string getWeaponNameWithMode(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA, const bool leftHanded) {
		static const std::string POWER_ARMOR_SUFFIX{"-PowerArmor"};
		static const std::string OFF_HAND_SUFFIX{"-offHand"};
		static const std::string THROWABLE_SUFFIX{"-throwable"};
		static const std::string BACK_OF_HAND_SUFFIX{"-backOfHand"};
		static const std::string LEFT_HANDED_SUFFIX{"-leftHanded"};
		switch (mode) {
		case WeaponOffsetsMode::Weapon:
			return name + (inPA ? POWER_ARMOR_SUFFIX : "") + (leftHanded ? LEFT_HANDED_SUFFIX : "");
		case WeaponOffsetsMode::OffHand:
			return name + OFF_HAND_SUFFIX + (inPA ? POWER_ARMOR_SUFFIX : "") + (leftHanded ? LEFT_HANDED_SUFFIX : "");
		case WeaponOffsetsMode::Throwable:
			return name + THROWABLE_SUFFIX + (inPA ? POWER_ARMOR_SUFFIX : "") + (leftHanded ? LEFT_HANDED_SUFFIX : "");
		case WeaponOffsetsMode::BackOfHandUI:
			return name + BACK_OF_HAND_SUFFIX + (inPA ? POWER_ARMOR_SUFFIX : "") + (leftHanded ? LEFT_HANDED_SUFFIX : "");
		}
	}

	/// <summary>
	/// Get weapon offsets for given weapon name and mode.
	/// Use non-PA mode if PA mode offsets not found.
	/// </summary>
	/// <returns></returns>
	std::optional<NiTransform> Config::getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA) const {
		const auto it = _weaponsOffsets.find(getWeaponNameWithMode(name, mode, inPA, leftHandedMode));
		if (it != _weaponsOffsets.end()) {
			return it->second;
		}
		// Check without PA (historic)
		return inPA ? getWeaponOffsets(name, mode, false) : std::nullopt;
	}

	/// <summary>
	/// Save the weapon offset to config and filesystem.
	/// </summary>
	void Config::saveWeaponOffsets(const std::string& name, const NiTransform& transform, const WeaponOffsetsMode& mode, const bool inPA) {
		const auto fullName = getWeaponNameWithMode(name, mode, inPA, leftHandedMode);
		_weaponsOffsets[fullName] = transform;
		saveOffsetsToJsonFile(fullName, transform, WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json");
	}

	/// <summary>
	/// Remove the weapon offset from the config and filesystem.
	/// </summary>
	void Config::removeWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA, const bool replaceWithEmbedded) {
		const auto fullName = getWeaponNameWithMode(name, mode, inPA, leftHandedMode);
		_weaponsOffsets.erase(fullName);
		if (replaceWithEmbedded && _weaponsEmbeddedOffsets.contains(fullName)) {
			_weaponsOffsets[fullName] = _weaponsEmbeddedOffsets[fullName];
		}

		const auto path = WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json";
		_MESSAGE("Removing weapon offsets '%s', file: '%s'", fullName.c_str(), path.c_str());
		if (!std::filesystem::remove(WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json")) {
			_WARNING("Failed to remove weapon offset file: %s", fullName.c_str());
		}
	}

	/**
	 * Load the given json object with offset data into an offset map.
	 */
	static void loadOffsetJsonToMap(const json& json, std::map<std::string, NiTransform>& offsetsMap, const bool log) {
		for (auto& [key, value] : json.items()) {
			NiTransform data;
			for (int i = 0; i < 12; i++) {
				data.rot.arr[i] = value["rotation"][i].get<double>();
			}
			data.pos.x = value["x"].get<double>();
			data.pos.y = value["y"].get<double>();
			data.pos.z = value["z"].get<double>();
			data.scale = value["scale"].get<double>();

			if (log) {
				_MESSAGE("Successfully loaded offset override '%s' (%s)", key.c_str(), offsetsMap.contains(key) ? "Override" : "New");
			}
			offsetsMap[key] = data;
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
		} catch (json::parse_error& ex) {
			_MESSAGE("cannot open %s: parse error at byte %d", file.c_str(), ex.byte);
			inF.close();
			return;
		}
		inF.close();

		loadOffsetJsonToMap(weaponJson, offsetsMap, true);
	}

	/// <summary>
	/// Load the pipboy screen and holo screen offsets from json files.
	/// </summary>
	void Config::loadPipboyOffsets() {
		createFileFromResourceIfNotExists(PIPBOY_HOLO_OFFSETS_PATH, IDR_PIPBOY_HOLO_OFFSETS, false);
		createFileFromResourceIfNotExists(PIPBOY_SCREEN_OFFSETS_PATH, IDR_PIPBOY_SCREEN_OFFSETS, false);
		loadOffsetJsonFile(PIPBOY_HOLO_OFFSETS_PATH, _pipboyOffsets);
		loadOffsetJsonFile(PIPBOY_SCREEN_OFFSETS_PATH, _pipboyOffsets);
	}

	/**
	 * Load all embedded weapons offsets.
	 */
	void Config::loadWeaponsOffsetsFromEmbedded() {
		for (WORD resourceId = 200; resourceId < 400; resourceId++) {
			auto resourceOpt = getEmbeddedResourceAsStringIfExists(resourceId);
			if (!resourceOpt.has_value()) {
				break;
			}
			json json = json::parse(resourceOpt.value());
			loadOffsetJsonToMap(json, _weaponsEmbeddedOffsets, false);
		}
		_weaponsOffsets.insert(_weaponsEmbeddedOffsets.begin(), _weaponsEmbeddedOffsets.end());
		_MESSAGE("Loaded (%d) embedded weapon offsets", _weaponsOffsets.size());
	}

	/// <summary>
	/// Load all the weapons offsets found in json files into the weapon offsets map.
	/// </summary>
	void Config::loadWeaponsOffsetsFromFilesystem() {
		for (const auto& file : std::filesystem::directory_iterator(WEAPONS_OFFSETS_PATH)) {
			if (file.exists() && !file.is_directory()) {
				loadOffsetJsonFile(file.path().string(), _weaponsOffsets);
			}
		}
		_MESSAGE("Loaded (%d) total weapon offsets", _weaponsOffsets.size());
	}

	/// <summary>
	/// Save the given offsets transform to a json file using the given name.
	/// </summary>
	void Config::saveOffsetsToJsonFile(const std::string& name, const NiTransform& transform, const std::string& file) {
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
		} catch (std::exception& e) {
			outF.close();
			_WARNING("Unable to save json '%s': %s", file.c_str(), e.what());
		}
	}

	/// <summary>
	/// Create all the folders needed for config to not handle later creating folders that don't exists.
	/// </summary>
	void Config::setupFolders() {
		createDirDeep(FRIK_INI_PATH);
		createDirDeep(MESH_HIDE_FACE_INI_PATH);
		createDirDeep(PIPBOY_HOLO_OFFSETS_PATH);
		createDirDeep(WEAPONS_OFFSETS_PATH);
	}

	/**
	 * One time migration of config files (ini,json) to common location to handle MO2 overwrite.
	 * See https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Development#frik-config-folder
	 * and https://github.com/rollingrock/Fallout-4-VR-Body/pull/80
	 * Always migrate config to handle custom weapon offsets at mod-list installation time.
	 */
	void Config::migrateConfigFilesIfNeeded() {
		// migrate pre v72 and v72 config files to v73 location
		_MESSAGE("Migrate configs if exists in old locations...");
		moveFileSafe(R"(.\Data\F4SE\plugins\FRIK.ini)", FRIK_INI_PATH);
		moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_Mesh_Hide\face.ini)", MESH_HIDE_FACE_INI_PATH);
		moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_Mesh_Hide\skins.ini)", MESH_HIDE_SKINS_INI_PATH);
		moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_Mesh_Hide\slots.ini)", MESH_HIDE_SLOTS_INI_PATH);
		moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_weapon_offsets\HoloPipboyPosition.json)", PIPBOY_HOLO_OFFSETS_PATH);
		moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_weapon_offsets\PipboyPosition.json)", PIPBOY_SCREEN_OFFSETS_PATH);
		moveAllFilesInFolderSafe(R"(.\Data\F4SE\plugins\FRIK_weapon_offsets)", WEAPONS_OFFSETS_PATH);

		// migrate v72 config files to v73 location
		moveFileSafe(R"(.\Data\FRIK_Config\FRIK.ini)", FRIK_INI_PATH);
		moveFileSafe(R"(.\Data\FRIK_Config\Mesh_Hide\face.ini)", MESH_HIDE_FACE_INI_PATH);
		moveFileSafe(R"(.\Data\FRIK_Config\Mesh_Hide\skins.ini)", MESH_HIDE_SKINS_INI_PATH);
		moveFileSafe(R"(.\Data\FRIK_Config\Mesh_Hide\slots.ini)", MESH_HIDE_SLOTS_INI_PATH);
		moveFileSafe(R"(.\Data\FRIK_Config\Pipboy_Offsets\HoloPipboyPosition.json)", PIPBOY_HOLO_OFFSETS_PATH);
		moveFileSafe(R"(.\Data\FRIK_Config\Pipboy_Offsets\PipboyPosition.json)", PIPBOY_SCREEN_OFFSETS_PATH);
		moveAllFilesInFolderSafe(R"(.\Data\FRIK_Config\Weapons_Offsets)", WEAPONS_OFFSETS_PATH);
	}
}
