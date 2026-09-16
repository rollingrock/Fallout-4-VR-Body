# Post-build: copy the plugin DLL and PDB into a mod folder's F4SE/Plugins/.
# Release only. A Debug DLL on a test rig is slow, behaves differently and invalidates any
# timing taken on it; build a Debug config to debug locally, it is never deployed.
string(TOLOWER "${CONFIG}" CONFIG_LOWER)
if(NOT CONFIG_LOWER STREQUAL "release")
  message("Skipping post-build plugin copy: not a release build: '${CONFIG}'")
  return()
endif()

set(DEST_DIR "${COPY_PATH}/F4SE/Plugins")
file(MAKE_DIRECTORY "${DEST_DIR}")
file(COPY "${TARGET_FILE}" "${TARGET_PDB_FILE}" DESTINATION "${DEST_DIR}")
message("Copied ${TARGET_FILE} and its .pdb to '${DEST_DIR}/'")
