#include "CommonUtils.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <shlobj_core.h>

#include "Logger.h"
#include "Version.h"

namespace common
{
    /**
     * Safe equal check as floats are not exact.
     */
    bool fEqual(const float left, const float right, const float epsilon)
    {
        return std::fabs(left - right) < epsilon;
    }

    bool fNotEqual(const float left, const float right, const float epsilon)
    {
        return std::fabs(left - right) > epsilon;
    }

    std::string str_tolower(std::string s)
    {
        std::ranges::transform(s, s.begin(),
            [](const unsigned char c) { return std::tolower(c); }
            );
        return s;
    }

    std::string ltrim(std::string s)
    {
        s.erase(s.begin(), std::ranges::find_if(s, [](const unsigned char ch) {
            return !std::isspace(ch);
        }));
        return s;
    }

    std::string rtrim(std::string s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
        return s;
    }

    std::string trim(const std::string& s)
    {
        return ltrim(rtrim(s));
    }

    float vec3Len(const RE::NiPoint3& v1)
    {
        return sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
    }

    /**
     * Find dll embedded resource by id and return its data as string if exists.
     * Return null if the resource is not found.
     */
    std::optional<std::string> getEmbeddedResourceAsStringIfExists(const WORD resourceId)
    {
        // Must specify the dll to read its resources and not the exe
        const HMODULE hModule = GetModuleHandleA((std::string(Version::PROJECT) + ".dll").c_str());
        const HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hRes) {
            return std::nullopt;
        }

        const HGLOBAL hResData = LoadResource(hModule, hRes);
        if (!hResData) {
            return std::nullopt;
        }

        const DWORD dataSize = SizeofResource(hModule, hRes);
        const void* pData = LockResource(hResData);
        if (!pData) {
            return std::nullopt;
        }

        return std::string(static_cast<const char*>(pData), dataSize);
    }

    /**
     * Find dll embedded resource by id and return its data as string.
     */
    std::string getEmbededResourceAsString(const WORD resourceId)
    {
        // Must specify the dll to read its resources and not the exe
        const HMODULE hModule = GetModuleHandleA((std::string(Version::PROJECT) + ".dll").c_str());
        const HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hRes) {
            throw std::runtime_error("Resource not found for id: " + std::to_string(resourceId));
        }

        const HGLOBAL hResData = LoadResource(hModule, hRes);
        if (!hResData) {
            throw std::runtime_error("Failed to load resource for id: " + std::to_string(resourceId));
        }

        const DWORD dataSize = SizeofResource(hModule, hRes);
        const void* pData = LockResource(hResData);
        if (!pData) {
            throw std::runtime_error("Failed to lock resource for id: " + std::to_string(resourceId));
        }

        return std::string(static_cast<const char*>(pData), dataSize);
    }

    /**
     * If file at a given path doesn't exist then create it from the embedded resource.
     */
    void createFileFromResourceIfNotExists(const std::string& filePath, const WORD resourceId, const bool fixNewline)
    {
        if (std::filesystem::exists(filePath)) {
            return;
        }

        logger::info("Creating '{}' file from resource id: {}...", filePath.c_str(), resourceId);
        auto data = getEmbededResourceAsString(resourceId);

        if (fixNewline) {
            // Remove all \r to ensure it uses only \n for new lines as ini library creates empty lines
            std::erase(data, '\r');
        }

        std::ofstream outFile(filePath, std::ios::trunc);
        if (!outFile) {
            throw std::runtime_error("Failed to create '" + filePath + "' file");
        }
        if (!outFile.write(data.data(), data.size())) {
            outFile.close();
            std::remove(filePath.c_str());
            throw std::runtime_error("Failed to write to '" + filePath + "' file");
        }
        outFile.close();

        logger::debug("File '{}' created successfully (size: {})", filePath.c_str(), data.size());
    }

    /**
     * Create a folder structure if it doesn't exist.
     * Check if the given path ends with a file name and if so, remove it.
     */
    void createDirDeep(const std::string& pathStr)
    {
        auto path = std::filesystem::path(pathStr);
        if (path.has_extension()) {
            path = path.parent_path();
        }
        if (!std::filesystem::exists(path)) {
            logger::info("Creating directory: {}", path.string().c_str());
            std::filesystem::create_directories(path);
        }
    }

    /**
     * Get path in the My Documents folder.
     */
    std::string getRelativePathInDocuments(const std::string& relPath)
    {
        char documentsPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, 0, documentsPath))) {
            return std::string(documentsPath) + relPath;
        }
        throw std::runtime_error("Failed to get My Documents folder path");
    }

    /**
     * Safely (no override, no errors) move a file from fromPath to toPath.
     */
    void moveFileSafe(const std::string& fromPath, const std::string& toPath)
    {
        try {
            if (!std::filesystem::exists(fromPath)) {
                return;
            }
            if (std::filesystem::exists(toPath)) {
                logger::info("Moving '{}' to '{}' failed, file already exists", fromPath.c_str(), toPath.c_str());
                return;
            }
            logger::info("Moving '{}' to '{}'", fromPath.c_str(), toPath.c_str());
            std::filesystem::rename(fromPath, toPath);
        } catch (const std::exception& e) {
            logger::error("Failed to move file to new location: {}", e.what());
        }
    }

    /**
     * Safely (no override, no errors) move all files (and files only) in the fromPath to the toPath.
     */
    void moveAllFilesInFolderSafe(const std::string& fromPath, const std::string& toPath)
    {
        if (!std::filesystem::exists(fromPath)) {
            return;
        }
        for (const auto& entry : std::filesystem::directory_iterator(fromPath)) {
            if (entry.is_regular_file()) {
                const auto& sourcePath = entry.path();
                const auto destinationPath = toPath / sourcePath.filename();
                moveFileSafe(sourcePath.string(), destinationPath.string());
            }
        }
    }

    /**
     * Wait for the debugger to attach.
     */
    void waitForDebugger()
    {
        while (!IsDebuggerPresent()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /**
     * Get the current time in milliseconds.
     */
    uint64_t nowMillis()
    {
        const auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
            ).count();
    }

    /**
     * Get the current time in nanoseconds.
     */
    uint64_t nowNanosec()
    {
        const auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()
            ).count();
    }

    /**
     * Is the current time is after the start + duration time.
     * Handles if start time is in the past or future.
     */
    bool isNowTimePassed(const uint64_t start, const int duration)
    {
        const uint64_t now = nowMillis();
        return now > start && std::cmp_greater(now - start, duration);
    }

    std::string toStringWithPrecision(const double value, const int precision)
    {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(precision) << value;
        return stream.str();
    }

    /**
     * Get nice formatted date-time string from time point.
     */
    std::string toDateTimeString(const std::filesystem::file_time_type time, const std::string& format)
    {
        const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(time);
        return toDateTimeString(systemTime, format);
    }

    /**
     * Get nice formatted date-time string from time point.
     */
    std::string toDateTimeString(const std::chrono::system_clock::time_point time, const std::string& format)
    {
        const std::time_t timeT = std::chrono::system_clock::to_time_t(time);
        const std::tm tm = *std::localtime(&timeT);
        std::ostringstream oss;
        oss << std::put_time(&tm, format.c_str());
        return oss.str();
    }

    /**
     * Get a simple string of the current time in HH:MM:SS format.
     */
    std::string getCurrentTimeString()
    {
        const std::time_t now = std::time(nullptr);
        std::tm localTime;
        localtime_s(&localTime, &now);
        char buffer[9];
        std::strftime(buffer, sizeof(buffer), "%H:%M:{}", &localTime);
        return std::string(buffer);
    }

    /**
     * Loads a list of string values from a file.
     * Each value is expected to be on a new line.
     */
    std::vector<std::string> loadListFromFile(const std::string& filePath)
    {
        std::ifstream input;
        input.open(filePath);

        std::vector<std::string> list;
        if (input.is_open()) {
            while (input) {
                std::string lineStr;
                input >> lineStr;
                if (!lineStr.empty()) {
                    list.push_back(trim(str_tolower(lineStr)));
                }
            }
        }

        input.close();
        return list;
    }
}
