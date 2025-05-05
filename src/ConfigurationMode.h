#pragma once

#include "Skeleton.h"

namespace FRIK {
	/// <summary>
	/// Handle the in-game configuration matrix UI.
	/// Triggered by pressing down both sticks.
	/// Allows configuration of:
	/// - Player height
	/// - Dampen hands
	/// - Etc.
	/// </summary>
	class ConfigurationMode {
	public:
		ConfigurationMode(Skeleton* skelly, OpenVRHookManagerAPI* hook) {
			_skelly = skelly;
			_vrhook = hook;
		}

		bool isCalibrateModeActive() const {
			return _calibrateModeActive;
		}

		void enterConfigurationMode() {
			_calibrateModeActive = true;
		}

		bool isPipBoyConfigModeActive() const {
			return _isPBConfigModeActive;
		}

		void onUpdate();
		void exitPBConfig();
		void openPipboyConfigurationMode();

	private:
		void configModeExit();
		void pipboyConfigurationMode();
		void mainConfigurationMode();
		void enterPipboyConfigMode();
		static void checkWeaponRepositionPipboyConflict();

		Skeleton* _skelly;
		OpenVRHookManagerAPI* _vrhook;

		// height calibration
		time_t _calibratePlayerHeightTime = 0;

		// persistant fields to be used for general configuration menu
		bool _MCTouchbuttons[10] = {false, false, false, false, false, false, false, false, false, false};
		bool _calibrationModeUIActive = false;
		bool _calibrateModeActive = false;
		bool _isHandsButtonPressed = false;
		bool _isWeaponButtonPressed = false;
		bool _isGripButtonPressed = false;

		// persistant fields to be used for pipboy configuration menu
		bool _PBTouchbuttons[12] = {false, false, false, false, false, false, false, false, false, false, false, false};
		bool _isPBConfigModeActive = false;
		bool _exitAndSavePressed = false;
		bool _exitWithoutSavePressed = false;
		bool _selfieButtonPressed = false;
		bool _UIHeightButtonPressed = false;
		bool _dampenHandsButtonPressed = false;
		int _configModeTimer = 0;
		int _configModeTimer2 = 0;
		bool _isSaveButtonPressed = false;
		bool _isModelSwapButtonPressed = false;
		bool _isGlanceButtonPressed = false;
		bool _isDampenScreenButtonPressed = false;
		int _PBConfigModeEnterCounter = 0;

		// backup and resture if erxist config without saving
		float _armLength_bkup = 0;
		float _powerArmor_up_bkup = 0;
		float _playerOffset_up_bkup = 0;
		float _rootOffset_bkup = 0;
		float _PARootOffset_bkup = 0;
		float _fVrScale_bkup = 0;
		float _playerOffset_forward_bkup = 0;
		float _powerArmor_forward_bkup = 0;
		float _cameraHeight_bkup = 0;
		float _PACameraHeight_bkup = 0;
		bool enableGripButtonToGrap_bkup = false;
		bool onePressGripButton_bkup = false;
		bool enableGripButtonToLetGo_bkup = false;
	};
}
