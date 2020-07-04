//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/execution/executors/execution.hpp>
#include <hpx/execution/executors/execution_information.hpp>
#include <hpx/parallel/algorithms/detail/sample_sort.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace hpx { namespace parallel { inline namespace v1 { namespace detail {

    /// \struct parallel_stable_sort
    /// \brief This a structure for to implement a parallel stable sort
    ///        exception safe
    template <typename Iter, typename Sent, typename Compare>
    struct parallel_stable_sort_helper
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;

        util::range<Iter, Sent> range_initial;
        Compare comp;
        std::size_t nelem;
        value_type* ptr;

        static constexpr std::size_t nelem_min = 1 << 16;

        parallel_stable_sort_helper(Iter first, Sent last, Compare cmp);

        // / brief Perform sorting operation
        template <typename ExPolicy>
        Iter operator()(ExPolicy&& policy, std::uint32_t nthread);

        /// \brief destructor of the typename. The utility is to destroy the
        ///        temporary buffer used in the sorting process
        ~parallel_stable_sort_helper()
        {
            if (ptr != nullptr)
            {
                std::return_temporary_buffer(ptr);
            }
        }
    };    // end struct parallel_stable_sort

    /// \brief constructor of the typename
    ///
    /// \param [in] range_initial : range of elements to sort
    /// \param [in] comp : object for to compare two elements
    /// \param [in] nthread : define the number of threads to use
    ///                  in the process. By default is the number of thread HW
    template <typename Iter, typename Sent, typename Compare>
    parallel_stable_sort_helper<Iter, Sent,
        Compare>::parallel_stable_sort_helper(Iter first, Sent last,
        Compare comp)
      : range_initial(first, last)
      , comp(comp)
      , nelem(range_initial.size())
      , ptr(nullptr)
    {
        HPX_ASSERT(range_initial.valid());
    }

    template <typename Iter, typename Sent, typename Compare>
    template <typename ExPolicy>
    Iter parallel_stable_sort_helper<Iter, Sent, Compare>::operator()(
        ExPolicy&& policy, std::uint32_t nthread)
    {
        std::size_t nptr = (nelem + 1) >> 1;
        Iter last = std::next(range_initial.begin(), nelem);

        if (nelem < nelem_min || nthread < 2)
        {
            spin_sort(range_initial.begin(), range_initial.end(), comp);
            return last;
        }

        if (detail::is_sorted_sequential(
                range_initial.begin(), range_initial.end(), comp))
        {
            return last;
        }

        ptr = std::get_temporary_buffer<value_type>(nptr).first;
        if (ptr == nullptr)
        {
            throw std::bad_alloc();
        }

        // Parallel Process
        util::range<Iter, Sent> range_first(
            range_initial.begin(), range_initial.begin() + nptr);
        util::range<Iter, Sent> range_second(
            range_initial.begin() + nptr, range_initial.end());
        util::range<value_type*> range_buffer(ptr, ptr + nptr);

        sample_sort(range_initial.begin(), range_initial.begin() + nptr, comp,
            nthread, range_buffer);

        sample_sort(range_initial.begin() + nptr, range_initial.end(), comp,
            nthread, range_buffer);

        range_buffer = init_move(range_buffer, range_first);
        range_initial =
            half_merge(range_initial, range_buffer, range_second, comp);

        return last;
    }

    template <typename ExPolicy, typename Iter, typename Sent, typename Compare>
    hpx::future<Iter> parallel_stable_sort(
        ExPolicy&& policy, Iter first, Sent last, Compare&& comp)
    {
        std::size_t const cores = execution::processing_units_count(
            policy.parameters(), policy.executor());

        parallel_stable_sort_helper<Iter, Sent,
            typename std::decay<Compare>::type>
            sorter(first, last, std::forward<Compare>(comp));

        return execution::async_execute(policy.executor(), std::move(sorter),
            std::forward<ExPolicy>(policy), cores);
    }

}}}}    // namespace hpx::parallel::v1::detail
