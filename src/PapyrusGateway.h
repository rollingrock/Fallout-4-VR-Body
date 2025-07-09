#pragma once
#include "f4vr/PapyrusGatewayBase.h"

namespace frik
{
    /**
     * FRIK Papyrus Gateway.
     */
    class PapyrusGateway : public f4vr::PapyrusGatewayBase
    {
    public:
        explicit PapyrusGateway() :
            PapyrusGatewayBase("FRIK:FRIK") {}

        static const PapyrusGateway* init()
        {
            if (_instance) {
                throw std::exception("Papyrus Gateway is already initialized, only single instance can be used!");
            }
            _instance = new PapyrusGateway();
            return instance();
        }

        static const PapyrusGateway* instance()
        {
            return dynamic_cast<PapyrusGateway*>(_instance);
        }

        /**
         * Enable the player ability to move, fire weapon, use vats, etc.
         * @param drawWeapon - if true will draw the equipped weapon (useful if disableWeapon was used calling DisablePlayerControls)
         */
        void enablePlayerControls(const bool drawWeapon) const
        {
            auto arguments = F4SEVR::getArgs(drawWeapon);
            executePapyrusScript("EnablePlayerControls", arguments);
        }

        /**
         * Disable the player ability to move, fire weapon, use vats, etc.
         *
         * Unfortunately Bethesda sucks, just disabling player controls doesn't prevent rotation and jumping.
         * Therefore, SetRestrained is used, but it also prevents any weapon use so we allow a flag to control it.
         * SetRestrained doesn't prevent the use of VATS and Favorites, so we still need DisablePlayerControls.
         * If restrain is false the code needs to implement a different way to prevent player rotation, jumping, and sneaking.
         * @param disableWeapon - if true will holster the weapon and prevent its use
         * @param restrain - will prevent player movement, rotation, jumping, sneaking, and weapon use (without holstering)
         */
        void disablePlayerControls(const bool disableWeapon, const bool restrain) const
        {
            auto arguments = F4SEVR::getArgs(disableWeapon, restrain);
            executePapyrusScript("DisablePlayerControls", arguments);
        }

        /**
         * Enable/disable using a weapons.
         */
        void enableDisableFighting(const bool enable, const bool drawWeapon) const
        {
            auto arguments = F4SEVR::getArgs(enable, drawWeapon);
            executePapyrusScript("EnableDisableFighting", arguments);
        }

        /**
         * Enable/disable that VATS feature in the game.
         * Disabling will free the "B" button for general use.
         */
        void enableDisableVats(const bool enable) const
        {
            auto arguments = F4SEVR::getArgs(enable);
            executePapyrusScript("EnableDisableVats", arguments);
        }

        /**
         * Draw the currently equipped weapon if any.
         */
        void drawWeapon() const
        {
            executePapyrusScript("DrawWeapon");
        }

        /**
         * Holster currently drawn weapon if any.
         */
        void holsterWeapon() const
        {
            executePapyrusScript("HolsterWeapon");
        }

        /**
         * Un-equip the currently equipped weapon.
         * NOT the same as holstering (not available)
         */
        void UnEquipCurrentWeapon() const
        {
            executePapyrusScript("UnEquipCurrentWeapon");
        }

        /**
         * Un-equip melee fist weapon.To handle having the player stuck in fist melee combat mode.
         */
        void fixStuckFistsMelee() const
        {
            executePapyrusScript("FixStuckFistsMelee");
        }

        /**
         * Un-equip melee fist weapon.To handle having the player stuck in fist melee combat mode.
         */
        void activateFix(const bool enable) const
        {
            auto arguments = F4SEVR::getArgs(enable);
            executePapyrusScript("ActivateFix", arguments);
        }
    };
}
