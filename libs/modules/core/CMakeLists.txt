# Copyright (c) 2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(hpx_core_interface_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/include/level0.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/level1.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/level2.hpp
)

set(hpx_core_interface_modules
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hpx.core.ixx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hpx.core.config_registry.ixx
)

add_library(hpx_core_module)

# cmake-format: off
target_sources(
  hpx_core_module
  PRIVATE FILE_SET hpx_module_headers
    TYPE HEADERS
    BASE_DIRS include
    FILES ${hpx_core_interface_headers}
  PUBLIC FILE_SET hpx_module_files
    TYPE CXX_MODULES
    BASE_DIRS src
    FILES ${hpx_core_interface_modules}
)
# cmake-format: on

set_target_properties(hpx_core_module PROPERTIES FOLDER "Core/Modules")
target_link_libraries(hpx_core_module PUBLIC hpx_core)

if(MSVC)
  # '#include <meow>' in the purview of module 'hpx.core' appears erroneous.
  target_compile_options(hpx_core_module PRIVATE -wd5244)
endif()

if(HPX_WITH_TESTS_UNIT)
  add_hpx_pseudo_target(tests.unit.modules.cpp_modules.core)
  add_hpx_pseudo_dependencies(
    tests.unit.modules.cpp_modules tests.unit.modules.cpp_modules.core
  )
  add_subdirectory(tests)
endif()
