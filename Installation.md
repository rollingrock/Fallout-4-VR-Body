**Requirements:**  
* [F4SEVR](https://f4se.silverlock.org/), 
* REQUIRES STEAMVR CONTROLLER BINDINGS FOR FINGER TRACKING TO WORK!!!! LOOK FOR FRIK BINDINGS PUBLISHED ON STEAM WORKSHOP

**Instructions:**  

* Install as normal through your mod manager and enable the esp
* Load order is not important

Legacy, I'm not sure it's really needed...

* You need to edit your Fallout4Custom.ini to add or amend the following lines to look like this (thanks u/JAPH):

  ```
  [Archive]
  sResourceDataDirsFinal=
  bInvalidateOlderFiles=1
  sResourceStartUpArchiveList=Fallout4 - Startup.ba2, Fallout4 - Shaders.ba2, Fallout4 - Interface.ba2, Fallout4_VR - Shaders.ba2
  sResourceIndexFileList=Fallout4 - Textures1.ba2, Fallout4 - Textures2.ba2, Fallout4 - Textures3.ba2, Fallout4 - Textures4.ba2, Fallout4 - Textures5.ba2, 
  Fallout4 - Textures6.ba2, Fallout4 - Textures7.ba2, Fallout4 - Textures8.ba2, Fallout4 - Textures9.ba2, Fallout4_VR - Main.ba2, Fallout4_VR - Textures.ba2
  ```

**Recommendations:**  
* On-wrist based Pipboy with the following scale settings in Fallout4Prefs.ini: (set the maxscale to taste)
  ```
  [VRPipboy]
  fPipboyMaxScale=3.0000
  fPipboyMinScale = 0.0100
  ```
* If anyone's using HUIDE_VR previously alongside Idle Hands, make sure to reinstall HUIDE_VR after disabling Idle Hands and choose Vanilla in the installer instead. Kept crashing until I did. ( thanks u/Zebrazilla !!)
* Note that pipboy touch operation requires you to be looking at the screen and the screen to be facing your face to work.   