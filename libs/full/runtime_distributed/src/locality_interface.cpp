//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/naming_base/detail/locality_interface_functions.hpp>
#include <hpx/naming_base/locality.hpp>
#include <hpx/parcelset/parcelhandler.hpp>
#include <hpx/runtime_distributed.hpp>

#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    namespace detail::impl {

        locality create_locality(std::string const& name)
        {
            HPX_ASSERT(get_runtime_ptr());
            return get_runtime_distributed()
                .get_parcel_handler()
                .create_locality(name);
        }
    }    // namespace detail::impl

    // initialize locality interface function pointers in naming_base module
    struct HPX_EXPORT locality_interface_functions
    {
        locality_interface_functions()
        {
            detail::create_locality = &detail::impl::create_locality;
        }
    };

    locality_interface_functions locality_init;
}    // namespace hpx::parcelset
