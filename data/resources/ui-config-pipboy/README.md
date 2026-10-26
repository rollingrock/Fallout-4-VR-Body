# FRIK Pipboy config-UI icons

Source PNGs for the icons on FRIK's in-VR **Pipboy configuration UI**. The UI is built from vrui
panels ([PipboyConfigMode.cpp](../../../src/config-mode/PipboyConfigMode.cpp)): each button's label is
text drawn at runtime, so these images hold only the icon, white on transparent, for the panel's
colour to read through.

| Icon | Used by |
|------|---------|
| `adjust-screen` | Adjust-screen mode toggle (move / rotate / scale the holo screen via stick + offhand grip/A) |
| `scale-model` | Scale-model mode toggle (resize the 3rd-person arm Pipboy) |
| `swap-model` | Swap the Pipboy model (action button) |
| `open-look-at` | Toggle "open Pipboy when looking at it" |
| `close-look-away` | Toggle "close Pipboy when looking away" |
| `dampen-screen` | All three states of the dampen-screen toggle (the state is its bottom line of text) |

Save / reset / exit use the framework's shared icons under `Textures\FRIK\f4cf\vrui\`, and the footer
prompts the controller icons under `Textures\FRIK\f4cf\bindings\`, both copied from the framework's
`mod-template`.

## Deploying

Each PNG ships as `data\mod\Textures\FRIK\vrui\pipboy\<icon>.dds`, the path the code passes to
`setImage` (`vrui\\pipboy\\<icon>.dds`, resolved under `Textures\FRIK\`). Export it in the format
the framework's panel icons use: 140x100, **uncompressed 32-bit BGRA, no mipmaps** (see the vrui
[Assets](../../../external/F4VR-CommonFramework/src/vrui/README.md#assets) notes).
