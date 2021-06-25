#pragma once
#include "f4se/NITypes.h"

#include "utils.h"
#include "matrix.h"


namespace F4VRBody {
	class Quaternion {
	public:
		Quaternion() {
			// default to identity
			w = 1.0;
			x = 0.0;
			y = 0.0;
			z = 0.0;
		}

		Quaternion(float X, float Y, float Z, float W) : w(W), x(X), y(Y), z(Z) { };

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

		Quaternion conjugate();

		float getMag();
		Quaternion getNorm();
		void normalize();

		void setAngleAxis(float angle, NiPoint3 axis);

		Matrix44 getRot();
		void fromRot(NiMatrix43 rot);

		//overload
		Quaternion  operator* (const float& f) const;
		Quaternion  operator* (const Quaternion& qr) const;
		Quaternion& operator*= (const float& f);
		Quaternion& operator*= (const Quaternion& qr);

	public:
		float w;
		float x;
		float y;
		float z;
	};
}
