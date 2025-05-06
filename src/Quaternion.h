#pragma once

#include "matrix.h"
#include "f4se/NITypes.h"

namespace frik {
	class Quaternion {
	public:
		Quaternion() {
			// default to identity
			w = 1.0;
			x = 0.0;
			y = 0.0;
			z = 0.0;
		}

		Quaternion(const float x, const float y, const float z, const float w)
			: w(w), x(x), y(y), z(z) {}

		Quaternion(const float real, const NiPoint3 v) {
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

		Quaternion get() const {
			Quaternion q;
			q.w = w;
			q.x = x;
			q.y = y;
			q.z = z;

			return q;
		}

		Quaternion conjugate() const;

		float getMag() const;
		Quaternion getNorm() const;
		void normalize();

		double dot(const Quaternion& q) const;

		void setAngleAxis(float angle, NiPoint3 axis);
		float getAngleFromAxisAngle(const Quaternion& target) const;

		Matrix44 getRot() const;
		void fromRot(const NiMatrix43& rot);

		void slerp(float interp, const Quaternion& target);

		void vec2Vec(NiPoint3 v1, NiPoint3 v2);

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
