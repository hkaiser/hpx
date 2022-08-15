//  Copyright (c) 2021-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/queries/get_scheduler.hpp>
#include <hpx/execution_base/sender.hpp>

#include <cstddef>

namespace hpx::execution::experimental {

    class system_scheduler;

    // The system_context creates a view on some underlying execution context
    // supporting parallel forward progress. A system_context must outlive any
    // work launched on it.
    //
    // The system_context must outlive work launched on it. If there is
    // outstanding work at the point of destruction, std::terminate will be
    // called.
    //
    // The system_context must outlive schedulers obtained from it. If there are
    // outstanding schedulers at destruction time, this is undefined behavior.
    class system_context
    {
    public:
        system_context();
        ~system_context();

        // The system_context is non-copyable and non-moveable.
        system_context(system_context const&) = delete;
        system_context(system_context&&) = delete;
        system_context& operator=(system_context const&) = delete;
        system_context& operator=(system_context&&) = delete;

        // return a system_scheduler instance that holds a reference to the
        // system_context.
        system_scheduler get_scheduler()
        {
            return system_scheduler(this);
        }

        // will return a value representing the maximum number of threads the
        // context may support. This is not a snapshot of the current number of
        // threads, and may return numeric_limits<size_t>::max. If the return
        // value is 0, then execute_chunk must be used by at least 1 thread to
        // drive the context.
        std::size_t max_concurrency() noexcept;
    };

    // A system_scheduler is a copyable handle to a system_context. It is the
    // means through which agents are launched on a system_context. The
    // system_scheduler instance does not have to outlive work submitted to it.
    //
    // A system_scheduler has reference semantics with respect to its
    // system_context. Calling any operation other than the destructor on a
    // system_scheduler after the system_context it was created from is
    // destroyed is undefined behavior, and that operation may access freed
    // memory.
    //
    // The system_scheduler:
    //  - satisfies the scheduler concept and implements the schedule
    //    customisation point to return an implementation-defined sender type.
    //  - implements the get_forward_progress_guarantee query to return
    //    parallel.
    //  - implements the bulk CPO to customise the bulk sender adapter such
    //    that: When execution::set_value(r, args...) is called on the created
    //    receiver, an agent is created with parallel forward progress on the
    //    underlying system_context for each i of type Shape from 0 to sh that
    //    calls f(i, args...).
    class system_scheduler
    {
    private:
        friend class system_context;

        system_scheduler(system_context* context)
          : context(context)
        {
        }

    public:
        // system_scheduler is not independely constructable, and must be
        // obtained from a system_context. It is both move and copy
        // constructable and assignable.
        system_scheduler() = delete;

        ~system_scheduler() {}

        system_scheduler(system_scheduler const& rhs)
          : context(rhs.context)
        {
        }
        system_scheduler(system_scheduler&& rhs) noexcept
          : context(rhs.context)
        {
            rhs.context = nullptr;
        }
        system_scheduler& operator=(system_scheduler const& rhs)
        {
            context = rhs.context;
            return *this;
        }
        system_scheduler& operator=(system_scheduler&& rhs) noexcept
        {
            if (this != &rhs)
            {
                context = rhs.context;
                rhs.context = nullptr;
            }
            return *this;
        }

        // Two system_schedulers compare equal if they share the same underlying
        // system_context.
        constexpr bool operator==(system_scheduler const& rhs) const noexcept
        {
            return context == rhs.context;
        }
        constexpr bool operator!=(system_scheduler const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        // schedule calls on a system_scheduler are non-blocking operations.
        friend system_scheduler_sender tag_invoke(
            schedule_t, system_scheduler const&) noexcept;

        friend forward_progress_guarantee tag_invoke(
            get_forward_progress_guarantee_t, system_scheduler const&) noexcept
        {
            return forward_progress_guarantee::parallel;
        }

        template <typename Shape, typename F>
        friend thread_pool_bulk_sender tag_invoke(
            bulk_t, system_scheduler const&,
            Shape const& sh, F&& f) noexcept;

    private:
        system_context* context = nullptr;
    };
}    // namespace hpx::execution::experimental
