cmake_minimum_required(VERSION 3.20...3.31)

project(
    AnagramExample
    LANGUAGES C CXX
    VERSION 0.1.0
)

include(GNUInstallDirs)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(CREATE_LINK "${CMAKE_BINARY_DIR}/compile_commands.json"
     "${CMAKE_SOURCE_DIR}/compile_commands.json" SYMBOLIC
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "Build type" FORCE
    )
endif()

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

find_package(range-v3 REQUIRED)
find_package(DPF REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cc")

dpf_add_plugin(${PROJECT_NAME} TARGETS lv2 FILES_DSP ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC config)
target_include_directories(${PROJECT_NAME}-dsp PRIVATE include)
target_link_libraries(${PROJECT_NAME}-dsp PRIVATE range-v3::range-v3)

target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC DISTRHO_PLUGIN_LABEL="${PROJECT_NAME}"
           DISTRHO_PLUGIN_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
           DISTRHO_PLUGIN_VERSION_MINOR=${PROJECT_VERSION_MINOR}
           DISTRHO_PLUGIN_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_compile_options(${PROJECT_NAME} PUBLIC "-ffile-prefix-map=${DPF_ROOT_DIR}=dpf")
target_compile_options(${PROJECT_NAME}-dsp PRIVATE -Wall -Wextra -Wshadow)

target_link_options(
    ${PROJECT_NAME}-lv2 PRIVATE -static-libstdc++ -static-libgcc LINKER:--exclude-libs,ALL
    LINKER:--gc-sections
)

set(DEVICE_COMPILE_OPTIONS)

if(CMAKE_CROSSCOMPILING)
    find_package(modla REQUIRED)

    set(DEVICE_COMPILE_OPTIONS -march=armv8.2-a -mtune=cortex-a76 -ffast-math -fno-finite-math-only
                               -fomit-frame-pointer
    )

    target_compile_definitions(
        ${PROJECT_NAME} PUBLIC __MOD_DEVICES__ _MOD_DEVICE_RK358x _DARKGLASS_DEVICE_PABLITO
    )

    target_compile_options(${PROJECT_NAME} PUBLIC ${DEVICE_COMPILE_OPTIONS})
    target_link_libraries(${PROJECT_NAME} PUBLIC modla::modla)

    target_link_options(
        ${PROJECT_NAME}-lv2 PRIVATE -march=armv8.2-a -mtune=cortex-a76 -fno-fast-math
        LINKER:-O1,--as-needed,--strip-all
    )

    find_program(PATCHELF patchelf REQUIRED)

    add_custom_command(
        TARGET ${PROJECT_NAME}-lv2
        POST_BUILD
        COMMAND "${PATCHELF}" --remove-rpath "$<TARGET_FILE:${PROJECT_NAME}-lv2>"
        COMMENT "Removing the build host's RUNPATH from the device binary"
        VERBATIM
    )
endif()

get_target_property(LV2_BUNDLE ${PROJECT_NAME} LV2_BUNDLE)

file(GLOB_RECURSE ASSETS CONFIGURE_DEPENDS "asset/*")

foreach(ASSET IN LISTS ASSETS)
    cmake_path(
        RELATIVE_PATH ASSET BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/asset" OUTPUT_VARIABLE
        ASSET_PATH
    )
    configure_file("${ASSET}" "${LV2_BUNDLE}/${ASSET_PATH}" COPYONLY)
endforeach()

install(DIRECTORY "${LV2_BUNDLE}" DESTINATION "${CMAKE_INSTALL_LIBDIR}/lv2")

find_package(LV2Tools REQUIRED)

enable_testing()

add_executable(${PROJECT_NAME}-test tests/dsp_test.cc)

target_include_directories(${PROJECT_NAME}-test PRIVATE include)
target_link_libraries(${PROJECT_NAME}-test PRIVATE range-v3::range-v3)
target_compile_options(
    ${PROJECT_NAME}-test PRIVATE -Wall -Wextra -Wshadow ${DEVICE_COMPILE_OPTIONS}
)

add_test(NAME dsp COMMAND ${PROJECT_NAME}-test)

add_test(NAME lv2_validate
         COMMAND "${CMAKE_COMMAND}" -D "LV2_TOOLS=${LV2Tools_CONFIG}" -D "BUNDLE=${LV2_BUNDLE}" -P
                 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lv2-validate.cmake"
)

add_test(NAME lv2lint
         COMMAND "${CMAKE_COMMAND}" -D "LV2_TOOLS=${LV2Tools_CONFIG}" -D "BUNDLE=${LV2_BUNDLE}" -P
                 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lv2-lint.cmake"
)
