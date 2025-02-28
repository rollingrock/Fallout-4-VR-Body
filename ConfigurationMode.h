#pragma once

#include "Skeleton.h"

namespace F4VRBody {

	/// <summary>
	/// Handle the in-game configuration matrix UI.
	/// Triggered by pressing down both sticks.
	/// Allows configuration of:
	/// - Player height
	/// - Dampen hands
	/// - Etc.
	/// </summary>
	class ConfigurationMode
	{
	public:
		ConfigurationMode(Skeleton* iSkelly, OpenVRHookManagerAPI* hook) {
			skelly = iSkelly;
			vrhook = hook;
		}

		inline bool isCalibrateModeActive() const {
			return c_CalibrateModeActive; 
		}

		inline bool isPipBoyConfigModeActive() const {
			return _isPBConfigModeActive;
		}

		void onUpdate();
		void exitPBConfig();

	private:
		void configModeExit();
		void pipboyConfigurationMode();
		void mainConfigurationMode();

		Skeleton* skelly;
		OpenVRHookManagerAPI* vrhook;

		// persistant fields to be used for general configuration menu
		bool _MCTouchbuttons[10] = { false, false, false, false, false, false, false, false, false, false };
		bool c_CalibrationModeUIActive = false;
		bool c_CalibrateModeActive = false;
		bool _isHandsButtonPressed = false;
		bool _isWeaponButtonPressed = false;
		bool _isGripButtonPressed = false;

		// persistant fields to be used for pipboy configuration menu
		bool _PBTouchbuttons[12] = { false, false, false, false, false, false, false, false, false, false, false, false };
		bool _isPBConfigModeActive = false;
		bool c_ExitandSavePressed = false;
		bool c_ExitnoSavePressed = false;
		bool c_SelfieButtonPressed = false;
		bool c_UIHeightButtonPressed = false;
		bool c_DampenHandsButtonPressed = false;
		int c_ConfigModeTimer = 0;
		int c_ConfigModeTimer2 = 0;
		bool _isSaveButtonPressed = false;
		bool _isModelSwapButtonPressed = false;
		bool _isGlanceButtonPressed = false;
		bool _isDampenScreenButtonPressed = false;
		int _PBConfigModeEnterCounter = 0;

		// backup and resture if erxist config without saving
		float c_armLengthbkup;
		float c_powerArmor_upbkup;
		float c_playerOffset_upbkup;
		float c_RootOffsetbkup;
		float c_PARootOffsetbkup;
		float c_fVrScalebkup;
		float c_playerOffset_forwardbkup;
		float c_powerArmor_forwardbkup;
		float c_cameraHeightbkup;
		float c_PACameraHeightbkup;
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	extern ConfigurationMode* g_configurationMode;

	static void initConfigurationMode(Skeleton* skelly, OpenVRHookManagerAPI* hook) {
		if (g_configurationMode) {
			_MESSAGE("ERROR: config already initialized");
			return;
		}

		_MESSAGE("Init config...");
		g_configurationMode = new ConfigurationMode(skelly, hook);
	}
}