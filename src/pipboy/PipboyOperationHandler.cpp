#include "PipboyOperationHandler.h"

#include "f4vr/VRControllersManager.h"

#include "Config.h"
#include "FRIK.h"
#include "utils.h"
#include "common/CommonUtils.h"
#include "f4vr/scaleformUtils.h"

using namespace RE::Scaleform;
using namespace std::chrono;
using namespace common;

namespace
{
    bool isPrimaryTriggerPressed()
    {
        return f4vr::VRControllers.isPressed(f4vr::Hand::Primary, vr::k_EButton_SteamVR_Trigger);
    }

    bool isAButtonPressed()
    {
        return f4vr::VRControllers.isPressed(f4vr::Hand::Primary, vr::k_EButton_A);
    }

    bool isBButtonPressed()
    {
        return f4vr::VRControllers.isPressed(f4vr::Hand::Primary, vr::k_EButton_ApplicationMenu);
    }

    bool isPrimaryGripPressHeldDown()
    {
        return f4vr::VRControllers.isPressHeldDown(f4vr::Hand::Primary, vr::k_EButton_Grip);
    }

    bool isPrimaryThumbstickPressed()
    {
        return f4vr::VRControllers.isPressed(f4vr::Hand::Primary, vr::k_EButton_Axis0);
    }

    bool isWorldMapVisible(const GFx::Movie* root)
    {
        return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.WorldMapHolder_mc");
    }

    std::string getCurrentMapPath(const GFx::Movie* root, const std::string& suffix = "")
    {
        return (isWorldMapVisible(root) ? "root.Menu_mc.CurrentPage.WorldMapHolder_mc" : "root.Menu_mc.CurrentPage.LocalMapHolder_mc") + suffix;
    }

    bool isQuestTabVisibleOnDataPage(const GFx::Movie* root)
    {
        return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.QuestsTab_mc");
    }

    bool isQuestTabObjectiveListEnabledOnDataPage(const GFx::Movie* root)
    {
        GFx::Value var;
        return root->GetVariable(&var, "root.Menu_mc.CurrentPage.QuestsTab_mc.ObjectivesList_mc.selectedIndex")
            && var.GetType() == GFx::Value::ValueType::kInt
            && var.GetInt() > -1;
    }

    bool isWorkshopsTabVisibleOnDataPage(const GFx::Movie* root)
    {
        return f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc");
    }
}

namespace frik
{
    GFx::Movie* PipboyOperationHandler::getPipboyMenuRoot()
    {
        const auto menu = RE::UI::GetSingleton()->GetMenu("PipboyMenu");
        return menu ? menu->uiMovie.get() : nullptr;
    }

    /**
     * Is context menu message box popup is visible or not. Only one can be visible at a time.
     */
    bool PipboyOperationHandler::isMessageHolderVisible(const GFx::Movie* root)
    {
        return root &&
        (f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.MessageHolder_mc")
            || f4vr::isElementVisible(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc"));
    }

    /**
     * Get the current open Pipboy page or null-opt if not open.
     */
    std::optional<PipboyPage> PipboyOperationHandler::getCurrentPipboyPage(const GFx::Movie* root)
    {
        if (!root) {
            return std::nullopt;
        }

        GFx::Value PBCurrentPage;
        const bool getVariableSuccessful = root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj.CurrentPage");
        if (getVariableSuccessful && PBCurrentPage.GetType() != GFx::Value::ValueType::kUndefined) {
            return static_cast<PipboyPage>(PBCurrentPage.GetUInt());
        }
        logger::sample("Failed to get current Pipboy page! getVariableSuccessful?({}) Type?({})",
            getVariableSuccessful, getVariableSuccessful ? static_cast<int>(PBCurrentPage.GetType()) : -1);
        return std::nullopt;
    }

    /**
     * Execute the given operation on the current Pipboy page.
     */
    void PipboyOperationHandler::exec(const PipboyOperation operation)
    {
        const auto root = getPipboyMenuRoot();
        if (!root) {
            logger::warn<>("Pipboy operation failed, root is null");
            return;
        }

        switch (operation) {
        case PipboyOperation::GOTO_PREV_PAGE:
            gotoPrevPage(root);
            break;
        case PipboyOperation::GOTO_NEXT_PAGE:
            gotoNextPage(root);
            break;
        case PipboyOperation::GOTO_PREV_TAB:
            gotoPrevTab(root);
            break;
        case PipboyOperation::GOTO_NEXT_TAB:
            gotoNextTab(root);
            break;
        case PipboyOperation::MOVE_LIST_SELECTION_UP:
            moveListSelectionUpDown(root, true);
            break;
        case PipboyOperation::MOVE_LIST_SELECTION_DOWN:
            moveListSelectionUpDown(root, false);
            break;
        case PipboyOperation::PRIMARY_PRESS:
            handlePrimaryControllerButtonsOperation(root, true);
            break;
        }
    }

    /**
     * Operate Pipboy using primary hand controller.
     */
    void PipboyOperationHandler::operate()
    {
        if (!g_config.enablePrimaryControllerPipboyUse || g_frik.isPipboyConfigurationModeActive()) {
            return;
        }

        // Use primary hand controller to operate Pipboy
        if (const auto pipboyRoot = getPipboyMenuRoot()) {
            handlePrimaryControllerThumbstickOperation(pipboyRoot);
            handlePrimaryControllerButtonsOperation(pipboyRoot, false);
        }
    }

    /**
     * Execute operation on the currently active Pipboy UI by primary thumbstick input.
     * On map page the map is moved.
     * On other pages the list selection is moved up/down or tabs are changed left/right.
     */
    void PipboyOperationHandler::handlePrimaryControllerThumbstickOperation(GFx::Movie* const root)
    {
        if (isPrimaryGripPressHeldDown()) {
            return;
        }

        const auto doinantHandStick = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Primary);
        if (fEqual(doinantHandStick.x, 0, 0.1f) && fEqual(doinantHandStick.y, 0, 0.1f)) {
            return; // No movement, no operation
        }

        const auto currentPipboyPage = getCurrentPipboyPage(root);
        if (currentPipboyPage == PipboyPage::MAP && !isMessageHolderVisible(root)) {
            // Map Tab
            GFx::Value akArgs[2];
            akArgs[0] = doinantHandStick.x * -1;
            akArgs[1] = doinantHandStick.y;
            // Move Map
            root->Invoke("root.Menu_mc.CurrentPage.WorldMapHolder_mc.PanMap", nullptr, akArgs, 2);
            root->Invoke("root.Menu_mc.CurrentPage.LocalMapHolder_mc.PanMap", nullptr, akArgs, 2);
        } else {
            const auto direction = f4vr::VRControllers.getThumbstickPressedDirection(f4vr::Hand::Primary);
            if (direction.has_value()) {
                switch (direction.value()) {
                case f4vr::Direction::Right:
                    if (!isMessageHolderVisible(root)) {
                        gotoNextTab(root);
                    }
                    break;
                case f4vr::Direction::Left:
                    if (!isMessageHolderVisible(root)) {
                        gotoPrevTab(root);
                    }
                    break;
                case f4vr::Direction::Up:
                    moveListSelectionUpDown(root, true);
                    break;
                case f4vr::Direction::Down:
                    moveListSelectionUpDown(root, false);
                    break;
                default: ;
                }
            }
        }
    }

    /**
     * Execute operation on the currently active Pipboy UI by primary controller buttons.
     * First thing is to detect is a message box is visible (context menu options) as message box and main lists are active at the
     * same time. So, if the message box is visible we will only operate on the message box list and not the main list.
     * See documentation: https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Development-%E2%80%90-Pipboy-Controls
     * @param root - Pipboy Scaleform UI root
     * @param triggerPressed - true if operate as trigger is pressed regardless of controller input (used for physical handler)
     */
    void PipboyOperationHandler::handlePrimaryControllerButtonsOperation(GFx::Movie* root, bool triggerPressed)
    {
        // page level navigation
        const bool gripPressHeldDown = isPrimaryGripPressHeldDown();
        if (!gripPressHeldDown && isAButtonPressed()) {
            gotoPrevPage(root);
            return;
        }
        if (!gripPressHeldDown && isBButtonPressed()) {
            gotoNextPage(root);
            return;
        }

        // include actual controller check
        triggerPressed = triggerPressed || isPrimaryTriggerPressed();

        // Context menu message box handling
        if (triggerPressed && isMessageHolderVisible(root)) {
            triggerShortHaptic();
            f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.MessageHolder_mc", f4vr::ScaleformListOp::Select);
            f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc", f4vr::ScaleformListOp::Select);
            // prevent affecting the main list if message box is visible
            return;
        }

        // Specific handling by current page
        const auto pipboyPage = getCurrentPipboyPage(root);
        if (pipboyPage.has_value()) {
            switch (pipboyPage.value()) {
            case PipboyPage::STATUS:
                handlePrimaryControllerOperationOnStatusPage(root, triggerPressed);
                break;
            case PipboyPage::INVENTORY:
                handlePrimaryControllerOperationOnInventoryPage(root, triggerPressed);
                break;
            case PipboyPage::DATA:
                handlePrimaryControllerOperationOnDataPage(root, triggerPressed);
                break;
            case PipboyPage::MAP:
                handlePrimaryControllerOperationOnMapPage(root, triggerPressed);
                break;
            case PipboyPage::RADIO:
                handlePrimaryControllerOperationOnRadioPage(root, triggerPressed);
                break;
            default: ;
            }
        }

        if (f4vr::VRControllers.isPressed(f4vr::Hand::Primary, vr::EVRButtonId::k_EButton_Axis0)) {
            root->Invoke("root.Menu_mc.CurrentPage.onMessageButtonPress()", nullptr, nullptr, 0);
        }
    }

    void PipboyOperationHandler::gotoPrevPage(GFx::Movie* root)
    {
        triggerShortHaptic();
        root->Invoke("root.Menu_mc.gotoPrevPage", nullptr, nullptr, 0);
    }

    void PipboyOperationHandler::gotoNextPage(GFx::Movie* root)
    {
        triggerShortHaptic();
        root->Invoke("root.Menu_mc.gotoNextPage", nullptr, nullptr, 0);
    }

    void PipboyOperationHandler::gotoPrevTab(GFx::Movie* root)
    {
        triggerShortHaptic();
        root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0);
    }

    void PipboyOperationHandler::gotoNextTab(GFx::Movie* root)
    {
        triggerShortHaptic();
        root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0);
    }

    /**
     * Execute moving selection on the currently active list up or down.
     * Whatever the list found to exist it will be moved, mostly we can "try" to move any of the lists and only the one that exists will be moved.
     * On DATA page all 3 tabs can exist at the same time, so we need to check which one is visible to prevent working on a hidden one.
     * First thing is to detect is a message box is visible (context menu options) as message box and main lists are active at the
     * same time. So, if the message box is visible we will only operate on the message box list and not the main list.
     */
    void PipboyOperationHandler::moveListSelectionUpDown(GFx::Movie* root, const bool moveUp)
    {
        triggerShortHaptic();

        const auto listOp = moveUp ? f4vr::ScaleformListOp::MoveUp : f4vr::ScaleformListOp::MoveDown;

        if (isMessageHolderVisible(root)) {
            f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.MessageHolder_mc", listOp);
            f4vr::doOperationOnScaleformMessageHolderList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc", listOp);
            // prevent affecting the main list if message box is visible
            return;
        }

        // Inventory, Radio, Special, and Perks tabs
        f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", listOp);
        f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc", listOp);
        f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.PerksTab_mc.List_mc", listOp);

        // Quest, Workshop, and Stats tabs exist at the same time, need to check which one is visible
        if (isQuestTabVisibleOnDataPage(root)) {
            // Quests tab has 2 lists for the main quests and quest objectives
            const char* listPath = isQuestTabObjectiveListEnabledOnDataPage(root)
                ? "root.Menu_mc.CurrentPage.QuestsTab_mc.ObjectivesList_mc"
                : "root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc";
            f4vr::doOperationOnScaleformList(root, listPath, listOp);
        } else if (isWorkshopsTabVisibleOnDataPage(root)) {
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc.List_mc", listOp);
        } else {
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.StatsTab_mc.CategoryList_mc", listOp);
        }
    }

    void PipboyOperationHandler::handlePrimaryControllerOperationOnStatusPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (triggerPressed) {
            triggerShortHaptic();
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.SPECIALTab_mc.List_mc", f4vr::ScaleformListOp::Select);
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.PerksTab_mc.List_mc", f4vr::ScaleformListOp::Select);
        }
    }

    void PipboyOperationHandler::handlePrimaryControllerOperationOnInventoryPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (triggerPressed) {
            triggerShortHaptic();
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", f4vr::ScaleformListOp::Select);
        } else if (isPrimaryThumbstickPressed()) {
            GFx::Value currentTab;
            if (root->GetVariable(&currentTab, "root.Menu_mc.DataObj.CurrentTab")) {
                // open context submenu
                triggerShortHaptic();
                GFx::Value args[1];
                args[0] = currentTab.GetUInt();
                root->Invoke("root.Menu_mc.CurrentPage.CloseMessage", nullptr, nullptr, 0);
                root->Invoke("root.Menu_mc.CurrentPage.OnOpenSubmenu", nullptr, args, 1);
            }
        }
    }

    /**
     * On DATA page all 3 tabs can exist at the same time, so we need to check which one is visible to prevent working on a hidden one.
     */
    void PipboyOperationHandler::handlePrimaryControllerOperationOnDataPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (triggerPressed) {
            triggerShortHaptic();
            if (isQuestTabVisibleOnDataPage(root)) {
                f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc", f4vr::ScaleformListOp::Select);
            } else {
                // open quest in map
                f4vr::invokeScaleformProcessUserEvent(root, "root.Menu_mc.CurrentPage.WorkshopsTab_mc", "XButton");
            }
        } else if (isPrimaryThumbstickPressed()) {
            // open context submenu
            triggerShortHaptic();
            if (isQuestTabVisibleOnDataPage(root)) {
                root->Invoke("root.Menu_mc.CurrentPage.QuestsTab_mc.OnOpenSubmenu", nullptr, nullptr, 0);
            } else {
                root->Invoke("root.Menu_mc.CurrentPage.WorkshopsTab_mc.OnOpenSubmenu", nullptr, nullptr, 0);
            }
        }
    }

    /**
     * For handling map faster travel or marker setting we need to know if fast travel can be done using "bCanFastTravel"
     * and then let the Pipboy code handle the rest by sending the right event to the currently visible map (world/local).
     */
    void PipboyOperationHandler::handlePrimaryControllerOperationOnMapPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (isPrimaryGripPressHeldDown()) {
            if (triggerPressed) {
                // switch world/local maps
                triggerShortHaptic();
                f4vr::invokeScaleformProcessUserEvent(root, "root.Menu_mc.CurrentPage", "XButton");
            } else {
                // zoom map
                const auto [_, primAxisY] = f4vr::VRControllers.getThumbstickValue(f4vr::Hand::Primary);
                if (common::fNotEqual(primAxisY, 0, 0.5f)) {
                    GFx::Value args[1];
                    args[0] = primAxisY / 100.f;
                    root->Invoke(getCurrentMapPath(root, ".ZoomMap").c_str(), nullptr, args, 1);
                }
            }
        } else if (triggerPressed) {
            triggerShortHaptic();

            // handle fast travel, custom marker
            const char* eventName = f4vr::getScaleformBool(root, getCurrentMapPath(root, ".bCanFastTravel").c_str()) ? "MapHolder:activate_marker" : "MapHolder:set_custom_marker";
            f4vr::invokeScaleformDispatchEvent(root, getCurrentMapPath(root), eventName);
        } else if (isPrimaryThumbstickPressed()) {
            // open context submenu
            triggerShortHaptic();
            root->Invoke("root.Menu_mc.CurrentPage.OnOpenSubmenu", nullptr, nullptr, 0);
        }
    }

    void PipboyOperationHandler::handlePrimaryControllerOperationOnRadioPage(GFx::Movie* root, const bool triggerPressed)
    {
        if (triggerPressed) {
            triggerShortHaptic();
            f4vr::doOperationOnScaleformList(root, "root.Menu_mc.CurrentPage.List_mc", f4vr::ScaleformListOp::Select);
        }
    }
}
