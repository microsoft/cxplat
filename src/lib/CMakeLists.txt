# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if("${CX_PLATFORM}" STREQUAL "winuser")
    set(SOURCES ${SOURCES} cxplat_winuser.c)
else()
    set(SOURCES ${SOURCES} cxplat_posix.c inline.c)
endif()

add_library(cxplat STATIC ${SOURCES})

target_link_libraries(cxplat PUBLIC inc)

target_link_libraries(cxplat PRIVATE warnings main_binary_link_args)

set_property(TARGET cxplat PROPERTY FOLDER "${CXPLAT_FOLDER_PREFIX}libraries")

target_include_directories(cxplat PRIVATE ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/src/inc)

target_compile_definitions(cxplat INTERFACE ${CXPLAT_COMMON_DEFINES})
