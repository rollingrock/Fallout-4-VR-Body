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

        void swapModel() const;

        void onFrameUpdate();

    private:
        void hideShowPipboyOnArm() const;
        static void restoreDefaultPipboyModelIfNeeded();
        void positionPipboyModelToPipboyOnArm() const;
        void replaceMeshes(bool force) const;
        bool replaceMeshes(const std::string& itemHide, const std::string& itemShow) const;

        void checkTurningOnByButton();
        void checkTurningOffByButton();
        void checkTurningOnByLookingAt();
        void checkTurningOffByLookingAway();
        void checkSwitchingFlashlightHeadHand();
        void adjustFlashlightTransformToHandOrHead() const;

        static void storeLastPipboyPage();
        void dampenPipboyScreen();
        void dampenPipboyRegularScreen(RE::NiNode* pipboyScreen);
        void dampenPipboyHoloScreen(RE::NiNode* pipboyScreen);
        bool isPlayerLookingAt() const;
        void leftHandedModePipboy() const;
        RE::NiAVObject* getPipboyNode() const;

        Skeleton* _skelly;

        PipboyPhysicalHandler _physicalHandler;

        bool _isOpen = false;

        // handle auto open/close
        uint64_t _startedLookingAtPip = 0;
        uint64_t _lastLookingAtPip = 0;

        // cooldown to stop flashlight haptic feedback after switch
        uint64_t _flashlightHapticCooldown = 0;

        // handle dampening of pipboy screen to reduce movement
        std::deque<RE::NiPoint3> _pipboyScreenPrevFrame;
        std::optional<RE::NiPoint3> _pipboyScreenStableFrame;
        int _dampenScreenAdjustCounter = 0;

        // static field to preserve the last pipboy page when existing PA
        inline static PipboyPage _lastPipboyPage = PipboyPage::STATUS;

        // used to prevent constant mesh replacing to the same meshes
        inline static bool _lastMeshReplaceIsHoloPipboy = false;

        // used to restore the original Pipboy if settings change from on-wrist Pipboy to other
        inline static RE::NiNode* _originalPipboyRootNifOnlyNode = nullptr;
    };
}
