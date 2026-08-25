include("${LV2_TOOLS}")

cmake_path(GET BUNDLE PARENT_PATH BUNDLE_PARENT)

set(LV2_PATH_ENV "LV2_PATH=${BUNDLE_PARENT}:${LV2_BUNDLES}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "${LV2_PATH_ENV}" ${LV2LS_TOOL}
    OUTPUT_VARIABLE URIS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE STATUS
)

if(NOT STATUS EQUAL 0)
    message(FATAL_ERROR "lv2ls failed on ${BUNDLE_PARENT}")
endif()

string(REPLACE "\n" ";" URIS "${URIS}")

if(NOT URIS)
    message(FATAL_ERROR "No plugin in ${BUNDLE_PARENT}, build the plugin first")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "${LV2_PATH_ENV}" ${LV2LINT_TOOL} -E warn -s lv2_generate_ttl
            -I "${BUNDLE}" ${URIS} RESULT_VARIABLE STATUS
)

if(NOT STATUS EQUAL 0)
    message(FATAL_ERROR "lv2lint rejected ${URIS}")
endif()
