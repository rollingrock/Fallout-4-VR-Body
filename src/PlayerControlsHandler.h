#pragma once

#include "config-mode/MainConfigMode.h"
#include "f4vr/F4VRThumbstickControls.h"
#include "pipboy/Pipboy.h"
#include "weapon-position/WeaponPositionAdjuster.h"

namespace frik
{
    class PlayerControlsHandler
    {
    public:
        PlayerControlsHandler();
        void onFrameUpdate(const MainConfigMode& mainConfigMode, const Pipboy* pipboy, const WeaponPositionAdjuster* weaponPosition);

    private:
        void enableControls();
        void disableControls();

        bool _disabledInput = false;
        REX::EnumSet<RE::UserEvents::USER_EVENT_FLAG, std::uint32_t> _disableFlags;
        REX::EnumSet<RE::UserEvents::USER_EVENT_FLAG, std::uint32_t> _lastFlags;
    };

    inline PlayerControlsHandler::PlayerControlsHandler()
    {
        // most disabled, leave some enabled that either needed (fast travel) or reported with bugs staying disabled
        _disableFlags.set(
            static_cast<RE::UserEvents::USER_EVENT_FLAG>(
                RE::OtherInputEvents::OTHER_EVENT_FLAG::kFastTravel
                | RE::OtherInputEvents::OTHER_EVENT_FLAG::kRunning
                | RE::OtherInputEvents::OTHER_EVENT_FLAG::kSprinting));
    }

    /**
     * It's just easier to have single place that controls if player controls are enabled or not.
     * The enable/disable functions have to be idempotent to avoid issues and simplify logic on when we enable/disable player controls.
     * Special handling for throwable weapon reposition mode to allow player to "cook" the throwable for repositioning. It's sucky but it's only
     * for this limited scenario so it's not a big deal.
     */
    inline void PlayerControlsHandler::onFrameUpdate(const MainConfigMode& mainConfigMode, const Pipboy* pipboy, const WeaponPositionAdjuster* weaponPosition)
    {
        if (pipboy->isOpen() || mainConfigMode.isBodyAdjustOpen() || weaponPosition->inWeaponRepositionMode() || pipboy->isOperatingWithFinger()) {
            if (weaponPosition->inThrowableWeaponRepositionMode()) {
                enableControls();
                f4vr::F4VRThumbstickControls::setControlsThumbstickEnableState(false);
                return;
            }

            disableControls();
            f4vr::F4VRThumbstickControls::setControlsThumbstickEnableState(true);

            // hide the weapon so the player can interact easily with the Pipboy (not relevant for left-handed)
            // Note: important this codes runs before WeaponPositionAdjuster to have it not change hand transform
            if (!f4vr::isLeftHandedMode() && pipboy->isOperatingWithFinger() && weaponPosition->isWeaponDrawn()) {
                f4vr::setNodeVisibility(f4vr::getWeaponNode(), false);
            }
            return;
        }

        enableControls();
        f4vr::F4VRThumbstickControls::setControlsThumbstickEnableState(true);
    }

    /**
     * Enable all player controls.
     */
    inline void PlayerControlsHandler::enableControls()
    {
        if (_disabledInput) {
            return;
        }
        _disabledInput = true;

        logger::info("Player controls - Enabled");
        f4vr::SetActorRestrained(RE::PlayerCharacter::GetSingleton(), false);

        // restore the force flags. clearing is incorrect as it seems the default is to have all of them force enabled
        // restoring original instead of forcing all enabled in case other mod set some restrictions
        RE::BSInputEnableManager::GetSingleton()->forceEnableInputUserEventsFlags = _lastFlags;
    }

    /**
     * Disable all player controls. no movement, no rotation, no weapon use, no VATS, no favorite.
     */
    inline void PlayerControlsHandler::disableControls()
    {
        if (!_disabledInput) {
            // the force state can change in some rare scenarios, redo our overrides
            if (RE::BSInputEnableManager::GetSingleton()->forceEnableInputUserEventsFlags != _disableFlags) {
                logger::debug("Player controls - reapply disabled flags");
                _lastFlags = RE::BSInputEnableManager::GetSingleton()->forceEnableInputUserEventsFlags;
                RE::BSInputEnableManager::GetSingleton()->forceEnableInputUserEventsFlags = _disableFlags;
            }
            return;
        }
        _disabledInput = false;

        logger::info("Player controls - Disabled");

        // restrict movement, rotation, and weapon use
        f4vr::SetActorRestrained(RE::PlayerCharacter::GetSingleton(), true);

        // store the current force enable flags to restore them when enabling
        _lastFlags = RE::BSInputEnableManager::GetSingleton()->forceEnableInputUserEventsFlags;

        // disable everything except fast travel
        RE::BSInputEnableManager::GetSingleton()->forceEnableInputUserEventsFlags = _disableFlags;
    }
}
