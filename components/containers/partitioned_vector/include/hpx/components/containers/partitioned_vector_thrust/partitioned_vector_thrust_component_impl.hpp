//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/assertion.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_component_impl.hpp>
#include <hpx/components/containers/partitioned_vector_thrust/partitioned_vector_thrust_component_decl.hpp>

#include <thrust/copy.h>
#include <thrust/device_vector.h>

#include <cstddef>

/// \cond NOINTERNAL

namespace hpx::server {

    namespace detail {
        template <typename T, typename Alloc>
        using thrust_partition =
            partitioned_vector<T, ::thrust::device_vector<T, Alloc>>;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Alloc>
    detail::thrust_partition<T, Alloc>::partitioned_vector()
    {
        HPX_ASSERT(false);    // shouldn't ever be called
    }

    template <typename T, typename Alloc>
    detail::thrust_partition<T, Alloc>::partitioned_vector(
        std::size_t partnum, std::vector<size_type> const& partition_sizes)
    {
        partitioned_vector_partition_.resize(partition_sizes[partnum]);
    }

    template <typename T, typename Alloc>
    detail::thrust_partition<T, Alloc>::partitioned_vector(std::size_t partnum,
        std::vector<size_type> const& partition_sizes, T const& val)
      : partitioned_vector_partition_(partition_sizes[partnum], val)
    {
    }

    template <typename T, typename Alloc>
    detail::thrust_partition<T, Alloc>::partitioned_vector(std::size_t partnum,
        std::vector<size_type> const& partition_sizes, T const& val,
        allocator_type const& alloc)
      : partitioned_vector_partition_(partition_sizes[partnum], val, alloc)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::get_data() -> data_type&
    {
        return partitioned_vector_partition_;
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::get_data() const
        -> data_type const&
    {
        return partitioned_vector_partition_;
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::get_copied_data() const
        -> data_type
    {
        return partitioned_vector_partition_;
    }

    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::set_data(data_type&& other)
    {
        partitioned_vector_partition_ = HPX_MOVE(other);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::begin() -> iterator_type
    {
        return partitioned_vector_partition_.begin();
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::begin() const
        -> const_iterator_type
    {
        return partitioned_vector_partition_.begin();
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::cbegin() const
        -> const_iterator_type
    {
        return partitioned_vector_partition_.cbegin();
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::end() -> iterator_type
    {
        return partitioned_vector_partition_.end();
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::end() const -> const_iterator_type
    {
        return partitioned_vector_partition_.end();
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::cend() const -> const_iterator_type
    {
        return partitioned_vector_partition_.cend();
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::size() const -> size_type
    {
        return partitioned_vector_partition_.size();
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::max_size() const -> size_type
    {
        return partitioned_vector_partition_.max_size();
    }

    template <typename T, typename Alloc>
    auto detail::thrust_partition<T, Alloc>::capacity() const -> size_type
    {
        return partitioned_vector_partition_.capacity();
    }

    template <typename T, typename Alloc>
    bool detail::thrust_partition<T, Alloc>::empty() const
    {
        return partitioned_vector_partition_.empty();
    }

    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::resize(
        size_type n, T const& val)
    {
        partitioned_vector_partition_.resize(n, val);
    }

    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::reserve(size_type n)
    {
        partitioned_vector_partition_.reserve(n);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Alloc>
    T detail::thrust_partition<T, Alloc>::get_value(size_type pos) const
    {
        return static_cast<T>(partitioned_vector_partition_[pos]);
    }

    template <typename T, typename Alloc>
    std::vector<T> detail::thrust_partition<T, Alloc>::get_values(
        std::vector<size_type> const& pos) const
    {
        size_type const offset = pos.empty() ? 0 : pos.front();

        // if pos is not empty, then only a portion of a segment is being used
        size_type const count =
            pos.empty() ? partitioned_vector_partition_.size() : pos.size();

        HPX_ASSERT(offset + count <= partitioned_vector_partition_.size());

        std::vector<T> result(count);
        auto const first = partitioned_vector_partition_.begin() + offset;
        ::thrust::copy(first, first + count, result.begin());
        return result;
    }

    template <typename T, typename Alloc>
    T detail::thrust_partition<T, Alloc>::front() const
    {
        return static_cast<T>(partitioned_vector_partition_.front());
    }

    template <typename T, typename Alloc>
    T detail::thrust_partition<T, Alloc>::back() const
    {
        return static_cast<T>(partitioned_vector_partition_.back());
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::assign(
        size_type n, T const& val)
    {
        partitioned_vector_partition_.assign(n, val);
    }

    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::push_back(T const& val)
    {
        partitioned_vector_partition_.push_back(val);
    }

    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::pop_back()
    {
        partitioned_vector_partition_.pop_back();
    }

    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::set_value(
        size_type pos, T const& val)
    {
        partitioned_vector_partition_[pos] = val;
    }

    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::set_values(
        std::vector<size_type> const& pos, std::vector<T> val)
    {
        size_type const offset = pos.empty() ? 0 : pos.front();

        HPX_ASSERT(pos.empty() || pos.size() == val.size());
        HPX_ASSERT(offset + val.size() < partitioned_vector_partition_.size());

        ::thrust::copy(val.begin(), val.end(),
            partitioned_vector_partition_.begin() + offset);
    }

    template <typename T, typename Alloc>
    void detail::thrust_partition<T, Alloc>::clear()
    {
        partitioned_vector_partition_.clear();
    }

    template <typename T, typename Alloc>
    template <typename F, typename... Ts>
    util::invoke_result_t<F, T, Ts...>
    detail::thrust_partition<T, Alloc>::apply(std::size_t pos, F f, Ts... ts)
    {
        return HPX_INVOKE(
            HPX_MOVE(f), partitioned_vector_partition_[pos], HPX_MOVE(ts)...);
    }
}    // namespace hpx::server

/// \endcond
