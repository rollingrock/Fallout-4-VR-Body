#pragma once

#include "vrui/UIButtonPanel.h"
#include "vrui/UIContainer.h"
#include "vrui/UITextPanel.h"
#include "vrui/UIToggleButtonPanel.h"

namespace frik
{
    class WeaponPositionAdjuster;

    /**
     * What is currently being configured
     */
    enum class RepositionTarget : uint8_t
    {
        Weapon = 0,
        PrimaryHand,
        Offhand,
        Throwable,
        BackOfHandUI,
        BetterScopes,
    };

    /**
     * Configuration mode to adjust weapon offset, offhand offset, and BetterScopesVR
     */
    class WeaponPositionConfigMode
    {
    public:
        explicit WeaponPositionConfigMode(WeaponPositionAdjuster* adjuster)
            : _adjuster(adjuster)
        {
            createConfigUI();
        }

        ~WeaponPositionConfigMode();

        bool isInThrowableWeaponRepositionMode() const;

        static RE::NiTransform getMeleeWeaponDefaultAdjustment(const RE::NiTransform& originalTransform);
        static RE::NiTransform getThrowableWeaponDefaultAdjustment(const RE::NiTransform& originalTransform, bool inPA);
        static RE::NiTransform getBackOfHandUIDefaultAdjustment(const RE::NiTransform& originalTransform, bool inPA);

        void onFrameUpdate(RE::NiNode* weapon);

    private:
        void handleReposition(RE::NiNode* weapon, RE::NiNode* throwable) const;
        void handleWeaponReposition(RE::NiNode* weapon) const;
        void handlePrimaryHandReposition() const;
        void handleOffhandReposition() const;
        void handleThrowableReposition(RE::NiNode* throwable) const;
        void handleBackOfHandUIReposition() const;
        static void handleBetterScopesReposition();
        void resetConfig() const;
        void saveConfig() const;
        void resetWeaponConfig() const;
        void saveWeaponConfig() const;
        void resetPrimaryHandConfig() const;
        void savePrimaryHandConfig() const;
        void resetOffhandConfig() const;
        void saveOffhandConfig() const;
        void resetThrowableConfig() const;
        void saveThrowableConfig() const;
        void resetBackOfHandUIConfig() const;
        void saveBackOfHandUIConfig() const;
        static void resetBetterScopesConfig();
        static void saveBetterScopesConfig();
        void createConfigUI();
        void showHideUIElements(bool weaponEquipped, bool throwableEquipped) const;

        // access the weapon/offhand transform to change
        WeaponPositionAdjuster* _adjuster;

        // what is currently being repositioned
        RepositionTarget _repositionTarget = RepositionTarget::Weapon;

        // configuration UI
        std::shared_ptr<vrui::UIContainer> _configUI;
        std::shared_ptr<vrui::UITextPanel> _complexAdjustFooter;
        std::shared_ptr<vrui::UITextPanel> _throwableAdjustFooter;
        std::shared_ptr<vrui::UITextPanel> _simpleAdjustFooter;
        std::shared_ptr<vrui::UIToggleButtonPanel> _weaponModeButton;
        std::shared_ptr<vrui::UIToggleButtonPanel> _primaryHandModeButton;
        std::shared_ptr<vrui::UIToggleButtonPanel> _offhandModeButton;
        std::shared_ptr<vrui::UIToggleButtonPanel> _throwableUIButton;
        std::shared_ptr<vrui::UIButtonPanel> _emptyHandsMessageBox;
        std::shared_ptr<vrui::UIToggleButtonPanel> _betterScopesModeButton;
        std::shared_ptr<vrui::UIButtonPanel> _saveButton;
        std::shared_ptr<vrui::UIButtonPanel> _resetButton;
        std::shared_ptr<vrui::UIButtonPanel> _throwableNotEquippedMessageBox;
    };
}
