#pragma once

#include <atomic>
#include <fstream>
#include <SimpleIni.h>
#include <nlohmann/json.hpp>
#include <thomasmonkman-filewatch/FileWatch.hpp>

#include "CommonUtils.h"
#include "Logger.h"

using json = nlohmann::json;

namespace common
{
    static const auto BASE_PATH = getRelativePathInDocuments(R"(\My Games\Fallout4VR\Mods_Config)");
    constexpr auto INI_SECTION_DEBUG = "Debug";

    class ConfigBase
    {
    public:
        ConfigBase(const std::string_view& iniFilePath, const WORD iniDefaultConfigEmbeddedResourceId) :
            _iniFilePath(iniFilePath),
            _iniDefaultConfigEmbeddedResourceId(iniDefaultConfigEmbeddedResourceId) {}

        virtual ~ConfigBase() = default;

        /**
         * Load the config from the INI file.
         */
        virtual void load()
        {
            logger::info("Load ini config...");
            createDirDeep(_iniFilePath);
            loadIniConfig();
        }

        // Can be used to test things at runtime during development
        // i.e. check "debugFlowFlag==1" somewhere in code and use config reload to change the value at runtime.
        float debugFlowFlag1 = 0;
        float debugFlowFlag2 = 0;
        float debugFlowFlag3 = 0;
        std::string debugFlowText1 = "";
        std::string debugFlowText2 = "";

        /**
         * Check if debug data dump is requested for the given name.
         * If matched, the name will be removed from the list to prevent multiple dumps.
         * Also saved into INI to prevent reloading the same dump name on next config reload.
         * Support specifying multiple names by any separator as only the matched sub-string is removed.
         */
        bool checkDebugDumpDataOnceFor(const char* name)
        {
            const auto idx = _debugDumpDataOnceNames.find(name);
            if (idx == std::string::npos) {
                return false;
            }
            _debugDumpDataOnceNames = _debugDumpDataOnceNames.erase(idx, strlen(name));
            // write to INI for auto-reload not to re-enable it
            saveIniConfigValue(INI_SECTION_DEBUG, "sDebugDumpDataOnceNames", _debugDumpDataOnceNames.c_str());

            logger::info("---- Debug Dump Data check passed for '{}' ----", name);
            return true;
        }

    protected:
        /**
         * Override to load your config values
         */
        virtual void loadIniConfigInternal(const CSimpleIniA& ini) = 0;

        /**
         * Override to save your config values
         */
        virtual void saveIniConfigInternal(CSimpleIniA&) {}

        /**
         * Load all INI config values.
         * If INI file doesn't exist it will be created from the default embedded resource.
         * If the found INI version is not the latest it will run update to the latest using embedded resource.
         */
        void loadIniConfig()
        {
            // create .ini if it doesn't exist
            createFileFromResourceIfNotExists(_iniFilePath, _iniDefaultConfigEmbeddedResourceId, true);

            loadIniConfigValues();

            const auto iniConfigLatestVersion = loadEmbeddedResourceIniConfigVersion();
            if (_iniConfigVersion < iniConfigLatestVersion) {
                logger::info("Updating INI config version {} -> {}", _iniConfigVersion, iniConfigLatestVersion);
                updateIniConfigToLatestVersion(_iniConfigVersion, iniConfigLatestVersion);

                // reload the config after update
                loadIniConfigValues();
            }

            startIniConfigFileWatch();
        }

        /**
         * Get the latest version of the embedded INI config resource to know if update migration is required.
         */
        int loadEmbeddedResourceIniConfigVersion() const
        {
            const auto embeddedIniStr = getEmbededResourceAsString(_iniDefaultConfigEmbeddedResourceId);

            CSimpleIniA ini;
            const SI_Error rc = ini.LoadData(embeddedIniStr);
            if (rc < 0) {
                logger::warn("Failed to load INI config file! Error:", rc);
                throw std::runtime_error("Failed to load INI config file! Error: " + std::to_string(rc));
            }

            return ini.GetLongValue(INI_SECTION_DEBUG, "iVersion", 0);
        }

        /**
         * Load all the config values from INI config file, override all existing values in the instance.
         * This code should be safe to run multiple times as changes are loaded from disk.
         */
        void loadIniConfigValues()
        {
            CSimpleIniA ini;
            const SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to load INI config file! Error:", rc);
                throw std::runtime_error("Failed to load INI config file! Error: " + std::to_string(rc));
            }

            _iniConfigVersion = ini.GetLongValue(INI_SECTION_DEBUG, "iVersion", 0);
            _logLevel = ini.GetLongValue(INI_SECTION_DEBUG, "iLogLevel", 2);
            _logPattern = ini.GetValue(INI_SECTION_DEBUG, "sLogPattern", "%H:%M:%S.%e %L: %v");
            debugFlowFlag1 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "fDebugFlowFlag1", 0));
            debugFlowFlag2 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "fDebugFlowFlag2", 0));
            debugFlowFlag3 = static_cast<float>(ini.GetDoubleValue(INI_SECTION_DEBUG, "fDebugFlowFlag3", 0));
            debugFlowText1 = ini.GetValue(INI_SECTION_DEBUG, "sDebugFlowText1", "");
            debugFlowText2 = ini.GetValue(INI_SECTION_DEBUG, "sDebugFlowText2", "");
            _debugDumpDataOnceNames = ini.GetValue(INI_SECTION_DEBUG, "sDebugDumpDataOnceNames", "");

            // set log after loading from config
            logger::setLogLevelAndPattern(_logLevel, _logPattern);

            // let inherited class load all its values
            loadIniConfigInternal(ini);
        }

        /**
         * Save the config values into the INI config file.
         * Load the file first to never lose existing values.
         */
        void saveIniConfig()
        {
            CSimpleIniA ini;
            SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to open INI config for saving with code: {}", rc);
                return;
            }

            // let inherited class save all its values
            saveIniConfigInternal(ini);

            _ignoreNextIniFileChange.store(true);
            rc = ini.SaveFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::error("Config: Failed to save .ini. Error: {}", rc);
            } else {
                logger::info("Config: Saving INI config successful");
            }
        }

        /**
         * Save specific key and bool value into .ini file.
         */
        void saveIniConfigValue(const char* section, const char* key, const bool value)
        {
            logger::info("Config: Saving \"{} = {}\"", key, value ? "true" : "false");
            CSimpleIniA ini;
            SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to save INI config value with code: {}", rc);
                return;
            }
            ini.SetBoolValue(section, key, value);
            _ignoreNextIniFileChange.store(true);
            rc = ini.SaveFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to save INI config value with code: {}", rc);
            }
        }

        /**
         * Save specific key and bool value into .ini file.
         */
        void saveIniConfigValue(const char* section, const char* key, const int value)
        {
            logger::info("Config: Saving \"{} = {}\"", key, value);
            CSimpleIniA ini;
            SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to save INI config value with code: {}", rc);
                return;
            }
            ini.SetLongValue(section, key, value);
            _ignoreNextIniFileChange.store(true);
            rc = ini.SaveFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to save INI config value with code: {}", rc);
            }
        }

        /**
         * Save specific key and double value into .ini file.
         */
        void saveIniConfigValue(const char* section, const char* key, const float value)
        {
            logger::info("Config: Saving \"{} = {}\"", key, value);
            CSimpleIniA ini;
            SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to save INI config value with code: {}", rc);
                return;
            }
            ini.SetDoubleValue(section, key, value);
            _ignoreNextIniFileChange.store(true);
            rc = ini.SaveFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to save INI config value with code: {}", rc);
            }
        }

        /**
         * Save specific key and string value into .ini file.
         */
        void saveIniConfigValue(const char* section, const char* key, const char* value)
        {
            logger::info("Config: Saving \"{} = {}\"", key, value);
            CSimpleIniA ini;
            SI_Error rc = ini.LoadFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to save INI config value with code: {}", rc);
                return;
            }
            ini.SetValue(section, key, value);
            _ignoreNextIniFileChange.store(true);
            rc = ini.SaveFile(_iniFilePath.c_str());
            if (rc < 0) {
                logger::warn("Failed to save INI config value with code: {}", rc);
            }
        }

        /**
         * Load offset data from given json file path and store it in the given map.
         * Use the entry key in the json file but for everything to work properly the name of the json should match the key.
         */
        static void loadOffsetJsonFile(const std::string& file, std::unordered_map<std::string, RE::NiTransform>& offsetsMap)
        {
            try {
                std::ifstream inF;
                inF.open(file, std::ios::in);
                if (inF.fail()) {
                    logger::warn("cannot open {}", file.c_str());
                    inF.close();
                    return;
                }

                json weaponJson;
                try {
                    inF >> weaponJson;
                } catch (json::parse_error& ex) {
                    logger::info("cannot open {}: parse error at byte {}", file.c_str(), ex.byte);
                    inF.close();
                    return;
                }
                inF.close();

                loadOffsetJsonToMap(weaponJson, offsetsMap);
            } catch (std::exception& ex) {
                std::throw_with_nested(std::runtime_error(fmt::format("Failed to load offset json from file '{}':\n\t{}", file.c_str(), ex.what())));
            }
        }

        /**
         * Load all embedded in resources offsets in the given resource range.
         */
        static std::unordered_map<std::string, RE::NiTransform> loadEmbeddedOffsets(const WORD fromResourceId, const WORD toResourceId)
        {
            std::unordered_map<std::string, RE::NiTransform> offsets;
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
        static std::unordered_map<std::string, RE::NiTransform> loadOffsetsFromFilesystem(const std::string& path)
        {
            std::unordered_map<std::string, RE::NiTransform> offsets;
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
        static void saveOffsetsToJsonFile(const std::string& name, const RE::NiTransform& transform, const std::string& file)
        {
            logger::info("Saving offsets '{}' to '{}'", name.c_str(), file.c_str());
            json offsetJson;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 4; j++) {
                    offsetJson[name]["rotation"].push_back(transform.rotate[i][j]);
                }
            }
            offsetJson[name]["x"] = transform.translate.x;
            offsetJson[name]["y"] = transform.translate.y;
            offsetJson[name]["z"] = transform.translate.z;
            offsetJson[name]["scale"] = transform.scale;

            std::ofstream outF;
            outF.open(file, std::ios::out);
            if (outF.fail()) {
                logger::info("cannot open '{}' for writing", file.c_str());
                return;
            }
            try {
                outF << std::setw(4) << offsetJson;
                outF.close();
            } catch (std::exception& e) {
                outF.close();
                logger::warn("Unable to save json '{}': {}", file.c_str(), e.what());
            }
        }

        /**
         * Load the given json object with offset data into an offset map.
         */
        static void loadOffsetJsonToMap(const json& json, std::unordered_map<std::string, RE::NiTransform>& offsetsMap)
        {
            try {
                for (auto& [key, value] : json.items()) {
                    RE::NiTransform data;
                    for (int i = 0; i < 3; i++) {
                        for (int j = 0; j < 4; j++) {
                            data.rotate[i][j] = value["rotation"][i * 4 + j].get<float>();
                        }
                    }
                    data.translate.x = value["x"].get<float>();
                    data.translate.y = value["y"].get<float>();
                    data.translate.z = value["z"].get<float>();
                    data.scale = value["scale"].get<float>();
                    offsetsMap[key] = data;
                }
            } catch (std::exception& ex) {
                std::throw_with_nested(std::runtime_error(fmt::format("Failed to load offset json:\n\t{}", ex.what())));
            }
        }

        /**
         * Current .ini file is older. Need to update it by:
         * 1. Overriding the file with the default .ini resource.
         * 2. Saving the current config values read from previous .ini to the new .ini file.
         * This preserves the user changed values, including new values and comments, and remove old values completely.
         * A backup of the previous file is created with the version number for safety.
         */
        void updateIniConfigToLatestVersion(const int currentVersion, const int latestVersion) const
        {
            CSimpleIniA oldIni;
            SI_Error rc = oldIni.LoadFile(_iniFilePath.c_str());
            if (rc < 0) {
                throw std::runtime_error("Failed to load old .ini file! Error: " + std::to_string(rc));
            }

            // override the file with the default .ini resource.
            const auto tmpIniPath = std::string(_iniFilePath) + ".tmp";
            createFileFromResourceIfNotExists(tmpIniPath, _iniDefaultConfigEmbeddedResourceId, true);

            CSimpleIniA newIni;
            rc = newIni.LoadFile(tmpIniPath.c_str());
            if (rc < 0) {
                throw std::runtime_error("Failed to load new .ini file! Error: " + std::to_string(rc));
            }

            // remove temp ini file
            auto res = std::remove(tmpIniPath.c_str());
            if (res != 0) {
                logger::warn("Failed to remove temp INI config with code: {}", res);
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
                        logger::info("Migrating {}.{} = {}", section.pItem, key.pItem, oldIni.GetValue(section.pItem, key.pItem));
                        newIni.SetValue(section.pItem, key.pItem, oldIni.GetValue(section.pItem, key.pItem));
                    } else {
                        logger::debug("Skipping {}.{} ({})", section.pItem, key.pItem, newVal == nullptr ? "removed" : "unchanged");
                    }
                }
            }

            // set the version to latest
            newIni.SetLongValue(INI_SECTION_DEBUG, "iVersion", latestVersion);

            updateIniConfigToLatestVersionCustom(currentVersion, latestVersion, oldIni, newIni);

            // backup the old ini file before overwriting
            auto nameStr = std::string(_iniFilePath);
            nameStr = nameStr.replace(nameStr.length() - 4, 4, "_backup_v" + std::to_string(_iniConfigVersion) + ".ini");
            res = std::rename(_iniFilePath.c_str(), nameStr.c_str());
            if (res != 0) {
                logger::warn("Failed to backup old .ini file to '{}'. Error: {}", nameStr.c_str(), res);
            }

            // save the new ini file
            rc = newIni.SaveFile(_iniFilePath.c_str());
            if (rc < 0) {
                throw std::runtime_error("Failed to save post update .ini file! Error: " + std::to_string(rc));
            }

            logger::info(".ini updated successfully");
        }

        /**
         * Custom code to migrate the INI config to the latest version.
         * Can be used is special handling is required for the specific config.
         */
        virtual void updateIniConfigToLatestVersionCustom(int /*currentVersion*/, int /*latestVersion*/, const CSimpleIniA& /*oldIni*/, CSimpleIniA& /*newIni*/) const {}

        /**
         * Setup filesystem watch on INI config file to reload config when changes are detected.
         * Handling duplicate modified events from file-watcher:
         * There can be 3-5 events fired for 1 change. Sometimes the last even can be a full second after a change.
         * To prevent it we check the file last write time and ignore events that
         */
        void startIniConfigFileWatch()
        {
            if (_iniConfigFileWatch) {
                return;
            }
            // use thread as otherwise there is a deadlock
            std::thread([this]() {
                logger::info("Start file watch in INI config '{}'", _iniFilePath.c_str());
                _iniConfigFileWatch = std::make_unique<filewatch::FileWatch<std::string>>(
                    _iniFilePath, [this](const std::string&, const filewatch::Event changeType) {
                        if (changeType != filewatch::Event::modified) {
                            return;
                        }

                        constexpr auto delay = std::chrono::milliseconds(200);

                        // ignore duplicate modified events, use atomic to make sure only 1 thread gets through
                        auto prevWriteTime = _lastIniFileWriteTime.load();
                        std::error_code ec;
                        const auto writeTime = fs::last_write_time(_iniFilePath, ec);
                        if (ec || !_lastIniFileWriteTime.compare_exchange_strong(prevWriteTime, writeTime) || writeTime - prevWriteTime < delay) {
                            logger::debug("Ignore INI config change duplicate (write: {}) (err:{})", writeTime.time_since_epoch().count(), ec.value());
                            return;
                        }

                        // ignore file modified if we who modified it
                        bool expected = true;
                        if (_ignoreNextIniFileChange.compare_exchange_strong(expected, false)) {
                            logger::debug("Ignoring INI config change by ignore flag");
                            return;
                        }

                        // wait until delay time is passed since the LAST file write time to prevent file lock issues and rapid modifications
                        auto now = fs::file_time_type::clock::now();
                        auto lastEventTime = _lastIniFileWriteTime.load();
                        while (now - lastEventTime < delay) {
                            std::this_thread::sleep_for(max(std::chrono::milliseconds(0), delay - (now - lastEventTime)));
                            now = fs::file_time_type::clock::now();
                            lastEventTime = _lastIniFileWriteTime.load();
                        }

                        logger::info("INI config change detected ({}), reload...", toDateTimeString(writeTime));
                        loadIniConfigValues();
                    });
            }).detach();
        }

    private:
        // location of ini config on disk
        std::string _iniFilePath;

        // resource id to use for default ini config
        WORD _iniDefaultConfigEmbeddedResourceId;

        // The INI config version to handle updates/migrations
        int _iniConfigVersion = 0;

        // The log level to set for the logger
        int _logLevel = 0;

        // the log message pattern to use for the logger
        std::string _logPattern;

        std::string _debugDumpDataOnceNames;

        // filesystem watch for changes to INI config file to have live reload
        std::unique_ptr<filewatch::FileWatch<std::string>> _iniConfigFileWatch;

        // INI config file last write time to prevent reload the same change because of OS multiple events
        std::atomic<std::filesystem::file_time_type> _lastIniFileWriteTime;

        // Handle ignoring file watch change event IFF the change was made by us
        std::atomic<bool> _ignoreNextIniFileChange = false;
    };
}
