#include "weaponOffset.h"

#include "include/json.hpp"

#include <iostream>
#include <fstream>

using json = nlohmann::json;


namespace F4VRBody {

	WeaponOffset* g_weaponOffsets = nullptr;

	WeaponOffset::Data WeaponOffset::getOffset(std::string name) {

		Data data;

		return data;

	}

	void WeaponOffset::addOffset(std::string name, Data someData) {
		offsets[name] = someData;
	}
		
	void readOffsetJson() {
		if (g_weaponOffsets) {
			delete g_weaponOffsets;
		}
		g_weaponOffsets = new WeaponOffset();

		json weaponJson;
		std::ifstream inF;

		inF.open(".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets.json", std::ios::in);

		if (inF.fail()) {
			_MESSAGE("cannot open FRIK_weapon_offsets.ini!!!");
			return;
		}

		inF >> weaponJson;
		inF.close();

		WeaponOffset::Data data;

		for (auto& [key, value] : weaponJson.items()) {
			data.pitch = value["pitch"].get<double>();
			data.roll = value["roll"].get<double>();
			data.yaw = value["yaw"].get<double>();
			data.offset.x = value["x"].get<double>();
			data.offset.y = value["y"].get<double>();
			data.offset.z = value["z"].get<double>();

			g_weaponOffsets->addOffset(key, data);

		}
	}

	void writeOffsetJson() {

		json weaponJson;
		std::ofstream outF;

		outF.open(".\\Data\\F4SE\\plugins\\FRIK_weapon_offsets.json", std::ios::out);

		if (outF.fail()) {
			_MESSAGE("cannot open FRIK_weapon_offsets.ini for writing!!!");
			return;
		}

		for (auto& item : g_weaponOffsets->offsets) {
			weaponJson[item.first]["pitch"] = item.second.pitch;
			weaponJson[item.first]["roll"] = item.second.roll;
			weaponJson[item.first]["yaw"] = item.second.yaw;
			weaponJson[item.first]["x"] = item.second.offset.x;
			weaponJson[item.first]["y"] = item.second.offset.y;
			weaponJson[item.first]["z"] = item.second.offset.z;
		}

		outF << weaponJson;
		outF.close();
	}

}
