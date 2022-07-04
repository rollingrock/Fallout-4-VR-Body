#pragma once

#include "f4se/NiObjects.h"
#include <optional>




namespace F4VRBody {

	class WeaponOffset {
	public:

		WeaponOffset() {
			offsets.clear();
		}

		std::optional<NiTransform> getOffset(const std::string &name);
		void addOffset(const std::string &name, NiTransform someData);
		void deleteOffset(const std::string& name);

		std::map<std::string, NiTransform> offsets;
	};

	void readOffsetJson();
	void writeOffsetJson();

	extern WeaponOffset* g_weaponOffsets;
}



