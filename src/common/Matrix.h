#pragma once

#include <algorithm>
#include <numbers>

#include "MatrixUtils.h"
#include "RE/Fallout.h"

namespace common
{
    class Matrix44
    {
    public:
        Matrix44()
        {
            for (auto& i : data) {
                for (float& j : i) {
                    j = 0.0;
                }
            }
        }

        explicit Matrix44(const RE::NiMatrix3& other)
        {
            for (auto& i : data) {
                for (float& j : i) {
                    j = 0.0;
                }
            }
            for (auto i = 0; i < 3; i++) {
                for (auto j = 0; j < 3; j++) {
                    data[i][j] = other.entry[i][j];
                }
            }
        }

        Matrix44& makeIdentity()
        {
            data[0][0] = 1.0;
            data[0][1] = 0.0;
            data[0][2] = 0.0;
            data[0][3] = 0.0;
            data[1][0] = 0.0;
            data[1][1] = 1.0;
            data[1][2] = 0.0;
            data[1][3] = 0.0;
            data[2][0] = 0.0;
            data[2][1] = 0.0;
            data[2][2] = 1.0;
            data[2][3] = 0.0;
            data[3][0] = 0.0;
            data[3][1] = 0.0;
            data[3][2] = 0.0;
            data[3][3] = 0.0;
            return *this;
        }

        void setPosition(const float x, const float y, const float z)
        {
            data[3][0] = x;
            data[3][1] = y;
            data[3][2] = z;
        }

        void setPosition(const RE::NiPoint3 pt)
        {
            data[3][0] = pt.x;
            data[3][1] = pt.y;
            data[3][2] = pt.z;
        }

        void makeTransformMatrix(const RE::NiMatrix3& rot, const RE::NiPoint3 pos)
        {
            for (auto i = 0; i < 3; i++) {
                for (auto j = 0; j < 3; j++) {
                    data[i][j] = rot.entry[i][j];
                }
            }
            data[0][3] = 0.0;
            data[1][3] = 0.0;
            data[2][3] = 0.0;
            data[3][3] = 1.0;
            data[3][0] = pos.x;
            data[3][1] = pos.y;
            data[3][2] = pos.z;
        }

        //overload
        void operator =(const float num)
        {
            for (auto& i : data) {
                for (float& j : i) {
                    j = num;
                }
            }
        }

        float data[4][4];

        static Matrix44 getIdentity()
        {
            Matrix44 ident;
            ident.makeIdentity();
            return ident;
        }

        static RE::NiMatrix3 getIdentity43()
        {
            RE::NiMatrix3 ident;
            ident.entry[0][0] = 1.0;
            ident.entry[0][1] = 0.0;
            ident.entry[0][2] = 0.0;
            ident.entry[1][0] = 0.0;
            ident.entry[1][1] = 1.0;
            ident.entry[1][2] = 0.0;
            ident.entry[2][0] = 0.0;
            ident.entry[2][1] = 0.0;
            ident.entry[2][2] = 1.0;
            return ident;
        }

        void getEulerAngles(float* heading, float* roll, float* attitude) const
        {
            if (data[2][0] < 1.0) {
                if (data[2][0] > -1.0) {
                    *heading = atan2(-data[2][1], data[2][2]);
                    *attitude = asin(data[2][0]);
                    *roll = atan2(-data[1][0], data[0][0]);
                } else {
                    *heading = -atan2(-data[0][1], data[1][1]);
                    *attitude = -std::numbers::pi_v<float> / 2;
                    *roll = 0.0;
                }
            } else {
                *heading = atan2(data[0][1], data[1][1]);
                *attitude = std::numbers::pi_v<float> / 2;
                *roll = 0.0;
            }
        }

        void setEulerAngles(const float heading, const float roll, const float attitude)
        {
            const float sinX = sin(heading);
            const float cosX = cos(heading);
            const float sinY = sin(roll);
            const float cosY = cos(roll);
            const float sinZ = sin(attitude);
            const float cosZ = cos(attitude);

            data[0][0] = cosY * cosZ;
            data[1][0] = sinX * sinY * cosZ + sinZ * cosX;
            data[2][0] = sinX * sinZ - cosX * sinY * cosZ;
            data[0][1] = -cosY * sinZ;
            data[1][1] = cosX * cosZ - sinX * sinY * sinZ;
            data[2][1] = cosX * sinY * sinZ + sinX * cosZ;
            data[0][2] = sinY;
            data[1][2] = -sinX * cosY;
            data[2][2] = cosX * cosY;
        }

        void rotateVectorVec(RE::NiPoint3 toVec, RE::NiPoint3 fromVec)
        {
            toVec = vec3Norm(toVec);
            fromVec = vec3Norm(fromVec);

            const float dotP = vec3Dot(fromVec, toVec);

            if (dotP >= 0.99999) {
                this->makeIdentity();
                return;
            }

            RE::NiPoint3 crossP = vec3Cross(toVec, fromVec);
            crossP = vec3Norm(crossP);

            const float phi = acosf(dotP);
            const float rCos = cos(phi);
            const float rSin = sin(phi);

            // Build the matrix
            data[0][0] = rCos + crossP.x * crossP.x * (1.0f - rCos);
            data[0][1] = -crossP.z * rSin + crossP.x * crossP.y * (1.0f - rCos);
            data[0][2] = crossP.y * rSin + crossP.x * crossP.z * (1.0f - rCos);
            data[1][0] = crossP.z * rSin + crossP.y * crossP.x * (1.0f - rCos);
            data[1][1] = rCos + crossP.y * crossP.y * (1.0f - rCos);
            data[1][2] = -crossP.x * rSin + crossP.y * crossP.z * (1.0f - rCos);
            data[2][0] = -crossP.y * rSin + crossP.z * crossP.x * (1.0f - rCos);
            data[2][1] = crossP.x * rSin + crossP.z * crossP.y * (1.0f - rCos);
            data[2][2] = rCos + crossP.z * crossP.z * (1.0f - rCos);
        }

        RE::NiMatrix3 make43() const
        {
            RE::NiMatrix3 ret;
            for (auto i = 0; i < 3; i++) {
                for (auto j = 0; j < 3; j++) {
                    ret.entry[i][j] = this->data[i][j];
                }
            }
            return ret;
        }

        RE::NiMatrix3 multiply43Left(const RE::NiMatrix3& mat) const
        {
            return this->make43() * mat;
            // return mult(mat, this->make43());
        }

        RE::NiMatrix3 multiply43Right(const RE::NiMatrix3& mat) const
        {
            return mat * this->make43();
            // return mult(this->make43(), mat);
        }

        static void matrixMultiply(const Matrix44* worldMat, const Matrix44* retMat, const Matrix44* localMat)
        {
            // This uses the native transform function that the updateWorld call makes
            using func_t = decltype(&matrixMultiply);
            REL::Relocation<func_t> func(REL::Offset(0x1a8d60));
            return func(worldMat, retMat, localMat);
        }

        static RE::NiMatrix3 mult(const RE::NiMatrix3& left, const RE::NiMatrix3& right)
        {
            RE::NiMatrix3 tmp;
            // shamelessly taken from SKSE
            tmp.entry[0][0] =
                right.entry[0][0] * left.entry[0][0] +
                right.entry[0][1] * left.entry[1][0] +
                right.entry[0][2] * left.entry[2][0];
            tmp.entry[1][0] =
                right.entry[1][0] * left.entry[0][0] +
                right.entry[1][1] * left.entry[1][0] +
                right.entry[1][2] * left.entry[2][0];
            tmp.entry[2][0] =
                right.entry[2][0] * left.entry[0][0] +
                right.entry[2][1] * left.entry[1][0] +
                right.entry[2][2] * left.entry[2][0];
            tmp.entry[0][1] =
                right.entry[0][0] * left.entry[0][1] +
                right.entry[0][1] * left.entry[1][1] +
                right.entry[0][2] * left.entry[2][1];
            tmp.entry[1][1] =
                right.entry[1][0] * left.entry[0][1] +
                right.entry[1][1] * left.entry[1][1] +
                right.entry[1][2] * left.entry[2][1];
            tmp.entry[2][1] =
                right.entry[2][0] * left.entry[0][1] +
                right.entry[2][1] * left.entry[1][1] +
                right.entry[2][2] * left.entry[2][1];
            tmp.entry[0][2] =
                right.entry[0][0] * left.entry[0][2] +
                right.entry[0][1] * left.entry[1][2] +
                right.entry[0][2] * left.entry[2][2];
            tmp.entry[1][2] =
                right.entry[1][0] * left.entry[0][2] +
                right.entry[1][1] * left.entry[1][2] +
                right.entry[1][2] * left.entry[2][2];
            tmp.entry[2][2] =
                right.entry[2][0] * left.entry[0][2] +
                right.entry[2][1] * left.entry[1][2] +
                right.entry[2][2] * left.entry[2][2];
            return tmp;
        }
    };

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

        float getAngleFromAxisAngle(const Quaternion& target) const
        {
            const auto ret = target * this->conjugate();
            return 2 * acosf(ret.w);
        }

        Matrix44 getRot() const
        {
            Matrix44 mat;

            mat.data[0][0] = 2 * (w * w + x * x) - 1;
            mat.data[0][1] = 2 * (x * y - w * z);
            mat.data[0][2] = 2 * (x * z + w * y);
            mat.data[1][0] = 2 * (x * y + w * z);
            mat.data[1][1] = 2 * (w * w + y * y) - 1;
            mat.data[1][2] = 2 * (y * z - w * x);
            mat.data[2][0] = 2 * (x * z - w * y);
            mat.data[2][1] = 2 * (y * z + w * x);
            mat.data[2][2] = 2 * (w * w + z * z) - 1;

            return mat;
        }

        RE::NiMatrix3 getMatrix3() const
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

        void fromRot(const RE::NiMatrix3& rot)
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
