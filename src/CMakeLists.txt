# ---- Options ----
option(COPY_BUILD "Copy the build output to the Fallout directory." FALSE)
option(BUILD_FALLOUTVR "Build for Fallout VR" ON)


set_root_directory()
macro(find_commonlib_path)
	if(CommonLibName AND NOT ${CommonLibName} STREQUAL "")
		# Check extern
		find_path(CommonLibPath
			CommonLibF4/include/REL/Relocation.h
			PATHS ${ROOT_DIR}/external/${CommonLibName}
			${ROOT_DIR}/extern/${CommonLibName})

		if(${CommonLibPath} STREQUAL "CommonLibPath-NOTFOUND")
			# Check path
			set_from_environment(${CommonLibName}Path)
			set(CommonLibPath ${${CommonLibName}Path})
		endif()
	endif()
endmacro()

set_from_environment(VCPKG_ROOT)
list(APPEND CMAKE_MODULE_PATH "${ROOT_DIR}/cmake")

add_compile_definitions(FALLOUTVR)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
set(CommonLibName "CommonLibF4")
set_from_environment(FalloutVRPath)
set(FalloutPath ${FalloutVRPath})
set(FalloutVersion "Fallout VR")

find_commonlib_path()
message(
	STATUS
	"Building ${NAME} ${VERSION} for ${FalloutVersion} at ${FalloutPath} with ${CommonLibName} at ${CommonLibPath}."
)

if(DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
else()
	message(
		WARNING
		"Variable VCPKG_ROOT is not set. Continuing without vcpkg."
	)
endif()

set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")


configure_file(
	${ROOT_DIR}/cmake/Version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
	@ONLY
)

configure_file(
	${ROOT_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	@ONLY
)

# ---- Globals ----
add_compile_definitions(
	F4SE_SUPPORT_XBYAK
)

if(MSVC)
	if(NOT ${CMAKE_GENERATOR} STREQUAL "Ninja")
		add_compile_options(
			/MP # Build with Multiple Processes
		)
	endif()
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(Boost_USE_STATIC_LIBS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# ---- Dependencies ----
if(DEFINED CommonLibPath AND NOT ${CommonLibPath} STREQUAL "" AND IS_DIRECTORY ${CommonLibPath})
	add_subdirectory(${CommonLibPath} ${CommonLibName})
else()
	message(
		FATAL_ERROR
		"Variable ${CommonLibName}Path is not set."
	)
endif()

find_package(OpenVR REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")

# ---- Add source files ----
include(${ROOT_DIR}/cmake/headerlist.cmake)
include(${ROOT_DIR}/cmake/sourcelist.cmake)

source_group(
	TREE
	${CMAKE_CURRENT_SOURCE_DIR}
	FILES
	${headers}
	${sources}
)

source_group(
	TREE
	${CMAKE_CURRENT_BINARY_DIR}
	FILES
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
)

# ---- Create DLL ----
add_library(
	${PROJECT_NAME}
	SHARED
	${headers}
	${sources}
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	${ROOT_DIR}/.clang-format
)

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
	cxx_std_23
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
	_UNICODE
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
	${CommonLibName}::${CommonLibName}
	OpenVR::OpenVR
)

target_precompile_headers(
	${PROJECT_NAME}
	PRIVATE
	${ROOT_DIR}/src/PCH.h
)

if(MSVC)
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
		/WX # Warning level (warnings are errors)

		/wd4702
		/wd4267
		/wd4456
		/wd4189
		/wd4244
		/wd4834
		/wd4189
		/wd4505

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

	# fix a warning with /Ob2 being overridden with /Ob3
	string(REPLACE "/Ob2" "/Ob3" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# ---- Post build ----
if(COPY_BUILD)
	if(DEFINED FalloutPath)
		string(REGEX REPLACE "[/\\][Dd][Aa][Tt][Aa][/\\]?$" "" FalloutPath "${FalloutPath}") # remove " /data " if exists; cmake regex doesn't have /i
		add_custom_command(
			TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${FalloutPath}/Data/F4SE/Plugins/
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> ${FalloutPath}/Data/F4SE/Plugins/
		)
	else()
		message(
			WARNING
			"Variable ${FalloutPath} is not defined. Skipping post-build copy command."
		)
	endif()
endif()

# ---- Create Package ----
if(PACKAGE)
	set(ZIP_DIR "${CMAKE_CURRENT_BINARY_DIR}/zip")
	add_custom_target(build-time-make-directory ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ZIP_DIR}"
		"${ZIP_DIR}/F4SE/Plugins/"
	)

	message("Will copy mod into ${ZIP_DIR}.")
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${ZIP_DIR}/F4SE/Plugins/"
	)

	if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo OR ${CMAKE_BUILD_TYPE} STREQUAL Release)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${ZIP_DIR}/F4SE/Plugins/")
	endif()

	set(TARGET_ZIP "${PROJECT_NAME}_${PROJECT_VERSION}.7z")
	message("Will zip ${ZIP_DIR} to ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_ZIP}.")
	ADD_CUSTOM_COMMAND(
		TARGET ${PROJECT_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E tar cf ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_ZIP} --format=7zip -- .
		WORKING_DIRECTORY ${ZIP_DIR}
	)
endif()
