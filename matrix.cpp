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

	Matrix44 Matrix44::rotateVectoVec(NiPoint3 toVec, NiPoint3 fromVec) {
		Matrix44 mat;

		toVec = vec3_norm(toVec);
		fromVec = vec3_norm(fromVec);

		double dotP = vec3_dot(fromVec, toVec);

		if (dotP >= 0.99999) {
			mat.makeIdentity();
			return mat;
		}

		NiPoint3 crossP = vec3_cross(toVec, fromVec);
		crossP = vec3_norm(crossP);

		float phi = acosf(dotP);
		float rcos = cos(phi);
		float rsin = sin(phi);

		// Build the matrix
		mat.data[0][0] = rcos + crossP.x * crossP.x * (1.0 - rcos);
		mat.data[0][1] = -crossP.z * rsin + crossP.x * crossP.y * (1.0 - rcos);
		mat.data[0][2] = crossP.y * rsin + crossP.x * crossP.z * (1.0 - rcos);
		mat.data[1][0] = crossP.z * rsin + crossP.y * crossP.x * (1.0 - rcos);
		mat.data[1][1] = rcos + crossP.y * crossP.y * (1.0 - rcos);
		mat.data[1][2] = -crossP.x * rsin + crossP.y * crossP.z * (1.0 - rcos);
		mat.data[2][0] = -crossP.y * rsin + crossP.z * crossP.x * (1.0 - rcos);
		mat.data[2][1] = crossP.x * rsin + crossP.z * crossP.y * (1.0 - rcos);
		mat.data[2][2] = rcos + crossP.z * crossP.z * (1.0 - rcos);

		return mat;
	}
}