# FRIK Pipboy config-UI sprites

Source PNGs for FRIK's in-VR **Pipboy configuration UI** — one PNG per sprite. The file name
becomes the `.nif` name referenced in [PipboyConfigMode.cpp](../../../src/config-mode/PipboyConfigMode.cpp).

| Sprite | Role |
|--------|------|
| `title` | Panel header |
| `btn-adjust-screen` | Adjust-screen mode toggle (move / rotate / scale the holo screen via stick + offhand grip/A) |
| `btn-scale-model` | Scale-model mode toggle (resize the 3rd-person arm Pipboy) |
| `btn-swap-model` | Swap the Pipboy model (action button) |
| `btn-open-look-at` | Toggle "open Pipboy when looking at it" |
| `btn-close-look-away` | Toggle "close Pipboy when looking away" |
| `btn-dampen-off` / `btn-dampen-smooth` / `btn-dampen-hold` | The three states of the dampen-screen multi-state toggle (None / Movement / HoldInPlace) |
| `msg-footer-main` | Footer shown when no adjust mode is selected |
| `msg-footer-adjust` | Footer shown while adjusting the screen |
| `msg-footer-adjust-simple` | Footer shown while scaling the model |
| `msg-not-on-wrist` | Message for when the Pipboy isn't on the wrist |

## Pack command

Bin-packs every PNG in this folder into a single `ui-config-pipboy.DDS` atlas plus one
ready-to-use `<sprite>.nif` per image, written into FRIK's deployable mod tree
(`Textures\FRIK\ui-config-pipboy.DDS` + `Meshes\FRIK\ui-config-pipboy\<sprite>.nif`):

```
python external\F4VR-CommonFramework\nif-tools\vrui_atlas.py pack --name ui-config-pipboy --texture-subpath FRIK data/Resources/ui-config-pipboy --output data\mod
```

`--texture-subpath FRIK` is both the in-game texture path baked into every nif
(`Textures\FRIK\ui-config-pipboy.DDS`) and the subfolder the files are written under.

Each PNG's file name becomes its `.nif` name — the same name passed to `UIButton` /
`UIToggleButton` / `UIMultiStateToggleButton` / `UIWidget` in code (e.g. `btn-swap-model.png` →
`FRIK\ui-config-pipboy\btn-swap-model.nif`). So renaming or removing a PNG here means updating the
matching string in [PipboyConfigMode.cpp](../../../src/config-mode/PipboyConfigMode.cpp) and
re-running the pack command (and deleting the now-stale `.nif` from the mod tree). Full options and
the reverse (`unpack`) are in
[nif-tools/README.md](../../../external/F4VR-CommonFramework/nif-tools/README.md).
