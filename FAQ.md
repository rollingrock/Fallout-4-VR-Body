## What changed in the last release?
Check the [CHANGELOG](https://github.com/rollingrock/Fallout-4-VR-Body/blob/main/CHANGELOG.md).

## How Do I configure?
See [In‐Game-Configuration-Guide](https://github.com/rollingrock/Fallout-4-VR-Body/wiki/In%E2%80%90Game-Configuration-Guide-(v69)).

## I don't see my head!
The head hiding is all in FRIK.ini config.  
Set `HideHead` and `HideEquipment` to false if you want full head to be rendered but it may obscure/clip your vision.  
If you just want to see the head in selfie set `selfieIgnoreHideFlags` to true.

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