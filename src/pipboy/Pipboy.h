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

        static bool isPlayerLookingAtPipboy(bool isPipboyOpen);

        bool isOpen() const { return _isOpen; }
        bool isOperatingWithFinger() const { return _physicalHandler.isOperating(); }
        void openClose(bool open);

        void swapModel();

        void onFrameUpdate();

    private:
        void exitPowerArmorBugFixHack(bool set);
        void hideShowPipboyOnArm() const;
        static void restoreDefaultPipboyModelIfNeeded();
        void updateSetupPipboyNodes();
        void showHideCorrectPipboyMesh(const std::string& itemHide, const std::string& itemShow) const;
        void setupPipboyRootNif();
        static void detachReplacedPipboyRootNif();

        void checkTurningOnByButton();
        bool checkAttaboyActivation();
        void checkTurningOffByButton();
        void checkTurningOnByLookingAt();
        void checkTurningOffByLookingAway();
        void checkSwitchingFlashlightHeadHand();
        void adjustFlashlightTransformToHandOrHead() const;

        static void storeLastPipboyPage();
        void holdPipboyScreenInPlace(RE::NiAVObject* pipboyScreen);
        void dampenPipboyScreen();
        void dampenPipboyScreenMovement(RE::NiAVObject* pipboyScreen);
        bool isPlayerLookingAtPipboy() const;
        void leftHandedModePipboy() const;
        RE::NiNode* getPipboyModelOnArmNode() const;

        Skeleton* _skelly;

        PipboyPhysicalHandler _physicalHandler;

        bool _isOpen = false;
        bool _attaboyGrabHapticActivated = 0;

        // see exitPowerArmorBugFixHack method
        bool _exitPowerArmorFixFirstFrame = true;

        // handle auto open/close
        uint64_t _startedLookingAtPip = 0;
        uint64_t _lastLookingAtPip = 0;

        // cooldown to stop flashlight haptic feedback after switch
        uint64_t _flashlightHapticCooldown = 0;

        // handle dampening of pipboy screen to reduce movement
        std::deque<RE::NiPoint3> _pipboyScreenPrevFrame;
        RE::NiTransform _pipboyScreenStableFrame;

        // Fallout London VR Attaboy handling of grabbing from the belt
        RE::NiNode* _attaboyOnBeltNode;

        // static field to preserve the last pipboy page when existing PA
        inline static PipboyPage _lastPipboyPage = PipboyPage::STATUS;

        // used to restore the original Pipboy if settings change from on-wrist Pipboy to other
        inline static RE::NiNode* _originalPipboyRootNifOnlyNode = nullptr;
        inline static RE::NiNode* _newPipboyRootNifOnlyNode = nullptr;
    };
}
