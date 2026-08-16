# Post-Build event script to package the mod into a .7z file ONLY for release builds

string(TOLOWER "${CONFIG}" CONFIG_LOWER)
if(NOT CONFIG_LOWER STREQUAL "release")
  message("Skipping post-build packaging: not a release build: '${CONFIG}'")
  return()
endif()

set(PACKAGE_DIR "${BUILD_DIR}/package")
set(PACKAGE_STAGE_DIR "${PACKAGE_DIR}/stageing")
set(PACKAGE_STAGE_PLUGINS_DIR "${PACKAGE_STAGE_DIR}/F4SE/Plugins")
string(TIMESTAMP PACKAGE_DATE "%Y%m%d")
set(TARGET_ZIP "${PACKAGE_DIR}/${PROJECT_FRIENDLY_NAME} - v${PROJECT_VERSION} - ${PACKAGE_DATE}.7z")

message("Packaging release build into '${TARGET_ZIP}'")

# Stage from scratch every time. Copying only ever adds files, so a mesh or texture that was renamed
# or deleted since the last release build would keep shipping out of the previous build's leftovers.
file(REMOVE_RECURSE "${PACKAGE_STAGE_DIR}")
file(MAKE_DIRECTORY "${PACKAGE_STAGE_PLUGINS_DIR}")
file(COPY "${ROOT_DIR}/data/mod/" DESTINATION "${PACKAGE_STAGE_DIR}")
file(COPY "${TARGET_FILE}" DESTINATION "${PACKAGE_STAGE_PLUGINS_DIR}")
file(COPY "${TARGET_PDB_FILE}" DESTINATION "${PACKAGE_STAGE_PLUGINS_DIR}")

# Drop any archive from an earlier build of the same version+date so a failed pack can't leave the
# previous .7z sitting there looking like the new one, then fail the build if packing didn't work.
file(REMOVE "${TARGET_ZIP}")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar cf "${TARGET_ZIP}" --format=7zip -- .
  WORKING_DIRECTORY "${PACKAGE_STAGE_DIR}"
  RESULT_VARIABLE PACKAGE_RESULT
)
if(NOT PACKAGE_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to package release into '${TARGET_ZIP}' (exit code: ${PACKAGE_RESULT})")
endif()
