#pragma once

#include "skeleton/Skeleton.h"
#include "vrui/UIButton.h"
#include "vrui/UIContainer.h"
#include "vrui/UIToggleButton.h"
#include "vrui/UIToggleGroupContainer.h"
#include "vrui/UIWidget.h"

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
     * Entered by long-pressing the configured binding while the Pipboy is on the wrist and open, or
     * from the main config menu. Built on the vrui widget system; the panel is attached above the
     * primary wand while active and torn down on exit.
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
        RE::NiAVObject* getPipboyModelNode() const;

        Skeleton* _skelly;

        // what the thumbstick is currently adjusting
        PipboyAdjustTarget _adjustTarget = PipboyAdjustTarget::None;

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;
        std::shared_ptr<vrui::UIToggleGroupContainer> _adjustModeGroup;

        // elements hidden when the Pipboy isn't on the wrist (only model scale stays relevant then)
        std::shared_ptr<vrui::UIToggleButton> _adjustScreenBtn;
        std::shared_ptr<vrui::UIButton> _swapModelBtn;
        std::shared_ptr<vrui::UIContainer> _row2Container;
        std::shared_ptr<vrui::UIWidget> _notOnWristMsg;

        // save/reset buttons - only visible while actively adjusting a target (exit button is always visible)
        std::shared_ptr<vrui::UIButton> _saveBtn;
        std::shared_ptr<vrui::UIButton> _resetBtn;

        // footers - only one visible at a time depending on the current adjust target
        std::shared_ptr<vrui::UIWidget> _footerMain;
        std::shared_ptr<vrui::UIWidget> _footerScreenAdjust;
        std::shared_ptr<vrui::UIWidget> _footerModelScale;
    };
}
