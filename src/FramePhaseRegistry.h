#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace frik
{
    /**
     * Points in FRIK's frame where external mods can run. Values are the API contract: append only.
     */
    enum class FramePhase : std::uint8_t
    {
        // The engine's animation graph output for the player, before FRIK touches the body.
        NativeGraphOutput = 0,
        // The body root is placed under the HMD and posture is set.
        BodyPlaced = 1,
        LegsSolved = 2,
        // Hand transforms published here are solved in this frame.
        BeforeArmSolve = 3,
        AfterArmSolve = 4,
        AfterHandPose = 5,
        AfterWeaponPosition = 6,
        BeforeWorldFinal = 7,
        // The last phase of the frame; every bone world transform is final.
        AfterWorldFinal = 8,
        // The start of FRIK's frame, before the skeleton check: the one phase that runs without a skeleton.
        FrameBegin = 9,
        // The end of FRIK's frame; runs every frame like FrameBegin, whether or not the skeleton phases ran.
        FrameEnd = 10,
    };

    inline constexpr std::uint32_t FRAME_PHASE_COUNT = 11;

    using FrameCallback = void(__cdecl*)(std::uint32_t phase, void* userData) noexcept;

    /**
     * Fixed-capacity registry of per-phase callbacks, keyed by tag and phase.
     *
     * Callbacks run by descending priority, then registration order, so at equal priority the newest
     * registration runs last and its writes win. Re-setting a tag and phase keeps its place in that order.
     * Registration and removal are refused while a callback is on the stack. Game thread only.
     */
    class FramePhaseRegistry
    {
    public:
        static constexpr std::size_t CAPACITY = 32;

        enum class Result : std::uint8_t
        {
            Registered,
            Replaced,
            BadTag,
            NullCallback,
            BadPhase,
            NegativePriority,
            Full,
            Reentrant,
        };

        Result set(const std::string_view tag, const std::uint32_t phase, const FrameCallback callback, void* const userData, const int priority)
        {
            if (_invoking) {
                return Result::Reentrant;
            }
            if (tag.empty()) {
                return Result::BadTag;
            }
            if (!callback) {
                return Result::NullCallback;
            }
            if (phase >= FRAME_PHASE_COUNT) {
                return Result::BadPhase;
            }
            if (priority < 0) {
                return Result::NegativePriority;
            }

            auto* entry = find(tag, phase);
            const bool replacing = entry != nullptr;
            if (!entry) {
                entry = findFree();
                if (!entry) {
                    return Result::Full;
                }
                entry->tag = std::string(tag);
                entry->phase = phase;
                entry->generation = ++_generation;
                entry->active = true;
            }
            entry->callback = callback;
            entry->userData = userData;
            entry->priority = priority;
            rebuildOrder();
            return replacing ? Result::Replaced : Result::Registered;
        }

        /**
         * Drop every phase registered under a tag. Unknown tags succeed.
         * @return false while a callback is on the stack or for an empty tag.
         */
        bool remove(const std::string_view tag, std::size_t* const outRemoved = nullptr)
        {
            if (_invoking || tag.empty()) {
                return false;
            }
            std::size_t removed = 0;
            for (auto& entry : _entries) {
                if (entry.active && entry.tag == tag) {
                    entry = {};
                    ++removed;
                }
            }
            if (removed > 0) {
                rebuildOrder();
            }
            if (outRemoved) {
                *outRemoved = removed;
            }
            return true;
        }

        /**
         * Run every callback registered for a phase, in order.
         * @return the number of callbacks run; 0 for an unknown phase or a nested invoke.
         */
        std::size_t invoke(const std::uint32_t phase)
        {
            if (_invoking || phase >= FRAME_PHASE_COUNT) {
                return 0;
            }
            const auto count = _orderCount[phase];
            _invoking = true;
            for (std::size_t i = 0; i < count; ++i) {
                const auto& entry = _entries[_order[phase][i]];
                entry.callback(phase, entry.userData);
            }
            _invoking = false;
            return count;
        }

        void clear()
        {
            for (auto& entry : _entries) {
                entry = {};
            }
            _orderCount.fill(0);
            _generation = 0;
            _invoking = false;
        }

        std::size_t count() const
        {
            std::size_t active = 0;
            for (const auto& entry : _entries) {
                active += entry.active ? 1 : 0;
            }
            return active;
        }

        std::size_t count(const std::uint32_t phase) const
        {
            return phase < FRAME_PHASE_COUNT ? _orderCount[phase] : 0;
        }

        bool isInvoking() const
        {
            return _invoking;
        }

    private:
        struct Entry
        {
            std::string tag;
            FrameCallback callback = nullptr;
            void* userData = nullptr;
            std::uint32_t phase = 0;
            int priority = 0;
            std::uint64_t generation = 0;
            bool active = false;
        };

        Entry* find(const std::string_view tag, const std::uint32_t phase)
        {
            for (auto& entry : _entries) {
                if (entry.active && entry.phase == phase && entry.tag == tag) {
                    return &entry;
                }
            }
            return nullptr;
        }

        Entry* findFree()
        {
            for (auto& entry : _entries) {
                if (!entry.active) {
                    return &entry;
                }
            }
            return nullptr;
        }

        /**
         * Recompute the per-phase invoke order so the frame walk is a plain index scan.
         */
        void rebuildOrder()
        {
            _orderCount.fill(0);
            for (std::uint8_t index = 0; index < CAPACITY; ++index) {
                const auto& entry = _entries[index];
                if (!entry.active) {
                    continue;
                }
                auto& order = _order[entry.phase];
                auto& count = _orderCount[entry.phase];
                std::size_t at = count;
                while (at > 0) {
                    const auto& before = _entries[order[at - 1]];
                    if (before.priority > entry.priority || (before.priority == entry.priority && before.generation < entry.generation)) {
                        break;
                    }
                    order[at] = order[at - 1];
                    --at;
                }
                order[at] = index;
                ++count;
            }
        }

        std::array<Entry, CAPACITY> _entries{};
        std::array<std::array<std::uint8_t, CAPACITY>, FRAME_PHASE_COUNT> _order{};
        std::array<std::size_t, FRAME_PHASE_COUNT> _orderCount{};
        std::uint64_t _generation = 0;
        bool _invoking = false;
    };
}
