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

	void Matrix44::setEulerAngles(float x, float y, float z) {
		float sinX = sin(x);
		float cosX = cos(x);
		float sinY = sin(y);
		float cosY = cos(y);
		float sinZ = sin(z);
		float cosZ = cos(z);

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

	NiMatrix43 Matrix44::make43() {
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

	NiMatrix43 Matrix44::multiply43Left(NiMatrix43 mat) {
		return this->mult(mat, this->make43());
	}

	NiMatrix43 Matrix44::multiply43Right(NiMatrix43 mat) {
		return this->mult(this->make43(), mat);
	}

	void Matrix44::matrixMultiply(Matrix44* worldMat, Matrix44* retMat, Matrix44* localMat) {   // This uses the native transform function that the updateWorld call makes
		using func_t = decltype(&Matrix44::matrixMultiply);
		RelocAddr<func_t> func(0x1a8d60);

		return func(worldMat, retMat, localMat);
	}

	NiMatrix43 Matrix44::mult(NiMatrix43 left, NiMatrix43 right) {
		NiMatrix43 tmp;
		// shamelessly taken from skse
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