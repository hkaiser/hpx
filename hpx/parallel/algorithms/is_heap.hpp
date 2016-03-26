//  Copyright (c) 2015 Grant Mercer
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/is_heap.hpp

#if !defined(HPX_PARALLEL_ALGORITHMS_IS_HEAP_DEC_15_2015_1012AM)
#define HPX_PARALLEL_ALGORITHMS_IS_HEAP_DEC_15_2015_1012AM

#include <hpx/traits/is_iterator.hpp>
#include <hpx/lcos/local/dataflow.hpp>
#include <hpx/lcos/wait_all.hpp>
#include <hpx/util/bind.hpp>
#include <hpx/util/unwrapped.hpp>

#include <hpx/parallel/config/inline_namespace.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>

#include <hpx/parallel/executors/executor_traits.hpp>
#include <hpx/parallel/execution_policy.hpp>
#include <hpx/parallel/util/detail/handle_local_exceptions.hpp>

#include <algorithm>
#include <iterator>
#include <functional>
#include <type_traits>

#include <boost/utility/enable_if.hpp>

namespace hpx { namespace parallel { HPX_INLINE_NAMESPACE(v1)
{
    ///////////////////////////////////////////////////////////////
    // is_heap_until
    namespace detail
    {
        template <typename RndIter, typename Pred>
        void comp_heap(RndIter first, Pred pred,
            typename std::iterator_traits<RndIter>::difference_type len,
            typename std::iterator_traits<RndIter>::difference_type start,
            typename std::iterator_traits<RndIter>::difference_type n,
            util::cancellation_token<std::size_t> &tok)
        {
            typedef typename std::iterator_traits<RndIter>::difference_type
                dtype;

            dtype p = start;
            dtype c = 2 * p + 1;
            RndIter pp = first + start;
            while (c < len && n > 0)
            {
                if (tok.was_cancelled(c))
                    break;

                RndIter cp = first + c;
                if (pred(*pp, *cp))
                {
                    tok.cancel(c);
                    break;
                }

                ++c;
                ++cp;
                if (c <= 0)
                    break;

                if (pred(*pp, *cp))
                {
                    tok.cancel(c);
                    break;
                }

                ++p;
                ++pp;
                --n;
                c = 2 * p + 1;
            }
        }

        /// \cond NOINTERNAL
        template <typename RndIter>
        struct is_heap_until
          : public detail::algorithm<is_heap_until<RndIter>, RndIter>
        {
            is_heap_until()
              : is_heap_until::algorithm("is_heap_until")
            {}

            template <typename ExPolicy, typename Pred>
            static RndIter
            sequential(ExPolicy&&, RndIter first, RndIter last, Pred && pred)
            {
                return std::is_heap_until(first, last, std::forward<Pred>(pred));
            }

            template <typename ExPolicy, typename Pred>
            static typename util::detail::algorithm_result<
                ExPolicy, RndIter
            >::type
            parallel(ExPolicy&& policy, RndIter first, RndIter last, Pred && pred)
            {
                typedef typename ExPolicy::executor_type executor_type;
                typedef typename hpx::parallel::executor_traits<executor_type>
                    executor_traits;
                typedef typename std::iterator_traits<RndIter>::difference_type
                    dtype;
                typedef typename util::detail::algorithm_result<ExPolicy,
                        RndIter> result;

                dtype len = last - first;

                if (len <= 1)
                    return result::get(std::move(last));

                std::list<boost::exception_ptr> errors;
                std::size_t chunk_size = 4;

                std::vector<hpx::future<void> > workitems;
                workitems.reserve(std::distance(first,last)/chunk_size);
                util::cancellation_token<std::size_t> tok(len);

                try {
                    if (pred(*first, *(first+1)) || pred(*first, *(first+2)))
                        return result::get(std::move(first));

                    dtype max_level = (dtype)ceil(log2(len));
                    for (dtype level = 2; level < max_level; ++level)
                    {
                        dtype start = (dtype)pow(2, level-1) - 1;
                        dtype end_exclusive = (dtype)pow(2, level) - 1;

                        std::size_t items = end_exclusive - start;

                        // If amount of work is less than chunk size, just run
                        // it sequentially
                        if (items < chunk_size)
                        {
                            auto op =
                                hpx::util::bind(&comp_heap<RndIter, Pred>,
                                    first, std::forward<Pred>(pred), len, start,
                                    items, tok);

                            workitems.push_back(
                                executor_traits::async_execute(
                                    policy.executor(), std::move(op)));
                        }
                        else
                        {
                            std::size_t cnt = 0;
                            while (cnt < items)
                            {
                                std::size_t size =
                                    (std::min)(chunk_size, items - cnt);

                                auto op =
                                    hpx::util::bind(&comp_heap<RndIter, Pred>,
                                    first, pred, len, start + cnt, size, tok);

                                workitems.push_back(
                                    executor_traits::async_execute(
                                        policy.executor(), std::move(op)));

                                cnt += size;
                            }
                        }

                        hpx::wait_all(workitems);
                    }
                }
                catch (...)
                {
                    util::detail::handle_local_exceptions<ExPolicy>::call(
                        boost::current_exception(), errors);
                }

                util::detail::handle_local_exceptions<ExPolicy>::call(
                    workitems, errors);

                dtype pos = static_cast<dtype>(tok.get_data());
                if (pos != len)
                    std::advance(first, pos);
                else
                    first = last;

                return result::get(std::move(first));
            }

            template <typename Pred>
            static typename util::detail::algorithm_result<
                parallel_task_execution_policy, RndIter
            >::type
            parallel(parallel_task_execution_policy && policy, RndIter first,
                RndIter last, Pred && pred)
            {
                typedef typename parallel_task_execution_policy::executor_type
                    executor_type;
                typedef typename hpx::parallel::executor_traits<executor_type>
                    executor_traits;
                typedef typename std::iterator_traits<RndIter>::difference_type
                    dtype;
                typedef typename util::detail::algorithm_result<
                        parallel_task_execution_policy, RndIter
                    > result;

                dtype len = last - first;
                if (len <= 1)
                    return result::get(std::move(last));

                std::list<boost::exception_ptr> errors;
                std::size_t chunk_size = 4;

                std::vector<hpx::future<void> > workitems;
                workitems.reserve(std::distance(first, last) / chunk_size);
                util::cancellation_token<dtype> tok(len);

                try {
                    if (pred(*first, *(first+1)) || pred(*first, *(first+2)))
                        return result::get(std::move(first));

                    dtype max_level = (dtype)ceil(log2(len));
                    for(dtype level = 2; level < max_level; ++level)
                    {
                        dtype start = (dtype)pow(2, level-1) - 1;
                        dtype end_exclusive = (dtype)pow(2, level) - 1;

                        std::size_t items = end_exclusive - start;

                        // If amount of work is less than chunk size, just run
                        // it sequentially
                        if (chunk_size > items)
                        {
                            auto op =
                                hpx::util::bind(&comp_heap<RndIter, Pred>,
                                    first, std::forward<Pred>(pred), len,
                                    start, items, tok);

                            workitems.push_back(
                                executor_traits::async_execute(
                                    policy.executor(), std::move(op)));
                        }
                        else
                        {
                            std::size_t cnt = 0;
                            while (cnt < items)
                            {
                                std::size_t size =
                                    (std::min)(chunk_size, items - cnt);

                                auto op =
                                    hpx::util::bind(&comp_heap<RndIter, Pred>,
                                        first, pred, len, start + cnt, size, tok);

                                workitems.push_back(
                                    executor_traits::async_execute(
                                        policy.executor(), std::move(op)));

                                cnt += size;
                            }
                        }

                        hpx::wait_all(workitems);
                    }
                }
                catch (std::bad_alloc const&) {
                    return hpx::make_exceptional_future<RndIter>(
                            boost::current_exception());
                }
                catch (...) {
                    util::detail::handle_local_exceptions<
                        parallel_task_execution_policy>::call(
                            boost::current_exception(), errors);
                }

                return hpx::lcos::local::dataflow(
                    [=](std::vector<hpx::future<void> > && r) mutable -> RndIter
                    {
                        util::detail::handle_local_exceptions<
                            parallel_task_execution_policy>::call(r, errors);

                        dtype pos = tok.get_data();
                        if(pos != len)
                            std::advance(first, pos);
                        else
                            first = last;
                        return std::move(first);
                    },
                    std::move(workitems));
            }
        };
        /// \endcond
    }

    /// Examines the range [first, last) and finds the largest range beginning
    /// at first which is a \a max \a heap.
    ///
    /// \note Complexity: at most (N) predicate evaluations where
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
    /// \param pred         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second. The signature of the function should be
    ///                     equivalent to
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a RndIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The predicate operations in the parallel \a is_heap_until algorithm
    /// invoked with an execution policy object of type
    /// \a sequential_execution_policy executes in sequential order in the
    /// calling thread.
    ///
    /// The comparison operations in the parallel \a is_heap_until algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_execution_policy or \a parallel_task_execution_policy are
    /// permitted to execute in an unordered fashion in unspecified threads,
    /// and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a is_heap_until algorithm returns a \a hpx::future<RndIter>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns a \a RndIter otherwise. The iterator corresponds
    ///           to the upper bound of the largest range beginning at first which
    ///           is a \a max \a heap.
    ///
    template <typename ExPolicy, typename RndIter, typename Pred>
    inline typename boost::enable_if<
        is_execution_policy<ExPolicy>,
        typename util::detail::algorithm_result<ExPolicy, RndIter>::type
    >::type
    is_heap_until(ExPolicy && policy, RndIter first, RndIter last, Pred && pred)
    {
        static_assert(hpx::traits::is_random_access_iterator<RndIter>::value,
            "Requires random access iterator.");

        typedef typename is_sequential_execution_policy<ExPolicy>::type is_seq;
        typedef typename std::iterator_traits<RndIter>::value_type value_type;

        return detail::is_heap_until<RndIter>().call(
            std::forward<ExPolicy>(policy), is_seq(), first, last,
            std::forward<Pred>(pred));
    }

    /// Examines the range [first, last) and finds the largest range beginning
    /// at first which is a \a max \a heap. Uses the operator \a < for comparison
    ///
    /// \note Complexity: at most (N) predicate evaluations where
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
    /// The predicate operations in the parallel \a is_heap_until algorithm invoked
    /// with an execution policy object of type \a sequential_execution_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a is_heap_until algorithm invoked
    /// with an execution policy object of type \a parallel_execution_policy
    /// or \a parallel_task_execution_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a is_heap_until algorithm returns a \a hpx::future<RndIter>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns a \a RndIter otherwise. The iterator corresponds
    ///           to the upper bound of the largest range beginning at first which
    ///           is a \a max \a heap.
    ///
    template <typename ExPolicy, typename RndIter>
    inline typename boost::enable_if<
        is_execution_policy<ExPolicy>,
        typename util::detail::algorithm_result<ExPolicy, RndIter>::type
    >::type
    is_heap_until(ExPolicy && policy, RndIter first, RndIter last)
    {
        static_assert(hpx::traits::is_random_access_iterator<RndIter>::value,
            "Requires random access iterator.");

        typedef typename is_sequential_execution_policy<ExPolicy>::type is_seq;
        typedef typename std::iterator_traits<RndIter>::value_type value_type;

        return detail::is_heap_until<RndIter>().call(
            std::forward<ExPolicy>(policy), is_seq(), first, last,
            std::less<value_type>());
    }

    namespace detail
    {
        /// \cond NOINTERNAL
        struct is_heap: public detail::algorithm<is_heap, bool>
        {
            is_heap()
               : is_heap::algorithm("is_heap")
            {}

            template <typename ExPolicy, typename RndIter, typename Pred>
            static bool
            sequential(ExPolicy, RndIter first, RndIter last, Pred && pred)
            {
                return std::is_heap(first, last, std::forward<Pred>(pred));
            }

            template <typename ExPolicy, typename RndIter, typename Pred>
            static typename util::detail::algorithm_result<ExPolicy, bool>::type
            parallel(ExPolicy && policy, RndIter first, RndIter last, Pred && pred)
            {
                typedef typename util::detail::algorithm_result<ExPolicy, bool>
                    result;

                return result::get(
                    is_heap_until<RndIter>().call(
                        policy, boost::mpl::false_(),
                        first, last, std::forward<Pred>(pred)
                    ) == last);
            }

            template <typename RndIter, typename Pred>
            static typename util::detail::algorithm_result<
                parallel_task_execution_policy, bool
            >::type
            parallel(parallel_task_execution_policy && policy,
                RndIter first, RndIter last, Pred && pred)
            {
                typedef typename util::detail::algorithm_result<
                    parallel_task_execution_policy, bool>
                    result;

                return is_heap_until<RndIter>().call(
                        policy, boost::mpl::false_(),
                        first, last, std::forward<Pred>(pred)
                    ).then(
                        [=](hpx::future<RndIter> f) -> bool
                        {
                            return f.get() == last;
                        });
            }
        };
    }

    /// Determines if the range [first, last) is a \a max \a heap.
    ///
    /// \note Complexity: at most (N) predicate evaluations where
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
    /// \param pred         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second. The signature of the function should be
    ///                     equivalent to
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a RndIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The predicate operations in the parallel \a is_heap algorithm invoked
    /// with an execution policy object of type \a sequential_execution_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a is_heap algorithm invoked
    /// with an execution policy object of type \a parallel_execution_policy
    /// or \a parallel_task_execution_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a is_heap algorithm returns a \a hpx::future<bool>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns a \a bool otherwise.
    ///           The \a is_heap algorithm returns true if each element in the sequence
    ///           for which pred returns true precedes those for which pred returns
    ///           false. Otherwise \a is_heap returns false. If the range [first,last)
    ///           contains less than two elements, the function always returns true.
    ///
    template <typename ExPolicy, typename RndIter, typename Pred>
    inline typename boost::enable_if<
        is_execution_policy<ExPolicy>,
        typename util::detail::algorithm_result<ExPolicy, bool>::type
    >::type
    is_heap(ExPolicy && policy, RndIter first, RndIter last, Pred && pred)
    {
        static_assert(hpx::traits::is_random_access_iterator<RndIter>::value,
            "Requires random access iterator.");

        typedef typename is_sequential_execution_policy<ExPolicy>::type is_seq;
        typedef typename std::iterator_traits<RndIter>::value_type value_type;

        return detail::is_heap().call(
            std::forward<ExPolicy>(policy), is_seq(), first, last,
            std::forward<Pred>(pred));
    }

    /// Determines if the range [first, last) is a \a max \a heap. Uses the
    /// operator \a < for comparisons
    ///
    /// \note Complexity: at most (N) predicate evaluations where
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
    /// The predicate operations in the parallel \a is_heap algorithm invoked
    /// with an execution policy object of type \a sequential_execution_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a is_heap algorithm invoked
    /// with an execution policy object of type \a parallel_execution_policy
    /// or \a parallel_task_execution_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a is_heap algorithm returns a \a hpx::future<bool>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns a \a bool otherwise.
    ///           The \a is_heap algorithm returns true if each element in the sequence
    ///           for which pred returns true precedes those for which the operator <
    ///           returns false. Otherwise \a is_heap returns false. If the range
    ///           [first,last) contains less than two elements, the function always
    ///           returns true.
    ///
    template <typename ExPolicy, typename RndIter>
    inline typename boost::enable_if<
        is_execution_policy<ExPolicy>,
        typename util::detail::algorithm_result<ExPolicy, bool>::type
    >::type
    is_heap (ExPolicy && policy, RndIter first, RndIter last)
    {
        static_assert(hpx::traits::is_random_access_iterator<RndIter>::value,
            "Requires random access iterator.");

        typedef typename is_sequential_execution_policy<ExPolicy>::type is_seq;
        typedef typename std::iterator_traits<RndIter>::value_type value_type;

        return detail::is_heap().call(
                std::forward<ExPolicy>(policy), is_seq(), first, last,
                std::less<value_type>());
    }
}}}

#endif
