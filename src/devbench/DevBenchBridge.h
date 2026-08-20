#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace frik::devbench
{
    /**
     * Which branch of FRIK::onFrameUpdate produced the snapshot.
     *
     * A reader must be able to tell "FRIK is running and the body is up" from "FRIK ran and
     * bailed early", because every other field means something different in the second case.
     */
    enum class SkeletonState : std::uint8_t
    {
        Unknown,
        NoPlayer,
        NotReady,
        Ready,
    };

    [[nodiscard]] const char* toString(SkeletonState state);

    /**
     * A frame of FRIK state, copied on the game thread and read on devbench's listener thread.
     *
     * PLAIN VALUE DATA ONLY. No RE::NiNode*, no RE::Actor*, nothing pointing into the scene
     * graph - the listener thread holds this for an unbounded time while it serializes, and
     * the scene graph is torn down and rebuilt underneath it on every skeleton release.
     */
    struct Snapshot
    {
        // ----- liveness. Read these before believing anything below them. -----
        std::uint64_t frame = 0; // bridge counter, ++ every published frame. NOT the engine's.
        std::uint64_t skeletonGeneration = 0; // ++ on every initSkeleton and releaseSkeleton
        std::int64_t publishedAtMs = 0; // steady_clock ms since the bridge was constructed
        SkeletonState state = SkeletonState::Unknown;

        // ----- body -----
        bool playerPresent = false;
        bool skeletonReady = false;
        bool inPowerArmor = false;
        bool selfieMode = false;

        // ----- pipboy -----
        bool pipboyOn = false;
        bool pipboyOperatingWithFinger = false;
        bool pipboyEnabled = false;

        // ----- weapon / hands -----
        bool meleeWeaponDrawn = false;
        bool offHandGrippingWeapon = false;
        bool weaponRepositionMode = false;
        bool weaponPositionEnabled = false;
        bool lookingThroughScope = false;
        bool inScopeMenu = false;

        // ----- ui / modes -----
        bool mainConfigModeActive = false;
        bool pipboyConfigModeActive = false;
        bool pipboyConfigModeAdjusting = false;
        bool pauseMenuOpen = false;
        bool favoritesMenuOpen = false;
        bool dialogueMenuOpen = false;

        // ----- subsystems -----
        bool flashlightEnabled = false;
        bool smoothMovementEnabled = false;
    };

    /**
     * Bridge between FRIK's game thread and devbench's HTTP listener thread.
     *
     * Two one-way channels, deliberately:
     *
     *  - READ: the game thread publishes an immutable Snapshot as a shared_ptr, the listener
     *    thread loads it. The game thread never blocks on a reader, and a reader holds a
     *    consistent frame alive by refcount for as long as it needs.
     *
     *  - WRITE: the listener thread queues a command and waits on a promise; the game thread
     *    runs it at the top of the frame. Anything that touches FRIK or game state goes here.
     *    Nothing may mutate FRIK from the listener thread - see ExternalAuthority.h, whose
     *    header comment is this codebase's statement of that rule.
     *
     * Zero cost until used: publishing is gated on `_armed`, which the tool handler sets on
     * its first invocation. Until some client actually asks, onFrameUpdate does one relaxed
     * atomic load per frame and nothing else.
     */
    class Bridge
    {
    public:
        /// Register FRIK's tool with a running devbench host. Safe, and a silent no-op, when
        /// devbench is absent - it is a development dependency and must never become a
        /// load-order requirement. Call from F4SE kPostPostLoad.
        void registerWithDevBench();

        /// Run queued commands. Game thread, top of FRIK::onFrameUpdate, before any subsystem
        /// reads state - so a config override lands before this frame consumes it.
        void drainCommands();

        /// Publish this frame's snapshot. Game thread, end of FRIK::onFrameUpdate, on EVERY
        /// exit path including the early returns - a snapshot frozen at its last good value
        /// through a loading screen is exactly the lie this is built to avoid.
        void publishSnapshot();

        /// ++ whenever the skeleton is created or released, so a reader can tell that the body
        /// it measured is not the body it is now looking at.
        void bumpSkeletonGeneration()
        {
            _skeletonGeneration.fetch_add(1, std::memory_order_relaxed);
        }

        /// Listener thread. Null until the first frame has been published.
        [[nodiscard]] std::shared_ptr<const Snapshot> readSnapshot() const
        {
            return _snapshot.load();
        }

        /// Listener thread. Queue `fn` for the game thread and block for its result.
        /// On timeout returns a failure JSON rather than throwing - an honest "the game did
        /// not answer" beats a stack trace in an HTTP response.
        std::string runOnGameThread(std::function<std::string()> fn);

        [[nodiscard]] bool isArmed() const
        {
            return _armed.load(std::memory_order_relaxed);
        }

        void arm()
        {
            _armed.store(true, std::memory_order_relaxed);
        }

    private:
        struct Command
        {
            std::function<std::string()> fn;
            // shared_ptr, never a pointer to the waiting thread's stack: on timeout that
            // thread returns and its stack goes away while the command is still queued.
            std::shared_ptr<std::promise<std::string>> result;
        };

        std::atomic<std::shared_ptr<const Snapshot>> _snapshot;
        std::atomic<bool> _armed{ false };
        std::atomic<bool> _registered{ false };
        std::atomic<std::uint64_t> _frame{ 0 };
        std::atomic<std::uint64_t> _skeletonGeneration{ 0 };
        std::atomic<std::uint32_t> _pendingCommands{ 0 };

        mutable std::mutex _commandsLock;
        std::vector<Command> _commands;
    };

    inline Bridge g_devBenchBridge;
}
