#pragma once

#include "matrix.h"
#include "f4se/NITypes.h"

namespace FRIK {
	class Quaternion {
	public:
		Quaternion() {
			// default to identity
			w = 1.0;
			x = 0.0;
			y = 0.0;
			z = 0.0;
		}

		Quaternion(float X, float Y, float Z, float W)
			: w(W), x(X), y(Y), z(Z) {};

		Quaternion(float real, NiPoint3 v) {
			w = real;
			x = v.x;
			y = v.y;
			z = v.z;
		}

		~Quaternion() {};

		void makeIdentity() {
			w = 1.0;
			x = 0.0;
			y = 0.0;
			z = 0.0;
		}

		Quaternion get() {
			Quaternion q;
			q.w = w;
			q.x = x;
			q.y = y;
			q.z = z;

			return q;
		}

		Quaternion conjugate();

		float getMag();
		Quaternion getNorm();
		void normalize();

		double dot(Quaternion q);

		void setAngleAxis(float angle, NiPoint3 axis);
		float getAngleFromAxisAngle(Quaternion target);

		Matrix44 getRot();
		void fromRot(NiMatrix43 rot);

		void slerp(float interp, Quaternion target);

		void vec2vec(NiPoint3 v1, NiPoint3 v2);

		//overload
		Quaternion operator*(const float& f) const;
		Quaternion operator*(const Quaternion& qr) const;
		Quaternion& operator*=(const float& f);
		Quaternion& operator*=(const Quaternion& qr);
		void operator=(const Quaternion& q);

		float w;
		float x;
		float y;
		float z;
	};
}
