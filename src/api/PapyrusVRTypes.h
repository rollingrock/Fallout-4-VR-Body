#pragma once

namespace PapyrusVR
{
	typedef struct Vector3
	{
		float x;
		float y;
		float z;

        static const Vector3 zero;

		Vector3() = default;

		inline Vector3(float aX, float aY, float aZ)
		{
			x = aX;
			y = aY;
			z = aZ;
		}

        inline float lengthSquared(  )
        {
            return x * x + y * y + z * z;
        };
	} Vector3;

	Vector3 operator-(Vector3 const& lhs, Vector3 const& rhs);
	Vector3 operator+(Vector3 const& lhs, Vector3 const& rhs);

	typedef struct Quaternion
	{
		float x;
		float y;
		float z;
		float w;
	} Quaternion;


	typedef struct Matrix33
	{
		float m[3][3];

		Matrix33() = default;

		inline Matrix33(float x00, float x01, float x02,
			float x10, float x11, float x12,
			float x20, float x21, float x22)
		{
			m[0][0] = x00;
			m[1][0] = x10;
			m[2][0] = x20;

			m[0][1] = x01;
			m[1][1] = x11;
			m[2][1] = x21;

			m[0][2] = x02;
			m[1][2] = x12;
			m[2][2] = x22;
		}

		Matrix33 operator*(Matrix33 const& rhs) const;
	} Matrix33;

	typedef struct Matrix34
	{
		float m[3][4];

		Matrix34() = default;

		inline Matrix34(float x00, float x01, float x02, float x03,
						float x10, float x11, float x12, float x13, 
						float x20, float x21, float x22, float x23)
		{
			m[0][0] = x00;
			m[1][0] = x10;
			m[2][0] = x20;

			m[0][1] = x01;
			m[1][1] = x11;
			m[2][1] = x21;

			m[0][2] = x02;
			m[1][2] = x12;
			m[2][2] = x22;

			m[0][3] = x03;
			m[1][3] = x13;
			m[2][3] = x23;
		}

		Matrix34 operator+(Matrix34 const& rhs);
		Matrix34 operator-(Matrix34 const& rhs);

	} Matrix34;
	Matrix34 operator+(Matrix34 const& lhs, Matrix34 const& rhs);
	Matrix34 operator-(Matrix34 const& lhs, Matrix34 const& rhs);
	Vector3 operator*(Matrix34 const& lhs, Vector3 const& rhs);


	Matrix33 Matrix33FromTransform(Matrix34 const* matrix);
	Matrix34 Matrix34FromRotation(Matrix33 const* matrix);

	typedef struct Matrix44
	{
		float m[4][4];
	} Matrix44;

	enum VRDevice
	{
		VRDevice_Unknown = -1,
		VRDevice_HMD = 0,
		VRDevice_RightController = 1,
		VRDevice_LeftController = 2
	};

	enum VREvent
	{
		VREvent_Negative = -1,
		VREvent_None = 0,
		VREvent_Positive = 1
	};

	enum VREventType
	{
		VREventType_Touched = 0,
		VREventType_Untouched = 1,
		VREventType_Pressed = 2,
		VREventType_Released = 3
	};

	enum VROverlapEvent
	{
		VROverlapEvent_None = 0,
		VROverlapEvent_OnEnter = 1,
		VROverlapEvent_OnExit = 2
	};
}