#================================================================================#
# LICENSE (MIT)                                                                  #
#================================================================================#
#                                                                                #
# The MIT License (MIT)                                                          #
#                                                                                #
# Copyright (c) 2025 - 2026 luau-project https://github.com/luau-project/lua-pcg #
#                                                                                #
# Permission is hereby granted, free of charge, to any person obtaining a copy   #
# of this software and associated documentation files (the "Software"), to deal  #
# in the Software without restriction, including without limitation the rights   #
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      #
# copies of the Software, and to permit persons to whom the Software is          #
# furnished to do so, subject to the following conditions:                       #
#                                                                                #
# The above copyright notice and this permission notice shall be included in all #
# copies or substantial portions of the Software.                                #
#                                                                                #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    #
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  #
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  #
# SOFTWARE.                                                                      #
#                                                                                #
#================================================================================#

#===============================#
# Main arguments:               #
#===============================#
#                               #
# LUA_VERSION (STRING)          #
#   Description:                #
#     The version of Lua.       #
#   Examples:                   #
#     * luajit                  #
#     * 5.1                     #
#     * 5.1.5                   #
#     * 5.2                     #
#     * 5.2.4                   #
#     * 5.3                     #
#     * 5.3.6                   #
#     * 5.4                     #
#     * 5.4.9                   #
#     * 5.5                     #
#     * 5.5.1                   #
#   Default value: (UNDEFINED)  #
#                               #
# LUA_PCG_ENABLE_TESTS (ON/OFF) #
#   Description:                #
#     Run tests.                #
#   Default value: ON           #
#                               #
# INSTALL_CMOD (PATH)           #
#   Description:                #
#     Relative path where Lua   #
#     finds modules written     #
#     in C.                     #
#   Examples:                   #
#     * lib/lua/5.4             #
#     * lib/x86_64-linux-gnu    #
#     * bin                     #
#   Default value: (UNDEFINED)  #
#                               #
#===============================#

CMAKE_MINIMUM_REQUIRED(VERSION 3.14)

PROJECT(lua-pcg
    VERSION 0.1.0
    LANGUAGES C
)

SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type.")
OPTION(LUA_PCG_ENABLE_TESTS "Enable testing `lua-pcg'" ON)

IF (LUA_PCG_ENABLE_TESTS)
    ENABLE_TESTING()
ENDIF()

SET(_TESTS_DIR "${PROJECT_SOURCE_DIR}/tests")
SET(_TESTS_TEST32 "${_TESTS_DIR}/test32.lua")
SET(_TESTS_TEST64 "${_TESTS_DIR}/test64.lua")

SET(_LUA_PCG_TEST_FILES
    ${_TESTS_TEST32}
    ${_TESTS_TEST64}
)

IF (APPLE)
    ADD_LIBRARY(lua-pcg MODULE)
ELSE()
    ADD_LIBRARY(lua-pcg SHARED)
ENDIF()

SET(_SRC_DIR "${PROJECT_SOURCE_DIR}/src")

TARGET_SOURCES(lua-pcg
    PRIVATE
    "${_SRC_DIR}/lua-pcg.c"
    "${_SRC_DIR}/lua-pcg.h"
)
TARGET_INCLUDE_DIRECTORIES(lua-pcg PRIVATE "${_SRC_DIR}")

SET_TARGET_PROPERTIES(lua-pcg
    PROPERTIES
    PREFIX ""
)

TARGET_COMPILE_DEFINITIONS(lua-pcg PRIVATE "LUA_PCG_BUILD_SHARED")

IF (WIN32)
    TARGET_SOURCES(lua-pcg PRIVATE "${PROJECT_SOURCE_DIR}/lua-pcg.def")
ELSEIF (APPLE)
    TARGET_LINK_OPTIONS(lua-pcg
        PRIVATE
        "LINKER:-bundle"
        "LINKER:-undefined,dynamic_lookup"
        "LINKER:-all_load"
    )
    SET_TARGET_PROPERTIES(lua-pcg
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        SUFFIX ".so"
    )
ELSE()
    SET_TARGET_PROPERTIES(lua-pcg
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
ENDIF()

# Macro used in multiple locations
# to setup lua-pcg for LuaJIT
MACRO(_SETUP_LUA_PCG_FOR_LUAJIT)
    IF (NOT DEFINED FIND_LUAJIT)
        INCLUDE("${PROJECT_SOURCE_DIR}/cmake/FindLuaJIT.cmake")
    ENDIF()
    FIND_LUAJIT(ERROR_VARIABLE ERROR_MSG)

    IF (NOT LUAJIT_FOUND)
        MESSAGE(FATAL_ERROR ${ERROR_MSG})
    ENDIF()

    SET(LUA_INCLUDE_DIR "${LUAJIT_INCLUDE_DIR}")
    SET(LUA_LIBRARIES "${LUAJIT_LIBRARIES}")
    SET(LUA_INTERPRETER "${LUAJIT_INTERPRETER}")

    TARGET_INCLUDE_DIRECTORIES(lua-pcg PRIVATE ${LUA_INCLUDE_DIR})
    IF (WIN32)
        TARGET_LINK_LIBRARIES(lua-pcg PRIVATE ${LUA_LIBRARIES})
    ENDIF()
ENDMACRO()

IF ("${LUA_VERSION}" MATCHES "^luajit")
    _SETUP_LUA_PCG_FOR_LUAJIT()
ELSE()
    IF (DEFINED LUA_VERSION)
        FIND_PACKAGE(Lua "${LUA_VERSION}" EXACT QUIET)
    ELSE()
        FIND_PACKAGE(Lua QUIET)
    ENDIF()

    IF (LUA_FOUND)
        MESSAGE(STATUS "Found Lua headers at `${LUA_INCLUDE_DIR}'")
        TARGET_INCLUDE_DIRECTORIES(lua-pcg PRIVATE ${LUA_INCLUDE_DIR})
        IF (WIN32)
            MESSAGE(STATUS "Found the Lua libraries at `${LUA_LIBRARIES}'")
            TARGET_LINK_LIBRARIES(lua-pcg PRIVATE ${LUA_LIBRARIES})
        ENDIF()

        IF (LUA_PCG_ENABLE_TESTS)
            FIND_PROGRAM(LUA_INTERPRETER
                NAMES
                    "lua${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}"
                    "lua${LUA_VERSION_MAJOR}${LUA_VERSION_MINOR}"
                    "lua-${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}"
                    "lua-${LUA_VERSION_MAJOR}${LUA_VERSION_MINOR}"
                    "lua"
                HINTS ENV LUA_DIR
                PATH_SUFFIXES "bin"
                OPTIONAL
            )

            IF (NOT LUA_INTERPRETER)
                MESSAGE(FATAL_ERROR "Failed to find Lua ${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
            ENDIF()

            EXECUTE_PROCESS(
                COMMAND ${LUA_INTERPRETER} "-v"
                OUTPUT_VARIABLE LUA_VERSION_CONTENT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            IF (NOT ("${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}" STREQUAL "5.1") AND
                NOT LUA_VERSION_CONTENT MATCHES "^Lua .* PUC-Rio")
                MESSAGE(FATAL_ERROR "Failed to find a working Lua interpreter")
            ENDIF()

            EXECUTE_PROCESS(
                COMMAND ${LUA_INTERPRETER} "-e" "print(_VERSION:sub(5))"
                OUTPUT_VARIABLE LUA_ABI_CONTENT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            IF (NOT LUA_ABI_CONTENT STREQUAL "${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
                MESSAGE(FATAL_ERROR "Failed to find a working Lua interpreter")
            ENDIF()
        ENDIF()
    ELSE()
        INCLUDE("${PROJECT_SOURCE_DIR}/cmake/FindPucLuaUsingPkgConfig.cmake")

        IF (DEFINED LUA_VERSION)
            FIND_PUC_LUA_USING_PKG_CONFIG("${LUA_VERSION}" ERROR_VARIABLE ERROR_MSG)
        ELSE()
            FOREACH(_LUA_V "5.5" "5.4" "5.3" "5.2" "5.1")
                FIND_PUC_LUA_USING_PKG_CONFIG("${_LUA_V}" ERROR_VARIABLE ERROR_MSG)

                IF (LUA_FOUND)
                    BREAK()
                ENDIF()
            ENDFOREACH()
        ENDIF()

        IF (LUA_FOUND)
            MESSAGE(STATUS "Found Lua headers at `${LUA_INCLUDE_DIR}'")
            TARGET_INCLUDE_DIRECTORIES(lua-pcg PRIVATE ${LUA_INCLUDE_DIR})
            IF (WIN32)
                MESSAGE(STATUS "Found the Lua libraries at `${LUA_LIBRARIES}'")
                TARGET_LINK_LIBRARIES(lua-pcg PRIVATE ${LUA_LIBRARIES})
            ENDIF()
        ELSE()
            _SETUP_LUA_PCG_FOR_LUAJIT()
        ENDIF()
    ENDIF()
ENDIF()

SET(_LUA_ABI "${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")

SET(_INSTALL_KIND)
SET(_INSTALL_CMOD)

IF (INSTALL_CMOD)
    IF (IS_ABSOLUTE "${INSTALL_CMOD}")
        MESSAGE(FATAL_ERROR "`INSTALL_CMOD' must be a relative path")
    ENDIF()

    SET(_INSTALL_CMOD "${INSTALL_CMOD}")

    IF (WIN32)
        SET(_INSTALL_KIND "RUNTIME")
    ELSE()
        SET(_INSTALL_KIND "LIBRARY")
    ENDIF()
ELSE()

    INCLUDE(GNUInstallDirs)

    IF (MSYS OR CYGWIN)
        SET(_INSTALL_KIND "RUNTIME")
        SET(_INSTALL_CMOD "${CMAKE_INSTALL_LIBDIR}/lua/${_LUA_ABI}")
    ELSEIF (WIN32)
        SET(_INSTALL_KIND "RUNTIME")
        IF ("${_LUA_ABI}" STREQUAL "5.1" OR "${_LUA_ABI}" STREQUAL "5.2")
            SET(_INSTALL_CMOD "${CMAKE_INSTALL_BINDIR}")
        ELSE()
            SET(_INSTALL_CMOD "${CMAKE_INSTALL_LIBDIR}/lua/${_LUA_ABI}")
        ENDIF()
    ELSE()
        SET(_INSTALL_KIND "LIBRARY")
        SET(_INSTALL_CMOD "${CMAKE_INSTALL_LIBDIR}/lua/${_LUA_ABI}")
    ENDIF()
ENDIF()

INSTALL(TARGETS lua-pcg
    "${_INSTALL_KIND}" DESTINATION "${_INSTALL_CMOD}"
)

IF (LUA_PCG_ENABLE_TESTS)
    FOREACH (_TEST_FILE ${_LUA_PCG_TEST_FILES})
        GET_FILENAME_COMPONENT(_BASENAME ${_TEST_FILE} NAME)
        ADD_TEST(NAME ${_BASENAME}
            COMMAND ${LUA_INTERPRETER} ${_TEST_FILE}
        )
        IF (WIN32)
            SET_TESTS_PROPERTIES(${_BASENAME}
                PROPERTIES
                ENVIRONMENT "LUA_CPATH=${CMAKE_CURRENT_BINARY_DIR}\\?.dll\;\;"
            )
        ELSE()
            SET_TESTS_PROPERTIES(${_BASENAME}
                PROPERTIES
                ENVIRONMENT "LUA_CPATH=${CMAKE_CURRENT_BINARY_DIR}/?.so\;\;"
            )
        ENDIF()
    ENDFOREACH()
ENDIF()

MESSAGE(STATUS " ")
MESSAGE(STATUS " Configuration summary:")
IF (LUAJIT_FOUND)
MESSAGE(STATUS "   LuaJIT")
ELSE()
MESSAGE(STATUS "   Lua")
ENDIF()
MESSAGE(STATUS "     Version: ........... ${_LUA_ABI}")
MESSAGE(STATUS "     Include dir: ....... ${LUA_INCLUDE_DIR}")
IF (WIN32)
MESSAGE(STATUS "     Libraries: ......... ${LUA_LIBRARIES}")
ENDIF()
IF (LUA_PCG_ENABLE_TESTS)
MESSAGE(STATUS "     Interpreter: ....... ${LUA_INTERPRETER}")
ENDIF()
MESSAGE(STATUS " ")
MESSAGE(STATUS " Install")
MESSAGE(STATUS "   Prefix: .............. ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "   lua-pcg dir: ......... ${CMAKE_INSTALL_PREFIX}/${_INSTALL_CMOD}")
MESSAGE(STATUS " ")
