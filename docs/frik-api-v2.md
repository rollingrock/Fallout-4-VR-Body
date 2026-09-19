# FRIK API v2

FRIK API v2 is the current **C ABI** for F4SE plugins that integrate with FRIK. It covers everything the older [v1.\* API](frik-api.md) does, plus external control of where a hand is placed, ownership of the primary weapon node, and visual weapon recoil.

The API is defined in a single header, [src/api/FRIKApiV2.h](../src/api/FRIKApiV2.h). Copy that header into your project **as-is** and call into FRIK through the exported `FRIKAPI_V2_GetApi` function. No linking against FRIK is required — the header resolves everything at runtime via `GetModuleHandle` / `GetProcAddress`. The header uses CommonLibF4's `RE::NiPoint3`, `RE::NiTransform` and `RE::NiNode` types, so include it where CommonLibF4 is already in scope.

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
| `5` | FRIK's v2 table is smaller than `minVersion` requires — FRIK is older than the version you asked for. |

## Versioning and compatibility

`FRIK_API_V2_VERSION` (currently **4**) identifies the v2 contract — this page documents v2.4. It is independent of `FRIK_API_VERSION`, which counts the revisions of the [v1.\*](frik-api.md) table: a v2 client never reads that table and vice versa.

Since v2.2 the table is **append-only**: FRIK only ever adds entries at the end and bumps `FRIK_API_V2_VERSION`, so a header you copied today keeps working against every newer FRIK. `initialize(minVersion)` checks `getVersion() >= minVersion` and that FRIK's table is at least as large as `minVersion` implies (code `5` otherwise). To also run against an older FRIK, pass the oldest version you can live with and gate every newer entry on `getVersion()`; each entry below is documented with the version that introduced it.

| `FRIK_API_V2_VERSION` | FRIK | Added |
| --- | --- | --- |
| `1` | 0.78 | The original 31-entry table (exact-size check at `initialize()`). |
| `2` | 0.79 | Append-only rule; `getSkeletonGeneration`, `isInPowerArmor`; lifecycle messages carry `SkeletonLifecycleData`; scope providers: `setScopeProvider`, `clearScopeProvider`, `setLookingThroughScope`, `isLookingThroughScope`; `kScopeEnter` / `kScopeExit` events. |
| `3` | 0.79 | Frame phases: `registerFrameCallback`, `unregisterFrameCallback`; a hand transform published in `BeforeArmSolve` is solved in the same frame. Body reads: `getTrackedHandTransform`, `getBoneWorldTransform`, `getArmChain`; `getHandSolveResult`; `setOffHandGripping`; `setWeaponNodeParentHand` / `clearWeaponNodeParentHand` (and `blockPrimaryWeaponNodeOwnership` no longer flips the parent hand). |

> A client built against the v2.1 header refuses any FRIK from 0.79 on (its exact-size check fails with code `5`). Recopy the header once; after that no further recopy is ever forced.

> **v2.3 compatibility note.** Two behaviours changed underneath existing entries, so a mod that relied on them must update in the same drop: (1) `blockPrimaryWeaponNodeOwnership` no longer parents the weapon node under the left hand as a side effect, nor flips the first-person arm source, the off-side hand pose copy and the recoil hand with it; a left-carry now also calls `setWeaponNodeParentHand`. A client that only blocks and reparents itself gets a right-hand pose and recoil on the wrong hand, with no error. (2) FRIK's patch at `0xF2F0A0` is a call detour now, so a mod that chained on the NOP bytes there must register for `NativeGraphOutput` instead. Neither is negotiable per client at runtime.

- `getVersion()` returns the v2 contract version FRIK was built with; `getModVersion()` returns the FRIK mod version string (e.g. `"0.78.1"`).

## Skeleton lifecycle

FRIK destroys and rebuilds the player skeleton on save load, power-armor change, and loading screens. **Every registration you publish is dropped when that happens** — hand poses, hand world transforms, and recoil controllers alike. Feature blocks and config overrides are not tied to the skeleton and survive.

FRIK broadcasts these as F4SE messages under `FRIK_F4SE_MOD_NAME`:

| `LifecycleEvent` | Value | Meaning |
| --- | --- | --- |
| `kSkeletonReady` | `100` | A new skeleton is built and spatial calls are valid. Republish here. |
| `kSkeletonDestroying` | `101` | The skeleton is about to go away; your registrations are being dropped. |
| `kScopeEnter` | `102` | The looking-through-scope state turned on (v2.2, no payload; see [Scope providers](#scope-providers-v22)). |
| `kScopeExit` | `103` | The looking-through-scope state turned off (v2.2, no payload). |

Since v2.2 the two skeleton messages carry a `SkeletonLifecycleData` payload in `msg->data` (`msg->dataLen == sizeof`):

| Field | Meaning |
| --- | --- |
| `generation` | Skeleton builds this session, `1` for the first. A different value than the one you measured against means the body was rebuilt. |
| `rootNode` | The skeleton root `RE::NiNode*`, valid for the duration of the message. |
| `inPowerArmor` | Whether this skeleton is the power armor rig. FRIK debounces the game's transient power-armor state before rebuilding, so this only changes together with `generation`. |

The same two values are available at any time through `getSkeletonGeneration()` and `isInPowerArmor()` (v2.2).

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

> Since v2.3 these transforms survive later `setHandPose*` updates of the same tag; `clearHandPose` or a new mask replaces them. (Before v2.3 every pose update cleared them.)

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

- The transform is **consumed by FRIK's arm solve**, not applied during your call: published from a `BeforeArmSolve` [frame callback](#frame-phases-v23) it is solved in that same frame, published from `AfterArmSolve` it is re-solved right there (second solve for that hand), published anywhere else it is solved on FRIK's next frame. The arm is solved exactly once per frame, so everything FRIK derives from the hand stays consistent with it.
- A published transform **keeps owning the hand until cleared**. Holding a hand steady needs no per-frame republishing; tracking a moving target means republishing whenever the target changes.
- The return value reports **validation only**. Whether the arm can actually reach the target is decided per frame by the solver, which falls back to FRIK's own posing for any frame it cannot solve. `getHandSolveResult(hand, &wrist)` (v2.3) reports that outcome for the frame: `Consumed`, `Unreachable` (fell back to the tracked hand), `NoClaim` or `SkeletonNotReady`, and fills the wrist world transform as rendered. It is latched once the frame's world transforms are final, so read it from `AfterWorldFinal` or the next frame.
- Call on the **game update thread**. The call is pure data publication — it does not need to run mid-scene-graph mutation.
- Registrations are cleared on skeleton destruction; see [Skeleton lifecycle](#skeleton-lifecycle).

## Weapon authority

| Function | Description |
| --- | --- |
| `bool blockPrimaryWeaponNodeOwnership(tag, block)` | Release FRIK's ownership of the primary weapon scene node so your mod can drive the weapon transform itself. |
| `bool blockPrimaryHandWeaponPose(tag, block)` | Stop FRIK's built-in primary weapon hand pose, including its per-weapon primary-hand grip rotation. |

Both are reference-counted by tag, like `blockFeature`. Taking weapon node ownership also releases an active offhand two-handed grip, so the grip and its pose don't stay latched while you own the weapon. While the node is blocked FRIK leaves it as you set it: no offsets, no re-glue to the hand each frame, and the first-person hands still follow the controllers. The scope camera and the muzzle flash keep following wherever you put it; a mod that drives the scope camera itself registers as a scope provider with `OwnsScopeCamera` and FRIK leaves the camera alone in every path.

`bool setWeaponNodeParentHand(const char* tag, Hand hand)` / `bool clearWeaponNodeParentHand(const char* tag)` (v2.3)

Ask FRIK to parent the primary weapon node under a hand, for a left-carry. FRIK does the reparent and its own bookkeeping (which weapon node drives each first-person arm, the off-side weapon hand pose copy, the recoil hand) and restores the game's left-handed setting when the tag clears or the skeleton rebuilds. The newest request wins. Before v2.3 `blockPrimaryWeaponNodeOwnership` flipped this topology as a side effect; it no longer does, so a left-carry needs both calls.

While the weapon is parented under the hand the game does not consider primary, FRIK also parents the engine's scope rig, `ScopeParent` (the vanilla scope widget's parent) and the scope camera, under the weapon node so the scope view and any lens hung on `ScopeParent` follow the carried weapon; they go back on the wand chain when the carry ends, keeping their world transform on each switch. A mod that reads either node should re-read its parent rather than cache it. A scope provider holding `OwnsScopeCamera` keeps the rig where it is; one holding `PlacesScopeWidget` (v2.4) gets `ScopeParent` under the carrying hand's wand node instead and only the camera follows the weapon node, since the scope widget is not drawn under the first-person skeleton.

`bool setOffHandGripping(const char* tag, bool active, Hand supportHand, const RE::NiTransform* supportWorld)` (v2.3)

Report your own two-handed grip so `isOffHandGrippingWeapon()` and every FRIK consumer of it (the Pip-Boy guards among them) treat the weapon as gripped. `supportHand` is the physical hand on the weapon, `supportWorld` its world transform or null. The grip is tied to the current weapon: FRIK drops it when the drawn weapon changes and on skeleton release, so re-report after `kSkeletonReady`. Pass `active = false` to release.

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

## Scope providers (v2.2)

FRIK keys every scope behaviour on one **looking-through-scope** state: whether the body root is hidden, the separate hand and recoil damping factors, Pip-Boy interaction, and the two-handed grip release rule. Without a provider that state is the vanilla `ScopeMenu`; a scope renderer registers itself and publishes the state directly.

`bool setScopeProvider(const char* tag, std::uint32_t capabilities)`
`bool clearScopeProvider(const char* tag)`

| `ScopeCapability` | FRIK's behaviour while registered |
| --- | --- |
| `KeepsBodyVisible` | The body is never hidden while scoped (the user's `HideBodyInVanillaScope` no longer applies). Without it FRIK culls the body geometry while scoped; since v2.3 it no longer collapses the root, so bone and hand transforms stay valid either way. |
| `OwnsScopeCamera` | FRIK leaves the `primaryWeaponScopeCamera` node alone. |
| `PublishesLookingThrough` | This provider's `setLookingThroughScope` replaces the vanilla `ScopeMenu` state. |
| `OwnsDamping` | FRIK does not dampen hands or recoil while scoped. |
| `PlacesScopeWidget` | (v2.4) The provider places its own widget on the scope. During a carry in the other hand FRIK hangs `ScopeParent` under that hand's wand node (`SecondaryWandNode`, world-preserving; back under `PrimaryUIAttachNode` with its rest local when the carry ends) and carries only the scope camera with the weapon node, because the engine does not draw the scope widget while `ScopeParent` hangs under the first-person skeleton. Re-read the parent rather than cache it. |

Providers survive skeleton rebuilds, like feature blocks, and capabilities are the union over registered tags. Register once on the game-loaded event.

`bool setLookingThroughScope(const char* tag, bool lookingThrough)`
`bool isLookingThroughScope()`

Publish on the game update thread whenever the state changes; only a provider registered with `PublishesLookingThrough` may. `isLookingThroughScope` returns the state FRIK keyed on this frame. When it flips FRIK broadcasts `kScopeEnter` / `kScopeExit`.

BetterScopesVR is registered by FRIK itself as a `PublishesLookingThrough` provider when its plugin is detected, mapping its legacy message onto this state.

`kScopeEnter` / `kScopeExit` are broadcast at the start of FRIK's frame, before any frame phase runs, so a callback in that frame already sees the new state.

## Frame phases (v2.3)

FRIK's frame is a fixed sequence, and a mod can run at named points of it instead of hooking around FRIK. Register once after FRIK has loaded; registrations survive skeleton rebuilds and every phase except `FrameBegin` and `FrameEnd` only runs while a skeleton exists.

`bool registerFrameCallback(const char* tag, std::uint32_t phase, FrameCallback callback, void* userData, int priority)`
`bool unregisterFrameCallback(const char* tag)`

`FrameCallback` is `void(FRIK_CALL*)(std::uint32_t phase, void* userData) noexcept`. One tag may register several phases; `unregisterFrameCallback` drops them all. Within a phase, callbacks run by descending priority, then registration order, so at equal priority the newest registration runs last and its writes win. Re-registering a tag and phase replaces the callback in place. Registering or unregistering from inside a callback is refused. The registry holds 32 registrations.

| `FramePhase` | When |
| --- | --- |
| `NativeGraphOutput` | The engine's animation graph output for the player, before FRIK touches the body. Runs from FRIK's detour of the player post-animation vfunc (`0xF2F0A0`), earlier in the game frame than the other phases; do not hook that site yourself. |
| `BodyPlaced` | The body root is under the HMD and posture is set. |
| `LegsSolved` | Legs and walking are solved. |
| `BeforeArmSolve` | Before the arm solve. A `setHandWorldTransform` published here is solved in this same frame. |
| `AfterArmSolve` | Both arms are solved to their targets, so a callback can read the solved arm (`getArmChain`). A hand transform published or cleared inside this phase is re-solved before the frame continues, at the cost of a second solve for that hand; publish in `BeforeArmSolve` when you do not need the solved arm first. |
| `AfterHandPose` | Finger poses are applied. |
| `AfterWeaponPosition` | Weapon offsets, two-handed grip and the scope camera are applied; the primary hand is final. |
| `BeforeWorldFinal` | Before FRIK pushes the frame into the flattened bone array. |
| `AfterWorldFinal` | The frame is complete; every bone world transform is final. On the first frame of a skeleton this runs after `kSkeletonReady`. |
| `FrameBegin` | The start of FRIK's frame, after the scope events and before the skeleton check. The only phase that also runs while no skeleton exists (loading screens, rebuilds), so a mod can keep per-frame housekeeping and its own provider dispatch alive without hooking the game loop. Numbered 9 but runs first in FRIK's pass (after `NativeGraphOutput`, which the engine fires earlier in the game frame). |
| `FrameEnd` | The end of FRIK's frame. Runs every frame like `FrameBegin`: after `AfterWorldFinal` when the skeleton phases ran, and right after FRIK's early return when they did not (no player, loading screen, skeleton released or rebuilt this frame). A `FrameBegin` / `FrameEnd` pair therefore always brackets a frame, so per-frame work that must not be skipped can run in `FrameEnd` when the skeleton phases did not fire. |

All callbacks run on the game update thread inside FRIK's frame. Any API call is allowed from a callback except `registerFrameCallback` / `unregisterFrameCallback`.

Register once per mod. A mod that fans a phase out to its own plugins (a provider API of its own) should be the only registrant, or the plugin's work runs twice. Use priority to order work within a phase: higher runs first, so a callback that only reads the pose registers above one that writes it.

## Reading tracked hands and bones (v2.3)

The inputs and outputs of FRIK's own solve, so a mod computes its claims from the same values FRIK uses instead of re-reading engine nodes.

`bool getTrackedHandTransform(Hand hand, TrackedHandKind kind, RE::NiTransform* outTransform)`

| `TrackedHandKind` | Value |
| --- | --- |
| `Wand` | The VR controller node for that hand. |
| `WeaponOffset` | The weapon offset node FRIK dampens (the `DampenHands*` settings) and drives the first-person arm from. |
| `FirstPersonHand` | The first-person hand FRIK solves the body arm to when no hand transform is published. |

Current from `BeforeArmSolve` on; read earlier in the frame they still hold the previous frame, and during a left-carry `FirstPersonHand` holds the game's own re-glue instead. Returns false without a skeleton or when the node does not exist.

`bool getBoneWorldTransform(const char* boneName, RE::NiTransform* outTransform)`

World transform of any body bone by its skeleton name (`LArm_Hand`, `Spine2`, ...), read from the flattened bone tree. Final after `AfterWorldFinal`; earlier in the frame it holds the previous frame. Returns false for an unknown name or without a skeleton.

`bool getArmChain(Hand hand, ArmChainTransforms* outChain)`

World transforms of the live arm nodes, shoulder to hand, in `ArmChainTransforms`. Bit `i` of `validMask` is set when bone `i` exists; `forearm2` / `forearm3` do not in power armor and read as identity. Valid after `AfterArmSolve`, final after `AfterWorldFinal`.

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
| Append-only struct, version-checked with `>=` | Same rule since v2.2 (v2.1 required an exact struct-size match). |
| — | `setHandWorldTransform`, `blockPrimaryWeaponNodeOwnership`, `blockPrimaryHandWeaponPose`, recoil controllers, per-bone finger transforms, `LifecycleEvent` broadcasts. |

## Best practices

- **Call `initialize()` late.** FRIK must be loaded first — do it on the F4SE game-loaded message, not at plugin query/load.
- **Listen for the lifecycle events** and republish poses, hand transforms, and recoil controllers after `kSkeletonReady`. Guard spatial reads with `isSkeletonReady()`.
- **Use a unique, stable tag** per system, and always release it. Tags are how FRIK keeps multiple mods — across both API majors — from clobbering each other.
- **Use `HAND_POSE_PRIORITY_DEFAULT`** unless you have a specific reason to outrank someone. Check `getHandPoseSetTagState` instead of escalating priority.
- **Call from the game update thread** (your F4SE message handlers or per-frame logic), the same context FRIK runs in.
- **Check `FRIK.log`** when an integration misbehaves. FRIK logs which module acquired the API, the state changes each call makes, and the reason any call was refused.
