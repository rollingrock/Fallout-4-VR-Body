# Fallout 4 VR Body - FRIK

Developing mod to add full body support with IK for Fallout 4 VR!

Now released on Nexus!!! https://www.nexusmods.com/fallout4/mods/53464/

## Dev Setup

Prerequisites:

- Install Visual Studio 2022 community edition
- Let's define `\root\` to be the root folder we will use (example: `\root\` for me is `C:\Dev\Modding\`)

### Setup `f4sevr`:

1. Download [Fallout 4 Script Extender (F4SE)](https://f4se.silverlock.org/)
2. Extract content of `f4sevr_0_6_21` folder into `\root\f4sevr`
3. Override changed files
   1. Git clone [`Fallout-4-VR-Body`](https://github.com/rollingrock/Fallout-4-VR-Body)
      - Should be `\root\Fallout-4-VR-Body\` for later
   2. Extract files in `\root\Fallout-4-VR-Body\f4se_updates_add_to_your_f4se_library\f4se_updates.zip` to `f4sevr\src\f4sevr\f4se` (overwrite)

### Build `FRIK`:

1. Git clone [`Fallout-4-VR-Body`](https://github.com/rollingrock/Fallout-4-VR-Body) (if not already from building "f4sevr")
   - Should be `\root\Fallout-4-VR-Body\`
2. Open `Fallout-4-VR-Body` in Visual Studio
   1. Double click `\root\Fallout-4-VR-Body\Fallout4VR_Body.sln`
3. Change target dropdown from "Debug" to "Release"
4. Setup post build copy environment variable
   1. `setx FRIK_MOD_PATH "C:\[myModsLocation]\FRIK\F4SE\Plugins"`
   2. Restart VS2022 to load the envar
5. Build > Build Solution
6. Find `\root\Fallout-4-VR-Body\x64\Release\FRIK.dll`

### Optional, build `f4sevr`:

1. Do all the steps in [setup `f4sevr`](#setup-f4sevr)
2. Open `f4sevr` solution in Visual Studio
   1. Double click `C:\Stuff\github\f4sevr\src\f4sevr\f4sevr.sln`
   2. Ignore all source control warnings
   3. Confirm retarget project to latest
      - Solution was configured for VS 2012 (v110), we can retarget it to latest 2022 (v143)
3. Fix compilation error due to more strict C++
   1. Open `PapyrusObjectReference.cpp`
   2. Go to line 558
   3. Add `const` to the end of the function declaration (just before the `{` bracket)
      - `bool operator()(const std::pair<BGSMaterialSwap::MaterialSwap*,float> lhs, const std::pair<BGSMaterialSwap::MaterialSwap*,float> rhs) const {`
4. Change target dropdown from "Debug" to "Release"
5. Change `f4se` project output configuration to Static Library
   1. Right click on `f4se` project; click Properties
   2. Configuration Properties > General;
   3. Change "Configuration Type" from "Dynamic Library (.dll)" to "Static Library (.lib)"
   4. close
6. Build > Build Solution
   - Ignore 3 post build errors to copy output files
7. Find the 3 `.lib` files
   1. `\root\f4sevr\src\f4sevr\x64\Release\f4se_common.lib`
   2. `\root\f4sevr\src\f4sevr\x64\Release\f4sevr_1_2_72.lib`
   3. `\root\f4sevr\src\f4sevr\x64_vc11\Release\common_vc11.lib`

## Tips

- Logs are found in `%USERPROFILE%\Documents\My Games\Fallout4VR\F4SE`

  - `Fallout4VRBody.log` for FRIK logs
  - `crash-<date time>.log` for crash logs

- Weapons Offset Overrides are in `<mod_list_folder>\overwrite\F4SE\Plugins\FRIK_weapon_offsets`  
  Where `<mod_list_folder>` can be `VR Essentials Overhaul` for example.

- Don't keep more than one `.dll` file in the `...\F4SE\Plugins` folder. For example, if you're backing up `FRIK.dll`, avoid renaming it to `FRIK_org.dll` in the same folder. This is because `F4SEVR` loads all `.dll` files in the directory, which results in the plugin being loaded twice. It appears to load plugins in alphabetical order, so `FRIK_org.dll` will be loaded after `FRIK.dll`, potentially causing issues and making it unclear why your code isn't running.

## Credits

- prog - I wouldn't get anywhere with this if not for the fine example of Skyrim VRIK. Thanks for all of the advice and code samples
- alandtse
- Shizof - Thanks for letting me adapt Smooth Movement directly into the mod
- Gingas
- CylonSurfer
- Atom
- lfrazer
- expired and F4SE/SKSE Team
- ryan for CommonLibF4 to reference
