//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/async.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/parallel/algorithms/detail/is_sorted.hpp>
#include <hpx/parallel/algorithms/detail/spin_sort.hpp>
#include <hpx/parallel/util/merge_four.hpp>
#include <hpx/parallel/util/merge_vector.hpp>
#include <hpx/parallel/util/range.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

namespace hpx { namespace parallel { inline namespace v1 { namespace detail {

    /// \struct sample_sort
    /// \brief This a structure for to implement a sample sort, exception
    ///        safe
    /// \tparam
    /// \remarks
    template <typename Iter, typename Sent, typename Compare>
    struct sample_sort_helper
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        using range_it = util::range<Iter, Sent>;
        using range_buf = util::range<value_type*>;

        static constexpr std::uint32_t thread_min = (1 << 16);

        std::uint32_t nthread;
        std::uint32_t ninterval;
        bool construct = false;
        bool owner = false;
        Compare comp;
        range_it global_range;
        range_buf global_buf;

        std::vector<std::vector<range_it>> vv_range_it;
        std::vector<std::vector<range_buf>> vv_range_buf;
        std::vector<range_it> vrange_it_ini;
        std::vector<range_buf> vrange_buf_ini;
        std::atomic<std::uint32_t> njob;

        void initial_configuration();

        /// \brief constructor of the typename
        ///
        /// \param [in] range_initial : range of objects to sort
        /// \param [in] comp : object for to Compare two elements
        /// \param [in] nthread : define the number of threads to use
        ///              in the process. By default is the number of thread HW
        sample_sort_helper(Iter first, Sent last, Compare cmp,
            std::uint32_t num_threads, value_type* paux, std::size_t naux);

        sample_sort_helper(Iter first, Sent last, std::uint32_t num_threads)
          : sample_sort_helper(first, last, Compare(), num_threads, nullptr, 0)
        {
        }

        sample_sort_helper(
            Iter first, Sent last, Compare cmp, std::uint32_t num_threads)
          : sample_sort_helper(first, last, cmp, num_threads, nullptr, 0)
        {
        }

        /// \brief constructor of the typename
        ///
        /// \param [in] range_initial : range of elements to sort
        /// \param [in] comp : object for to Compare two elements
        /// \param [in] nthread :define the number of threads to use
        ///             in the process. By default is the number of thread HW
        /// \param [in] range_buf_initial : range used as auxiliary memory
        sample_sort_helper(Iter first, Sent last, Compare cmp,
            std::uint32_t num_threads, range_buf range_buf_initial)
          : sample_sort_helper(first, last, cmp, num_threads,
                range_buf_initial.first, range_buf_initial.size())
        {
        }

        /// \brief destructor of the typename. The utility is to destroy the
        ///        temporary buffer used in the sorting process
        ~sample_sort_helper();

        /// \brief this a function to assign to each thread in the first merge
        inline void execute_first(void)
        {
            std::uint32_t job = 0;
            while ((job = ++njob) < ninterval)
            {
                uninit_merge_level4(vrange_buf_ini[job], vv_range_it[job],
                    vv_range_buf[job], comp);
            }
        }

        /// \brief this is a function to assign each thread the final merge
        inline void execute(void)
        {
            uint32_t job = 0;
            while ((job = ++njob) < ninterval)
            {
                merge_vector4(vrange_buf_ini[job], vrange_it_ini[job],
                    vv_range_buf[job], vv_range_it[job], comp);
            }
        }

        /// \brief Implement the merge of the initially sparse ranges
        inline void first_merge(void)
        {
            njob = 0;
            std::vector<hpx::future<void>> vfuture(nthread);

            for (std::uint32_t i = 0; i < nthread; ++i)
            {
                vfuture[i] =
                    hpx::async(&sample_sort_helper::execute_first, this);
            }
            for (std::uint32_t i = 0; i < nthread; ++i)
                vfuture[i].get();
        }

        /// \brief Implement the final merge of the ranges
        /// \exception
        /// \return
        /// \remarks
        inline void final_merge()
        {
            njob = 0;
            std::vector<hpx::future<void>> vfuture(nthread);
            for (std::uint32_t i = 0; i < nthread; ++i)
            {
                vfuture[i] = hpx::async(&sample_sort_helper::execute, this);
            }
            for (std::uint32_t i = 0; i < nthread; ++i)
                vfuture[i].get();
        }
    };

    /// \brief constructor of the typename
    ///
    /// \param [in] range_initial : range of objects to sort
    /// \param [in] comp : object for to Compare two elements
    /// \param [in] nthread : NThread object for to define the number of threads
    ///            to use in the process. By default is the number of thread HW
    template <typename Iter, typename Sent, typename Compare>
    sample_sort_helper<Iter, Sent, Compare>::sample_sort_helper(Iter first,
        Sent last, Compare cmp, std::uint32_t num_threads, value_type* paux,
        std::size_t naux)
      : nthread(num_threads)
      , owner(false)
      , construct(false)
      , comp(cmp)
      , global_range(first, last)
      , global_buf(nullptr, nullptr)
      , njob(0)
    {
        HPX_ASSERT(detail::distance(first, last) >= 0);
        std::size_t nelem =
            static_cast<std::size_t>(detail::distance(first, last));

        // Adjust when have many threads and only a few elements
        while (nelem > thread_min && (nthread * nthread) > (nelem >> 3))
        {
            nthread /= 2;
        }

        ninterval = (nthread << 3);

        if (nthread < 2 || nelem <= (thread_min))
        {
            spin_sort(first, last, comp);
            return;
        }

        if (detail::is_sorted_sequential(first, last, comp))
            return;

        if (paux != nullptr)
        {
            HPX_ASSERT(naux != 0);
            global_buf.first = paux;
            global_buf.last = paux + naux;
            owner = false;
        }
        else
        {
            value_type* ptr =
                std::get_temporary_buffer<value_type>(nelem).first;
            if (ptr == nullptr)
            {
                throw std::bad_alloc();
            }
            owner = true;
            global_buf = range_buf(ptr, ptr + nelem);
        }

        initial_configuration();

        first_merge();

        construct = true;
        final_merge();
    }

    /// \brief destructor of the typename. The utility is to destroy the temporary
    ///        buffer used in the sorting process
    template <typename Iter, typename Sent, typename Compare>
    sample_sort_helper<Iter, Sent, Compare>::~sample_sort_helper(void)
    {
        if (construct)
        {
            destroy(global_buf);
            construct = false;
        }

        if (global_buf.first != nullptr && owner)
        {
            std::return_temporary_buffer(global_buf.first);
        }
    }

    /// \brief Create the internal data structures, and obtain the inital set of
    ///        ranges to merge
    /// \exception
    /// \return
    /// \remarks
    template <typename Iter, typename Sent, typename Compare>
    void sample_sort_helper<Iter, Sent, Compare>::initial_configuration()
    {
        std::vector<range_it> vmem_thread;
        std::vector<range_buf> vbuf_thread;
        std::size_t nelem = global_range.size();

        std::size_t cupo = nelem / nthread;
        Iter it_first = global_range.first;
        value_type* buf_first = global_buf.first;

        for (std::uint32_t i = 0; i < nthread - 1;
             ++i, it_first += cupo, buf_first += cupo)
        {
            vmem_thread.emplace_back(it_first, it_first + cupo);
            vbuf_thread.emplace_back(buf_first, buf_first + cupo);
        }

        vmem_thread.emplace_back(it_first, global_range.last);
        vbuf_thread.emplace_back(buf_first, global_buf.last);

        std::vector<hpx::future<void>> vfuture(nthread);

        for (std::uint32_t i = 0; i < nthread; ++i)
        {
            auto func = [=]() {
                spin_sort(vmem_thread[i].first, vmem_thread[i].last, comp,
                    vbuf_thread[i]);
            };
            vfuture[i] = hpx::async(func);
        }

        for (std::uint32_t i = 0; i < nthread; ++i)
        {
            vfuture[i].get();
        }

        // Obtain the vector of milestones
        std::vector<Iter> vsample;
        vsample.reserve(nthread * (ninterval - 1));

        for (std::uint32_t i = 0; i < nthread; ++i)
        {
            std::size_t distance = vmem_thread[i].size() / ninterval;
            for (std::size_t j = 1, pos = distance; j < ninterval;
                 ++j, pos += distance)
            {
                vsample.push_back(vmem_thread[i].first + pos);
            }
        }

        typedef less_ptr_no_null<Iter, Sent, Compare> compare_ptr;
        typedef typename std::vector<Iter>::iterator it_to_it;
        spin_sort<it_to_it, it_to_it, compare_ptr>(
            vsample.begin(), vsample.end(), compare_ptr(comp));

        // Create the final milestone vector
        std::vector<Iter> vmilestone;
        vmilestone.reserve(ninterval);

        for (std::uint32_t pos = nthread >> 1; pos < vsample.size();
             pos += nthread)
        {
            vmilestone.push_back(vsample[pos]);
        }

        // Creation of the first vector of ranges
        std::vector<std::vector<util::range<Iter>>> vv_range_first(nthread);

        for (std::uint32_t i = 0; i < nthread; ++i)
        {
            Iter itaux = vmem_thread[i].first;

            for (std::uint32_t k = 0; k < (ninterval - 1); ++k)
            {
                Iter it2 = std::upper_bound(
                    itaux, vmem_thread[i].last, *vmilestone[k], comp);

                vv_range_first[i].emplace_back(itaux, it2);
                itaux = it2;
            }
            vv_range_first[i].emplace_back(itaux, vmem_thread[i].last);
        }

        // Copy in buffer and creation of the final matrix of ranges
        vv_range_it.resize(ninterval);
        vv_range_buf.resize(ninterval);
        vrange_it_ini.reserve(ninterval);
        vrange_buf_ini.reserve(ninterval);

        for (std::uint32_t i = 0; i < ninterval; ++i)
        {
            vv_range_it[i].reserve(nthread);
            vv_range_buf[i].reserve(nthread);
        }

        Iter it = global_range.first;
        value_type* it_buf = global_buf.first;

        for (std::uint32_t k = 0; k < ninterval; ++k)
        {
            std::size_t nelem_interval = 0;

            for (std::uint32_t i = 0; i < nthread; ++i)
            {
                size_t nelem_range = vv_range_first[i][k].size();
                if (nelem_range != 0)
                {
                    vv_range_it[k].push_back(vv_range_first[i][k]);
                }
                nelem_interval += nelem_range;
            }

            vrange_it_ini.emplace_back(it, it + nelem_interval);
            vrange_buf_ini.emplace_back(it_buf, it_buf + nelem_interval);

            it += nelem_interval;
            it_buf += nelem_interval;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iter, typename Sent, typename Compare, typename Value>
    void sample_sort(Iter first, Sent last, Compare&& comp,
        std::uint32_t num_threads, Value* paux, std::size_t naux)
    {
        sample_sort_helper<Iter, Sent, typename std::decay<Compare>::type>
            sorter(first, last, std::forward<Compare>(comp), num_threads, paux,
                naux);
    }

    template <typename Iter, typename Sent, typename Compare>
    void sample_sort(
        Iter first, Sent last, Compare&& comp, std::uint32_t num_threads)
    {
        sample_sort(
            first, last, std::forward<Compare>(comp), num_threads, nullptr, 0);
    }

    template <typename Iter, typename Sent, typename Compare>
    void sample_sort(Iter first, Sent last, Compare&& comp,
        std::uint32_t num_threads,
        util::range<typename std::iterator_traits<Iter>::value_type*>
            range_buf_initial)
    {
        sample_sort(first, last, std::forward<Compare>(comp), num_threads,
            range_buf_initial.first, range_buf_initial.size());
    }

    template <typename Iter, typename Sent>
    void sample_sort(Iter first, Sent last, std::uint32_t num_threads)
    {
        using compare =
            std::less<typename std::iterator_traits<Iter>::value_type>;

        sample_sort(
            first, last, compare{}, num_threads, nullptr, std::size_t(0));
    }

    template <typename Iter, typename Sent>
    void sample_sort(Iter first, Sent last)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        using compare = std::less<value_type>;

        sample_sort(first, last, compare{},
            (std::uint32_t) hpx::threads::hardware_concurrency(),
            (value_type*) nullptr, std::size_t(0));
    }

}}}}    // namespace hpx::parallel::v1::detail
