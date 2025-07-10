#include "MatrixUtils.h"

#include <numbers>

#include "Matrix.h"

namespace common
{
    RE::NiTransform getTransform(const float x, const float y, const float z, const float r1, const float r2, const float r3, const float r4, const float r5, const float r6,
        const float r7, const float r8, const float r9, const float scale)
    {
        RE::NiTransform transform;
        transform.translate = RE::NiPoint3(x, y, z);
        transform.rotate.entry[0][0] = r1;
        transform.rotate.entry[1][0] = r2;
        transform.rotate.entry[2][0] = r3;
        transform.rotate.entry[0][1] = r4;
        transform.rotate.entry[1][1] = r5;
        transform.rotate.entry[2][1] = r6;
        transform.rotate.entry[0][2] = r7;
        transform.rotate.entry[1][2] = r8;
        transform.rotate.entry[2][2] = r9;
        transform.scale = scale;
        return transform;
    }

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
        Matrix44 rot;

        rot.makeTransformMatrix(getRotationAxisAngle(vec3Norm(rotAxis), angle), RE::NiPoint3(0, 0, 0));

        return rot.make43() * vec;
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
