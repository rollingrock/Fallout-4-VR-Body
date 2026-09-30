# Unit tests for game-independent logic. Sources under src/ that pull in F4SE or the
# framework cannot be tested here; keep the pure logic in headers or standalone .cpp files.

find_package(Catch2 3 CONFIG REQUIRED)

add_executable(FRIK_tests
  ClaimLogDebounceTests.cpp
  FramePhaseRegistryTests.cpp
  GripDetectionTests.cpp
  ScopeAuthorityTests.cpp
  TagBlockSetTests.cpp
  # the framework's plain-std perf sources, which the frame phase callbacks are timed with
  ${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}/src/perf/FrameContext.cpp
  ${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}/src/perf/Perf.cpp
)

# no link-time optimization: the tests don't need it, and MSVC's link-time code generation of perf/Perf.cpp
# crashed (C1001) in a test binary without perf/Report.cpp
set_target_properties(FRIK_tests PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

target_compile_features(FRIK_tests PRIVATE cxx_std_23)
# the framework's src/ for its plain-std headers, such as perf/Perf.h
target_include_directories(FRIK_tests PRIVATE ${SOURCE_DIR} ${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}/src)
target_compile_options(FRIK_tests PRIVATE /utf-8 /permissive- /W4 /external:anglebrackets /external:W0)
target_link_libraries(FRIK_tests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(FRIK_tests)
