# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set(SOURCES
    cxplat_gtest.cpp
    cxplat_gtest.h
)

add_executable(cxplattest ${SOURCES})

target_include_directories(cxplattest PRIVATE ${PROJECT_SOURCE_DIR}/src/test ${PROJECT_SOURCE_DIR}/inc ${PROJECT_SOURCE_DIR}/submodules/googletest/googletest/include)

target_compile_definitions(cxplattest INTERFACE ${CXPLAT_COMMON_DEFINES})

set_property(TARGET cxplattest PROPERTY FOLDER "${CXPLAT_FOLDER_PREFIX}tests")
set_property(TARGET cxplattest APPEND PROPERTY BUILD_RPATH "$ORIGIN")

target_link_libraries(cxplattest cxplat testlib)
target_link_libraries(cxplattest gtest)


# At least /W3 must be used on all windows builds to pass compliance
if(MSVC)
    target_compile_options(cxplattest PRIVATE /W3 /bigobj)
endif()

add_test(NAME cxplattest
         COMMAND cxplattest
         WORKING_DIRECTORY ${CXPLAT_OUTPUT_DIR})
