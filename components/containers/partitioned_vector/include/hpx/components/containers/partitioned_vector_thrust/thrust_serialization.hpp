//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/components/containers/partitioned_vector_thrust/device_staging.hpp>

#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/universal_vector.h>

#include <cstddef>
#include <type_traits>
#include <vector>

namespace hpx::serialization {

    namespace detail {

        // host_vector and universal_vector own host-accessible memory, which
        // lives in the parcel as long as the vector does: serialize it like a
        // std::vector, zero-copy chunks included.
        template <typename Vector>
        void save_host_accessible(output_archive& ar, Vector const& v)
        {
            std::size_t const size = v.size();
            ar << size;
            if (size != 0)
            {
                ar << hpx::serialization::make_array(
                    ::thrust::raw_pointer_cast(v.data()), size);
            }
        }

        template <typename Vector>
        void load_host_accessible(input_archive& ar, Vector& v)
        {
            std::size_t size = 0;
            ar >> size;
            v.resize(size);
            if (size != 0)
            {
                ar >> hpx::serialization::make_array(
                          ::thrust::raw_pointer_cast(v.data()), size);
            }
        }
    }    // namespace detail

    // thrust::device_vector<T, Alloc>
    //
    // The data must be complete when the vector is serialized (sent): the
    // work that produces it has to have finished, as for any other argument
    // nobody may still write to while it is sent. Thrust's algorithms
    // synchronize already. Kernels launched otherwise must be waited for
    // first, e.g. by sending from the continuation of an HPX CUDA future on
    // their stream (hpx::cuda::experimental::get_future_with_event or
    // get_future_with_callback). The copy to the host runs on a separate
    // stream and is not ordered after that work (HPX threads move between
    // OS threads, so not even after the sending thread's default stream).
    // Debug builds warn if a default stream is still busy.

    template <typename T, typename Alloc>
    void serialize(output_archive& ar,
        ::thrust::device_vector<T, Alloc> const& v, unsigned)
    {
        static_assert(std::is_trivially_copyable_v<T>,
            "device_vector elements are serialized as raw bytes");

        std::size_t const size = v.size();
        ar << size;
        if (size == 0)
            return;

        std::size_t const bytes = size * sizeof(T);
        auto const* buffer = detail::device_host_copy_of(
            ar, &v, ::thrust::raw_pointer_cast(v.data()), bytes);

        // tells the receiver how the bytes were stored: it can't tell a
        // zero-copy chunk from data copied into the archive by size alone
        bool const chunked = buffer != nullptr;
        ar << chunked;

        if (chunked)
        {
            ar.save_binary_chunk(
                detail::fundamental_types::none, buffer->data, bytes);
            return;
        }

        // no host copy prepared (small data, or no preprocessing pass): copy
        // the bytes into the archive before the temporary goes away
#if defined(HPX_DEBUG)
        detail::warn_if_device_busy();
#endif
        std::vector<T> host(size);
        ::thrust::copy(v.begin(), v.end(), host.begin());
        ar.save_binary(detail::fundamental_types::none, host.data(), bytes);
    }

    template <typename T, typename Alloc>
    void serialize(
        input_archive& ar, ::thrust::device_vector<T, Alloc>& v, unsigned)
    {
        static_assert(std::is_trivially_copyable_v<T>,
            "device_vector elements are serialized as raw bytes");

        std::size_t size = 0;
        ar >> size;
        v.resize(size);
        if (size == 0)
            return;

        bool chunked = false;
        ar >> chunked;

        std::size_t const bytes = size * sizeof(T);
        if (chunked && bytes >= ar.get_zero_copy_serialization_threshold() &&
            ar.try_get_extra_data<detail::allow_zero_copy_receive>() != nullptr)
        {
            // Zero-copy receive: the networking layer places the data at the
            // address handed over here only after this function returns, and
            // it can't write to device memory. Hand it a pinned staging
            // buffer and copy to the device once the data has arrived, before
            // the parcel's action runs.
            void* staging = detail::stage_device_receive(
                ar, ::thrust::raw_pointer_cast(v.data()), bytes);
            ar.load_binary_chunk(
                detail::fundamental_types::none, staging, bytes, true);
            return;
        }

        // the data is complete when the archive returns it
        std::vector<T> host(size);
        if (chunked)
        {
            ar.load_binary_chunk(
                detail::fundamental_types::none, host.data(), bytes, false);
        }
        else
        {
            ar.load_binary(detail::fundamental_types::none, host.data(), bytes);
        }
        ::thrust::copy(host.begin(), host.end(), v.begin());
    }

    // thrust::universal_vector<T, Alloc>

    template <typename T, typename Alloc>
    void serialize(output_archive& ar,
        ::thrust::universal_vector<T, Alloc> const& v, unsigned)
    {
        detail::save_host_accessible(ar, v);
    }

    template <typename T, typename Alloc>
    void serialize(
        input_archive& ar, ::thrust::universal_vector<T, Alloc>& v, unsigned)
    {
        detail::load_host_accessible(ar, v);
    }

    // thrust::host_vector<T, Alloc>

    template <typename T, typename Alloc>
    void serialize(
        output_archive& ar, ::thrust::host_vector<T, Alloc> const& v, unsigned)
    {
        detail::save_host_accessible(ar, v);
    }

    template <typename T, typename Alloc>
    void serialize(
        input_archive& ar, ::thrust::host_vector<T, Alloc>& v, unsigned)
    {
        detail::load_host_accessible(ar, v);
    }
}    // namespace hpx::serialization
