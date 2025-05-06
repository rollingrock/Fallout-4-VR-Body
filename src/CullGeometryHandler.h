#pragma once

#include <vector>

#include "f4se/NiNodes.h"

namespace frik {
	class CullGeometryHandler {
	public:
		void cullPlayerGeometry();

	private:
		static void restoreGeometry();
		void preProcessHideGeometryIndexes(BSFadeNode* rn);

		time_t _lastPreProcessTime = 0;
		int _lastHiddenGeometryIdx = -1;
		std::string _lastHiddenGeometryName;
		std::vector<int> _hideFaceSkinGeometryIndexes;
	};
}
