#pragma once

#include <optional>
#include <vector>
#include <f4se/NiTypes.h>

namespace common {
	// string related functions
	std::string str_tolower(std::string s);
	std::string ltrim(std::string s);
	std::string rtrim(std::string s);
	std::string trim(const std::string& s);

	// 3D space related functions
	float vec3Len(const NiPoint3& v1);
	NiPoint3 vec3Norm(NiPoint3 v1);
	float vec3Dot(const NiPoint3& v1, const NiPoint3& v2);
	NiPoint3 vec3Cross(const NiPoint3& v1, const NiPoint3& v2);
	float vec3Det(NiPoint3 v1, NiPoint3 v2, NiPoint3 n);
	float degreesToRads(float deg);
	float radsToDegrees(float rad);
	NiPoint3 rotateXY(NiPoint3 vec, float angle);
	NiPoint3 pitchVec(NiPoint3 vec, float angle);
	NiMatrix43 getRotationAxisAngle(NiPoint3 axis, float theta);
	bool isCameraLookingAtObject(const NiTransform& cameraTrans, const NiTransform& objectTrans, float detectThresh);

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
	uint64_t nowMillis();
	std::string toStringWithPrecision(double value, int precision = 2);
	std::string getCurrentTimeString();
	std::vector<std::string> loadListFromFile(const std::string& filePath);
	void windowFocus(const std::string& name);

	// Comparator
	struct CaseInsensitiveComparator {
		bool operator()(const std::string& a, const std::string& b) const noexcept {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		}
	};
}
