if(NOT DEFINED FRIK_SOURCE OR NOT EXISTS "${FRIK_SOURCE}")
  message(FATAL_ERROR "FRIK Query verification source is unavailable: '${FRIK_SOURCE}'")
endif()

file(READ "${FRIK_SOURCE}" FRIK_SOURCE_TEXT)

string(FIND "${FRIK_SOURCE_TEXT}" "F4SEPlugin_Query" QUERY_START)
string(FIND "${FRIK_SOURCE_TEXT}" "F4SEPlugin_Load" LOAD_START)
if(QUERY_START EQUAL -1 OR LOAD_START EQUAL -1 OR LOAD_START LESS QUERY_START)
  message(FATAL_ERROR "Could not isolate F4SEPlugin_Query in '${FRIK_SOURCE}'")
endif()

math(EXPR QUERY_LENGTH "${LOAD_START} - ${QUERY_START}")
string(SUBSTRING "${FRIK_SOURCE_TEXT}" ${QUERY_START} ${QUERY_LENGTH} QUERY_SOURCE)

foreach(REQUIRED_TOKEN
    "REL::Module::IsVR()"
    "REL::Module::get().version()"
    "F4SE::RUNTIME_VR_1_2_72")
  string(FIND "${QUERY_SOURCE}" "${REQUIRED_TOKEN}" TOKEN_INDEX)
  if(TOKEN_INDEX EQUAL -1)
    message(FATAL_ERROR "F4SEPlugin_Query is missing required FO4VR guard '${REQUIRED_TOKEN}'")
  endif()
endforeach()

string(FIND "${QUERY_SOURCE}" "RuntimeVersion()" QUERY_RUNTIME_VERSION_INDEX)
if(NOT QUERY_RUNTIME_VERSION_INDEX EQUAL -1)
  message(FATAL_ERROR "F4SEPlugin_Query must not compare QueryInterface::RuntimeVersion() with an executable runtime constant")
endif()
