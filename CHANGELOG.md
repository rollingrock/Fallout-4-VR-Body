## v0.73

- Rewrite Weapon Position Adjustment UX with new friendly UI.
- Improve adjustment accuracy by using controllers thumbsticks.
- Create default adjustments for most game weapons (mostly right-handed).
- Fixed vanilla scope misalignment with weapon after adjustment.
- Fixed offhand gripping position adjustment misc issues.
- Fixed left-handed weapon adjustment and separate saved files from right-handed.
- Fixed left-handed main and pipboy config UI being inverted.

## v0.72

- Update FRIK Configuration holotape to use new config modes + readme
- Move all ini and json config file into `.\Data\FRIK_Config`
- Fixed selfie ignore hide geometries and distance
- Support FRIK.ini live reloading while game is running
- Improve FRIK.ini configuration layout and documentation
- Fixed Pipboy position json files overwritten on update
- Fixed FRIK.ini file overwritten on update by stopping distributing it with the mod
- Fixed left handed mode having right hand incorrect positioning when a throwable weapon is equipped.
- Fixed not being able to operate in-front or projected pipbot when in weapon resposition mode.
- Fixed blurry of 3D objects rendering in specific on screen (Pipboy, crafting, transfer)
- Fixed PipBot scale not saved when save button is pressed.

## v0.71

- Fixed Thumb pose issues introduced last version
- Added better logic around look-at Pipboy detection
- Fixed issue with dampen Pipboy that caused screen to move from the last position it was opened, leading to strange behavior
- Added Pipboy Config UI options for glance and dampening settings
- Fixed sheathed weapons from being drawn on Pipboy exit
- Added function to autofocus window

## v0.70

- Fixed Pipboy crash in power armor
- Added function to open Pipboy while looking at it
- Added function to dampen Pipboy screen movement frame by frame for easier reading on the wrist
- Added function to tune or toggle hand dampening while in vanilla scope

## v0.69

- New Pipboy controls and major improvements to immersion (see main page for details)
- New config menu for adjusting body and weapon position
- Improved IK function

## v0.68

- Hotfix to handle null pointer crash with missing nodes in the cull geometry function

## v0.67

- Fixed workshop crash
- Added new method to hide head geometry based on Biped position in the `TESObjectARMO` form
- Added substring matching for the mesh and skin INIs (thanks @alandtse)
- Added clamp on the leg IK code to prevent mesh from overextending ("no more stretchy boy")

## v0.66

- Fixed SmoothMovement again
- Fixed finger grip issue when equipped with weapon
- Updated Native `GetBoneIndex` function

## v0.65

- Fixed save-while-in-power-armor bug
- Fixed engine bug with skeleton switching between power armor and human
- Fixed power armor leg rotation issues and improved skeleton pose data
- Refactored hand UI placement function
- Fixed SmoothMovement to behave like version 0.58
- Adjusted update hooks for more stability

## v0.64

- Hotfix to fix crash with face and skin hiding

## v0.63

- Better left-handed mode support
- Fixed crash with Amazing Follower Tweaks
- Fixed issue with two-handed aiming
- Minor pose improvements (crouch issues still pending)
- Hotfix to fix crash with face and skin hiding

## v0.58

- Added master mode toggle for weapon repositioning (configurable via holotape or INI)

## v0.57

- Fix for workbench inventory crash when moving items

## v0.56

- Forgot to add relocation to previous crash fix

## v0.55

- Fixed crash when unequipping items with bad equip data

## v0.54

- Added function to dampen excessive hand jitter
- Possible crash fix in `setNodes`

## v0.52

- Reimplemented pipe gun crash fix without breaking shadows

## v0.51

- Reverted pipe gun patch due to shadow artifacts (new fix coming later)

## v0.50

- Fixed crash with some weapons and mods

## v0.49

- Fixes for weapon reposition mode
- Added PDB
- Prevented reticle movement when not repositioning
- Adjusted analog stick controls (Y/Z) in weapon reposition mode

## v0.48

- Fixed muzzle flashes to match weapon repositioning

## v0.47

- Added weapon and reticle repositioning system
  > Fixes many issues with weapon placements (see sticky post)
- Fixed pipe gun crashes with certain scopes
- Fixed rare weapon name crash
- Misc. polish

## v0.45

- Added INI option: hold grip to grab; let go to release two-handed weapons

## v0.44

- Added grip button release for two-handed mode
- Improved Pipboy interaction reliability

## v0.43

- Added toggle option for magnetic vs. push-to-grip method
- Fixed bug when letting go of weapon while moving
- Updated INI

## v0.42

- New two-handed weapon feature: control barrel with off hand
- Added Pipboy button activation (configurable in INI)
- Reorganized and commented INI

## v0.39

- New mesh for power armor that aligns PA parts and body
- Hands hidden when looking through scope
- Hand UI position configurable via holotape
- Head can now be hidden via holotape
- Re-enabled dynamic weapon gripping (toggle to static in holotape)
- Misc. bug fixes

## v0.38

- Fixed body pretzel and corruption issues with some armors

## v0.37

- Fixed save/load issue

## v0.36

- Added Papyrus support to control finger poses

## v0.35

- Fixed issue with saving INI to wrong filename
- Improved usability for high grip angle weapons
- Fixed scope alignment — crosshairs now accurate

## v0.34

- Fixed glove compatibility for finger poses
- Crash fix for out-of-bounds position references

## v0.33

- Added Finger Tracking
- More dynamic grip positioning
- Crash fixes

## v0.32

- Crash fix for searching child nodes
- Switch to move Pipboy to right hand
- Calibration and posture updates

## v0.30

- Improved bonesphere code robustness
- Fixed SmoothMovement bug where jumping caused flying
- INI option to disable SmoothMovement
- Fixed leg stretch/crazy movement during flight/jump/fall
- Function to detect grounded status

## v0.28

- Added left-handed mode support
- Added Papyrus function for left-handed mode detection
- Hid UI for Pipboy interaction in projected mode

## v0.10

- Initial general release
