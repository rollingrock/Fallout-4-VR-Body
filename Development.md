## Runtime Requirements
- [F4SE/VR](https://f4se.silverlock.org/)
- [VR Address Library for F4SEVR Plugins](https://www.nexusmods.com/fallout4/mods/64879)

## Development Requirements
- [CMake](https://cmake.org/)
- [Vcpkg](https://github.com/microsoft/vcpkg)
  - `git clone https://github.com/microsoft/vcpkg.git`
  - run `bootstrap-vcpkg.bat`. Example: `C:\github\vcpkg\bootstrap-vcpkg.bat`
  - Set environment variable `VCPKG_ROOT`. Example: `setx VCPKG_ROOT "C:\github\vcpkg"`
- [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
  - Desktop development with C++

## Building
Clone repo and setup CommonLibF4 submodule:
```
git clone https://github.com/rollingrock/Fallout-4-VR-Body.git
cd Fallout-4-VR-Body
git submodule update --init --recursive
```

Create VS2022 solution:
```
cmake --preset default
```
- Open `.../Fallout-4-VR-Body/build/FRIK.sln` in VS2022.
  - Build and debug in VS as usual
  - Any project changes should be done in `CMakeLists.txt`
- see `CMakeUserPresets.json.template` to customize presets by providing path for post-build event.
  - Use `cmake --preset custom`

Build and package:
```
cmake --preset default
cmake --build buildvr --config Release
```
- Will automatically create a 7zip package of the mod in `build/package`

---

## Tips

- Check those wikis
  - [Skyrim CommonLib - Getting Started](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/wiki/Getting-Started) 
  - [Skyrim CommonLib - Query and Load](https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/wiki/Query-and-Load)

- Logs are found in `%USERPROFILE%\Documents\My Games\Fallout4VR\F4SE`
  - `FRIK.log` for FRIK logs
  - `crash-<date time>.log` for crash logs
  - use `tail -f "%USERPROFILE%\Documents\My Games\Fallout4VR\F4SE\FRIK.log"` to tail the log

- To be able to attach debugger and use breakpoint you need [Steamless](https://github.com/atom0s/Steamless)
  - Get latest [Steamless](https://github.com/atom0s/Steamless/releases)
  - Unpack `Fallout4VR.exe` (use default settings)
  - Rename `Fallout4VR.exe.unpacked.exe` to `Fallout4VR.exe`
  - Add `-ForceSteamLoader` argument when running the game via `f4sevr_loader.exe` (or you will get buffout was loaded too late)

- Open win environment variables edit using `rundll32.exe sysdm.cpl,EditEnvironmentVariables`

- Don't keep more than one `.dll` file in the `...\F4SE\Plugins` folder. For example, if you're backing up `FRIK.dll`, avoid renaming it to `FRIK_org.dll` in the same folder. This is because `F4SEVR` loads all `.dll` files in the directory, which results in the plugin being loaded twice. It appears to load plugins in alphabetical order, so `FRIK_org.dll` will be loaded after `FRIK.dll`, potentially causing issues and making it unclear why your code isn't running.

## FRIK Config Folder
All FRIK configuration files are located in `%USERPROFILE%\Documents\My Games\Fallout4VR\FRIK_Config`.  
This folder is shared across all versions and instances of FRIK.  
Using a shared location was necessary due to two constraints: configuration files cannot be included in the mod distributable (ZIP file), and Mod Organizer 2 (MO2) restricts write access to the mods folder ([details](https://stepmodifications.org/wiki/Guide:Mod_Organizer#Overwrite), [more details](https://stepmodifications.org/wiki/Guide:Mod_Organizer/Advanced)).  
We avoid including config files in the distributable to prevent user configuration from being overwritten during upgrades. Instead, FRIK.dll includes a default configuration, which is used if no existing config is found on disk. However, since MO2 does not allow writing to the mods folder and instead redirects all writes to its "[overwrite](https://stepmodifications.org/wiki/Guide:Mod_Organizer#Overwrite)" folder, this causes config files to be stored in an unexpected location. To avoid this confusion, we chose to use the standard %USERPROFILE%\Documents\My Games\Fallout4VR\FRIK_Config path, which is consistent and accessible outside of MO2’s virtual file system.
