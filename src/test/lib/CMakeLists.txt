# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

set(SOURCES
    CryptTest.cpp
    EventTest.cpp
    LockTest.cpp
    MemoryTest.cpp
    ProcTest.cpp
    ThreadTest.cpp
    TimeTest.cpp
    VectorTest.cpp
)

add_library(testlib STATIC ${SOURCES})

target_include_directories(testlib PRIVATE ${PROJECT_SOURCE_DIR}/src/test)

target_link_libraries(testlib PRIVATE inc warnings)

set_property(TARGET testlib PROPERTY FOLDER "${CXPLAT_FOLDER_PREFIX}tests")
