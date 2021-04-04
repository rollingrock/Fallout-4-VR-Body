#include "utils.h"


namespace F4VRBody {

	double vec3_len(NiPoint3 v1) {

		return sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
	}

	NiPoint3 vec3_norm(NiPoint3 v1) {

		double mag = vec3_len(v1);

		v1.x /= mag;
		v1.y /= mag;
		v1.z /= mag;

		return v1;
	}

	double vec3_dot(NiPoint3 v1, NiPoint3 v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	NiPoint3 vec3_cross(NiPoint3 v1, NiPoint3 v2) {
		NiPoint3 crossP;

		crossP.x = v1.y * v2.z - v1.z * v2.y;
		crossP.y = v1.z * v2.x - v1.x * v2.z;
		crossP.z = v1.x * v2.y - v1.y * v2.x;

		return crossP;
	}
}
