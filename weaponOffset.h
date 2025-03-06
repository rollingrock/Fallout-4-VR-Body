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
		NiTransform WeaponOffset::getPipboyOffset();
		void WeaponOffset::savePipboyOffset(NiTransform someData);

		std::optional<NiTransform> getOffset(const std::string& name, const Mode mode = normal);
		void addOffset(const std::string &name, NiTransform someData, const Mode mode = normal);
		void deleteOffset(const std::string& name, const Mode mode = normal);
		std::size_t getSize();

		std::map<std::string, NiTransform> offsets;
	};

	static const std::string powerArmorSuffix{ "-PowerArmor" };
	static const std::string offHandSuffix{ "-offHand" };
	constexpr const char* PIPBOY_HOLO_OFFSETS_PATH = ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets\\HoloPipboyPosition.json";
	constexpr const char* PIPBOY_SCREEN_OFFSETS_PATH = ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets\\PipboyPosition.json";
	static const std::string offsetsPath{ ".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets" };

	void loadWeaponOffsetsJsons();
	void loadWeaponOffsetJsonFile(const std::string& file);
	void saveWeaponOffsetsJsons();
	void saveOffsetJsonFile(const nlohmann::json& weaponJson, const std::string& file);

	extern WeaponOffset* g_weaponOffsets;
}



