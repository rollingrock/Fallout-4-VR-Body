# AGENTS.md

This file provides guidance to Codex (Codex.ai/code) when working with code in this repository.

## What This Is

FRIK (Fallout 4 VR Body) is a Fallout 4 VR mod that adds full-body IK, weapon repositioning, an in-VR Pipboy, and hand-pose / finger control. It builds as an F4SE plugin DLL (`FRIK.dll`).

The project is built on top of [F4VR-CommonFramework](external/F4VR-CommonFramework/AGENTS.md), which is included as a git submodule and provides the plugin lifecycle (`ModBase`), config base (`ConfigBase`), VR controller framework (`vrcf`), VR UI widgets (`vrui`), and game/skeleton helpers (`f4vr`, `f4sevr`). Reading the framework's `AGENTS.md` is the fastest way to understand the namespaces (`f4cf::`, `f4cf::f4vr`, `f4cf::vrcf`, `f4cf::vrui`, `f4cf::common`) and the `ModBase` lifecycle hooks.

## Build

**Prerequisites:**
- `VCPKG_ROOT` environment variable must point to a vcpkg installation
- Visual Studio 2022 (v143) or 2026 (v145), x64
- CMake 4.2+
- Init submodules: `git submodule update --init --recursive`

**Generate solution:**
```
cmake --preset default        # uses vs2026 by default
```
For local development, copy `CMakeUserPresets.json.template` → `CMakeUserPresets.json` and set:
- `POST_BUILD_COPY_PLUGIN: true` and `COPY_PLUGIN_BASE_PATH` to your MO2 mod folder(s) (semicolon-separated for multiple) — this auto-copies `FRIK.dll` + `.pdb` to `<path>/F4SE/Plugins/` after every build.
- `F4VR_COMMON_FRAMEWORK_PATH` to point to a sibling checkout of F4VR-CommonFramework if you want to develop against it instead of the submodule.

**Build (and ALWAYS check the output before reporting done):**
```
cmake --build build 2>&1 | tee build_output.txt
```
Then read `build_output.txt`. Release builds also produce a versioned `.7z` package in `build/package/`.

## Architecture

### Entry point and global

[src/FRIK.cpp](src/FRIK.cpp) defines `F4SEPlugin_Query`/`F4SEPlugin_Load`, which delegate to `f4cf::g_mod`. The mod itself is the `frik::FRIK` class in [src/FRIK.h](src/FRIK.h), instantiated as the global singleton `frik::g_frik`. It extends `f4cf::ModBase` and overrides:
- `onModLoaded` — installs game hooks via `frik::hook::patchAll()` / `hookMain()` ([src/GameHooks.h](src/GameHooks.h))
- `onGameLoaded` — registers Papyrus natives, detects Fallout London VR / BetterScopesVR, adds embedded flashlight keyword
- `onGameSessionLoaded` — fires on new game and every save load; releases skeleton and reconfigures game INI vars
- `onFrameUpdate` — drives the per-frame update of every subsystem

`FRIK` owns the lifetimes of `Skeleton`, `Pipboy`, `ConfigurationMode`, and `WeaponPositionAdjuster`. They are created lazily in `initSkeleton()` once the game's player + camera + bone-tree nodes are all available (`isGameReadyForSkeletonInitialization`), and destroyed/recreated on power-armor change, root-node release, or loading-screen transitions (`releaseSkeleton`).

### Frame update flow (every frame)

`FRIK::onFrameUpdate()` runs in this order — order matters because later subsystems read state set by earlier ones:
1. Validate root node and power-armor state (rebuild skeleton if either changed)
2. `Skeleton::onFrameUpdate` — IK, body posture, walking, hand pose ([src/skeleton/Skeleton.cpp](src/skeleton/Skeleton.cpp))
3. `BoneSpheresHandler::onFrameUpdate` — interaction-sphere collision/events
4. `PlayerControlsHandler::onFrameUpdate` — enable/disable player movement based on UI state
5. `WeaponPositionAdjuster::onFrameUpdate` — weapon offsets, offhand grip, reposition mode
6. `Pipboy::onFrameUpdate` — wrist Pipboy logic, finger interaction, flashlight
7. `vrui::g_uiManager->onFrameUpdate` — VR UI widgets (driven by `FrameUpdateContext` adapter that exposes the offhand index-finger tip and hand-pointing pose)
8. `MainConfigMode` + `ConfigurationMode` — config UI rendering
9. `updateWorldFinal` — three engine-level scene-graph updates (`BSFadeNode_MergeWorldBounds`, `BSFlattenedBoneTree_UpdateBoneArray`, `BSFadeNode_UpdateGeomArray`) needed for cull geometry, finger position, and Pipboy interaction to work correctly

`FRIK::smoothMovement` is invoked from a separate hook (not from `onFrameUpdate`).

### Subsystem map

| Subsystem | Location | Responsibility |
|-----------|----------|----------------|
| Skeleton/IK | [src/skeleton/](src/skeleton/) | Body IK, leg walking, head/neck posture, arm solver, fingers via `HandPose` |
| Hand pose | [src/skeleton/HandPose.cpp](src/skeleton/HandPose.cpp), [HandPoseData.cpp](src/skeleton/HandPoseData.cpp) | Tagged hand-pose override stack with priority; supports Dynamic, Override, and PrimaryWeaponPose sources |
| Pipboy | [src/pipboy/](src/pipboy/) | Wrist & holo Pipboy, physical (finger-touch) interaction, flashlight |
| Weapon positioning | [src/weapon-position/](src/weapon-position/) | Per-weapon offsets, offhand-grip two-handed mode, in-game reposition tool |
| Smooth movement | [src/smooth-movement/](src/smooth-movement/) | Reduce VR motion sickness on locomotion |
| Config UI | [src/config-mode/](src/config-mode/) | In-VR config menus (`MainConfigMode`, `ConfigurationMode`, `BodyAdjustmentSubConfigMode`) |
| Reload | [src/reload/](src/reload/) | Two-handed gun reload interaction |
| Public API | [src/api/FRIKApi.h](src/api/FRIKApi.h) | C ABI exposed to other mods (loaded via `GetProcAddress` on `FRIKAPI_GetApi`) |
| Papyrus API | [src/PapyrusApi.h](src/PapyrusApi.h) | Native functions for in-game scripts |

### Config

[src/Config.h](src/Config.h) / [src/Config.cpp](src/Config.cpp) extends `f4cf::ConfigBase`. Files live under `%USERPROFILE%\Documents\My Games\Fallout4VR\FRIK_Config\`:
- `FRIK.ini` — main settings (also embedded as RCDATA `IDR_FRIK_INI`, extracted on first run)
- `FRIK_FOLVR.ini` — Fallout London VR overrides, merged on top when that mod is detected
- `Mesh_Hide/face.ini`, `skins.ini`, `slots.ini` — geometry hide lists
- `Pipboy_Offsets/*.json` — per-Pipboy-style world offsets
- `Weapons_Offsets/*.json` — per-weapon offsets, by mode (`WeaponOffsetsMode`: Weapon / PrimaryHand / OffHand / Throwable / BackOfHandUI) with `_PA` and `_left` suffix variants

`g_config.fEqual()` and friends — see "Conventions" below for float comparisons. The base class auto-reloads on file change via the file watcher.

### Public C API for other mods

`frik::api::FRIKApi` ([src/api/FRIKApi.h](src/api/FRIKApi.h)) is a struct of C function pointers exposed via the exported `FRIKAPI_GetApi` ([src/exports.def](src/exports.def)). Other mods copy the header into their project, call `FRIKApi::initialize()`, and use `inst->setHandPose(...)`, `inst->getIndexFingerTipPosition(...)`, `inst->blockOffHandWeaponGripping(...)`, etc. **Bump `FRIK_API_VERSION` whenever you change the struct layout** — clients version-check against it. Hand poses use a string `tag` so multiple systems can layer overrides without clobbering each other.

For mods that want a button in FRIK's main config menu: they call `registerOpenModSettingButtonToMainConfig`; FRIK dispatches an F4SE message back to them when the button is clicked. Sender name is `F4VRBody` (also exposed as `BETTER_SCOPES_VR_MOD_NAME`/`FRIK_F4SE_MOD_NAME`).

### External mod integrations

- **BetterScopesVR** — registers as a message listener at startup; messages of type 15 update `_isLookingThroughScope`, which gates dampening behavior.
- **Fallout London VR** — detected via `isFalloutLondonVRModLoaded()`; loads `FRIK_FOLVR.ini` overrides and switches Pipboy to "Attaboy" mode. Can be force-disabled with `ignoreFalloutLondonVR`.
- **Immersive Flashlight VR** — if loaded, FRIK skips its embedded flashlight to avoid conflict.

## Conventions

- **Build verification:** Always build via `cmake --build build 2>&1 | tee build_output.txt` and read `build_output.txt` before reporting a change as done. Don't skip this step.
- **Float comparisons:** Use `common::fEqual()` (from `F4VR-CommonFramework`'s `common/CommonUtils.h`) instead of `==` / `!= 0.0f` against floats.
- **Code style:** clang-format enforced (LLVM-based, 180-col, 4-space indent, CRLF, pointer-left, namespace indentation). C++23, MSVC `/W4 /WX` (linker), `_UNICODE`, `/permissive-`, all common `/Zc:` conformance flags.
- **PCH:** [src/PCH.h](src/PCH.h) is precompiled — `F4SE/F4SE.h`, `RE/Fallout.h`, `REL/Relocation.h`, `Logger.h`, `Version.h`, and `using namespace f4cf` are implicit in every TU.
- **Logging:** Use `logger::trace/debug/info/warn/error` (spdlog wrappers). `logger::sample(ms, ...)` rate-limits noisy logs to once per `ms`.
- **No in-source builds:** CMake hard-fails if you try.
