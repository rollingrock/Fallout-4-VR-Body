#pragma once

#include "f4se/NiObjects.h"
#include <optional>
#include "include/json.hpp"




namespace F4VRBody {

	class WeaponOffset {
	public:

		WeaponOffset() {
			offsets.clear();
		}

		std::optional<NiTransform> getOffset(const std::string &name);
		void addOffset(const std::string &name, NiTransform someData);
		void deleteOffset(const std::string& name);
		std::size_t getSize();

		std::map<std::string, NiTransform> offsets;
	};

	static const std::string defaultJson{ ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets.json" };
	static const std::string offsetsPath{ ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets" };

	void readOffsetJson();
	void loadOffsetJsonFile(const std::string& file = defaultJson);
	void writeOffsetJson();
	void saveOffsetJsonFile(const nlohmann::json& weaponJson, const std::string& file = defaultJson);

	extern WeaponOffset* g_weaponOffsets;
}



