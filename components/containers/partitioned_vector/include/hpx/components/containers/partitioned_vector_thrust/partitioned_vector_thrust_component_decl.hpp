//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file partitioned_vector_thrust_component_decl.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector_fwd.hpp>
#include <hpx/components/containers/partitioned_vector/partitioned_vector_component_decl.hpp>
#include <hpx/components/containers/partitioned_vector_thrust/thrust_serialization.hpp>

#include <thrust/device_vector.h>

#include <type_traits>

namespace hpx::server {

    template <typename T, typename Alloc>
    class partitioned_vector<T, ::thrust::device_vector<T, Alloc>>
      : public components::locking_hook<components::component_base<
            partitioned_vector<T, ::thrust::device_vector<T, Alloc>>>>
    {
    public:
        using data_type = ::thrust::device_vector<T, Alloc>;

        using allocator_type = Alloc;
        using size_type = typename data_type::size_type;
        using iterator_type = typename data_type::iterator;
        using const_iterator_type = typename data_type::const_iterator;

        using base_type = components::locking_hook<components::component_base<
            partitioned_vector<T, ::thrust::device_vector<T, Alloc>>>>;

        data_type partitioned_vector_partition_;

        ///////////////////////////////////////////////////////////////////////
        // Constructors
        ///////////////////////////////////////////////////////////////////////

        partitioned_vector();

        explicit partitioned_vector(
            std::size_t partnum, std::vector<size_type> const& partition_sizes);

        partitioned_vector(std::size_t partnum,
            std::vector<size_type> const& partition_sizes, T const& val);

        partitioned_vector(std::size_t partnum,
            std::vector<size_type> const& partition_sizes, T const& val,
            allocator_type const& alloc);

        partitioned_vector(partitioned_vector const& rhs) = default;
        partitioned_vector(partitioned_vector&& rhs) = default;

        partitioned_vector& operator=(partitioned_vector const& rhs) = default;
        partitioned_vector& operator=(partitioned_vector&& rhs) = default;

        ///////////////////////////////////////////////////////////////////////
        data_type& get_data();
        data_type const& get_data() const;

        data_type get_copied_data() const;

        void set_data(data_type&& other);

        ///////////////////////////////////////////////////////////////////////
        iterator_type begin();
        const_iterator_type begin() const;
        const_iterator_type cbegin() const;

        iterator_type end();
        const_iterator_type end() const;
        const_iterator_type cend() const;

        ///////////////////////////////////////////////////////////////////////
        size_type size() const;
        size_type max_size() const;
        size_type capacity() const;
        bool empty() const;

        void resize(size_type n, T const& val);
        void reserve(size_type n);

        ///////////////////////////////////////////////////////////////////////
        // each one element access is a device transfer
        T get_value(size_type pos) const;

        // uses bulk transfers to avoid inefficiencies
        std::vector<T> get_values(std::vector<size_type> const& pos) const;

        T front() const;
        T back() const;

        ///////////////////////////////////////////////////////////////////////
        void assign(size_type n, T const& val);
        void push_back(T const& val);
        void pop_back();

        // each one element access is a device transfer
        void set_value(size_type pos, T const& val);

        // uses bulk transfers to avoid inefficiencies
        void set_values(std::vector<size_type> const& pos, std::vector<T> val);

        void clear();

        ///////////////////////////////////////////////////////////////////////
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, size)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, resize)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, get_value)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, get_values)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, set_value)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, set_values)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, get_copied_data)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(partitioned_vector, set_data)

        template <typename F, typename... Ts>
        util::invoke_result_t<F, T, Ts...> apply(
            std::size_t pos, F f, Ts... ts);

        template <typename F, typename... Ts>
        struct apply_action
          : hpx::actions::make_action_t<
                decltype(&partitioned_vector::apply<F, Ts...>),
                &partitioned_vector::apply<F, Ts...>, apply_action<F, Ts...>>
        {
        };
    };
}    // namespace hpx::server
