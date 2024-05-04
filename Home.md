Welcome to the Fallout-4-VR-Body wiki!

# Repositioning

With FRIK 0.0.47 and above, both individual weapons and the scope reticles (if [Better Scopes VR](https://www.nexusmods.com/fallout4/mods/61214) is installed) can be repositioned. Repositioning requires the use of two inputs. Please help fill this in for different HMDs. Button numbers are defined [here](https://github.com/ValveSoftware/openvr/blob/v1.23.7/headers/openvr.h#L981-L1011).

| ini Setting  | Default (int / enum)| Quest 2/3/Pro | Index | 
|---|---|---|---|
| `RepositionButtonID` | 33 / vr::EVRButtonId::k_EButton_SteamVR_Trigger | Offhand trigger |  |  |  |
| `OffHandActivateButtonID` | 7 / vr::EVRButtonId::k_EButton_A |Offhand X |  |  |  |

## Enable Repositioning

Use the FRIK holotape or edit the  `\Data\F4SE\Plugins\FRIK.ini` (before loading Fallout) to enable repositioning. The default setting is disabled. After you've adjusted, you may want to turn it off to avoid accidental repositioning.

```ini
# Weapon and scope reposition settings
EnableRepositionMode = true
```

## File location

All saved files can be found in `\Data\F4SE\Plugins\frik_weapon_offsets`. There can be four types of file which can be shared with others.
1. `Weapon.json` - The weapon offset.
2. `Weapon-PowerAmor.json` - The weapon offsets for wearing Power Armor.
3. `Weapon-offHand.json` - The offhand offset.
4. `Weapon-offHand-PowerArmor.json` - The offhand offset for wearing Power Armor.

Files are reread when a new weapon is detected. Edits can be made externally while the game is running and will be reflected by swapping weapons.

## Recommended Settings

These settings are recommended during repositioning and can be restored after repositioning.

* Settings -> VR -> Rotation Type -> None - The analog joystick on the main hand is used for repositioning. 
* FRIK.ini - Recommended so grip does not need to be held down during repositioning.
    - EnableGripButton = true
    - EnableGripButtonToLetGo = true

## Weapon Rotation

Weapon Rotation currently does not require any mode to access and is available as soon as gripping with the offhand.

1. Select weapon
2. Grip with off hand
3. Adjust angle of rifle using offhand.
4. Click `RepositionButtonID` to save.

## Weapon and Offhand Repositioning

Weapon and Offhand Repositioning is done by entering 3 modes while gripping. They cycle when cliking`OffHandActivateButtonID` through:
1. [Weapon Mode](#weapon-mode)
2. [Offhand Mode](#offhand-mode)
3. [Reset To Default Mode](#reset-to-default-mode)

### Weapon Mode

Use this mode to change the offsets for a weapon relative to your main hand.

1. Select weapon
2. Grip with off hand
3. Hold `RepositionButtonID` until haptic pulse (default 1 second `HoldDelay`) to enter reposition mode. This will start in `Weapon Mode`. 
    - The Offhand will show a wand in this mode.
    - All changes will be previewed.
    - X translation (e.g., move weapon forward and back down the barrel) - Move off hand away from starting position in 3.
    - Z/Y (up-down relative to world and left-right relative to arm) - Use main hand analog stick.
4. Release `RepositionButtonID` to save for weapon.

### Offhand Mode

Use this mode to change the z-axis (up-down to the world) from where your offhand grabs the weapon. This is useful if there is a bigger stock. It is currently calibrated for palm-up grips.

1. Select weapon
2. Grip with off hand
3. Hold `RepositionButtonID` until haptic pulse (default 1 second `HoldDelay`) to enter reposition mode. This will start in `Weapon Reposition Mode`.
4. Click `OffHandActivateButtonID` to cycle to `Offhand Mode`. 
   - The main hand will show the wand. 
   - The offhand will no longer be attached to the weapon.
   - Move the offhand the distance below the gun where you want to grip. For best results, keep gun flat.
5. Release `RepositionButtonID` to save for offhand.

### Reset to Default Mode

Use this mode to reset to game defaults. This will delete all json files for the weapon.

1. Select weapon
2. Grip with off hand
3. Hold `RepositionButtonID` until haptic pulse (default 1 second `HoldDelay`) to enter reposition mode. This will start in `Weapon Reposition Mode`.
4. Click `OffHandActivateButtonID` to cycle to `Reset to Default Mode`. 
    - Both hands will show the wand. 
    - Weapon translation with the offhand and main hand analog movement stick will be disabled. The weapon will act as if not in reposition mode.
5. Release `RepositionButtonID` to delete offset for weapon.

## Scope Reticle Repositioning

Scopes may need to be calibrated so the reticles are true to the impact.

## Reticle Position

1. Select weapon
2. Bring scope close to HMD to enter scope zoom mode. (Default 20 units, BetterScopesVR ini `lookScopeDistanceThreshold`)
3. Bring offhand close to scope (default 15 units `ScopeAdjustDistance`)
4. Hold `RepositionButtonID` until haptic pulse (default 1 second `HoldDelay`) to enter reposition mode.
5. Use main hand analog stick to adjust reticle. Movement sensitivity configurable in BetterScopesVR.ini `retMoveInterval`.
6. Release `RepositionButtonID` to save.

## Reticle Reset
1. Select weapon
2. Bring scope close to HMD to enter scope zoom mode. (Default 20 units, BetterScopesVR ini `lookScopeDistanceThreshold`)
3. Bring offhand close to scope (default 15 units `ScopeAdjustDistance`)
4. Hold `RepositionButtonID` until haptic pulse (default 1 second `HoldDelay`) to enter reposition mode.
5. Click `OffHandActivateButtonID` to reset to default. You should see the reticle return to the default position.
6. Release `RepositionButtonID` to save.

# Scope Zoom Toggling

Scope zooms can be adjusted from lower magnitudes up to the scope's maximum defined zoom. The steps are defined in Better Scopes VR's ini `ZoomValues`. The defaults are `2.5,4.0,8.0`.

1. Select weapon
2. Bring scope close to HMD to enter scope zoom mode. (Default 20 units, BetterScopesVR ini `lookScopeDistanceThreshold`)
3. Bring offhand close to scope (default 15 units `ScopeAdjustDistance`)
4. Click `OffHandActivateButtonID` to cycle through available scope zooms. You should see the zoom change. The setting is saved automatically.
