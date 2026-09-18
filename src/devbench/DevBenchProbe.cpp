#include "devbench/DevBenchProbe.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ExternalAuthority.h"
#include "FRIK.h"
#include "ScopeAuthority.h"
#include "api/ApiCore.h"
#include "common/MatrixUtils.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/PlayerNodes.h"
#include "vrcf/VRControllersHaptic.h"
#include "vrcf/VRControllersManager.h"

namespace frik::devbench
{
    namespace
    {
        namespace core = api::core;
        using nlohmann::json;

        constexpr auto PROBE_TAG = "frik.probe";
        constexpr auto PROBE_EARLY_TAG = "frik.probe.early";
        constexpr std::uint32_t PHASE_NOW = 255;

        /**
         * One claim request, executed by the probe callback when its phase runs (or right away for PHASE_NOW).
         */
        struct ClaimRequest
        {
            bool isLeft = false;
            std::uint32_t phase = PHASE_NOW;
            bool clear = false;
            bool unreachable = false;
            RE::NiPoint3 offset{ 0, 0, 10 };
        };

        /**
         * What the probe saw at AfterWorldFinal of one frame, for both hands.
         */
        struct FrameRecord
        {
            std::uint64_t frame = 0;
            std::array<core::HandSolveState, 2> state{};
            std::array<RE::NiTransform, 2> wrist{};
        };

        /**
         * One frame of a left-carry: the weapon after the AfterArmSolve callbacks and at AfterWorldFinal, and the right hand against its claim.
         */
        struct CarryRecord
        {
            std::uint64_t frame = 0;
            // latched before the other AfterArmSolve callbacks run, i.e. what they read
            RE::NiTransform weaponBeforeCallbacks{};
            RE::NiTransform rightBoneBeforeCallbacks{};
            RE::NiTransform rightHandNodeBeforeCallbacks{};
            RE::NiTransform leftHandBeforeCallbacks{};
            std::uint64_t leftRevisionBeforeCallbacks = 0;
            RE::NiTransform weaponAfterArmSolve{};
            RE::NiTransform leftHandAfterArmSolve{};
            std::uint64_t leftRevisionAfterCallbacks = 0;
            RE::NiTransform leftHandWorldFinal{};
            RE::NiTransform weaponAfterWorldFinal{};
            bool rightClaimed = false;
            RE::NiTransform rightClaim{};
            core::HandSolveState rightState = core::HandSolveState::NoClaim;
            RE::NiTransform rightWrist{};
            RE::NiTransform rightBone{};
            // the Weapon's own local and its parent's world at each point, and the previous frame's final ones
            RE::NiTransform weaponBeforeArmSolve{};
            RE::NiTransform localBeforeArmSolve{};
            RE::NiTransform parentBeforeArmSolve{};
            RE::NiTransform localPrevFinal{};
            RE::NiTransform parentPrevFinal{};
            RE::NiTransform localBeforeCallbacks{};
            RE::NiTransform parentBeforeCallbacks{};
            RE::NiTransform localAfterArmSolve{};
            RE::NiTransform parentAfterArmSolve{};
            RE::NiTransform localWorldFinal{};
            RE::NiTransform parentWorldFinal{};
        };

        // world of a node from its parent's world and its local, the same composition updateTransforms uses
        RE::NiTransform composeWorld(const RE::NiTransform& parent, const RE::NiTransform& local)
        {
            RE::NiTransform world;
            world.translate = parent.translate + parent.rotate.Transpose() * (local.translate * parent.scale);
            world.rotate = local.rotate * parent.rotate;
            world.scale = parent.scale * local.scale;
            return world;
        }

        // the Weapon node's local and its parent's world, when it has a parent
        bool weaponFrames(RE::NiTransform& outLocal, RE::NiTransform& outParentWorld)
        {
            const auto weapon = f4vr::getWeaponNode();
            if (!weapon || !weapon->parent) {
                return false;
            }
            outLocal = weapon->local;
            outParentWorld = weapon->parent->world;
            return true;
        }

        // Arm recorder: the world transforms of one arm's bones (and the tracked wand) at AfterWorldFinal, every frame, for a sitting
        // to measure elbow swivel, roll, collarbone and twist instead of judging them by eye. Right hand first.
        constexpr std::array<const char*, 7> ARM_BONE_SUFFIXES = { "Collarbone", "UpperArm", "UpperTwist1", "ForeArm1", "ForeArm2", "ForeArm3", "Hand" };
        constexpr std::size_t ARM_SAMPLE_SLOTS = ARM_BONE_SUFFIXES.size() + 1; // + wand
        constexpr std::size_t ARM_RECORD_MAX_FRAMES = 6000;

        struct ArmSample
        {
            std::uint64_t frame = 0;
            std::array<std::array<RE::NiTransform, ARM_SAMPLE_SLOTS>, 2> slots{};
            std::array<std::uint8_t, ARM_SAMPLE_SLOTS * 2> valid{};
            std::array<std::uint8_t, 2> state{};
        };

        struct ProbeState
        {
            bool registered = false;
            // arm recorder
            std::vector<ArmSample> armRecord;
            std::size_t armRecordTarget = 0;
            bool armRecording = false;
            // controller chords (grip + A / B / trigger), detected at FrameEnd, acknowledged by a haptic
            std::uint64_t chordSeq = 0;
            std::string chordLast;
            std::uint64_t chordFrame = 0;
            std::array<std::uint64_t, FRAME_PHASE_COUNT> counts{};
            std::array<std::uint8_t, 16> orderCurrent{};
            std::size_t orderCurrentCount = 0;
            std::array<std::uint8_t, 16> orderLast{};
            std::size_t orderLastCount = 0;
            std::uint64_t frame = 0;
            std::optional<ClaimRequest> pending;
            // the last executed claim and the frame it was published in
            std::optional<ClaimRequest> lastClaim;
            std::uint64_t lastClaimFrame = 0;
            RE::NiTransform lastTarget{};
            std::array<FrameRecord, 4> records{};
            std::size_t recordNext = 0;
            // the frame the last claim was published in and the one after, latched so a slow reader still sees them
            std::array<FrameRecord, 2> claimRecords{};
            // FirstPersonHand as a client reads it in AfterArmSolve, right hand first
            std::array<RE::NiTransform, 2> afterArmSolveHand{};
            // the last left-carry frame, the largest weapon shift seen after AfterArmSolve, and how many carry frames were recorded
            CarryRecord carry{};
            CarryRecord carryStaging{};
            bool carryPending = false;
            float carryMaxWeaponShift = 0;
            std::uint64_t carryFrames = 0;
        };

        ProbeState g_probe;

        json transformJson(const RE::NiTransform& t)
        {
            float heading = 0, roll = 0, attitude = 0;
            common::MatrixUtils::getEulerAnglesFromMatrixDegrees(t.rotate, &heading, &roll, &attitude);
            return { { "pos", { t.translate.x, t.translate.y, t.translate.z } }, { "eulerDeg", { heading, roll, attitude } }, { "scale", t.scale } };
        }

        float rotationAngleDeg(const RE::NiMatrix3& a, const RE::NiMatrix3& b)
        {
            const auto d = a * b.Transpose();
            const float trace = d.entry[0][0] + d.entry[1][1] + d.entry[2][2];
            return common::MatrixUtils::radsToDegrees(std::acos(std::clamp((trace - 1.0f) * 0.5f, -1.0f, 1.0f)));
        }

        json diffJson(const RE::NiTransform& a, const RE::NiTransform& b)
        {
            return { { "distance", common::MatrixUtils::vec3Len(a.translate - b.translate) }, { "angleDeg", rotationAngleDeg(a.rotate, b.rotate) } };
        }

        const char* stateName(const core::HandSolveState s)
        {
            switch (s) {
            case core::HandSolveState::SkeletonNotReady:
                return "SkeletonNotReady";
            case core::HandSolveState::NoClaim:
                return "NoClaim";
            case core::HandSolveState::Consumed:
                return "Consumed";
            case core::HandSolveState::Unreachable:
                return "Unreachable";
            }
            return "?";
        }

        void handNodeWorld(const bool isLeft, RE::NiTransform& out)
        {
            if (const auto* skelly = g_frik.getSkeleton(); skelly && skelly->getArm(isLeft).hand) {
                out = skelly->getArm(isLeft).hand->world;
            }
        }

        void executeClaim(const ClaimRequest& request)
        {
            if (request.clear) {
                core::clearHandWorldTransform(PROBE_TAG, request.isLeft);
            } else {
                RE::NiTransform target;
                if (!core::getTrackedHandTransform(request.isLeft, core::TrackedHandKind::FirstPersonHand, target)) {
                    return;
                }
                target.translate += request.unreachable ? RE::NiPoint3(0, 0, 300) : request.offset;
                core::setHandWorldTransform(PROBE_TAG, request.isLeft, target, 100);
                g_probe.lastTarget = target;
            }
            g_probe.lastClaim = request;
            g_probe.lastClaimFrame = g_probe.frame;
            g_probe.claimRecords = {};
        }

        void __cdecl probeCallback(const std::uint32_t phase, void*) noexcept
        {
            if (phase >= FRAME_PHASE_COUNT) {
                return;
            }
            ++g_probe.counts[phase];

            // NativeGraphOutput opens a frame's order list (FrameBegin follows it, then 1..8)
            if (g_probe.orderCurrentCount > 0 && phase == static_cast<std::uint32_t>(FramePhase::NativeGraphOutput)) {
                g_probe.orderLast = g_probe.orderCurrent;
                g_probe.orderLastCount = g_probe.orderCurrentCount;
                g_probe.orderCurrentCount = 0;
            }
            if (g_probe.orderCurrentCount < g_probe.orderCurrent.size()) {
                g_probe.orderCurrent[g_probe.orderCurrentCount++] = static_cast<std::uint8_t>(phase);
            }

            if (g_probe.pending && g_probe.pending->phase == phase) {
                const auto request = *g_probe.pending;
                g_probe.pending.reset();
                executeClaim(request);
            }

            if (phase == static_cast<std::uint32_t>(FramePhase::AfterArmSolve)) {
                for (const bool isLeft : { false, true }) {
                    core::getTrackedHandTransform(isLeft, core::TrackedHandKind::FirstPersonHand, g_probe.afterArmSolveHand[isLeft ? 1 : 0]);
                }
                if (const auto weapon = f4vr::getWeaponNode(); weapon && g_probe.carryPending) {
                    g_probe.carryStaging.weaponAfterArmSolve = weapon->world;
                    handNodeWorld(true, g_probe.carryStaging.leftHandAfterArmSolve);
                    g_probe.carryStaging.leftRevisionAfterCallbacks = g_externalAuthority.getHandClaimRevision(true);
                    weaponFrames(g_probe.carryStaging.localAfterArmSolve, g_probe.carryStaging.parentAfterArmSolve);
                }
            }

            if (phase == static_cast<std::uint32_t>(FramePhase::BeforeArmSolve) && g_frik.isWeaponInLeftHand()) {
                auto& carry = g_probe.carryStaging;
                if (const auto weapon = f4vr::getWeaponNode()) {
                    carry.weaponBeforeArmSolve = weapon->world;
                }
                weaponFrames(carry.localBeforeArmSolve, carry.parentBeforeArmSolve);
                carry.localPrevFinal = g_probe.carry.localWorldFinal;
                carry.parentPrevFinal = g_probe.carry.parentWorldFinal;
            }

            if (phase == static_cast<std::uint32_t>(FramePhase::AfterWorldFinal) && g_probe.carryPending) {
                g_probe.carryPending = false;
                auto& carry = g_probe.carryStaging;
                carry.frame = g_probe.frame;
                if (const auto weapon = f4vr::getWeaponNode()) {
                    carry.weaponAfterWorldFinal = weapon->world;
                }
                handNodeWorld(true, carry.leftHandWorldFinal);
                weaponFrames(carry.localWorldFinal, carry.parentWorldFinal);
                carry.rightClaimed = g_externalAuthority.getHandWorldTransform(false, carry.rightClaim);
                carry.rightState = core::getHandSolveResult(false, carry.rightWrist);
                core::getBoneWorldTransform("RArm_Hand", &carry.rightBone);
                g_probe.carryMaxWeaponShift =
                    (std::max)(g_probe.carryMaxWeaponShift, common::MatrixUtils::vec3Len(carry.weaponAfterWorldFinal.translate - carry.weaponBeforeCallbacks.translate));
                g_probe.carry = carry;
                ++g_probe.carryFrames;
            }

            if (phase == static_cast<std::uint32_t>(FramePhase::AfterWorldFinal) && g_probe.armRecording) {
                if (g_probe.armRecord.size() >= g_probe.armRecordTarget) {
                    g_probe.armRecording = false;
                } else {
                    ArmSample sample;
                    sample.frame = g_probe.frame;
                    for (const bool isLeft : { false, true }) {
                        const auto side = isLeft ? 1 : 0;
                        for (std::size_t b = 0; b < ARM_BONE_SUFFIXES.size(); ++b) {
                            const std::string name = std::string(isLeft ? "LArm_" : "RArm_") + ARM_BONE_SUFFIXES[b];
                            sample.valid[side * ARM_SAMPLE_SLOTS + b] = core::getBoneWorldTransform(name.c_str(), &sample.slots[side][b]) ? 1 : 0;
                        }
                        const std::size_t wandSlot = ARM_BONE_SUFFIXES.size();
                        sample.valid[side * ARM_SAMPLE_SLOTS + wandSlot] = core::getTrackedHandTransform(isLeft, core::TrackedHandKind::Wand, sample.slots[side][wandSlot]) ? 1 : 0;
                        RE::NiTransform wrist;
                        sample.state[side] = static_cast<std::uint8_t>(core::getHandSolveResult(isLeft, wrist));
                    }
                    g_probe.armRecord.push_back(sample);
                }
            }

            if (phase == static_cast<std::uint32_t>(FramePhase::FrameEnd)) {
                // grip held on a hand plus an edge on A (yes), B / menu (no) or trigger (repeat); the same hand acknowledges with a haptic
                for (const auto hand : { vrcf::Hand::Right, vrcf::Hand::Left }) {
                    if (!vrcf::VRControllers.isPressHeldDown(hand, vr::k_EButton_Grip)) {
                        continue;
                    }
                    const char* chord = nullptr;
                    auto pattern = vrcf::HapticPattern::Click;
                    if (vrcf::VRControllers.isPressed(hand, vr::k_EButton_A)) {
                        chord = "yes";
                    } else if (vrcf::VRControllers.isPressed(hand, vr::k_EButton_ApplicationMenu)) {
                        chord = "no";
                        pattern = vrcf::HapticPattern::DoubleClick;
                    } else if (vrcf::VRControllers.isPressed(hand, vr::k_EButton_SteamVR_Trigger)) {
                        chord = "repeat";
                        pattern = vrcf::HapticPattern::TripleClick;
                    }
                    if (chord) {
                        ++g_probe.chordSeq;
                        g_probe.chordLast = chord;
                        g_probe.chordFrame = g_probe.frame;
                        vrcf::VRHaptics.trigger(hand, pattern);
                        break;
                    }
                }
            }

            if (phase == static_cast<std::uint32_t>(FramePhase::AfterWorldFinal)) {
                auto& record = g_probe.records[g_probe.recordNext];
                g_probe.recordNext = (g_probe.recordNext + 1) % g_probe.records.size();
                record.frame = g_probe.frame;
                for (const bool isLeft : { false, true }) {
                    record.state[isLeft ? 1 : 0] = core::getHandSolveResult(isLeft, record.wrist[isLeft ? 1 : 0]);
                }
                if (g_probe.lastClaim && g_probe.frame >= g_probe.lastClaimFrame && g_probe.frame <= g_probe.lastClaimFrame + 1) {
                    g_probe.claimRecords[g_probe.frame - g_probe.lastClaimFrame] = record;
                }
                ++g_probe.frame;
            }
        }

        // runs first in AfterArmSolve, to latch what the other callbacks of that phase read
        void __cdecl probeEarlyCallback(const std::uint32_t, void*) noexcept
        {
            const auto weapon = f4vr::getWeaponNode();
            g_probe.carryPending = weapon && g_frik.isWeaponInLeftHand();
            if (!g_probe.carryPending) {
                return;
            }
            auto& carry = g_probe.carryStaging;
            carry.weaponBeforeCallbacks = weapon->world;
            core::getBoneWorldTransform("RArm_Hand", &carry.rightBoneBeforeCallbacks);
            handNodeWorld(false, carry.rightHandNodeBeforeCallbacks);
            handNodeWorld(true, carry.leftHandBeforeCallbacks);
            carry.leftRevisionBeforeCallbacks = g_externalAuthority.getHandClaimRevision(true);
            weaponFrames(carry.localBeforeCallbacks, carry.parentBeforeCallbacks);
        }

        bool ensureRegistered()
        {
            if (g_probe.registered) {
                return true;
            }
            if (!core::registerFrameCallback(PROBE_EARLY_TAG, static_cast<std::uint32_t>(FramePhase::AfterArmSolve), &probeEarlyCallback, nullptr, 10000)) {
                return false;
            }
            for (std::uint32_t phase = 0; phase < FRAME_PHASE_COUNT; ++phase) {
                if (!core::registerFrameCallback(PROBE_TAG, phase, &probeCallback, nullptr, 0)) {
                    return false;
                }
            }
            g_probe.registered = true;
            return true;
        }

        bool parseHand(const json& args, bool& outIsLeft)
        {
            const auto hand = args.value("hand", "right");
            outIsLeft = hand == "left";
            return hand == "left" || hand == "right";
        }

        json recordJson(const FrameRecord& r)
        {
            return { { "frame", r.frame },
                { "right", { { "state", stateName(r.state[0]) }, { "wrist", transformJson(r.wrist[0]) }, { "toTarget", diffJson(r.wrist[0], g_probe.lastTarget) } } },
                { "left", { { "state", stateName(r.state[1]) }, { "wrist", transformJson(r.wrist[1]) }, { "toTarget", diffJson(r.wrist[1], g_probe.lastTarget) } } } };
        }
    }

    std::string runProbe(const std::string& argsJson)
    {
        json args;
        try {
            args = argsJson.empty() ? json::object() : json::parse(argsJson);
        } catch (const std::exception& ex) {
            return json{ { "ok", false }, { "error", std::string("bad arguments JSON: ") + ex.what() } }.dump();
        }
        const auto op = args.value("op", "phases");

        if (op == "reset") {
            core::unregisterFrameCallback(PROBE_TAG);
            core::unregisterFrameCallback(PROBE_EARLY_TAG);
            core::clearHandWorldTransform(PROBE_TAG, false);
            core::clearHandWorldTransform(PROBE_TAG, true);
            core::setOffHandGripping(PROBE_TAG, false, false, nullptr);
            core::clearWeaponNodeParentHand(PROBE_TAG);
            core::clearScopeProvider(PROBE_TAG);
            core::blockPrimaryWeaponNodeOwnership(PROBE_TAG, false);
            g_probe = {};
            return json{ { "ok", true } }.dump();
        }

        if (!g_frik.isSkeletonReady()) {
            return json{ { "ok", false }, { "error", "skeleton not ready" } }.dump();
        }
        if (!ensureRegistered()) {
            return json{ { "ok", false }, { "error", "registerFrameCallback failed" } }.dump();
        }

        if (op == "phases") {
            json order = json::array();
            for (std::size_t i = 0; i < g_probe.orderLastCount; ++i) {
                order.push_back(g_probe.orderLast[i]);
            }
            return json{ { "ok", true }, { "frames", g_probe.frame }, { "counts", g_probe.counts }, { "lastFrameOrder", order }, { "generation", g_frik.getSkeletonGeneration() } }
                .dump();
        }

        if (op == "claim") {
            ClaimRequest request;
            if (!parseHand(args, request.isLeft)) {
                return json{ { "ok", false }, { "error", "hand must be left or right" } }.dump();
            }
            const auto kind = args.value("kind", "offset");
            request.clear = kind == "clear";
            request.unreachable = kind == "unreachable";
            if (args.contains("offset") && args["offset"].is_array() && args["offset"].size() == 3) {
                request.offset = { args["offset"][0].get<float>(), args["offset"][1].get<float>(), args["offset"][2].get<float>() };
            }
            const auto phase = args.value("phase", -1);
            request.phase = phase < 0 ? PHASE_NOW : static_cast<std::uint32_t>(phase);
            if (request.phase == PHASE_NOW) {
                executeClaim(request);
                return json{ { "ok", true }, { "publishedFrame", g_probe.lastClaimFrame }, { "when", "before this frame's skeleton pass" } }.dump();
            }
            if (request.phase >= FRAME_PHASE_COUNT) {
                return json{ { "ok", false }, { "error", "phase out of range" } }.dump();
            }
            g_probe.pending = request;
            return json{ { "ok", true }, { "queuedForPhase", request.phase }, { "frameNow", g_probe.frame } }.dump();
        }

        if (op == "solve") {
            json records = json::array();
            for (std::size_t i = 0; i < g_probe.records.size(); ++i) {
                const auto& r = g_probe.records[(g_probe.recordNext + i) % g_probe.records.size()];
                if (r.frame > 0 || i == g_probe.records.size() - 1) {
                    records.push_back(recordJson(r));
                }
            }
            json now;
            for (const bool isLeft : { false, true }) {
                RE::NiTransform wrist;
                const auto state = core::getHandSolveResult(isLeft, wrist);
                now[isLeft ? "left" : "right"] = { { "state", stateName(state) }, { "wrist", transformJson(wrist) } };
            }
            return json{
                { "ok", true },
                { "claimFrame", g_probe.lastClaim ? json(g_probe.lastClaimFrame) : json(nullptr) },
                { "claimPhase", g_probe.lastClaim ? json(g_probe.lastClaim->phase) : json(nullptr) },
                { "pending", g_probe.pending.has_value() },
                { "target", transformJson(g_probe.lastTarget) },
                { "records", records },
                { "claimRecords", { recordJson(g_probe.claimRecords[0]), recordJson(g_probe.claimRecords[1]) } },
                { "now", now }
            }.dump();
        }

        if (op == "chain") {
            bool isLeft = false;
            if (!parseHand(args, isLeft)) {
                return json{ { "ok", false }, { "error", "hand must be left or right" } }.dump();
            }
            core::ArmChainTransforms chain{};
            core::getArmChain(isLeft, chain);
            RE::NiTransform bone;
            const bool boneOk = core::getBoneWorldTransform(isLeft ? "LArm_Hand" : "RArm_Hand", &bone);
            RE::NiTransform tracked[3];
            json trackedJson;
            const char* kinds[] = { "wand", "weaponOffset", "firstPersonHand" };
            for (int k = 0; k < 3; ++k) {
                if (core::getTrackedHandTransform(isLeft, static_cast<core::TrackedHandKind>(k), tracked[k])) {
                    trackedJson[kinds[k]] = transformJson(tracked[k]);
                }
            }
            return json{
                { "ok", true },
                { "validMask", chain.validMask },
                { "shoulder", transformJson(chain.shoulder) },
                { "forearm1", transformJson(chain.forearm1) },
                { "hand", transformJson(chain.hand) },
                { "boneHand", boneOk ? transformJson(bone) : json(nullptr) },
                { "chainVsBone", boneOk ? diffJson(chain.hand, bone) : json(nullptr) },
                { "tracked", trackedJson },
                { "firstPersonHandAfterArmSolve", transformJson(g_probe.afterArmSolveHand[isLeft ? 1 : 0]) }
            }.dump();
        }

        if (op == "record") {
            // start recording both arms for `frames` frames (replaces any previous recording)
            const auto frames = static_cast<std::size_t>(std::clamp(args.value("frames", 900), 1, static_cast<int>(ARM_RECORD_MAX_FRAMES)));
            g_probe.armRecord.clear();
            g_probe.armRecord.reserve(frames);
            g_probe.armRecordTarget = frames;
            g_probe.armRecording = true;
            return json{ { "ok", true }, { "frames", frames } }.dump();
        }

        if (op == "recordStop") {
            g_probe.armRecording = false;
            return json{ { "ok", true }, { "recorded", g_probe.armRecord.size() } }.dump();
        }

        if (op == "recordDump") {
            // frames [from, from+count) as compact rows: per hand, per slot [px,py,pz, r00..r22] (null when the bone was missing)
            const auto from = static_cast<std::size_t>((std::max)(args.value("from", 0), 0));
            const auto count = static_cast<std::size_t>(std::clamp(args.value("count", 300), 1, 1000));
            json rows = json::array();
            for (std::size_t i = from; i < g_probe.armRecord.size() && i < from + count; ++i) {
                const auto& s = g_probe.armRecord[i];
                json hands = json::array();
                for (int side = 0; side < 2; ++side) {
                    json slots = json::array();
                    for (std::size_t k = 0; k < ARM_SAMPLE_SLOTS; ++k) {
                        if (!s.valid[side * ARM_SAMPLE_SLOTS + k]) {
                            slots.push_back(nullptr);
                            continue;
                        }
                        const auto& t = s.slots[side][k];
                        slots.push_back({ t.translate.x,
                            t.translate.y,
                            t.translate.z,
                            t.rotate.entry[0][0],
                            t.rotate.entry[0][1],
                            t.rotate.entry[0][2],
                            t.rotate.entry[1][0],
                            t.rotate.entry[1][1],
                            t.rotate.entry[1][2],
                            t.rotate.entry[2][0],
                            t.rotate.entry[2][1],
                            t.rotate.entry[2][2] });
                    }
                    hands.push_back({ { "state", stateName(static_cast<core::HandSolveState>(s.state[side])) }, { "slots", slots } });
                }
                rows.push_back({ { "frame", s.frame }, { "hands", hands } });
            }
            return json{
                { "ok", true },
                { "recording", g_probe.armRecording },
                { "total", g_probe.armRecord.size() },
                { "from", from },
                { "slotNames", { "Collarbone", "UpperArm", "UpperTwist1", "ForeArm1", "ForeArm2", "ForeArm3", "Hand", "Wand" } },
                { "handOrder", { "right", "left" } },
                { "rows", rows }
            }.dump();
        }

        if (op == "chord") {
            // the last controller chord (grip + A yes / B no / trigger repeat); `since` = the seq the caller already saw
            const auto since = static_cast<std::uint64_t>((std::max)(args.value("since", 0), 0));
            return json{ { "ok", true }, { "seq", g_probe.chordSeq }, { "fresh", g_probe.chordSeq > since }, { "chord", g_probe.chordLast }, { "frame", g_probe.chordFrame } }
                .dump();
        }

        if (op == "bone") {
            // one bone by name: its scene-graph node (parent, local) and its flattened-tree entry (world), to check who owns what
            const auto name = args.value("name", "RArm_UpperTwist1");
            json out{ { "ok", true }, { "name", name } };
            RE::NiTransform world;
            out["treeWorld"] = core::getBoneWorldTransform(name.c_str(), &world) ? transformJson(world) : json(nullptr);
            if (const auto* skeleton = g_frik.getSkeleton(); skeleton) {
                if (auto* node = f4vr::findAVObject(f4vr::getCommonNode(), name.c_str())) {
                    out["node"] = { { "parent", node->parent ? node->parent->name.c_str() : "" },
                        { "local", transformJson(node->local) },
                        { "world", transformJson(node->world) } };
                } else {
                    out["node"] = nullptr;
                }
            }
            return out.dump();
        }

        if (op == "visibility") {
            // raw NiAVObject flags of the player body root, FRIK's skeleton root and the first-person skeleton, for the "no body after a scope exit" hunt
            json out{ { "ok", true } };
            auto flagsOf = [](const RE::NiAVObject* node) -> json {
                if (!node) {
                    return nullptr;
                }
                return { { "name", node->name.c_str() },
                    { "flags", static_cast<std::uint64_t>(node->flags.flags) },
                    { "scale", node->local.scale },
                    { "worldZ", node->world.translate.z } };
            };
            const auto player = f4vr::getPlayer();
            const RE::NiAVObject* body = player && player->loadedData ? player->loadedData->data3D.get() : nullptr;
            out["body"] = flagsOf(body);
            out["root"] = flagsOf(f4vr::getRootNode());
            out["firstPerson"] = flagsOf(f4vr::getFirstPersonSkeleton());
            out["common"] = flagsOf(f4vr::getCommonNode());
            out["hideBodyInScope"] = g_frik.shouldHideBodyInScope();
            out["lookingThrough"] = g_frik.isLookingThroughScope();
            out["inScopeMenu"] = g_frik.isInScopeMenu();
            return out.dump();
        }

        if (op == "selfie") {
            g_frik.setSelfieMode(args.value("on", true));
            return json{ { "ok", true }, { "selfie", g_frik.isSelfieModeOn() } }.dump();
        }

        if (op == "pipboy") {
            // opens/closes FRIK's Pip-Boy the way the button does; the engine pushes PipboyMenu with it (verified headless), so a
            // run with nobody in the headset can hold a blocking menu open. Override PipBoyCloseWhenLookAway first or it closes again.
            if (args.value("on", true)) {
                g_frik.openPipboy();
            } else {
                g_frik.closePipboy();
            }
            return json{ { "ok", true }, { "pipboyOn", g_frik.isPipboyOn() } }.dump();
        }

        if (op == "grip") {
            bool isLeft = false;
            parseHand(args, isLeft);
            const bool on = args.value("on", true);
            const bool ok = core::setOffHandGripping(PROBE_TAG, on, isLeft, nullptr);
            return json{ { "ok", ok }, { "gripping", g_frik.isOffHandGrippingWeapon() } }.dump();
        }

        if (op == "parent") {
            const auto hand = args.value("hand", "clear");
            const bool ok = hand == "clear" ? core::clearWeaponNodeParentHand(PROBE_TAG) : core::setWeaponNodeParentHand(PROBE_TAG, hand == "left");
            return json{ { "ok", ok }, { "weaponInLeftHand", g_frik.isWeaponInLeftHand() } }.dump();
        }

        if (op == "scope") {
            const bool on = args.value("on", true);
            // takeover: drop True Scopes' registration for the test so the no-provider (culling) path runs; restored on off
            const bool takeover = args.value("takeover", false);
            constexpr auto TRUE_SCOPES_TAG = "TrueScopes";
            constexpr std::uint32_t TRUE_SCOPES_CAPABILITIES = 0x5;
            bool ok = true;
            if (on) {
                if (takeover) {
                    core::clearScopeProvider(TRUE_SCOPES_TAG);
                }
                ok = core::setScopeProvider(PROBE_TAG, static_cast<std::uint32_t>(ScopeCapability::PublishesLookingThrough)) && core::setLookingThroughScope(PROBE_TAG, true);
            } else {
                ok = core::clearScopeProvider(PROBE_TAG);
                if (takeover) {
                    core::setScopeProvider(TRUE_SCOPES_TAG, TRUE_SCOPES_CAPABILITIES);
                }
            }
            return json{ { "ok", ok }, { "lookingThroughScope", g_frik.isLookingThroughScope() }, { "hideBody", g_frik.shouldHideBodyInScope() } }.dump();
        }

        if (op == "block") {
            const bool on = args.value("on", true);
            const bool ok = core::blockPrimaryWeaponNodeOwnership(PROBE_TAG, on);
            return json{ { "ok", ok }, { "blocked", g_externalAuthority.isPrimaryWeaponNodeOwnershipBlocked() } }.dump();
        }

        if (op == "nodes") {
            const auto weapon = f4vr::getWeaponNode();
            const auto root = f4vr::getRootNode();
            RE::NiTransform spine;
            const bool spineOk = core::getBoneWorldTransform("SPINE2", &spine);
            // a node's world plus its parent chain, so a probe can tell which hand the engine's scope rig hangs on
            const auto nodeJson = [](const RE::NiAVObject* node) -> json {
                if (!node) {
                    return nullptr;
                }
                json chain = json::array();
                for (auto p = node->parent; p && chain.size() < 6; p = p->parent) {
                    chain.push_back(p->name.c_str());
                }
                const auto rows = [](const RE::NiMatrix3& m) {
                    json out = json::array();
                    for (int r = 0; r < 3; ++r) {
                        out.push_back({ m.entry[r][0], m.entry[r][1], m.entry[r][2] });
                    }
                    return out;
                };
                return { { "world", transformJson(node->world) }, { "local", transformJson(node->local) }, { "worldRot", rows(node->world.rotate) }, { "parents", chain } };
            };
            const auto pn = f4vr::getPlayerNodes();
            const auto fp = f4vr::getFirstPersonSkeleton();
            return json{
                { "ok", true },
                { "weapon", nodeJson(weapon) },
                { "scopeParent", nodeJson(pn ? pn->ScopeParentNode : nullptr) },
                { "scopeCamera", nodeJson(pn ? pn->primaryWeaponScopeCamera : nullptr) },
                // the scope shape a scope mod places its widget on, wherever it hangs (it should be a descendant of the Weapon node)
                { "scopeShape", nodeJson(weapon ? f4vr::findAVObjectStartsWith(weapon, "P-Scope") : nullptr) },
                { "scopeShapeUnderRoot", nodeJson(root && !(weapon && f4vr::findAVObjectStartsWith(weapon, "P-Scope")) ? f4vr::findAVObjectStartsWith(root, "P-Scope") : nullptr) },
                { "rHand", nodeJson(fp ? f4vr::findNode(fp, "RArm_Hand") : nullptr) },
                { "lHand", nodeJson(fp ? f4vr::findNode(fp, "LArm_Hand") : nullptr) },
                // ScopeParent's world as the engine would compose it from its live parent and its own local, and the gap to the world it actually
                // carries: a persistent gap means someone wrote the world directly after the local was set
                { "scopeParentComposed",
                    pn && pn->ScopeParentNode && pn->ScopeParentNode->parent ? transformJson(composeWorld(pn->ScopeParentNode->parent->world, pn->ScopeParentNode->local))
                                                                             : json(nullptr) },
                { "scopeParentGap",
                    pn && pn->ScopeParentNode && pn->ScopeParentNode->parent
                        ? json(common::MatrixUtils::vec3Len(
                              composeWorld(pn->ScopeParentNode->parent->world, pn->ScopeParentNode->local).translate - pn->ScopeParentNode->world.translate))
                        : json(nullptr) },
                // ScopeParent's subtree: names, worlds, scales and the app-culled flag of every child (a scope mod's widget lives here)
                { "scopeParentChildren",
                    [&]() -> json {
                        json out = json::array();
                        const RE::NiNode* sp = pn ? pn->ScopeParentNode : nullptr;
                        if (!sp) {
                            return out;
                        }
                        for (const auto& child : sp->children) {
                            if (!child) {
                                continue;
                            }
                            out.push_back({ { "name", child->name.c_str() },
                                { "world", transformJson(child->world) },
                                { "localScale", child->local.scale },
                                { "appCulled", child->GetAppCulled() },
                                { "flags", static_cast<std::uint64_t>(child->flags.flags) } });
                            if (out.size() >= 16) {
                                break;
                            }
                        }
                        return out;
                    }() },
                { "scopeParentAppCulled", pn && pn->ScopeParentNode ? json(pn->ScopeParentNode->GetAppCulled()) : json(nullptr) },
                { "weaponInLeftHand", g_frik.isWeaponInLeftHand() },
                { "weaponLocal", weapon ? transformJson(weapon->local) : json(nullptr) },
                { "weaponWorld", weapon ? transformJson(weapon->world) : json(nullptr) },
                { "rootScale", root ? root->local.scale : 0.0f },
                { "spine2", spineOk ? transformJson(spine) : json(nullptr) },
                { "camera", { f4vr::getCameraPosition().x, f4vr::getCameraPosition().y, f4vr::getCameraPosition().z } }
            }.dump();
        }

        if (op == "carry") {
            const auto& carry = g_probe.carry;
            return json{
                { "ok", true },
                { "carryFrames", g_probe.carryFrames },
                { "lastCarryFrame", carry.frame },
                { "now", g_probe.frame },
                { "weaponBeforeCallbacksToAfterArmSolve", diffJson(carry.weaponBeforeCallbacks, carry.weaponAfterArmSolve) },
                { "weaponAfterArmSolveToWorldFinal", diffJson(carry.weaponAfterArmSolve, carry.weaponAfterWorldFinal) },
                { "weaponBeforeCallbacksToWorldFinal", diffJson(carry.weaponBeforeCallbacks, carry.weaponAfterWorldFinal) },
                { "rightBoneBeforeCallbacksToWorldFinal", diffJson(carry.rightBoneBeforeCallbacks, carry.rightBone) },
                { "rightHandNodeToBoneBeforeCallbacks", diffJson(carry.rightHandNodeBeforeCallbacks, carry.rightBoneBeforeCallbacks) },
                { "leftHandBeforeCallbacksToAfterArmSolve", diffJson(carry.leftHandBeforeCallbacks, carry.leftHandAfterArmSolve) },
                { "leftHandAfterArmSolveToWorldFinal", diffJson(carry.leftHandAfterArmSolve, carry.leftHandWorldFinal) },
                { "leftRevisionChangedInCallbacks", carry.leftRevisionAfterCallbacks != carry.leftRevisionBeforeCallbacks },
                { "localPrevFinalToBeforeArmSolve", diffJson(carry.localPrevFinal, carry.localBeforeArmSolve) },
                { "parentPrevFinalToBeforeArmSolve", diffJson(carry.parentPrevFinal, carry.parentBeforeArmSolve) },
                { "localBeforeArmSolveToBeforeCallbacks", diffJson(carry.localBeforeArmSolve, carry.localBeforeCallbacks) },
                { "parentBeforeArmSolveToBeforeCallbacks", diffJson(carry.parentBeforeArmSolve, carry.parentBeforeCallbacks) },
                { "weaponBeforeArmSolveToBeforeCallbacks", diffJson(carry.weaponBeforeArmSolve, carry.weaponBeforeCallbacks) },
                { "localBeforeCallbacksToAfterArmSolve", diffJson(carry.localBeforeCallbacks, carry.localAfterArmSolve) },
                { "parentBeforeCallbacksToAfterArmSolve", diffJson(carry.parentBeforeCallbacks, carry.parentAfterArmSolve) },
                { "localAfterArmSolveToWorldFinal", diffJson(carry.localAfterArmSolve, carry.localWorldFinal) },
                { "parentAfterArmSolveToWorldFinal", diffJson(carry.parentAfterArmSolve, carry.parentWorldFinal) },
                { "weaponBeforeArmSolveVsComposed", diffJson(carry.weaponBeforeArmSolve, composeWorld(carry.parentBeforeArmSolve, carry.localBeforeArmSolve)) },
                { "weaponBeforeCallbacksVsComposed", diffJson(carry.weaponBeforeCallbacks, composeWorld(carry.parentBeforeCallbacks, carry.localBeforeCallbacks)) },
                { "weaponFinalVsComposed", diffJson(carry.weaponAfterWorldFinal, composeWorld(carry.parentWorldFinal, carry.localWorldFinal)) },
                { "maxWeaponShift", g_probe.carryMaxWeaponShift },
                { "rightClaimed", carry.rightClaimed },
                { "rightState", stateName(carry.rightState) },
                { "rightWristToClaim", carry.rightClaimed ? diffJson(carry.rightWrist, carry.rightClaim) : json(nullptr) },
                { "rightBoneToClaim", carry.rightClaimed ? diffJson(carry.rightBone, carry.rightClaim) : json(nullptr) },
                { "rightBoneToWrist", diffJson(carry.rightBone, carry.rightWrist) },
                { "weaponWorldFinal", transformJson(carry.weaponAfterWorldFinal) },
                { "rightClaim", transformJson(carry.rightClaim) }
            }.dump();
        }

        return json{ { "ok", false }, { "error", "unknown op (phases|claim|solve|chain|grip|parent|scope|block|nodes|carry|reset)" } }.dump();
    }
}
