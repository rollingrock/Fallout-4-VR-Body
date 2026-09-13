# FRIK API v3

FRIK API v3 is the **append-only C ABI** for F4SE plugins that integrate with FRIK. It starts as the full [v2](frik-api-v2.md) surface and grows from there: FRIK only ever adds entries at the end of the table, so a plugin built against an older `FRIKApiV3.h` keeps working against a newer FRIK.

The API is defined in a single header, [src/api/FRIKApiV3.h](../src/api/FRIKApiV3.h). Copy it into your project **as-is** and call into FRIK through the exported `FRIKAPI_V3_GetApi` function. No linking against FRIK is required.

> **Which API should I use?** Use v3 for new integrations. v2 stays exported and frozen for the mods already built on it, and v1.\* is still there for older mods. All three sit on the same internal state, so clients of different majors arbitrate through the same tags instead of fighting invisibly.

## How it differs from v2

| | v2 | v3 |
| --- | --- | --- |
| Table changes | Frozen. | Append-only; every addition bumps `FRIK_API_V3_VERSION`. |
| `initialize()` size check | FRIK's table must match the header exactly (code `5`). | FRIK's table must be **at least** as large as the header (code `5` only when FRIK is older than your header). |
| Version check | `getVersion() >= minVersion` | Same. Check it against the version that introduced an entry before calling that entry. |
| Exports | `FRIKAPI_V2_GetApi`, `FRIKAPI_V2_GetApiStructSize` | `FRIKAPI_V3_GetApi`, `FRIKAPI_V3_GetApiStructSize` |
| `HandPoseTagState` | `Overriden` | `Overridden` (same value). |

Everything else, including every function, struct, enum value, priority rule, lifecycle event, and the recoil controller contract, is identical to v2. See the [v2 page](frik-api-v2.md) for the per-function documentation; only the additions below are documented here.

## Getting started

```cpp
#include "FRIKApiV3.h" // copied verbatim from FRIK

using frik::api::FRIKApiV3;

void onGameLoaded()
{
    const int err = FRIKApiV3::initialize();
    if (err != 0) {
        logger::error("FRIK API v3 init failed: {}", err);
        return;
    }
    logger::info("FRIK v{}, API v3 v{}", FRIKApiV3::inst->getModVersion(), FRIKApiV3::inst->getVersion());
}
```

### `initialize` return codes

| Code | Meaning |
| --- | --- |
| `0` | Success (also returned if already initialized). |
| `1` | `FRIK.dll` not found. |
| `2` | `FRIKAPI_V3_GetApi` not exported. FRIK build without API v3. |
| `3` | `FRIKAPI_V3_GetApi` returned null. |
| `4` | FRIK's API v3 version is older than `minVersion`. |
| `5` | FRIK's v3 table is smaller than your header. FRIK is older than the header you compiled against. |

## Skeleton lifecycle payload (v3.2)

The `kSkeletonReady` and `kSkeletonDestroying` messages now carry a `SkeletonLifecycleData` payload in `msg->data` (`msg->dataLen == sizeof`):

| Field | Meaning |
| --- | --- |
| `generation` | Skeleton builds this session, `1` for the first. A different value than the one you measured against means the body was rebuilt. |
| `rootNode` | The skeleton root `RE::NiNode*`, valid for the duration of the message. |
| `inPowerArmor` | Whether this skeleton is the power armor rig. FRIK debounces the game's transient power-armor state before rebuilding, so this only changes together with `generation`. |

The same two values are also available at any time through `getSkeletonGeneration()` and `isInPowerArmor()`. v2 clients receive the same messages and can keep ignoring the payload.

## Scope providers (v3.3)

FRIK keys every scope behaviour on one **looking-through-scope** state: whether the body root is hidden, the separate hand and recoil damping factors, Pip-Boy interaction, and the two-handed grip release rule. Without a provider that state is the vanilla `ScopeMenu`; a scope renderer registers itself and publishes the state directly.

`bool setScopeProvider(const char* tag, std::uint32_t capabilities)`
`bool clearScopeProvider(const char* tag)`

| `ScopeCapability` | FRIK's behaviour while registered |
| --- | --- |
| `KeepsBodyVisible` | The body root is never hidden while scoped (the user's `HideBodyInVanillaScope` no longer applies). |
| `OwnsScopeCamera` | FRIK leaves the `primaryWeaponScopeCamera` node alone. |
| `PublishesLookingThrough` | This provider's `setLookingThroughScope` replaces the vanilla `ScopeMenu` state. |
| `OwnsDamping` | FRIK does not dampen hands or recoil while scoped. |

Providers survive skeleton rebuilds, like feature blocks, and capabilities are the union over registered tags. Register once on the game-loaded event.

`bool setLookingThroughScope(const char* tag, bool lookingThrough)`
`bool isLookingThroughScope()`

Publish on the game update thread whenever the state changes; only a provider registered with `PublishesLookingThrough` may. `isLookingThroughScope` returns the state FRIK keyed on this frame. When it flips FRIK broadcasts `kScopeEnter` (`102`) / `kScopeExit` (`103`) with no payload.

BetterScopesVR is registered by FRIK itself as a `PublishesLookingThrough` provider when its plugin is detected, mapping its legacy message onto this state.

## Version history

| `FRIK_API_V3_VERSION` | FRIK | Added |
| --- | --- | --- |
| `1` | 0.79 | The v2 surface as of v2.1 (31 entries). |
| `2` | 0.79 | `getSkeletonGeneration`, `isInPowerArmor`; lifecycle messages carry `SkeletonLifecycleData`. |
| `3` | 0.79 | `setScopeProvider`, `clearScopeProvider`, `setLookingThroughScope`, `isLookingThroughScope`; `kScopeEnter` / `kScopeExit` events. |
