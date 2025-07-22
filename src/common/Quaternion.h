#pragma once

#include <algorithm>
#include <numbers>

#include "MatrixUtils.h"
#include "RE/Fallout.h"

namespace common
{
    class Quaternion
    {
    public:
        Quaternion()
        {
            // default to identity
            w = 1.0;
            x = 0.0;
            y = 0.0;
            z = 0.0;
        }

        Quaternion(const float x, const float y, const float z, const float w) :
            w(w), x(x), y(y), z(z) {}

        Quaternion(const float real, const RE::NiPoint3 v)
        {
            w = real;
            x = v.x;
            y = v.y;
            z = v.z;
        }

        ~Quaternion() = default;

        void makeIdentity()
        {
            w = 1.0;
            x = 0.0;
            y = 0.0;
            z = 0.0;
        }

        Quaternion get() const
        {
            Quaternion q;
            q.w = w;
            q.x = x;
            q.y = y;
            q.z = z;

            return q;
        }

        float getMag() const
        {
            return sqrtf(w * w + x * x + y * y + z * z);
        }

        Quaternion getNorm() const
        {
            const float mag = getMag();
            return Quaternion(w / mag, RE::NiPoint3(x / mag, y / mag, z / mag));
        }

        void normalize()
        {
            const float mag = getMag();

            w /= mag;
            x /= mag;
            y /= mag;
            z /= mag;
        }

        double dot(const Quaternion& q) const
        {
            return w * q.w + x * q.x + y * q.y + z * q.z;
        }

        Quaternion conjugate() const
        {
            Quaternion q;
            q.w = w;
            q.x = x == 0.0f ? 0 : -x;
            q.y = y == 0.0f ? 0 : -y;
            q.z = z == 0.0f ? 0 : -z;

            return q;
        }

        void setAngleAxis(float angle, RE::NiPoint3 axis)
        {
            axis = vec3Norm(axis);

            angle /= 2;

            const float sinAngle = sinf(angle);

            w = cosf(angle);
            x = sinAngle * axis.x;
            y = sinAngle * axis.y;
            z = sinAngle * axis.z;
        }

        RE::NiMatrix3 getMatrix() const
        {
            RE::NiMatrix3 ret;

            ret.entry[0][0] = 2 * (w * w + x * x) - 1;
            ret.entry[0][1] = 2 * (x * y - w * z);
            ret.entry[0][2] = 2 * (x * z + w * y);

            ret.entry[1][0] = 2 * (x * y + w * z);
            ret.entry[1][1] = 2 * (w * w + y * y) - 1;
            ret.entry[1][2] = 2 * (y * z - w * x);

            ret.entry[2][0] = 2 * (x * z - w * y);
            ret.entry[2][1] = 2 * (y * z + w * x);
            ret.entry[2][2] = 2 * (w * w + z * z) - 1;

            return ret;
        }

        void fromMatrix(const RE::NiMatrix3& rot)
        {
            Quaternion q;
            q.w = sqrtf(std::max<float>(0.0f, 1 + rot.entry[0][0] + rot.entry[1][1] + rot.entry[2][2])) / 2;
            q.x = sqrtf(std::max<float>(0.0f, 1 + rot.entry[0][0] - rot.entry[1][1] - rot.entry[2][2])) / 2;
            q.y = sqrtf(std::max<float>(0.0f, 1 - rot.entry[0][0] + rot.entry[1][1] - rot.entry[2][2])) / 2;
            q.z = sqrtf(std::max<float>(0.0f, 1 - rot.entry[0][0] - rot.entry[1][1] + rot.entry[2][2])) / 2;

            w = q.w;
            x = _copysign(q.x, rot.entry[2][1] - rot.entry[1][2]);
            y = _copysign(q.y, rot.entry[0][2] - rot.entry[2][0]);
            z = _copysign(q.z, rot.entry[1][0] - rot.entry[0][1]);
        }

        // slerp function adapted from VRIK - credit prog for math

        void slerp(const float interp, const Quaternion& target)
        {
            const Quaternion save = this->get();

            double dotp = this->dot(target);

            if (dotp < 0.0f) {
                w = -w;
                x = -x;
                y = -y;
                z = -z;
                dotp = -dotp;
            }

            if (dotp > 0.999995) {
                w = save.w;
                x = save.x;
                y = save.y;
                z = save.z;
                return;
            }
            const float theta0 = acosf(dotp); // theta_0 = angle between input vectors
            const float theta = theta0 * interp; // theta = angle between q1 and result
            const float sinTheta = sinf(theta); // compute this value only once
            const float sinTheta0 = sinf(theta0); // compute this value only once
            const float s0 = cosf(theta) - dotp * sinTheta / sinTheta0; // == sin(theta_0 - theta) / sin(theta_0)
            const float s1 = sinTheta / sinTheta0;

            w = s0 * w + s1 * target.w;
            x = s0 * x + s1 * target.x;
            y = s0 * y + s1 * target.y;
            z = s0 * z + s1 * target.z;
        }

        void vec2Vec(const RE::NiPoint3 v1, const RE::NiPoint3 v2)
        {
            RE::NiPoint3 cross = vec3Cross(vec3Norm(v1), vec3Norm(v2));

            const float dotP = vec3Dot(vec3Norm(v1), vec3Norm(v2));

            if (dotP > 0.99999999) {
                this->makeIdentity();
                return;
            }
            if (dotP < -0.99999999) {
                // reverse it
                cross = vec3Norm(vec3Cross(RE::NiPoint3(0, 1, 0), v1));
                if (vec3Len(cross) < 0.00000001) {
                    cross = vec3Norm(vec3Cross(RE::NiPoint3(1, 0, 0), v1));
                }
                this->setAngleAxis(std::numbers::pi_v<float>, cross);
                this->normalize();
                return;
            }

            w = sqrt(pow(vec3Len(v1), 2) * pow(vec3Len(v2), 2)) + dotP;
            x = cross.x;
            y = cross.y;
            z = cross.z;

            this->normalize();
        }

        Quaternion operator*(const Quaternion& qr) const
        {
            Quaternion q;

            q.w = w * qr.w - x * qr.x - y * qr.y - z * qr.z;
            q.x = w * qr.x + x * qr.w + y * qr.z - z * qr.y;
            q.y = w * qr.y - x * qr.z + y * qr.w + z * qr.x;
            q.z = w * qr.z + x * qr.y - y * qr.x + z * qr.w;

            return q;
        }

        Quaternion operator*(const float& f) const
        {
            Quaternion q;

            q.w = w * f;
            q.x = x * f;
            q.y = y * f;
            q.z = z * f;

            return q;
        }

        Quaternion& operator*=(const Quaternion& qr)
        {
            Quaternion q;

            q.w = w * qr.w - x * qr.x - y * qr.y - z * qr.z;
            q.x = w * qr.x + x * qr.w + y * qr.z - z * qr.y;
            q.y = w * qr.y - x * qr.z + y * qr.w + z * qr.x;
            q.z = w * qr.z + x * qr.y - y * qr.x + z * qr.w;

            *this = q;
            return *this;
        }

        Quaternion& operator*=(const float& f)
        {
            w *= f;
            x *= f;
            y *= f;
            z *= f;

            return *this;
        }

        void operator=(const Quaternion& q)
        {
            w = q.w;
            x = q.x;
            y = q.y;
            z = q.z;
        }

        float w;
        float x;
        float y;
        float z;
    };
}
