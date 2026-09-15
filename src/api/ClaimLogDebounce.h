#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace frik::api
{
    /**
     * Folds a hand claim that a client clears and re-sets within a short window, typically every frame,
     * into one logged start and one logged end, so a churning client cannot flood the log.
     *
     * An end is only known once the claim has stayed cleared for the whole window, so takeEnded()
     * reports it that late. Clients may publish from any thread, so every call takes the lock.
     */
    class ClaimLogDebounce
    {
    public:
        using Clock = std::chrono::steady_clock;

        struct Ended
        {
            std::string tag;
            bool isLeft;
            std::uint32_t restarts;
        };

        explicit ClaimLogDebounce(const Clock::duration window = std::chrono::seconds(1))
            : _window(window)
        {}

        /**
         * A claim was inserted.
         * @return true when it starts a new claim and should be logged, false when it re-sets one cleared within the window.
         */
        bool onStart(const std::string_view tag, const bool isLeft)
        {
            std::lock_guard lock(_lock);
            auto [it, inserted] = _claims.try_emplace(Key{ std::string(tag), isLeft });
            auto& claim = it->second;
            // still active means a skeleton release dropped it without a clear, so this is a new claim
            if (inserted || claim.active) {
                claim = { .active = true };
                return true;
            }
            claim.active = true;
            ++claim.restarts;
            return false;
        }

        /**
         * A claim was removed; its end is reported by takeEnded() once it stays cleared for the window.
         */
        void onEnd(const std::string_view tag, const bool isLeft, const Clock::time_point now)
        {
            std::lock_guard lock(_lock);
            auto& claim = _claims[Key{ std::string(tag), isLeft }];
            claim.active = false;
            claim.clearedAt = now;
        }

        /**
         * Remove and return the claims that have stayed cleared for the whole window.
         */
        std::vector<Ended> takeEnded(const Clock::time_point now)
        {
            std::vector<Ended> ended;
            std::lock_guard lock(_lock);
            for (auto it = _claims.begin(); it != _claims.end();) {
                if (!it->second.active && now - it->second.clearedAt >= _window) {
                    ended.push_back({ it->first.first, it->first.second, it->second.restarts });
                    it = _claims.erase(it);
                } else {
                    ++it;
                }
            }
            return ended;
        }

    private:
        using Key = std::pair<std::string, bool>;

        struct Claim
        {
            bool active = false;
            std::uint32_t restarts = 0;
            Clock::time_point clearedAt{};
        };

        Clock::duration _window;
        std::mutex _lock;
        std::map<Key, Claim> _claims;
    };
}
