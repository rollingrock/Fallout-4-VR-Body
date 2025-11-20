#pragma once

#include <Version.h>

#include "ModBase.h"
#include "PlayerControlsHandler.h"
#include "config-mode/ConfigurationMode.h"
#include "config-mode/MainConfigMode.h"
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
        FRIK() :
            ModBase(Settings(
                    "F4VRBody",
                    Version::NAME,
                    &g_config,
                    "FRIK",
                    512,
                    true)
                ) {}

        bool isInScopeMenu() { return _gameMenusHandler.isInScopeMenu(); }
        bool isPauseMenuOpen() { return _gameMenusHandler.isPauseMenuOpen(); }
        bool isFavoritesMenuOpen() { return _gameMenusHandler.isFavoritesMenuOpen(); }

        bool getSelfieMode() const { return _selfieMode; }
        void setSelfieMode(const bool selfieMode) { _selfieMode = selfieMode; }
        float getDynamicCameraHeight() const { return _dynamicCameraHeight; }
        void setDynamicCameraHeight(const float dynamicCameraHeight) { _dynamicCameraHeight = dynamicCameraHeight; }
        bool isLookingThroughScope() const { return _isLookingThroughScope; }
        void setLookingThroughScope(const bool isLookingThroughScope) { _isLookingThroughScope = isLookingThroughScope; }

        bool isPipboyOn() const { return _pipboy && _pipboy->isOpen(); }
        bool isPipboyOperatingWithFinger() const { return _pipboy && _pipboy->isOperatingWithFinger(); }
        void swapPipboyModel() const { if (_pipboy) { _pipboy->swapModel(); } }

        bool isMainConfigurationModeActive() const { return _mainConfigMode.isOpen(); }
        bool isPipboyConfigurationModeActive() const { return _configurationMode && _configurationMode->isPipBoyConfigModeActive(); }
        void openMainConfigurationModeActive() { _mainConfigMode.openConfigMode(); }

        void openPipboyConfigurationModeActive() const
        {
            if (_pipboy) { _pipboy->openClose(true); }
            if (_configurationMode) { _configurationMode->openPipboyConfigurationMode(); }
        }

        void closePipboyConfigurationModeActive() const { if (_configurationMode) { _configurationMode->exitPBConfig(); } }

        bool isMeleeWeaponDrawn() const { return _weaponPosition && _weaponPosition->isMeleeWeaponDrawn(); }

        bool inWeaponRepositionMode() const { return _weaponPosition && _weaponPosition->inWeaponRepositionMode(); }
        void toggleWeaponRepositionMode() const { if (_weaponPosition) { _weaponPosition->toggleWeaponRepositionMode(); } }

        void dispatchMessageToBetterScopesVR(std::uint32_t messageType, void* data, std::uint32_t dataLen) const;

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
        static void removeEmbeddedFlashlight();
        static void onBetterScopesMessage(F4SE::MessagingInterface::Message* msg);

        bool _inPowerArmor = false;
        bool _isLookingThroughScope = false;
        float _dynamicCameraHeight = 0;
        bool _selfieMode = false;

        // the currently root node used in skeleton
        RE::NiNode* _workingRootNode = nullptr;

        Skeleton* _skelly = nullptr;
        Pipboy* _pipboy = nullptr;
        MainConfigMode _mainConfigMode;
        ConfigurationMode* _configurationMode = nullptr;
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
