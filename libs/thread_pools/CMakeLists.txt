# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(thread_pools_headers
    hpx/thread_pools/detail/scoped_background_timer.hpp
    hpx/thread_pools/scheduled_thread_pool.hpp
    hpx/thread_pools/scheduled_thread_pool_impl.hpp
    hpx/thread_pools/scheduling_loop.hpp
)

set(thread_pools_compat_headers hpx/runtime/threads/thread_pools.hpp
                                hpx/runtime/threads/scoped_background_timer.hpp
)

set(thread_pools_sources scheduled_thread_pool.cpp)

include(HPX_AddModule)
add_hpx_module(
  thread_pools
  COMPATIBILITY_HEADERS ON
  DEPRECATION_WARNINGS
  GLOBAL_HEADER_GEN ON
  SOURCES ${thread_pools_sources}
  HEADERS ${thread_pools_headers}
  COMPAT_HEADERS ${thread_pools_compat_headers}
  DEPENDENCIES hpx_assertion hpx_config hpx_errors hpx_logging hpx_schedulers
  CMAKE_SUBDIRS examples tests
)
