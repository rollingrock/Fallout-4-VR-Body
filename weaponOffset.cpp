#include "weaponOffset.h"
#include "Config.h"
#include "utils.h"
#include "resource.h"

#include <iostream>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;


namespace F4VRBody {

	WeaponOffset* g_weaponOffsets = nullptr;

	std::string getSearchName(const std::string& name, const Mode mode) {
		std::string searchName;
		switch (mode) {
		case powerArmor:
			searchName = name + powerArmorSuffix;
			break;
		case offHand:
			searchName = name + offHandSuffix;
			break;
		case offHandwithPowerArmor:
			searchName = name + offHandSuffix + powerArmorSuffix;
			break;
		case normal:
		default:
			searchName = name;
			break;
		}
		return searchName;
	}

	std::optional<NiTransform> WeaponOffset::getOffset(const std::string &name, const Mode mode) {
		auto it = offsets.find(getSearchName(name, mode));
		if (it == offsets.end()) {
			switch (mode) { //check without PA
			case powerArmor:
			case offHandwithPowerArmor:
				return getOffset(name);
			default:
				return { };
			}
		}

		return it->second;
	}

	/// <summary>
	/// Get the Pipboy offset of the currently used Pipboy type.
	/// </summary>
	NiTransform WeaponOffset::getPipboyOffset() {
		return g_config->isHoloPipboy
			? offsets["HoloPipboyPosition"]
			: offsets["PipboyPosition"];
	}

	/// <summary>
	/// Save the Pipboy offset to the offsets map.
	/// </summary>
	void WeaponOffset::savePipboyOffset(NiTransform transform) {
		g_config->isHoloPipboy
			? g_weaponOffsets->addOffset("HoloPipboyPosition", transform, Mode::normal)
			: g_weaponOffsets->addOffset("PipboyPosition", transform, Mode::normal);
		saveWeaponOffsetsJsons();
	}

	void WeaponOffset::addOffset(const std::string &name, NiTransform someData, const Mode mode) {
		offsets[getSearchName(name, mode)] = someData;
	}

	void WeaponOffset::deleteOffset(const std::string& name, const Mode mode) {
		offsets.erase(getSearchName(name, mode));
	}

	std::size_t WeaponOffset::getSize() {
		return offsets.size();
	}

	void loadWeaponOffsetsJsons() {
		if (g_weaponOffsets) {
			delete g_weaponOffsets;
		}

		createFileFromResourceIfNotExists(PIPBOY_HOLO_OFFSETS_PATH, IDR_PIPBOY_HOLO_OFFSETS);
		createFileFromResourceIfNotExists(PIPBOY_SCREEN_OFFSETS_PATH, IDR_PIPBOY_SCREEN_OFFSETS);

		g_weaponOffsets = new WeaponOffset();
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
				loadWeaponOffsetJsonFile(file.path().string());
			}
	}

	void loadWeaponOffsetJsonFile(const std::string& file) {
		std::ifstream inF;
		inF.open(file, std::ios::in);
		if (inF.fail()) {
			_WARNING("cannot open %s", file.c_str());
			inF.close();
			return;
		}

		json weaponJson;
		try {
			inF >> weaponJson;
		}
		catch (json::parse_error& ex) {
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
		}
		catch (std::exception& e) {
			outF.close();
			_WARNING("Unable to save json %s: %s", file.c_str(), e.what());
		}
	}

	void saveWeaponOffsetsJsons() {
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
