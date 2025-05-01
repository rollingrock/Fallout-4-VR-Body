![image](https://github.com/user-attachments/assets/cc7195e8-d9e5-439e-922d-13e263c01a8e)

## Defaults
FRIK ships with 120+ embedded pre-created offset files covering most vanilla weapons in right-handed mode.  
It should just work for most players but it's probable that changing VR scale (or something) will cause misalignment. This is what Weapon Adjustment config is for.

Note: Resetting weapon position in config mode will reset to the game default position and not the embedded default. If the player then exists config mode without saving the embedded default will be loaded. If you wish to use the game default just save after reseting.

## Enable Adjustment Config

1. Select "Weapon Adjustment" mode in [main config mode](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/In%E2%80%90Game-Configuration-Guide-(v69)).
2. Toggle "Weapon Adjustment" mode in FRIK Settings holotape.

When in config mode both controllers thumbstick are disabled to prevent movement.

## Files location

All saved files can be found in `\Data\FRIK_Config\Weapons_Offsets`.  
There can be 8 of file variants per weapon which can be shared with others.
1. `Weapon.json` - The weapon offset.
1. `Weapon-PowerAmor.json` - The weapon offsets for wearing Power Armor.
1. `Weapon-leftHanded.json` - The weapon offset for left-handed mode.
1. `Weapon-PowerAmor-leftHanded.json` - The weapon offsets for wearing Power Armor in left-handed mode.
1. `Weapon-offHand.json` - The offhand grip offset.
1. `Weapon-offHand-PowerArmor.json` - The offhand grip offset for wearing Power Armor.
1. `Weapon-offHand-leftHanded.json` - The offhand grip offset for left-handed mode.
1. `Weapon-offHand-PowerArmor-leftHanded.json` - The offhand grip offset for wearing Power Armor in left-handed mode.
1. `Weapon-backOfHand` - The back-of-hand UI offset.
1. `Weapon-backOfHand-leftHanded` - The back-of-hand UI offset for left-handed mode.

For back-of-hand UI there is an additional `EmptyHand` "weapon" that is used as a global offset used if no weapon specific offset is found. Player can adjust it in weapon adjustment config mode by unequipping a weapon and switching to back-of-hand adjustment in the UI.

## Offhand Mode

Use this mode to change the x/z-axis (righ/left, up/down to the world) from where your offhand grabs the weapon. This is useful if there is a bigger stock.

## Back-of-Hand UI
The UI that shows the HP, AP, Ammo, effects, etc.  
There are hardcoded defaults for in/out of power-armor and right/left-handed modes that should just work regardless of equipped weapon.  
But in case it's not true the player can adjust the back-of-hand UI globally and for each weapon.  
Unequip a weapon to adjust back-of-hand UI global offsets. It will create `EmptyHand-backOfHand` offset json file.

## Better Scopes VR

The config mode supports scope reticle repositioning if you have BetterScopesVR mod installed.  
Scopes may need to be calibrated so the reticles are true to the impact.

*Scope Zoom Toggling*
Scope zooms can be adjusted from lower magnitudes up to the scope's maximum defined zoom. The steps are defined in Better Scopes VR's ini `ZoomValues`. The defaults are `2.5,4.0,8.0`.