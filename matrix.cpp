#include "matrix.h"

#include <math.h>


namespace F4VRBody {

	void Matrix44::getEulerAngles(float* heading, float* roll, float* attitude) {

		if (data[2][0] < 1.0) {
			if (data[2][0] > -1.0) {
				*heading = atan2(-data[2][1], data[2][2]);
				*attitude = asin(data[2][0]);
				*roll = atan2(-data[1][0], data[0][0]);
			}
			else {
				*heading = -atan2(-data[0][1], data[1][1]);
				*attitude = -PI / 2;
				*roll = 0.0;
			}
		}
		else {
			*heading = atan2(data[0][1], data[1][1]);
			*attitude = PI / 2;
			*roll = 0.0;
		}
	}

	void Matrix44::setEulerAngles(float heading, float roll, float attitude) {
		float sinX = sin(heading);
		float cosX = cos(heading);
		float sinY = sin(attitude);
		float cosY = cos(attitude);
		float sinZ = sin(roll);
		float cosZ = cos(roll);

		data[0][0] = cosY * cosZ;
		data[0][1] = sinX * sinY * cosZ + sinZ * cosX;
		data[0][2] = sinX * sinZ - cosX * sinY * cosZ;
		data[1][0] = -cosY * sinZ;
		data[1][1] = cosX * cosZ - sinX * sinY * sinZ;
		data[1][2] = cosX * sinY * sinZ + sinX * cosZ;
		data[2][0] = sinY;
		data[2][1] = -sinX * cosY;
		data[2][2] = cosX * cosY;
	}

	void Matrix44::rotateVectoVec(NiPoint3 toVec, NiPoint3 fromVec) {

		toVec = vec3_norm(toVec);
		fromVec = vec3_norm(fromVec);

		double dotP = vec3_dot(fromVec, toVec);

		if (dotP >= 0.99999) {
			this->makeIdentity();
			return;
		}

		NiPoint3 crossP = vec3_cross(toVec, fromVec);
		crossP = vec3_norm(crossP);

		float phi = acosf(dotP);
		float rcos = cos(phi);
		float rsin = sin(phi);

		// Build the matrix
		data[0][0] = rcos + crossP.x * crossP.x * (1.0 - rcos);
		data[0][1] = -crossP.z * rsin + crossP.x * crossP.y * (1.0 - rcos);
		data[0][2] = crossP.y * rsin + crossP.x * crossP.z * (1.0 - rcos);
		data[1][0] = crossP.z * rsin + crossP.y * crossP.x * (1.0 - rcos);
		data[1][1] = rcos + crossP.y * crossP.y * (1.0 - rcos);
		data[1][2] = -crossP.x * rsin + crossP.y * crossP.z * (1.0 - rcos);
		data[2][0] = -crossP.y * rsin + crossP.z * crossP.x * (1.0 - rcos);
		data[2][1] = crossP.x * rsin + crossP.z * crossP.y * (1.0 - rcos);
		data[2][2] = rcos + crossP.z * crossP.z * (1.0 - rcos);
	}

	NiMatrix43 Matrix44::multiply43Left(NiMatrix43 mat) {
		NiMatrix43 retMat;
		Matrix44* result = new Matrix44;

		matrixMultiply((Matrix44*)&(mat), result, this);

		for (auto i = 0; i < 3; i++) {
			for (auto j = 0; j < 3; j++) {
				retMat.data[i][j] = result->data[i][j];

			}
		}

		retMat.data[0][3] = 0.0f;
		retMat.data[1][3] = 0.0f;
		retMat.data[2][3] = 0.0f;

		return retMat;
	}

	NiMatrix43 Matrix44::multiply43Right(NiMatrix43 mat) {
		NiMatrix43 retMat;
		Matrix44* result = new Matrix44;

		matrixMultiply(this, result, (Matrix44*)&(mat));

		for (auto i = 0; i < 3; i++) {
			for (auto j = 0; j < 3; j++) {
				retMat.data[i][j] = result->data[i][j];

			}
		}

		retMat.data[0][3] = 0.0f;
		retMat.data[1][3] = 0.0f;
		retMat.data[2][3] = 0.0f;

		return retMat;
	}

	void Matrix44::matrixMultiply(Matrix44* worldMat, Matrix44* retMat, Matrix44* localMat) {
		using func_t = decltype(&Matrix44::matrixMultiply);
		RelocAddr<func_t> func(0x1a8d60);

		return func(worldMat, retMat, localMat);
	}

}