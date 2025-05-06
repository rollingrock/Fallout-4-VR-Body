#pragma once

#include <chrono>
#include "utils.h"

namespace frik {
	enum ReloadState {
		idle,
		reloadingStart,
		newMagReady,
		magInserted
	};

	class GunReload {
	public:
		GunReload() {
			startAnimCap = false;
			state = idle;
			reloadButtonPressed = false;
		}

		void startAnimationCapture() {
			startAnimCap = !startAnimCap; // hook gets called twice once at the start of reload and once after animation is done
			startCapTime = std::chrono::high_resolution_clock::now();
		}

		void DoAnimationCapture() const;
		void Update();

		bool StartReloading();
		bool SetAmmoMesh();

	private:
		std::chrono::high_resolution_clock::time_point startCapTime;
		bool startAnimCap;
		ReloadState state;
		bool reloadButtonPressed;
		TESAmmo* currentAmmo{nullptr};
		NiNode* magMesh{nullptr};
		TESObjectREFR* currentRefr{nullptr};
	};

	extern GunReload* g_gunReloadSystem;
	extern float g_animDeltaTime;

	inline void InitGunReloadSystem() {
		g_gunReloadSystem = new GunReload();
	}
}
