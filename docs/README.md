# FRIK

FRIK brings the full player body to life in VR! Inspired by VRIK from Skyrim VR, FRIK aims to bring the same level of immersion of having your body visible to Fallout 4 VR. Full-body inverse kinematics animates the arms and legs based on your controller and HMD position.

## Quick Links

- [Root](../README.md)
- [Installation](installation.md)
- [FAQ and Troubleshooting](faq.md)
- [In-Game Configuration Guide](in-game-configuration-guide.md)
- [Weapon Adjustment Guide](weapon-adjustment-guide.md)
- [Screenshots and Videos](screenshots-videos.md)
- [Changelog](changelog.md)
- [FRIK API (for mod developers)](frik-api.md)

## Shortcuts

- Open Main Configuration - Press and hold both thumbsticks (the `Sprint` and `Favorites` buttons) for ~1.5 seconds. The hold time is configurable via `sOpenConfigButton` in `FRIK.ini`.
- Open Pip-Boy Configuration - While the Pip-Boy is open, press and hold the primary-hand thumbstick (the `Favorites` button) for ~2 seconds.
- These can also be opened via the FRIK Settings holotape.

## Body Adjustments

Use [main configuration mode](in-game-configuration-guide.md) to fine-tune your avatar body and camera position.
Use selfie mode to see how your body looks in the world.
Make sure to adjust both in and out of power armor.

## Pip-Boy Configuration

Only applicable when using the on-wrist Pip-Boy.
Check the holo and non-holo Pip-Boy screen types, then reposition them for your comfort.
Note: The game window must be in focus for primary-hand control to work.

## Weapon Adjustment Mode

Enter weapon adjustment mode from [main configuration](in-game-configuration-guide.md).
Use this mode to adjust the primary-hand weapon grip position, offhand grip position, throwable holding position, and back-of-hand UI offset.
Vanilla weapons should all be properly positioned (unless VR scale was changed).
Use it to fix positions for new modded weapons.

## Advanced Configuration in FRIK.ini

Most configuration is stored in the FRIK.ini file, including many settings that are not exposed through the in-game configuration interfaces.
Changes made to `FRIK.ini` are live-loaded into the running game.

_Location_: `%USERPROFILE%\Documents\My Games\Fallout4VR\FRIK_Config`
