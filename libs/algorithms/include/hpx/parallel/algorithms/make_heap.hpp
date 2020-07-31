//  Copyright (c) 2015 Grant Mercer
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/make_heap.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Constructs a \a max \a heap in the range [first, last).
    ///
    /// \note Complexity: at most (3*N) comparisons where
    ///       \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution of
    ///                     the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RndIter     The type of the source iterators used for algorithm.
    ///                     This iterator must meet the requirements for a
    ///                     random access iterator.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param comp         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second. The signature of the function should be
    ///                     equivalent to
    ///                     \code
    ///                     bool comp(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a RndIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The predicate operations in the parallel \a make_heap algorithm invoked
    /// with an execution policy object of type \a sequential_execution_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a make_heap algorithm invoked
    /// with an execution policy object of type \a parallel_execution_policy
    /// or \a parallel_task_execution_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a make_heap algorithm returns a \a hpx::future<void>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns \a void otherwise.
    ///
    template <typename ExPolicy, typename RndIter, typename Comp>
    typename util::detail::algorithm_result<ExPolicy>::type make_heap(
        ExPolicy&& policy, RndIter first, RndIter last, Comp&& comp);

    /// Constructs a \a max \a heap in the range [first, last). Uses the
    /// operator \a < for comparisons.
    ///
    /// \note Complexity: at most (3*N) comparisons where
    ///       \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution of
    ///                     the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RndIter     The type of the source iterators used for algorithm.
    ///                     This iterator must meet the requirements for a
    ///                     random access iterator.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    ///
    /// The predicate operations in the parallel \a make_heap algorithm invoked
    /// with an execution policy object of type \a sequential_execution_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a make_heap algorithm invoked
    /// with an execution policy object of type \a parallel_execution_policy
    /// or \a parallel_task_execution_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a make_heap algorithm returns a \a hpx::future<void>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns \a void otherwise.
    ///
    template <typename ExPolicy, typename RndIter>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy>::type
    make_heap(ExPolicy&& policy, RndIter first, RndIter last);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/concepts/concepts.hpp>
#include <hpx/functional/bind_front.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/functional/tag_invoke.hpp>
#include <hpx/functional/traits/is_invocable.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/iterator_support/traits/is_iterator.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_local.hpp>

#include <hpx/execution/executors/execution.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/executors/execution_policy.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/chunk_size.hpp>
#include <hpx/parallel/util/detail/handle_local_exceptions.hpp>
#include <hpx/parallel/util/detail/scoped_executor_parameters.hpp>
#include <hpx/parallel/util/projection_identity.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

namespace hpx { namespace parallel { inline namespace v1 {

    //////////////////////////////////////////////////////////////////////
    // make_heap
    namespace detail {

        template <typename ExPolicy, typename F1, typename RndIter>
        // requires traits::is_future<Future>
        std::vector<hpx::util::tuple<RndIter, std::size_t>>
        get_bottomup_heap_bulk_iteration_shape(ExPolicy&& policy, F1&& f1,
            RndIter& first, std::size_t size, std::size_t& chunk_size)
        {
            std::size_t const cores = execution::processing_units_count(
                policy.parameters(), policy.executor());

            // Take a standard chunk size (amount of work / cores), and only
            // take a quarter of that. If our chunk size is too large a LOT of
            // the work will be done sequentially due to the level barrier of
            // heap parallelism.
            // 1/4 of the standard chunk size is an estimate to lower the
            // average number of levels done sequentially
            chunk_size = execution::get_chunk_size(
                policy.parameters(), policy.executor(),
                [](std::size_t) { return 0; }, cores, size);
            chunk_size /= 4;

            std::size_t max_chunks = execution::maximal_number_of_chunks(
                policy.parameters(), policy.executor(), cores, size);

            util::detail::adjust_chunk_size_and_max_chunks(
                cores, size, chunk_size, max_chunks);

            using tuple_type = hpx::util::tuple<RndIter, std::size_t>;
            std::vector<tuple_type> shape;

            std::size_t start = (size - 2) / 2;
            while (start > 0)
            {
                // Index of start of level, and amount of items in level
                std::size_t end_exclusive =
                    (std::size_t) std::pow(2, std::floor(std::log2(start))) - 2;
                std::size_t level_items = (start - end_exclusive);

                // If we can't at least run two chunks in parallel, don't bother
                // parallelizing and simply run sequentially
                if (chunk_size * 2 > level_items)
                {
                    f1(first + start, level_items);
                }
                else
                {
                    shape.push_back(
                        hpx::util::make_tuple(first + start, level_items));
                }

                start = end_exclusive;
            }

            // Perform f1 on head node
            if (shape.empty())
            {
                f1(first, 1);
            }
            else
            {
                shape.push_back(hpx::util::make_tuple(first, 1));
            }

            return shape;
        }

        // Perform bottom up heap construction given a range of elements.
        // sift_down_range will take a range from [start,start-count) and
        // apply sift_down to each element in the range
        template <typename RndIter, typename Comp, typename Proj>
        void sift_down(RndIter first, Comp&& comp, Proj&& proj,
            typename std::iterator_traits<RndIter>::difference_type len,
            RndIter start)
        {
            typename std::iterator_traits<RndIter>::difference_type child =
                start - first;

            if (len < 2 || (len - 2) / 2 < child)
                return;

            child = 2 * child + 1;
            RndIter child_i = first + child;

            if ((child + 1) < len &&
                hpx::util::invoke(comp, hpx::util::invoke(proj, *child_i),
                    hpx::util::invoke(proj, *(child_i + 1))))
            {
                ++child_i;
                ++child;
            }

            if (hpx::util::invoke(comp, hpx::util::invoke(proj, *child_i),
                    hpx::util::invoke(proj, *start)))
                return;

            typename std::iterator_traits<RndIter>::value_type top = *start;

            do
            {
                *start = *child_i;
                start = child_i;

                if ((len - 2) / 2 < child)
                    break;

                child = 2 * child + 1;
                child_i = first + child;

                if ((child + 1) < len &&
                    hpx::util::invoke(comp, hpx::util::invoke(proj, *child_i),
                        hpx::util::invoke(proj, *(child_i + 1))))
                {
                    ++child_i;
                    ++child;
                }

            } while (!hpx::util::invoke(comp, hpx::util::invoke(proj, *child_i),
                hpx::util::invoke(proj, top)));

            *start = top;
        }

        template <typename RndIter, typename Comp, typename Proj>
        void sift_down_range(RndIter first, Comp&& comp, Proj&& proj,
            typename std::iterator_traits<RndIter>::difference_type len,
            RndIter start, std::size_t count)
        {
            for (std::size_t i = 0; i != count; ++i)
            {
                sift_down(first, comp, proj, len, start - i);
            }
        }

        template <typename Iter, typename Sent, typename Comp, typename Proj>
        Iter sequential_make_heap(
            Iter first, Sent last, Comp&& comp, Proj&& proj)
        {
            using difference_type =
                typename std::iterator_traits<Iter>::difference_type;

            difference_type n = last - first;
            if (n > 1)
            {
                for (difference_type start = (n - 2) / 2; start >= 0; --start)
                {
                    sift_down(first, comp, proj, n, first + start);
                }
                return first + n;
            }
            return first;
        }

        //////////////////////////////////////////////////////////////////////
        template <typename Iter>
        struct make_heap : public detail::algorithm<make_heap<Iter>, Iter>
        {
            make_heap()
              : make_heap::algorithm("make_heap")
            {
            }

            template <typename ExPolicy, typename RndIter, typename Sent,
                typename Comp, typename Proj>
            static RndIter sequential(
                ExPolicy, RndIter first, Sent last, Comp&& comp, Proj&& proj)
            {
                return sequential_make_heap(first, last,
                    std::forward<Comp>(comp), std::forward<Proj>(proj));
            }

            template <typename ExPolicy, typename RndIter, typename Sent,
                typename Comp, typename Proj>
            static
                typename util::detail::algorithm_result<ExPolicy, RndIter>::type
                parallel(ExPolicy&& policy, RndIter first, Sent last,
                    Comp&& comp, Proj&& proj)
            {
                typename std::iterator_traits<RndIter>::difference_type n =
                    last - first;
                if (n <= 1)
                {
                    return util::detail::algorithm_result<ExPolicy,
                        RndIter>::get(std::move(first));
                }

                using execution_policy = typename std::decay<ExPolicy>::type;
                using parameters_type =
                    typename execution_policy::executor_parameters_type;
                using executor_type = typename execution_policy::executor_type;

                using scoped_executor_parameters =
                    util::detail::scoped_executor_parameters_ref<
                        parameters_type, executor_type>;

                scoped_executor_parameters scoped_params(
                    policy.parameters(), policy.executor());

                using tuple_type =
                    typename hpx::util::tuple<RndIter, std::size_t>;

                std::vector<hpx::future<void>> workitems;
                std::list<std::exception_ptr> errors;

                auto op = [=, comp = std::forward<Comp>(comp),
                              proj = std::forward<Proj>(proj)](
                              RndIter it, std::size_t size) {
                    std::cout << (it - first) << "," << size << "\n";
                    sift_down_range(
                        first, comp, proj, (std::size_t) n, it, size);
                };

                try
                {
                    // Get workitems that are to be run in parallel
                    std::size_t chunk_size = 0;
                    std::vector<tuple_type> shape =
                        get_bottomup_heap_bulk_iteration_shape(
                            policy, op, first, (std::size_t) n, chunk_size);

                    for (auto const& iteration : shape)
                    {
                        // Chunk up range of each iteration and execute
                        // asynchronously
                        RndIter begin = hpx::util::get<0>(iteration);
                        std::size_t length = hpx::util::get<1>(iteration);
                        while (length != 0)
                        {
                            std::size_t chunk = (std::min)(chunk_size, length);

                            workitems.push_back(execution::async_execute(
                                policy.executor(), op, begin, chunk));

                            length -= chunk;
                            begin += chunk;
                        }

                        hpx::wait_all(workitems);

                        // collect exceptions
                        util::detail::handle_local_exceptions<ExPolicy>::call(
                            workitems, errors, false);
                        workitems.clear();

                        if (!errors.empty())
                            break;    // stop on errors
                    }

                    scoped_params.mark_end_of_scheduling();
                }
                catch (...)
                {
                    util::detail::handle_local_exceptions<ExPolicy>::call(
                        std::current_exception(), errors);
                }

                // rethrow exceptions, if any
                util::detail::handle_local_exceptions<ExPolicy>::call(
                    workitems, errors);

                std::advance(first, n);
                return util::detail::algorithm_result<ExPolicy, RndIter>::get(
                    std::move(first));
            }

            template <typename RndIter, typename Sent, typename Comp,
                typename Proj>
            static typename util::detail::algorithm_result<
                execution::parallel_task_policy, RndIter>::type
            parallel(execution::parallel_task_policy policy, RndIter first,
                Sent last, Comp&& comp, Proj&& proj)
            {
                typename std::iterator_traits<RndIter>::difference_type n =
                    last - first;
                if (n <= 1)
                {
                    return util::detail::algorithm_result<
                        execution::parallel_task_policy,
                        RndIter>::get(std::move(first));
                }

                // inform parameter traits
                using execution_policy = execution::parallel_task_policy;
                using parameters_type =
                    typename execution_policy::executor_parameters_type;
                using executor_type = typename execution_policy::executor_type;

                using scoped_executor_parameters =
                    util::detail::scoped_executor_parameters_ref<
                        parameters_type, executor_type>;

                // inform parameter traits
                std::shared_ptr<scoped_executor_parameters> scoped_params =
                    std::make_shared<scoped_executor_parameters>(
                        policy.parameters(), policy.executor());

                typedef typename hpx::util::tuple<RndIter, std::size_t>
                    tuple_type;

                std::vector<hpx::future<void>> workitems;
                std::list<std::exception_ptr> errors;

                auto op = [=, comp = std::forward<Comp>(comp),
                              proj = std::forward<Proj>(proj)](
                              RndIter it, std::size_t size) {
                    sift_down_range(
                        first, comp, proj, (std::size_t) n, it, size);
                };

                try
                {
                    // Get workitems that are to be run in parallel
                    std::size_t chunk_size = 0;
                    std::vector<tuple_type> shape =
                        get_bottomup_heap_bulk_iteration_shape(
                            policy, op, first, (std::size_t) n, chunk_size);

                    for (auto const& iteration : shape)
                    {
                        // Chunk up range of each iteration and execute
                        // asynchronously
                        RndIter begin = hpx::util::get<0>(iteration);
                        std::size_t length = hpx::util::get<1>(iteration);
                        while (length != 0)
                        {
                            std::size_t chunk = (std::min)(chunk_size, length);

                            workitems.push_back(execution::async_execute(
                                policy.executor(), op, begin, chunk));

                            length -= chunk;
                            std::advance(begin, chunk);
                        }

                        hpx::wait_all(workitems);

                        // collect exceptions
                        util::detail::handle_local_exceptions<
                            execution::parallel_task_policy>::call(workitems,
                            errors, false);
                        workitems.clear();

                        if (!errors.empty())
                            break;    // stop on errors
                    }

                    scoped_params->mark_end_of_scheduling();
                }
                catch (std::bad_alloc const&)
                {
                    return hpx::make_exceptional_future<RndIter>(
                        std::current_exception());
                }
                catch (...)
                {
                    util::detail::handle_local_exceptions<
                        execution::parallel_task_policy>::
                        call(std::current_exception(), errors);
                }

                // Perform local exception handling within a dataflow,
                // because otherwise the exception would be thrown outside
                // of the future which is not the desired behavior
                return hpx::dataflow(
                    [first, n, errors = std::move(errors),
                        scoped_params = std::move(scoped_params)](
                        std::vector<hpx::future<void>>&& r) mutable {
                        util::detail::handle_local_exceptions<
                            execution::parallel_task_policy>::call(r, errors);

                        std::advance(first, n);
                        return first;
                    },
                    std::move(workitems));
            }
        };
    }    // namespace detail
}}}      // namespace hpx::parallel::v1

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::make_heap
    HPX_INLINE_CONSTEXPR_VARIABLE struct make_heap_t final
      : hpx::functional::tag<make_heap_t>
    {
    private:
        // clang-format off
        template <typename ExPolicy, typename RndIter, typename Comp,
            HPX_CONCEPT_REQUIRES_(
                hpx::parallel::execution::is_execution_policy<ExPolicy>::value &&
                hpx::traits::is_iterator<RndIter>::value &&
                hpx::traits::is_invocable<Comp,
                    typename std::iterator_traits<RndIter>::value_type,
                    typename std::iterator_traits<RndIter>::value_type
                >::value
            )>
        // clang-format on
        friend typename hpx::parallel::util::detail::algorithm_result<
            ExPolicy>::type
        tag_invoke(make_heap_t, ExPolicy&& policy, RndIter first, RndIter last,
            Comp&& comp)
        {
            static_assert(
                hpx::traits::is_random_access_iterator<RndIter>::value,
                "Requires random access iterator.");

            using is_seq =
                hpx::parallel::execution::is_sequenced_execution_policy<
                    ExPolicy>;

            return hpx::parallel::util::detail::algorithm_result<ExPolicy>::get(
                hpx::parallel::v1::detail::make_heap<RndIter>().call(
                    std::forward<ExPolicy>(policy), is_seq{}, first, last,
                    std::forward<Comp>(comp),
                    hpx::parallel::util::projection_identity{}));
        }

        // clang-format off
        template <typename ExPolicy, typename RndIter,
            HPX_CONCEPT_REQUIRES_(
                hpx::parallel::execution::is_execution_policy<ExPolicy>::value &&
                hpx::traits::is_iterator<RndIter>::value
            )>
        // clang-format on
        friend typename hpx::parallel::util::detail::algorithm_result<
            ExPolicy>::type
        tag_invoke(make_heap_t, ExPolicy&& policy, RndIter first, RndIter last)
        {
            static_assert(
                hpx::traits::is_random_access_iterator<RndIter>::value,
                "Requires random access iterator.");

            using is_seq =
                hpx::parallel::execution::is_sequenced_execution_policy<
                    ExPolicy>;
            using value_type =
                typename std::iterator_traits<RndIter>::value_type;

            return hpx::parallel::util::detail::algorithm_result<ExPolicy>::get(
                hpx::parallel::v1::detail::make_heap<RndIter>().call(
                    std::forward<ExPolicy>(policy), is_seq{}, first, last,
                    std::less<value_type>(),
                    hpx::parallel::util::projection_identity{}));
        }

        // clang-format off
        template <typename RndIter, typename Comp,
            HPX_CONCEPT_REQUIRES_(
                hpx::traits::is_iterator<RndIter>::value &&
                hpx::traits::is_invocable<Comp,
                    typename std::iterator_traits<RndIter>::value_type,
                    typename std::iterator_traits<RndIter>::value_type
                >::value
            )>
        // clang-format on
        friend void tag_invoke(
            make_heap_t, RndIter first, RndIter last, Comp&& comp)
        {
            static_assert(
                hpx::traits::is_random_access_iterator<RndIter>::value,
                "Requires random access iterator.");

            hpx::parallel::v1::detail::make_heap<RndIter>().call(
                hpx::parallel::execution::seq, std::true_type{}, first, last,
                std::forward<Comp>(comp),
                hpx::parallel::util::projection_identity{});
        }

        // clang-format off
        template <typename RndIter,
            HPX_CONCEPT_REQUIRES_(
                hpx::traits::is_iterator<RndIter>::value
            )>
        // clang-format on
        friend void tag_invoke(make_heap_t, RndIter first, RndIter last)
        {
            static_assert(
                hpx::traits::is_random_access_iterator<RndIter>::value,
                "Requires random access iterator.");

            using value_type =
                typename std::iterator_traits<RndIter>::value_type;

            hpx::parallel::v1::detail::make_heap<RndIter>().call(
                hpx::parallel::execution::seq, std::true_type{}, first, last,
                std::less<value_type>(),
                hpx::parallel::util::projection_identity{});
        }
    } make_heap;
}    // namespace hpx

#endif    // DOXYGEN
