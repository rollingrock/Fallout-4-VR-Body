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
file(MAKE_DIRECTORY "${PACKAGE_STAGE_PLUGINS_DIR}")
file(COPY "${ROOT_DIR}/data/mod/" DESTINATION "${PACKAGE_STAGE_DIR}")
file(COPY "${TARGET_FILE}" DESTINATION "${PACKAGE_STAGE_PLUGINS_DIR}")
file(COPY "${TARGET_PDB_FILE}" DESTINATION "${PACKAGE_STAGE_PLUGINS_DIR}")

execute_process(COMMAND ${CMAKE_COMMAND} -E tar cf "${TARGET_ZIP}" --format=7zip -- . WORKING_DIRECTORY "${PACKAGE_STAGE_DIR}")
