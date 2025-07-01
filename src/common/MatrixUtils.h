#pragma once

namespace common
{
    // 3D space related functions
    RE::NiTransform getTransform(float x, float y, float z, float r1, float r2, float r3, float r4, float r5, float r6, float r7, float r8, float r9, float scale);
    float vec3Len(const RE::NiPoint3& v1);
    RE::NiPoint3 vec3Norm(RE::NiPoint3 v1);
    float vec3Dot(const RE::NiPoint3& v1, const RE::NiPoint3& v2);
    RE::NiPoint3 vec3Cross(const RE::NiPoint3& v1, const RE::NiPoint3& v2);
    float vec3Det(RE::NiPoint3 v1, RE::NiPoint3 v2, RE::NiPoint3 n);
    float distanceNoSqrt(RE::NiPoint3 po1, RE::NiPoint3 po2);
    float distanceNoSqrt2d(float x1, float y1, float x2, float y2);
    float degreesToRads(float deg);
    float radsToDegrees(float rad);
    RE::NiPoint3 rotateXY(RE::NiPoint3 vec, float angle);
    RE::NiPoint3 pitchVec(RE::NiPoint3 vec, float angle);
    RE::NiMatrix3 getRotationAxisAngle(RE::NiPoint3 axis, float theta);
    RE::NiTransform getDeltaTransform(const RE::NiTransform& from, const RE::NiTransform& to);
    RE::NiTransform getTargetTransform(const RE::NiTransform& baseFrom, const RE::NiTransform& baseTo, const RE::NiTransform& targetFrom);
    bool isCameraLookingAtObject(const RE::NiTransform& cameraTrans, const RE::NiTransform& objectTrans, float detectThresh);
}
