include("${LV2_TOOLS}")

file(GLOB ONTOLOGIES "${LV2_BUNDLES}/*/*.ttl")
file(GLOB TURTLES "${BUNDLE}/*.ttl")

if(NOT TURTLES)
    message(FATAL_ERROR "No Turtle file in ${BUNDLE}, build the plugin first")
endif()

execute_process(COMMAND "${SORD_VALIDATE_TOOL}" ${ONTOLOGIES} ${TURTLES} RESULT_VARIABLE STATUS)

if(NOT STATUS EQUAL 0)
    message(FATAL_ERROR "sord_validate rejected the bundle metadata")
endif()
