![Weapon Adjustment Config](https://github.com/user-attachments/assets/54e395af-b4f3-4ccc-ab92-cf390793c9eb)

## Defaults
FRIK ships with 160+ embedded pre-created offset files covering vanilla weapons (including throwables) in right-handed mode.  
It should just work for most players but it's probable that changing VR scale (or something) will cause misalignment. This is what Weapon Adjustment config is for.  
Note: See "Resetting" behavior.

## Enable Adjustment Config

1. Select "Weapon Adjustment" mode in [main config mode](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/In%E2%80%90Game-Configuration-Guide-(v69)).
2. Toggle "Weapon Adjustment" mode in FRIK Settings holotape.

When in config mode both controllers thumbstick are disabled to prevent movement.  
Primary controller thumbstick (right for right-handed) controls "horizontal" axis (right/left, forward/backward), secondary for vertical (up/down).  
To control rotation, hold the secondary controller grip button. Both thumbsticks will switch to rotation mode.

### Saving
Adjustments are ONLY saved when save is requested explicitly via button or shortcut. On exiting adjustment mode or switching weapons the weapon position offsets will be reloaded to last saved configuration.

### Resetting
Resetting weapon adjustment deletes the saved offset config and loads the game default state for the game. If the weapon has embedded default, it will not reset to it while adjustment config mode is open. If no custom adjustment is saved the weapon will reset to the embedded default on exist from config mode or switching weapon. If the player wishes not to use the embedded default, just save after resetting to override the offset config.

## Files location

All saved files can be found in `\Data\FRIK_Config\Weapons_Offsets`.  
There can be 12 of file variants per weapon which can be shared with others.
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
1. `Weapon-throwable.json` - The throwable weapon offset.
1. `Weapon-throwable-PowerArmor.json` - The throwable weapon offset for wearing Power Armor.
1. `Weapon-throwable-leftHanded.json` - The throwable weapon offset for left-handed mode.
1. `Weapon-throwable-PowerArmor-leftHanded.json` - The throwable weapon offset for wearing Power Armor in left-handed mode.

For back-of-hand UI there is an additional `EmptyHand` "weapon" that is used as a global offset used if no weapon specific offset is found. Player can adjust it in weapon adjustment config mode by unequipping a weapon and switching to back-of-hand adjustment in the UI.

## Throwable Weapon
![image](https://github.com/user-attachments/assets/bd538d25-79be-46b1-b816-58b16b50542e)

Throwable weapon adjustment requires the player to prime the throwable and *KEEP HOLDING* while using the thumbsticks to adjust. This is because the throwable weapon game object only exists when primed to be thrown and removed as soon as released.

TIP: to cancel throwing the throwable weapon the player can, while still holding, open either pause menu or Pipboy and then release the throwable. It will close the menu and cancel the throwing.

## Offhand Grip

Use this mode to change the x/z-axis (right/left, up/down to the world) from where your offhand grabs the weapon. This is useful if there is a bigger stock.  
There is a known issue with the primary hand being "dislodged" from the weapon grip. It is due the weapon "center" point being away from the grip resulting in rotation moving the grip away from the hand.

## Back-of-Hand UI
The UI that shows the HP, AP, Ammo, effects, etc.  
There are hardcoded defaults for in/out of power-armor and right/left-handed modes that should just work regardless of equipped weapon.  
But in case it's not true the player can adjust the back-of-hand UI globally and for each weapon.  
Unequip a weapon to adjust back-of-hand UI global offsets. It will create `EmptyHand-backOfHand` offset json file.

## Better Scopes VR
![image](https://github.com/user-attachments/assets/285d8b51-0d38-414b-a695-da399acfe97d)

The config mode supports scope reticle repositioning if you have BetterScopesVR mod installed.  
Scopes may need to be calibrated so the reticles are true to the impact.

*Scope Zoom Toggling*
Scope zooms can be adjusted from lower magnitudes up to the scope's maximum defined zoom. The steps are defined in Better Scopes VR's ini `ZoomValues`. The defaults are `2.5,4.0,8.0`.