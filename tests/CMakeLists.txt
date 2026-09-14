# Unit tests for game-independent logic. Sources under src/ that pull in F4SE or the
# framework cannot be tested here; keep the pure logic in headers or standalone .cpp files.

find_package(Catch2 3 CONFIG REQUIRED)

add_executable(FRIK_tests
  ScopeAuthorityTests.cpp
  TagBlockSetTests.cpp
)

target_compile_features(FRIK_tests PRIVATE cxx_std_23)
target_include_directories(FRIK_tests PRIVATE ${SOURCE_DIR})
target_compile_options(FRIK_tests PRIVATE /utf-8 /permissive- /W4 /external:anglebrackets /external:W0)
target_link_libraries(FRIK_tests PRIVATE Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(FRIK_tests)
