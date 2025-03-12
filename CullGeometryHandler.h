#pragma once

#include "utils.h"

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

	// Not a fan of globals but it may be easiest to refactor code right now
	extern CullGeometryHandler* g_cullGeometry;

	static void initCullGeometryHandler() {
		if (g_cullGeometry) {
			delete g_cullGeometry;
		}

		_MESSAGE("Init configuration mode...");
		g_cullGeometry = new CullGeometryHandler();
	}
}