#pragma once

#include <f4se/ScaleformMovie.h>

#include "Skeleton.h"
#include "utils.h"
#include "f4vr/scaleformUtils.h"

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
			// TODO: do we need this?
			turnPipBoyOff();
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
		void onFrameUpdate();
		void onSetNodes();

	private:
		void operatePipBoy();
		static void gotoPrevPage(GFxMovieRoot* root);
		static void gotoNextPage(GFxMovieRoot* root);
		static void gotoPrevTab(GFxMovieRoot* root);
		static void gotoNextTab(GFxMovieRoot* root);
		static void moveListSelectionUpDown(GFxMovieRoot* root, bool moveUp);
		static void handlePrimaryControllerOperationOnStatusPage(GFxMovieRoot* root, bool triggerPressed);
		static void handlePrimaryControllerOperationOnInventoryPage(GFxMovieRoot* root, bool triggerPressed);
		static void handlePrimaryControllerOperationOnDataPage(GFxMovieRoot* root, bool triggerPressed);
		static void handlePrimaryControllerOperationOnMapPage(GFxMovieRoot* root, bool triggerPressed);
		static void handlePrimaryControllerOperationOnRadioPage(GFxMovieRoot* root, bool triggerPressed);
		void storeLastPipboyPage(const GFxMovieRoot* root);
		static void handlePrimaryControllerOperation(GFxMovieRoot* root, bool triggerPressed);
		void replaceMeshes(const std::string& itemHide, const std::string& itemShow);
		void pipboyManagement();
		void dampenPipboyScreen();
		bool isLookingAtPipBoy() const;
		void rightStickXSleep(int time);
		void rightStickYSleep(int time);
		void secondaryTriggerSleep(int time);
		static void fixMissingScreen();

		Skeleton* _skelly;

		bool meshesReplaced = false;
		bool _stickypip = false;
		bool _pipboyStatus = false;
		int _pipTimer = 0;
		uint64_t _startedLookingAtPip = 0;
		uint64_t _lastLookingAtPip = 0;
		RE::NiTransform _pipboyScreenPrevFrame;

		// Pipboy interaction with hand variables
		bool _stickyoffpip = false;
		bool _stickybpip = false;
		bool stickyPBlight = false;
		bool stickyPBRadio = false;
		bool _PBConfigSticky = false;
		bool _PBControlsSticky[7] = {false, false, false, false, false, false, false};
		bool _SwithLightButtonSticky = false;
		bool _SwitchLightHaptics = true;
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
