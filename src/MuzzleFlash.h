#pragma once

#include "f4se/NiNodes.h"


namespace FRIK {

	class MuzzleFlash {
	public:
		uint64_t unk00;
		uint64_t unk08;
		NiNode* fireNode;
		NiNode* projectileNode;
	};

}
