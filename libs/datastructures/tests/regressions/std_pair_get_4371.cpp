//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datastructures.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/testing.hpp>

#include <cstddef>
#include <utility>

struct data_hash_key : public std::pair<std::size_t, std::size_t>
{
    data_hash_key(std::pair<std::size_t, std::size_t> const& key)
      : std::pair<std::size_t, std::size_t>(key)
    {
    }

    std::size_t namespace_hash() const
    {
        return std::get<0>(*this);
    }

    std::size_t name_hash() const
    {
        return std::get<1>(*this);
    }
};

int main()
{
    data_hash_key h(std::make_pair(0, 0));

    HPX_TEST_EQ(h.namespace_hash(), std::size_t(0));
    HPX_TEST_EQ(h.name_hash(), std::size_t(0));

    return hpx::util::report_errors();
}
