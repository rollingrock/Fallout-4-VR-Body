#pragma once

#include "f4se/NiObjects.h"




namespace F4VRBody {

	class WeaponOffset {
	public:

		WeaponOffset() {
			offsets.clear();
		}

		NiTransform getOffset(std::string name);
		void addOffset(std::string name, NiTransform someData);

		std::map<std::string, NiTransform> offsets;
	};

	void readOffsetJson();
	void writeOffsetJson();

	extern WeaponOffset* g_weaponOffsets;
}



