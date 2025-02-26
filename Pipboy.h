#pragma once

#include "Skeleton.h"
#include "weaponOffset.h"

namespace F4VRBody {

	class Pipboy	{
	public:
		Pipboy(Skeleton* skelly, OpenVRHookManagerAPI* hook) {
			playerSkelly = skelly;
			vrhook = hook;
		}

		void Pipboy::replaceMeshes(bool force);
		void Pipboy::onUpdate();
			
	private:
		Skeleton* playerSkelly;
		OpenVRHookManagerAPI* vrhook;
		bool meshesReplaced = false;

		// persistant fields to be used for ipboy configuration menu
		bool c_ExitandSavePressed = false;
		bool c_ExitnoSavePressed = false;
		bool c_SelfieButtonPressed = false;
		bool c_UIHeightButtonPressed = false;
		bool c_DampenHandsButtonPressed = false;
		int c_ConfigModeTimer = 0;
		int c_ConfigModeTimer2 = 0;

		void Pipboy::replaceMeshes(std::string itemHide, std::string itemShow);
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	extern Pipboy* g_pipboy;
}