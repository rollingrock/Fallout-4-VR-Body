# FRIK main config-UI sprites

Source PNGs for FRIK's in-VR **main configuration menu** and its **body-adjustment
sub-config**. One PNG per sprite — the buttons (open body/pipboy/weapon config, dampen
hands, two-handed grip modes, play seated, hide head, and the body/forward/arms-length/
VR-scale toggles), the message lines, and the menu titles. Re-skin any of them by editing
the PNG and re-running the pack command below.

## Pack command

Bin-packs every PNG in this folder into a single `ui-config-main.DDS` atlas plus one
ready-to-use `<sprite>.nif` per image, written into FRIK's deployable mod tree
(`Textures\FRIK\ui-config-main.DDS` + `Meshes\FRIK\ui-config-main\<sprite>.nif`):

```
python external\F4VR-CommonFramework\nif-tools\vrui_atlas.py pack --name ui-config-main --texture-subpath FRIK data/Resources/ui-config-main --output data\mod
```

`--texture-subpath FRIK` is both the in-game texture path baked into every nif
(`Textures\FRIK\ui-config-main.DDS`) and the subfolder the files are written under.

Each PNG's file name becomes its `.nif` name — the same name passed to `UIButton` /
`UIWidget` in code (e.g. `btn-body-config.png` → `FRIK\ui-config-main\btn-body-config.nif`,
referenced from [MainConfigMode.cpp](../../../src/config-mode/MainConfigMode.cpp) and
[BodyAdjustmentSubConfigMode.cpp](../../../src/config-mode/BodyAdjustmentSubConfigMode.cpp)).
So renaming a PNG here means updating the matching string in the source. Full options and
the reverse (`unpack`) are in
[nif-tools/README.md](../../../external/F4VR-CommonFramework/nif-tools/README.md).
