//  Copyright (c) 2014-2017 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_POLICIES_LOGGING
#define HPX_PARCELSET_POLICIES_LOGGING

#include <bitset>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
//
#include <hpx/config.hpp>
#include <hpx/config/parcelport_defines.hpp>
//
#include <hpx/debugging/print.hpp>

#define LOG_EXCLUSIVE(x)
//
#define FUNC_START_DEBUG_MSG LOG_TRACE_MSG("*** Enter " << __func__);
#define FUNC_END_DEBUG_MSG LOG_TRACE_MSG("### Exit  " << __func__);
//
#define LOG_FORMAT_MSG(x)
#define LOG_DEBUG_MSG(x)
#define LOG_TRACE_MSG(...)
#define LOG_INFO_MSG(x)
#define LOG_WARN_MSG(x)
#define LOG_ERROR_MSG(x)
#define LOG_FATAL_MSG(x) LOG_ERROR_MSG(x)
//
#define LOG_DEVEL_MSG(x)
//
#define LOG_TIMED_INIT(name)
#define LOG_TIMED_MSG(name, level, delay, x)
#define LOG_TIMED_BLOCK(name, level, delay, x)

#endif
