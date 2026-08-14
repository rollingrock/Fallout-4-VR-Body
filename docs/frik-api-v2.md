# FRIK API v2

FRIK API v2 is the current **C ABI** for F4SE plugins that integrate with FRIK. It covers everything the older [v1.\* API](frik-api.md) does, plus external control of where a hand is placed, ownership of the primary weapon node, and visual weapon recoil.

The API is defined in a single header, [src/api/FRIKApiV2.h](../src/api/FRIKApiV2.h). Copy that header into your project **as-is** and call into FRIK through the exported `FRIKAPI_V2_GetApi` function. No linking against FRIK is required — the header resolves everything at runtime via `GetModuleHandle` / `GetProcAddress`.

> **Which API should I use?** Use v2 for new integrations. The v1.\* API is still exported and fully supported for existing mods, and the two can be used side by side — both sit on the same internal state, so a v1.\* client and a v2 client arbitrate through the same tags instead of fighting invisibly. There is no need to migrate a working v1.\* mod unless you want something only v2 offers.

## How it works

- FRIK exports `FRIKAPI_V2_GetApi`, which returns a pointer to a `FRIKApiV2` struct of function pointers, and `FRIKAPI_V2_GetApiStructSize`, which returns the size of that struct.
- `FRIKApiV2::initialize()` finds `FRIK.dll`, calls both exports, version- and size-checks the result, and stores it in the static `FRIKApiV2::inst`.
- All calls go through `FRIKApiV2::inst->...`. The struct is plain C function pointers (`__cdecl`), so it is compiler- and language-agnostic.

## Getting started

Call `initialize()` once, after all mods have loaded (e.g. on the F4SE game-loaded message — FRIK must already be in memory). Then use `FRIKApiV2::inst`.

```cpp
#include "FRIKApiV2.h" // copied verbatim from FRIK

using frik::api::FRIKApiV2;

void onGameLoaded()
{
    const int err = FRIKApiV2::initialize();
    if (err != 0) {
        logger::error("FRIK API v2 init failed: {}", err);
        return;
    }
    logger::info("FRIK v{}, API v2 v{}", FRIKApiV2::inst->getModVersion(), FRIKApiV2::inst->getVersion());
}

void onFrame()
{
    if (!FRIKApiV2::inst || !FRIKApiV2::inst->isSkeletonReady()) {
        return;
    }

    // Read the left index-fingertip world position.
    const RE::NiPoint3 tip = FRIKApiV2::inst->getIndexFingerTipPosition(FRIKApiV2::Hand::Left);

    // Override the primary hand to a pointing pose, tagged so it never clobbers other systems.
    FRIKApiV2::inst->setHandPose("MyMod_Interaction",
        FRIKApiV2::Hand::Primary,
        FRIKApiV2::HandPoseKind::Pointing,
        FRIKApiV2::HAND_POSE_PRIORITY_DEFAULT);

    // Later, release the override:
    FRIKApiV2::inst->clearHandPose("MyMod_Interaction", FRIKApiV2::Hand::Primary);
}
```

### `initialize` return codes

`int FRIKApiV2::initialize(uint32_t minVersion = FRIK_API_V2_VERSION)`

| Code | Meaning |
| --- | --- |
| `0` | Success (also returned if already initialized). |
| `1` | `FRIK.dll` not found — FRIK isn't loaded, or you called too early. |
| `2` | `FRIKAPI_V2_GetApi` not exported — FRIK build without API v2. |
| `3` | `FRIKAPI_V2_GetApi` returned null. |
| `4` | FRIK's API v2 version is older than `minVersion`. |
| `5` | The loaded v2 table does not exactly match your header — update your copy of `FRIKApiV2.h`. |

## Versioning and compatibility

`FRIK_API_V2_VERSION` (currently **1**) identifies the v2 contract — this page documents v2.1. It is independent of `FRIK_API_VERSION`, which counts the revisions of the [v1.\*](frik-api.md) table: a v2 client never reads that table and vice versa.

Unlike v1.\*, **v2 is not append-only**: `initialize()` requires the struct size FRIK exports to match your header exactly (code `5`). This trades tolerance for certainty — a mismatched header is refused at load rather than misreading the table at runtime. When FRIK's v2 table changes, recopy the header and rebuild.

- `getVersion()` returns the v2 contract version FRIK was built with; `getModVersion()` returns the FRIK mod version string (e.g. `"0.78.1"`).

## Skeleton lifecycle

FRIK destroys and rebuilds the player skeleton on save load, power-armor change, and loading screens. **Every registration you publish is dropped when that happens** — hand poses, hand world transforms, and recoil controllers alike. Feature blocks and config overrides are not tied to the skeleton and survive.

FRIK broadcasts these as F4SE messages under `FRIK_F4SE_MOD_NAME`:

| `LifecycleEvent` | Value | Meaning |
| --- | --- | --- |
| `kSkeletonReady` | `100` | A new skeleton is built and spatial calls are valid. Republish here. |
| `kSkeletonDestroying` | `101` | The skeleton is about to go away; your registrations are being dropped. |

```cpp
F4SE::GetMessagingInterface()->RegisterListener(onFrikMessage, FRIKApiV2::FRIK_F4SE_MOD_NAME);

void onFrikMessage(F4SE::MessagingInterface::Message* msg)
{
    switch (static_cast<FRIKApiV2::LifecycleEvent>(msg->type)) {
    case FRIKApiV2::LifecycleEvent::kSkeletonReady:
        republishMyOverrides();
        break;
    case FRIKApiV2::LifecycleEvent::kSkeletonDestroying:
        markMyOverridesDropped();
        break;
    default:
        break;
    }
}
```

Registering before `kSkeletonReady` fails with a "skeleton not ready" line in `FRIK.log`.

## Hands

Most functions take a `Hand`:

| `Hand` value | Resolves to |
| --- | --- |
| `Primary` | The weapon/dominant hand (right by default, left in left-handed mode). |
| `Offhand` | The other hand. |
| `Right` | Always the right hand. |
| `Left` | Always the left hand. |

Use `Primary` / `Offhand` to follow the player's handedness automatically; use `Right` / `Left` when you mean a physical hand.

## Priorities

Hand poses, hand world transforms, and recoil controllers are all tagged and prioritized the same way. Every override carries a string **tag** that uniquely identifies your system, and an `int priority` (must be `>= 0`).

- **Highest priority wins.** Equal priorities are broken by the most recently registered tag.
- **Re-setting a tag you already hold updates it in place** and keeps its position in that order. Refreshing every frame never walks you past an equal-priority peer — clear the tag and set it again if you want to claim the tie.

| Constant | Value | Use |
| --- | --- | --- |
| `HAND_POSE_PRIORITY_DEFAULT` | `50` | Default claim. Every unprioritized v1.\* client sits here. |
| `HAND_POSE_PRIORITY_FRIK_INTERNAL` | `90` | Where FRIK's own interaction poses sit — Pip-Boy pointing, forced pointing, offhand grip, Attaboy. |

Match `HAND_POSE_PRIORITY_FRIK_INTERNAL` to tie with FRIK (newest registration wins); exceed it to reliably outrank FRIK itself. Outranking FRIK means FRIK cannot reclaim the hand for its own interactions, so prefer `blockFeature` for a wholesale takeover.

## Hand poses

Hand poses own the **fingers**. They are independent of `setHandWorldTransform`, which owns where the wrist sits — a tag can set either, both, or neither.

### Predefined poses

`bool setHandPose(const char* tag, Hand hand, HandPoseKind handPose, int priority)`

| `HandPoseKind` | Notes |
| --- | --- |
| `Open` | Open/relaxed hand. |
| `Pointing` | Index finger extended. |
| `HoldingWeapon` | Generic primary weapon grip pose. |
| `HoldingGun` | Gun-specific grip pose. |
| `HoldingMelee` | Melee-specific grip pose. |
| `OffhandGrip` | Two-handed offhand grip pose. |
| `Fist` | Closed fist. |
| `Attaboy` | Fallout London VR Attaboy pose. |
| `ThumbsUp` | Thumbs-up. |
| `Unset` | Passing `Unset` clears this tag's override (same as `clearHandPose`). |
| `Custom` | Not valid here — use `setHandPoseCustom`; `setHandPose` returns `false`. |

`HoldingGun`, `HoldingMelee`, and `Fist` are v2-only. `getCurrentHandPose` on the v1.\* table folds all three down to `HoldingWeapon`, since a v1.\* client's header has no enumerator for them.

### Custom poses

`bool setHandPoseCustom(const char* tag, Hand hand, const HandPoseData& handPose, int priority)`

Full control: per-joint finger values (`prox`, `mid`, `dist`), per-finger `splay`, and `palmPitch` / `palmYaw`. Each finger value is `0..1` (`0` = fully bent, `1` = fully straight); for a uniform per-finger flex, set `prox` / `mid` / `dist` to the same value.

`HandPoseData` is a tightly packed 22-float struct with a canonical layout, so you can move poses across a language or process boundary as a plain float array:

```
[0..3]   thumb  { prox, mid, dist, splay }
[4..7]   index  { prox, mid, dist, splay }
[8..11]  middle { prox, mid, dist, splay }
[12..15] ring   { prox, mid, dist, splay }
[16..19] pinky  { prox, mid, dist, splay }
[20]     palmPitch
[21]     palmYaw
```

Use `fromFloats` / `toFloats` for a copy, or `asFloatView` for a zero-copy view.

### Per-bone finger transforms

For full authorship of the 15 finger bones, bypassing FRIK's authored flex/splay model:

| Function | Description |
| --- | --- |
| `bool setHandPoseCustomLocalTransforms(tag, hand, const FingerLocalTransformOverride*, priority)` | Replace the finger bone local transforms of an override this tag **already holds**. Fails if the tag holds no override, so call a `setHandPose*` function first. |
| `bool getHandPoseLocalTransformsForPose(hand, const HandPoseData&, FingerLocalTransformOverride* out)` | Resolve the transforms FRIK *would* use for a pose, without applying it. Accounts for power armor and works before a skeleton exists. |
| `bool mirrorFingerLocalTransforms(sourceHand, const FingerLocalTransformOverride* src, FingerLocalTransformOverride* out)` | Convert a complete physical-hand pose into the opposite hand's anatomical pose. `sourceHand` must be `Left` or `Right`. |

Both resolver functions return `true` only if all 15 bones resolved, and zero their outputs on failure, so a partial pose can never leak out. `mirrorFingerLocalTransforms` rejects a partial source mask outright.

`FingerLocalTransformOverride` carries `localTransforms[15]` plus an `enabledMask` — one bit per bone index, and only bones whose bit is set are read. Bone indices are 3 joints (`prox`, `mid`, `dist`) per finger, thumb first:

```
0..2   thumb  (prox, mid, dist)
3..5   index
6..8   middle
9..11  ring
12..14 pinky
```

> Any later `setHandPose*` call on the same tag clears these transforms, so republish them after every pose update.

### Clearing and querying

| Function | Description |
| --- | --- |
| `bool clearHandPose(tag, hand)` | Release this tag's override so FRIK (or the next-highest tag) regains control. |
| `HandPoseKind getCurrentHandPose(hand)` | The pose currently active on the hand. |
| `HandPoseTagState getHandPoseSetTagState(tag, hand)` | `None` (not set), `Active` (set and winning), or `Overriden` (set but another tag wins). |

Use `getHandPoseSetTagState` to detect when another system has taken over the pose and react accordingly.

## Placing a hand

`bool setHandWorldTransform(const char* tag, Hand hand, const RE::NiTransform& worldTransform, int priority)`
`bool clearHandWorldTransform(const char* tag, Hand hand)`

Take over where a hand is placed, giving FRIK the world transform to solve the arm to instead of the tracked controller. `worldTransform` is the **wrist transform in world space**, not hand-local space.

- The transform is **consumed by FRIK's arm solve on its next skeleton frame**, not applied during your call. The arm is solved exactly once per frame, so everything FRIK derives from the hand stays consistent with it.
- A published transform **keeps owning the hand until cleared**. Holding a hand steady needs no per-frame republishing; tracking a moving target means republishing whenever the target changes.
- The return value reports **validation only**. Whether the arm can actually reach the target is decided per frame by the solver, which falls back to FRIK's own posing for any frame it cannot solve.
- Call on the **game update thread**. The call is pure data publication — it does not need to run mid-scene-graph mutation.
- Registrations are cleared on skeleton destruction; see [Skeleton lifecycle](#skeleton-lifecycle).

## Weapon authority

| Function | Description |
| --- | --- |
| `bool blockPrimaryWeaponNodeOwnership(tag, block)` | Release FRIK's ownership of the primary weapon scene node so your mod can drive the weapon transform itself. |
| `bool blockPrimaryHandWeaponPose(tag, block)` | Stop FRIK's built-in primary weapon hand pose, including its per-weapon primary-hand grip rotation. |

Both are reference-counted by tag, like `blockFeature`. Taking weapon node ownership also releases an active offhand two-handed grip, so the grip and its pose don't stay latched while you own the weapon.

## Weapon hand recoil

`bool registerWeaponHandRecoilController(const char* tag, WeaponHandRecoilController controller, void* userData, int priority)`
`bool unregisterWeaponHandRecoilController(const char* tag)`

Drive the **visual** hand/arm recoil yourself. This does not alter gameplay recoil, spread, camera shake, or the engine's own visual kickback node.

```cpp
bool FRIK_CALL onRecoil(const FRIKApiV2::RecoilSample* sample, FRIKApiV2::RecoilResponse* outResponse, void* userData) noexcept
{
    // outResponse arrives pre-filled neutral: identity kick, Primary hand, Direct delivery.
    outResponse->controlledKickLocal = myKickIn(sample->nativeKickLocal);
    outResponse->handMask = static_cast<std::uint32_t>(FRIKApiV2::RecoilHandMask::Primary)
        | static_cast<std::uint32_t>(FRIKApiV2::RecoilHandMask::Offhand);
    outResponse->delivery = FRIKApiV2::RecoilDelivery::Damped;
    return true; // consume FRIK's native hand recoil for this frame
}

FRIKApiV2::inst->registerWeaponHandRecoilController("MyMod_Recoil", onRecoil, this, 50);
```

**The callback contract.** FRIK calls it synchronously on its game update thread, at most once per skeleton frame, after the native kick node and its parent frame have been validated. It must be `noexcept`, bounded, nonblocking, must not mutate scene nodes, and must not re-enter the recoil registration functions.

Return `true` to consume FRIK's native hand-recoil contribution and use `outResponse`. Return `false` to decline **only the current frame** — FRIK tries the next registered controller, and finally falls back to its own recoil.

**The response.** FRIK pre-fills `outResponse` with neutral defaults before every callback, so the normal shape of a controller is to edit only the fields it cares about.

| Field | Meaning |
| --- | --- |
| `controlledKickLocal` | The kick, in the same local frame as `RecoilSample::nativeKickLocal`. |
| `handMask` | `RecoilHandMask` bits: `Primary`, `Offhand`, or both. A zero mask intentionally suppresses hand recoil entirely. |
| `delivery` | `Damped` runs the kick through FRIK's smoothing; `Direct` applies it as given. |
| `structSize` | FRIK's own bookkeeping, reported for your information. It is ignored on the way back in — neither set nor preserve it. |

> If you build a response locally and assign it wholesale rather than editing `outResponse` in place, **fill `controlledKickLocal` yourself even when you mean to suppress recoil**: a default-constructed `RE::NiTransform` carries a zero rotation matrix rather than identity, and is refused as non-rigid.

Responses crossing the C ABI are validated as plausible rigid transforms (finite, rigid rotation, bounded translation) before use. The bounds are deliberately loose — they exist to reject garbage or uninitialized data, not to hold a controller to any particular precision. A rejected response is treated as a declined frame.

Registration fails (and logs the reason) on a reentrant call, an empty tag, a null controller, a negative priority, or a full registry. Register and unregister on the game update thread, and republish after `kSkeletonReady`.

## State queries

All return current FRIK state. Check `isSkeletonReady()` before relying on spatial data.

| Function | Description |
| --- | --- |
| `bool isSkeletonReady()` | FRIK is loaded and the skeleton is initialized. |
| `RE::NiPoint3 getIndexFingerTipPosition(hand)` | World position of the index fingertip. |
| `bool isConfigOpen()` | Any FRIK config UI is open (main, Pip-Boy, or weapon adjustment). |
| `bool isSelfieModeOn()` | Selfie mode state. |
| `void setSelfieModeOn(bool)` | Turn selfie mode on/off. |
| `bool isOffHandGrippingWeapon()` | The weapon is currently held two-handed (offhand on the weapon). |
| `bool isWristPipboyOpen()` | The wrist Pip-Boy is currently open. |

## Blocking FRIK features

When your mod replaces or conflicts with part of FRIK, you can turn that part off. Blocks are **reference-counted by tag** — a feature stays off while any tag is still blocking it, so independent mods don't fight over it. Use a unique tag and release it when done.

`bool blockFeature(const char* tag, Feature feature, bool block)`
`bool isFeatureBlocked(Feature feature)`

| `Feature` | Turns off |
| --- | --- |
| `Flashlight` | FRIK's embedded flashlight (head/hand switching and light positioning). |
| `WeaponPositioning` | Per-weapon offsets, offhand two-handed grip, and reposition mode. |
| `Pipboy` | Wrist Pip-Boy show/hide and physical finger interaction (flashlight unaffected). |
| `SmoothMovement` | Anti-motion-sickness locomotion smoothing. |

`bool blockOffHandWeaponGripping(const char* tag, bool block)`

A narrower, dedicated block for just the offhand two-handed grip (e.g. for a virtual-reload mod that needs both hands free). Also reference-counted by tag.

Feature blocks are **not** cleared when the skeleton is released.

## Reading and overriding config

Read FRIK config values, or set session-only overrides that survive `FRIK.ini` live-reload but are never written to disk (and are cleared on game restart). The two override setters take a `caller` name used only for FRIK's logging.

`int getConfigValue(section, key, char* outBuf, int bufLen, defaultValue)`

Writes the effective value (session override → on-disk value → `defaultValue`) into `outBuf` as a **raw string** that you parse yourself (e.g. `std::strtof` / `std::atoi`). Always null-terminates when `bufLen > 0`. Returns the full value length excluding the null terminator; a return `>= bufLen` means the value was truncated.

| Function | Description |
| --- | --- |
| `bool hasConfigValueOverride(section, key)` | Whether a session override is currently set. |
| `bool setConfigValueOverride(caller, section, key, value)` | Set a session override (string, parsed by FRIK's type-appropriate reader; works for bool/int/float/string and compound transform/binding/pose values). FRIK reloads immediately. |
| `bool clearConfigValueOverride(caller, section, key)` | Remove a session override; the value reverts to the on-disk `FRIK.ini` value. |

## Adding a button to FRIK's config menu

A mod can add a button to FRIK's main config menu so users open its settings from there.

`bool registerOpenModSettingButtonToMainConfig(const OpenExternalModConfigData& data)`

```cpp
FRIKApiV2::OpenExternalModConfigData data{
    .buttonIconNifPath = "MyMod\\btn-settings.nif", // NIF used as the button icon
    .callbackReceiverName = "MyMod",                // your mod's F4SE messaging name
    .callbackMessageType = 42,                      // a message type you choose
};
FRIKApiV2::inst->registerOpenModSettingButtonToMainConfig(data);
```

When the user clicks the button, FRIK closes its own config UI and dispatches an F4SE message of `callbackMessageType` (with no payload) to `callbackReceiverName`. Register an F4SE listener for your mod name and open your own config when that message arrives.

Pick a `callbackMessageType` that doesn't collide with the `LifecycleEvent` values (`100`, `101`) if you listen for both on the same handler.

## F4SE messaging

`FRIKApiV2::FRIK_F4SE_MOD_NAME` (`"F4VRBody"`) is FRIK's F4SE messaging name. Use it to register a listener for messages from FRIK, or as the target when dispatching messages to FRIK:

```cpp
F4SE::GetMessagingInterface()->RegisterListener(onFrikMessage, FRIKApiV2::FRIK_F4SE_MOD_NAME);
```

## Differences from the v1.\* API

If you're porting an existing integration:

| v1.\* | v2 |
| --- | --- |
| `setHandPose(tag, hand, kind)` + `forceTop` flag on `setHandPoseCustom` | `setHandPose(tag, hand, kind, priority)` — priority is explicit on every setter, `forceTop` is gone. Plain v1.\* calls map to `HAND_POSE_PRIORITY_DEFAULT`, `forceTop = true` maps to `90`. |
| `setHandPoseCustomFingerPositions(tag, hand, thumb, index, middle, ring, pinky)` | Use `setHandPoseCustom` with `prox` / `mid` / `dist` set to the same value per finger. |
| `setHandPoseFingerPositions` / `clearHandPoseFingerPositions` (deprecated, tagless) | Removed. Use the tagged calls. |
| `getConfigValue(caller, ...)`, `hasConfigValueOverride(caller, ...)` | No `caller` parameter — neither function logs. |
| Append-only struct, version-checked with `>=` | Exact struct-size match required at `initialize()`. |
| — | `setHandWorldTransform`, `blockPrimaryWeaponNodeOwnership`, `blockPrimaryHandWeaponPose`, recoil controllers, per-bone finger transforms, `LifecycleEvent` broadcasts. |

## Best practices

- **Call `initialize()` late.** FRIK must be loaded first — do it on the F4SE game-loaded message, not at plugin query/load.
- **Listen for the lifecycle events** and republish poses, hand transforms, and recoil controllers after `kSkeletonReady`. Guard spatial reads with `isSkeletonReady()`.
- **Use a unique, stable tag** per system, and always release it. Tags are how FRIK keeps multiple mods — across both API majors — from clobbering each other.
- **Use `HAND_POSE_PRIORITY_DEFAULT`** unless you have a specific reason to outrank someone. Check `getHandPoseSetTagState` instead of escalating priority.
- **Call from the game update thread** (your F4SE message handlers or per-frame logic), the same context FRIK runs in.
- **Check `FRIK.log`** when an integration misbehaves. FRIK logs which module acquired the API, the state changes each call makes, and the reason any call was refused.
