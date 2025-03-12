#pragma once

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/NiNodes.h"

namespace F4VRBody {
	// TODO: remove this in favor of ini setting
	void dumpGeometryArray(StaticFunctionTag* base);

	class CullGeometryHandler
	{
	public:
		void cullPlayerGeometry();
	
	private:
		void restoreGeometry();
		void preProcessHideGeometryIndexes(BSFadeNode* rn);

		time_t _lastPreProcessTime = 0;
		std::vector<int> _hideFaceSkinGeometryIndexes;
	};
}