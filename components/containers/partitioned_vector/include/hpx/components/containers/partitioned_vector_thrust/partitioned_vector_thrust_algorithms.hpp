//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file partitioned_vector_thrust_algorithms.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/thrust.hpp>

#include <hpx/components/containers/partitioned_vector/partitioned_vector_decl.hpp>
#include <hpx/components/containers/partitioned_vector_thrust/partitioned_vector_thrust_component_decl.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::thrust::detail {

    template <typename Iter, typename Policy>
    struct thrust_fill_object
      : hpx::parallel::detail::algorithm<thrust_fill_object<Iter, Policy>, Iter>
    {
        constexpr thrust_fill_object()
          : hpx::parallel::detail::algorithm<thrust_fill_object, Iter>(
                "thrust_fill")
        {
        }

        template <typename ExPolicy, typename InIterB, typename InIterE,
            typename F, typename Proj>
        static InIterB sequential(
            ExPolicy&&, InIterB first, InIterE last, F&& f, Proj&&)
        {
            hpx::thrust::hpx_invoke(
                hpx::fill_t{}, Policy{}, first.base(), last.base(), f.value_);
            return last;
        }
    };

}    // namespace hpx::thrust::detail

namespace hpx::thrust {

    // must have thrust execution policy and use segmented iterators
    HPX_CXX_EXPORT template <typename Policy, typename SegIter, typename T>
        requires(is_thrust_execution_policy_v<std::decay_t<Policy>> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    SegIter segmented_fill(Policy&&, SegIter first, SegIter last, T const& value)
    {
        if (first == last)
        {
            return first;
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using local_iterator = typename iterator_traits::local_iterator;
        using value_type = typename std::iterator_traits<SegIter>::value_type;

        return hpx::parallel::detail::segmented_for_each(
            detail::thrust_fill_object<local_iterator, std::decay_t<Policy>>(),
            hpx::execution::seq, first, last,
            hpx::parallel::detail::fill_function<value_type>(value),
            hpx::identity_v, std::true_type{});
    }

    // this does not compile yet. If it did, you could simply call hpx::fill()
    // on a partitioned device vector instead of having to explicitly use
    // hpx::thrust::segmented_fill()
    HPX_CXX_EXPORT template <typename Policy, typename SegIter, typename T>
        requires(is_thrust_execution_policy_v<std::decay_t<Policy>> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    SegIter hpx_invoke(
        hpx::fill_t, Policy&& policy, SegIter first, SegIter last, T const& value)
    {
        return segmented_fill(HPX_FORWARD(Policy, policy), HPX_MOVE(first),
            HPX_MOVE(last), value);
    }
}    // namespace hpx::thrust
