#include "devbench/DevBenchBridge.h"

#include <chrono>

#include <nlohmann/json.hpp>

#include "Config.h"
#include "FRIK.h"
#include "devbench/DevBenchAPI.h"
#include "f4vr/F4VRUtils.h"

namespace frik::devbench
{
    namespace
    {
        // devbench's own stall watchdog is 5000ms, so answer well before it so a slow frame
        // reads as "FRIK did not answer" rather than "the whole devbench host is stalled".
        constexpr auto kCommandTimeout = std::chrono::milliseconds(2000);

        std::chrono::steady_clock::time_point g_bridgeStart = std::chrono::steady_clock::now();

        [[nodiscard]] std::int64_t nowMs()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_bridgeStart).count();
        }

        [[nodiscard]] nlohmann::json livenessOf(const Snapshot& snapshot)
        {
            return {
                { "frame", snapshot.frame },
                { "skeletonGeneration", snapshot.skeletonGeneration },
                { "ageMs", nowMs() - snapshot.publishedAtMs },
                { "state", toString(snapshot.state) },
            };
        }

        [[nodiscard]] std::string errorJson(const std::string& message)
        {
            // Deliberately carries no data keys. A caller must not be able to read a zero out
            // of a failed call and mistake it for a measurement.
            return nlohmann::json{ { "ok", false }, { "error", message } }.dump();
        }

        [[nodiscard]] std::string stateJson(const Snapshot& s)
        {
            const nlohmann::json body{
                { "ok", true },
                { "liveness", livenessOf(s) },
                { "body", { { "playerPresent", s.playerPresent }, { "skeletonReady", s.skeletonReady }, { "inPowerArmor", s.inPowerArmor }, { "selfieMode", s.selfieMode } } },
                { "pipboy", { { "on", s.pipboyOn }, { "operatingWithFinger", s.pipboyOperatingWithFinger }, { "enabled", s.pipboyEnabled } } },
                { "weapon",
                    { { "meleeDrawn", s.meleeWeaponDrawn },
                        { "offHandGripping", s.offHandGrippingWeapon },
                        { "repositionMode", s.weaponRepositionMode },
                        { "positioningEnabled", s.weaponPositionEnabled },
                        { "lookingThroughScope", s.lookingThroughScope },
                        { "inScopeMenu", s.inScopeMenu } } },
                { "ui",
                    { { "mainConfigMode", s.mainConfigModeActive },
                        { "pipboyConfigMode", s.pipboyConfigModeActive },
                        { "pipboyConfigAdjusting", s.pipboyConfigModeAdjusting },
                        { "pauseMenu", s.pauseMenuOpen },
                        { "favoritesMenu", s.favoritesMenuOpen },
                        { "dialogueMenu", s.dialogueMenuOpen } } },
                { "subsystems", { { "flashlight", s.flashlightEnabled }, { "smoothMovement", s.smoothMovementEnabled } } },
            };
            return body.dump();
        }

        /**
         * The tool handler. Runs on devbench's listener thread, NEVER the game thread, so it
         * may only read the published snapshot or queue work - never touch FRIK directly.
         */
        void frikToolHandler(void*, const char* argsJson, void* sink, const DevBenchAPI::WriteFn write)
        {
            g_devBenchBridge.arm();

            std::string action = "state";
            std::string section;
            std::string key;
            std::string value;
            try {
                if (argsJson && *argsJson) {
                    const auto args = nlohmann::json::parse(argsJson);
                    action = args.value("action", action);
                    section = args.value("section", std::string(INI_SECTION_MAIN));
                    key = args.value("key", std::string());
                    value = args.value("value", std::string());
                } else {
                    section = INI_SECTION_MAIN;
                }
            } catch (const std::exception& ex) {
                write(sink, errorJson(std::string("bad arguments JSON: ") + ex.what()).c_str());
                return;
            }

            if (action == "health") {
                // Answerable without a published frame, so a caller can tell "FRIK is loaded
                // but has not run a frame" from "FRIK is not there at all".
                const auto snapshot = g_devBenchBridge.readSnapshot();
                const nlohmann::json body{
                    { "ok", true },
                    { "plugin", "FRIK" },
                    { "version", Version::NAME },
                    { "armed", g_devBenchBridge.isArmed() },
                    { "hasSnapshot", snapshot != nullptr },
                    { "liveness", snapshot ? livenessOf(*snapshot) : nlohmann::json(nullptr) },
                };
                write(sink, body.dump().c_str());
                return;
            }

            if (action == "state") {
                const auto snapshot = g_devBenchBridge.readSnapshot();
                if (!snapshot) {
                    write(sink, errorJson("no frame published yet - FRIK is loaded but onFrameUpdate has not run since the tool was armed").c_str());
                    return;
                }
                write(sink, stateJson(*snapshot).c_str());
                return;
            }

            if (action == "config") {
                if (key.empty()) {
                    write(sink, errorJson("action='config' needs 'key' (and optionally 'section')").c_str());
                    return;
                }
                // Reads the INI from disk, so it is queued rather than run here.
                write(sink,
                    g_devBenchBridge
                        .runOnGameThread([section, key]() -> std::string {
                            const auto read = g_config.getConfigValue(section.c_str(), key.c_str());
                            return nlohmann::json{ { "ok", true }, { "section", section }, { "key", key }, { "value", read } }.dump();
                        })
                        .c_str());
                return;
            }

            if (action == "set") {
                if (key.empty() || value.empty()) {
                    write(sink, errorJson("action='set' needs both 'key' and 'value'").c_str());
                    return;
                }
                // setConfigOverride re-reads the INI and rewrites every typed config member,
                // which the game thread reads mid-frame. Queue it; never run it from here.
                write(sink,
                    g_devBenchBridge
                        .runOnGameThread([section, key, value]() -> std::string {
                            g_config.setConfigOverride(section.c_str(), key.c_str(), config::IniValue(value));
                            return nlohmann::json{ { "ok", true }, { "section", section }, { "key", key }, { "value", value }, { "note", "session override; not written to disk" } }
                                .dump();
                        })
                        .c_str());
                return;
            }

            if (action == "clear") {
                if (key.empty()) {
                    write(sink, errorJson("action='clear' needs 'key'").c_str());
                    return;
                }
                write(sink,
                    g_devBenchBridge
                        .runOnGameThread([section, key]() -> std::string {
                            g_config.clearConfigOverride(section.c_str(), key.c_str());
                            return nlohmann::json{ { "ok", true }, { "section", section }, { "key", key }, { "note", "override cleared; the on-disk value applies again" } }.dump();
                        })
                        .c_str());
                return;
            }

            write(sink, errorJson("unknown action '" + action + "' (state|config|set|clear|health)").c_str());
        }

        constexpr auto kDescriptor = R"({
"description":"FRIK (Fallout 4 VR body IK) live state and config. 'state' returns the body, Pip-Boy, weapon-positioning, config-UI and subsystem flags as of the last rendered frame, with a 'liveness' block carrying frame, skeletonGeneration and ageMs - ALWAYS read liveness first: a stale or absent frame means the rest is not a measurement. 'health' answers even before FRIK has run a frame, so it distinguishes 'FRIK is loaded but idle' from 'FRIK is absent'. 'config' reads one FRIK.ini value; 'set' overrides one for the session without writing to disk; 'clear' drops the override. skeletonGeneration increments on every skeleton build AND release, so a change between two reads means the body you measured is not the body you are looking at now.",
"readOnly":false,
"inputSchema":{
 "type":"object",
 "properties":{
  "action":{"type":"string","default":"state","enum":["state","config","set","clear","health"]},
  "section":{"type":"string","description":"INI section; defaults to FRIK's main section."},
  "key":{"type":"string","description":"config/set/clear: the setting name."},
  "value":{"type":"string","description":"set: the new value."}
 }
}})";
    }

    const char* toString(const SkeletonState state)
    {
        switch (state) {
        case SkeletonState::NoPlayer:
            return "noPlayer";
        case SkeletonState::NotReady:
            return "notReady";
        case SkeletonState::Ready:
            return "ready";
        case SkeletonState::Unknown:
        default:
            return "unknown";
        }
    }

    void Bridge::registerWithDevBench()
    {
        if (_registered.load(std::memory_order_relaxed)) {
            return;
        }

        g_devBenchInterface = DevBenchAPI::GetDevBenchInterface001();
        if (!g_devBenchInterface) {
            // Not an error. devbench is a development dependency and must never become a
            // load-order requirement.
            logger::info("devbench not present; FRIK tool not registered");
            return;
        }

        const auto build = g_devBenchInterface->GetBuildNumber();
        g_devBenchInterface->RegisterTool("frik", kDescriptor, &frikToolHandler, nullptr);
        _registered.store(true, std::memory_order_relaxed);
        logger::info("devbench build {} - registered the 'frik' tool", build);
    }

    void Bridge::drainCommands()
    {
        if (_pendingCommands.load(std::memory_order_relaxed) == 0) {
            return;
        }

        std::vector<Command> commands;
        {
            std::lock_guard lock(_commandsLock);
            commands.swap(_commands);
            _pendingCommands.store(0, std::memory_order_relaxed);
        }

        for (auto& command : commands) {
            std::string result;
            try {
                result = command.fn();
            } catch (const std::exception& ex) {
                result = errorJson(std::string("command threw on the game thread: ") + ex.what());
            }
            // The waiter may already have timed out and gone; the promise is owned by this
            // queue entry, so setting it is still safe.
            try {
                command.result->set_value(std::move(result));
            } catch (const std::future_error&) {
            }
        }
    }

    void Bridge::publishSnapshot()
    {
        if (!_armed.load(std::memory_order_relaxed)) {
            // Nobody has asked, so this costs one relaxed load per frame and nothing else.
            return;
        }

        auto snapshot = std::make_shared<Snapshot>();
        snapshot->frame = _frame.fetch_add(1, std::memory_order_relaxed) + 1;
        snapshot->skeletonGeneration = _skeletonGeneration.load(std::memory_order_relaxed);
        snapshot->publishedAtMs = nowMs();

        snapshot->playerPresent = RE::PlayerCharacter::GetSingleton() != nullptr;
        snapshot->skeletonReady = g_frik.isSkeletonReady();
        snapshot->state = !snapshot->playerPresent ? SkeletonState::NoPlayer : snapshot->skeletonReady ? SkeletonState::Ready : SkeletonState::NotReady;

        if (snapshot->playerPresent) {
            snapshot->inPowerArmor = f4vr::isInPowerArmor();
        }

        snapshot->selfieMode = g_frik.isSelfieModeOn();

        snapshot->pipboyOn = g_frik.isPipboyOn();
        snapshot->pipboyOperatingWithFinger = g_frik.isPipboyOperatingWithFinger();
        snapshot->pipboyEnabled = g_frik.isPipboyEnabled();

        snapshot->meleeWeaponDrawn = g_frik.isMeleeWeaponDrawn();
        snapshot->offHandGrippingWeapon = g_frik.isOffHandGrippingWeapon();
        snapshot->weaponRepositionMode = g_frik.inWeaponRepositionMode();
        snapshot->weaponPositionEnabled = g_frik.isWeaponPositionEnabled();
        snapshot->lookingThroughScope = g_frik.isLookingThroughScope();
        snapshot->inScopeMenu = g_frik.isInScopeMenu();

        snapshot->mainConfigModeActive = g_frik.isMainConfigurationModeActive();
        snapshot->pipboyConfigModeActive = g_frik.isPipboyConfigurationModeActive();
        snapshot->pipboyConfigModeAdjusting = g_frik.isPipboyConfigurationModeAdjusting();
        snapshot->pauseMenuOpen = g_frik.isPauseMenuOpen();
        snapshot->favoritesMenuOpen = g_frik.isFavoritesMenuOpen();
        snapshot->dialogueMenuOpen = g_frik.isDialogueMenuOpen();

        snapshot->flashlightEnabled = g_frik.isFlashlightEnabled();
        snapshot->smoothMovementEnabled = g_frik.isSmoothMovementEnabled();

        _snapshot.store(std::move(snapshot));
    }

    std::string Bridge::runOnGameThread(std::function<std::string()> fn)
    {
        auto promise = std::make_shared<std::promise<std::string>>();
        auto future = promise->get_future();

        {
            std::lock_guard lock(_commandsLock);
            _commands.emplace_back(Command{ std::move(fn), promise });
            _pendingCommands.store(static_cast<std::uint32_t>(_commands.size()), std::memory_order_relaxed);
        }

        if (future.wait_for(kCommandTimeout) != std::future_status::ready) {
            return errorJson("the game thread did not run the command within 2000ms - it is stalled, paused, or FRIK's frame update is not running");
        }
        return future.get();
    }
}
