#include "MatrixUtils.h"

#include <numbers>

namespace common
{
    RE::NiPoint3 vec3Norm(RE::NiPoint3 v1)
    {
        const float mag = vec3Len(v1);

        if (mag < 0.000001) {
            const float maxX = abs(v1.x);
            const float maxY = abs(v1.y);
            const float maxZ = abs(v1.z);

            if (maxX >= maxY && maxX >= maxZ) {
                return v1.x >= 0 ? RE::NiPoint3(1, 0, 0) : RE::NiPoint3(-1, 0, 0);
            }
            if (maxY > maxZ) {
                return v1.y >= 0 ? RE::NiPoint3(0, 1, 0) : RE::NiPoint3(0, -1, 0);
            }
            return v1.z >= 0 ? RE::NiPoint3(0, 0, 1) : RE::NiPoint3(0, 0, -1);
        }

        v1.x /= mag;
        v1.y /= mag;
        v1.z /= mag;

        return v1;
    }

    float vec3Dot(const RE::NiPoint3& v1, const RE::NiPoint3& v2)
    {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    RE::NiPoint3 vec3Cross(const RE::NiPoint3& v1, const RE::NiPoint3& v2)
    {
        return RE::NiPoint3(
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
            );
    }

    // the determinant is proportional to the sin of the angle between two vectors.   In 3d case find the sin of the angle between v1 and v2
    // along their angle of rotation with unit vector n
    // https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors/16544330#16544330
    float vec3Det(const RE::NiPoint3 v1, const RE::NiPoint3 v2, const RE::NiPoint3 n)
    {
        return v1.x * v2.y * n.z + v2.x * n.y * v1.z + n.x * v1.y * v2.z - v1.z * v2.y * n.x - v2.z * n.y * v1.x - n.z * v1.y * v2.x;
    }

    float distanceNoSqrt(const RE::NiPoint3 po1, const RE::NiPoint3 po2)
    {
        const float x = po1.x - po2.x;
        const float y = po1.y - po2.y;
        const float z = po1.z - po2.z;
        return x * x + y * y + z * z;
    }

    float distanceNoSqrt2d(const float x1, const float y1, const float x2, const float y2)
    {
        const float x = x1 - x2;
        const float y = y1 - y2;
        return x * x + y * y;
    }

    float degreesToRads(const float deg)
    {
        return deg * std::numbers::pi_v<float> / 180;
    }

    float radsToDegrees(const float rad)
    {
        return rad * 180 / std::numbers::pi_v<float>;
    }

    RE::NiPoint3 rotateXY(const RE::NiPoint3 vec, const float angle)
    {
        RE::NiPoint3 retV;

        retV.x = vec.x * cosf(angle) - vec.y * sinf(angle);
        retV.y = vec.x * sinf(angle) + vec.y * cosf(angle);
        retV.z = vec.z;

        return retV;
    }

    RE::NiPoint3 pitchVec(const RE::NiPoint3 vec, const float angle)
    {
        const auto rotAxis = RE::NiPoint3(vec.y, -vec.x, 0);
        return getRotationAxisAngle(vec3Norm(rotAxis), angle) * vec;
    }

    RE::NiMatrix3 getIdentityMatrix()
    {
        RE::NiMatrix3 iden;
        iden.MakeIdentity();
        return iden;
    }

    RE::NiMatrix3 getMatrix(const float r1, const float r2, const float r3, const float r4, const float r5, const float r6, const float r7, const float r8, const float r9)
    {
        RE::NiMatrix3 result;
        result.entry[0][0] = r1;
        result.entry[1][0] = r2;
        result.entry[2][0] = r3;
        result.entry[0][1] = r4;
        result.entry[1][1] = r5;
        result.entry[2][1] = r6;
        result.entry[0][2] = r7;
        result.entry[1][2] = r8;
        result.entry[2][2] = r9;
        return result;
    }

    void getEulerAnglesFromMatrix(const RE::NiMatrix3& matrix, float* heading, float* roll, float* attitude)
    {
        if (matrix.entry[2][0] < 1.0) {
            if (matrix.entry[2][0] > -1.0) {
                *heading = atan2(-matrix.entry[2][1], matrix.entry[2][2]);
                *attitude = asin(matrix.entry[2][0]);
                *roll = atan2(-matrix.entry[1][0], matrix.entry[0][0]);
            } else {
                *heading = -atan2(-matrix.entry[0][1], matrix.entry[1][1]);
                *attitude = -std::numbers::pi_v<float> / 2;
                *roll = 0.0;
            }
        } else {
            *heading = atan2(matrix.entry[0][1], matrix.entry[1][1]);
            *attitude = std::numbers::pi_v<float> / 2;
            *roll = 0.0;
        }
    }

    RE::NiMatrix3 getMatrixFromEulerAngles(const float heading, const float roll, const float attitude)
    {
        const float sinX = sin(heading);
        const float cosX = cos(heading);
        const float sinY = sin(roll);
        const float cosY = cos(roll);
        const float sinZ = sin(attitude);
        const float cosZ = cos(attitude);

        RE::NiMatrix3 result;
        result.entry[0][0] = cosY * cosZ;
        result.entry[1][0] = sinX * sinY * cosZ + sinZ * cosX;
        result.entry[2][0] = sinX * sinZ - cosX * sinY * cosZ;
        result.entry[0][1] = -cosY * sinZ;
        result.entry[1][1] = cosX * cosZ - sinX * sinY * sinZ;
        result.entry[2][1] = cosX * sinY * sinZ + sinX * cosZ;
        result.entry[0][2] = sinY;
        result.entry[1][2] = -sinX * cosY;
        result.entry[2][2] = cosX * cosY;
        return result;
    }

    RE::NiMatrix3 getMatrixFromRotateVectorVec(const RE::NiPoint3& toVec, const RE::NiPoint3& fromVec)
    {
        const auto toVecNorm = vec3Norm(toVec);
        const auto fromVecNorm = vec3Norm(fromVec);

        const float dotP = vec3Dot(fromVecNorm, toVecNorm);

        if (dotP >= 0.99999) {
            return getIdentityMatrix();
        }

        const auto crossP = vec3Norm(vec3Cross(toVecNorm, fromVecNorm));
        const float phi = acosf(dotP);
        const float rCos = cos(phi);
        const float rSin = sin(phi);

        // Build the matrix
        RE::NiMatrix3 result;
        result.entry[0][0] = rCos + crossP.x * crossP.x * (1.0f - rCos);
        result.entry[0][1] = -crossP.z * rSin + crossP.x * crossP.y * (1.0f - rCos);
        result.entry[0][2] = crossP.y * rSin + crossP.x * crossP.z * (1.0f - rCos);
        result.entry[1][0] = crossP.z * rSin + crossP.y * crossP.x * (1.0f - rCos);
        result.entry[1][1] = rCos + crossP.y * crossP.y * (1.0f - rCos);
        result.entry[1][2] = -crossP.x * rSin + crossP.y * crossP.z * (1.0f - rCos);
        result.entry[2][0] = -crossP.y * rSin + crossP.z * crossP.x * (1.0f - rCos);
        result.entry[2][1] = crossP.x * rSin + crossP.z * crossP.y * (1.0f - rCos);
        result.entry[2][2] = rCos + crossP.z * crossP.z * (1.0f - rCos);
        return result;
    }

    // Gets a rotation matrix from an axis and an angle
    RE::NiMatrix3 getRotationAxisAngle(RE::NiPoint3 axis, const float theta)
    {
        RE::NiMatrix3 result;
        // This math was found online http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/
        const float c = cosf(theta);
        const float s = sinf(theta);
        const float t = 1.0f - c;
        axis = vec3Norm(axis);
        result.entry[0][0] = c + axis.x * axis.x * t;
        result.entry[1][1] = c + axis.y * axis.y * t;
        result.entry[2][2] = c + axis.z * axis.z * t;
        float tmp1 = axis.x * axis.y * t;
        float tmp2 = axis.z * s;
        result.entry[1][0] = tmp1 + tmp2;
        result.entry[0][1] = tmp1 - tmp2;
        tmp1 = axis.x * axis.z * t;
        tmp2 = axis.y * s;
        result.entry[2][0] = tmp1 - tmp2;
        result.entry[0][2] = tmp1 + tmp2;
        tmp1 = axis.y * axis.z * t;
        tmp2 = axis.x * s;
        result.entry[2][1] = tmp1 + tmp2;
        result.entry[1][2] = tmp1 - tmp2;
        return result.Transpose();
    }

    RE::NiTransform getTransform(const float x, const float y, const float z, const float r1, const float r2, const float r3, const float r4, const float r5, const float r6,
        const float r7, const float r8, const float r9, const float scale)
    {
        RE::NiTransform transform;
        transform.translate = RE::NiPoint3(x, y, z);
        transform.rotate = getMatrix(r1, r2, r3, r4, r5, r6, r7, r8, r9);
        transform.scale = scale;
        return transform;
    }

    /**
     * Compute the delta transform between two transforms.
     * i.e. the transform that takes from "from" transform to the "to" transform.
     */
    RE::NiTransform getDeltaTransform(const RE::NiTransform& from, const RE::NiTransform& to)
    {
        RE::NiTransform delta;
        delta.scale = to.scale / from.scale;
        delta.rotate = from.rotate.Transpose() * to.rotate;
        delta.translate = to.translate - delta.rotate.Transpose() * (from.translate * delta.scale);
        return delta;
    }

    /**
     * Compute the target transform starting with base using the delta transform from "from" to "to".
     * i.e. made the same change as from->to on base to get target.
     */
    RE::NiTransform getTargetTransform(const RE::NiTransform& baseFrom, const RE::NiTransform& baseTo, const RE::NiTransform& targetFrom)
    {
        const auto delta = getDeltaTransform(baseFrom, baseTo);
        RE::NiTransform target;
        target.scale = delta.scale * targetFrom.scale;
        target.rotate = targetFrom.rotate * delta.rotate;
        target.translate = delta.rotate.Transpose() * ((targetFrom.translate * delta.scale) + delta.translate);
        return target;
    }

    /**
     * Check if the camera is looking at the object and the object facing the camera
     */
    bool isCameraLookingAtObject(const RE::NiTransform& cameraTrans, const RE::NiTransform& objectTrans, const float detectThresh)
    {
        // Get the position of the camera and the object
        const auto cameraPos = cameraTrans.translate;
        const auto objectPos = objectTrans.translate;

        // Calculate the direction vector from the camera to the object
        const auto direction = vec3Norm(RE::NiPoint3(objectPos.x - cameraPos.x, objectPos.y - cameraPos.y, objectPos.z - cameraPos.z));

        // Get the forward vector of the camera (assuming it's the y-axis)
        const auto cameraForward = vec3Norm(cameraTrans.rotate.Transpose() * (RE::NiPoint3(0, 1, 0)));

        // Get the forward vector of the object (assuming it's the y-axis)
        const auto objectForward = vec3Norm(objectTrans.rotate.Transpose() * (RE::NiPoint3(0, 1, 0)));

        // Check if the camera is looking at the object
        const float cameraDot = vec3Dot(cameraForward, direction);
        const bool isCameraLooking = cameraDot > detectThresh; // Adjust the threshold as needed

        // Check if the object is facing the camera
        const float objectDot = vec3Dot(objectForward, direction);
        const bool isObjectFacing = objectDot > detectThresh; // Adjust the threshold as needed

        return isCameraLooking && isObjectFacing;
    }
}
