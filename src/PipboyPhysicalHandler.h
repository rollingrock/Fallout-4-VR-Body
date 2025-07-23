#pragma once

#include "PipboyOperationHandler.h"

namespace frik
{
    class Skeleton;
    class Pipboy;

    /**
     * Handle Pipboy virtual physical hand interaction.
     * i.e. using offhand index finger to turn on/off Pipboy, light, radio, and other controls.
     */
    class PipboyPhysicalHandler
    {
    public:
        explicit PipboyPhysicalHandler(Skeleton* skelly, Pipboy* pipboy) :
            _skelly(skelly), _pipboy(pipboy) {}

        bool isOperating() const { return _isOperatingPipboy; }

        void operate(PipboyPage lastPipboyPage, bool isPBMessageBoxVisible);

    private:
        void checkHandStateToOperatePipboy(RE::NiPoint3 fingerPos);
        void operatePowerButton(RE::NiPoint3 fingerPos);
        void operateLightButton(RE::NiPoint3 fingerPos);
        void operateRadioButton(RE::NiPoint3 fingerPos);
        void updatePipboyPhysicalElements(PipboyPage lastPipboyPage, bool isPBMessageBoxVisible);
        void pipboyManagement(RE::NiPoint3 fingerPos);
        void operatePipboyPhysicalElement(RE::NiPoint3 fingerPos, PipboyOperation operation);

        Skeleton* _skelly;
        Pipboy* _pipboy;

        bool _isOperatingPipboy = false;

        // Pipboy interaction with hand variables
        bool _stickyPower = false;
        bool _stickyLight = false;
        bool _stickyRadio = false;
        float _lastRadioFreq = 0.0;

        // the other 7 points of interaction with the Pipboy (not including power, light, and radio)
        bool _controlsSticky[7] = { false, false, false, false, false, false, false };
        inline static std::string BONES_NAMES[7] = {
            "TabChangeUp", "TabChangeDown", "PageChangeUp", "PageChangeDown", "ScrollItemsUp", "ScrollItemsDown", "SelectButton02"
        };
        inline static std::string TRANS_NAMES[7] = {
            "TabChangeUpTrans", "TabChangeDownTrans", "PageChangeUpTrans", "PageChangeDownTrans", "ScrollItemsUpTrans", "ScrollItemsDownTrans", "SelectButtonTrans"
        };
        inline static std::string ORBS_NAMES[7] = {
            "TabChangeUpOrb", "TabChangeDownOrb", "PageChangeUpOrb", "PageChangeDownOrb", "ScrollItemsUpOrb", "ScrollItemsDownOrb", "SelectItemsOrb"
        };
        inline static float BONES_DISTANCES[7] = { 2.0f, 2.0f, 2.0f, 2.0f, 1.5f, 1.5f, 2.0f };
        inline static float TRANS_DISTANCES[7] = { 0.6f, 0.6f, 0.6f, 0.6f, 0.1f, 0.1f, 0.4f };
        inline static float MAX_DISTANCES[7] = { 1.2f, 1.2f, 1.2f, 1.2f, 1.2f, 1.2f, 0.6f };
    };
}
