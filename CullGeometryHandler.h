#pragma once

namespace F4VRBody {
	void cullPlayerGeometry();
	void restoreGeometry();

	// TODO: remove this in favor of ini setting
	void dumpGeometryArray(StaticFunctionTag* base);
}