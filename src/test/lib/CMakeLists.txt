# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set(SOURCES
    BasicTest.cpp
)

add_library(testlib STATIC ${SOURCES})

target_include_directories(testlib PRIVATE ${PROJECT_SOURCE_DIR}/src/test ${PROJECT_SOURCE_DIR}/inc)

target_compile_definitions(testlib PRIVATE ${CXPLAT_COMMON_DEFINES})

set_property(TARGET testlib PROPERTY FOLDER "${CXPLAT_FOLDER_PREFIX}tests")
