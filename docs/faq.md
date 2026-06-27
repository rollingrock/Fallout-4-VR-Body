## What changed in the last release?
Check the [Changelog](changelog.md).


## How Do I Configure FRIK?
See the [In-Game Configuration Guide](in-game-configuration-guide.md).


## Where Do I Find FRIK.ini?
Location: `%USERPROFILE%\Documents\My Games\Fallout4VR\FRIK_Config\FRIK.ini`  
Most configuration is stored in the FRIK.ini file, including many settings that are not exposed through the in-game configuration interfaces.
Changes made to FRIK.ini are live-loaded into the running game.


## I don't see my head!
Head rendering is controlled in `FRIK.ini`. By default the head equipment is hidden (`bHidePlayerHeadEquipment = true`) and the head is pushed back out of view (`fHeadBackPositionOffset = 5`).
To render the full head, set `bHidePlayerHeadEquipment = false` and lower `fHeadBackPositionOffset` toward `0` (keep `bHidePlayerHead = false`). Be aware this may obscure or clip your vision.
If you only want to see the head in selfie mode, leave `bSelfieIgnoreHideFlags = true`.


## How Do I Operate the Flashlight?
Long-press the secondary-hand trigger to activate the flashlight.
To move the flashlight from your head to your hand, move either hand to your forehead. When you feel haptic feedback, press grip.


## What are the Different Grip Modes?
Grip modes affect how to grip the weapon with the offhand:  
* Mode 1: The hand automatically snaps to the barrel when in range; move your hand quickly to let go.
* Mode 2: The hand automatically snaps to the barrel when in range; press the grip button to let go.
* Mode 3: Hold the grip button to snap to the barrel; release it to let go.
* Mode 4: Press the grip button to snap to the barrel; press it again to let go.


## GAME CRASHED!
It happens. It could be a bad installation (see the [installation guide](installation.md)), related to FRIK, caused by a conflict between FRIK and other mods, or unrelated to FRIK.

### Known Crashes
- Not updating `Fallout4Custom.ini` during manual installation. See the [installation](installation.md) page.
- "Amazing Follower Tweaks" mod is known to conflict with FRIK in the past.
- Anything that messes directly with the **skeleton** can cause a crash (body, heels, etc.).
- New games were reported as problematic in the past. I don't believe they still are, but:
  - Disabling FRIK before starting a new game and re-enabling it after exiting the vault should be good enough.
  - The last confirmed new-game conflict involved a combination of 3 mods with skeleton changes ([#107](https://github.com/rollingrock/Fallout-4-VR-Body/issues/107)).

### Reporting Crash Information
If I, the developer, can reproduce the crash, I can usually fix it. Help me do that:
1. Describe in as much detail as possible what led to the crash.
  Are you using MO2? Is the Fallout game folder outside Program Files?
2. Identify what combination of mods are causing the crash.  
  Does it crash when FRIK is disabled? What other mods do you have?
3. Add the crash log and FRIK log to the report.

### Getting Logs
- To collect F4VR crash logs, you need the [Buffout 4 NG](https://www.nexusmods.com/fallout4/mods/64880) mod.
- Logs are found in `%USERPROFILE%\Documents\My Games\Fallout4VR\F4SE`  
  - `crash-<date time>.log` for crash logs
  - `FRIK.log` for FRIK logs
  - `f4sevr.log` and `f4sevr_loader.log` for mod loading failures.
- Share the logs via [pastebin.com](https://pastebin.com)
- **Bonus points** if you set `iLogLevel = 0` in FRIK.ini before reproducing the crash to capture more debug logs. This is performance-expensive.


## When I launch the game, I do not see a body!

It means FRIK did not load into your game, even if you see the holotape.

- Most commonly caused by not launching your game with `f4sevr_loader.exe`. Make sure you have your F4SEVR install up to date and that you're launching it correctly with your mod manager.
- FRIK may not have been installed correctly, which is common with manual installation.
- **MO2 is recommended** as the best manager for complex mods like FRIK.
  - [Example of a player issue](https://www.reddit.com/r/fo4vr/comments/1n97xs8/frik_doesnt_work_but_every_other_mod_does/) that was only solved by using MO2!

### MO2
Watch this guide: [YouTube - MO2 F4SE Setup](https://www.youtube.com/watch?v=-rFwk4tb6Ew)

### Vortex

**WARNING:** Many players have had weird issues with Vortex. Using MO2 is highly recommended.

Set up Vortex to launch Fallout 4 VR through F4SEVR:
1. Click on **Dashboard**
2. Click **Add Tool**
3. Navigate to F4SE and click on `F4SEVR_Loader.exe`
4. Click **OK**
5. Back at the dashboard, click on the three vertical dots next to F4SE and pick **Make Primary**

Now, whenever you start the game with Vortex, it will launch `F4SEVR_Loader.exe`.

**Source:** [Nexus Mods Forum](https://forums.nexusmods.com/index.php?/topic/7064136-launching-fallout-4-with-f4se/)


## A Lot of Vanilla Weapons Are Not Aligned in My Hands
You're probably using mods that contain VR fixes that are no longer required for FRIK and may actually cause this issue. [VR Weapon Overhaul](https://www.nexusmods.com/fallout4/mods/64610) is a common one.
Either remove it, find a way to remove the VR fix in it, or adjust the weapon in FRIK using [Weapon Adjustment](weapon-adjustment-guide.md).


## I Keep Triggering VATS When Using the On-Wrist Pip-Boy
You're probably using [Bullet Time VATS VR](https://www.nexusmods.com/fallout4/mods/72502). It doesn't respect game VATS disabling logic.  
Install this fix: [Bullet Time VATS VR - FRIK Pipboy Fix](https://www.nexusmods.com/fallout4/mods/99702)


## Armor / Power Armor Obstructs My View!
Honestly, this is somewhat intentional: you're wearing **bulky armor**, and some line-of-sight issues are expected based on Bethesda's mesh design.
Tip: Use configuration to move the body down or the camera up a little if needed.


## I Don't Like the Built-In Flashlight. How Do I Remove It?
Set `bRemoveFlashlight = true` in `FRIK.ini`  
It will remove the FRIK built-in flashlight.

If you do not want to remove the FRIK flashlight and only want to change how it looks, see this [reddit comment](https://www.reddit.com/r/fo4vr/comments/1o54ejq/comment/nj9thh9/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button).


## Does Hiding the Head Prevent Getting Hit in the Head?
AFAIK, no!  
I tested it by hiding all body parts and making the entire player invisible. In combat, every body part was still hit by bullets, causing damage and even crippling limbs. This behaves differently from the now-removed `ArmsOnlyMode`, where enemies definitely could not hit hidden body parts.


## Two-Handed Weapon Aiming Doesn't Work in Vanilla V.A.T.S.
If you're using fully vanilla VATS please use one-handed aiming.


## Pip-Boy on the Right Hand Is Broken
It needs A LOT of work.


## Why Is the Config in "Documents\My Games\Fallout4VR\FRIK_Config"?
[See this](development.md#frik-config-folder).

