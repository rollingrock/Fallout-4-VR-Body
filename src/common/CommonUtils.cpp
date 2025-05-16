#include "CommonUtils.h"

#include <filesystem>
#include <numbers>
#include <shlobj_core.h>
#include <fstream>
#include <chrono>

#include "Logger.h"
#include "Matrix.h"

namespace common {
	std::string str_tolower(std::string s) {
		std::ranges::transform(s, s.begin(),
			[](const unsigned char c) { return std::tolower(c); }
		);
		return s;
	}

	std::string ltrim(std::string s) {
		s.erase(s.begin(), std::ranges::find_if(s, [](const unsigned char ch) {
			return !std::isspace(ch);
		}));
		return s;
	}

	std::string rtrim(std::string s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](const unsigned char ch) {
			return !std::isspace(ch);
		}).base(), s.end());
		return s;
	}

	std::string trim(const std::string& s) {
		return ltrim(rtrim(s));
	}

	float vec3Len(const NiPoint3& v1) {
		return sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
	}

	NiPoint3 vec3Norm(NiPoint3 v1) {
		const float mag = vec3Len(v1);

		if (mag < 0.000001) {
			const float maxX = abs(v1.x);
			const float maxY = abs(v1.y);
			const float maxZ = abs(v1.z);

			if (maxX >= maxY && maxX >= maxZ) {
				return v1.x >= 0 ? NiPoint3(1, 0, 0) : NiPoint3(-1, 0, 0);
			}
			if (maxY > maxZ) {
				return v1.y >= 0 ? NiPoint3(0, 1, 0) : NiPoint3(0, -1, 0);
			}
			return v1.z >= 0 ? NiPoint3(0, 0, 1) : NiPoint3(0, 0, -1);
		}

		v1.x /= mag;
		v1.y /= mag;
		v1.z /= mag;

		return v1;
	}

	float vec3Dot(const NiPoint3& v1, const NiPoint3& v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	NiPoint3 vec3Cross(const NiPoint3& v1, const NiPoint3& v2) {
		return NiPoint3(
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x
		);
	}

	// the determinant is proportional to the sin of the angle between two vectors.   In 3d case find the sin of the angle between v1 and v2
	// along their angle of rotation with unit vector n
	// https://stackoverflow.com/questions/14066933/direct-way-of-computing-clockwise-angle-between-2-vectors/16544330#16544330
	float vec3Det(const NiPoint3 v1, const NiPoint3 v2, const NiPoint3 n) {
		return v1.x * v2.y * n.z + v2.x * n.y * v1.z + n.x * v1.y * v2.z - v1.z * v2.y * n.x - v2.z * n.y * v1.x - n.z * v1.y * v2.x;
	}

	float degreesToRads(const float deg) {
		return deg * std::numbers::pi_v<float> / 180;
	}

	float radsToDegrees(const float rad) {
		return rad * 180 / std::numbers::pi_v<float>;
	}

	NiPoint3 rotateXY(const NiPoint3 vec, const float angle) {
		NiPoint3 retV;

		retV.x = vec.x * cosf(angle) - vec.y * sinf(angle);
		retV.y = vec.x * sinf(angle) + vec.y * cosf(angle);
		retV.z = vec.z;

		return retV;
	}

	NiPoint3 pitchVec(const NiPoint3 vec, const float angle) {
		const auto rotAxis = NiPoint3(vec.y, -vec.x, 0);
		Matrix44 rot;

		rot.makeTransformMatrix(getRotationAxisAngle(vec3Norm(rotAxis), angle), NiPoint3(0, 0, 0));

		return rot.make43() * vec;
	}

	// Gets a rotation matrix from an axis and an angle
	NiMatrix43 getRotationAxisAngle(NiPoint3 axis, const float theta) {
		NiMatrix43 result;
		// This math was found online http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/
		const float c = cosf(theta);
		const float s = sinf(theta);
		const float t = 1.0f - c;
		axis = vec3Norm(axis);
		result.data[0][0] = c + axis.x * axis.x * t;
		result.data[1][1] = c + axis.y * axis.y * t;
		result.data[2][2] = c + axis.z * axis.z * t;
		float tmp1 = axis.x * axis.y * t;
		float tmp2 = axis.z * s;
		result.data[1][0] = tmp1 + tmp2;
		result.data[0][1] = tmp1 - tmp2;
		tmp1 = axis.x * axis.z * t;
		tmp2 = axis.y * s;
		result.data[2][0] = tmp1 - tmp2;
		result.data[0][2] = tmp1 + tmp2;
		tmp1 = axis.y * axis.z * t;
		tmp2 = axis.x * s;
		result.data[2][1] = tmp1 + tmp2;
		result.data[1][2] = tmp1 - tmp2;
		return result.Transpose();
	}

	/**
	 * Check if the camera is looking at the object and the object is facing the camera
	 */
	bool isCameraLookingAtObject(const NiTransform& cameraTrans, const NiTransform& objectTrans, const float detectThresh) {
		// Get the position of the camera and the object
		const auto cameraPos = cameraTrans.pos;
		const auto objectPos = objectTrans.pos;

		// Calculate the direction vector from the camera to the object
		const auto direction = vec3Norm(NiPoint3(objectPos.x - cameraPos.x, objectPos.y - cameraPos.y, objectPos.z - cameraPos.z));

		// Get the forward vector of the camera (assuming it's the y-axis)
		const auto cameraForward = vec3Norm(cameraTrans.rot * NiPoint3(0, 1, 0));

		// Get the forward vector of the object (assuming it's the y-axis)
		const auto objectForward = vec3Norm(objectTrans.rot * NiPoint3(0, 1, 0));

		// Check if the camera is looking at the object
		const float cameraDot = vec3Dot(cameraForward, direction);
		const bool isCameraLooking = cameraDot > detectThresh; // Adjust the threshold as needed

		// Check if the object is facing the camera
		const float objectDot = vec3Dot(objectForward, direction);
		const bool isObjectFacing = objectDot > detectThresh; // Adjust the threshold as needed

		return isCameraLooking && isObjectFacing;
	}

	/**
	 * Find dll embedded resource by id and return its data as string if exists.
	 * Return null if the resource is not found.
	 */
	std::optional<std::string> getEmbeddedResourceAsStringIfExists(const WORD resourceId) {
		// Must specify the dll to read its resources and not the exe
		const HMODULE hModule = GetModuleHandle("FRIK.dll");
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
	std::string getEmbededResourceAsString(const WORD resourceId) {
		// Must specify the dll to read its resources and not the exe
		const HMODULE hModule = GetModuleHandle("FRIK.dll");
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
	void createFileFromResourceIfNotExists(const std::string& filePath, const WORD resourceId, const bool fixNewline) {
		if (std::filesystem::exists(filePath)) {
			return;
		}

		Log::info("Creating '%s' file from resource id: %d...", filePath.c_str(), resourceId);
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

		Log::verbose("File '%s' created successfully (size: %d)", filePath.c_str(), data.size());
	}

	/**
	 * Create a folder structure if it doesn't exist.
	 * Check if the given path ends with a file name and if so, remove it.
	 */
	void createDirDeep(const std::string& pathStr) {
		auto path = std::filesystem::path(pathStr);
		if (path.has_extension()) {
			path = path.parent_path();
		}
		if (!std::filesystem::exists(path)) {
			Log::info("Creating directory: %s", path.string().c_str());
			std::filesystem::create_directories(path);
		}
	}

	/**
	 * Get path in the My Documents folder.
	 */
	std::string getRelativePathInDocuments(const std::string& relPath) {
		char documentsPath[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, 0, documentsPath))) {
			return std::string(documentsPath) + relPath;
		}
		throw std::runtime_error("Failed to get My Documents folder path");
	}

	/**
	 * Safely (no override, no errors) move a file from fromPath to toPath.
	 */
	void moveFileSafe(const std::string& fromPath, const std::string& toPath) {
		try {
			if (!std::filesystem::exists(fromPath)) {
				return;
			}
			if (std::filesystem::exists(toPath)) {
				Log::info("Moving '%s' to '%s' failed, file already exists", fromPath.c_str(), toPath.c_str());
				return;
			}
			Log::info("Moving '%s' to '%s'", fromPath.c_str(), toPath.c_str());
			std::filesystem::rename(fromPath, toPath);
		} catch (const std::exception& e) {
			Log::error("Failed to move file to new location: %s", e.what());
		}
	}

	/**
	 * Safely (no override, no errors) move all files (and files only) in the fromPath to the toPath.
	 */
	void moveAllFilesInFolderSafe(const std::string& fromPath, const std::string& toPath) {
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
	 * Get the current time in milliseconds.
	 */
	uint64_t nowMillis() {
		const auto now = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			now.time_since_epoch()
		).count();
	}

	std::string toStringWithPrecision(const double value, const int precision) {
		std::ostringstream stream;
		stream << std::fixed << std::setprecision(precision) << value;
		return stream.str();
	}

	/**
	 * Get a simple string of the current time in HH:MM:SS format.
	 */
	std::string getCurrentTimeString() {
		const std::time_t now = std::time(nullptr);
		std::tm localTime;
		localtime_s(&localTime, &now);
		char buffer[9];
		std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &localTime);
		return std::string(buffer);
	}

	/**
	 * Loads a list of string values from a file.
	 * Each value is expected to be on a new line.
	 */
	std::vector<std::string> loadListFromFile(const std::string& filePath) {
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

	void windowFocus(const std::string& name) {
		const HWND hwnd = ::FindWindowEx(nullptr, nullptr, name.c_str(), nullptr);
		if (!hwnd) {
			Log::info("Window Not Found");
			return;
		}
		const HWND foreground = GetForegroundWindow();
		if (foreground != hwnd) {
			{
				//PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
				//PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
				SendMessage(foreground, WM_SYSCOMMAND, SC_MINIMIZE, 0); // restore the minimize window
				SendMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0); // restore the minimize window
				SetForegroundWindow(hwnd);
				SetActiveWindow(hwnd);
				SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
			}
		}
	}
}
