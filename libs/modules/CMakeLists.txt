# Copyright (c) 2023 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(HPX_Message)
include(HPX_Option)

# enable C++ modules only if C++20 was enabled
hpx_option(
  HPX_WITH_CXX20_MODULES BOOL "Enable C++20 modules. (default: OFF)" OFF
  ADVANCED
)

if(NOT HPX_WITH_CXX20_MODULES)
  return()
endif()

if(CMAKE_CXX_STANDARD LESS 20)
  hpx_error(
    "C++ modules are available only if compiling with at least C++20 enabled"
  )
endif()

# enable C++ modules in CMake
include(HPX_SetupCxxModules)

if(HPX_WITH_TESTS_UNIT)
  add_hpx_pseudo_target(tests.unit.modules.cpp_modules)
  add_hpx_pseudo_dependencies(tests.unit.modules tests.unit.modules.cpp_modules)
endif()

set(hpx_MODULE_TARGETS)

foreach(lib "core")
  # add actual C++ module
  add_subdirectory(${lib})

  # expose this module to the installation
  set(hpx_MODULE_TARGETS ${hpx_MODULE_TARGETS} hpx_${lib}_module)
endforeach()

install(
  TARGETS ${hpx_MODULE_TARGETS}
  EXPORT modules
  FILE_SET hpx_module_files
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/miu"
  CXX_MODULES_BMI
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/bmi"
)

# Export HPXModuleTargets in the build directory
export(
  EXPORT modules
  NAMESPACE HPX::
  FILE "${PROJECT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXModuleTargets.cmake"
       CXX_MODULES_DIRECTORY "cxx-modules"
)

# Export HPXModuleTargets in the install directory
install(
  EXPORT modules
  NAMESPACE HPX::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}"
  FILE "HPXModuleTargets.cmake"
  CXX_MODULES_DIRECTORY "cxx-modules"
)
