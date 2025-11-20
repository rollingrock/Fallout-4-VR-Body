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

        bool isPipBoyConfigModeActive() const
        {
            return _isPBConfigModeActive;
        }

        void onFrameUpdate();
        void exitPBConfig();
        void openPipboyConfigurationMode();

    private:
        void pipboyConfigurationMode();
        void enterPipboyConfigMode();

        Skeleton* _skelly;

        // persistent fields to be used for pipboy configuration menu
        bool _PBTouchbuttons[12] = { false, false, false, false, false, false, false, false, false, false, false, false };
        bool _isPBConfigModeActive = false;
        bool _isSaveButtonPressed = false;
        bool _isModelSwapButtonPressed = false;
        bool _isGlanceButtonPressed = false;
        bool _isDampenScreenButtonPressed = false;
        int _PBConfigModeEnterCounter = 0;
    };
}
