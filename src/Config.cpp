// ReSharper disable StringLiteralTypo

#include <fstream>
#include "include/json.hpp"
#include "include/SimpleIni.h"

#include "Config.h"

#include <filesystem>
#include <shlobj_core.h>

#include "utils.h"
#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "res/resource.h"

using namespace common;

namespace frik {
	/**
	 * Open the FRIK.ini file in Notepad for editing.
	 */
	void Config::openInNotepad() {
		ShellExecute(nullptr, "open", "notepad.exe", FRIK_INI_PATH.c_str(), nullptr, SW_SHOWNORMAL);
	}

	/**
	 * Load the FRIK.ini config, hide meshes, and weapon offsets.
	 * Handle creating the FRIK.ini file if it doesn't exist.
	 * Handle updating the FRIK.ini file if the version is old.
	 */
	void Config::loadAllConfig() {
		setupFolders();
		migrateConfigFilesIfNeeded();

		Log::info("Load ini config...");
		loadIniConfig();

		Log::info("Load hide meshes...");
		loadHideMeshes();

		Log::info("Load pipboy offsets...");
		loadPipboyOffsets();

		Log::info("Load weapon offsets...");
		loadWeaponsOffsets();
	}

	void Config::loadIniConfigInternal(const CSimpleIniA& ini) {
		playerHeight = ini.GetFloatValue(INI_SECTION_MAIN, "PlayerHeight", 120.4828f);
		setScale = ini.GetBoolValue(INI_SECTION_MAIN, "setScale", false);
		fVrScale = ini.GetFloatValue(INI_SECTION_MAIN, "fVrScale", 70.0);
		playerOffset_forward = ini.GetFloatValue(INI_SECTION_MAIN, "playerOffset_forward", -4.0);
		playerOffset_up = ini.GetFloatValue(INI_SECTION_MAIN, "playerOffset_up", -2.0);
		powerArmor_forward = ini.GetFloatValue(INI_SECTION_MAIN, "powerArmor_forward", 0.0);
		powerArmor_up = ini.GetFloatValue(INI_SECTION_MAIN, "powerArmor_up", 0.0);
		pipboyDetectionRange = ini.GetFloatValue(INI_SECTION_MAIN, "pipboyDetectionRange", 15.0);
		armLength = ini.GetFloatValue(INI_SECTION_MAIN, "armLength", 36.74f);
		cameraHeight = ini.GetFloatValue(INI_SECTION_MAIN, "cameraHeightOffset", 0.0);
		PACameraHeight = ini.GetFloatValue(INI_SECTION_MAIN, "powerArmor_cameraHeightOffset", 0.0);
		rootOffset = ini.GetFloatValue(INI_SECTION_MAIN, "RootOffset", 0.0);
		PARootOffset = ini.GetFloatValue(INI_SECTION_MAIN, "powerArmor_RootOffset", 0.0);
		showPAHUD = ini.GetBoolValue(INI_SECTION_MAIN, "showPAHUD");
		hidePipboy = ini.GetBoolValue(INI_SECTION_MAIN, "hidePipboy");
		leftHandedPipBoy = ini.GetBoolValue(INI_SECTION_MAIN, "PipboyRightArmLeftHandedMode");
		armsOnly = ini.GetBoolValue(INI_SECTION_MAIN, "EnableArmsOnlyMode");
		hideHead = ini.GetBoolValue(INI_SECTION_MAIN, "HideHead");
		hideEquipment = ini.GetBoolValue(INI_SECTION_MAIN, "HideEquipment");
		hideSkin = ini.GetBoolValue(INI_SECTION_MAIN, "HideSkin");
		pipBoyLookAtGate = ini.GetFloatValue(INI_SECTION_MAIN, "PipBoyLookAtThreshold", 0.7f);
		pipBoyOffDelay = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOffDelay", 5000));
		pipBoyOnDelay = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOnDelay", 5000));
		gripLetGoThreshold = ini.GetFloatValue(INI_SECTION_MAIN, "GripLetGoThreshold", 15.0f);
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
		dampenHandsRotation = ini.GetFloatValue(INI_SECTION_MAIN, "DampenHandsRotation", 0.7f);
		dampenHandsTranslation = ini.GetFloatValue(INI_SECTION_MAIN, "DampenHandsTranslation", 0.7f);
		dampenHandsRotationInVanillaScope = ini.GetFloatValue(INI_SECTION_MAIN, "DampenHandsRotationInVanillaScope", 0.2f);
		dampenHandsTranslationInVanillaScope = ini.GetFloatValue(INI_SECTION_MAIN, "DampenHandsTranslationInVanillaScope", 0.2f);
		dampenPipboyRotation = ini.GetFloatValue(INI_SECTION_MAIN, "DampenPipboyRotation", 0.7f);
		dampenPipboyTranslation = ini.GetFloatValue(INI_SECTION_MAIN, "DampenPipboyTranslation", 0.7f);
		selfieOutFrontDistance = ini.GetFloatValue(INI_SECTION_MAIN, "selfieOutFrontDistance", 120.0);
		selfieIgnoreHideFlags = ini.GetBoolValue(INI_SECTION_MAIN, "selfieIgnoreHideFlags", false);

		//Pipboy & Main Config Mode Buttons
		pipBoyScale = ini.GetFloatValue(INI_SECTION_MAIN, "PipboyScale", 1.0);
		switchUIControltoPrimary = ini.GetBoolValue(INI_SECTION_MAIN, "PipboyUIPrimaryController", true);
		autoFocusWindow = ini.GetBoolValue(INI_SECTION_MAIN, "AutoFocusWindow", false);

		// weaponPositioning
		scopeAdjustDistance = ini.GetFloatValue(INI_SECTION_MAIN, "ScopeAdjustDistance", 15.f);

		//Smooth Movement
		disableSmoothMovement = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableSmoothMovement");
		smoothingAmount = ini.GetFloatValue(INI_SECTION_SMOOTH_MOVEMENT, "SmoothAmount", 15.0);
		smoothingAmountHorizontal = ini.GetFloatValue(INI_SECTION_SMOOTH_MOVEMENT, "SmoothAmountHorizontal", 5.0);
		dampingMultiplier = ini.GetFloatValue(INI_SECTION_SMOOTH_MOVEMENT, "Damping", 1.0);
		dampingMultiplierHorizontal = ini.GetFloatValue(INI_SECTION_SMOOTH_MOVEMENT, "DampingHorizontal", 1.0);
		stoppingMultiplier = ini.GetFloatValue(INI_SECTION_SMOOTH_MOVEMENT, "StoppingMultiplier", 0.6f);
		stoppingMultiplierHorizontal = ini.GetFloatValue(INI_SECTION_SMOOTH_MOVEMENT, "StoppingMultiplierHorizontal", 0.6f);
		disableInteriorSmoothing = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableInteriorSmoothing", true);
		disableInteriorSmoothingHorizontal = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableInteriorSmoothingHorizontal", true);
	}

	/**
	 * Load hide meshes from config ini files. Creating if it doesn't exist on the disk.
	 */
	void Config::loadHideMeshes() {
		createFileFromResourceIfNotExists(MESH_HIDE_FACE_INI_PATH, IDR_MESH_HIDE_FACE, true);
		_faceGeometry = loadListFromFile(MESH_HIDE_FACE_INI_PATH);

		createFileFromResourceIfNotExists(MESH_HIDE_SKINS_INI_PATH, IDR_MESH_HIDE_SKINS, true);
		_skinGeometry = loadListFromFile(MESH_HIDE_SKINS_INI_PATH);

		createFileFromResourceIfNotExists(MESH_HIDE_SLOTS_INI_PATH, IDR_MESH_HIDE_SLOTS, true);
		loadHideEquipmentSlots();
	}

	/**
	 * Slot Ids base on this link: https://falloutck.uesp.net/wiki/Biped_Slots
	 */
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

		_hideEquipSlotIndexes.clear();
		for (auto& geometry : slotsGeometry) {
			if (slotToIndexMap.contains(geometry)) {
				_hideEquipSlotIndexes.push_back(slotToIndexMap[geometry]);
			}
		}
	}

	void Config::saveIniConfigInternal(CSimpleIniA& ini) const {
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
	}

	/**
	 * Get the Pipboy offset of the currently used Pipboy type.
	 */
	NiTransform Config::getPipboyOffset() {
		return _pipboyOffsets[isHoloPipboy ? "HoloPipboyPosition" : "PipboyPosition"];
	}

	/**
	 * Save the Pipboy offset to the offsets map.
	 */
	void Config::savePipboyOffset(const NiTransform& transform) {
		const auto type = isHoloPipboy ? "HoloPipboyPosition" : "PipboyPosition";
		_pipboyOffsets[type] = transform;
		saveOffsetsToJsonFile(type, transform, isHoloPipboy ? PIPBOY_HOLO_OFFSETS_PATH : PIPBOY_SCREEN_OFFSETS_PATH);
	}

	/**
	 * Get weapon offsets for given weapon name and mode.
	 * Use non-PA mode if PA mode offsets not found.
	 */
	/// <returns></returns>
	std::optional<NiTransform> Config::getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA) const {
		const auto it = _weaponsOffsets.find(getWeaponNameWithMode(name, mode, inPA, leftHandedMode));
		if (it != _weaponsOffsets.end()) {
			return it->second;
		}
		// Check without PA (historic)
		return inPA ? getWeaponOffsets(name, mode, false) : std::nullopt;
	}

	/**
	 * Save the weapon offset to config and filesystem.
	 */
	void Config::saveWeaponOffsets(const std::string& name, const NiTransform& transform, const WeaponOffsetsMode& mode, const bool inPA) {
		const auto fullName = getWeaponNameWithMode(name, mode, inPA, leftHandedMode);
		_weaponsOffsets[fullName] = transform;
		saveOffsetsToJsonFile(fullName, transform, WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json");
	}

	/**
	 * Remove the weapon offset from the config and filesystem.
	 */
	void Config::removeWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA, const bool replaceWithEmbedded) {
		const auto fullName = getWeaponNameWithMode(name, mode, inPA, leftHandedMode);
		_weaponsOffsets.erase(fullName);
		if (replaceWithEmbedded && _weaponsEmbeddedOffsets.contains(fullName)) {
			_weaponsOffsets[fullName] = _weaponsEmbeddedOffsets[fullName];
		}

		const auto path = WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json";
		Log::info("Removing weapon offsets '%s', file: '%s'", fullName.c_str(), path.c_str());
		if (!std::filesystem::remove(WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json")) {
			Log::warn("Failed to remove weapon offset file: %s", fullName.c_str());
		}
	}

	/**
	 * Load the pipboy screen and holo screen offsets from json files.
	 */
	void Config::loadPipboyOffsets() {
		createFileFromResourceIfNotExists(PIPBOY_HOLO_OFFSETS_PATH, IDR_PIPBOY_HOLO_OFFSETS, false);
		createFileFromResourceIfNotExists(PIPBOY_SCREEN_OFFSETS_PATH, IDR_PIPBOY_SCREEN_OFFSETS, false);
		_pipboyOffsets.clear();
		loadOffsetJsonFile(PIPBOY_HOLO_OFFSETS_PATH, _pipboyOffsets);
		loadOffsetJsonFile(PIPBOY_SCREEN_OFFSETS_PATH, _pipboyOffsets);
	}

	/**
	 * Load all the weapons offsets embedded in resource and override custom from filesystem.
	 */
	void Config::loadWeaponsOffsets() {
		_weaponsOffsets.clear();
		_weaponsEmbeddedOffsets = loadEmbeddedOffsets(200, 400);
		_weaponsOffsets.insert(_weaponsEmbeddedOffsets.begin(), _weaponsEmbeddedOffsets.end());

		const auto weaponCustomOffsets = loadOffsetsFromFilesystem(WEAPONS_OFFSETS_PATH);
		for (auto& [key, value] : weaponCustomOffsets) {
			_weaponsOffsets.insert_or_assign(key, value);
		}
		Log::info("Loaded weapon offsets	; Total:%d, Embedded:%d, Custom:%d", _weaponsOffsets.size(), _weaponsEmbeddedOffsets.size(), weaponCustomOffsets.size());
	}

	/**
	 * Create all the folders needed for config to not handle later creating folders that don't exist.
	 */
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
		Log::info("Migrate configs if exists in old locations...");
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

	/**
	 * Get the name for the weapon offset to use depending on the mode.
	 * Basically a hack to store multiple modes of the same weapon by adding suffix to the name.
	 */
	std::string Config::getWeaponNameWithMode(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA, const bool leftHanded) {
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
		default:
			throw std::invalid_argument("Invalid weapon offset mode");
		}
	}
}
