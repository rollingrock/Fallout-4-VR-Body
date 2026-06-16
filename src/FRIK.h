#pragma once

#include <Version.h>

#include "Config.h"
#include "ModBase.h"
#include "PlayerControlsHandler.h"
#include "config-mode/MainConfigMode.h"
#include "config-mode/PipboyConfigMode.h"
#include "f4vr/GameMenusHandler.h"
#include "pipboy/Pipboy.h"
#include "skeleton/BoneSpheresHandler.h"
#include "smooth-movement/SmoothMovementVR.h"
#include "weapon-position/WeaponPositionAdjuster.h"

namespace frik
{
    constexpr auto BETTER_SCOPES_VR_MOD_NAME = "FO4VRBETTERSCOPES";

    class FRIK : public f4cf::ModBase
    {
    public:
        FRIK()
            : ModBase({ Version::PROJECT, "F4VRBody", Version::NAME, &g_config, Version::PROJECT, 512, true, true })
        {}

        bool isSkeletonReady() const
        {
            return _skelly != nullptr;
        }

        bool isInScopeMenu()
        {
            return _gameMenusHandler.isInScopeMenu();
        }
        bool isPauseMenuOpen()
        {
            return _gameMenusHandler.isPauseMenuOpen();
        }
        bool isFavoritesMenuOpen()
        {
            return _gameMenusHandler.isFavoritesMenuOpen();
        }
        bool isDialogueMenuOpen()
        {
            return _gameMenusHandler.isDialogueMenuOpen();
        }

        bool isSelfieModeOn() const
        {
            return _selfieMode;
        }
        void setSelfieMode(const bool selfieMode)
        {
            _selfieMode = selfieMode;
        }
        float getDynamicCameraHeight() const
        {
            return _dynamicCameraHeight;
        }
        void setDynamicCameraHeight(const float dynamicCameraHeight)
        {
            _dynamicCameraHeight = dynamicCameraHeight;
        }
        bool isLookingThroughScope() const
        {
            return _isLookingThroughScope;
        }
        void setLookingThroughScope(const bool isLookingThroughScope)
        {
            _isLookingThroughScope = isLookingThroughScope;
        }

        bool isPipboyOn() const
        {
            return _pipboy && _pipboy->isOpen();
        }
        bool isPipboyOperatingWithFinger() const
        {
            return _pipboy && _pipboy->isOperatingWithFinger();
        }
        void swapPipboyModel() const
        {
            if (_pipboy) {
                _pipboy->swapModel();
            }
        }
        void closePipboy() const
        {
            if (_pipboy) {
                _pipboy->openClose(false);
            }
        }

        bool isMainConfigurationModeActive() const
        {
            return _mainConfigMode.isOpen();
        }
        bool isPipboyConfigurationModeActive() const
        {
            return _pipboyConfigMode && _pipboyConfigMode->isPipBoyConfigModeActive();
        }
        bool isPipboyConfigurationModeAdjusting() const
        {
            return _pipboyConfigMode && _pipboyConfigMode->isAdjusting();
        }
        void openMainConfigurationModeActive()
        {
            _mainConfigMode.openConfigMode();
        }

        void openPipboyConfigurationModeActive() const
        {
            if (_pipboy) {
                _pipboy->openClose(true);
            }
            if (_pipboyConfigMode) {
                _pipboyConfigMode->openPipboyConfigurationMode();
            }
        }

        void closePipboyConfigurationModeActive() const
        {
            if (_pipboyConfigMode) {
                _pipboyConfigMode->exitPBConfig();
            }
        }

        void registerOpenSettingButton(const OpenExternalModConfigData& data)
        {
            _mainConfigMode.registerOpenExternalModSettingButton(data);
        }

        bool isMeleeWeaponDrawn() const
        {
            return _weaponPosition && _weaponPosition->isMeleeWeaponDrawn();
        }
        bool isOffHandGrippingWeapon() const
        {
            return _weaponPosition && _weaponPosition->isOffHandGrippingWeapon();
        }
        bool isOffHandGrippingEnabled() const
        {
            return WeaponPositionAdjuster::isOffHandGrippingEnabled();
        }
        void setOffHandGrippingEnabled(const bool enabled)
        {
            WeaponPositionAdjuster::setOffHandGrippingEnabled(enabled);
        }

        bool inWeaponRepositionMode() const
        {
            return _weaponPosition && _weaponPosition->inWeaponRepositionMode();
        }
        void toggleWeaponRepositionMode() const
        {
            // reposition mode is unavailable while weapon positioning is disabled via API
            if (_weaponPosition && _weaponPositionEnabled) {
                _weaponPosition->toggleWeaponRepositionMode();
            }
        }

        // Feature enable/disable toggled via the public API so other mods can turn off FRIK
        // subsystems they replace (e.g. a mod providing its own flashlight). Default: enabled.
        bool isFlashlightEnabled() const
        {
            return _flashlightEnabled;
        }
        void setFlashlightEnabled(const bool enabled)
        {
            _flashlightEnabled = enabled;
        }
        bool isWeaponPositionEnabled() const
        {
            return _weaponPositionEnabled;
        }
        void setWeaponPositionEnabled(const bool enabled)
        {
            _weaponPositionEnabled = enabled;
            // release transient state (offhand grip pose, reposition mode) so nothing stays stuck while disabled
            if (!enabled && _weaponPosition) {
                _weaponPosition->resetOnDisable();
            }
        }
        bool isPipboyEnabled() const
        {
            return _pipboyEnabled;
        }
        void setPipboyEnabled(const bool enabled)
        {
            _pipboyEnabled = enabled;
            // close the Pipboy and clear finger-operation state on disable so player controls / hand pose don't stay stuck
            if (!enabled && _pipboy) {
                _pipboy->resetOnDisable();
            }
        }
        bool isSmoothMovementEnabled() const
        {
            return _smoothMovementEnabled;
        }
        void setSmoothMovementEnabled(const bool enabled)
        {
            _smoothMovementEnabled = enabled;
        }

        void dispatchMessageToExternalMod(const std::string& receivingModName, std::uint32_t messageType, void* data, std::uint32_t dataLen) const;

        void smoothMovement();

    protected:
        virtual void onModLoaded(const F4SE::LoadInterface* f4SE) override;
        virtual void onGameLoaded() override;
        virtual void onGameSessionLoaded() override;
        virtual void onFrameUpdate() override;
        virtual void checkDebugDump() const override;

    private:
        void initSkeleton();
        void onGameMenuOpened(const std::string& name, bool isOpened);
        void releaseSkeleton();
        static void updateWorldFinal();
        static void configureGameVars();
        static bool isGameReadyForSkeletonInitialization();
        bool isRootNodeValid() const;
        static void addEmbeddedFlashlightKeywordIfNeeded();
        static void onBetterScopesMessage(F4SE::MessagingInterface::Message* msg);
        static void initForFalloutLondonVR();

        bool _inPowerArmor = false;
        bool _isLookingThroughScope = false;
        float _dynamicCameraHeight = 0;
        bool _selfieMode = false;

        // Feature enable/disable flags toggled via the public API (see blockFeature). Default: enabled.
        bool _flashlightEnabled = true;
        bool _weaponPositionEnabled = true;
        bool _pipboyEnabled = true;
        bool _smoothMovementEnabled = true;

        // the currently root node used in skeleton
        RE::NiNode* _workingRootNode = nullptr;

        Skeleton* _skelly = nullptr;
        Pipboy* _pipboy = nullptr;
        MainConfigMode _mainConfigMode;
        PipboyConfigMode* _pipboyConfigMode = nullptr;
        WeaponPositionAdjuster* _weaponPosition = nullptr;

        // handler for the interaction spheres around the skeleton
        BoneSpheresHandler _boneSpheres;

        // handler for smooth movement logic
        SmoothMovementVR _smoothMovement;

        // handler for game menus checking
        f4vr::GameMenusHandler _gameMenusHandler;

        // handler to enable/disable player movement and other controls
        PlayerControlsHandler _playerControlsHandler;
    };

    // The ONE global to rule them ALL
    inline FRIK g_frik;
}
