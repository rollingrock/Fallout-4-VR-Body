cmake_minimum_required(VERSION 4.2)


# >>> Project
set(NAME "FRIK")
set(FRIENDLY_NAME "FRIK")
set(VERSION 0.77.3)

message(">>> Start build for '${NAME}' v${VERSION}")

# >>> Folders
set(ROOT_DIR  "${CMAKE_CURRENT_SOURCE_DIR}")
set(SOURCE_DIR "${ROOT_DIR}/src")
set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# >>> Include guards
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR ">> In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# >>> Set variable from environment by same name
macro(set_from_environment VARIABLE)
  if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
    set(${VARIABLE} $ENV{${VARIABLE}})
  endif()
endmacro()


# >>> Setup vcpkg
message(">>> Setup vcpkg")
set_from_environment(VCPKG_ROOT)
if(DEFINED VCPKG_ROOT)
  set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
  set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
else()
  message(FATAL_ERROR ">> Variable VCPKG_ROOT is not set!")
endif()


# >>> Locate F4VR-CommonFramework (submodule or external path)
if(DEFINED F4VR_COMMON_FRAMEWORK_PATH AND NOT F4VR_COMMON_FRAMEWORK_PATH STREQUAL "")
  set(F4VR_COMMON_FRAMEWORK_PATH_SELECTED ${F4VR_COMMON_FRAMEWORK_PATH} CACHE STRING "" FORCE)
  message(">>> Using F4VR-CommonFramework from custom variable: '${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}'")
else()
  set(F4VR_COMMON_FRAMEWORK_PATH_SELECTED "${ROOT_DIR}/external/F4VR-CommonFramework" CACHE STRING "" FORCE)
  message(">>> Using F4VR-CommonFramework in external: '${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}'")
endif()
# Verify path is correct
if(NOT EXISTS "${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}/src/ModBase.h")
  message(FATAL_ERROR ">> F4VR-CommonFramework path not found in '${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}'!\n >> Did you init submodules? (git submodule update --init --recursive)\n >> or set F4VR_COMMON_FRAMEWORK_PATH?")
endif()

# >>> Create Project
project(${NAME} VERSION ${VERSION} LANGUAGES CXX)
message(">>> Building project '${PROJECT_NAME}' v:${PROJECT_VERSION}")
message(">>> Using found F4VR-CommonFramework in: '${F4VR_COMMON_FRAMEWORK_PATH_SELECTED}'")

# >>> Build flags
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")
set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

# >>> Bring in the framework (it brings CommonLibF4 transitively)
add_subdirectory(${F4VR_COMMON_FRAMEWORK_PATH_SELECTED} "F4VR-CommonFramework" EXCLUDE_FROM_ALL)
# >>> IMPORTANT:
# For CommonLibF4 to build successfully you must satisfy it's vcpkg dependencies
# AND specify <"builtin-baseline": "b4a3d89125e45bc8f80fb94bef9761d4f4e14fb9"> as it will fail to build with latests"


# >>> Version & resource files
configure_file(${ROOT_DIR}/cmake/version.rc.in  ${BUILD_DIR}/version.rc  @ONLY)
configure_file(${ROOT_DIR}/cmake/Version.h.in   ${BUILD_DIR}/include/Version.h @ONLY)
configure_file(${ROOT_DIR}/cmake/resources.rc.in ${BUILD_DIR}/resources.rc @ONLY)
source_group(TREE ${BUILD_DIR} FILES ${BUILD_DIR}/include/Version.h ${BUILD_DIR}/resources.rc)

# >>> Sources
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${SOURCE_DIR}/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE inls    CONFIGURE_DEPENDS "${SOURCE_DIR}/*.inl")
source_group(TREE ${ROOT_DIR} FILES ${headers} ${sources} ${inls})

# >>> Resource files
list(APPEND resource_files
  "${ROOT_DIR}/data/config/${NAME}.ini"
  "${ROOT_DIR}/data/config/mesh_hide_face.ini"
  "${ROOT_DIR}/data/config/mesh_hide_skins.ini"
  "${ROOT_DIR}/data/config/mesh_hide_slots.ini"
  "${ROOT_DIR}/data/config/pipboy_holo_offsets.json"
  "${ROOT_DIR}/data/config/pipboy_screen_offsets.json"
)
# Add all weapons offset files
file(GLOB weapons_offset_files "${ROOT_DIR}/data/config/weapons_offsets/*.json")
list(APPEND resource_files ${weapons_offset_files})
source_group("Resource Files" FILES ${resource_files})

# >>> Target (DLL)
add_library(${PROJECT_NAME} SHARED
  ${headers}
  ${sources}
  ${inls}
  ${resource_files}
  ${BUILD_DIR}/include/Version.h
  ${BUILD_DIR}/version.rc
  ${BUILD_DIR}/resources.rc
  "${ROOT_DIR}/.clang-format"
  "${ROOT_DIR}/.editorconfig"
)

# >>> C++ standard & defs
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE)

# >>> Includes
target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${BUILD_DIR}/include
    ${SOURCE_DIR}
)

# >>> Dependencies
# Link only the framework; it re-exports CommonLibF4 and other usage requirements.
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    F4VRCommon::framework
)

# cpptrace via vcpkg
find_package(cpptrace CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE cpptrace::cpptrace)

# >>> PCH
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

# >>> Compile / Link options
target_compile_options(${PROJECT_NAME}
  PRIVATE
    /sdl # Enable Additional Security Checks
	/utf-8 # Set Source and Executable character sets to UTF-8
	/Zi # Debug Information Format

	/permissive- # Standards conformance

	/Zc:alignedNew # C++17 over-aligned allocation
	/Zc:auto # Deduce Variable Type
	/Zc:char8_t
	/Zc:__cplusplus # Enable updated __cplusplus macro
	/Zc:externC
	/Zc:externConstexpr # Enable extern constexpr variables
	/Zc:forScope # Force Conformance in for Loop Scope
	/Zc:hiddenFriend
	/Zc:implicitNoexcept # Implicit Exception Specifiers
	/Zc:lambda
	/Zc:noexceptTypes # C++17 noexcept rules
	/Zc:preprocessor # Enable preprocessor conformance mode
	/Zc:referenceBinding # Enforce reference binding rules
	/Zc:rvalueCast # Enforce type conversion rules
	/Zc:sizedDealloc # Enable Global Sized Deallocation Functions
	/Zc:strictStrings # Disable string literal type conversion
	/Zc:ternary # Enforce conditional operator rules
	/Zc:threadSafeInit # Thread-safe Local Static Initialization
	/Zc:tlsGuards
	/Zc:trigraphs # Trigraphs Substitution
	/Zc:wchar_t # wchar_t Is Native Type

	/external:anglebrackets
	/external:W0

    /W4 # Warning level
    "$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
)

target_link_options(${PROJECT_NAME}
  PRIVATE
    /WX # Treat Linker Warnings as Errors
    "$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
    "$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
)

# >>> fix a warning with /Ob2 being overridden with /Ob3
string(REPLACE "/Ob2" "/Ob3" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# >>> Post build command to copy plugins (supports multiple paths separated by ';')
if(POST_BUILD_COPY_PLUGIN)
  if(DEFINED COPY_PLUGIN_BASE_PATH)
    # Treat semicolon-separated COPY_PLUGIN_BASE_PATH as a CMake list
    set(COPY_PLUGIN_PATH_LIST ${COPY_PLUGIN_BASE_PATH})

    message(">>> Create post build command: copy targets to:")
    foreach(copy_path IN LISTS COPY_PLUGIN_PATH_LIST)
      message("    ${copy_path}/F4SE/Plugins/")
      add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMENT "Copying ${PROJECT_NAME}.dll and .pdb to plugins folder: '${copy_path}/F4SE/Plugins/'"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${copy_path}/F4SE/Plugins/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${copy_path}/F4SE/Plugins/
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> ${copy_path}/F4SE/Plugins/
      )
    endforeach()
  else()
    message(FATAL_ERROR "Variable 'COPY_PLUGIN_BASE_PATH' is not defined but 'POST_BUILD_COPY_PLUGIN' is true!")
  endif()
else()
  message(">>> No post build copy")
endif()

# >>> Post build for release only to package the mod into a .7z file
message(">>> Create post build command: package mod on release build")
add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND}
          -DPROJECT_NAME=${PROJECT_NAME}
          -DPROJECT_FRIENDLY_NAME=${FRIENDLY_NAME}
          -DPROJECT_VERSION=${PROJECT_VERSION}
          -DROOT_DIR=${ROOT_DIR}
          -DBUILD_DIR=${BUILD_DIR}
          -DTARGET_FILE="$<TARGET_FILE:${PROJECT_NAME}>"
          -DTARGET_PDB_FILE="$<TARGET_PDB_FILE:${PROJECT_NAME}>"
          -DCONFIG=$(Configuration)
          -P "${ROOT_DIR}/cmake/package.cmake"
)
