#pragma once

#include "skeleton/Skeleton.h"
#include "vrui/UIButtonPanel.h"
#include "vrui/UIContainer.h"
#include "vrui/UITextPanel.h"
#include "vrui/UIToggleButtonPanel.h"
#include "vrui/UIToggleGroupContainer.h"

namespace frik
{
    /**
     * What the player thumbstick is currently adjusting in Pipboy configuration mode.
     * Screen move/rotate/scale are all handled by the single ScreenAdjust target via different
     * stick + button combinations (like the weapon reposition mode).
     */
    enum class PipboyAdjustTarget : uint8_t
    {
        None = 0,
        ScreenAdjust,
        ModelScale,
    };

    /**
     * In-VR Pipboy configuration menu.
     * Allows configuration of the holo-screen position/scale/rotation, the 3rd-person Pipboy model
     * scale, swapping the Pipboy model, "open on glance" and "dampen screen" toggles, and saving.
     * Opened from the main config menu, Papyrus, or Mods Config FO4 VR's Pip-Boy page, with the Pipboy
     * on the wrist opened for it. Built from vrui panels (text and icons drawn at runtime, no NIFs); the
     * UI is attached above the primary wand while active and torn down on exit.
     */
    class PipboyConfigMode
    {
    public:
        explicit PipboyConfigMode(Skeleton* skelly)
            : _skelly(skelly)
        {}

        ~PipboyConfigMode();

        bool isPipBoyConfigModeActive() const
        {
            return _configUI != nullptr;
        }

        /**
         * True while a target is selected and the thumbstick is actively adjusting the Pipboy
         * (as opposed to just having the config UI open). Used to block Pipboy menu input.
         */
        bool isAdjusting() const
        {
            return _adjustTarget != PipboyAdjustTarget::None;
        }

        void onFrameUpdate();
        void exitPBConfig();
        void openPipboyConfigurationMode();

    private:
        void enterPipboyConfigMode();
        void createConfigUI();
        void handleAdjustment() const;
        static void handleScreenAdjust();
        void handleModelScaleAdjust() const;
        bool saveConfig() const;
        void resetConfig() const;
        static void onOpenWhenLookAtToggled(bool on);
        static void onCloseWhenLookAwayToggled(bool on);
        static void onDampenScreenModeChanged();
        void appendFooterRows(std::vector<vrui::TextRow>& rows) const;
        RE::NiAVObject* getPipboyModelNode() const;

        Skeleton* _skelly;

        // what the thumbstick is currently adjusting
        PipboyAdjustTarget _adjustTarget = PipboyAdjustTarget::None;

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;
        std::shared_ptr<vrui::UIToggleGroupContainer> _adjustModeGroup;

        // elements hidden when the Pipboy isn't on the wrist (only model scale stays relevant then)
        std::shared_ptr<vrui::UIToggleButtonPanel> _adjustScreenBtn;
        std::shared_ptr<vrui::UIButtonPanel> _swapModelBtn;
        std::shared_ptr<vrui::UIContainer> _row2Container;
        std::shared_ptr<vrui::UITextPanel> _notOnWristMsg;

        // save/reset buttons - only enabled while actively adjusting a target (exit button is always enabled)
        std::shared_ptr<vrui::UIButtonPanel> _saveBtn;
        std::shared_ptr<vrui::UIButtonPanel> _resetBtn;
    };
}
