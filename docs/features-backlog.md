## Planned
¯\\(°_o)/¯

### Backlog
- Sitting body position when the character is sitting.
- Locking offhand grip in two-handed mode ([see discussion](https://github.com/rollingrock/Fallout-4-VR-Body/discussions/105)).
- Improve selfie mode so the player character does not move and the camera can rotate around the character.
- Better hand pose for different weapons and throwables.
- Bug: two-handed scope alignment issue in comfort sneak. Better in v76, but not fully fixed ([Nexus post](https://www.nexusmods.com/fallout4/mods/53464?tab=posts&comment_id=159843574&BH=0)).
- Fix Pip-Boy on right arm not working.

### Engineering
Found with the perf work's Tracy captures (2026-09-29).
- Session load runs off the game thread: `onGameSessionLoaded`, with the `releaseSkeleton` in it and the devbench
  `sessionLoaded` event, ran on the thread that delivers the load message while the game thread kept running
  frames, so the skeleton can be destroyed in the middle of a frame that uses it. Defer the release to the next
  game-thread frame, in FRIK or in the framework's `ModBase` for every mod.
- The `NativeGraphOutput` frame phase fired once off the game thread during a load, with ROCK's callback in it,
  though phase callbacks are game-thread only: the detour at 0xF2F0A0 can run on a loading thread. Invoke the phase
  on the game thread only.
- Opening the main config menu costs one frame of about 53 ms, in `UIManager::onFrameUpdate` under
  `MainConfigMode::onFrameUpdate`, most likely loading the menu's NIFs. Preload them, or spread the loading over
  frames.

## Requested
Please put feature requests here for triage.
(Link to issue if long explanation is required.)
