#pragma once

namespace frik
{
    enum class PipboyOperation : std::uint8_t
    {
        GOTO_PREV_PAGE = 0,
        GOTO_NEXT_PAGE,
        GOTO_PREV_TAB,
        GOTO_NEXT_TAB,
        MOVE_LIST_SELECTION_UP,
        MOVE_LIST_SELECTION_DOWN,
        PRIMARY_PRESS
    };

    enum class PipboyPage : std::uint8_t
    {
        STATUS = 0,
        INVENTORY,
        DATA,
        MAP,
        RADIO,
    };

    class PipboyOperationHandler
    {
    public:
        static RE::Scaleform::GFx::Movie* getPipboyMenuRoot();
        static bool isMessageHolderVisible(const RE::Scaleform::GFx::Movie* root);
        static std::optional<PipboyPage> getCurrentPipboyPage(const RE::Scaleform::GFx::Movie* root);

        static void exec(PipboyOperation operation);
        static void operate();

    private:
        static void handlePrimaryControllerThumbstickOperation(RE::Scaleform::GFx::Movie* root);
        static void handlePrimaryControllerButtonsOperation(RE::Scaleform::GFx::Movie* root, bool triggerPressed);

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
    };
}
