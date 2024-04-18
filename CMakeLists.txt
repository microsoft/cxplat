# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    cmake_minimum_required(VERSION 3.20)
else()
    cmake_minimum_required(VERSION 3.16)
endif()

# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
")
endif()

message(STATUS "CMAKE Version: ${CMAKE_VERSION}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

message(STATUS "Source Dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Host System name: ${CMAKE_HOST_SYSTEM_NAME}")
if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    if (NOT DEFINED CMAKE_SYSTEM_VERSION)
        set(CMAKE_SYSTEM_VERSION 10.0.22621.0 CACHE STRING INTERNAL FORCE)
    endif()
endif()

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
    message(STATUS "Setting policy 0091")
else()
    message(WARNING "CMake version too old to support Policy 0091; CRT static linking won't work")
endif()

if (POLICY CMP0111)
    cmake_policy(SET CMP0111 NEW)
endif()

project(cxplat)

# Set a default build type if none was specified
set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
        STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "System version: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Platform version: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CXPLAT_MAJOR_VERSION 0)
set(CXPLAT_FULL_VERSION 0.1.0)

if (WIN32)
    set(CX_PLATFORM "winuser")
elseif (APPLE)
    set(CX_PLATFORM "darwin")
elseif (UNIX)
    set(CX_PLATFORM "linux")
endif()
message(STATUS "Platform: ${CX_PLATFORM}")

set(FILENAME_DEP_REPLACE "get_filename_component(SELF_DIR \"$\{CMAKE_CURRENT_LIST_FILE\}\" PATH)")
set(SELF_DIR "$\{SELF_DIR\}")

option(CXPLAT_BUILD_TEST "Builds the test code" OFF)
option(CXPLAT_UWP_BUILD "Build for UWP" OFF)
option(CXPLAT_GAMECORE_BUILD "Build for GameCore" OFF)
option(CXPLAT_EMBED_GIT_HASH "Embed git commit hash in the binary" ON)
option(CXPLAT_PDBALTPATH "Enable PDBALTPATH setting on MSVC" ON)
option(CXPLAT_OPTIMIZE_LOCAL "Optimize code for local machine architecture" OFF)
option(CXPLAT_CI "CI Specific build" OFF)
option(CXPLAT_SKIP_CI_CHECKS "Disable CI specific build checks" OFF)
option(CXPLAT_OFFICIAL_RELEASE "Configured the build for an official release" OFF)
set(CXPLAT_FOLDER_PREFIX "" CACHE STRING "Optional prefix for source group folders when using an IDE generator")
set(CXPLAT_LIBRARY_NAME "cxplat" CACHE STRING "Override the output library name")

if (CXPLAT_GAMECORE_BUILD)
    if(${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} VERSION_LESS "10.0.20348.0")
        message(FATAL_ERROR "gamecore builds require Windows 10 SDK version 20348 or later.")
    endif()
endif()

if (CXPLAT_UWP_BUILD OR CXPLAT_GAMECORE_BUILD)
    message(STATUS "UWP And GameCore builds disable all executables")
    set(CXPLAT_BUILD_TEST OFF)
endif()

if (NOT CXPLAT_BUILD_SHARED)
    cmake_minimum_required(VERSION 3.20)
endif()

if (CXPLAT_PDBALTPATH AND MSVC)
#    Disabled in all cases because generation is broken.
#    file(READ ${CMAKE_CURRENT_LIST_DIR}/cmake/PdbAltPath.txt PDBALTPATH)
#    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PDBALTPATH}")
#    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${PDBALTPATH}")
#    message(STATUS ${CMAKE_EXE_LINKER_FLAGS})
endif()

include(${PROJECT_SOURCE_DIR}/cmake/GitCommands.cmake)
get_git_current_hash(${PROJECT_SOURCE_DIR} GIT_CURRENT_HASH)

if(NOT GIT_CURRENT_HASH)
    message("Failed to get git hash. Binary will not contain git hash")
    set(CXPLAT_EMBED_GIT_HASH OFF)
endif()

set(CXPLAT_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CXPLAT_OUTPUT_DIR ${CXPLAT_BUILD_DIR}/bin/$<IF:$<CONFIG:Debug>,Debug,Release> CACHE STRING "Output directory for build artifacts")

set(CXPLAT_VER_BUILD_ID "0" CACHE STRING "The version build ID")
set(CXPLAT_VER_SUFFIX "-private" CACHE STRING "The version suffix")

message(STATUS "Version Build ID: ${CXPLAT_VER_BUILD_ID}")
message(STATUS "Version Suffix: ${CXPLAT_VER_SUFFIX}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CXPLAT_BUILD_DIR}/obj/$<IF:$<CONFIG:Debug>,Debug,Release>)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CXPLAT_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CXPLAT_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CXPLAT_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CXPLAT_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CXPLAT_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CXPLAT_OUTPUT_DIR})

if (WIN32)
    set(CXPLAT_WARNING_FLAGS /WX /W4 /sdl /wd4206 CACHE INTERNAL "")
    set(CXPLAT_COMMON_FLAGS "")

    include(CheckCCompilerFlag)

    if(NOT CXPLAT_ENABLE_SANITIZERS)
        check_c_compiler_flag(/Qspectre HAS_SPECTRE)
    endif()
    if(HAS_SPECTRE)
        list(APPEND CXPLAT_COMMON_FLAGS /Qspectre)
    endif()

    check_c_compiler_flag(/guard:cf HAS_GUARDCF)
    if(HAS_GUARDCF)
        list(APPEND CXPLAT_COMMON_FLAGS /guard:cf)
    endif()

    # Require /Qspectre and /guard:cf in CI builds
    if(CXPLAT_CI AND NOT CXPLAT_SKIP_CI_CHECKS)
        if(NOT HAS_GUARDCF)
            message(FATAL_ERROR "/guard:cf must exist for CI builds")
        endif()
        if(NOT HAS_SPECTRE AND NOT CXPLAT_ENABLE_SANITIZERS AND NOT CXPLAT_UWP_BUILD)
            message(FATAL_ERROR "/Qspectre must exist for CI builds")
        endif()
    endif()

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        list(APPEND CXPLAT_COMMON_FLAGS /MP)
    endif()
    set(CXPLAT_COMMON_DEFINES WIN32_LEAN_AND_MEAN SECURITY_WIN32)
else()

    include(CheckSymbolExists)
    include(CheckFunctionExists)
    include(CheckIncludeFile)

    if (CX_PLATFORM STREQUAL "linux")
        include(CheckCCompilerFlag)
    endif()

    set(CXPLAT_COMMON_FLAGS "")
    set(CXPLAT_COMMON_DEFINES _GNU_SOURCE)

    set(CXPLAT_WARNING_FLAGS -Werror -Wall -Wextra -Wformat=2 -Wno-type-limits
        -Wno-unknown-pragmas -Wno-multichar -Wno-missing-field-initializers
        CACHE INTERNAL "")
    if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        list(APPEND CXPLAT_WARNING_FLAGS -Wno-strict-aliasing)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND CXPLAT_WARNING_FLAGS -Wno-missing-braces -Wno-microsoft-anon-tag)
    endif()
endif()

list(APPEND CXPLAT_COMMON_DEFINES VER_BUILD_ID=${CXPLAT_VER_BUILD_ID})
list(APPEND CXPLAT_COMMON_DEFINES VER_SUFFIX=${CXPLAT_VER_SUFFIX})

if (CXPLAT_EMBED_GIT_HASH)
    list(APPEND CXPLAT_COMMON_DEFINES VER_GIT_HASH=${GIT_CURRENT_HASH})
endif()

if(CXPLAT_OFFICIAL_RELEASE)
    list(APPEND CXPLAT_COMMON_DEFINES CXPLAT_OFFICIAL_RELEASE=1)
    message(STATUS "Configured for official release build")
endif()

if (NOT MSVC AND NOT APPLE AND NOT ANDROID)
    find_library(ATOMIC NAMES atomic libatomic.so.1)
    if (ATOMIC)
        message(STATUS "Found libatomic: ${ATOMIC}")
    else()
        message(STATUS "libatomic not found. If build fails, install libatomic")
    endif()

    find_library(NUMA NAMES NUMA libnuma.so.1)
    if (NUMA)
        message(STATUS "Found libnuma: ${NUMA}")
        find_file(NUMA-HEADER NAMES "numa.h")
        if (NUMA-HEADER)
            message(STATUS "Found numa.h: ${NUMA-HEADER}")
            list(APPEND CXPLAT_COMMON_DEFINES CXPLAT_NUMA_AWARE)
        else()
            message(STATUS "numa.h not found. If build fails, install libnuma-dev")
        endif()
    else()
        message(STATUS "libnuma not found. If build fails, install libnuma")
    endif()
endif()

if(WIN32)
    if (CMAKE_GENERATOR_PLATFORM STREQUAL "")
        string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} SYSTEM_PROCESSOR)
    else()
        string(TOLOWER ${CMAKE_GENERATOR_PLATFORM} SYSTEM_PROCESSOR)
    endif()

    if (CXPLAT_UWP_BUILD)
        list(APPEND CXPLAT_COMMON_DEFINES CXPLAT_UWP_BUILD CXPLAT_RESTRICTED_BUILD)
        set(CMAKE_CXX_STANDARD_LIBRARIES "")
        set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "")
        set(CMAKE_C_STANDARD_LIBRARIES "")
        set(CMAKE_C_STANDARD_LIBRARIES_INIT "")
    endif()

    if (CXPLAT_GAMECORE_BUILD)
        list(APPEND CXPLAT_COMMON_DEFINES WINAPI_FAMILY=WINAPI_FAMILY_GAMES CXPLAT_GAMECORE_BUILD CXPLAT_RESTRICTED_BUILD)
        set(CMAKE_CXX_STANDARD_LIBRARIES "")
        set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "")
        set(CMAKE_C_STANDARD_LIBRARIES "")
        set(CMAKE_C_STANDARD_LIBRARIES_INIT "")

        set(UnsupportedLibs advapi32.lib comctl32.lib comsupp.lib dbghelp.lib gdi32.lib gdiplus.lib guardcfw.lib kernel32.lib mmc.lib msimg32.lib msvcole.lib msvcoled.lib mswsock.lib ntstrsafe.lib ole2.lib ole2autd.lib ole2auto.lib ole2d.lib ole2ui.lib ole2uid.lib ole32.lib oleacc.lib oleaut32.lib oledlg.lib oledlgd.lib oldnames.lib runtimeobject.lib shell32.lib shlwapi.lib strsafe.lib urlmon.lib user32.lib userenv.lib wlmole.lib wlmoled.lib onecore.lib)
        set(Console_LinkOptions /DYNAMICBASE /NXCOMPAT)
        foreach(arg ${UnsupportedLibs})
            list(APPEND Console_LinkOptions "/NODEFAULTLIB:${arg}")
        endforeach()

        set(Console_ArchOptions /favor:AMD64)

        list(APPEND Console_ArchOptions /arch:AVX)

        # Locate Software Development Kits
        get_filename_component(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\GDK;InstallPath]" ABSOLUTE CACHE)

        if(NOT EXISTS ${Console_SdkRoot})
            if (EXISTS "C:\\Program Files (x86)\\Microsoft GDK")
                set(Console_SdkRoot "C:\\Program Files (x86)\\Microsoft GDK")
            else()
                message(FATAL_ERROR "Could not find GDK install")
            endif()
        endif()

        file(GLOB GdkFolders ${Console_SdkRoot}/*)

        set(XdkEditionTarget 5000000000)

        foreach(GdkFolder ${GdkFolders})
            if (IS_DIRECTORY ${GdkFolder})
                file(GLOB SubDirs ${GdkFolder}/*)
                foreach(SubDir ${SubDirs})
                    if (SubDir MATCHES "GXDK$")
                        # Make sure library exists
                        set(GxdkLibDirectory "${SubDir}/gameKit/Lib/amd64")
                        set(GxdkRuntimeLib "${GxdkLibDirectory}/xgameplatform.lib")
                        if (EXISTS ${GxdkRuntimeLib})
                            get_filename_component(PotentialXdkEditionTarget ${GdkFolder} NAME)
                            # Always select lowest version equal or higher than 211000
                            if (PotentialXdkEditionTarget LESS XdkEditionTarget AND
                                PotentialXdkEditionTarget GREATER_EQUAL 211000)
                                set(XdkEditionTarget ${PotentialXdkEditionTarget})
                                set(Console_EndpointLibRoot "${GxdkLibDirectory}")
                            endif()
                        endif()
                    endif()
                endforeach()
            endif()
        endforeach()

        if (XdkEditionTarget EQUAL 5000000000)
            message(FATAL_ERROR "Gxdk Target Not Found")
        endif()

        message(STATUS "Chosing ${XdkEditionTarget} with root ${Console_EndpointLibRoot}")

    endif()

    list(APPEND CXPLAT_COMMON_DEFINES CX_PLATFORM_WINUSER)

    set(CXPLAT_C_FLAGS ${CXPLAT_COMMON_FLAGS})
    set(CXPLAT_CXX_FLAGS ${CXPLAT_COMMON_FLAGS} /EHsc /permissive-)

    # These cannot be updated until CMake 3.13
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /GL /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /IGNORE:4075 /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /IGNORE:4075 /DEBUG /OPT:REF /OPT:ICF")

else() #!WIN32
    # Custom build flags.

    if (CXPLAT_OPTIMIZE_LOCAL AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL arm)
        set(MARCH -march=native)
    endif()

    set(CMAKE_C_FLAGS_DEBUG "-Og -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -fno-omit-frame-pointer ${MARCH} -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 ${MARCH} -DNDEBUG")
    if (CX_PLATFORM STREQUAL "darwin")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -gdwarf")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -gdwarf")
    else()
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb3")
        set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -ggdb3")
    endif()
    set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
    set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

    list(APPEND CXPLAT_COMMON_FLAGS -fms-extensions -fPIC)
    if (CX_PLATFORM STREQUAL "darwin")
        list(APPEND CXPLAT_COMMON_DEFINES CX_PLATFORM_DARWIN)
        list(APPEND CXPLAT_COMMON_FLAGS -Wno-microsoft-anon-tag -Wno-tautological-constant-out-of-range-compare -Wmissing-field-initializers)
    else()
        list(APPEND CXPLAT_COMMON_DEFINES CX_PLATFORM_LINUX)
    endif()

    set(CXPLAT_C_FLAGS ${CXPLAT_COMMON_FLAGS})
    set(CXPLAT_CXX_FLAGS ${CXPLAT_COMMON_FLAGS})
endif()

# Product code
add_subdirectory(src/lib/)

# Test code
if(CXPLAT_BUILD_TEST)
    include(FetchContent)

    enable_testing()

    # Build the googletest framework.

    # Enforce static builds for test artifacts
    set(PREV_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} CACHE INTERNAL "")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject")
    set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)")
    FetchContent_Declare(
        googletest
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/googletest
    )
    FetchContent_MakeAvailable(googletest)
    set(BUILD_SHARED_LIBS ${PREV_BUILD_SHARED_LIBS} CACHE INTERNAL "")

    set_property(TARGET gtest PROPERTY CXX_STANDARD 17)
    set_property(TARGET gtest PROPERTY FOLDER "${CXPLAT_FOLDER_PREFIX}tests")

    set_property(TARGET gtest_main PROPERTY CXX_STANDARD 17)
    set_property(TARGET gtest_main PROPERTY FOLDER "${CXPLAT_FOLDER_PREFIX}tests")
    set_property(TARGET gtest_main PROPERTY EXCLUDE_FROM_ALL ON)
    set_property(TARGET gtest_main PROPERTY EXCLUDE_FROM_DEFAULT_BUILD ON)

    if (HAS_SPECTRE)
        target_compile_options(gtest PRIVATE /Qspectre)
        target_compile_options(gtest_main PRIVATE /Qspectre)
    endif()

    if (HAS_GUARDCF)
        target_compile_options(gtest PRIVATE /guard:cf)
        target_compile_options(gtest_main PRIVATE /guard:cf)
    endif()

    add_subdirectory(src/test/lib)
    add_subdirectory(src/test/bin)
endif()
