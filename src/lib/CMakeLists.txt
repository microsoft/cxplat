# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if("${CX_PLATFORM}" STREQUAL "winuser")
    set(SOURCES ${SOURCES} cxplat_winuser.c)
else()
    set(SOURCES ${SOURCES} cxplat_posix.c)
endif()

add_library(cxplat STATIC ${SOURCES})
