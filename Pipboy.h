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

		inline bool status() const {
			return _pipboyStatus;
		}

		inline bool isOperatingPipboy() const {
			return c_IsOperatingPipboy; 
		}

		void swapPipboy();
		void replaceMeshes(bool force);
		void onUpdate();
		void onSetNodes();
		void operatePipBoy();

	private:
		Skeleton* playerSkelly;
		OpenVRHookManagerAPI* vrhook;
		bool meshesReplaced = false;
		
		bool _stickypip;
		bool _pipboyStatus = false;
		int _pipTimer = 0;
		uint64_t _startedLookingAtPip = 0;
		uint64_t _lastLookingAtPip = 0;
		NiTransform _pipboyScreenPrevFrame;

		// Cylons Vars
		bool _stickyoffpip = false;
		bool _stickybpip = false;
		bool stickyPBlight = false;
		bool stickyPBRadio = false;
		bool _PBConfigSticky = false;
		bool _PBControlsSticky[7] = { false, false, false,  false,  false,  false, false };
		bool _SwithLightButtonSticky = false;
		bool _SwitchLightHaptics = true;
		bool _UISelectSticky = false;
		bool _UIAltSelectSticky = false;
		UInt32 LastPipboyPage = 0;
		float lastRadioFreq = 0.0;
		bool c_IsOperatingPipboy = false;
		bool isWeaponinHand = false;
		bool weaponStateDetected = false;
		// End

		void replaceMeshes(std::string itemHide, std::string itemShow);
		void pipboyManagement();
		void dampenPipboyScreen();
		bool isLookingAtPipBoy();
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	extern Pipboy* g_pipboy;
}