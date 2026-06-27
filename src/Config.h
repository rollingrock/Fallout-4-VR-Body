#pragma once

#include <Version.h>
#include <optional>

#include "ConfigBase.h"
#include "common/CommonUtils.h"
#include "resources.h"
#include "vrcf/VRControllersManager.h"

namespace frik
{
    constexpr auto INI_SECTION_MAIN = "Fallout4VRBody";
    constexpr auto INI_SECTION_SMOOTH_MOVEMENT = "SmoothMovementVR";

    static const auto BASE_PATH = common::getRelativePathInDocuments(R"(\My Games\Fallout4VR\FRIK_Config)");
    static const auto FRIK_INI_PATH = BASE_PATH + R"(\FRIK.ini)";
    static const auto FRIK_FOLVR_INI_PATH = BASE_PATH + R"(\FRIK_FOLVR.ini)";
    static const auto MESH_HIDE_FACE_INI_PATH = BASE_PATH + R"(\Mesh_Hide\face.ini)";
    static const auto MESH_HIDE_SKINS_INI_PATH = BASE_PATH + R"(\Mesh_Hide\skins.ini)";
    static const auto MESH_HIDE_SLOTS_INI_PATH = BASE_PATH + R"(\Mesh_Hide\slots.ini)";
    static const auto PIPBOY_HOLO_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\HoloPipboyPosition_v2.json)";
    static const auto PIPBOY_SCREEN_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\PipboyPosition_v2.json)";
    static const auto PIPBOY_ATTABOY_OFFSETS_PATH = BASE_PATH + R"(\Pipboy_Offsets\AttaboyPosition_v2.json)";
    static const auto WEAPONS_OFFSETS_PATH = BASE_PATH + R"(\Weapons_Offsets)";

    constexpr float DEFAULT_CAMERA_HEIGHT = 120.4828f;

    /**
     * Type of weapon related position offsets.
     */
    enum class WeaponOffsetsMode : uint8_t
    {
        // The weapon offset in the primary hand.
        Weapon = 0,
        // The primary hand holding the stock.
        PrimaryHand,
        // The secondary hand gripping on the weapon.
        OffHand,
        // The throwable weapon offset in the primary hand.
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
    class Config : public ConfigBase
    {
    public:
        explicit Config()
            : ConfigBase(Version::PROJECT, FRIK_INI_PATH, IDR_FRIK_INI)
        {}

        virtual void load() override;
        void loadIniOnly();

        void reloadForFalloutLondonVR();
        void setFlashlightLocation(FlashlightLocation location);
        void toggleIsHoloPipboy();
        void toggleDampenPipboyScreen();
        void togglePipBoyOpenWhenLookAt();
        void togglePipBoyCloseWhenLookAway();
        void savePipboyScale(float scale);
        void saveIsPlayingSeated(bool iIsPlayingSeated);
        void saveHideHeadEquipment(bool hide);
        void saveDampenHands(bool iDampenHands);

        float getPlayerBodyOffsetUp() const;
        void setPlayerBodyOffsetUp(float value);
        float getPlayerBodyOffsetForward() const;
        void setPlayerBodyOffsetForward(float value);
        float getPlayerHMDOffsetUp() const;
        void setPlayerHMDOffsetUp(float value);
        float getPlayerLegSlackAdjustOffset() const;
        void setPlayerLegSlackAdjustOffset(float value);

        static void openInNotepad();
        RE::NiTransform getPipboyOffset();
        RE::NiTransform getDefaultPipboyOffset();
        bool savePipboyOffset(const RE::NiTransform& transform);
        std::optional<RE::NiTransform> getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, bool inPA) const;
        bool saveWeaponOffsets(const std::string& name, const RE::NiTransform& transform, const WeaponOffsetsMode& mode, bool inPA);
        void removeWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, bool inPA, bool replaceWithEmbedded);

        // Config variables: See FRIK.ini for descriptions.

        // Player/Skeleton
        bool setScale = false;
        float fVrScale = 0;
        float playerHeight = 0;
        float armLength = 0;

        // Head Geometry Hide
        bool hideHead = false;
        bool hideHeadEquipment = false;
        bool hideSkin = false;

        const std::vector<std::string>& faceGeometry() const
        {
            return _faceGeometry;
        }

        const std::vector<std::string>& skinGeometry() const
        {
            return _skinGeometry;
        }

        const std::vector<int>& hideEquipSlotIndexes() const
        {
            return _hideEquipSlotIndexes;
        }

        // is the player playing standing or sitting
        bool isPlayingSeated = false;
        float comfortSneakHackStaticBodyPitchAngle = 0;

        // HMD, body, and leg slack adjust offsets for standing/sitting and in/out of Power Armor
        float playerHMDOffsetUpStanding = 0;
        float playerBodyOffsetUpStanding = 0;
        float playerLegSlackAdjustOffsetStanding = 0;
        float playerBodyOffsetForwardStanding = 0;
        float playerHMDOffsetUpSitting = 0;
        float playerBodyOffsetUpSitting = 0;
        float playerLegSlackAdjustOffsetSitting = 0;
        float playerBodyOffsetForwardSitting = 0;

        float playerHMDOffsetUpStandingInPA = 0;
        float playerBodyOffsetUpStandingInPA = 0;
        float playerLegSlackAdjustOffsetStandingInPA = 0;
        float playerBodyOffsetForwardStandingInPA = 0;
        float playerHMDOffsetUpSittingInPA = 0;
        float playerBodyOffsetUpSittingInPA = 0;
        float playerLegSlackAdjustOffsetSittingInPA = 0;
        float playerBodyOffsetForwardSittingInPA = 0;

        float headBackPositionOffset = 0;
        float skeletonLegSlackTarget = 0;

        // Pipboy
        float pipBoyScale = 0;
        bool hidePipboy = false;
        bool isHoloPipboy = false;
        bool holoPipboyKeepWristModel = false;
        bool leftHandedPipBoy = false;
        bool enablePrimaryControllerPipboyUse = false;
        bool pipboyOpenWhenLookAt = false;
        bool pipboyOpenWithButtonOnlyWhenLookingAt = false;
        bool pipboyCloseWhenLookAway = false;
        bool pipboyCloseWhenMovingWhileLookingAway = false;
        bool pipboyHolsterWeaponForOperation = false;
        float pipboyLookAtThreshold = 0;
        float pipboyLookAwayThreshold = 0;
        float pipboyButtonLookAtThreshold = 0;
        float pipboyOperationFingerDetectionRange = 0;
        int pipBoyOnDelay = 0;
        int pipBoyOffDelay = 0;
        // Controller input that opens / closes the Pipboy.
        vrcf::InputBinding pipboyOpenBinding{ vrcf::Hand::Offhand, vrcf::ActivationType::Tap, vr::k_EButton_SteamVR_Trigger };
        vrcf::InputBinding pipboyCloseBinding{ vrcf::Hand::Offhand, vrcf::ActivationType::Tap, vr::k_EButton_Grip };
        // Controller input that must be held to move the holo Pipboy screen (see DampenPipboyScreenMode::HoldInPlace).
        vrcf::InputBinding holdPipboyScreenBinding;
        // Controller input that enters the wrist-Pipboy configuration mode (long-press the favorites button).
        vrcf::InputBinding enterPipboyConfigBinding;

        // Pipboy Torch/Flashlight
        bool removeFlashlight = false;
        bool flashlightEnabled = false;
        FlashlightLocation flashlightLocation = FlashlightLocation::Head;
        // Switch the torch between head and hand: tapped on whichever hand is near the head (one binding per hand).
        vrcf::InputBinding switchTorchLeftBinding{ vrcf::Hand::Left, vrcf::ActivationType::Tap, vr::k_EButton_Grip };
        vrcf::InputBinding switchTorchRightBinding{ vrcf::Hand::Right, vrcf::ActivationType::Tap, vr::k_EButton_Grip };

        // Fallout London VR support
        vrcf::InputBinding attaboyGrabBinding{ vrcf::Hand::Left, vrcf::ActivationType::Tap, vr::k_EButton_Grip };
        float attaboyGrabActivationDistance = 0;

        // Weapon offhand grip
        bool enableOffHandGripping = false;
        bool enableGripButtonToGrap = false;
        bool enableGripButtonToLetGo = false;
        bool onePressGripButton = false;
        float gripLetGoThreshold = 0;
        // Offhand input that grips / releases a two-handed weapon (press = grab and release toggle, modes 2/3/4).
        vrcf::InputBinding offhandGripBinding{ vrcf::Hand::Offhand, vrcf::ActivationType::Press, vr::k_EButton_Grip };
        // Offhand input that must stay held to keep gripping (mode 3 releases the grip when this is no longer held).
        vrcf::InputBinding offhandGripHoldBinding{ vrcf::Hand::Offhand, vrcf::ActivationType::HoldDown, vr::k_EButton_Grip };

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
        // Controller input that toggles selfie mode (only when the Pipboy is off, as it shares the button).
        vrcf::InputBinding toggleSelfieBinding;
        float scopeAdjustDistance = 0;
        // Controller input that opens the in-VR configuration menu. Default: long-press both thumbsticks
        // (primary thumbstick held past the duration while the offhand thumbstick is also held).
        vrcf::InputBinding openMainConfigBinding;

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

        // is the game is a Fallout London VR modded version
        bool isFalloutLondonVR = false;
        bool ignoreFalloutLondonVR = false;

    protected:
        virtual void loadIniConfigInternal(const CSimpleIniA& ini) override;
        virtual void saveIniConfigInternal(CSimpleIniA& ini) override;

    private:
        void loadHideMeshes();
        void loadHideEquipmentSlots();
        void loadPipboyOffsets();
        void loadWeaponsOffsets();
        static void setupFolders();
        static void migrateConfigFilesIfNeeded();
        static std::string getWeaponNameWithMode(const std::string& name, const WeaponOffsetsMode& mode, bool inPA, bool leftHanded);
        std::string getPipboyOffsetKey() const;
        std::string getPipboyOffsetPath() const;

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
    inline Config g_config; // NOLINT(clang-diagnostic-unique-object-duplication)
}
