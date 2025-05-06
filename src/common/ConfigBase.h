#pragma once

#include <fstream>

#include "Logger.h"
#include "../include/json.hpp"
#include "../include/SimpleIni.h"

using json = nlohmann::json;

namespace common {
	constexpr auto INI_SECTION_DEBUG = "Debug";

	class ConfigBase {
	public:
		ConfigBase(std::string iniFilePath, const WORD iniDefaultConfigEmbeddedResourceId, const int latestVersion)
			: _iniFilePath(std::move(iniFilePath)),
			  _iniDefaultConfigEmbeddedResourceId(iniDefaultConfigEmbeddedResourceId),
			  _iniConfigLatestVersion(latestVersion) {}

		virtual ~ConfigBase() = default;

		// Can be used to test things at runtime during development
		// i.e. check "debugFlowFlag==1" somewhere in code and use config reload to change the value at runtime.
		float debugFlowFlag1 = 0;
		float debugFlowFlag2 = 0;
		float debugFlowFlag3 = 0;

		int getAutoReloadConfigInterval() const {
			return _reloadConfigInterval;
		}

		void toggleAutoReloadConfig() {
			_reloadConfigInterval = _reloadConfigInterval == 0 ? 5 : 0;
			saveIniConfigValue(INI_SECTION_DEBUG, "ReloadConfigInterval", std::to_string(_reloadConfigInterval).c_str());
		}

		/**
		 * Check if debug data dump is requested for the given name.
		 * If matched, the name will be removed from the list to prevent multiple dumps.
		 * Also saved into INI to prevent reloading the same dump name on next config reload.
		 * Support specifying multiple names by any separator as only the matched sub-string is removed.
		 */
		bool checkDebugDumpDataOnceFor(const char* name) {
			const auto idx = _debugDumpDataOnceNames.find(name);
			if (idx == std::string::npos) {
				return false;
			}
			_debugDumpDataOnceNames = _debugDumpDataOnceNames.erase(idx, strlen(name));
			// write to INI for auto-reload not to re-enable it
			saveIniConfigValue(INI_SECTION_DEBUG, "DebugDumpDataOnceNames", _debugDumpDataOnceNames.c_str());

			Log::info("---- Debug Dump Data check passed for '%s' ----", name);
			return true;
		}

		/**
		 * Runs on every game frame.
		 * Used to reload the config file if the reload interval has passed.
		 */
		void onUpdateFrame() {
			try {
				if (_reloadConfigInterval <= 0) {
					return;
				}

				const auto now = std::time(nullptr);
				if (now - _lastReloadTime < _reloadConfigInterval) {
					return;
				}

				Log::verbose("Reloading INI config file...");
				_lastReloadTime = now;
				reloadConfig();
				Log::setLogLevel(_logLevel);
			} catch (const std::exception& e) {
				Log::warn("Failed to reload INI config file: %s", e.what());
			}
		}

	protected:
		/**
		 * Live reload of config
		 */
		virtual void reloadConfig() = 0;

		/**
		 * Override to load your config values
		 */
		virtual void loadIniConfigInternal(const CSimpleIniA& ini) = 0;

		/**
		 * Override to save your config values
		 */
		virtual void saveIniConfigInternal(CSimpleIniA& ini) const = 0;

		/**
		 * Load all INI config values.
		 * If INI file doesn't exist it will be created from the default embedded resource.
		 * If the found INI version is not the latest it will run update to the latest using embedded resource.
		 */
		void loadIniConfig() {
			// create FRIK.ini if it doesn't exist
			createFileFromResourceIfNotExists(_iniFilePath, _iniDefaultConfigEmbeddedResourceId, true);

			CSimpleIniA ini;
			const SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
			if (rc < 0) {
				throw std::runtime_error("Failed to load INI config file! Error: " + std::to_string(rc));
			}

			loadIniConfigValues(ini);

			if (_iniConfigVersion < _iniConfigLatestVersion) {
				Log::info("Updating INI config version %d -> %d", _iniConfigVersion, _iniConfigLatestVersion);
				updateIniConfigToLatestVersion();
				// reload the config after update
				loadIniConfigValues(ini);
			}
		}

		void loadIniConfigValues(const CSimpleIniA& ini) {
			_iniConfigVersion = ini.GetLongValue(INI_SECTION_DEBUG, "Version", 0);
			_logLevel = ini.GetLongValue(INI_SECTION_DEBUG, "LogLevel", 3);
			_reloadConfigInterval = ini.GetLongValue(INI_SECTION_DEBUG, "ReloadConfigInterval", 3);
			debugFlowFlag1 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "DebugFlowFlag1", 0));
			debugFlowFlag2 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "DebugFlowFlag2", 0));
			debugFlowFlag3 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "DebugFlowFlag3", 0));
			_debugDumpDataOnceNames = ini.GetValue(INI_SECTION_DEBUG, "DebugDumpDataOnceNames", "");

			// set log after loading from config
			Log::setLogLevel(_logLevel);

			// let inherited class load all its values
			loadIniConfigInternal(ini);
		}

		/**
		 * Save the config values into the INI config file.
		 * Load the file first to never lose existing values.
		 */
		void saveIniConfig() const {
			CSimpleIniA ini;
			SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
			if (rc < 0) {
				Log::warn("Failed to open INI config for saving with code: %d", rc);
				return;
			}

			// let inherited class save all its values
			saveIniConfigInternal(ini);

			rc = ini.SaveFile(_iniFilePath.c_str());
			if (rc < 0) {
				Log::error("Config: Failed to save FRIK.ini. Error: %d", rc);
			} else {
				Log::info("Config: Saving INI config successful");
			}
		}

		/**
		 * Save specific key and bool value into FRIK.ini file.
		 */
		void saveIniConfigValue(const char* section, const char* key, const bool value) const {
			Log::info("Config: Saving \"%s = %s\"", key, value ? "true" : "false");
			CSimpleIniA ini;
			SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
			if (rc < 0) {
				Log::warn("Failed to save INI config value with code: %d", rc);
				return;
			}
			ini.SetBoolValue(section, key, value);
			rc = ini.SaveFile(_iniFilePath.c_str());
			if (rc < 0) {
				Log::warn("Failed to save INI config value with code: %d", rc);
			}
		}

		/**
		 * Save specific key and double value into FRIK.ini file.
		 */
		void saveIniConfigValue(const char* section, const char* key, const float value) const {
			Log::info("Config: Saving \"%s = %f\"", key, value);
			CSimpleIniA ini;
			SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
			if (rc < 0) {
				Log::warn("Failed to save INI config value with code: %d", rc);
				return;
			}
			ini.SetDoubleValue(section, key, value);
			rc = ini.SaveFile(_iniFilePath.c_str());
			if (rc < 0) {
				Log::warn("Failed to save INI config value with code: %d", rc);
			}
		}

		/**
		 * Save specific key and string value into FRIK.ini file.
		 */
		void saveIniConfigValue(const char* section, const char* key, const char* value) const {
			Log::info("Config: Saving \"%s = %s\"", key, value);
			CSimpleIniA ini;
			SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
			if (rc < 0) {
				Log::warn("Failed to save INI config value with code: %d", rc);
				return;
			}
			ini.SetValue(section, key, value);
			rc = ini.SaveFile(_iniFilePath.c_str());
			if (rc < 0) {
				Log::warn("Failed to save INI config value with code: %d", rc);
			}
		}

		/**
		 * Load offset data from given json file path and store it in the given map.
		 * Use the entry key in the json file but for everything to work properly the name of the json should match the key.
		 */
		static void loadOffsetJsonFile(const std::string& file, std::unordered_map<std::string, NiTransform>& offsetsMap) {
			std::ifstream inF;
			inF.open(file, std::ios::in);
			if (inF.fail()) {
				Log::warn("cannot open %s", file.c_str());
				inF.close();
				return;
			}

			json weaponJson;
			try {
				inF >> weaponJson;
			} catch (json::parse_error& ex) {
				Log::info("cannot open %s: parse error at byte %d", file.c_str(), ex.byte);
				inF.close();
				return;
			}
			inF.close();

			loadOffsetJsonToMap(weaponJson, offsetsMap);
		}

		/**
		 * Load all embedded in resources offsets in the given resource range.
		 */
		static std::unordered_map<std::string, NiTransform> loadEmbeddedOffsets(const WORD fromResourceId, const WORD toResourceId) {
			std::unordered_map<std::string, NiTransform> offsets;
			for (WORD resourceId = fromResourceId; resourceId <= toResourceId; resourceId++) {
				auto resourceOpt = getEmbeddedResourceAsStringIfExists(resourceId);
				if (resourceOpt.has_value()) {
					json json = json::parse(resourceOpt.value());
					loadOffsetJsonToMap(json, offsets);
				}
			}
			return offsets;
		}

		/**
		 * Load all the offsets found in json files in a specific folder.
		 */
		static std::unordered_map<std::string, NiTransform> loadOffsetsFromFilesystem(const std::string& path) {
			std::unordered_map<std::string, NiTransform> offsets;
			for (const auto& file : std::filesystem::directory_iterator(path)) {
				if (file.exists() && !file.is_directory()) {
					loadOffsetJsonFile(file.path().string(), offsets);
				}
			}
			return offsets;
		}

		/**
		 * Save the given offsets transform to a json file using the given name.
		 */
		static void saveOffsetsToJsonFile(const std::string& name, const NiTransform& transform, const std::string& file) {
			Log::info("Saving offsets '%s' to '%s'", name.c_str(), file.c_str());
			json offsetJson;
			offsetJson[name]["rotation"] = transform.rot.arr;
			offsetJson[name]["x"] = transform.pos.x;
			offsetJson[name]["y"] = transform.pos.y;
			offsetJson[name]["z"] = transform.pos.z;
			offsetJson[name]["scale"] = transform.scale;

			std::ofstream outF;
			outF.open(file, std::ios::out);
			if (outF.fail()) {
				Log::info("cannot open '%s' for writing", file.c_str());
				return;
			}
			try {
				outF << std::setw(4) << offsetJson;
				outF.close();
			} catch (std::exception& e) {
				outF.close();
				Log::warn("Unable to save json '%s': %s", file.c_str(), e.what());
			}
		}

		/**
		 * Load the given json object with offset data into an offset map.
		 */
		static void loadOffsetJsonToMap(const ::json& json, std::unordered_map<std::string, NiTransform>& offsetsMap) {
			for (auto& [key, value] : json.items()) {
				NiTransform data;
				for (int i = 0; i < 12; i++) {
					data.rot.arr[i] = value["rotation"][i].get<float>();
				}
				data.pos.x = value["x"].get<float>();
				data.pos.y = value["y"].get<float>();
				data.pos.z = value["z"].get<float>();
				data.scale = value["scale"].get<float>();
				offsetsMap[key] = data;
			}
		}

		/**
		 * Current FRIK.ini file is older. Need to update it by:
		 * 1. Overriding the file with the default FRIK.ini resource.
		 * 2. Saving the current config values read from previous FRIK.ini to the new FRIK.ini file.
		 * This preserves the user changed values, including new values and comments, and remove old values completely.
		 * A backup of the previous file is created with the version number for safety.
		 */
		void updateIniConfigToLatestVersion() const {
			CSimpleIniA oldIni;
			SI_Error rc = oldIni.LoadFile(_iniFilePath.c_str());
			if (rc < 0) {
				throw std::runtime_error("Failed to load old FRIK.ini file! Error: " + std::to_string(rc));
			}

			// override the file with the default FRIK.ini resource.
			const auto tmpIniPath = std::string(_iniFilePath) + ".tmp";
			createFileFromResourceIfNotExists(tmpIniPath, _iniDefaultConfigEmbeddedResourceId, true);

			CSimpleIniA newIni;
			rc = newIni.LoadFile(tmpIniPath.c_str());
			if (rc < 0) {
				throw std::runtime_error("Failed to load new FRIK.ini file! Error: " + std::to_string(rc));
			}

			// remove temp ini file
			auto res = std::remove(tmpIniPath.c_str());
			if (res != 0) {
				Log::warn("Failed to remove temp INI config with code: %d", res);
			}

			// update all values in the new ini with the old ini values but only if they exist in the new
			std::list<CSimpleIniA::Entry> sectionsList;
			oldIni.GetAllSections(sectionsList);
			for (const auto& section : sectionsList) {
				std::list<CSimpleIniA::Entry> keysList;
				oldIni.GetAllKeys(section.pItem, keysList);
				for (const auto& key : keysList) {
					const auto oldVal = oldIni.GetValue(section.pItem, key.pItem);
					const auto newVal = newIni.GetValue(section.pItem, key.pItem);
					if (newVal != nullptr && std::strcmp(oldVal, newVal) != 0) {
						Log::info("Migrating %s.%s = %s", section.pItem, key.pItem, oldIni.GetValue(section.pItem, key.pItem));
						newIni.SetValue(section.pItem, key.pItem, oldIni.GetValue(section.pItem, key.pItem));
					} else {
						Log::verbose("Skipping %s.%s (%s)", section.pItem, key.pItem, newVal == nullptr ? "removed" : "unchanged");
					}
				}
			}

			// set the version to latest
			newIni.SetLongValue(INI_SECTION_DEBUG, "Version", _iniConfigLatestVersion);

			// backup the old ini file before overwriting
			auto nameStr = std::string(_iniFilePath);
			nameStr = nameStr.replace(nameStr.length() - 4, 4, "_bkp_v" + std::to_string(_iniConfigVersion) + ".ini");
			res = std::rename(_iniFilePath.c_str(), nameStr.c_str());
			if (res != 0) {
				Log::warn("Failed to backup old FRIK.ini file to '%s'. Error: %d", nameStr.c_str(), res);
			}

			// save the new ini file
			rc = newIni.SaveFile(_iniFilePath.c_str());
			if (rc < 0) {
				throw std::runtime_error("Failed to save post update FRIK.ini file! Error: " + std::to_string(rc));
			}

			Log::info("FRIK.ini updated successfully");
		}

		// Reload config interval in seconds (0 - no reload)
		// TODO: change to filesystem watch
		int _reloadConfigInterval = 0;
		time_t _lastReloadTime = 0;

	private:
		// location of ini config on disk
		std::string _iniFilePath;

		// resource id to use for default ini config
		WORD _iniDefaultConfigEmbeddedResourceId;

		// current INI config version to handle updates
		int _iniConfigLatestVersion;

		// The INI config version to handle updates/migrations
		int _iniConfigVersion = 0;

		// The log level to set for the logger
		int _logLevel = 0;

		std::string _debugDumpDataOnceNames;
	};
}
