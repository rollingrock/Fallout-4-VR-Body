#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace frik
{
    /**
     * What a registered scope provider (True Scopes, ...) takes over from FRIK while the player looks through a scope.
     */
    enum class ScopeCapability : std::uint32_t
    {
        // The body root is never hidden while scoped; the provider renders the main view with the body in it.
        KeepsBodyVisible = 1u << 0,
        // FRIK leaves the primaryWeaponScopeCamera node alone.
        OwnsScopeCamera = 1u << 1,
        // The provider's setLookingThroughScope replaces the vanilla ScopeMenu state as the looking-through signal.
        PublishesLookingThrough = 1u << 2,
        // FRIK does not dampen hands or recoil while scoped; the provider smooths its own view.
        OwnsDamping = 1u << 3,
        // The provider places its own widget on the scope: during an external carry FRIK hangs ScopeParent under the wand of the
        // carrying hand (world-preserving) and carries only the scope camera with the weapon node, because the scope widget is not
        // drawn under the first-person skeleton.
        PlacesScopeWidget = 1u << 4,
    };

    inline constexpr std::uint32_t SCOPE_CAPABILITY_ALL = 0x1F;

    /**
     * The scope provider registry and the looking-through-scope state FRIK keys every scope behaviour on.
     *
     * Providers register once per session with the capabilities they take over and survive skeleton
     * rebuilds, like feature blocks. Capabilities are the union over registered tags. The looking-through
     * flag is true while any publishing provider says so, kept per tag so one provider leaving or flipping
     * cannot strand another's state; without a publisher the vanilla ScopeMenu state stands in.
     * Registration arrives from client mods while the flags are read every frame on the game thread, so
     * mutation takes the lock and mirrors into atomics the readers use without one.
     */
    class ScopeAuthority
    {
    public:
        /**
         * Register or replace a provider's capabilities.
         *
         * @param outChanged optional; whether the effective capabilities changed.
         * @return false if the tag is empty or capabilities carries unknown bits.
         */
        bool setProvider(const std::string_view tag, const std::uint32_t capabilities, bool* const outChanged = nullptr)
        {
            if (tag.empty() || (capabilities & ~SCOPE_CAPABILITY_ALL) != 0) {
                return false;
            }

            std::lock_guard lock(_lock);
            // (re)registration starts from not looking through; the provider publishes its next edge
            _providers.insert_or_assign(std::string(tag), Provider{ capabilities, false });
            const bool changed = refreshCapabilities();
            if (outChanged) {
                *outChanged = changed;
            }
            return true;
        }

        /**
         * Drop a provider. Unknown tags succeed, since the tag holds nothing either way.
         */
        bool clearProvider(const std::string_view tag, bool* const outChanged = nullptr)
        {
            if (tag.empty()) {
                return false;
            }

            std::lock_guard lock(_lock);
            const bool removed = _providers.erase(std::string(tag)) > 0;
            const bool changed = refreshCapabilities() || removed;
            if (outChanged) {
                *outChanged = changed;
            }
            return true;
        }

        /**
         * Publish the looking-through-scope state. Only a registered provider with PublishesLookingThrough may.
         */
        bool setLookingThroughScope(const std::string_view tag, const bool lookingThrough)
        {
            std::lock_guard lock(_lock);
            const auto it = _providers.find(std::string(tag));
            if (it == _providers.end() || (it->second.capabilities & static_cast<std::uint32_t>(ScopeCapability::PublishesLookingThrough)) == 0) {
                return false;
            }
            it->second.lookingThrough = lookingThrough;
            refreshCapabilities();
            return true;
        }

        bool hasCapability(const ScopeCapability capability) const
        {
            return (_capabilities.load(std::memory_order_acquire) & static_cast<std::uint32_t>(capability)) != 0;
        }

        /**
         * The effective looking-through-scope state: the published flag when a provider owns it, else the vanilla ScopeMenu state.
         */
        bool isLookingThroughScope(const bool vanillaScopeMenuOpen) const
        {
            return hasCapability(ScopeCapability::PublishesLookingThrough) ? _lookingThrough.load(std::memory_order_acquire) : vanillaScopeMenuOpen;
        }

        std::size_t providerCount() const
        {
            std::lock_guard lock(_lock);
            return _providers.size();
        }

    private:
        // Requires _lock. Recomputes the capability union and the looking-through OR over publishing providers.
        bool refreshCapabilities()
        {
            std::uint32_t capabilities = 0;
            bool lookingThrough = false;
            for (const auto& [_, provider] : _providers) {
                capabilities |= provider.capabilities;
                lookingThrough = lookingThrough || ((provider.capabilities & static_cast<std::uint32_t>(ScopeCapability::PublishesLookingThrough)) != 0 && provider.lookingThrough);
            }
            _lookingThrough.store(lookingThrough, std::memory_order_release);
            return _capabilities.exchange(capabilities, std::memory_order_acq_rel) != capabilities;
        }

        struct Provider
        {
            std::uint32_t capabilities = 0;
            bool lookingThrough = false;
        };

        mutable std::mutex _lock;
        std::unordered_map<std::string, Provider> _providers;
        std::atomic<std::uint32_t> _capabilities{ 0 };
        std::atomic<bool> _lookingThrough{ false };
    };

    // Global singleton for easy access
    inline ScopeAuthority g_scopeAuthority;
}
