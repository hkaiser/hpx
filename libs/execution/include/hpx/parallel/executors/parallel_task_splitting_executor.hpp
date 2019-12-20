//  Copyright (c) 2007-2019 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/parallel_task_splitting_executor.hpp

#if !defined(HPX_PARALLEL_TASK_SPLITTING_EXECUTOR_DEC_08_2019_0758PM)
#define HPX_PARALLEL_TASK_SPLITTING_EXECUTOR_DEC_08_2019_0758PM

#include <hpx/config.hpp>
#include <hpx/allocator_support/internal_allocator.hpp>
#include <hpx/assertion.hpp>
#include <hpx/iterator_support/range.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/parallel/algorithms/detail/predicates.hpp>
#include <hpx/parallel/executors/parallel_executor.hpp>
#include <hpx/parallel/executors/static_chunk_size.hpp>
#include <hpx/runtime/launch_policy.hpp>
#include <hpx/runtime/threads/policies/scheduler_base.hpp>
#include <hpx/runtime/threads/thread_data.hpp>
#include <hpx/runtime/threads/thread_data_fwd.hpp>
#include <hpx/runtime/threads/thread_helpers.hpp>
#include <hpx/runtime/threads/thread_pool_base.hpp>
#include <hpx/serialization/serialize.hpp>
#include <hpx/synchronization/latch.hpp>
#include <hpx/traits/future_traits.hpp>
#include <hpx/traits/is_executor.hpp>
#include <hpx/util/unwrap.hpp>

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx { namespace parallel {
    namespace execution {

        namespace detail {

            ////////////////////////////////////////////////////////////////////////
            template <typename F, typename Shape, typename... Ts>
            struct bulk_function_result;

            ////////////////////////////////////////////////////////////////////////
            template <typename F, typename Shape, typename Future,
                typename... Ts>
            struct bulk_then_execute_result;

            template <typename F, typename Shape, typename Future,
                typename... Ts>
            struct then_bulk_function_result;
        }    // namespace detail

        ////////////////////////////////////////////////////////////////////////////
        /// A \a parallel_task_splitting_executor creates groups of parallel
        /// execution agents which execute in threads implicitly created by the
        /// executor. This executor creates splittable tasks for bulk execution.
        ///
        /// This executor conforms to the concepts of a TwoWayExecutor,
        /// and a BulkTwoWayExecutor
        template <typename Policy>
        struct parallel_splittable_task_policy_executor
          : parallel_policy_executor
        {
            /// Associate the parallel_execution_tag executor tag type as a default
            /// with this executor.
            typedef parallel_execution_tag execution_category;

            /// Associate the static_chunk_size executor parameters type as a default
            /// with this executor.
            typedef static_chunk_size executor_parameters_type;

            /// Create a new parallel executor
            HPX_CONSTEXPR explicit parallel_splittable_task_policy_executor(
                Policy l = detail::get_default_policy<Policy>::call())
              : parallel_policy_executor(l)
            {
            }

            /// \cond NOINTERNAL
            bool operator==(parallel_splittable_task_policy_executor const& rhs)
                const noexcept
            {
                return parallel_policy_executor(*this) == rhs;
            }

            bool operator!=(parallel_splittable_task_policy_executor const& rhs)
                const noexcept
            {
                return !(*this == rhs);
            }

            parallel_splittable_task_policy_executor const& context()
                const noexcept
            {
                return *this;
            }
            /// \endcond

            /// \cond NOINTERNAL

            // The TwoWayExecutor interface is inherited from the base class
            using parallel_policy_executor::async_execute;
            using parallel_policy_executor::then_execute;

            // The NonBlockingOneWayExecutor (adapted) interface is inherited from
            // the base class
            using parallel_policy_executor::post;

            // BulkTwoWayExecutor interface
            template <typename Shape>
            struct splittable_task
            {
                // a splittable task is scheduled as a nullary function
                void operator()() {}
            };

            template <typename F, typename S, typename... Ts>
            std::vector<hpx::future<void>> bulk_async_execute(
                F&& f, S const& shape, Ts&&... ts) const
            {
                // for now, wrap single future in a vector to avoid having to
                // change the executor and algorithm infrastructure
                splittable_task<S> task(std::);
                std::vector<hpx::future<void>> result;
                result.push_back(
                    parallel_policy_executor::async_execute(std::move(task)));
                return result;
            }

            typedef std::vector<hpx::future<
                typename detail::bulk_function_result<F, S, Ts...>::type>>
                result_type;

            result_type results;
            std::size_t size = hpx::util::size(shape);
            results.resize(size);

            lcos::local::latch l(size);
            if (hpx::detail::has_async_policy(policy_))
            {
                spawn_hierarchical(results, l, 0, size, num_tasks, f,
                    hpx::util::begin(shape), ts...);
            }
            else
            {
                spawn_sequential(
                    results, l, 0, size, f, hpx::util::begin(shape), ts...);
            }
            l.wait();

            return results;
        }

        template <typename F, typename S, typename Future, typename... Ts>
        hpx::future<typename detail::bulk_then_execute_result<F, S, Future,
            Ts...>::type>
        bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts)
        {
            using func_result_type =
                typename detail::then_bulk_function_result<F, S, Future,
                    Ts...>::type;

            // std::vector<future<func_result_type>>
            using result_type = std::vector<hpx::future<func_result_type>>;

            auto&& func =
                detail::make_fused_bulk_async_execute_helper<result_type>(*this,
                    std::forward<F>(f), shape,
                    hpx::util::make_tuple(std::forward<Ts>(ts)...));

            // void or std::vector<func_result_type>
            using vector_result_type =
                typename detail::bulk_then_execute_result<F, S, Future,
                    Ts...>::type;

            // future<vector_result_type>
            using result_future_type = hpx::future<vector_result_type>;

            using shared_state_type =
                typename hpx::traits::detail::shared_state_ptr<
                    vector_result_type>::type;

            using future_type = typename std::decay<Future>::type;

            // vector<future<func_result_type>> -> vector<func_result_type>
            shared_state_type p =
                lcos::detail::make_continuation_alloc<vector_result_type>(
                    hpx::util::internal_allocator<>{},
                    std::forward<Future>(predecessor), policy_,
                    [HPX_CAPTURE_MOVE(func)](future_type&& predecessor) mutable
                    -> vector_result_type {
                        // use unwrap directly (instead of lazily) to avoid
                        // having to pull in dataflow
                        return hpx::util::unwrap(func(std::move(predecessor)));
                    });

            return hpx::traits::future_access<result_future_type>::create(
                std::move(p));
        }
        /// \endcond

    protected:
        /// \cond NOINTERNAL
        /// \endcond

    private:
        /// \cond NOINTERNAL
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            // clang-format off
            ar & serialization::base_object<parallel_policy_executor>(*this);
            // clang-format on
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        /// \endcond
    };    // namespace execution

    using parallel_splittable_task_executor =
        parallel_splittable_task_policy_executor<hpx::launch>;
}}    // namespace hpx::parallel
}    // namespace hpx::parallel::execution

namespace hpx { namespace parallel { namespace execution {

    /// \cond NOINTERNAL
    template <typename Policy>
    struct is_one_way_executor<
        parallel::execution::parallel_splittable_task_policy_executor<Policy>>
      : std::true_type
    {
    };

    template <typename Policy>
    struct is_two_way_executor<
        parallel::execution::parallel_splittable_task_policy_executor<Policy>>
      : std::true_type
    {
    };

    template <typename Policy>
    struct is_bulk_two_way_executor<
        parallel::execution::parallel_splittable_task_policy_executor<Policy>>
      : std::true_type
    {
    };
    /// \endcond
}}}    // namespace hpx::parallel::execution

#endif
