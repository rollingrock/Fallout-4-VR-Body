#include "matrix.h"
#include <cmath>
#include "utils.h"

namespace frik {
	void Matrix44::getEulerAngles(float* heading, float* roll, float* attitude) const {
		if (data[2][0] < 1.0) {
			if (data[2][0] > -1.0) {
				*heading = atan2(-data[2][1], data[2][2]);
				*attitude = asin(data[2][0]);
				*roll = atan2(-data[1][0], data[0][0]);
			} else {
				*heading = -atan2(-data[0][1], data[1][1]);
				*attitude = -PI / 2;
				*roll = 0.0;
			}
		} else {
			*heading = atan2(data[0][1], data[1][1]);
			*attitude = PI / 2;
			*roll = 0.0;
		}
	}

	void Matrix44::setEulerAngles(const float heading, const float roll, const float attitude) {
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

	void Matrix44::rotateVectorVec(NiPoint3 toVec, NiPoint3 fromVec) {
		toVec = vec3Norm(toVec);
		fromVec = vec3Norm(fromVec);

		const float dotP = vec3Dot(fromVec, toVec);

		if (dotP >= 0.99999) {
			this->makeIdentity();
			return;
		}

		NiPoint3 crossP = vec3Cross(toVec, fromVec);
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

	NiMatrix43 Matrix44::make43() const {
		NiMatrix43 ret;
		for (auto i = 0; i < 3; i++) {
			for (auto j = 0; j < 3; j++) {
				ret.data[i][j] = this->data[i][j];
			}
		}
		ret.data[0][3] = 0.0f;
		ret.data[1][3] = 0.0f;
		ret.data[2][3] = 0.0f;
		return ret;
	}

	NiMatrix43 Matrix44::multiply43Left(const NiMatrix43& mat) const {
		return mult(mat, this->make43());
	}

	NiMatrix43 Matrix44::multiply43Right(const NiMatrix43& mat) const {
		return mult(this->make43(), mat);
	}

	void Matrix44::matrixMultiply(const Matrix44* worldMat, const Matrix44* retMat, const Matrix44* localMat) {
		// This uses the native transform function that the updateWorld call makes
		using func_t = decltype(&Matrix44::matrixMultiply);
		RelocAddr<func_t> func(0x1a8d60);

		return func(worldMat, retMat, localMat);
	}

	NiMatrix43 Matrix44::mult(const NiMatrix43& left, const NiMatrix43& right) {
		NiMatrix43 tmp;
		// shamelessly taken from SKSE
		tmp.data[0][0] =
			right.data[0][0] * left.data[0][0] +
			right.data[0][1] * left.data[1][0] +
			right.data[0][2] * left.data[2][0];
		tmp.data[1][0] =
			right.data[1][0] * left.data[0][0] +
			right.data[1][1] * left.data[1][0] +
			right.data[1][2] * left.data[2][0];
		tmp.data[2][0] =
			right.data[2][0] * left.data[0][0] +
			right.data[2][1] * left.data[1][0] +
			right.data[2][2] * left.data[2][0];
		tmp.data[0][1] =
			right.data[0][0] * left.data[0][1] +
			right.data[0][1] * left.data[1][1] +
			right.data[0][2] * left.data[2][1];
		tmp.data[1][1] =
			right.data[1][0] * left.data[0][1] +
			right.data[1][1] * left.data[1][1] +
			right.data[1][2] * left.data[2][1];
		tmp.data[2][1] =
			right.data[2][0] * left.data[0][1] +
			right.data[2][1] * left.data[1][1] +
			right.data[2][2] * left.data[2][1];
		tmp.data[0][2] =
			right.data[0][0] * left.data[0][2] +
			right.data[0][1] * left.data[1][2] +
			right.data[0][2] * left.data[2][2];
		tmp.data[1][2] =
			right.data[1][0] * left.data[0][2] +
			right.data[1][1] * left.data[1][2] +
			right.data[1][2] * left.data[2][2];
		tmp.data[2][2] =
			right.data[2][0] * left.data[0][2] +
			right.data[2][1] * left.data[1][2] +
			right.data[2][2] * left.data[2][2];
		tmp.data[0][3] = 0.0f;
		tmp.data[1][3] = 0.0f;
		tmp.data[2][3] = 0.0f;
		return tmp;
	}
}
