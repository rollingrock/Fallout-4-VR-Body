#include "devbench/DevBenchProbe.h"

#include <array>
#include <cmath>
#include <optional>

#include <nlohmann/json.hpp>

#include "ExternalAuthority.h"
#include "FRIK.h"
#include "ScopeAuthority.h"
#include "api/ApiCore.h"
#include "common/MatrixUtils.h"
#include "f4vr/F4VRUtils.h"

namespace frik::devbench
{
    namespace
    {
        namespace core = api::core;
        using nlohmann::json;

        constexpr auto PROBE_TAG = "frik.probe";
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

        struct ProbeState
        {
            bool registered = false;
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
        }

        void __cdecl probeCallback(const std::uint32_t phase, void*) noexcept
        {
            if (phase >= FRAME_PHASE_COUNT) {
                return;
            }
            ++g_probe.counts[phase];

            // a phase index not above the previous one starts a new frame's order list
            if (g_probe.orderCurrentCount > 0 && phase <= g_probe.orderCurrent[g_probe.orderCurrentCount - 1]) {
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

            if (phase == static_cast<std::uint32_t>(FramePhase::AfterWorldFinal)) {
                auto& record = g_probe.records[g_probe.recordNext];
                g_probe.recordNext = (g_probe.recordNext + 1) % g_probe.records.size();
                record.frame = g_probe.frame;
                for (const bool isLeft : { false, true }) {
                    record.state[isLeft ? 1 : 0] = core::getHandSolveResult(isLeft, record.wrist[isLeft ? 1 : 0]);
                }
                ++g_probe.frame;
            }
        }

        bool ensureRegistered()
        {
            if (g_probe.registered) {
                return true;
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
                { "tracked", trackedJson }
            }.dump();
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
            bool ok = true;
            if (on) {
                ok = core::setScopeProvider(PROBE_TAG, static_cast<std::uint32_t>(ScopeCapability::PublishesLookingThrough)) && core::setLookingThroughScope(PROBE_TAG, true);
            } else {
                ok = core::clearScopeProvider(PROBE_TAG);
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
            return json{
                { "ok", true },
                { "weaponLocal", weapon ? transformJson(weapon->local) : json(nullptr) },
                { "weaponWorld", weapon ? transformJson(weapon->world) : json(nullptr) },
                { "rootScale", root ? root->local.scale : 0.0f },
                { "spine2", spineOk ? transformJson(spine) : json(nullptr) },
                { "camera", { f4vr::getCameraPosition().x, f4vr::getCameraPosition().y, f4vr::getCameraPosition().z } }
            }.dump();
        }

        return json{ { "ok", false }, { "error", "unknown op (phases|claim|solve|chain|grip|parent|scope|block|nodes|reset)" } }.dump();
    }
}
