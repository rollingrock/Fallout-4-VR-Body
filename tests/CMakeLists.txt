# FRIK's unit tests, run with ctest, in two executables built of the framework's test pieces
# (external/F4VR-CommonFramework/docs/mod-tests.md):
# - The files of this folder test sources under src/ that are plain std: each is compiled here by itself, with no
#   PCH, F4SE or CommonLibF4. Keep pure logic in such headers and standalone .cpp files, so it is tested this way.
# - The files of linked/ test code that needs the framework and CommonLibF4, and not the game to run.
# Test files sit in a folder per src/ area (weapon-position/, ...), and the folder starts each test's CTest name
# ("weapon-position: Grip mode: ..."). A test file is named *Tests.cpp, and its name is unique across the folders.

if(NOT TARGET F4VRCommon::test_main)
  message(FATAL_ERROR ">> Catch2 not found for the unit tests. Add \"catch2\" to vcpkg.json, or configure with -DFRIK_BUILD_TESTS=OFF.")
endif()

add_executable(FRIK_tests
  FramePhaseRegistryTests.cpp
  ScopeAuthorityTests.cpp
  TagBlockSetTests.cpp
  api/ClaimLogDebounceTests.cpp
  skeleton/RotationSmoothingTests.cpp
  weapon-position/GripDetectionTests.cpp
  weapon-position/GripModeTests.cpp
  # the plain-std sources under test
  ${SOURCE_DIR}/weapon-position/GripMode.cpp
  # the framework's plain-std perf sources, which the frame phase callbacks are timed with
  ${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}/src/perf/FrameContext.cpp
  ${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}/src/perf/Perf.cpp
)

# No link-time optimization, which the framework's test pieces are not compiled for, and MSVC's link-time code
# generation of perf/Perf.cpp crashed (C1001) in a test binary without perf/Report.cpp
set_target_properties(FRIK_tests PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

target_compile_features(FRIK_tests PRIVATE cxx_std_23)
# the framework's src/ for its plain-std headers, such as perf/Perf.h
target_include_directories(FRIK_tests PRIVATE ${SOURCE_DIR} ${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}/src)
# toml++ for weapon-position/GripMode, header-only here: the test binary does not link the framework's compiled toml++
find_path(TOMLPLUSPLUS_INCLUDE_DIR "toml++/toml.hpp" REQUIRED)
target_include_directories(FRIK_tests SYSTEM PRIVATE ${TOMLPLUSPLUS_INCLUDE_DIR})
target_compile_options(FRIK_tests PRIVATE /utf-8 /permissive- /W4 /external:anglebrackets /external:W0)
target_link_libraries(FRIK_tests PRIVATE F4VRCommon::test_main)

f4cf_discover_tests_by_folder(FRIK_tests)


# >>> The tests of linked/, linked with the framework and CommonLibF4. There is no game in the process:
# F4VRCommon::test_no_game sets REL::NO_GAME, so a game address resolves into a range that cannot be used, and a test
# that calls the game fails with an access violation.
add_executable(FRIK_linked_tests
  linked/ConfigTests.cpp
  # FRIK's sources under test
  ${SOURCE_DIR}/Config.cpp
  ${SOURCE_DIR}/weapon-position/GripMode.cpp
  # FRIK's embedded resources, which the tests read from the executable as FRIK does from its DLL
  ${BUILD_DIR}/resources.rc
)

set_target_properties(FRIK_linked_tests PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
target_compile_features(FRIK_linked_tests PRIVATE cxx_std_23)
target_compile_definitions(FRIK_linked_tests PRIVATE _UNICODE)
target_include_directories(FRIK_linked_tests PRIVATE ${BUILD_DIR}/include ${SOURCE_DIR})
target_precompile_headers(FRIK_linked_tests PRIVATE ${SOURCE_DIR}/PCH.h)
target_compile_options(FRIK_linked_tests PRIVATE /utf-8 /permissive- /W4 /external:anglebrackets /external:W0)
target_link_libraries(FRIK_linked_tests PRIVATE F4VRCommon::framework F4VRCommon::test_main F4VRCommon::test_no_game delayimp)

# openvr_api.dll is in the game's folder and not here, and the executable would not start without it. So it is
# loaded on its first call, which no test makes. Release drops the unused import whole, hence the ignored warning.
target_link_options(FRIK_linked_tests PRIVATE /DELAYLOAD:openvr_api.dll /IGNORE:4199)

f4cf_discover_tests_by_folder(FRIK_linked_tests)
