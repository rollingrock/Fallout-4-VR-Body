# FRIK API

FRIK exposes a small **C ABI** that other F4SE plugins can use to read FRIK state and drive a few of its systems — hand poses, the index-fingertip position, offhand weapon gripping, subsystem on/off control, live config overrides, and a button in FRIK's main config menu.

The API is defined in a single header, [src/api/FRIKApi.h](../src/api/FRIKApi.h). Copy that header into your project **as-is** and call into FRIK through the exported `FRIKAPI_GetApi` function. No linking against FRIK is required — the header resolves everything at runtime via `GetModuleHandle` / `GetProcAddress`.

## How it works

- FRIK exports a single C function, `FRIKAPI_GetApi`, which returns a pointer to a `FRIKApi` struct of function pointers.
- `FRIKApi::initialize()` finds `FRIK.dll`, calls `FRIKAPI_GetApi`, version-checks the result, and stores it in the static `FRIKApi::inst`.
- All calls go through `FRIKApi::inst->...`. The struct is plain C function pointers (`__cdecl`), so it is compiler- and language-agnostic.

## Getting started

Call `initialize()` once, after all mods have loaded (e.g. on the F4SE game-loaded message — FRIK must already be in memory). Then use `FRIKApi::inst`.

```cpp
#include "FRIKApi.h" // copied verbatim from FRIK

using frik::api::FRIKApi;

void onGameLoaded()
{
    const int err = FRIKApi::initialize();
    if (err != 0) {
        logger::error("FRIK API init failed: {}", err);
        return;
    }
    logger::info("FRIK v{}, API v{}", FRIKApi::inst->getModVersion(), FRIKApi::inst->getVersion());
}

void onFrame()
{
    if (!FRIKApi::inst || !FRIKApi::inst->isSkeletonReady()) {
        return;
    }

    // Read the left index-fingertip world position.
    const RE::NiPoint3 tip = FRIKApi::inst->getIndexFingerTipPosition(FRIKApi::Hand::Left);

    // Override the primary hand to a pointing pose, tagged so it never clobbers other systems.
    FRIKApi::inst->setHandPose("MyMod_Interaction", FRIKApi::Hand::Primary, FRIKApi::HandPoseKind::Pointing);

    // Later, release the override:
    FRIKApi::inst->clearHandPose("MyMod_Interaction", FRIKApi::Hand::Primary);
}
```

### `initialize` return codes

`int FRIKApi::initialize(uint32_t minVersion = FRIK_API_VERSION)`

| Code | Meaning |
| --- | --- |
| `0` | Success (also returned if already initialized). |
| `1` | `FRIK.dll` not found — FRIK isn't loaded, or you called too early. |
| `2` | `FRIKAPI_GetApi` not exported — incompatible FRIK build. |
| `3` | `FRIKAPI_GetApi` returned null. |
| `4` | FRIK's API version is older than `minVersion`. |

## Versioning and compatibility

`FRIK_API_VERSION` (currently **4**) identifies the struct layout. The struct is **append-only**: new functions are added at the end and the version is bumped, so a newer FRIK stays binary-compatible with an older client.

- `getVersion()` returns the API version FRIK was built with; `getModVersion()` returns the FRIK mod version string (e.g. `"0.78.0"`).
- Pass a `minVersion` to `initialize()` if you require functions added in a later API version, then guard those calls. Each function below lists the API version it was added in.

## Hands

Most functions take a `Hand`:

| `Hand` value | Resolves to |
| --- | --- |
| `Primary` | The weapon/dominant hand (right by default, left in left-handed mode). |
| `Offhand` | The other hand. |
| `Right` | Always the right hand. |
| `Left` | Always the left hand. |

Use `Primary` / `Offhand` to follow the player's handedness automatically; use `Right` / `Left` when you mean a physical hand.

## Hand poses

FRIK lets multiple systems layer hand-pose overrides without clobbering each other. Every override carries a string **tag** that uniquely identifies your system; FRIK keeps a prioritized stack of tagged overrides per hand. Always release your tag with `clearHandPose` when you're done.

### Predefined poses

`bool setHandPose(const char* tag, Hand hand, HandPoseKind handPose)` — *(since v2)*

Applies one of the predefined poses:

| `HandPoseKind` | Notes |
| --- | --- |
| `Open` | Open/relaxed hand. |
| `Pointing` | Index finger extended. |
| `HoldingWeapon` | Primary weapon grip pose. |
| `OffhandGrip` | Two-handed offhand grip pose. |
| `Attaboy` | Fallout London VR Attaboy pose. |
| `ThumbsUp` | Thumbs-up. |
| `Unset` | Passing `Unset` clears this tag's override (same as `clearHandPose`). |
| `Custom` | Not valid here — use `setHandPoseCustom*` instead; `setHandPose` returns `false`. |

### Custom poses

`bool setHandPoseCustomFingerPositions(const char* tag, Hand hand, float thumb, float index, float middle, float ring, float pinky)` — *(since v2)*

Each value is `0..1` (`0` = fully bent, `1` = fully straight) applied uniformly across that finger's joints.

`bool setHandPoseCustom(const char* tag, Hand hand, const HandPoseData& handPose, bool forceTop)` — *(since v4)*

Full control: per-joint finger values (`prox`, `mid`, `dist`), per-finger `splay`, and `palmPitch` / `palmYaw`. `HandPoseData` is a tightly packed 22-float struct (see the header for the canonical layout and the `fromFloats` / `toFloats` / `asFloatView` helpers). Leave `forceTop = false`; `true` forces your pose to the top of the stack and can thrash with FRIK's own logic.

### Clearing and querying

| Function | Since | Description |
| --- | --- | --- |
| `bool clearHandPose(tag, hand)` | v2 | Release this tag's override so FRIK regains control. |
| `HandPoseKind getCurrentHandPose(hand)` | v2 | The pose currently active on the hand (v4+ returns canonical `HandPoseKind`). |
| `HandPoseTagState getHandPoseSetTagState(tag, hand)` | v2 | `None` (not set), `Active` (set and winning), or `Overriden` (set but another tag wins). |

Use `getHandPoseSetTagState` to detect when another system has taken over the pose and react accordingly.

> **Deprecated:** `setHandPoseFingerPositions` / `clearHandPoseFingerPositions` (v1) are tagless and share a single legacy slot. Use the tagged `setHandPoseCustomFingerPositions` / `clearHandPose` instead.

## State queries

All return current FRIK state. Check `isSkeletonReady()` before relying on spatial data.

| Function | Since | Description |
| --- | --- | --- |
| `bool isSkeletonReady()` | v1 | FRIK is loaded and the skeleton is initialized. |
| `RE::NiPoint3 getIndexFingerTipPosition(hand)` | v1 | World position of the index fingertip. |
| `bool isConfigOpen()` | v2 | Any FRIK config UI is open (main, Pip-Boy, or weapon adjustment). |
| `bool isSelfieModeOn()` | v2 | Selfie mode state. |
| `void setSelfieModeOn(bool)` | v2 | Turn selfie mode on/off. |
| `bool isOffHandGrippingWeapon()` | v2 | The weapon is currently held two-handed (offhand on the weapon). |
| `bool isWristPipboyOpen()` | v2 | The wrist Pip-Boy is currently open. |

## Blocking FRIK features

When your mod replaces or conflicts with part of FRIK, you can turn that part off. Blocks are **reference-counted by tag** — a feature stays off while any tag is still blocking it, so independent mods don't fight over it. Use a unique tag and release it when done.

`bool blockFeature(const char* tag, Feature feature, bool block)` — *(since v4)*
`bool isFeatureBlocked(Feature feature)` — *(since v4)*

| `Feature` | Turns off |
| --- | --- |
| `Flashlight` | FRIK's embedded flashlight (head/hand switching and light positioning). |
| `WeaponPositioning` | Per-weapon offsets, offhand two-handed grip, and reposition mode. |
| `Pipboy` | Wrist Pip-Boy show/hide and physical finger interaction (flashlight unaffected). |
| `SmoothMovement` | Anti-motion-sickness locomotion smoothing. |

`bool blockOffHandWeaponGripping(const char* tag, bool block)` — *(since v3)*

A narrower, dedicated block for just the offhand two-handed grip (e.g. for a virtual-reload mod that needs both hands free). Also reference-counted by tag.

## Reading and overriding config

Read FRIK config values, or set session-only overrides that survive `FRIK.ini` live-reload but are never written to disk (and are cleared on game restart). All take a `caller` name used only for FRIK's logging.

`int getConfigValue(caller, section, key, char* outBuf, int bufLen, defaultValue)` — *(since v4)*

Writes the effective value (session override → on-disk value → `defaultValue`) into `outBuf` as a **raw string** that you parse yourself (e.g. `std::strtof` / `std::atoi`). Always null-terminates when `bufLen > 0`. Returns the full value length excluding the null terminator; a return `>= bufLen` means the value was truncated.

| Function | Description |
| --- | --- |
| `bool hasConfigValueOverride(caller, section, key)` | Whether a session override is currently set. |
| `bool setConfigValueOverride(caller, section, key, value)` | Set a session override (string, parsed by FRIK's type-appropriate reader; works for bool/int/float/string and compound transform/binding/pose values). FRIK reloads immediately. |
| `bool clearConfigValueOverride(caller, section, key)` | Remove a session override; the value reverts to the on-disk `FRIK.ini` value. |

## Adding a button to FRIK's config menu

A mod can add a button to FRIK's main config menu so users open its settings from there.

`bool registerOpenModSettingButtonToMainConfig(const OpenExternalModConfigData& data)` — *(since v2)*

```cpp
FRIKApi::OpenExternalModConfigData data{
    .buttonIconNifPath = "MyMod\\btn-settings.nif", // NIF used as the button icon
    .callbackReceiverName = "MyMod",                // your mod's F4SE messaging name
    .callbackMessageType = 42,                      // a message type you choose
};
FRIKApi::inst->registerOpenModSettingButtonToMainConfig(data);
```

When the user clicks the button, FRIK closes its own config UI and dispatches an F4SE message of `callbackMessageType` (with no payload) to `callbackReceiverName`. Register an F4SE listener for your mod name and open your own config when that message arrives:

```cpp
F4SE::GetMessagingInterface()->RegisterListener(onFrikMessage, "MyMod");

void onFrikMessage(F4SE::MessagingInterface::Message* msg)
{
    if (msg->type == 42) {
        openMyModConfig();
    }
}
```

## F4SE messaging

`FRIKApi::FRIK_F4SE_MOD_NAME` (`"F4VRBody"`) is FRIK's F4SE messaging name. Use it to register a listener for messages from FRIK, or as the target when dispatching messages to FRIK:

```cpp
F4SE::GetMessagingInterface()->RegisterListener(onFrikMessage, FRIKApi::FRIK_F4SE_MOD_NAME);
```

## Best practices

- **Call `initialize()` late.** FRIK must be loaded first — do it on the F4SE game-loaded message, not at plugin query/load.
- **Guard spatial reads** with `isSkeletonReady()`; the skeleton is rebuilt across loads, power-armor changes, and loading screens.
- **Use a unique, stable tag** per system for hand-pose / gripping / feature blocks, and always release it. Tags are how FRIK keeps multiple mods from clobbering each other.
- **Call from the game thread** (your F4SE message handlers or per-frame logic), the same context FRIK runs in.
- **Bump-safe:** require only the API version you need via `initialize(minVersion)` and feature-check before calling newer functions.
