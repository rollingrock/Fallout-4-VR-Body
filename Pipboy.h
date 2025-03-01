#pragma once

#include "Skeleton.h"
#include "weaponOffset.h"

namespace F4VRBody {

	/// <summary>
	/// Hnadle Pipboy:
	/// 1. On wrist UI override.
	/// 2. Hand interaction with on-wrist pipboy.
	/// 3. Flashlight on-wrist / head location toggle.
	/// </summary>
	class Pipboy	{
	public:
		Pipboy(Skeleton* skelly, OpenVRHookManagerAPI* hook) {
			_skelly = skelly;
			_vrhook = hook;
		}

		inline bool status() const {
			return _pipboyStatus;
		}

		/// <summary>
		/// True if on-wrist pipboy is open.
		/// </summary>
		inline bool isOperatingPipboy() const {
			return _isOperatingPipboy; 
		}

		void swapPipboy();
		void replaceMeshes(bool force);
		void onUpdate();
		void onSetNodes();
		void operatePipBoy();

	private:
		void replaceMeshes(std::string itemHide, std::string itemShow);
		void pipboyManagement();
		void dampenPipboyScreen();
		bool isLookingAtPipBoy();

		Skeleton* _skelly;
		OpenVRHookManagerAPI* _vrhook;
		
		bool meshesReplaced = false;
		bool _stickypip = false;
		bool _pipboyStatus = false;
		int _pipTimer = 0;
		uint64_t _startedLookingAtPip = 0;
		uint64_t _lastLookingAtPip = 0;
		NiTransform _pipboyScreenPrevFrame;

		// Pipboy interaction with hand variables
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
		bool _isOperatingPipboy = false;
		bool _isWeaponinHand = false;
		bool _weaponStateDetected = false;
		UInt32 _lastPipboyPage = 0;
		float lastRadioFreq = 0.0;
	};

	// Not a fan of globals but it may be easiest to refactor code right now
	extern Pipboy* g_pipboy;
	
	static void initPipboy(Skeleton* skelly, OpenVRHookManagerAPI* hook) {
		if (g_pipboy) {
			_ERROR("ERROR: pipboy already initialized");
			return;
		}

		_MESSAGE("Init pipboy...");
		g_pipboy = new Pipboy(skelly, hook);
	}
}