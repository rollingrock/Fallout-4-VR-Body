#pragma once

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/NiNodes.h"

namespace F4VRBody {
	
	class CullGeometryHandler
	{
	public:
		void cullPlayerGeometry();
	
	private:
		void restoreGeometry();
		void preProcessHideGeometryIndexes(BSFadeNode* rn);

		time_t _lastPreProcessTime = 0;
		int _lastHiddenGeometryIdx = -1;
		std::string _lastHiddenGeometryName;
		std::vector<int> _hideFaceSkinGeometryIndexes;
	};
}