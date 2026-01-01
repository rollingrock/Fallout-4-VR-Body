**Requirements:**  
- [F4SE VR](https://f4se.silverlock.org/)
- [VR Address Library](https://www.nexusmods.com/fallout4/mods/64879)

### Instructions:

1. Install as normal through your mod manager and enable the esp
2. Load order is not important
3. Edit your `Fallout4Custom.ini` (this is a pain point and players complained that different settings work)  
  **Important:** Not having it may result in game crash on starting new game or loading a save.

This is what I have with clean install:
```
[Archive]
bInvalidateOlderFiles=1
sResourceIndexFileList=Fallout4 - Textures1.ba2, Fallout4 - Textures2.ba2, Fallout4 - Textures3.ba2, Fallout4 - Textures4.ba2, Fallout4 - Textures5.ba2, Fallout4 - Textures6.ba2, Fallout4 - Textures7.ba2, Fallout4 - Textures8.ba2, Fallout4 - Textures9.ba2
sResourceStartUpArchiveList=Fallout4 - Startup.ba2, Fallout4 - Shaders.ba2, Fallout4 - Interface.ba2
SResourceArchiveList=Fallout4 - Voices.ba2, Fallout4 - Meshes.ba2, Fallout4 - MeshesExtra.ba2, Fallout4 - Misc.ba2, Fallout4 - Sounds.ba2, Fallout4 - Materials.ba2
SResourceArchiveList2=Fallout4 - Animations.ba2
sResourceDataDirsFinal=STRINGS\
SGeometryPackageList=Fallout4 - Geometry.csg
SCellResourceIndexFileList=Fallout4.cdx
SResourceArchiveMemoryCacheList= Fallout4 - Misc.ba2, Fallout4 - Shaders.ba2, Fallout4 - Interface.ba2, Fallout4 - Materials.ba2
```

This was in the guide for a long time but some player report it doesn't work well: (originally by u/JAPH):  
```
[Archive]
sResourceDataDirsFinal=
bInvalidateOlderFiles=1
sResourceStartUpArchiveList=Fallout4 - Startup.ba2, Fallout4 - Shaders.ba2, Fallout4 - Interface.ba2, Fallout4_VR - Shaders.ba2
sResourceIndexFileList=Fallout4 - Textures1.ba2, Fallout4 - Textures2.ba2, Fallout4 - Textures3.ba2, Fallout4 - Textures4.ba2, Fallout4 - Textures5.ba2, 
Fallout4 - Textures6.ba2, Fallout4 - Textures7.ba2, Fallout4 - Textures8.ba2, Fallout4 - Textures9.ba2, Fallout4_VR - Main.ba2, Fallout4_VR - Textures.ba2
```

Another player said it still caused crashes and removed the DLCs helped him but didn't give more details.

There is [this guy](https://www.reddit.com/r/SteamDeck/comments/1pwxwb8/fallout_4_vr_finally_works_on_steam_deck_jitter/?share_id=ckyVBjlPoYRWIpem5kUA-&utm_medium=android_app&utm_name=androidcss&utm_source=share&utm_term=1) that had texture issue:
> I narrowed it down to the Fallout4custom.ini config and specifically the ini tweaks that FRIK tells you to use. I don't know if this is a Linux issue but it doesn't work on Linux. The fix was simple, remove all the texture b2a files and others from the list of things in the ini tweak and it works.


### Recommendations:

* If anyone's using HUIDE_VR previously alongside Idle Hands, make sure to reinstall HUIDE_VR after disabling Idle Hands and choose Vanilla in the installer instead. Kept crashing until I did. ( thanks u/Zebrazilla !!)
* Note that pipboy touch operation requires you to be looking at the screen and the screen to be facing your face to work.   
* Index con trollers: requires SteamVR controller bindings for finger tracking to work. Look for FRIK bindings published on Steam Workshop.
