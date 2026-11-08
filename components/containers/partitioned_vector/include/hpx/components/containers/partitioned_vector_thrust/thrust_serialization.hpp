//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>

#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/universal_vector.h>
#include <thrust/host_vector.h>

#include <cstddef>
#include <string>
#include <vector>

namespace hpx::serialization {

    // thrust::device_vector<T, Alloc>

    template <typename T, typename Alloc>
    void serialize(output_archive& ar,
        thrust::device_vector<T, Alloc> const& v, unsigned)
    {
        std::vector<T> host(v.size());
        thrust::copy(v.begin(), v.end(), host.begin());
        ar << host;
    }

    template <typename T, typename Alloc>
    void serialize(input_archive& ar,
        thrust::device_vector<T, Alloc>& v, unsigned)
    {
        std::vector<T> host;
        ar >> host;
        v.resize(host.size());
        thrust::copy(host.begin(), host.end(), v.begin());
    }

    // thrust::universal_vector<T, Alloc>

    template <typename T, typename Alloc>
    void serialize(output_archive& ar,
        thrust::universal_vector<T, Alloc> const& v, unsigned)
    {
        std::vector<T> host(v.size());
        thrust::copy(v.begin(), v.end(), host.begin());
        ar << host;
    }

    template <typename T, typename Alloc>
    void serialize(input_archive& ar,
        thrust::universal_vector<T, Alloc>& v, unsigned)
    {
        std::vector<T> host;
        ar >> host;
        v.resize(host.size());
        thrust::copy(host.begin(), host.end(), v.begin());
    }

    // thrust::host_vector<T, Alloc>

    template <typename T, typename Alloc>
    void serialize(output_archive& ar,
        thrust::host_vector<T, Alloc> const& v, unsigned)
    {
        std::vector<T> host(v.size());
        thrust::copy(v.begin(), v.end(), host.begin());
        ar << host;
    }

    template <typename T, typename Alloc>
    void serialize(input_archive& ar,
        thrust::host_vector<T, Alloc>& v, unsigned)
    {
        std::vector<T> host;
        ar >> host;
        v.resize(host.size());
        thrust::copy(host.begin(), host.end(), v.begin());
    }

}    // namespace hpx::serialization