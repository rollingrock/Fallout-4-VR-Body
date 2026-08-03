# FRIK migration: `F4SEVR::` Forms → `RE::` (CommonLibF4VR)

Tracks the FRIK-side application of the framework migration described in
`external/F4VR-CommonFramework/Analysis/f4sevr-to-commonlib-migration.md`.

Build verified: clean Debug build, no warnings or errors.

---

## Files changed

### 1. `src/utils.cpp` — `isArmorHasHeadLamp()` and `getMuzzleFlashNodes()`

**Equipment access:** read armor slot 0 from the player's `BipedAnim` instead of
the old `equipData->slots[0]`. CommonLib treats the slot array as
`BIPOBJECT object[]` where the form lives at `parent.object`.

```cpp
// before
if (const auto equippedItem = f4vr::getPlayer()->equipData->slots[0].item) {
    if (const auto torchEnabledArmor = dynamic_cast<F4SEVR::TESObjectARMO*>(equippedItem)) {
        ...
    }
}

// after
const auto biped = f4vr::getPlayer()->biped.get();
if (!biped) return false;
if (const auto equippedItem = biped->object[0].parent.object) {
    if (const auto torchEnabledArmor = equippedItem->As<RE::TESObjectARMO>()) {
        ...
    }
}
```

`As<RE::TESObjectARMO>()` is CommonLib's idiomatic replacement for
`dynamic_cast` and is what the rest of the framework uses.

**Muzzle flash field:** `EquippedWeaponData::unk28` was renamed to
`muzzleFlash`. Same offset (0x28), same pointer — only the field name changed.
We still `reinterpret_cast` to our own `f4cf::f4vr::MuzzleFlash` because that
type carries the `fireNode` + `projectileNode` named members the function
reads.

```cpp
// before
const auto muzzle = reinterpret_cast<f4vr::MuzzleFlash*>(equipWeaponData->unk28);
// after
const auto muzzle = reinterpret_cast<f4vr::MuzzleFlash*>(equipWeaponData->muzzleFlash);
```

### 2. `src/FRIK.cpp` — skeleton init log + readiness checks

`player->unkF0` → `player->loadedData`,
`player->unkF0->rootNode` (raw `NiNode*`) → `player->loadedData->data3D` (`NiPointer<NiAVObject>`),
`camera->cameraNode` → `camera->cameraRoot` (`NiPointer<NiNode>`).

The init log call now uses `data3D.get()` to print the raw pointer. Null
checks against `data3D` and `cameraRoot` work the same — `NiPointer` is
truthy when it holds a non-null pointer.

### 3. `src/GameHooks.cpp` — `fixPA3D`, `fixPA3DEnter`, `hookMainUpdatePlayer`

Two distinct changes:

- `player->middleProcess` → `player->currentProcess`. Same offset (0x300),
  same pointer; CommonLib types it as `AIProcess*`.
  `f4vr::AIProcess_Set3DUpdateFlags` already takes `RE::AIProcess*`, so this
  is now type-clean instead of relying on layout coincidence.
- `playerCamera->cameraNode` → `playerCamera->cameraRoot` and
  `player->unkF0->rootNode` → `player->loadedData->data3D.get()` in
  `hookMainUpdatePlayer`. The body-position-from-camera logic that copies
  X/Y of the camera world translate into the body's local + world translate
  is unchanged — the field accesses (`->local.translate.x`,
  `->world.translate.x`) all still resolve through `NiPointer::operator->`.

### 4. `src/skeleton/BoneSpheresHandler.cpp` — `registerBoneSphereOffset`

`f4vr::getPlayer()->unkF0` → `f4vr::getPlayer()->loadedData`. Pure rename of
the "is loaded data ready?" guard.

### 5. `src/pipboy/Pipboy.cpp` — `isPlayerLookingAtPipboy`

`f4vr::getPlayerCamera()->cameraNode` → `f4vr::getPlayerCamera()->cameraRoot.get()`.
`isCameraLookingAtObject` still takes a raw `RE::NiAVObject*`, so we
unwrap the smart pointer.

### 6. `src/skeleton/CullGeometryHandler.cpp` — `setEquipmentSlotByIndexVisibility`

The biggest structural change in this PR. `equipData->slots[N]` (which had
member fields `item` and `node`) became `biped->object[N]` of type
`BIPOBJECT`, where the form lives at `parent.object` and the attached
scene-graph node lives at `partClone` (`NiPointer<NiAVObject>`).

```cpp
// before
const auto& slot = f4vr::getPlayer()->equipData->slots[slotId];
if (slot.item == nullptr || slot.node == nullptr) return;
const auto formType = slot.item->GetFormType();
...
f4vr::setNodeVisibility(slot.node, !toHide);

// after
const auto biped = f4vr::getPlayer()->biped.get();
if (!biped) return;
const auto& slot = biped->object[slotId];
if (slot.parent.object == nullptr || slot.partClone == nullptr) return;
const auto formType = slot.parent.object->GetFormType();
...
f4vr::setNodeVisibility(slot.partClone.get(), !toHide);
```

Slot indices are unchanged (still BIPED_OBJECT enum values 0…43) per the
migration doc.

---

## Risk assessment & manual verification

### Higher risk (please test these in-game)

1. **`CullGeometryHandler::setEquipmentSlotByIndexVisibility`** — this is
   the only change that switches data sources entirely (from `equipData`
   array to `biped->object` array). The migration doc says they map to the
   same underlying memory and the same slot indices, but the *path* to the
   data is different. **What to test:**
   - The "hide head/face/hair geometry" feature in Pipboy and selfie modes
     (slots 0, 1, 2, 16, 17 are touched in nearby code).
   - Hide-on-pipboy-open / show-on-pipboy-close behavior: open and close
     both wrist and projected pipboy and confirm the body parts behave the
     same as before.
   - Whether equipped armor that ships the headlamp keyword still
     suppresses FRIK's embedded flashlight (the `isArmorHasHeadLamp` path).

2. **`utils.cpp::isArmorHasHeadLamp`** — same data-source change as above,
   but only reads slot 0. **What to test:** equip an armor mod that
   provides a built-in headlamp (e.g. mining helmet keyword `0xB34A6`) and
   confirm FRIK's embedded flashlight does *not* fire.

3. **`utils.cpp::getMuzzleFlashNodes`** — only the field rename
   `unk28 → muzzleFlash`, but this feeds the muzzle-flash positioning that
   FRIK uses for weapon offset. **What to test:** fire a few weapons and
   confirm muzzle flash position/orientation looks identical to the
   pre-migration build.

4. **`GameHooks.cpp::hookMainUpdatePlayer`** — touches the body-root
   transform every frame. The math is unchanged, but the pointer
   dereferenced is now `data3D.get()` (a `NiAVObject*` cast from
   `NiPointer<NiAVObject>`) instead of the old raw `NiNode*` from
   `unkF0->rootNode`. The migration doc explicitly notes
   `data3D` may need `.get()->IsNode()` to recover a `NiNode*`. We don't
   call any `NiNode`-only methods on it (only `local` and `world`), so it
   should be safe — but **the body-on-camera-XY tracking should be sanity
   checked.** Walk around and confirm the body follows correctly and
   doesn't snap, drift, or detach.

5. **`GameHooks.cpp::fixPA3D` / `fixPA3DEnter`** — runs on enter/exit
   power armor. `currentProcess` is the typed equivalent of
   `middleProcess` at the same offset, so this is the lowest-risk of the
   "higher risk" group. **What to test:** enter and exit power armor a
   few times and confirm the 3D mesh fix-up still works (no missing limbs
   or stretched geometry on transition).

### Lower risk (rename-only)

- `FRIK.cpp` `isGameReadyForSkeletonInitialization` — pure rename of
  null-check fields. If the names didn't compile, the function would fail.
  Worth a smoke test of "load a save and confirm FRIK initializes the
  skeleton on first frame", but it's guarded behavior.
- `FRIK.cpp` skeleton-init log line — informational only.
- `BoneSpheresHandler.cpp` — pure rename in a new-game guard. Test by
  starting a new game and confirming bone spheres can register after the
  player is fully loaded (e.g. via a mod that uses bone spheres, or just
  no-crash-on-new-game).
- `Pipboy.cpp` `isPlayerLookingAtPipboy` — rename + `.get()`. Test
  pipboy-stays-open vs auto-closes-when-look-away.

### Suggested test plan

1. **Boot smoke:** load any save, confirm FRIK initializes (check
   `frik.log` for "Initialize Skeleton…" and absence of crash).
2. **New game:** start a new game → confirm skeleton spawns correctly
   after Vault 111 character creation.
3. **Power armor:** enter / exit a frame a few times.
4. **Pipboy open/close:** wrist pipboy *and* projected pipboy. Confirm
   look-at threshold logic still feels the same.
5. **Body-part hiding:** equip a helmet that should hide hair → confirm
   hair hides; remove → confirm it shows. Same for face slots when in
   pipboy.
6. **Headlamp armor:** equip a headlamp-providing armor → confirm FRIK's
   own flashlight stays off.
7. **Weapon firing:** fire a 10mm and a missile launcher → confirm muzzle
   flash position is correct (this exercises `getMuzzleFlashNodes`).
8. **General locomotion:** walk, turn, and crouch → confirm the body-root
   tracks the camera's XY without drift.

---

## Out of scope

The migration doc explicitly leaves Papyrus / `Common.h` types alone:
`F4SEVR::BSFixedString`, `F4SEVR::VMArray`, `F4SEVR::VMObject`,
`F4SEVR::StaticFunctionTag`, `F4SEVR::execPapyrusGlobalFunction`. These
are still referenced in `BoneSpheresHandler.h`, `BoneSpheresHandler.cpp`,
and `utils.cpp` (Papyrus radio toggle). They're intentionally untouched —
those headers still ship in the framework.

Commented-out historical code in `GameHooks.cpp`, `GunReload.cpp`,
`GunReload.h`, `Skeleton.cpp`, and `SelfieHandler.cpp` still references
the old field names. Left as-is to preserve the historical reference
intent.
