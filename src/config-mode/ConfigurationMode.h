#pragma once

#include "skeleton/Skeleton.h"

namespace frik
{
    /**
     * Handle the in-game configuration matrix UI.
     * Triggered by pressing down both sticks.
     * Allows configuration of:
     * - Player height
     * - Dampen hands
     * - Etc.
     */
    class ConfigurationMode
    {
    public:
        explicit ConfigurationMode(Skeleton* skelly)
        {
            _skelly = skelly;
        }

        bool isCalibrateModeActive() const
        {
            return _calibrateModeActive;
        }

        void enterConfigurationMode()
        {
            _calibrateModeActive = true;
        }

        bool isPipBoyConfigModeActive() const
        {
            return _isPBConfigModeActive;
        }

        void onFrameUpdate();
        void exitPBConfig();
        void openPipboyConfigurationMode();

    private:
        void configModeExit();
        void pipboyConfigurationMode();
        void mainConfigurationMode();
        void enterPipboyConfigMode();

        Skeleton* _skelly;

        // height calibration
        time_t _calibratePlayerHeightTime = 0;

        // persistant fields to be used for general configuration menu
        bool _MCTouchbuttons[10] = { false, false, false, false, false, false, false, false, false, false };
        bool _calibrationModeUIActive = false;
        bool _calibrateModeActive = false;
        bool _isHandsButtonPressed = false;
        bool _isWeaponButtonPressed = false;
        bool _isGripButtonPressed = false;

        // persistant fields to be used for pipboy configuration menu
        bool _PBTouchbuttons[12] = { false, false, false, false, false, false, false, false, false, false, false, false };
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

        // backup and restore if exiting config without saving
        float _armLength_bkup = 0;
        float _fVrScale_bkup = 0;
        float _playerBodyOffsetUpStanding_bkup = 0;
        float _playerBodyOffsetForwardStanding_bkup = 0;
        float _playerBodyOffsetUpSitting_bkup = 0;
        float _playerBodyOffsetForwardSitting_bkup = 0;
        float _playerHMDOffsetUpSitting_bkup = 0;
        float _playerHMDOffsetUpStanding_bkup = 0;
        float _playerBodyOffsetUpStandingInPA_bkup = 0;
        float _playerBodyOffsetForwardStandingInPA_bkup = 0;
        float _playerHMDOffsetUpStandingInPA_bkup = 0;
        float _playerBodyOffsetUpSittingInPA_bkup = 0;
        float _playerBodyOffsetForwardSittingInPA_bkup = 0;
        float _playerHMDOffsetUpSittingInPA_bkup = 0;
        bool enableGripButtonToGrap_bkup = false;
        bool onePressGripButton_bkup = false;
        bool enableGripButtonToLetGo_bkup = false;
    };
}
