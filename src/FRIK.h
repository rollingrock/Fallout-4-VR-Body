#pragma once

#include "BoneSpheresHandler.h"
#include "ConfigurationMode.h"
#include "Pipboy.h"
#include "PlayerControlsHandler.h"
#include "SmoothMovementVR.h"
#include "WeaponPositionAdjuster.h"
#include "f4se/PapyrusEvents.h"
#include "f4vr/GameMenusHandler.h"

namespace frik {
	constexpr auto BETTER_SCOPES_VR_MOD_NAME = "FO4VRBETTERSCOPES";

	class FRIK {
	public:
		bool isInScopeMenu() { return _gameMenusHandler.isInScopeMenu(); }

		bool getSelfieMode() const { return _selfieMode; }
		void setSelfieMode(const bool selfieMode) { _selfieMode = selfieMode; }
		float getDynamicCameraHeight() const { return _dynamicCameraHeight; }
		void setDynamicCameraHeight(const float dynamicCameraHeight) { _dynamicCameraHeight = dynamicCameraHeight; }
		bool isLookingThroughScope() const { return _isLookingThroughScope; }
		void setLookingThroughScope(const bool isLookingThroughScope) { _isLookingThroughScope = isLookingThroughScope; }

		bool isPipboyOn() const { return _pipboy && _pipboy->status(); }
		bool isOperatingPipboy() const { return _pipboy && _pipboy->isOperatingPipboy(); }
		void turnOnPipboy() const { if (_pipboy) { _pipboy->turnOn(); } }
		void replacePipboyMeshes(const bool force) const { if (_pipboy) { _pipboy->replaceMeshes(force); } }

		bool isMainConfigurationModeActive() const { return _configurationMode && _configurationMode->isCalibrateModeActive(); }
		bool isPipboyConfigurationModeActive() const { return _configurationMode && _configurationMode->isPipBoyConfigModeActive(); }
		void openMainConfigurationModeActive() const { if (_configurationMode) { _configurationMode->enterConfigurationMode(); } }
		void openPipboyConfigurationModeActive() const { if (_configurationMode) { _configurationMode->openPipboyConfigurationMode(); } }
		void closePipboyConfigurationModeActive() const { if (_configurationMode) { _configurationMode->exitPBConfig(); } }

		bool inWeaponRepositionMode() const { return _weaponPosition && _weaponPosition->inWeaponRepositionMode(); }
		void toggleWeaponRepositionMode() const { if (_weaponPosition) { _weaponPosition->toggleWeaponRepositionMode(); } }

		void dispatchMessageToBetterScopesVR(UInt32 messageType, void* data, UInt32 dataLen) const;

		void initialize(const F4SEInterface* f4se);
		void onFrameUpdate();
		void smoothMovement();

	private:
		void initOnGameLoaded();
		void initOnGameSessionLoaded();
		void initSkeleton();
		void releaseSkeleton();
		static void updateWorldFinal();
		static void configureGameVars();
		static bool isGameReadyForSkeletonInitialization();
		bool isRootNodeValid() const;
		static void onF4VRSEMessage(F4SEMessagingInterface::Message* msg);
		static void onBetterScopesMessage(F4SEMessagingInterface::Message* msg);
		void checkDebugDump();

		bool _inPowerArmor = false;
		bool _isLookingThroughScope = false;
		float _dynamicCameraHeight = 0;
		bool _selfieMode = false;

		// the currently root node used in skeleton
		BSFadeNode* _workingRootNode;

		Skeleton* _skelly = nullptr;
		Pipboy* _pipboy = nullptr;
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

		PluginHandle _pluginHandle = kPluginHandle_Invalid;
		F4SEMessagingInterface* _messaging = nullptr;
	};

	// The ONE global to rule them ALL
	inline FRIK g_frik;
}
