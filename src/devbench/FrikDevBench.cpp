#include "devbench/FrikDevBench.h"

#include <array>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "Config.h"
#include "FRIK.h"
#include "devbench/DevBench.h"
#include "devbench/DevBenchProbe.h"
#include "f4vr/F4VRUtils.h"

namespace frik::devbench
{
    namespace
    {
        using json = nlohmann::json;

        /**
         * How far FRIK::onFrameUpdate got. A reader must tell "FRIK is running and the body is up" from "FRIK ran and
         * bailed early", as every flag means something different in the second case.
         */
        enum class SkeletonState : std::uint8_t
        {
            NoPlayer,
            NotReady,
            Ready,
        };

        const char* toString(const SkeletonState state)
        {
            switch (state) {
            case SkeletonState::NoPlayer:
                return "noPlayer";
            case SkeletonState::NotReady:
                return "notReady";
            case SkeletonState::Ready:
            default:
                return "ready";
            }
        }

        /**
         * A frame of FRIK state. Plain values only: a snapshot outlives its frame, and the scene graph is torn down and
         * rebuilt on every skeleton release.
         */
        struct FrikState
        {
            std::uint64_t skeletonGeneration = 0;
            SkeletonState skeleton = SkeletonState::NoPlayer;

            bool playerPresent = false;
            bool skeletonReady = false;
            bool inPowerArmor = false;
            bool selfieMode = false;

            bool pipboyOn = false;
            bool pipboyOperatingWithFinger = false;
            bool pipboyEnabled = false;

            bool meleeWeaponDrawn = false;
            bool offHandGrippingWeapon = false;
            bool weaponRepositionMode = false;
            bool weaponPositionEnabled = false;
            bool lookingThroughScope = false;
            bool inScopeMenu = false;

            bool mainConfigModeActive = false;
            bool pipboyConfigModeActive = false;
            bool pipboyConfigModeAdjusting = false;
            bool pauseMenuOpen = false;
            bool favoritesMenuOpen = false;
            bool dialogueMenuOpen = false;

            bool flashlightEnabled = false;
            bool smoothMovementEnabled = false;
        };

        /**
         * One flag of the state action: where it goes in the JSON, and which member holds it.
         */
        struct Flag
        {
            const char* group;
            const char* name;
            bool FrikState::*value;
        };

        /**
         * Every flag FRIK reports, in one list: the state JSON, its description and the transition events are all built
         * from it, so they can't drift apart. A FrikState member that isn't listed here is never reported. Flags of a
         * group stay together.
         */
        constexpr std::array FLAGS = {
            Flag{ "body", "playerPresent", &FrikState::playerPresent },
            Flag{ "body", "skeletonReady", &FrikState::skeletonReady },
            Flag{ "body", "inPowerArmor", &FrikState::inPowerArmor },
            Flag{ "body", "selfieMode", &FrikState::selfieMode },
            Flag{ "pipboy", "on", &FrikState::pipboyOn },
            Flag{ "pipboy", "operatingWithFinger", &FrikState::pipboyOperatingWithFinger },
            Flag{ "pipboy", "enabled", &FrikState::pipboyEnabled },
            Flag{ "weapon", "meleeDrawn", &FrikState::meleeWeaponDrawn },
            Flag{ "weapon", "offHandGripping", &FrikState::offHandGrippingWeapon },
            Flag{ "weapon", "repositionMode", &FrikState::weaponRepositionMode },
            Flag{ "weapon", "positioningEnabled", &FrikState::weaponPositionEnabled },
            Flag{ "weapon", "lookingThroughScope", &FrikState::lookingThroughScope },
            Flag{ "weapon", "inScopeMenu", &FrikState::inScopeMenu },
            Flag{ "ui", "mainConfigMode", &FrikState::mainConfigModeActive },
            Flag{ "ui", "pipboyConfigMode", &FrikState::pipboyConfigModeActive },
            Flag{ "ui", "pipboyConfigAdjusting", &FrikState::pipboyConfigModeAdjusting },
            Flag{ "ui", "pauseMenu", &FrikState::pauseMenuOpen },
            Flag{ "ui", "favoritesMenu", &FrikState::favoritesMenuOpen },
            Flag{ "ui", "dialogueMenu", &FrikState::dialogueMenuOpen },
            Flag{ "subsystems", "flashlight", &FrikState::flashlightEnabled },
            Flag{ "subsystems", "smoothMovement", &FrikState::smoothMovementEnabled },
        };

        // A flag becomes an event once its new value has held this many frames, so one that flickers (a finger grazing
        // the Pip-Boy screen, a grip right at its threshold) can't flood the event ring every mod shares in devbench.
        constexpr std::uint32_t STABLE_FRAMES = 3;

        /**
         * The value each flag was last reported as, and how many frames a different one has held. Game thread only.
         */
        struct Transitions
        {
            bool primed = false;
            std::array<bool, FLAGS.size()> reported{};
            std::array<std::uint32_t, FLAGS.size()> pendingFrames{};
        };

        Transitions g_transitions;

        /**
         * Publish frik.<group>.<flag> for every flag whose new value has held STABLE_FRAMES frames. It rides on the state
         * capture, so it runs only while the tool is armed (devbench installed, a Tracy viewer connected, or perf_reset),
         * and costs nothing beyond comparing what was captured.
         */
        void emitTransitions(const FrikState& state)
        {
            auto& t = g_transitions;
            if (!t.primed) {
                // the first capture after arming is the baseline: nothing has changed yet
                for (std::size_t i = 0; i < FLAGS.size(); ++i) {
                    t.reported[i] = state.*FLAGS[i].value;
                }
                t.primed = true;
                return;
            }
            for (std::size_t i = 0; i < FLAGS.size(); ++i) {
                const bool value = state.*FLAGS[i].value;
                if (value == t.reported[i]) {
                    t.pendingFrames[i] = 0;
                    continue;
                }
                if (++t.pendingFrames[i] < STABLE_FRAMES) {
                    continue;
                }
                t.reported[i] = value;
                t.pendingFrames[i] = 0;
                f4cf::devbench::emit(std::format("{}.{}", FLAGS[i].group, FLAGS[i].name), [&] {
                    return json{ { "value", value }, { "skeletonGeneration", state.skeletonGeneration } };
                });
            }
        }

        /**
         * Game thread, after every onFrameUpdate while the tool is armed.
         */
        void captureState(FrikState& state)
        {
            state.skeletonGeneration = g_frik.getSkeletonGeneration();

            state.playerPresent = RE::PlayerCharacter::GetSingleton() != nullptr;
            state.skeletonReady = g_frik.isSkeletonReady();
            state.skeleton = !state.playerPresent ? SkeletonState::NoPlayer : state.skeletonReady ? SkeletonState::Ready : SkeletonState::NotReady;
            state.inPowerArmor = state.playerPresent && f4vr::isInPowerArmor();
            state.selfieMode = g_frik.isSelfieModeOn();

            state.pipboyOn = g_frik.isPipboyOn();
            state.pipboyOperatingWithFinger = g_frik.isPipboyOperatingWithFinger();
            state.pipboyEnabled = g_frik.isPipboyEnabled();

            state.meleeWeaponDrawn = g_frik.isMeleeWeaponDrawn();
            state.offHandGrippingWeapon = g_frik.isOffHandGrippingWeapon();
            state.weaponRepositionMode = g_frik.inWeaponRepositionMode();
            state.weaponPositionEnabled = g_frik.isWeaponPositionEnabled();
            state.lookingThroughScope = g_frik.isLookingThroughScope();
            state.inScopeMenu = g_frik.isInScopeMenu();

            state.mainConfigModeActive = g_frik.isMainConfigurationModeActive();
            state.pipboyConfigModeActive = g_frik.isPipboyConfigurationModeActive();
            state.pipboyConfigModeAdjusting = g_frik.isPipboyConfigurationModeAdjusting();
            state.pauseMenuOpen = g_frik.isPauseMenuOpen();
            state.favoritesMenuOpen = g_frik.isFavoritesMenuOpen();
            state.dialogueMenuOpen = g_frik.isDialogueMenuOpen();

            state.flashlightEnabled = g_frik.isFlashlightEnabled();
            state.smoothMovementEnabled = g_frik.isSmoothMovementEnabled();

            emitTransitions(state);
        }

        /**
         * Listener thread: reads only its FrikState.
         */
        json stateToJson(const FrikState& state)
        {
            json out = json::object();
            for (const auto& flag : FLAGS) {
                out[flag.group][flag.name] = state.*flag.value;
            }
            out["liveness"] = { { "skeletonGeneration", state.skeletonGeneration }, { "state", toString(state.skeleton) } };
            return out;
        }

        /**
         * What the state keys mean, for the state action's description, listed from FLAGS.
         */
        std::string stateDescription()
        {
            std::string groups;
            std::string_view group;
            for (const auto& flag : FLAGS) {
                if (flag.group != group) {
                    groups += std::format("{}{} (", group.empty() ? "" : "), ", flag.group);
                    group = flag.group;
                } else {
                    groups += ", ";
                }
                groups += flag.name;
            }
            groups += ")";
            return std::format(
                "{}. liveness adds skeletonGeneration (FRIK's body build counter: a change between two reads means a different "
                "body) and state (noPlayer|notReady|ready: how far FRIK's frame got; without a body every body flag is off). "
                "Every flag's change is also an event, frik.<group>.<flag> with its value, once the new value has held {} frames",
                groups,
                STABLE_FRAMES);
        }

        json probeArguments()
        {
            const auto arg = [](const char* type, const char* description) {
                return json{ { "type", type }, { "description", description } };
            };
            auto offset = arg("array", "probe claim: [x,y,z] from the tracked hand, default [0,0,10]");
            offset["items"] = { { "type", "number" } };
            return {
                { "op",
                    arg("string",
                        "probe: phases (default)|claim|solve|chain|record|recordStop|recordDump|chord|bone|nodes|carry|visibility|grip|parent|block|"
                        "scope|scopeRig|weaponUpdate|pipboy|selfie|reset") },
                { "hand", arg("string", "probe claim/chain/grip: left|right; parent: left|right|clear") },
                { "phase", arg("integer", "probe claim: the FramePhase index to publish in; omit to publish before this frame's skeleton pass") },
                { "kind", arg("string", "probe claim: offset (default: the tracked hand plus offset)|unreachable|clear") },
                { "offset", offset },
                { "on", arg("boolean", "probe grip/block/scope/pipboy/selfie: on (default) or off") },
                { "frames", arg("integer", "probe record: frames to record, default 900") },
                { "from", arg("integer", "probe recordDump: the first recorded frame to return, default 0") },
                { "count", arg("integer", "probe recordDump: frames to return, default 300, at most 1000") },
                { "since", arg("integer", "probe chord: the chord seq already seen") },
                { "name", arg("string", "probe bone: the bone name, default RArm_UpperTwist1") },
                { "under", arg("string", "probe scopeRig: weapon|wand|offwand|clear (default)") },
                { "takeover", arg("boolean", "probe scope: drop True Scopes' registration for the test; restored with on=false") },
            };
        }

        constexpr auto PROBE_DESCRIPTION =
            "dev-only exerciser of FRIK's API from inside FRIK, chosen by op: frame phases (phases), hand claims and the arm solve (claim, solve, "
            "chain), an arm recorder (record, recordStop, recordDump), controller chords (chord), bones and nodes (bone, nodes, visibility), the "
            "left-hand carry (carry), grips and the weapon node (grip, parent, block, weaponUpdate), scopes (scope, scopeRig) and FRIK's own modes "
            "(pipboy, selfie); reset drops everything it registered. pipboy on opens FRIK's Pip-Boy, which puts the engine's PipboyMenu on the "
            "stack: set PipBoyCloseWhenLookAway=false first when nobody is in the headset";
    }

    void setupDevBenchTool()
    {
        f4cf::devbench::setToolDescription(
            "FRIK (Fallout 4 VR Body): the player's full body in VR - body, arm and leg IK, hand and finger poses, the wrist and holo Pip-Boy, "
            "weapon positioning and two-handed grips, scopes, the flashlight and smooth movement - and the API other mods drive it through "
            "(hand claims, frame phases, grips, weapon node ownership).");
        f4cf::devbench::setDefaultConfigSection(INI_SECTION_MAIN);
        f4cf::devbench::setStateProvider<FrikState>(stateDescription(), &captureState, &stateToJson);
        f4cf::devbench::addAction({ "probe", PROBE_DESCRIPTION, probeArguments(), &runProbe });
    }
}
