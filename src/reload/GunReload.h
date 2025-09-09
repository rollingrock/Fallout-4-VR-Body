// #pragma once
//
// #include "common/CommonUtils.h"
// #include "f4vr/F4SEVRStructs.h"
//
// namespace frik
// {
//     enum ReloadState
//     {
//         idle,
//         reloadingStart,
//         newMagReady,
//         magInserted
//     };
//
//     class GunReload
//     {
//     public:
//         GunReload()
//         {
//             startAnimCap = false;
//             state = idle;
//             reloadButtonPressed = false;
//         }
//
//         void startAnimationCapture()
//         {
//             startAnimCap = !startAnimCap; // hook gets called twice once at the start of reload and once after animation is done
//             startCapTime = common::nowMillis();
//         }
//
//         void DoAnimationCapture() const;
//         void Update();
//
//         bool StartReloading();
//         bool SetAmmoMesh();
//
//     private:
//         uint64_t startCapTime;
//         bool startAnimCap;
//         ReloadState state;
//         bool reloadButtonPressed;
//         RE::TESAmmo* currentAmmo{ nullptr };
//         RE::NiNode* magMesh{ nullptr };
//         F4SEVR::TESObjectREFR* currentRefr{ nullptr };
//     };
//
//     extern GunReload* g_gunReloadSystem;
//     extern float g_animDeltaTime;
//
//     inline void InitGunReloadSystem()
//     {
//         g_gunReloadSystem = new GunReload();
//     }
// }
