#pragma once

#include "PipboyPhysicalHandler.h"

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
        explicit Pipboy(Skeleton* skelly);

        bool isOpen() const { return _isOpen; }
        void openClose(bool open);
        bool isOperatingPipboy() const { return _physicalHandler.isOperating(); }
        bool isPlayerLookingAt() const;

        void swapModel();
        void onFrameUpdate();

    private:
        void replaceMeshes(bool force);
        void replaceMeshes(const std::string& itemHide, const std::string& itemShow);

        void checkTurningOnByButton();
        void checkTurningOffByButton();
        void checkTurningOnByLookingAt();
        void checkTurningOffByLookingAway();
        void checkSwitchingFlashlightHeadHand();
        void adjustFlashlightTransformToHandOrHead() const;

        void storeLastPipboyPage();
        void dampenPipboyScreen();

        Skeleton* _skelly;

        PipboyPhysicalHandler _physicalHandler;

        bool _isOpen = false;

        bool _meshesReplaced = false;

        // handle auto open/close
        uint64_t _startedLookingAtPip = 0;
        uint64_t _lastLookingAtPip = 0;

        // cooldown to stop flashlight haptic feedback after switch
        uint64_t _flashlightHapticCooldown = 0;

        // handle dampening of pipboy screen to reduce movement
        RE::NiTransform _pipboyScreenPrevFrame;

        PipboyPage _lastPipboyPage = PipboyPage::STATUS;
        float lastRadioFreq = 0.0;
    };
}
