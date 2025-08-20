cmake_minimum_required(VERSION 3.23)


# >>> Project
set(NAME "FRIK")
set(FRIENDLY_NAME "FRIK")
set(VERSION 0.75.3)


# >>> Folders
set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
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
set_from_environment(VCPKG_ROOT)
if(DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
else()
	message(FATAL_ERROR ">> Variable VCPKG_ROOT is not set!")
endif()


# >>> Setup CommonLibF4 path
set_from_environment(CommonLibF4Path)
find_path(CommonLibF4PathLocal
	CommonLibF4/include/REL/Relocation.h
	PATHS
	  ${CommonLibF4Path}
	  CommonLibF4
	  external/CommonLibF4
	  ../CommonLibF4
)
if(${CommonLibF4PathLocal} STREQUAL "CommonLibF4PathLocal-NOTFOUND")
	message(FATAL_ERROR ">> CommonLibF4 Path not found! didn't init git modules? or not set CommonLibF4Path env variable?")
endif()


# >>> Create Project
project(
	${NAME}
	VERSION ${VERSION}
	LANGUAGES CXX
)
message(">>> Building '${PROJECT_NAME}' v:${PROJECT_VERSION}")
message(">>> Using CommonLibF4: '${CommonLibF4PathLocal}'")


# >>> Set build flags
# Suppresses Microsoft's "unsafe function" warnings (like strcpy, sprintf, etc.) in MSVC.
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
# Optimizations
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
set(Boost_USE_STATIC_LIBS ON)


# >>> Build CommonLibF4 dependency for Fallout-4-VR
set(F4SE_SUPPORT_XBYAK ON CACHE BOOL "")
set(ENABLE_FALLOUT_NG OFF CACHE BOOL "")
set(ENABLE_FALLOUT_F4 OFF CACHE BOOL "")
set(ENABLE_FALLOUT_VR ON CACHE BOOL "")
if(NOT F4SE_SUPPORT_XBYAK OR ENABLE_FALLOUT_F4 OR ENABLE_FALLOUT_NG OR NOT ENABLE_FALLOUT_VR)
    message(FATAL_ERROR ">> Invalid CommonLibF4 config, try deleting buildvr and rebuilding")
endif()
add_subdirectory(${CommonLibF4PathLocal} "CommonLibF4" EXCLUDE_FROM_ALL)
# >>> IMPORTANT:
# For CommonLibF4 to build successfully you must satisfy it's vcpkg dependencies
# AND specify <"builtin-baseline": "b4a3d89125e45bc8f80fb94bef9761d4f4e14fb9"> as it will fail to build with latests"


# >>> Add version files
configure_file(
	${ROOT_DIR}/cmake/version.rc.in
	${BUILD_DIR}/version.rc
	@ONLY
)
configure_file(
	${ROOT_DIR}/cmake/Version.h.in
	${BUILD_DIR}/include/Version.h
	@ONLY
)
configure_file(
	${ROOT_DIR}/cmake/resources.rc.in
	${BUILD_DIR}/resources.rc
	@ONLY
)
source_group(
	TREE ${BUILD_DIR}
	FILES ${BUILD_DIR}/include/Version.h ${BUILD_DIR}/resources.rc
)


# >>> Add all headers and source files in src folder
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${SOURCE_DIR}/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE inls CONFIGURE_DEPENDS "${SOURCE_DIR}/*.inl")
source_group(
	TREE ${ROOT_DIR}
	FILES ${headers} ${sources}	${inls}
)

# >>> Add resource files
list(APPEND resource_files
	"${ROOT_DIR}/data/config/FRIK.ini"
	"${ROOT_DIR}/data/config/mesh_hide_face.ini"
	"${ROOT_DIR}/data/config/mesh_hide_skins.ini"
	"${ROOT_DIR}/data/config/mesh_hide_slots.ini"
	"${ROOT_DIR}/data/config/pipboy_holo_offsets.json"
	"${ROOT_DIR}/data/config/pipboy_screen_offsets.json"
)

# Add all weapons offset files
file(GLOB weapons_offset_files "${ROOT_DIR}/data/config/weapons_offsets/*.json")
list(APPEND resource_files ${weapons_offset_files})

source_group(
	"Resource Files"
	FILES ${resource_files}
)


# >>> Create VS project for DLL
add_library(
	${PROJECT_NAME}
	SHARED
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

# Set compile target to C++23
target_compile_features(
	${PROJECT_NAME}
	PRIVATE
	cxx_std_23
)
# Use _UNICODE macro to support CreateFileW
target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
	_UNICODE
)
# Include paths
target_include_directories(
	${PROJECT_NAME}
	PRIVATE
	${BUILD_DIR}/include
	${SOURCE_DIR}
	${CommonLibF4PathLocal}/CommonLibF4/include
	${ROOT_DIR}/external/openvr
)
# Libraries (.lib) to link
target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
	CommonLibF4::CommonLibF4
	${ROOT_DIR}/external/openvr/openvr_api.lib
)
# Add precompiled header (PCH.h)
target_precompile_headers(
	${PROJECT_NAME}
	PRIVATE
	src/PCH.h
)


# >>> Set project options
target_compile_options(
	${PROJECT_NAME}
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
	# /WX # Warning level (warnings are errors)

	# Potentials warnings to disable
	# /wd4702
	# /wd4267
	# /wd4456
	# /wd4189
	# /wd4244
	# /wd4834
	# /wd4189
	# /wd4505

	"$<$<CONFIG:DEBUG>:>"
	"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
)

target_link_options(
	${PROJECT_NAME}
	PRIVATE
	/WX # Treat Linker Warnings as Errors

	"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
	"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
)

# >>> fix a warning with /Ob2 being overridden with /Ob3
string(REPLACE "/Ob2" "/Ob3" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")


# >>> Post build command to copy plugins
if(POST_BUILD_COPY_PLUGIN)
	if(DEFINED COPY_PLUGIN_BASE_PATH)
		# Split paths by ";" into a list
		string(REPLACE ";" ";" COPY_PLUGIN_PATH_LIST "${COPY_PLUGIN_BASE_PATH}")

		message(">>> Create post build command: copy targets to:")
		foreach(copy_path IN LISTS COPY_PLUGIN_PATH_LIST)
			message("    ${copy_path}/F4SE/Plugins/")
			add_custom_command(
				TARGET ${PROJECT_NAME}
				POST_BUILD
				COMMENT "Copying ${PROJECT_NAME}.dll and .pdb to plugins folder: '${copy_path}/F4SE/Plugins/'"
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
  COMMAND ${CMAKE_COMMAND} -DPROJECT_NAME=${PROJECT_NAME}
						   -DPROJECT_FRIENDLY_NAME=${FRIENDLY_NAME}
						   -DPROJECT_VERSION=${PROJECT_VERSION}
                           -DROOT_DIR=${ROOT_DIR}
                           -DBUILD_DIR=${BUILD_DIR}
						   -DTARGET_FILE="$<TARGET_FILE:${PROJECT_NAME}>"
						   -DTARGET_PDB_FILE="$<TARGET_PDB_FILE:${PROJECT_NAME}>"
						   -DCONFIG=$(Configuration)
                           -P "${ROOT_DIR}/cmake/package.cmake"
)
