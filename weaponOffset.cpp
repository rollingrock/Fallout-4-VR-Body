#include "weaponOffset.h"

#include <iostream>
#include <fstream>
#include <filesystem>

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

	std::size_t WeaponOffset::getSize() {
		return offsets.size();
	}

	void readOffsetJson() {
		if (g_weaponOffsets) {
			delete g_weaponOffsets;
		}
		g_weaponOffsets = new WeaponOffset();
		loadOffsetJsonFile(); //load default list if it exists
		if (!(std::filesystem::exists(offsetsPath) && std::filesystem::is_directory(offsetsPath)))
			std::filesystem::create_directory(offsetsPath);
		for (const auto& file : std::filesystem::directory_iterator(offsetsPath)) {
			std::wstring path = L"";
			try {
				path = file.path().wstring();
			}
			catch (std::exception& e) {
				_WARNING("Unable to convert path to string: %s", e.what());
			}
			if (file.exists() && !file.is_directory())
				loadOffsetJsonFile(file.path().string());
			}
	}
	void loadOffsetJsonFile(const std::string &file) {

		json weaponJson;
		std::ifstream inF;

		inF.open(file, std::ios::in);

		if (inF.fail()) {
			_WARNING("cannot open %s", file.c_str());
			inF.close();
			return;
		}
		try
		{
			inF >> weaponJson;
		}
		catch (json::parse_error& ex)
		{
			_MESSAGE("cannot open %s: parse error at byte %d", file.c_str(), ex.byte);
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
		_MESSAGE("Successfully loaded %d offsets from %s: total %d", weaponJson.size(), file.c_str(), g_weaponOffsets->getSize());
	}

	void saveOffsetJsonFile(const json& weaponJson, const std::string& file) {
		std::ofstream outF;

		outF.open(file, std::ios::out);

		if (outF.fail()) {
			_MESSAGE("cannot open %s for writing", file.c_str());
			return;
		}

		try {
			outF << std::setw(4) << weaponJson;
			outF.close();
		}catch (std::exception& e) {
			outF.close();
			_WARNING("Unable to save json %s: %s", file.c_str(), e.what());
		}
	}
	void writeOffsetJson() {

		std::string file;

		for (auto& item : g_weaponOffsets->offsets) {
			json weaponJson;
			weaponJson[item.first]["rotation"] = item.second.rot.arr;
			weaponJson[item.first]["x"] = item.second.pos.x;
			weaponJson[item.first]["y"] = item.second.pos.y;
			weaponJson[item.first]["z"] = item.second.pos.z;
			weaponJson[item.first]["scale"] = item.second.scale;
			saveOffsetJsonFile(weaponJson, offsetsPath + "\\" + item.first + ".json");
		}
	}
}
