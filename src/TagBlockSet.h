#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>

namespace frik
{
    /**
     * A set of external tags blocking one FRIK behavior, which stays blocked while
     * at least one tag is still holding it.
     *
     * Tags let several client mods block the same behavior without clobbering each
     * other - the values mean nothing to FRIK, only whether the set is empty.
     * Registration arrives from client mods while the resulting state is read once
     * per frame on the game thread, so mutation takes the lock and mirrors the size
     * into an atomic that isBlocked() reads without one.
     */
    class TagBlockSet
    {
    public:
        /**
         * Add or remove one tag's block.
         *
         * @return false if the tag is empty, which cannot identify a blocker.
         */
        bool setBlocked(const std::string_view tag, const bool blocked)
        {
            if (tag.empty()) {
                return false;
            }

            std::lock_guard lock(_lock);
            if (blocked) {
                _tags.emplace(tag);
            } else {
                _tags.erase(std::string(tag));
            }
            _count.store(static_cast<std::uint32_t>(_tags.size()), std::memory_order_release);
            return true;
        }

        /**
         * Return whether any tag is currently blocking.
         * Read per hand per frame, so it uses the atomic count rather than the lock.
         */
        bool isBlocked() const
        {
            return _count.load(std::memory_order_acquire) != 0;
        }

        /**
         * Number of tags currently blocking, for logging.
         */
        std::uint32_t blockingCount() const
        {
            return _count.load(std::memory_order_acquire);
        }

        /**
         * Drop every block, for when the skeleton is released and clients must republish.
         */
        void clear()
        {
            std::lock_guard lock(_lock);
            _tags.clear();
            _count.store(0, std::memory_order_release);
        }

    private:
        mutable std::mutex _lock;
        std::unordered_set<std::string> _tags;
        std::atomic<std::uint32_t> _count{ 0 };
    };
}
