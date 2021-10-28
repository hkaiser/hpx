//  Copyright (c) 2019-2020 ETH Zurich
//  Copyright (c) 2007-2019 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/datastructures/member_pack.hpp>
#include <hpx/errors/try_catch_exception_ptr.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/execution_base/traits/is_executor.hpp>
#include <hpx/functional/deferred_call.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/functional/unique_function.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/futures/packaged_task.hpp>
#include <hpx/iterator_support/range.hpp>
#include <hpx/synchronization/spinlock.hpp>
#include <hpx/threading/thread.hpp>
#include <hpx/type_support/decay.hpp>
#include <hpx/type_support/pack.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx { namespace execution {

    struct thread_pool_executor
    {
    private:
        using task_type = hpx::util::unique_function_nonser<void()>;

        struct executor_data
        {
            executor_data(unsigned int num_threads)
              : total_tasks_(0)
              , running_(true)
            {
                create_threads(num_threads);
            }

            ~executor_data()
            {
                wait_for_tasks();
                running_.store(false, std::memory_order_release);
                destroy_threads();
            }

        private:
            friend struct thread_pool_executor;

            void create_threads(unsigned int num_threads)
            {
                for (unsigned int i = 0; i != num_threads; ++i)
                {
                    threads_.emplace_back(&executor_data::worker, this, i);
                }
            }

            void destroy_threads()
            {
                for (auto& t : threads_)
                {
                    t.join();
                }
            }

            template <typename F>
            void push_task(F&& task)
            {
                ++total_tasks_;

                std::scoped_lock lock(mtx_);
                tasks_.push(task_type(std::forward<F>(task)));
            }

            bool get_next_task(task_type& task, unsigned int num_thread)
            {
                std::scoped_lock lock(mtx_);
                if (tasks_.empty())
                {
                    return false;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
                return true;
            }

            void worker(unsigned int thread_num)
            {
                while (running_.load(std::memory_order_acquire))
                {
                    if (total_tasks_.load(std::memory_order_acquire) != 0)
                    {
                        task_type task;
                        if (get_next_task(task, thread_num))
                        {
                            task();
                            --total_tasks_;
                        }
                    }
                    else
                    {
                        hpx::this_thread::yield();
                    }
                }
            }

            void wait_for_tasks()
            {
                while (true)
                {
                    if (total_tasks_.load(std::memory_order_acquire) == 0)
                        break;
                    hpx::this_thread::yield();
                }
            }

        private:
            hpx::lcos::local::spinlock mtx_;
            std::atomic<std::size_t> total_tasks_;
            std::vector<hpx::thread> threads_;
            std::queue<task_type> tasks_;
            std::atomic<bool> running_;
        };

    public:
        // Associate the parallel_execution_tag executor tag type as a default
        // with this executor.
        using execution_category = hpx::execution::parallel_execution_tag;

        // Associate the static_chunk_size executor parameters type as a default
        // with this executor.
        using executor_parameters_type = hpx::execution::static_chunk_size;

        explicit thread_pool_executor(
            unsigned int num_threads = hpx::thread::hardware_concurrency())
          : data_(std::make_shared<executor_data>(num_threads))
        {
        }

        template <typename F, typename... Ts,
            typename R = hpx::util::detail::invoke_deferred_result_t<F, Ts...>>
        hpx::future<R> async_execute(F&& f, Ts&&... ts)
        {
            using result_type =
                hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

            hpx::lcos::local::packaged_task<result_type()> pt(
                hpx::util::deferred_call(
                    std::forward<F>(f), std::forward<Ts>(ts)...));

            auto result = pt.get_future();
            data_->push_task(std::move(pt));
            return result;
        }

    private:
        template <typename F, typename Is, typename... Ts>
        struct bulk_exec;

        template <typename F, std::size_t... Is, typename... Ts>
        struct bulk_exec<F, hpx::util::index_pack<Is...>, Ts...>
          : std::enable_shared_from_this<
                bulk_exec<F, hpx::util::index_pack<Is...>, Ts...>>
        {
            template <typename F_, typename... Ts_,
                typename = std::enable_if_t<std::is_constructible_v<F, F_&&>>>
            explicit bulk_exec(F_&& f, hpx::lcos::local::promise<void>&& p,
                std::size_t count, Ts_&&... ts)
              : f_(std::forward<F_>(f))
              , args_(std::piecewise_construct, std::forward<Ts_>(ts)...)
              , p_(std::move(p))
              , count_(count)
            {
            }

            bulk_exec(bulk_exec const&) = delete;
            bulk_exec& operator=(bulk_exec const&) = delete;

            template <typename S>
            void schedule_tasks(
                std::shared_ptr<executor_data> const& data, S const& shape)
            {
                auto this_ = this->shared_from_this();
                for (auto s : shape)
                {
                    data->push_task([s, this_]() {
                        hpx::detail::try_catch_exception_ptr(
                            [&]() {
                                HPX_INVOKE(this_->f_, s,
                                    this_->args_.template get<Is>()...);
                            },
                            [&](std::exception_ptr ep) {
                                // store the first caught exception only
                                std::lock_guard l(this_->mtx_);
                                if (!this_->e_)
                                {
                                    this_->e_ = std::move(ep);
                                }
                            });

                        // count down in any case
                        if (--this_->count_ == 0)
                        {
                            // propage result if done
                            if (this_->e_)
                            {
                                this_->p_.set_exception(std::move(this_->e_));
                            }
                            else
                            {
                                this_->p_.set_value();
                            }
                        }
                    });
                }
            }

            F f_;
            hpx::util::member_pack_for<Ts...> args_;

            hpx::lcos::local::promise<void> p_;
            std::atomic<std::size_t> count_;
            hpx::lcos::local::spinlock mtx_;
            std::exception_ptr e_;
        };

    public:
        template <typename F, typename S, typename... Ts,
            typename R = hpx::util::detail::invoke_deferred_result_t<F,
                typename hpx::traits::range_traits<S>::value_type, Ts...>>
        std::vector<hpx::future<R>> bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts)
        {
            using result_type = hpx::util::detail::invoke_deferred_result_t<F,
                typename hpx::traits::range_traits<S>::value_type, Ts...>;

            std::vector<hpx::future<result_type>> results;
            if constexpr (std::is_void_v<result_type>)
            {
                hpx::lcos::local::promise<void> p;
                results.push_back(p.get_future());

                using bulk_exec_type = bulk_exec<std::decay_t<F>,
                    hpx::util::make_index_pack_t<sizeof...(Ts)>,
                    hpx::util::decay_unwrap_t<Ts>...>;

                auto create_tasks = std::make_shared<bulk_exec_type>(
                    std::forward<F>(f), std::move(p), std::size(shape),
                    std::forward<Ts>(ts)...);

                create_tasks->schedule_tasks(data_, shape);
            }
            else
            {
                results.reserve(std::size(shape));

                for (auto s : shape)
                {
                    hpx::lcos::local::packaged_task<result_type()> pt(
                        hpx::util::deferred_call(f, std::move(s), ts...));

                    results.push_back(pt.get_future());

                    data_->push_task(std::move(pt));
                }
            }
            return results;
        }

        /// \cond NOINTERNAL
        constexpr bool operator==(
            thread_pool_executor const& rhs) const noexcept
        {
            return true;
        }

        constexpr bool operator!=(
            thread_pool_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        constexpr thread_pool_executor const& context() const noexcept
        {
            return *this;
        }
        /// \endcond

    private:
        std::shared_ptr<executor_data> data_;
    };
}}    // namespace hpx::execution

namespace hpx { namespace parallel { namespace execution {
    /// \cond NOINTERNAL
    template <>
    struct is_two_way_executor<hpx::execution::thread_pool_executor>
      : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<hpx::execution::thread_pool_executor>
      : std::true_type
    {
    };
    /// \endcond
}}}    // namespace hpx::parallel::execution
