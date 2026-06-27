# Fallout 4 VR Body - FRIK

[![standard-readme compliant](https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square)](https://github.com/RichardLitt/standard-readme)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square)](LICENSE)

> A Fallout 4 VR F4SE plugin that adds full-body IK, weapon repositioning, an in-VR Pip-Boy, and hand-pose / finger control.

FRIK gives Fallout 4 VR players a visible full body with inverse kinematics and VR-focused interaction systems. It runs as an F4SE plugin DLL (`FRIK.dll`) and uses [F4VR Common Framework](external/F4VR-CommonFramework) for plugin lifecycle, VR controller input, config hot-reload, VR UI widgets, and shared game utilities.

## Table of Contents

- [Background](#background)
- [Install](#install)
- [Usage](#usage)
- [Development](#development)
- [Maintainers](#maintainers)
- [Contributing](#contributing)
- [Acknowledgements](#acknowledgements)
- [License](#license)

## Background

Fallout 4 VR renders the player as disembodied hands. FRIK adds a configurable full-body avatar, body and camera alignment tools, two-handed weapon handling, physical Pip-Boy interaction, and in-game configuration UI.

What it provides:

- **Full-body IK** - body posture, arm solving, leg walking, head/neck posture, and hand poses.
- **Weapon positioning** - per-weapon offsets, offhand grip, throwable adjustment, and back-of-hand UI placement.
- **Pip-Boy interaction** - wrist and holo Pip-Boy positioning, finger interaction, flashlight controls, and related hand poses.
- **In-game configuration** - VR UI panels for body, Pip-Boy, and weapon adjustment.
- **Mod integration** - public FRIK API, Papyrus functions, BetterScopesVR support, Fallout London VR handling, and Immersive Flashlight VR compatibility.

## Install

### Requirements

- [F4SE VR](https://f4se.silverlock.org/)
- [VR Address Library for F4SEVR Plugins](https://www.nexusmods.com/fallout4/mods/64879)

### Recommended install

Download from [Nexus Mods](https://www.nexusmods.com/fallout4/mods/53464/) and install the published mod package through your mod manager, then enable the `FRIK.esp` plugin. Load order does not matter.

Manual setup, including the required `Fallout4Custom.ini` edits, is covered in the [installation guide](docs/installation.md). Skipping those edits is a common cause of crashes on new games and save loads.

### Compatibility

- **BetterScopesVR** - supported; FRIK repositions scope reticles and adjusts dampening while looking through a scope.
- **Fallout London VR** - detected automatically; swaps the Pip-Boy for the Attaboy and loads `FRIK_FOLVR.ini` overrides.
- **Immersive Flashlight VR** - compatible; FRIK disables its embedded flashlight to avoid conflicts.
- Mods that modify the player **skeleton** (body, heel, or pose mods) can conflict and cause crashes - run only what you need.

## Usage

Once installed, FRIK shows your full body in VR automatically. Open the in-game configuration to fine-tune the body and camera (hold both thumbsticks for ~1.5 seconds), reposition weapons, and adjust the Pip-Boy. Most settings apply live, and advanced options live in `FRIK.ini`.

See the **[documentation](docs/README.md)** for shortcuts, body and Pip-Boy configuration, weapon adjustment, and advanced INI settings.

### Documentation

- [Overview and Quick Links](docs/README.md)
- [Installation](docs/installation.md)
- [FAQ and Troubleshooting](docs/faq.md)
- [In-Game Configuration Guide](docs/in-game-configuration-guide.md)
- [Weapon Adjustment Guide](docs/weapon-adjustment-guide.md)
- [Screenshots and Videos](docs/screenshots-videos.md)
- [Changelog](docs/changelog.md)
- [FRIK API for mod developers](docs/frik-api.md)

## Development

### Prerequisites

- `VCPKG_ROOT` environment variable pointing to a [vcpkg](https://github.com/microsoft/vcpkg) installation
  - `git clone https://github.com/microsoft/vcpkg.git`
  - run `bootstrap-vcpkg.bat`. Example: `C:\github\vcpkg\bootstrap-vcpkg.bat`
  - Set environment variable `VCPKG_ROOT`. Example: `setx VCPKG_ROOT "C:\github\vcpkg"`
- Visual Studio 2022 (v143) or 2026 (v145), x64 only with C++ Desktop Development
  - `winget install -e --id Microsoft.VisualStudio.Community --source winget --override "--passive --wait --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --norestart"`
- CMake 4.2+
  - `winget install -e --id Kitware.CMake --source winget`

### Build

```sh
git clone https://github.com/rollingrock/Fallout-4-VR-Body.git
cd Fallout-4-VR-Body
git submodule update --init --recursive
cmake --preset default
cmake --build build --config Release
```

This generates the Visual Studio solution in `build/`. Open `build/FRIK.slnx` if you prefer building or debugging in Visual Studio. Project configuration belongs in `CMakeLists.txt`, not the generated VS project files. Release builds automatically produce a `.7z` package in `build/package`.

For local post-build copying, copy `CMakeUserPresets.json.template` to `CMakeUserPresets.json` and set `COPY_PLUGIN_BASE_PATH` to your MO2 mod folder or Fallout 4 VR `Data` folder. More development notes are in [docs/development.md](docs/development.md).

## Maintainers

- [@RollingRock](https://github.com/rollingrock)
- [@ArthurHub](https://github.com/ArthurHub)

## Contributing

PRs accepted. For larger changes, please open an issue first to discuss what you would like to change.

Code style is enforced by clang-format and pre-commit. After cloning, run `pre-commit install` once so local checks run before commits. See [AGENTS.md](AGENTS.md) for repository architecture, build expectations, and coding conventions.

## Acknowledgements

FRIK builds on years of Fallout VR and Skyrim VR modding work.

- prog - I would never have gotten anywhere without your help and guidance. Thanks so much for making VRIK and supporting me.
- LH.Adonis - For many fixes and improvements in versions 0.71 through 0.77+.
- CylonSurfer - For the Idle Hands meshes, configuration UI, Holo Pip-Boy, and many other great improvements.
- shizof - For the Smooth Movement VR code integrated into the mod.
- Gingas - For the valuable feedback and all the other awesome work you have done.
- Atom - Thanks for the code advice and helping me out of jams.
- lfrazer - Fo4VR Tools saved a lot of time.
- expired and F4SE/SKSE Team.
- ryan - For CommonLibF4, which helped me reference objects I was struggling with.
- alandtse - For more CommonLibF4 work.
- Thanks to everyone who helped me test.

## License

[MIT](LICENSE) (c) 2021 Jason Pharis
