#include "Quaternion.h"

#include <math.h>

#include "utils.h"

namespace FRIK {
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

	double Quaternion::dot(Quaternion q) {
		return w * q.w + x * q.x + y * q.y + z * q.z;
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

	float Quaternion::getAngleFromAxisAngle(Quaternion target) {
		Quaternion ret;

		ret = target * this->conjugate();

		return 2 * acosf(ret.w);
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
		//float tr = 0.0f;

		//if (rot.data[2][2] < 0) {
		//	if (rot.data[0][0] > rot.data[1][1]) {
		//		tr = 1 + rot.data[0][0] - rot.data[1][1] - rot.data[2][2];
		//		q = Quaternion(tr, rot.data[0][1] + rot.data[1][0], rot.data[2][0] + rot.data[0][2], rot.data[2][1] - rot.data[1][2]);
		//	}
		//	else {
		//		tr = 1 - rot.data[0][0] + rot.data[1][1] - rot.data[2][2];
		//		q = Quaternion(rot.data[0][1] + rot.data[1][0], tr, rot.data[1][2] + rot.data[2][1], rot.data[0][2] - rot.data[2][0]);
		//	}
		//}
		//else {
		//	if (rot.data[0][0] < -rot.data[1][1]) {
		//		tr = 1 - rot.data[0][0] - rot.data[1][1] + rot.data[2][2];
		//		q = Quaternion(rot.data[2][0] + rot.data[0][2], rot.data[1][2] + rot.data[2][1], tr, rot.data[1][0] - rot.data[0][1]);
		//	}
		//	else {
		//		tr = 1 + rot.data[0][0] + rot.data[1][1] + rot.data[2][2];
		//		q = Quaternion(rot.data[2][1] - rot.data[1][2], rot.data[2][0] - rot.data[0][2], rot.data[1][0] - rot.data[0][1], tr);
		//	}
		//}

		//q *= 0.5 / sqrtf(tr);
		//w = q.w;
		//x = q.x;
		//y = q.y;
		//z = q.z;

		q.w = sqrtf(((std::max)(0.0f, 1 + rot.data[0][0] + rot.data[1][1] + rot.data[2][2]))) / 2;
		q.x = sqrtf(((std::max)(0.0f, 1 + rot.data[0][0] - rot.data[1][1] - rot.data[2][2]))) / 2;
		q.y = sqrtf(((std::max)(0.0f, 1 - rot.data[0][0] + rot.data[1][1] - rot.data[2][2]))) / 2;
		q.z = sqrtf(((std::max)(0.0f, 1 - rot.data[0][0] - rot.data[1][1] + rot.data[2][2]))) / 2;

		w = q.w;
		x = _copysign(q.x, rot.data[2][1] - rot.data[1][2]);
		y = _copysign(q.y, rot.data[0][2] - rot.data[2][0]);
		z = _copysign(q.z, rot.data[1][0] - rot.data[0][1]);
	}

	// slerp function adapted from VRIK - credit prog for math

	void Quaternion::slerp(float interp, Quaternion target) {
		Quaternion save = this->get();

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
		float theta_0 = acosf(dotp); // theta_0 = angle between input vectors
		float theta = theta_0 * interp; // theta = angle between q1 and result
		float sin_theta = sinf(theta); // compute this value only once
		float sin_theta_0 = sinf(theta_0); // compute this value only once
		float s0 = cosf(theta) - dotp * sin_theta / sin_theta_0; // == sin(theta_0 - theta) / sin(theta_0)
		float s1 = sin_theta / sin_theta_0;

		w = s0 * w + s1 * target.w;
		x = s0 * x + s1 * target.x;
		y = s0 * y + s1 * target.y;
		z = s0 * z + s1 * target.z;
	}

	void Quaternion::vec2vec(NiPoint3 v1, NiPoint3 v2) {
		NiPoint3 cross = vec3_cross(vec3_norm(v1), vec3_norm(v2));

		float dotP = vec3_dot(vec3_norm(v1), vec3_norm(v2));

		if (dotP > 0.99999999) {
			this->makeIdentity();
			return;
		}
		if (dotP < -0.99999999) {
			// reverse it
			cross = vec3_norm(vec3_cross(NiPoint3(0, 1, 0), v1));
			if (vec3_len(cross) < 0.00000001) {
				cross = vec3_norm(vec3_cross(NiPoint3(1, 0, 0), v1));
			}
			this->setAngleAxis(PI, cross);
			this->normalize();
			return;
		}

		w = sqrt(pow(vec3_len(v1), 2) * pow(vec3_len(v2), 2)) + dotP;
		x = cross.x;
		y = cross.y;
		z = cross.z;

		this->normalize();
	}

	Quaternion Quaternion::operator*(const Quaternion& qr) const {
		Quaternion q;

		q.w = w * qr.w - x * qr.x - y * qr.y - z * qr.z;
		q.x = w * qr.x + x * qr.w + y * qr.z - z * qr.y;
		q.y = w * qr.y - x * qr.z + y * qr.w + z * qr.x;
		q.z = w * qr.z + x * qr.y - y * qr.x + z * qr.w;

		return q;
	}

	Quaternion Quaternion::operator*(const float& f) const {
		Quaternion q;

		q.w = w * f;
		q.x = x * f;
		q.y = y * f;
		q.z = z * f;

		return q;
	}

	Quaternion& Quaternion::operator*=(const Quaternion& qr) {
		Quaternion q;

		q.w = w * qr.w - x * qr.x - y * qr.y - z * qr.z;
		q.x = w * qr.x + x * qr.w + y * qr.z - z * qr.y;
		q.y = w * qr.y - x * qr.z + y * qr.w + z * qr.x;
		q.z = w * qr.z + x * qr.y - y * qr.x + z * qr.w;

		*this = q;
		return *this;
	}

	Quaternion& Quaternion::operator*=(const float& f) {
		w *= f;
		x *= f;
		y *= f;
		z *= f;

		return *this;
	}

	void Quaternion::operator=(const Quaternion& q) {
		w = q.w;
		x = q.x;
		y = q.y;
		z = q.z;
	}
}
