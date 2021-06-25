#include "Quaternion.h"


namespace F4VRBody {

	float Quaternion::getMag() {
		return sqrtf(w * w + x * x + y * y + z * z);
	}

	Quaternion Quaternion::getNorm() {
		float mag = getMag();
		return Quaternion(w / mag, NiPoint3(x / mag, y / mag, z / mag));
	}

	void Quaternion::normalize() {
		float mag = getMag();

		w /= mag;
		x /= mag;
		y /= mag;
		z /= mag;
	}

	Quaternion Quaternion::conjugate() {
		Quaternion q;
		q.w = w;
		q.x = x == 0.0f ? 0 : -x;
		q.y = y == 0.0f ? 0 : -y;
		q.z = z == 0.0f ? 0 : -z;

		return q;
	}

	void Quaternion::setAngleAxis(float angle, NiPoint3 axis) {
		axis = vec3_norm(axis);

		angle /= 2;

		float sinAngle = sinf(angle);

		w = cosf(angle);
		x = sinAngle * axis.x;
		y = sinAngle * axis.y;
		z = sinAngle * axis.z;
	}

	Matrix44 Quaternion::getRot() {
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

	void Quaternion::fromRot(NiMatrix43 rot) {
		Quaternion q;
		float tr = 0.0f;

		if (rot.data[2][2] < 0) {
			if (rot.data[0][0] > rot.data[1][1]) {
				tr = 1 + rot.data[0][0] - rot.data[1][1] - rot.data[2][2];
				q = Quaternion(tr, rot.data[0][1] + rot.data[1][0], rot.data[2][0] + rot.data[0][2], rot.data[2][1] - rot.data[1][2]);
			}
			else {
				tr = 1 - rot.data[0][0] + rot.data[1][1] - rot.data[2][2];
				q = Quaternion(rot.data[0][1] + rot.data[1][0], tr, rot.data[1][2] + rot.data[2][1], rot.data[0][2] - rot.data[2][0]);
			}
		}
		else {
			if (rot.data[0][0] < -rot.data[1][1]) {
				tr = 1 - rot.data[0][0] - rot.data[1][1] + rot.data[2][2];
				q = Quaternion(rot.data[2][0] + rot.data[0][2], rot.data[1][2] + rot.data[2][1], tr, rot.data[1][0] - rot.data[0][1]);
			}
			else {
				tr = 1 + rot.data[0][0] + rot.data[1][1] + rot.data[2][2];
				q = Quaternion(rot.data[2][1] - rot.data[1][2], rot.data[2][0] - rot.data[0][2], rot.data[1][0] - rot.data[0][1], tr);
			}
		}

		q *= 0.5 / sqrtf(tr);
		*this = q;
	}


	Quaternion Quaternion::operator* (const Quaternion& qr) const {
		Quaternion q;

		q.w = w * qr.w - x * qr.x - y * qr.y - z * qr.z;
		q.x = w * qr.x + x * qr.w + y * qr.z - z * qr.y;
		q.y = w * qr.y - x * qr.z + y * qr.w + z * qr.x;
		q.z = w * qr.z + x * qr.y - y * qr.x + z * qr.w;

		return q;

	}

	Quaternion Quaternion::operator* (const float& f) const {
		Quaternion q;

		q.w = w * f;
		q.x = x * f;
		q.y = y * f;
		q.z = z * f;

		return q;
	}

	Quaternion& Quaternion::operator*= (const Quaternion& qr) {
		Quaternion q;

		q.w = w * qr.w - x * qr.x - y * qr.y - z * qr.z;
		q.x = w * qr.x + x * qr.w + y * qr.z - z * qr.y;
		q.y = w * qr.y - x * qr.z + y * qr.w + z * qr.x;
		q.z = w * qr.z + x * qr.y - y * qr.x + z * qr.w;

		*this = q;
		return *this;
	}

	Quaternion& Quaternion::operator*= (const float& f) {

		w *= f;
		x *= f;
		y *= f;
		z *= f;

		return *this;
	}
}