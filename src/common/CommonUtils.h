#pragma once

#include <optional>
#include <vector>
#include <Windows.h>

namespace common
{
    bool fEqual(float left, float right, float epsilon = 0.00001f);
    bool fNotEqual(float left, float right, float epsilon = 0.00001f);

    // string related functions
    std::string str_tolower(std::string s);
    std::string ltrim(std::string s);
    std::string rtrim(std::string s);
    std::string trim(const std::string& s);

    // Resource related functions
    std::optional<std::string> getEmbeddedResourceAsStringIfExists(WORD resourceId);
    std::string getEmbededResourceAsString(WORD resourceId);
    void createFileFromResourceIfNotExists(const std::string& filePath, WORD resourceId, bool fixNewline);

    // Filesystem related functions
    void createDirDeep(const std::string& pathStr);
    std::string getRelativePathInDocuments(const std::string& relPath);
    void moveFileSafe(const std::string& fromPath, const std::string& toPath);
    void moveAllFilesInFolderSafe(const std::string& fromPath, const std::string& toPath);

    // Miscellaneous functions
    void waitForDebugger();
    uint64_t nowMillis();
    uint64_t nowNanosec();
    bool isNowTimePassed(uint64_t start, int duration);
    std::string toStringWithPrecision(double value, int precision = 2);
    std::string toDateTimeString(std::filesystem::file_time_type time, const std::string& format = "%Y-%m-%d %H:%M:%S");
    std::string toDateTimeString(std::chrono::system_clock::time_point time, const std::string& format = "%Y-%m-%d %H:%M:%S");
    std::string getCurrentTimeString();
    std::vector<std::string> loadListFromFile(const std::string& filePath);

    // Comparator
    struct CaseInsensitiveComparator
    {
        bool operator()(const std::string& a, const std::string& b) const noexcept
        {
            return _stricmp(a.c_str(), b.c_str()) < 0;
        }
    };
}
