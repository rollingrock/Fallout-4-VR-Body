#include "HandPoseMath.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string_view>

#include "common/CommonUtils.h"
#include "common/MatrixUtils.h"
#include "common/Quaternion.h"
#include "f4vr/F4VRUtils.h"
#include "utils.h"

using namespace common;
using namespace frik::skeleton::data;

namespace
{
    /**
     * Copy the authored 3x4 rotation rows from pose data into a runtime transform.
     */
    void copyRotationIntoTransform(const RotationData& rotationData, RE::NiTransform& transform)
    {
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 4; col++) {
                transform.rotate.entry[row][col] = rotationData[row * 4 + col];
            }
        }
    }

    /**
     * The authored open/closed reference for a single hand bone.
     *
     * The rotations are the same for every skeleton; only the rest translation differs,
     * between the normal and the power-armor hand.
     */
    struct AuthoredHandBone
    {
        RE::NiMatrix3 openRotation;
        RE::NiMatrix3 closedRotation;
        RE::NiPoint3 openTranslation;
        RE::NiPoint3 openTranslationInPowerArmor;

        const RE::NiPoint3& restTranslation(const bool inPowerArmor) const
        {
            return inPowerArmor ? openTranslationInPowerArmor : openTranslation;
        }
    };

    /**
     * The authored hand bone reference data by bone name, built once from the static pose data.
     */
    const std::map<std::string, AuthoredHandBone>& authoredHandBones()
    {
        static const auto bones = [] {
            std::map<std::string, AuthoredHandBone> built;
            for (const auto& boneData : getHandBoneData()) {
                RE::NiTransform open{};
                RE::NiTransform closed{};
                copyRotationIntoTransform(boneData.openRotation, open);
                copyRotationIntoTransform(boneData.closedRotation, closed);
                built.emplace(boneData.boneName,
                    AuthoredHandBone{
                        .openRotation = open.rotate,
                        .closedRotation = closed.rotate,
                        .openTranslation = boneData.openTranslation,
                        .openTranslationInPowerArmor = boneData.openTranslationInPowerArmor,
                    });
            }
            return built;
        }();
        return bones;
    }

    /**
     * Build the local transform one bone would take under an authored pose.
     *
     * Interpolates the authored closed rotation toward the open one by the bone's
     * flex value, then applies proximal splay on the first joint of each finger,
     * mirrored per hand. Translation comes from the open pose, in the power-armor
     * variant when applicable, since posing never moves a bone off its rest offset.
     */
    RE::NiTransform buildPoseBoneLocalTransform(const HandBonePoseData& boneData, const frik::HandFingersPose& pose, const bool inPowerArmor)
    {
        const auto& authored = authoredHandBones().at(boneData.boneName);

        Quaternion qOpen;
        Quaternion qClosed;
        qOpen.fromMatrix(authored.openRotation);
        qClosed.fromMatrix(authored.closedRotation);
        qClosed.slerp(std::clamp(pose.getFlexAt(frik::HandPoseMath::boneToFlexIndex(boneData.boneName)), -1.0f, 2.0f), qOpen);

        RE::NiTransform result{};
        result.translate = authored.restTranslation(inPowerArmor);
        result.rotate = qClosed.getMatrix();
        if (std::string_view(boneData.boneName).back() == '1') {
            const float sign = boneData.boneName[0] == 'L' ? -1.0f : 1.0f;
            result.rotate =
                MatrixUtils::getMatrixFromEulerAngles(0.0f, sign * pose.getFingerAt(frik::HandPoseMath::boneToFingerIndex(boneData.boneName)).splay, 0.0f) * result.rotate;
        }
        return result;
    }
}

namespace frik
{
    // -- Bone name vocabulary -----------------------------------------------------------

    bool HandPoseMath::isLeftHandBone(const std::string& boneName)
    {
        return boneName[0] == 'L';
    }

    int HandPoseMath::boneToFingerIndex(const std::string& boneName)
    {
        return boneName[boneName.size() - 2] - '1';
    }

    int HandPoseMath::boneToFlexIndex(const std::string& boneName)
    {
        return boneToFingerIndex(boneName) * 3 + (boneName.back() - '1');
    }

    // -- Pose to rotation ---------------------------------------------------------------

    std::map<std::string, RE::NiTransform> HandPoseMath::buildOpenPoseTransforms(const bool inPowerArmor)
    {
        std::map<std::string, RE::NiTransform> openPose;
        for (const auto& [boneName, authored] : authoredHandBones()) {
            auto& open = openPose[boneName];
            open.rotate = authored.openRotation;
            open.translate = authored.restTranslation(inPowerArmor);
        }
        return openPose;
    }

    RE::NiMatrix3 HandPoseMath::blendBoneRotation(const std::string& boneName, const float flex, const float splay)
    {
        const auto& authored = authoredHandBones().at(boneName);
        Quaternion qOpen, qClosed;
        qOpen.fromMatrix(authored.openRotation);
        qClosed.fromMatrix(authored.closedRotation);
        qClosed.slerp(flex, qOpen);
        if (fEqual(splay, 0.0f)) {
            return qClosed.getMatrix();
        }
        const float sign = isLeftHandBone(boneName) ? -1.0f : 1.0f;
        return MatrixUtils::getMatrixFromEulerAngles(0, sign * splay, 0) * qClosed.getMatrix();
    }

    RE::NiMatrix3 HandPoseMath::getPoseBoneRotation(const std::string& boneName, const HandFingersPose& pose)
    {
        const int fingerIndex = boneToFingerIndex(boneName);
        const float flex = std::clamp(pose.getFlexAt(boneToFlexIndex(boneName)), -1.0f, 2.0f);
        const float splay = boneName.back() == '1' ? pose.getFingerAt(fingerIndex).splay : 0.0f;
        return blendBoneRotation(boneName, flex, splay);
    }

    bool HandPoseMath::buildFingerLocalTransformsForPose(const bool isLeft, const HandFingersPose& pose, std::array<RE::NiTransform, FINGER_BONE_COUNT>& outTransforms,
        std::uint16_t& outEnabledMask)
    {
        outTransforms = {};
        outEnabledMask = 0;

        const bool inPowerArmor = f4vr::isInPowerArmor();
        std::array<RE::NiTransform, FINGER_BONE_COUNT> transforms{};
        std::uint16_t enabledMask = 0;
        for (const auto& boneData : getHandBoneData()) {
            if (isLeft != (boneData.boneName[0] == 'L')) {
                continue;
            }

            const int flatBoneIndex = boneToFlexIndex(boneData.boneName);
            if (flatBoneIndex < 0 || flatBoneIndex >= static_cast<int>(FINGER_BONE_COUNT)) {
                return false;
            }

            const auto transform = buildPoseBoneLocalTransform(boneData, pose, inPowerArmor);
            if (!isFiniteTransform(transform)) {
                return false;
            }

            transforms[static_cast<std::size_t>(flatBoneIndex)] = transform;
            enabledMask = static_cast<std::uint16_t>(enabledMask | (1U << flatBoneIndex));
        }

        if (enabledMask != FULL_LOCAL_TRANSFORM_MASK) {
            return false;
        }

        outTransforms = transforms;
        outEnabledMask = enabledMask;
        return true;
    }

    // -- Rotation back to pose ----------------------------------------------------------

    /**
     * Recover the flex value that would reproduce an observed bone rotation.
     *
     * The inverse of blendBoneRotation's slerp: it projects the closed-to-animated
     * rotation onto the closed-to-open arc and returns how far along that arc the
     * animation sits. Used to read a game-animated hand back into pose values.
     *
     * Returns the same -1..2 range blendBoneRotation accepts, so hyperextension and
     * overcurl past the authored poses survive a round trip. Degenerate bones whose
     * open and closed rotations nearly coincide report fully open.
     */
    float HandPoseMath::inverseBlendFlex(const std::string& boneName, const RE::NiMatrix3& animatedRotation)
    {
        const auto& authored = authoredHandBones().at(boneName);
        Quaternion qOpen, qClosed, qAnimated;
        qOpen.fromMatrix(authored.openRotation);
        qClosed.fromMatrix(authored.closedRotation);
        qAnimated.fromMatrix(animatedRotation);

        const auto relativeFrom = [](const Quaternion& from, Quaternion to) {
            if (from.dot(to) < 0.0f) {
                to *= -1.0f;
            }
            return from.conjugate() * to;
        };
        const auto axisAngle = [](const Quaternion& q, RE::NiPoint3& axis) {
            const float w = std::clamp(q.w, -1.0f, 1.0f);
            const float sinHalf = std::sqrt((std::max)(0.0f, 1.0f - w * w));
            if (sinHalf < 1e-6f) {
                axis = RE::NiPoint3(0, 0, 0);
                return 0.0f;
            }
            axis = RE::NiPoint3(q.x / sinHalf, q.y / sinHalf, q.z / sinHalf);
            return 2.0f * std::acos(w);
        };

        RE::NiPoint3 arcAxis;
        const float arcAngle = axisAngle(relativeFrom(qClosed, qOpen), arcAxis);
        constexpr float kMinArcAngle = 0.001f;
        if (arcAngle < kMinArcAngle) {
            return 1.0f;
        }

        RE::NiPoint3 animatedAxis;
        const float animatedAngle = axisAngle(relativeFrom(qClosed, qAnimated), animatedAxis);
        const float angleAlongArc = animatedAngle * MatrixUtils::vec3Dot(animatedAxis, arcAxis);
        return std::clamp(angleAlongArc / arcAngle, -1.0f, 2.0f);
    }

    /**
     * Decompose an animated bone rotation into the flex and splay that produced it.
     *
     * Flex is measured first, then whatever rotation remains after removing the
     * pure-flex result is read as lateral splay about the Y axis. Only the proximal
     * joint of each finger carries splay, so other bones report zero. Once splay is
     * known it is removed and flex is re-measured, since the first pass absorbed
     * some of the lateral rotation into its arc projection.
     *
     * Splay is returned in the hand's own convention (mirrored for the left hand).
     * A residual that is not finite or is implausibly large for an authored pose is
     * discarded rather than propagated.
     */
    void HandPoseMath::measureAnimatedFlexSplay(const std::string& sourceBoneName, const RE::NiMatrix3& animatedRotation, float& outFlex, float& outSplay)
    {
        outFlex = inverseBlendFlex(sourceBoneName, animatedRotation);
        outSplay = 0.0f;
        if (sourceBoneName.back() != '1') {
            return;
        }

        const RE::NiMatrix3 flexOnly = blendBoneRotation(sourceBoneName, outFlex, 0.0f);
        const RE::NiMatrix3 residual = animatedRotation * flexOnly.Transpose();
        Quaternion residualQ;
        residualQ.fromMatrix(residual);
        if (residualQ.w < 0.0f) {
            residualQ *= -1.0f;
        }
        const float w = std::clamp(residualQ.w, -1.0f, 1.0f);
        const float sinHalf = std::sqrt((std::max)(0.0f, 1.0f - w * w));
        if (sinHalf < 1e-6f) {
            return;
        }
        const float residualAngle = 2.0f * std::acos(w);
        const float splayCandidate = residualAngle * (residualQ.y / sinHalf);
        constexpr float kMaxAuthoredSplayRadians = 0.6f;
        if (!std::isfinite(splayCandidate) || std::fabs(splayCandidate) > kMaxAuthoredSplayRadians) {
            return;
        }
        const bool sourceIsLeft = isLeftHandBone(sourceBoneName);
        outSplay = sourceIsLeft ? -splayCandidate : splayCandidate;

        const RE::NiMatrix3 desplayed = MatrixUtils::getMatrixFromEulerAngles(0, sourceIsLeft ? outSplay : -outSplay, 0) * animatedRotation;
        outFlex = inverseBlendFlex(sourceBoneName, desplayed);
    }

    // -- Mirroring onto the opposite hand -----------------------------------------------

    /**
     * Mirror the thumb base rotation from one hand onto the other.
     *
     * The thumb's proximal joint rotates about an axis that is neither pure flex nor
     * pure splay, so measureAnimatedFlexSplay cannot describe it and the generic path
     * distorts the pose. Instead this builds an orthonormal basis around each hand's
     * own closed-to-open rotation axis, maps the source rotation between those bases,
     * and negates the vector part to flip handedness.
     *
     * @return false when either arc axis is too close to vertical to build a stable
     * basis from, or the result is not finite - callers fall back to the generic
     * flex/splay path.
     */
    bool HandPoseMath::tryTransferMirroredThumbBase(const std::string& sourceBoneName, const std::string& targetBoneName, const RE::NiMatrix3& animatedRotation,
        RE::NiMatrix3& outRotation)
    {
        const RE::NiMatrix3& closedSource = authoredHandBones().at(sourceBoneName).closedRotation;
        const RE::NiMatrix3& openSource = authoredHandBones().at(sourceBoneName).openRotation;
        const RE::NiMatrix3& closedTarget = authoredHandBones().at(targetBoneName).closedRotation;
        const RE::NiMatrix3& openTarget = authoredHandBones().at(targetBoneName).openRotation;

        const auto parentDeltaAxis = [](const RE::NiMatrix3& open, const RE::NiMatrix3& closed, RE::NiPoint3& outAxis) {
            Quaternion q;
            q.fromMatrix(open * closed.Transpose());
            if (q.w < 0.0f) {
                q *= -1.0f;
            }
            const float w = std::clamp(q.w, -1.0f, 1.0f);
            const float sinHalf = std::sqrt((std::max)(0.0f, 1.0f - w * w));
            if (sinHalf < 1e-4f) {
                return false;
            }
            outAxis = RE::NiPoint3(q.x / sinHalf, q.y / sinHalf, q.z / sinHalf);
            return true;
        };
        RE::NiPoint3 sourceArcAxis{}, targetArcAxis{};
        if (!parentDeltaAxis(openSource, closedSource, sourceArcAxis) || !parentDeltaAxis(openTarget, closedTarget, targetArcAxis)) {
            return false;
        }

        const RE::NiPoint3 y{ 0.0f, 1.0f, 0.0f };
        const RE::NiPoint3 mirroredTargetArc{ -targetArcAxis.x, -targetArcAxis.y, -targetArcAxis.z };
        RE::NiPoint3 sourcePerpendicular{ sourceArcAxis.x, 0.0f, sourceArcAxis.z };
        RE::NiPoint3 targetPerpendicular{ mirroredTargetArc.x, 0.0f, mirroredTargetArc.z };
        constexpr float kMinPerpComponent = 0.05f;
        if (MatrixUtils::vec3Len(sourcePerpendicular) < kMinPerpComponent || MatrixUtils::vec3Len(targetPerpendicular) < kMinPerpComponent) {
            return false;
        }
        sourcePerpendicular = MatrixUtils::vec3Norm(sourcePerpendicular);
        targetPerpendicular = MatrixUtils::vec3Norm(targetPerpendicular);
        const RE::NiPoint3 sourceCross = MatrixUtils::vec3Cross(y, sourcePerpendicular);
        const RE::NiPoint3 targetCrossFlipped = MatrixUtils::vec3Cross(y, targetPerpendicular) * -1.0f;

        RE::NiMatrix3 sourceBasis, targetBasis;
        for (int row = 0; row < 3; ++row) {
            const float* yv = &y.x;
            sourceBasis.entry[row][0] = yv[row];
            sourceBasis.entry[row][1] = (&sourcePerpendicular.x)[row];
            sourceBasis.entry[row][2] = (&sourceCross.x)[row];
            targetBasis.entry[row][0] = yv[row];
            targetBasis.entry[row][1] = (&targetPerpendicular.x)[row];
            targetBasis.entry[row][2] = (&targetCrossFlipped.x)[row];
        }
        const RE::NiMatrix3 mirrorMap = targetBasis * sourceBasis.Transpose();

        Quaternion deltaQ;
        deltaQ.fromMatrix(animatedRotation * closedSource.Transpose());
        if (deltaQ.w < 0.0f) {
            deltaQ *= -1.0f;
        }
        const RE::NiPoint3 v{ deltaQ.x, deltaQ.y, deltaQ.z };
        const RE::NiPoint3 mv = mirrorMap * v;
        Quaternion mirroredDelta{ -mv.x, -mv.y, -mv.z, deltaQ.w };
        mirroredDelta.normalize();
        outRotation = mirroredDelta.getMatrix() * closedTarget;
        return isFiniteRotation(outRotation);
    }

    std::optional<RE::NiMatrix3> HandPoseMath::mirrorBoneRotation(const std::string& sourceBoneName, const std::string& targetBoneName, const RE::NiMatrix3& animatedRotation)
    {
        const auto& bones = authoredHandBones();
        if (!bones.contains(sourceBoneName) || !bones.contains(targetBoneName)) {
            return std::nullopt;
        }

        RE::NiMatrix3 mirrored{};
        if (!sourceBoneName.ends_with("Arm_Finger11") || !tryTransferMirroredThumbBase(sourceBoneName, targetBoneName, animatedRotation, mirrored)) {
            float flex = 1.0f;
            float splay = 0.0f;
            measureAnimatedFlexSplay(sourceBoneName, animatedRotation, flex, splay);
            mirrored = blendBoneRotation(targetBoneName, flex, splay);
        }

        return isFiniteRotation(mirrored) ? std::optional(mirrored) : std::nullopt;
    }

    /**
     * Place a mirrored rotation on the target bone's own rest offset.
     *
     * @return nullopt if the rotation could not be mirrored or the result is not finite.
     */
    std::optional<RE::NiTransform> HandPoseMath::mirrorBoneToOppositeHand(const std::string& sourceBoneName, const std::string& targetBoneName,
        const RE::NiMatrix3& animatedRotation, const bool inPowerArmor)
    {
        const auto mirroredRotation = mirrorBoneRotation(sourceBoneName, targetBoneName, animatedRotation);
        if (!mirroredRotation) {
            return std::nullopt;
        }

        RE::NiTransform mirrored{};
        mirrored.rotate = *mirroredRotation;
        mirrored.translate = authoredHandBones().at(targetBoneName).restTranslation(inPowerArmor);

        return isFiniteTransform(mirrored) ? std::optional(mirrored) : std::nullopt;
    }

    bool HandPoseMath::mirrorFingerLocalTransforms(const bool sourceIsLeft, const std::array<RE::NiTransform, FINGER_BONE_COUNT>& sourceTransforms,
        const std::uint16_t sourceEnabledMask, std::array<RE::NiTransform, FINGER_BONE_COUNT>& outTargetTransforms, std::uint16_t& outTargetEnabledMask)
    {
        outTargetTransforms = {};
        outTargetEnabledMask = 0;
        if ((sourceEnabledMask & FULL_LOCAL_TRANSFORM_MASK) != FULL_LOCAL_TRANSFORM_MASK) {
            return false;
        }

        const char sourcePrefix = sourceIsLeft ? 'L' : 'R';
        const bool inPowerArmor = f4vr::isInPowerArmor();
        std::array<RE::NiTransform, FINGER_BONE_COUNT> mirroredTransforms{};
        std::uint16_t mirroredMask = 0;
        for (const auto& boneData : getHandBoneData()) {
            if (boneData.boneName[0] != sourcePrefix) {
                continue;
            }

            const std::string sourceBoneName{ boneData.boneName };
            const int flatBoneIndex = boneToFlexIndex(sourceBoneName);
            if (flatBoneIndex < 0 || flatBoneIndex >= static_cast<int>(FINGER_BONE_COUNT)) {
                return false;
            }

            const auto& animatedSource = sourceTransforms[static_cast<std::size_t>(flatBoneIndex)];
            if (!isFiniteTransform(animatedSource)) {
                return false;
            }

            std::string targetBoneName = sourceBoneName;
            targetBoneName[0] = sourceIsLeft ? 'R' : 'L';
            const auto mirrored = mirrorBoneToOppositeHand(sourceBoneName, targetBoneName, animatedSource.rotate, inPowerArmor);
            if (!mirrored) {
                return false;
            }

            mirroredTransforms[static_cast<std::size_t>(flatBoneIndex)] = *mirrored;
            mirroredMask = static_cast<std::uint16_t>(mirroredMask | (1U << flatBoneIndex));
        }

        if (mirroredMask != FULL_LOCAL_TRANSFORM_MASK) {
            return false;
        }

        outTargetTransforms = mirroredTransforms;
        outTargetEnabledMask = mirroredMask;
        return true;
    }
}
