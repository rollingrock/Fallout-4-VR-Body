## What changed in the last release?
Check the [Changelog](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Changelog).


## How Do I Configure?
See [In‐Game-Configuration-Guide](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/In%E2%80%90Game-Configuration-Guide-(v69)).


## Find FRIK.ini for All Config Options
Location: `%USERPROFILE%\Documents\My Games\Fallout4VR\FRIK_Config\FRIK.ini`  
Most configuration is stored in FRIK.ini file. Much more than can be configured via in-game configuration interfaces.
Any changes made to FRIK.ini will be live loaded into the running game to experience the effects.


## I don't see my head!
The head hiding is all in `FRIK.ini` config.  
Set `HideHead` and `HideEquipment` to false if you want full head to be rendered but it may obscure/clip your vision.  
If you just want to see the head in selfie set `selfieIgnoreHideFlags` to true.


## How do I Operate the Flashlight
Long press on secondary hand trigger activates the flashlight.  
To move the flashlight from head to hand: move the left or right hand to the forehead, should feel haptic, press grip.  


## What are the Different Grip Modes?
Grip modes affect how to grip the weapon with the offhand:  
* Mode 1: Hand automatically snap to the barrel when in range, move hand quickly to let go.
* Mode 2: Hand automatically snap to the barrel when in range, press grip button to let go.
* Mode 3: Holding grip button to snap to the barrel, release to let go.
* Mode 4: Press grip button to snap to the barrel, press again to let go.


## GAME CRASHED!
It happens, it could be bad install (see [installation wiki](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Installation)), could be related to FRIK, related to conflict between FRIK and other mods, and it could be unrelated to FRIK.  

### Known Crashes
- Not updating `Fallout4Custom.ini` during manual install, see [installation](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Installation) wiki page.
- "Amazing Follower Tweaks" mod is known to conflict with FRIK in the past.
- Anything that messes directly with the **skeleton** can cause a crash (body, heels, etc.).
- New Game reported in the past as problematic, I don't believe it still is, but:
  - Disabling before starting a game and re-enabling just after or after exiting the vault would probably be good enough.
  - The last confirmed new game conflict was for a combination of 3 mods with skeleton change ([#107](https://github.com/rollingrock/Fallout-4-VR-Body/issues/107)).

### Reporting Crash Information
If I, the developer, can reproduce the crash in 95% of the times I can fix it. Help me do so:
1. Describe in as much details as possible what led to the crash.  
  Are you using MO2? Is fallout game folder outside program files? 
2. Identify what combination of mods are causing the crash.  
  Does it crash when disabling FRIK? What other mods do you have?
3. Add crash log and FRIK log in the report.

### Getting Logs
- For collecting F4VR crash log you need [Buffout 4 NG](https://www.nexusmods.com/fallout4/mods/64880) mod.  
- Logs are found in `%USERPROFILE%\Documents\My Games\Fallout4VR\F4SE`  
  - `crash-<date time>.log` for crash logs
  - `FRIK.log` for FRIK logs
  - `f4sevr.log` and `f4sevr_loader.log` for mod loading failures.
- Share the logs via [pastebin.com](https://pastebin.com)
- **Bonus points** if you set `iLogLevel = 0` in FRIK.ini before reproducing the crash for more debug logs (performance expensive).


## When I launch the game, I do not see a body!

It means FRIK was not loaded into your game (even if you see the holotape).

- Most commonly caused by not launching your game with `f4sevr_loader.exe`. Make sure you have your F4SEVR install up to date and that you're launching it correctly with your mod manager.
- Possible FRIK was not installed correctly, common with manual install.
- **Recommended to use MO2** as the best manager for complex mods like FRIK.
  - [Example of a player issue](https://www.reddit.com/r/fo4vr/comments/1n97xs8/frik_doesnt_work_but_every_other_mod_does/) that was only solved by using MO2!

### MO2
Watch this guide: [YouTube - MO2 F4SE Setup](https://www.youtube.com/watch?v=-rFwk4tb6Ew)

### Vortex

**WARNING:** Many players had weird issues with Vortex, using MO2 is highly recommended!

It's the **same** way you set up F4SE to launch with NMM:
1. Click on **Dashboard**
2. Click **Add Tool**
3. Navigate to F4SE and click on `F4SEVR_Loader.exe`
4. Click **OK**
5. Back at the dashboard, click on the three vertical dots next to F4SE and pick **Make Primary**

Now, whenever you start the game with Vortex, it will launch `F4SE_Launcher`.

**Source:** [Nexus Mods Forum](https://forums.nexusmods.com/index.php?/topic/7064136-launching-fallout-4-with-f4se/)


## A Lot of Vanilla Weapons are Not Aligned in Hands
You're probably using mods that contain fixes for VR that are not required anymore for FRIK and actually causing it. [VR Weapon Overhaul](https://www.nexusmods.com/fallout4/mods/64610) is a common one.  
Either remove it, find a way to remove the VR fix in it, or adjust the weapon in FRIK using [Weapon Adjustment](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Weapon-Adjustment-Guide-(v73)).


## Keep triggering VATS when using On-Wrist Pipboy
You're probably using [Bullet Time VATS VR](https://www.nexusmods.com/fallout4/mods/72502). It doesn't respect game VATS disabling logic.  
Install this fix: [Bullet Time VATS VR - FRIK Pipboy Fix](https://www.nexusmods.com/fallout4/mods/99702)


## Armor / Power Armor obstructs my view!
Honestly, this is somewhat intentional—you’re wearing **bulky armor**, and some line-of-sight issues are expected based on Bethesda’s mesh design.  
Tip: Use configuration to move the body downwards or camera upwards a little if needed.


## I don't like the built-in flashlight, how to remove it?
Set `bRemoveFlashlight = true` in `FRIK.ini`  
It will remove the FRIK built-in flashlight.

If I don't want to remove FRIK flashlight, only change how it looks: see this [reddit comment](https://www.reddit.com/r/fo4vr/comments/1o54ejq/comment/nj9thh9/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button).


## Is hiding the head prevents getting hit in the head?
AFAIK, no!  
I tested it by hiding all body parts and making the entire player invisible. Yet in combat, every body part was still hit by bullets, causing damage and even crippling limbs. This behaves differently from the now-removed “ArmsOnlyMode,” where enemies definitely could not hit hidden body parts.


## Two Handed Weapon Aiming doesn't work in Vanilla V.A.T.S.
If you're using fully vanilla VATS please use one-handed aiming.


## Pipboy on the Right Hand is Broken
It needs A LOT of work.


## Why is the Config in "Documents\My Games\Fallout4VR\FRIK_Config"
[See this](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Development#frik-config-folder).

