// ReSharper disable StringLiteralTypo

#include "Config.h"

#include <filesystem>

#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/VRControllersManager.h"

using namespace common;

namespace frik
{
    /**
     * Open the FRIK.ini file in Notepad for editing.
     */
    void Config::openInNotepad()
    {
        ShellExecuteA(nullptr, "open", "notepad.exe", FRIK_INI_PATH.c_str(), nullptr, SW_SHOWNORMAL);
    }

    /**
     * Load the FRIK.ini config, hide meshes, and weapon offsets.
     * Handle creating the FRIK.ini file if it doesn't exist.
     * Handle updating the FRIK.ini file if the version is old.
     */
    void Config::load()
    {
        setupFolders();
        migrateConfigFilesIfNeeded();

        logger::info("Load ini config...");
        loadIniConfig();

        logger::info("Load hide meshes...");
        loadHideMeshes();

        logger::info("Load pipboy offsets...");
        loadPipboyOffsets();

        logger::info("Load weapon offsets...");
        loadWeaponsOffsets();
    }

    void Config::loadIniConfigInternal(const CSimpleIniA& ini)
    {
        // Player/Skeleton
        setScale = ini.GetBoolValue(INI_SECTION_MAIN, "setScale", false);
        fVrScale = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "fVrScale", 70.0));
        playerHeight = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "PlayerHeight", 120.4828f));
        armLength = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "armLength", 36.74f));
        armsOnly = ini.GetBoolValue(INI_SECTION_MAIN, "EnableArmsOnlyMode");

        // Head Geometry Hide
        hideHead = ini.GetBoolValue(INI_SECTION_MAIN, "HideHead");
        hideEquipment = ini.GetBoolValue(INI_SECTION_MAIN, "HideEquipment");
        hideSkin = ini.GetBoolValue(INI_SECTION_MAIN, "HideSkin");

        // Camera and Body offsets
        rootOffset = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "RootOffset", 0.0));
        PARootOffset = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_RootOffset", 0.0));
        cameraHeight = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "cameraHeightOffset", 0.0));
        PACameraHeight = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_cameraHeightOffset", 0.0));
        playerOffset_forward = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "playerOffset_forward", -4.0));
        playerOffset_up = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "playerOffset_up", -2.0));
        powerArmor_forward = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_forward", 0.0));
        powerArmor_up = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "powerArmor_up", 0.0));

        // Pipboy
        pipBoyScale = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "PipboyScale", 1.0));
        hidePipboy = ini.GetBoolValue(INI_SECTION_MAIN, "hidePipboy");
        isHoloPipboy = ini.GetBoolValue(INI_SECTION_MAIN, "HoloPipBoyEnabled", true);
        leftHandedPipBoy = ini.GetBoolValue(INI_SECTION_MAIN, "PipboyRightArmLeftHandedMode");
        enablePrimaryControllerPipboyUse = ini.GetBoolValue(INI_SECTION_MAIN, "PipboyUIPrimaryController", true);
        pipboyOpenWhenLookAt = ini.GetBoolValue(INI_SECTION_MAIN, "PipBoyOpenWhenLookAt", false);
        pipboyCloseWhenLookAway = ini.GetBoolValue(INI_SECTION_MAIN, "PipBoyCloseWhenLookAway", false);
        pipboyCloseWhenMovingWhileLookingAway = ini.GetBoolValue(INI_SECTION_MAIN, "AllowMovementWhenNotLookingAtPipboy", true);
        pipboyHolsterWeaponForOperation = ini.GetBoolValue(INI_SECTION_MAIN, "bHolsterWeaponForOperation", false);
        pipboyLookAtThreshold = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "PipBoyLookAtThreshold", 0.75f));
        pipboyLookAwayThreshold = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "PipBoyLookAwayThreshold", 0.3f));
        pipboyOperationFingerDetectionRange = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "fPipboyOperationFingerDetectionRange", 12.0));
        pipBoyOnDelay = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOnDelay", 400));
        pipBoyOffDelay = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "PipBoyOffDelay", 1000));
        pipBoyButtonArm = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonArm", 0));
        pipBoyButtonID = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonID", f4vr::VRButtonId::k_EButton_SteamVR_Trigger));
        pipBoyButtonOffArm = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonOffArm", 0));
        pipBoyButtonOffID = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "OperatePipboyWithButtonOffID", f4vr::VRButtonId::k_EButton_Grip));

        // Torch/Flashlight
        removeFlashlight = ini.GetBoolValue(INI_SECTION_MAIN, "bRemoveFlashlight", false);
        flashlightLocation = static_cast<FlashlightLocation>(ini.GetLongValue(INI_SECTION_MAIN, "iFlashlightLocation", 0));
        switchTorchButton = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "SwitchTorchButton", 2));

        // Fallout London VR support
        attaboyGrabButtonId = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "iAttaboyGrabButtonId", f4vr::VRButtonId::k_EButton_Grip));
        attaboyGrabActivationDistance = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "fAttaboyGrabActivationDistance", 15.0));

        // Two-handed gripping
        enableOffHandGripping = ini.GetBoolValue(INI_SECTION_MAIN, "EnableOffHandGripping", true);
        enableGripButtonToGrap = ini.GetBoolValue(INI_SECTION_MAIN, "EnableGripButton", true);
        enableGripButtonToLetGo = ini.GetBoolValue(INI_SECTION_MAIN, "EnableGripButtonToLetGo", true);
        onePressGripButton = ini.GetBoolValue(INI_SECTION_MAIN, "EnableGripButtonOnePress", true);
        gripLetGoThreshold = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "GripLetGoThreshold", 15.0f));
        gripButtonID = static_cast<int>(ini.GetLongValue(INI_SECTION_MAIN, "GripButtonID", vr::EVRButtonId::k_EButton_Grip)); // 2

        // Dampen hands
        dampenHands = ini.GetBoolValue(INI_SECTION_MAIN, "DampenHands", true);
        dampenHandsInVanillaScope = ini.GetBoolValue(INI_SECTION_MAIN, "DampenHandsInVanillaScope", true);
        dampenHandsRotation = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "DampenHandsRotation", 0.7f));
        dampenHandsTranslation = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "DampenHandsTranslation", 0.7f));
        dampenHandsRotationInVanillaScope = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "DampenHandsRotationInVanillaScope", 0.2f));
        dampenHandsTranslationInVanillaScope = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "DampenHandsTranslationInVanillaScope", 0.2f));

        // Dampen Pipboy
        dampenPipboyScreenMode = static_cast<DampenPipboyScreenMode>(ini.GetLongValue(INI_SECTION_MAIN, "iDampenPipboyScreenMode", 1));
        dampenPipboyThreshold = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "fDampenPipboyThreshold", 1.1f));
        dampenPipboyMultiplier = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "fDampenPipboyMultiplier", 0.7f));

        // Misc
        showPAHUD = ini.GetBoolValue(INI_SECTION_MAIN, "showPAHUD");
        selfieOutFrontDistance = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "selfieOutFrontDistance", 120.0));
        selfieIgnoreHideFlags = ini.GetBoolValue(INI_SECTION_MAIN, "selfieIgnoreHideFlags", false);
        scopeAdjustDistance = static_cast<float>(ini.GetDoubleValue(INI_SECTION_MAIN, "ScopeAdjustDistance", 15.f));

        //Smooth Movement
        disableSmoothMovement = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableSmoothMovement");
        smoothingAmount = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "SmoothAmount", 15.0));
        smoothingAmountHorizontal = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "SmoothAmountHorizontal", 5.0));
        dampingMultiplier = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "Damping", 1.0));
        dampingMultiplierHorizontal = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "DampingHorizontal", 1.0));
        stoppingMultiplier = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "StoppingMultiplier", 0.6f));
        stoppingMultiplierHorizontal = static_cast<float>(ini.GetDoubleValue(INI_SECTION_SMOOTH_MOVEMENT, "StoppingMultiplierHorizontal", 0.6f));
        disableInteriorSmoothing = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableInteriorSmoothing", true);
        disableInteriorSmoothingHorizontal = ini.GetBoolValue(INI_SECTION_SMOOTH_MOVEMENT, "DisableInteriorSmoothingHorizontal", true);
    }

    /**
     * Load hide meshes from config ini files. Creating if it doesn't exist on the disk.
     */
    void Config::loadHideMeshes()
    {
        createFileFromResourceIfNotExists(MESH_HIDE_FACE_INI_PATH, _module, IDR_MESH_HIDE_FACE, true);
        _faceGeometry = loadListFromFile(MESH_HIDE_FACE_INI_PATH);

        createFileFromResourceIfNotExists(MESH_HIDE_SKINS_INI_PATH, _module, IDR_MESH_HIDE_SKINS, true);
        _skinGeometry = loadListFromFile(MESH_HIDE_SKINS_INI_PATH);

        createFileFromResourceIfNotExists(MESH_HIDE_SLOTS_INI_PATH, _module, IDR_MESH_HIDE_SLOTS, true);
        loadHideEquipmentSlots();
    }

    /**
     * Slot Ids base on this link: https://falloutck.uesp.net/wiki/Biped_Slots
     */
    void Config::loadHideEquipmentSlots()
    {
        const auto slotsGeometry = loadListFromFile(MESH_HIDE_SLOTS_INI_PATH);

        std::unordered_map<std::string, int> slotToIndexMap = {
            { "hairtop", 0 }, // i.e. helmet
            { "hairlong", 1 },
            { "head", 2 },
            { "headband", 16 },
            { "eyes", 17 }, // i.e. glasses
            { "beard", 18 },
            { "mouth", 19 },
            { "neck", 20 },
            { "scalp", 22 }
        };

        _hideEquipSlotIndexes.clear();
        for (auto& geometry : slotsGeometry) {
            if (slotToIndexMap.contains(geometry)) {
                _hideEquipSlotIndexes.push_back(slotToIndexMap[geometry]);
            }
        }
    }

    void Config::saveIniConfigInternal(CSimpleIniA& ini)
    {
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
        ini.SetLongValue(INI_SECTION_MAIN, "iFlashlightLocation", static_cast<int>(flashlightLocation));
        ini.SetLongValue(INI_SECTION_MAIN, "iDampenPipboyScreenMode", static_cast<int>(dampenPipboyScreenMode));
        ini.SetBoolValue(INI_SECTION_MAIN, "PipBoyOpenWhenLookAt", pipboyOpenWhenLookAt);
        ini.SetBoolValue(INI_SECTION_MAIN, "DampenHands", dampenHands);
        ini.SetDoubleValue(INI_SECTION_MAIN, "DampenHandsRotation", dampenHandsRotation);
        ini.SetDoubleValue(INI_SECTION_MAIN, "DampenHandsTranslation", dampenHandsTranslation);
        ini.SetDoubleValue(INI_SECTION_MAIN, "powerArmor_RootOffset", PARootOffset);
        ini.SetDoubleValue(INI_SECTION_MAIN, "RootOffset", rootOffset);
        ini.SetBoolValue(INI_SECTION_MAIN, "EnableGripButton", enableGripButtonToGrap);
        ini.SetBoolValue(INI_SECTION_MAIN, "EnableGripButtonToLetGo", enableGripButtonToLetGo);
        ini.SetBoolValue(INI_SECTION_MAIN, "EnableGripButtonOnePress", onePressGripButton);
    }

    void Config::updateIniConfigToLatestVersionCustom(const int currentVersion, const int, const CSimpleIniA&, CSimpleIniA& newIni) const
    {
        // fix bad log pattern in old ini version
        if (currentVersion == 8) {
            newIni.SetValue(INI_SECTION_DEBUG, "sLogPattern", "%H:%M:%S.%e %L: %v");
        }
    }

    std::string Config::getPipboyOffsetKey() const
    {
        if (isFalloutLondonVR) {
            return "AttaboyPosition";
        }
        return isHoloPipboy ? "HoloPipboyPosition" : "PipboyPosition";
    }

    std::string Config::getPipboyOffsetPath() const
    {
        if (isFalloutLondonVR) {
            return PIPBOY_ATTABOY_OFFSETS_PATH;
        }
        return isHoloPipboy ? PIPBOY_HOLO_OFFSETS_PATH : PIPBOY_SCREEN_OFFSETS_PATH;
    }

    /**
     * Get the Pipboy offset of the currently used Pipboy type.
     */
    RE::NiTransform Config::getPipboyOffset()
    {
        return _pipboyOffsets[getPipboyOffsetKey()];
    }

    /**
     * Save the Pipboy offset to the offsets map.
     */
    void Config::savePipboyOffset(const RE::NiTransform& transform)
    {
        const auto key = getPipboyOffsetKey();
        _pipboyOffsets[key] = transform;
        saveOffsetsToJsonFile(key, transform, getPipboyOffsetPath());
    }

    /**
     * Get weapon offsets for given weapon name and mode.
     * Use non-PA mode if PA mode offsets not found.
     */
    std::optional<RE::NiTransform> Config::getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA) const
    {
        const auto it = _weaponsOffsets.find(getWeaponNameWithMode(name, mode, inPA, f4vr::isLeftHandedMode()));
        if (it != _weaponsOffsets.end()) {
            return it->second;
        }
        // Check without PA (historic)
        return inPA ? getWeaponOffsets(name, mode, false) : std::nullopt;
    }

    /**
     * Save the weapon offset to config and filesystem.
     */
    void Config::saveWeaponOffsets(const std::string& name, const RE::NiTransform& transform, const WeaponOffsetsMode& mode, const bool inPA)
    {
        const auto fullName = getWeaponNameWithMode(name, mode, inPA, f4vr::isLeftHandedMode());
        _weaponsOffsets[fullName] = transform;
        saveOffsetsToJsonFile(fullName, transform, WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json");
    }

    /**
     * Remove the weapon offset from the config and filesystem.
     */
    void Config::removeWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA, const bool replaceWithEmbedded)
    {
        const auto fullName = getWeaponNameWithMode(name, mode, inPA, f4vr::isLeftHandedMode());
        _weaponsOffsets.erase(fullName);
        if (replaceWithEmbedded && _weaponsEmbeddedOffsets.contains(fullName)) {
            _weaponsOffsets[fullName] = _weaponsEmbeddedOffsets[fullName];
        }

        const auto path = WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json";
        logger::info("Removing weapon offsets '{}', file: '{}'", fullName.c_str(), path.c_str());
        if (!std::filesystem::remove(WEAPONS_OFFSETS_PATH + "\\" + fullName + ".json")) {
            logger::warn("Failed to remove weapon offset file: {}", fullName.c_str());
        }
    }

    /**
     * Load the pipboy screen and holo screen offsets from json files.
     */
    void Config::loadPipboyOffsets()
    {
        createFileFromResourceIfNotExists(PIPBOY_HOLO_OFFSETS_PATH, _module, IDR_PIPBOY_HOLO_OFFSETS, false);
        createFileFromResourceIfNotExists(PIPBOY_SCREEN_OFFSETS_PATH, _module, IDR_PIPBOY_SCREEN_OFFSETS, false);
        createFileFromResourceIfNotExists(PIPBOY_ATTABOY_OFFSETS_PATH, _module, IDR_PIPBOY_ATTABOY_OFFSETS, false);
        _pipboyOffsets.clear();
        loadOffsetJsonFile(PIPBOY_HOLO_OFFSETS_PATH, _pipboyOffsets);
        loadOffsetJsonFile(PIPBOY_SCREEN_OFFSETS_PATH, _pipboyOffsets);
        loadOffsetJsonFile(PIPBOY_ATTABOY_OFFSETS_PATH, _pipboyOffsets);
    }

    /**
     * Load all the weapons offsets embedded in resource and override custom from filesystem.
     */
    void Config::loadWeaponsOffsets()
    {
        _weaponsOffsets.clear();
        _weaponsEmbeddedOffsets = loadEmbeddedOffsets(200, 600);
        _weaponsOffsets.insert(_weaponsEmbeddedOffsets.begin(), _weaponsEmbeddedOffsets.end());

        const auto weaponCustomOffsets = loadOffsetsFromFilesystem(WEAPONS_OFFSETS_PATH);
        for (auto& [key, value] : weaponCustomOffsets) {
            _weaponsOffsets.insert_or_assign(key, value);
        }
        logger::info("Loaded weapon offsets ; Total:{}, Embedded:{}, Custom:{}", _weaponsOffsets.size(), _weaponsEmbeddedOffsets.size(), weaponCustomOffsets.size());
    }

    /**
     * Create all the folders needed for config to not handle later creating folders that don't exist.
     */
    void Config::setupFolders()
    {
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
    void Config::migrateConfigFilesIfNeeded()
    {
        // migrate pre v72 and v72 config files to v73 location
        logger::info("Migrate configs if exists in old locations...");
        moveFileSafe(R"(.\Data\F4SE\plugins\FRIK.ini)", FRIK_INI_PATH);
        moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_FOLVR.ini)", FRIK_FOLVR_INI_PATH);
        moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_Mesh_Hide\face.ini)", MESH_HIDE_FACE_INI_PATH);
        moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_Mesh_Hide\skins.ini)", MESH_HIDE_SKINS_INI_PATH);
        moveFileSafe(R"(.\Data\F4SE\plugins\FRIK_Mesh_Hide\slots.ini)", MESH_HIDE_SLOTS_INI_PATH);
        moveAllFilesInFolderSafe(R"(.\Data\F4SE\plugins\FRIK_weapon_offsets)", WEAPONS_OFFSETS_PATH);
    }

    /**
     * Get the name for the weapon offset to use depending on the mode.
     * Basically a hack to store multiple modes of the same weapon by adding suffix to the name.
     */
    std::string Config::getWeaponNameWithMode(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA, const bool leftHanded)
    {
        static const std::string POWER_ARMOR_SUFFIX{ "-PowerArmor" };
        static const std::string OFF_HAND_SUFFIX{ "-offHand" };
        static const std::string THROWABLE_SUFFIX{ "-throwable" };
        static const std::string BACK_OF_HAND_SUFFIX{ "-backOfHand" };
        static const std::string LEFT_HANDED_SUFFIX{ "-leftHanded" };
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
