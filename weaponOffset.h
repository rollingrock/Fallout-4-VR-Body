#pragma once

#include "f4se/NiObjects.h"
#include <optional>
#include "include/json.hpp"




namespace F4VRBody {

	enum Mode {
		normal = 0,
		powerArmor,
		offHand,
		offHandwithPowerArmor,
	};

	class WeaponOffset {
	public:

		WeaponOffset() {
			offsets.clear();
		}

		std::optional<NiTransform> getOffset(const std::string& name, const Mode mode = normal);
		void addOffset(const std::string &name, NiTransform someData, const Mode mode = normal);
		void deleteOffset(const std::string& name, const Mode mode = normal);
		std::size_t getSize();

		std::map<std::string, NiTransform> offsets;
	};

	static const std::string powerArmorSuffix{ "-PowerArmor" };
	static const std::string offHandSuffix{ "-offHand" };
	static const std::string defaultJson{ ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets.json" };
	static const std::string offsetsPath{ ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets" };

	void loadWeaponOffsetsJsons();
	void loadWeaponOffsetJsonFile(const std::string& file = defaultJson);
	void saveWeaponOffsetsJsons();
	void saveOffsetJsonFile(const nlohmann::json& weaponJson, const std::string& file = defaultJson);

	extern WeaponOffset* g_weaponOffsets;
}



