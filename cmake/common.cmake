option(COPY_OUTPUT "copy the output of build operations to the game directory" OFF)

macro(set_from_environment VARIABLE)
	if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif()
endmacro()

function(set_root_directory)
	set(ROOT_DIR "${PROJECT_SOURCE_DIR}")
	cmake_path(NORMAL_PATH ROOT_DIR)
	if("${ROOT_DIR}" MATCHES "[\\/]$")
		string(LENGTH "${ROOT_DIR}" ROOT_DIR_LENGTH)
		math(EXPR ROOT_DIR_LENGTH "${ROOT_DIR_LENGTH} - 1")
		string(SUBSTRING "${ROOT_DIR}" 0 "${ROOT_DIR_LENGTH}" ROOT_DIR)
	endif()
	set(ROOT_DIR "${ROOT_DIR}" PARENT_SCOPE)
endfunction()

macro(handle_data_files)
	set(_PREFIX handle_data_files)

	set(_OPTIONS)
	set(_ONE_VALUE_ARGS
		DESTINATION
	)
	set(_MULTI_VALUE_ARGS
		FILES
	)

	set(_REQUIRED
		FILES
	)

	cmake_parse_arguments(
		"${_PREFIX}"
		"${_OPTIONS}"
		"${_ONE_VALUE_ARGS}"
		"${_MULTI_VALUE_ARGS}"
		"${ARGN}"
	)

	foreach(_ARG ${_REQUIRED})
		if(NOT DEFINED "${_PREFIX}_${_ARG}")
			message(FATAL_ERROR "Argument is required to be defined: ${_ARG}")
		endif()
	endforeach()

	if(DEFINED ${_PREFIX}_DESTINATION)
		set(${_PREFIX}_INSTALL_DESTINATION "${${_PREFIX}_DESTINATION}")
		set(${_PREFIX}_COPY_DESTINATION "${${_PREFIX}_DESTINATION}")
	else()
		set(${_PREFIX}_INSTALL_DESTINATION "/")
		set(${_PREFIX}_COPY_DESTINATION "")
	endif()

	install(
		FILES ${${_PREFIX}_FILES}
		DESTINATION "${${_PREFIX}_INSTALL_DESTINATION}"
		COMPONENT "main"
	)

	if("${COPY_OUTPUT}")
		foreach(_FILE ${${_PREFIX}_FILES})
			add_custom_command(
				TARGET "${PROJECT_NAME}"
				POST_BUILD
				COMMAND
					"${CMAKE_COMMAND}"
					-E
					copy_if_different
					"${_FILE}"
					"${Fallout4Path}/Data/${${_PREFIX}_COPY_DESTINATION}"
				VERBATIM
			)
		endforeach()
	endif()
endmacro()
