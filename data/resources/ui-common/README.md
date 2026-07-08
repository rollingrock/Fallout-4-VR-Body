# Shared common-UI sprites (`ui-common` atlas)

The framework's shared button sprites reused by FRIK's VR config UI. Unlike the mod-specific
atlases (e.g. [`ui-config-main`](../ui-config-main/)), the source PNGs do not live here — they are
the framework's shared set in `external\F4VR-CommonFramework\mod-template\data\resources\common`.

## Pack command

Regenerates `Textures\FRIK\ui-common.DDS` and `Meshes\FRIK\ui-common\<button>.nif` in the
deployable tree:

```
python external\F4VR-CommonFramework\nif-tools\vrui_atlas.py pack --name ui-common --texture-subpath FRIK external\F4VR-CommonFramework\mod-template\data\resources\common --output data\mod
```

Full options and the reverse (`unpack`) are in
[nif-tools/README.md](../../../external/F4VR-CommonFramework/nif-tools/README.md).
