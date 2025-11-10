#pragma once

#include <optional>
#include <Version.h>

#include "resources.h"
#include "common/CommonUtils.h"
#include "common/ConfigBase.h"

namespace frik
{
    constexpr auto INI_SECTION_MAIN = "Fallout4VRBody";
    constexpr auto INI_SECTION_SMOOTH_MOVEMENT = "SmoothMovementVR";

    static const auto BASE_PATH = common::getRelativePathInDocuments(R"(\My Games\Fallout4VR\FRIK_Config)");
    static const auto FRIK_INI_PATH = BASE_PATH + R"(\FRIK.ini)";
    static const auto MESH_HIDE_FACE_INI_PATH = BASE_PATH + R"(\Mesh_Hide\face.ini)";
    static const auto MESH_HIDE_SKINS_INI_PATH = BASE_PATH + R"(\Mesh_Hide\skins.ini)";
    static const auto MESH_HIDE_SLOTS_INI_PATH = BASE_PATH + R"(\Mesh_Hide\slots.ini)";
    static const auto PIPBOY_HOLO_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\HoloPipboyPosition_v2.json)";
    static const auto PIPBOY_SCREEN_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\PipboyPosition_v2.json)";
    static const auto WEAPONS_OFFSETS_PATH = BASE_PATH + R"(\Weapons_Offsets)";

    constexpr float DEFAULT_CAMERA_HEIGHT = 120.4828f;

    /**
     * Type of weapon related position offsets.
     */
    enum class WeaponOffsetsMode : uint8_t
    {
        // The weapon offset in the primary hand.
        Weapon = 0,
        // The secondary hand gripping position offset.
        OffHand,
        // The secondary hand gripping position offset.
        Throwable,
        // Back of hand UI (HP,Ammo,etc.) offset on hand.
        BackOfHandUI,
    };

    enum class FlashlightLocation : uint8_t
    {
        Head = 0,
        LeftArm,
        RightArm
    };

    enum class DampenPipboyScreenMode : uint8_t
    {
        None = 0,
        Movement,
        HoldInPlace
    };

    /**
     * Holds all the configuration variables used in the mod.
     * Most of the configuration variables are loaded from the FRIK.ini file.
     * Some values can be changed in-game and persisted into the FRIK.ini file.
     */
    class Config : public common::ConfigBase
    {
    public:
        explicit Config() :
            ConfigBase(Version::PROJECT, FRIK_INI_PATH, IDR_FRIK_INI) {}

        virtual void load() override;
        void save() { saveIniConfig(); }

        void setFlashlightLocation(const FlashlightLocation location)
        {
            flashlightLocation = location;
            saveIniConfigValue(INI_SECTION_MAIN, "iFlashlightLocation", static_cast<int>(flashlightLocation));
        }

        void toggleIsHoloPipboy()
        {
            isHoloPipboy = !isHoloPipboy;
            saveIniConfigValue(INI_SECTION_MAIN, "HoloPipBoyEnabled", isHoloPipboy);
        }

        void toggleDampenPipboyScreen()
        {
            dampenPipboyScreenMode = static_cast<DampenPipboyScreenMode>((static_cast<uint8_t>(dampenPipboyScreenMode) + 1) % 3);
            saveIniConfigValue(INI_SECTION_MAIN, "iDampenPipboyScreenMode", static_cast<int>(dampenPipboyScreenMode));
        }

        void togglePipBoyOpenWhenLookAt()
        {
            pipboyOpenWhenLookAt = !pipboyOpenWhenLookAt;
            saveIniConfigValue(INI_SECTION_MAIN, "PipBoyOpenWhenLookAt", pipboyOpenWhenLookAt);
        }

        void savePipboyScale(const float scale)
        {
            pipBoyScale = scale;
            saveIniConfigValue(INI_SECTION_MAIN, "PipboyScale", pipBoyScale);
        }

        static void openInNotepad();
        RE::NiTransform getPipboyOffset();
        void savePipboyOffset(const RE::NiTransform& transform);
        std::optional<RE::NiTransform> getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, bool inPA) const;
        void saveWeaponOffsets(const std::string& name, const RE::NiTransform& transform, const WeaponOffsetsMode& mode, bool inPA);
        void removeWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, bool inPA, bool replaceWithEmbedded);

        // Config variables: See FRIK.ini for descriptions.

        // Player/Skeleton
        bool setScale = false;
        float fVrScale = 0;
        float playerHeight = 0;
        float armLength = 0;
        bool armsOnly = false;

        // Head Geometry Hide
        bool hideHead = false;
        bool hideEquipment = false;
        bool hideSkin = false;
        const std::vector<std::string>& faceGeometry() const { return _faceGeometry; }
        const std::vector<std::string>& skinGeometry() const { return _skinGeometry; }
        const std::vector<int>& hideEquipSlotIndexes() const { return _hideEquipSlotIndexes; }

        // Camera and Body offsets
        float rootOffset = 0;
        float PARootOffset = 0;
        float cameraHeight = 0;
        float PACameraHeight = 0;
        float playerOffset_forward = 0;
        float playerOffset_up = 0;
        float powerArmor_forward = 0;
        float powerArmor_up = 0;

        // Pipboy
        float pipBoyScale = 0;
        bool hidePipboy = false;
        bool isHoloPipboy = false;
        bool leftHandedPipBoy = false;
        bool enablePrimaryControllerPipboyUse = false;
        bool pipboyOpenWhenLookAt = false;
        bool pipboyCloseWhenLookAway = false;
        bool pipboyCloseWhenMovingWhileLookingAway = false;
        bool pipboyHolsterWeaponForOperation = false;
        float pipboyLookAtThreshold = 0;
        float pipboyLookAwayThreshold = 0;
        float pipboyOperationFingerDetectionRange = 0;
        int pipBoyOnDelay = 0;
        int pipBoyOffDelay = 0;
        int pipBoyButtonArm = 0;
        int pipBoyButtonID = 0;
        int pipBoyButtonOffArm = 0;
        int pipBoyButtonOffID = 0;

        // Pipboy Torch/Flashlight
        bool removeFlashlight = false;
        FlashlightLocation flashlightLocation = FlashlightLocation::Head;
        int switchTorchButton = 2;

        // Weapon offhand grip
        bool enableOffHandGripping = false;
        bool enableGripButtonToGrap = false;
        bool enableGripButtonToLetGo = false;
        bool onePressGripButton = false;
        float gripLetGoThreshold = 0;
        int gripButtonID = 0;

        // Dampen hands
        bool dampenHands = false;
        bool dampenHandsInVanillaScope = false;
        float dampenHandsRotation = 0;
        float dampenHandsTranslation = 0;
        float dampenHandsRotationInVanillaScope = 0;
        float dampenHandsTranslationInVanillaScope = 0;

        // Dampen Pipboy screen
        DampenPipboyScreenMode dampenPipboyScreenMode = DampenPipboyScreenMode::None;
        float dampenPipboyThreshold = 0;
        float dampenPipboyMultiplier = 0;

        // Misc
        bool showPAHUD = false;
        float selfieOutFrontDistance = 0;
        bool selfieIgnoreHideFlags = false;
        float scopeAdjustDistance = 0;

        // Smooth Movement
        bool disableSmoothMovement = false;
        float smoothingAmount = 0;
        float smoothingAmountHorizontal = 0;
        float dampingMultiplier = 0;
        float dampingMultiplierHorizontal = 0;
        float stoppingMultiplier = 0;
        float stoppingMultiplierHorizontal = 0;
        int disableInteriorSmoothing = 0;
        int disableInteriorSmoothingHorizontal = 0;

    protected:
        virtual void loadIniConfigInternal(const CSimpleIniA& ini) override;
        virtual void saveIniConfigInternal(CSimpleIniA& ini) override;
        virtual void updateIniConfigToLatestVersionCustom(int currentVersion, int latestVersion, const CSimpleIniA& oldIni, CSimpleIniA& newIni) const override;

    private:
        void loadHideMeshes();
        void loadHideEquipmentSlots();
        void loadPipboyOffsets();
        void loadWeaponsOffsets();
        static void setupFolders();
        static void migrateConfigFilesIfNeeded();
        static std::string getWeaponNameWithMode(const std::string& name, const WeaponOffsetsMode& mode, bool inPA, bool leftHanded);

        // hide meshes
        std::vector<std::string> _faceGeometry;
        std::vector<std::string> _skinGeometry;
        std::vector<int> _hideEquipSlotIndexes;

        // offsets
        std::unordered_map<std::string, RE::NiTransform> _pipboyOffsets;
        std::unordered_map<std::string, RE::NiTransform> _weaponsOffsets;
        std::unordered_map<std::string, RE::NiTransform> _weaponsEmbeddedOffsets;
    };

    // Global singleton for easy access
    inline Config g_config;
}
