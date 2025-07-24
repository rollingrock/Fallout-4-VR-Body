#pragma once

#include "PipboyPhysicalHandler.h"
#include "utils.h"

namespace frik
{
    class Skeleton;

    /**
     * Handle Pipboy:
     * 1. On wrist UI override.
     * 2. Hand interaction with on-wrist Pipboy.
     * 3. Flashlight on-wrist / head location toggle.
     */
    class Pipboy
    {
    public:
        explicit Pipboy(Skeleton* skelly):
            _skelly(skelly), _physicalHandler(skelly, this)
        {
            // TODO: do we need this?
            turnPipBoyOff();
        }

        bool isOn() const { return _isOnStatus; }
        void setOnOff(bool setOn);
        bool isOperatingPipboy() const { return _physicalHandler.isOperating(); }

        bool isLookingAtPipBoy() const;
        void replaceMeshes(bool force);
        void onFrameUpdate();

    private:
        void replaceMeshes(const std::string& itemHide, const std::string& itemShow);

        void checkTurningOnByButton();
        void checkTurningOffByButton();
        void checkTurningOnByLookingAt();
        void checkTurningOffByLookingAway();
        void checkSwitchingFlashlightHeadHand();

        void storeLastPipboyPage();
        void dampenPipboyScreen();

        Skeleton* _skelly;

        PipboyOperationHandler _operationHandler;
        PipboyPhysicalHandler _physicalHandler;

        bool _isOnStatus = false;
        bool _meshesReplaced = false;
        uint64_t _startedLookingAtPip = 0;
        uint64_t _lastLookingAtPip = 0;
        RE::NiTransform _pipboyScreenPrevFrame;

        // Pipboy interaction "sticky" flags
        bool _stickySwithLight = false;

        PipboyPage _lastPipboyPage = PipboyPage::STATUS;
        float lastRadioFreq = 0.0;
    };
}
