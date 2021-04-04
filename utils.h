#pragma once

#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"

#include "matrix.h"

namespace F4VRBody {

	 float vec3_len(NiPoint3 v1);
	 NiPoint3 vec3_norm(NiPoint3 v1);

	 float vec3_dot(NiPoint3 v1, NiPoint3 v2);
	 
	 NiPoint3 vec3_cross(NiPoint3 v1, NiPoint3 v2);

	 float degrees_to_rads(float deg);

	 NiPoint3 rotateXY(NiPoint3 vec, float angle);

	 NiMatrix43 getRotationAxisAngle(NiPoint3 axis, float theta);

}
