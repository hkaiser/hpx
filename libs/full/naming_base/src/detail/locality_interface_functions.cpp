//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/naming_base/locality.hpp>
#include <hpx/naming_base/detail/locality_interface_functions.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset::detail {

    locality (*create_locality)(std::string const& name) = nullptr;
}    // namespace hpx::parcelset
