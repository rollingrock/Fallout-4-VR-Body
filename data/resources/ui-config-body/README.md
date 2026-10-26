# FRIK body-adjustment config-UI icons

Source PNGs for the icons on FRIK's in-VR **body adjustment** sub-config. The UI is built from vrui
panels ([BodyAdjustmentSubConfigMode.cpp](../../../src/config-mode/BodyAdjustmentSubConfigMode.cpp)):
each button's label is text drawn at runtime, so these images hold only the icon, white on
transparent, for the panel's colour to read through.

| Icon | Used by |
|------|---------|
| `play-seated` | Toggle seated play |
| `hide-head` | Toggle hiding the player's head gear |
| `body-height` | HMD / body up-down adjustment toggle |
| `body-forward` | Body in / out adjustment toggle |
| `arms-length` | Arms length adjustment toggle |
| `vr-scale` | Game VR scale adjustment toggle |

Save / reset / back use the framework's shared icons under `Textures\FRIK\f4cf\vrui\`, and the footer
prompts the controller icons under `Textures\FRIK\f4cf\bindings\`.

## Deploying

Each PNG ships as `data\mod\Textures\FRIK\vrui\body\<icon>.dds`, the path the code passes to
`setImage` (`vrui\\body\\<icon>.dds`, resolved under `Textures\FRIK\`). Export it in the format the
framework's panel icons use: 140x100, **uncompressed 32-bit BGRA, no mipmaps** (see the vrui
[Assets](../../../external/F4VR-CommonFramework/src/vrui/README.md#assets) notes).
