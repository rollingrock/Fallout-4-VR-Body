// ReSharper disable StringLiteralTypo

#include "Config.h"

#include <filesystem>

#include "common/CommonUtils.h"
#include "f4vr/F4VRUtils.h"
#include "vrcf/VRControllersManager.h"

using namespace common;

namespace frik
{
    namespace
    {
        /**
         * A right-handed weapon offset as the left hand holds it: a half turn about the barrel and z flipped, which is what
         * every shipped right/left weapon pair differs by (32 pairs, median 1.4 degrees off the hand-tuned left one).
         */
        RE::NiTransform mirroredForLeftHand(RE::NiTransform transform)
        {
            for (int i = 0; i < 3; ++i) {
                transform.rotate.entry[i][1] = -transform.rotate.entry[i][1];
                transform.rotate.entry[i][2] = -transform.rotate.entry[i][2];
            }
            transform.translate.z = -transform.translate.z;
            return transform;
        }
    }

    /**
     * Load the FRIK.toml config, hide meshes, and weapon offsets.
     * Handle creating the FRIK.toml file if it doesn't exist, from the player's FRIK.ini when there is one.
     * Handle updating the FRIK.toml file if the version is old.
     */
    void Config::load()
    {
        setupFolders();

        logger::info("Load config...");
        loadConfig();

        logger::info("Load hide meshes...");
        loadHideMeshes();

        logger::info("Load pipboy offsets...");
        loadPipboyOffsets();

        logger::info("Load weapon offsets...");
        loadWeaponsOffsets();
    }

    /**
     * Load only the FRIK.toml config from the file and override in-memory values.
     */
    void Config::loadConfigOnly()
    {
        logger::info("Load config only...");
        loadConfig();
    }

    /**
     * Reloads the config for Fallout London VR mod.
     * Using different config file as the mod changes a lot of the game.
     */
    void Config::reloadForFalloutLondonVR()
    {
        switchConfigFile(FRIK_FOLVR_CONFIG_PATH);
    }

    void Config::setFlashlightLocation(const FlashlightLocation location)
    {
        flashlightLocation = location;
        saveConfigValue(CONFIG_SECTION_MAIN, "iFlashlightLocation", static_cast<int>(flashlightLocation));
    }

    void Config::toggleIsHoloPipboy()
    {
        isHoloPipboy = !isHoloPipboy;
        saveConfigValue(CONFIG_SECTION_MAIN, "HoloPipBoyEnabled", isHoloPipboy);
    }

    void Config::toggleDampenPipboyScreen()
    {
        dampenPipboyScreenMode = static_cast<DampenPipboyScreenMode>((static_cast<uint8_t>(dampenPipboyScreenMode) + 1) % 3);
        saveConfigValue(CONFIG_SECTION_MAIN, "iDampenPipboyScreenMode", static_cast<int>(dampenPipboyScreenMode));
    }

    void Config::togglePipBoyOpenWhenLookAt()
    {
        pipboyOpenWhenLookAt = !pipboyOpenWhenLookAt;
        saveConfigValue(CONFIG_SECTION_MAIN, "PipBoyOpenWhenLookAt", pipboyOpenWhenLookAt);
    }

    void Config::togglePipBoyCloseWhenLookAway()
    {
        pipboyCloseWhenLookAway = !pipboyCloseWhenLookAway;
        saveConfigValue(CONFIG_SECTION_MAIN, "PipBoyCloseWhenLookAway", pipboyCloseWhenLookAway);
    }

    void Config::savePipboyScale(const float scale)
    {
        pipBoyScale = scale;
        saveConfigValue(CONFIG_SECTION_MAIN, "PipboyScale", pipBoyScale);
    }

    void Config::saveIsPlayingSeated(const bool iIsPlayingSeated)
    {
        this->isPlayingSeated = iIsPlayingSeated;
        saveConfigValue(CONFIG_SECTION_MAIN, "bIsPlayingSeated", iIsPlayingSeated);
    }

    void Config::saveHideHeadEquipment(const bool hide)
    {
        hideHeadEquipment = hide;
        saveConfigValue(CONFIG_SECTION_MAIN, "bHidePlayerHeadEquipment", hide);
    }

    void Config::saveDampenHands(const bool iDampenHands)
    {
        this->dampenHands = iDampenHands;
        saveConfigValue(CONFIG_SECTION_MAIN, "DampenHands", iDampenHands);
    }

    float Config::getPlayerBodyOffsetUp() const
    {
        if (isPlayingSeated) {
            return f4vr::isInPowerArmor() ? playerBodyOffsetUpSittingInPA : playerBodyOffsetUpSitting;
        }
        return f4vr::isInPowerArmor() ? playerBodyOffsetUpStandingInPA : playerBodyOffsetUpStanding;
    }

    void Config::setPlayerBodyOffsetUp(const float value)
    {
        if (isPlayingSeated) {
            if (f4vr::isInPowerArmor()) {
                playerBodyOffsetUpSittingInPA = value;
            } else {
                playerBodyOffsetUpSitting = value;
            }
        } else {
            if (f4vr::isInPowerArmor()) {
                playerBodyOffsetUpStandingInPA = value;
            } else {
                playerBodyOffsetUpStanding = value;
            }
        }
    }

    float Config::getPlayerBodyOffsetForward() const
    {
        if (isPlayingSeated) {
            return f4vr::isInPowerArmor() ? playerBodyOffsetForwardSittingInPA : playerBodyOffsetForwardSitting;
        }
        return f4vr::isInPowerArmor() ? playerBodyOffsetForwardStandingInPA : playerBodyOffsetForwardStanding;
    }

    void Config::setPlayerBodyOffsetForward(const float value)
    {
        if (isPlayingSeated) {
            if (f4vr::isInPowerArmor()) {
                playerBodyOffsetForwardSittingInPA = value;
            } else {
                playerBodyOffsetForwardSitting = value;
            }
        } else {
            if (f4vr::isInPowerArmor()) {
                playerBodyOffsetForwardStandingInPA = value;
            } else {
                playerBodyOffsetForwardStanding = value;
            }
        }
    }

    float Config::getPlayerHMDOffsetUp() const
    {
        if (isPlayingSeated) {
            return f4vr::isInPowerArmor() ? playerHMDOffsetUpSittingInPA : playerHMDOffsetUpSitting;
        }
        return f4vr::isInPowerArmor() ? playerHMDOffsetUpStandingInPA : playerHMDOffsetUpStanding;
    }

    void Config::setPlayerHMDOffsetUp(const float value)
    {
        if (isPlayingSeated) {
            if (f4vr::isInPowerArmor()) {
                playerHMDOffsetUpSittingInPA = value;
            } else {
                playerHMDOffsetUpSitting = value;
            }
        } else {
            if (f4vr::isInPowerArmor()) {
                playerHMDOffsetUpStandingInPA = value;
            } else {
                playerHMDOffsetUpStanding = value;
            }
        }
    }

    float Config::getPlayerLegSlackAdjustOffset() const
    {
        if (isPlayingSeated) {
            return f4vr::isInPowerArmor() ? playerLegSlackAdjustOffsetSittingInPA : playerLegSlackAdjustOffsetSitting;
        }
        return f4vr::isInPowerArmor() ? playerLegSlackAdjustOffsetStandingInPA : playerLegSlackAdjustOffsetStanding;
    }

    void Config::setPlayerLegSlackAdjustOffset(const float value)
    {
        if (isPlayingSeated) {
            if (f4vr::isInPowerArmor()) {
                playerLegSlackAdjustOffsetSittingInPA = value;
            } else {
                playerLegSlackAdjustOffsetSitting = value;
            }
        } else {
            if (f4vr::isInPowerArmor()) {
                playerLegSlackAdjustOffsetStandingInPA = value;
            } else {
                playerLegSlackAdjustOffsetStanding = value;
            }
        }
    }

    /**
     * Open the FRIK.toml file in Notepad for editing.
     */
    void Config::openInNotepad()
    {
        ShellExecuteA(nullptr, "open", "notepad.exe", FRIK_CONFIG_PATH.c_str(), nullptr, SW_SHOWNORMAL);
    }

    /**
     * Get the Pipboy offset of the currently used Pipboy type.
     */
    RE::NiTransform Config::getPipboyOffset()
    {
        return _pipboyOffsets[getPipboyOffsetKey()];
    }

    /**
     * Get the factory-default Pipboy offset for the currently used Pipboy type, read from the embedded resources.
     */
    RE::NiTransform Config::getDefaultPipboyOffset()
    {
        auto defaults = loadEmbeddedOffsets(IDR_PIPBOY_HOLO_OFFSETS, IDR_PIPBOY_ATTABOY_OFFSETS);
        return defaults[getPipboyOffsetKey()];
    }

    /**
     * Save the Pipboy offset to the offsets map.
     */
    bool Config::savePipboyOffset(const RE::NiTransform& transform)
    {
        const auto key = getPipboyOffsetKey();
        _pipboyOffsets[key] = transform;
        return saveOffsetsToJsonFile(key, transform, getPipboyOffsetPath());
    }

    /**
     * Get weapon offsets for given weapon name and mode.
     * Use non-PA mode if PA mode offsets not found.
     */
    std::optional<RE::NiTransform> Config::getWeaponOffsets(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA) const
    {
        const bool leftHanded = f4vr::isLeftHandedMode();
        const auto it = _weaponsOffsets.find(getWeaponNameWithMode(name, mode, inPA, leftHanded));
        if (it != _weaponsOffsets.end()) {
            return it->second;
        }
        // a left-handed player without a left-handed weapon entry gets the right-handed one mirrored, not as is (that held
        // the gun upside down); the hand-pose modes have no such relation between their pairs, so they still get none
        if (leftHanded && mode == WeaponOffsetsMode::Weapon) {
            const auto rightIt = _weaponsOffsets.find(getWeaponNameWithMode(name, mode, inPA, false));
            if (rightIt != _weaponsOffsets.end()) {
                return mirroredForLeftHand(rightIt->second);
            }
        }
        // Check without PA (historic)
        return inPA ? getWeaponOffsets(name, mode, false) : std::nullopt;
    }

    /**
     * Save the weapon offset to config and filesystem.
     */
    bool Config::saveWeaponOffsets(const std::string& name, const RE::NiTransform& transform, const WeaponOffsetsMode& mode, const bool inPA)
    {
        const auto fullName = getWeaponNameWithMode(name, mode, inPA, f4vr::isLeftHandedMode());
        _weaponsOffsets[fullName] = transform;
        const auto offsetFilePath = fs::path{ WEAPONS_OFFSETS_PATH } / (sanitizePathWindows(fullName) + ".json");
        return saveOffsetsToJsonFile(fullName, transform, offsetFilePath.string());
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
     * Read the config values from the loaded FRIK.toml, each with its default when the file does not have it.
     */
    void Config::loadConfigInternal(const toml::table& config)
    {
        const auto mainSection = config[CONFIG_SECTION_MAIN];
        const auto smoothMovement = config[CONFIG_SECTION_SMOOTH_MOVEMENT];

        // Player/Skeleton
        setScale = mainSection["setScale"].value_or(true);
        fVrScale = mainSection["fVrScale"].value_or(70.0f);
        playerHeight = mainSection["PlayerHeight"].value_or(120.4828f);
        armLength = mainSection["armLength"].value_or(36.74f);

        // Arm solver
        armElbowMinFlexionDeg = mainSection["fArmElbowMinFlexionDeg"].value_or(10.0f);
        armElbowMaxFlexionDeg = mainSection["fArmElbowMaxFlexionDeg"].value_or(145.0f);
        armTwistWindowMinDeg = mainSection["fArmTwistWindowMinDeg"].value_or(15.0f);
        armSmoothBlends = mainSection["bArmSmoothBlends"].value_or(true);
        armShoulderReachFraction = mainSection["fArmShoulderReachFraction"].value_or(0.15f);
        armShoulderDownwardDamp = mainSection["fArmShoulderDownwardDamp"].value_or(0.4f);
        armUpperTwistSplit = mainSection["fArmUpperTwistSplit"].value_or(0.0f);
        armForearmTwistWeight1 = mainSection["fArmForearmTwistWeight1"].value_or(0.0f);
        armForearmTwistWeight2 = mainSection["fArmForearmTwistWeight2"].value_or(0.5f);
        armForearmTwistWeight3 = mainSection["fArmForearmTwistWeight3"].value_or(0.5f);
        armFrameRateIndependentSmoothing = mainSection["bArmFrameRateIndependentSmoothing"].value_or(true);

        // Head Geometry Hide
        hideHead = mainSection["bHidePlayerHead"].value_or(false);
        hideHeadEquipment = mainSection["bHidePlayerHeadEquipment"].value_or(false);
        hideSkin = mainSection["bHidePlayerSkin"].value_or(false);

        // is the player playing standing or sitting
        isPlayingSeated = mainSection["bIsPlayingSeated"].value_or(false);

        // HMD, body, and leg slack adjust offsets
        playerHMDOffsetUpStanding = mainSection["fPlayerHMDOffsetUpStanding"].value_or(8.0f);
        playerBodyOffsetUpStanding = mainSection["fPlayerBodyOffsetUpStanding"].value_or(-3.0f);
        playerLegSlackAdjustOffsetStanding = mainSection["fPlayerLegSlackAdjustOffsetStanding"].value_or(0.0f);
        playerBodyOffsetForwardStanding = mainSection["fPlayerBodyOffsetForwardStanding"].value_or(-3.0f);
        playerHMDOffsetUpSitting = mainSection["fPlayerHMDOffsetUpSitting"].value_or(31.0f);
        playerBodyOffsetUpSitting = mainSection["fPlayerBodyOffsetUpSitting"].value_or(-4.0f);
        playerLegSlackAdjustOffsetSitting = mainSection["fPlayerLegSlackAdjustOffsetSitting"].value_or(0.0f);
        playerBodyOffsetForwardSitting = mainSection["fPlayerBodyOffsetForwardSitting"].value_or(2.0f);

        playerHMDOffsetUpStandingInPA = mainSection["fPlayerHMDOffsetUpStandingInPA"].value_or(23.0f);
        playerBodyOffsetUpStandingInPA = mainSection["fPlayerBodyOffsetUpStandingInPA"].value_or(-8.0f);
        playerLegSlackAdjustOffsetStandingInPA = mainSection["fPlayerLegSlackAdjustOffsetStandingInPA"].value_or(0.0f);
        playerBodyOffsetForwardStandingInPA = mainSection["fPlayerBodyOffsetForwardStandingInPA"].value_or(-15.0f);
        playerHMDOffsetUpSittingInPA = mainSection["fPlayerHMDOffsetUpSittingInPA"].value_or(53.0f);
        playerBodyOffsetUpSittingInPA = mainSection["fPlayerBodyOffsetUpSittingInPA"].value_or(-7.0f);
        playerLegSlackAdjustOffsetSittingInPA = mainSection["fPlayerLegSlackAdjustOffsetSittingInPA"].value_or(0.0f);
        playerBodyOffsetForwardSittingInPA = mainSection["fPlayerBodyOffsetForwardSittingInPA"].value_or(-9.0f);

        headBackPositionOffset = mainSection["fHeadBackPositionOffset"].value_or(4.0f);
        skeletonLegSlackTarget = mainSection["fSkeletonLegSlackTarget"].value_or(2.5f);

        comfortSneakHackStaticBodyPitchAngle = mainSection["fComfortSneakHackStaticBodyPitchAngle"].value_or(30.0f);

        // Pipboy
        pipBoyScale = mainSection["PipboyScale"].value_or(1.0f);
        hidePipboy = mainSection["hidePipboy"].value_or(false);
        isHoloPipboy = mainSection["HoloPipBoyEnabled"].value_or(true);
        holoPipboyKeepWristModel = mainSection["HoloPipBoyKeepWristModel"].value_or(false);
        leftHandedPipBoy = mainSection["PipboyRightArmLeftHandedMode"].value_or(false);
        enablePrimaryControllerPipboyUse = mainSection["PipboyUIPrimaryController"].value_or(true);
        pipboyOpenWhenLookAt = mainSection["PipBoyOpenWhenLookAt"].value_or(false);
        pipboyOpenWithButtonOnlyWhenLookingAt = mainSection["PipBoyOpenWithButtonOnlyWhenLookingAt"].value_or(false);
        pipboyCloseWhenLookAway = mainSection["PipBoyCloseWhenLookAway"].value_or(false);
        pipboyCloseWhenMovingWhileLookingAway = mainSection["AllowMovementWhenNotLookingAtPipboy"].value_or(true);
        pipboyHolsterWeaponForOperation = mainSection["bHolsterWeaponForOperation"].value_or(false);
        pipboyLookAtThreshold = mainSection["PipBoyLookAtThreshold"].value_or(0.75f);
        pipboyLookAwayThreshold = mainSection["PipBoyLookAwayThreshold"].value_or(0.3f);
        pipboyButtonLookAtThreshold = mainSection["PipBoyButtonLookAtThreshold"].value_or(0.5f);
        pipboyOperationFingerDetectionRange = mainSection["fPipboyOperationFingerDetectionRange"].value_or(12.0f);
        pipBoyOnDelay = mainSection["PipBoyOnDelay"].value_or(400);
        pipBoyOffDelay = mainSection["PipBoyOffDelay"].value_or(1000);
        pipboyOpenBinding = getInputBindingValue(config,
            CONFIG_SECTION_MAIN,
            "sPipboyOpenButton",
            vrcf::InputBinding{ vrcf::Hand::Offhand, vrcf::ActivationType::Tap, vr::k_EButton_SteamVR_Trigger });
        pipboyCloseBinding =
            getInputBindingValue(config, CONFIG_SECTION_MAIN, "sPipboyCloseButton", vrcf::InputBinding{ vrcf::Hand::Offhand, vrcf::ActivationType::Tap, vr::k_EButton_Grip });
        holdPipboyScreenBinding = getInputBindingValue(config,
            CONFIG_SECTION_MAIN,
            "sHoldPipboyScreenButton",
            vrcf::InputBinding{
                .hand = vrcf::Hand::Offhand,
                .type = vrcf::ActivationType::HoldDown,
                .button = vr::k_EButton_Grip,
                .duration = 0.3f,
            });
        enterPipboyConfigBinding = getInputBindingValue(config,
            CONFIG_SECTION_MAIN,
            "sEnterPipboyConfigButton",
            vrcf::InputBinding{
                .hand = vrcf::Hand::Primary,
                .type = vrcf::ActivationType::LongPress,
                .button = vr::k_EButton_SteamVR_Touchpad,
                .duration = 2.0f,
            });

        // Torch/Flashlight
        removeFlashlight = mainSection["bRemoveFlashlight"].value_or(false);
        flashlightLocation = static_cast<FlashlightLocation>(mainSection["iFlashlightLocation"].value_or(0));
        // change hand / head button (one binding per hand)
        switchTorchLeftBinding =
            getInputBindingValue(config, CONFIG_SECTION_MAIN, "sSwitchTorchLeftButton", vrcf::InputBinding{ vrcf::Hand::Left, vrcf::ActivationType::Tap, vr::k_EButton_Grip });
        switchTorchRightBinding =
            getInputBindingValue(config, CONFIG_SECTION_MAIN, "sSwitchTorchRightButton", vrcf::InputBinding{ vrcf::Hand::Right, vrcf::ActivationType::Tap, vr::k_EButton_Grip });

        // Fallout London VR support: the Attaboy grab proximity gesture (anchored to the on-belt Attaboy node).
        static const f4vr::WandActivationConfig attaboyGrabDefaults = [] {
            f4vr::WandActivationConfig cfg;
            cfg.zone.MakeIdentity();
            cfg.zone.scale = 30.0f; // sphere diameter in game units (radius = scale * beltNodeWorldScale * 0.5)
            cfg.primary = vrcf::InputBinding{ vrcf::Hand::Left, vrcf::ActivationType::Tap, vr::k_EButton_Grip };
            cfg.entryHaptic = vrcf::HapticPattern::Click; // matches the previous "hand entered the grab zone" haptic
            cfg.primaryHaptic = vrcf::HapticPattern::DoubleClick;
            cfg.showSphere = f4vr::ActivationSphereVisibility::Never;
            return cfg;
        }();
        attaboyGrab = loadWandActivationConfig(config, CONFIG_SECTION_ATTABOY_GRAB, attaboyGrabDefaults);

        // Two-handed gripping
        enableOffHandGripping = mainSection["EnableOffHandGripping"].value_or(true);
        enableGripButtonToGrap = mainSection["EnableGripButton"].value_or(true);
        enableGripButtonToLetGo = mainSection["EnableGripButtonToLetGo"].value_or(true);
        onePressGripButton = mainSection["EnableGripButtonOnePress"].value_or(true);
        gripLetGoThreshold = mainSection["GripLetGoThreshold"].value_or(15.0f);
        offhandGripBinding =
            getInputBindingValue(config, CONFIG_SECTION_MAIN, "sGripButton", vrcf::InputBinding{ vrcf::Hand::Offhand, vrcf::ActivationType::Press, vr::k_EButton_Grip });
        offhandGripHoldBinding =
            getInputBindingValue(config, CONFIG_SECTION_MAIN, "sGripHoldButton", vrcf::InputBinding{ vrcf::Hand::Offhand, vrcf::ActivationType::HoldDown, vr::k_EButton_Grip });

        // Dampen hands
        dampenHands = mainSection["DampenHands"].value_or(true);
        dampenHandsInVanillaScope = mainSection["DampenHandsInVanillaScope"].value_or(true);
        hideBodyInVanillaScope = mainSection["HideBodyInVanillaScope"].value_or(true);
        dampenHandsRotation = mainSection["DampenHandsRotation"].value_or(0.7f);
        dampenHandsTranslation = mainSection["DampenHandsTranslation"].value_or(0.7f);
        dampenHandsRotationInVanillaScope = mainSection["DampenHandsRotationInVanillaScope"].value_or(0.2f);
        dampenHandsTranslationInVanillaScope = mainSection["DampenHandsTranslationInVanillaScope"].value_or(0.2f);

        // Dampen Pipboy
        dampenPipboyScreenMode = static_cast<DampenPipboyScreenMode>(mainSection["iDampenPipboyScreenMode"].value_or(1));
        dampenPipboyThreshold = mainSection["fDampenPipboyThreshold"].value_or(1.1f);
        dampenPipboyMultiplier = mainSection["fDampenPipboyMultiplier"].value_or(0.7f);

        // Misc
        showPAHUD = mainSection["showPAHUD"].value_or(false);
        selfieOutFrontDistance = mainSection["selfieOutFrontDistance"].value_or(120.0f);
        selfieIgnoreHideFlags = mainSection["bSelfieIgnoreHideFlags"].value_or(true);
        toggleSelfieBinding =
            getInputBindingValue(config, CONFIG_SECTION_MAIN, "sToggleSelfieButton", vrcf::InputBinding{ vrcf::Hand::Primary, vrcf::ActivationType::Tap, vr::k_EButton_A });
        scopeAdjustDistance = mainSection["ScopeAdjustDistance"].value_or(15.0f);
        openMainConfigBinding = getInputBindingValue(config,
            CONFIG_SECTION_MAIN,
            "sOpenConfigButton",
            vrcf::InputBinding{
                .hand = vrcf::Hand::Primary,
                .type = vrcf::ActivationType::LongPress,
                .button = vr::k_EButton_SteamVR_Touchpad,
                .modifier = vrcf::InputModifier{ vr::k_EButton_SteamVR_Touchpad, vrcf::Hand::Offhand },
                .duration = 1.5f,
            });

        // special disable of Fallout London so Pipboy can be used - a hacky hack for a specific player
        ignoreFalloutLondonVR = mainSection["bIgnoreFalloutLondonVR"].value_or(true);

        //Smooth Movement
        disableSmoothMovement = smoothMovement["DisableSmoothMovement"].value_or(false);
        smoothingAmount = smoothMovement["SmoothAmount"].value_or(15.0f);
        smoothingAmountHorizontal = smoothMovement["SmoothAmountHorizontal"].value_or(5.0f);
        dampingMultiplier = smoothMovement["Damping"].value_or(1.0f);
        dampingMultiplierHorizontal = smoothMovement["DampingHorizontal"].value_or(1.0f);
        stoppingMultiplier = smoothMovement["StoppingMultiplier"].value_or(0.6f);
        stoppingMultiplierHorizontal = smoothMovement["StoppingMultiplierHorizontal"].value_or(0.6f);
        disableInteriorSmoothing = smoothMovement["DisableInteriorSmoothing"].value_or(true);
        disableInteriorSmoothingHorizontal = smoothMovement["DisableInteriorSmoothingHorizontal"].value_or(true);
    }

    /**
     * Save the values the in-game config menus change, all with one write of FRIK.toml.
     */
    void Config::save()
    {
        saveConfigValues(CONFIG_SECTION_MAIN,
            {
                { "bIsPlayingSeated", isPlayingSeated },
                { "fVrScale", fVrScale },
                { "bHidePlayerHead", hideHead },
                { "bHidePlayerHeadEquipment", hideHeadEquipment },

                { "fPlayerBodyOffsetUpStanding", playerBodyOffsetUpStanding },
                { "fPlayerBodyOffsetForwardStanding", playerBodyOffsetForwardStanding },
                { "fPlayerHMDOffsetUpStanding", playerHMDOffsetUpStanding },
                { "fPlayerLegSlackAdjustOffsetStanding", playerLegSlackAdjustOffsetStanding },
                { "fPlayerBodyOffsetUpSitting", playerBodyOffsetUpSitting },
                { "fPlayerBodyOffsetForwardSitting", playerBodyOffsetForwardSitting },
                { "fPlayerHMDOffsetUpSitting", playerHMDOffsetUpSitting },
                { "fPlayerLegSlackAdjustOffsetSitting", playerLegSlackAdjustOffsetSitting },

                { "fPlayerBodyOffsetUpStandingInPA", playerBodyOffsetUpStandingInPA },
                { "fPlayerBodyOffsetForwardStandingInPA", playerBodyOffsetForwardStandingInPA },
                { "fPlayerHMDOffsetUpStandingInPA", playerHMDOffsetUpStandingInPA },
                { "fPlayerLegSlackAdjustOffsetStandingInPA", playerLegSlackAdjustOffsetStandingInPA },
                { "fPlayerBodyOffsetUpSittingInPA", playerBodyOffsetUpSittingInPA },
                { "fPlayerBodyOffsetForwardSittingInPA", playerBodyOffsetForwardSittingInPA },
                { "fPlayerHMDOffsetUpSittingInPA", playerHMDOffsetUpSittingInPA },
                { "fPlayerLegSlackAdjustOffsetSittingInPA", playerLegSlackAdjustOffsetSittingInPA },

                { "fSkeletonLegSlackTarget", skeletonLegSlackTarget },
                { "armLength", armLength },
                { "hidePipboy", hidePipboy },
                { "PipboyScale", pipBoyScale },
                { "HoloPipBoyEnabled", isHoloPipboy },
                { "HoloPipBoyKeepWristModel", holoPipboyKeepWristModel },
                { "iFlashlightLocation", static_cast<int>(flashlightLocation) },
                { "iDampenPipboyScreenMode", static_cast<int>(dampenPipboyScreenMode) },
                { "PipBoyOpenWhenLookAt", pipboyOpenWhenLookAt },
                { "DampenHands", dampenHands },

                { "EnableGripButton", enableGripButtonToGrap },
                { "EnableGripButtonToLetGo", enableGripButtonToLetGo },
                { "EnableGripButtonOnePress", onePressGripButton },
            });
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

        std::unordered_map<std::string, int> slotToIndexMap = { { "hairtop", 0 }, // i.e. helmet
            { "hairlong", 1 },
            { "head", 2 },
            { "headband", 16 },
            { "eyes", 17 }, // i.e. glasses
            { "beard", 18 },
            { "mouth", 19 },
            { "neck", 20 },
            { "scalp", 22 } };

        _hideEquipSlotIndexes.clear();
        for (auto& geometry : slotsGeometry) {
            if (slotToIndexMap.contains(geometry)) {
                _hideEquipSlotIndexes.push_back(slotToIndexMap[geometry]);
            }
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
        createDirDeep(FRIK_CONFIG_PATH);
        createDirDeep(MESH_HIDE_FACE_INI_PATH);
        createDirDeep(PIPBOY_HOLO_OFFSETS_PATH);
        createDirDeep(WEAPONS_OFFSETS_PATH);
    }

    /**
     * Get the name for the weapon offset to use depending on the mode.
     * Basically a hack to store multiple modes of the same weapon by adding suffix to the name.
     */
    std::string Config::getWeaponNameWithMode(const std::string& name, const WeaponOffsetsMode& mode, const bool inPA, const bool leftHanded)
    {
        static const std::string POWER_ARMOR_SUFFIX{ "-PowerArmor" };
        static const std::string PRIM_HAND_SUFFIX{ "-primHand" };
        static const std::string OFF_HAND_SUFFIX{ "-offHand" };
        static const std::string THROWABLE_SUFFIX{ "-throwable" };
        static const std::string BACK_OF_HAND_SUFFIX{ "-backOfHand" };
        static const std::string LEFT_HANDED_SUFFIX{ "-leftHanded" };
        switch (mode) {
        case WeaponOffsetsMode::Weapon:
            return name + (inPA ? POWER_ARMOR_SUFFIX : "") + (leftHanded ? LEFT_HANDED_SUFFIX : "");
        case WeaponOffsetsMode::PrimaryHand:
            return name + PRIM_HAND_SUFFIX + (inPA ? POWER_ARMOR_SUFFIX : "") + (leftHanded ? LEFT_HANDED_SUFFIX : "");
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
}
