## What changed in the last release?
Check the [Changelog](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Changelog).

## Any Known Issues?
[Yes, a few](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Known-Issues).

## How Do I configure?
See [In‐Game-Configuration-Guide](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/In%E2%80%90Game-Configuration-Guide-(v69)).

## I don't see my head!
The head hiding is all in FRIK.ini config.  
Set `HideHead` and `HideEquipment` to false if you want full head to be rendered but it may obscure/clip your vision.  
If you just want to see the head in selfie set `selfieIgnoreHideFlags` to true.

## How do I Operate the Flashlight
Long press on secondary hand trigger activates the flashlight.  
To move the flashlight from hand to head: move the Pipboy hand to the head, should feel haptic, press grip.  

## Reporting Crash Log
Please attach the crash log file and FRIK main log file.
Logs are found in `%USERPROFILE%\Documents\My Games\Fallout4VR\F4SE`  
- `crash-<date time>.log` for crash logs
- `Fallout4VRBody.log` for FRIK logs

## Why is the Config in "Documents\My Games\Fallout4VR\FRIK_Config"
[See this](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/Development#frik-config-folder).

## When I launch the game, I do not see a body!!

Most commonly caused by not launching your game with `f4sevr_loader.exe`. Make sure you have your F4SEVR install up to date and that you're launching it correctly with your mod manager.

### Vortex
It's the **same** way you set up F4SE to launch with NMM:
1. Click on **Dashboard**
2. Click **Add Tool**
3. Navigate to F4SE and click on `F4SEVR_Loader.exe`
4. Click **OK**
5. Back at the dashboard, click on the three vertical dots next to F4SE and pick **Make Primary**

Now, whenever you start the game with Vortex, it will launch `F4SE_Launcher`.

**Source:** [Nexus Mods Forum](https://forums.nexusmods.com/index.php?/topic/7064136-launching-fallout-4-with-f4se/)

### MO2
Watch this guide: [YouTube - MO2 F4SE Setup](https://www.youtube.com/watch?v=-rFwk4tb6Ew)

---

## My game crashes!!

- The most common cause of crashes is follower mods like **Amazing Follower Tweaks**. I still don’t know exactly what causes this. Once I do, I’ll work on compatibility.
- Anything that messes directly with the **skeleton** can cause a crash.
- Sometimes loading into a save can crash. Often this can be fixed by:
  - Completely exiting the game
  - Reloading from the **main menu**

This is still **alpha** software, so not all crash vectors are fixed, but progress is ongoing.  
If you find a **reproducible crash**, please post mod and crash logs!

## Armor / Power Armor obstructs my view!

Honestly, this is somewhat intentional—you’re wearing **bulky armor**, and some line of sight issues are expected based on Bethesda’s mesh design.  
Tip: Use configuration to move the body downwards or camera upwards a little if needed.

## Configuration changes is not being saved!

Usually due to **permission issues**. Try:
- Running the game from a directory with **full write permission**
- Launching as **Admin** (less recommended)

## Can I have Holsters for my weapons?

Check out **CylonSurfer's** excellent mod:  [Virtual Holsters](https://www.nexusmods.com/fallout4/mods/51224)

## I don't like the built-in torch, how to remove it?
1. Open xEdit (F4VREdit)
2. Click on FRIK.esp
3. Select "Pipboy" under Armor
4. Find and Remove "PowerArmorHelmetLightOverride"

![image](https://github.com/user-attachments/assets/fec4c7d4-2c2d-4f80-9b7a-1d177ebdfb5d)
