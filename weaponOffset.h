#pragma once

#include "f4se/NiObjects.h"




namespace F4VRBody {

	class WeaponOffset {
	public:

		struct Data {
			float pitch;
			float roll;
			float yaw;
			NiPoint3 offset;
		};

		WeaponOffset() {
			offsets.clear();
		}

		Data getOffset(std::string name);
		void addOffset(std::string name, Data someData);

		std::map<std::string, Data> offsets;
	};

	void readOffsetJson();
	void writeOffsetJson();

	extern WeaponOffset* g_weaponOffsets;
}



