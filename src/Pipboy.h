#pragma once

#include "Skeleton.h"

namespace frik {
	/**
	 * Handle Pipboy:
	 * 1. On wrist UI override.
	 * 2. Hand interaction with on-wrist Pipboy.
	 * 3. Flashlight on-wrist / head location toggle.
	 */
	class Pipboy {
	public:
		explicit Pipboy(Skeleton* skelly) {
			_skelly = skelly;
		}

		bool status() const {
			return _pipboyStatus;
		}

		/**
		 * True if on-wrist Pipboy is open.
		 */
		bool isOperatingPipboy() const {
			return _isOperatingPipboy;
		}

		void turnOn();
		void swapPipboy();
		void replaceMeshes(bool force);
		void onUpdate();
		void onSetNodes();
		void operatePipBoy();

	private:
		void replaceMeshes(const std::string& itemHide, const std::string& itemShow);
		void pipboyManagement();
		void dampenPipboyScreen();
		bool isLookingAtPipBoy() const;
		void rightStickXSleep(int time);
		void rightStickYSleep(int time);
		void secondaryTriggerSleep(int time);

		Skeleton* _skelly;

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
		bool _PBControlsSticky[7] = {false, false, false, false, false, false, false};
		bool _SwithLightButtonSticky = false;
		bool _SwitchLightHaptics = true;
		bool _UISelectSticky = false;
		bool _UIAltSelectSticky = false;
		bool _isOperatingPipboy = false;
		bool _isWeaponinHand = false;
		bool _weaponStateDetected = false;
		UInt32 _lastPipboyPage = 0;
		float lastRadioFreq = 0.0;

		bool _controlSleepStickyX = false;
		bool _controlSleepStickyY = false;
		bool _controlSleepStickyT = false;
	};
}
