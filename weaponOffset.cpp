#include "weaponOffset.h"

#include "include/json.hpp"

#include <iostream>
#include <fstream>

using json = nlohmann::json;


namespace F4VRBody {

	WeaponOffset* g_weaponOffsets = nullptr;

	std::optional<NiTransform> WeaponOffset::getOffset(const std::string &name) {

		auto it = offsets.find(name);
		if (it == offsets.end()) {
			return { };
		}

		return it->second;
	}

	void WeaponOffset::addOffset(const std::string &name, NiTransform someData) {
		offsets[name] = someData;
	}


	void WeaponOffset::deleteOffset(const std::string& name) {
		offsets.erase(name);
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
			_MESSAGE("cannot open FRIK_weapon_offsets.json!!!");
			inF.close();
			return;
		}
		try
		{
			inF >> weaponJson;
		}
		catch (json::parse_error& ex)
		{
			_MESSAGE("cannot open FRIK_weapon_offsets.json: parse error at byte %d", ex.byte);
			inF.close();
			return;
		}
		inF.close();

		NiTransform data;

		for (auto& [key, value] : weaponJson.items()) {
			for (int i = 0; i < 12; i++) {
				data.rot.arr[i] = value["rotation"][i].get<double>();
			}
			data.pos.x = value["x"].get<double>();
			data.pos.y = value["y"].get<double>();
			data.pos.z = value["z"].get<double>();
			data.scale = value["scale"].get<double>();

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
			weaponJson[item.first]["rotation"] = item.second.rot.arr;
			weaponJson[item.first]["x"] = item.second.pos.x;
			weaponJson[item.first]["y"] = item.second.pos.y;
			weaponJson[item.first]["z"] = item.second.pos.z;
			weaponJson[item.first]["scale"] = item.second.scale;
		}

		outF << weaponJson;
		outF.close();
	}

}
