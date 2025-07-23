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

        static void execOperation(PipboyOperation operation);

    private:
        void replaceMeshes(const std::string& itemHide, const std::string& itemShow);

        void checkTurningOnByButton();
        void checkTurningOffByButton();
        void checkTurningOnByLookingAt();
        void checkTurningOffByLookingAway();
        void checkSwitchingFlashlightHeadHand();

        static void gotoPrevPage(RE::Scaleform::GFx::Movie* root);
        static void gotoNextPage(RE::Scaleform::GFx::Movie* root);
        static void gotoPrevTab(RE::Scaleform::GFx::Movie* root);
        static void gotoNextTab(RE::Scaleform::GFx::Movie* root);
        static void moveListSelectionUpDown(RE::Scaleform::GFx::Movie* root, bool moveUp);
        static void handlePrimaryControllerOperationOnStatusPage(RE::Scaleform::GFx::Movie* root, bool triggerPressed);
        static void handlePrimaryControllerOperationOnInventoryPage(RE::Scaleform::GFx::Movie* root, bool triggerPressed);
        static void handlePrimaryControllerOperationOnDataPage(RE::Scaleform::GFx::Movie* root, bool triggerPressed);
        static void handlePrimaryControllerOperationOnMapPage(RE::Scaleform::GFx::Movie* root, bool triggerPressed);
        static void handlePrimaryControllerOperationOnRadioPage(RE::Scaleform::GFx::Movie* root, bool triggerPressed);
        void storeLastPipboyPage();
        void handlePrimaryControllerThumbstickOperation(RE::Scaleform::GFx::Movie* root);
        static void handlePrimaryControllerButtonsOperation(RE::Scaleform::GFx::Movie* root, bool triggerPressed);

        void dampenPipboyScreen();
        void rightStickXSleep(int time);
        void rightStickYSleep(int time);
        void secondaryTriggerSleep(int time);

        Skeleton* _skelly;
        PipboyPhysicalHandler _physicalHandler;

        bool _isOnStatus = false;
        bool _meshesReplaced = false;
        uint64_t _startedLookingAtPip = 0;
        uint64_t _lastLookingAtPip = 0;
        RE::NiTransform _pipboyScreenPrevFrame;

        // Pipboy interaction "sticky" flags
        bool _stickyTurnOn = false;
        bool _stickyTurnOff = false;
        bool _stickySwithLight = false;

        PipboyPage _lastPipboyPage = PipboyPage::STATUS;
        float lastRadioFreq = 0.0;

        bool _controlSleepStickyX = false;
        bool _controlSleepStickyY = false;
        bool _controlSleepStickyT = false;
    };
}
