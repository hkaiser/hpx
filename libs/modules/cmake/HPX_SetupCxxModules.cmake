# Copyright (c) 2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.29")
  hpx_warn(
    "CMake version not supported yet (CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API not set)"
  )
  return()
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28")
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API
      "bf70d4b0-9fb7-465c-9803-34014e70d112"
  )
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API
      "aa1f7df0-828a-4fcd-9afc-2dc80491aca7"
  )
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.26")
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API
      "2182bf5c-ef0d-489a-91da-49dbc3090d2a"
  )
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.25")
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API
      "3c375311-a3c9-4396-a187-3227ef642046"
  )
else()
  hpx_error("C++ modules are available only when using CMake V3.25 or newer")
endif()

hpx_info("\nUsing CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API: "
         ${CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API} "\n"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMake_TEST_CXXModules_UUID "a246741c-d067-4019-a8fb-3d16b0c9d1d3")

  set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
  string(
    CONCAT
      CMAKE_EXPERIMENTAL_CXX_SCANDEP_SOURCE
      "${CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS}"
      " -format=p1689 --p1689-targeted-file-name=<SOURCE> --p1689-targeted-output=<OBJECT> "
      " --p1689-makeformat-output=<DEP_FILE>"
      " --"
      " <DEFINES> <INCLUDES> <FLAGS> -x c++ <SOURCE>"
      " -MT <DYNDEP_FILE> -MD"
      " > <DYNDEP_FILE>"
  )
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "clang")
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG "@<MODULE_MAP_FILE>")

  # Default to C++ extensions being off. Clang's modules support have trouble
  # with extensions right now.
  set(CMAKE_CXX_EXTENSIONS OFF)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
  string(
    CONCAT
      CMAKE_EXPERIMENTAL_CXX_SCANDEP_SOURCE
      "<CMAKE_CXX_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -E -x c++ <SOURCE>"
      " -MT <DYNDEP_FILE> -MD -MF <DEP_FILE>"
      " -fmodules-ts -fdep-file=<DYNDEP_FILE> -fdep-output=<OBJECT> -fdep-format=trtbd"
      " -o <PREPROCESSED_SOURCE>"
  )
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FORMAT "gcc")
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_MAP_FLAG
      "-fmodules-ts -fmodule-mapper=<MODULE_MAP_FILE> -fdep-format=trtbd -x c++"
  )
elseif(MSVC)
  set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
else()
  hpx_error(
    "CMake does not support using the compiler (${CMAKE_CXX_COMPILER_ID}) for compiling C++20 modules"
  )
endif()
