#pragma once

#include <array>
#include <cstdint>

#include "f4vr/PlayerNodes.h"

namespace frik
{
    /**
     * Hand recoil ("kickback") authority for the equipped weapon.
     *
     * The game drives recoil through a single animated node under the primary wand
     * (PlayerNodes::primaryWeaponKickbackRecoilNode) that the first-person arm update folds
     * into the hand. External mods can take that over through the FRIK API: each frame the
     * native kick is sampled, offered to the registered recoil controllers, and whatever they
     * return is applied to the IK hand world targets instead. While a controller owns the frame
     * the native kick is neutralized (see ScopedNativeKickNeutralizer) so it is not applied twice.
     *
     * State is per-frame, so onFrameUpdate must run before the arms are solved. The per-hand
     * world deltas are then built on first use and cached, because both the tracked arm pass and
     * an external hand authority can ask for the same hand within one frame.
     */
    class WeaponHandRecoil
    {
    public:
        void onFrameUpdate(f4vr::PlayerNodes* playerNodes, bool physicalPrimaryIsLeft);

        /**
         * Return whether a controller accepted ownership of this frame's recoil.
         * False means the hands keep the native kick the game already animated.
         */
        bool isControlled() const
        {
            return _responseAccepted;
        }

        bool applyToHandWorldTarget(bool isLeft, RE::NiTransform& target);

        /**
         * Zero the native kick node for the duration of the scope and restore it on exit.
         *
         * Bracket the game's first-person arm update with this so the native kick isn't left
         * stacked underneath the controlled one. Does nothing while recoil is uncontrolled,
         * which is what keeps vanilla recoil working when no controller is registered.
         */
        class ScopedNativeKickNeutralizer
        {
        public:
            explicit ScopedNativeKickNeutralizer(const WeaponHandRecoil& recoil);
            ~ScopedNativeKickNeutralizer();

            ScopedNativeKickNeutralizer(const ScopedNativeKickNeutralizer&) = delete;
            ScopedNativeKickNeutralizer& operator=(const ScopedNativeKickNeutralizer&) = delete;

        private:
            RE::NiNode* _node = nullptr;
            RE::NiTransform _savedLocal{};
            bool _active = false;
        };

    private:
        RE::NiTransform dampen(const RE::NiTransform& kick);
        bool buildWorldDelta(bool isLeft, RE::NiTransform& outWorldDelta) const;

        f4vr::PlayerNodes* _playerNodes = nullptr;

        bool _responseAccepted = false;

        // Which hand roles the accepted response selected, as RecoilHandMask bits. The rest of the
        // response is consumed while resolving the frame and never outlives it.
        std::uint32_t _handMask = 0;

        // The hand physically holding the weapon, which is what the response's hand mask is keyed
        // on. Not the same as the game's left-handed setting while a mod owns the primary weapon node.
        bool _physicalPrimaryIsLeft = false;

        // This frame's controlled kick, in the native kick node's parent frame
        RE::NiTransform _controlledKickLocal{};

        // Per-hand world deltas derived from the controlled kick, built on demand ([0] = right, [1] = left)
        std::array<RE::NiTransform, 2> _worldDeltas{};
        std::array<bool, 2> _worldDeltaValid{};

        // Carried across frames for damped delivery, and dropped on any frame that doesn't damp
        RE::NiTransform _smoothedKickLocal{};
        bool _smoothedValid = false;
    };
}
