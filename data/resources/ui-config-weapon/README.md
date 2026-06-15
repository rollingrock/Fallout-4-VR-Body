# FRIK weapon config-UI sprites

Source PNGs for FRIK's in-VR **weapon-reposition configuration UI**. One PNG per sprite —
the reposition-target buttons (weapon, primary hand, offhand, throwable, back-of-hand UI,
BetterScopesVR), the message lines (empty hands, throwable empty hands), the footers, and
the menu title. Re-skin any of them by editing the PNG and re-running the pack command
below.

## Pack command

Bin-packs every PNG in this folder into a single `ui-config-weapon.DDS` atlas plus one
ready-to-use `<sprite>.nif` per image, written into FRIK's deployable mod tree
(`Textures\FRIK\ui-config-weapon.DDS` + `Meshes\FRIK\ui-config-weapon\<sprite>.nif`):

```
python external\F4VR-CommonFramework\nif-tools\vrui_atlas.py pack --name ui-config-weapon --texture-subpath FRIK data/Resources/ui-config-weapon --output data\mod
```

`--texture-subpath FRIK` is both the in-game texture path baked into every nif
(`Textures\FRIK\ui-config-weapon.DDS`) and the subfolder the files are written under.

Each PNG's file name becomes its `.nif` name — the same name passed to `UIButton` /
`UIWidget` in code (e.g. `btn-weapon.png` → `FRIK\ui-config-weapon\btn-weapon.nif`,
referenced from
[WeaponPositionConfigMode.cpp](../../../src/weapon-position/WeaponPositionConfigMode.cpp)).
So renaming a PNG here means updating the matching string in the source. Full options and
the reverse (`unpack`) are in
[nif-tools/README.md](../../../external/F4VR-CommonFramework/nif-tools/README.md).
