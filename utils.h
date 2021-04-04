#pragma once

#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"



namespace F4VRBody {

	 double vec3_len(NiPoint3 v1);
	 NiPoint3 vec3_norm(NiPoint3 v1);

	 double vec3_dot(NiPoint3 v1, NiPoint3 v2);
	 
	 NiPoint3 vec3_cross(NiPoint3 v1, NiPoint3 v2);



}
