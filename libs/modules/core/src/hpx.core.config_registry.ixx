//  Copyright (c) 2023 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// In a module-file, the optional `module;` must appear first; see [cpp.pre].
module;

// This named module expects to be built with classic headers, not header units.
#define HPX_BUILD_MODULE

#include <hpx/config.hpp>

#include <string>
#include <vector>

export module hpx.core:config_registry;

#include <hpx/modules/config_registry.hpp>
