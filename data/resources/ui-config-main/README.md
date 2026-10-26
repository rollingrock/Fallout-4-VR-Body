# FRIK main config-UI icons

Source PNGs for the icons on FRIK's in-VR **main configuration menu**. The UI is built from vrui
panels ([MainConfigMode.cpp](../../../src/config-mode/MainConfigMode.cpp)): each button's label is
text drawn at runtime, so these images hold only the icon, white on transparent, for the panel's
colour to read through.

| Icon | Used by |
|------|---------|
| `body-config` | Open the body adjustment sub-config |
| `dampen-hands` | Toggle dampening the hands' movement |
| `grip-mode` | All four states of the two-handed grip mode toggle (the mode is its bottom line of text) |
| `pipboy-config` | Open the Pipboy config |
| `weapon-config` | Open the weapon position config |

Open config file / help wiki / exit use the framework's shared icons under `Textures\FRIK\f4cf\vrui\`,
and the footer and selfie prompts the controller icons under `Textures\FRIK\f4cf\bindings\`. A button
another mod adds through FRIK's API is the NIF that mod registers.

## Deploying

Each PNG ships as `data\mod\Textures\FRIK\vrui\main\<icon>.dds`, the path the code passes to
`setImage` (`vrui\\main\\<icon>.dds`, resolved under `Textures\FRIK\`). Export it in the format the
framework's panel icons use: 140x100, **uncompressed 32-bit BGRA, no mipmaps** (see the vrui
[Assets](../../../external/F4VR-CommonFramework/src/vrui/README.md#assets) notes).
