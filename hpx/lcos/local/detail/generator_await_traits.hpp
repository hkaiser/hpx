//  Copyright (c) 2017 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_LCOS_GENERATOR_AWAIT_TRAITS_HPP)
#define HPX_LCOS_GENERATOR_AWAIT_TRAITS_HPP

#if defined(HPX_HAVE_AWAIT)

#include <hpx/config.hpp>
#include <hpx/lcos/local/channel.hpp>

#if defined(HPX_HAVE_EMULATE_COROUTINE_SUPPORT_LIBRARY)
#include <hpx/util/await_traits.hpp>
#else
#include <experimental/coroutine>
#endif

#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>

#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace lcos { namespace local { namespace detail
{
    template <typename T, typename Derived>
    struct coroutine_generator_base
      : public hpx::lcos::local::detail::one_element_channel<T>
    {
        using base_type = hpx::lcos::local::detail::one_element_channel<T>;

        coroutine_generator_base()
        {
            // the shared state is held alive by the coroutine
            hpx::lcos::local::detail::intrusive_ptr_add_ref(this);
        }

        hpx::generator<T> get_return_object()
        {
            return hpx::generator<T>(this);
        }

        std::experimental::suspend_never initial_suspend()
        {
            return std::experimental::suspend_never{};
        }

        std::experimental::suspend_if final_suspend()
        {
            // close the channel in any case
            base_type::close();

            // This gives up the coroutine's reference count on the shared
            // state. If this was the last reference count, the coroutine
            // should not suspend before exiting.
            return std::experimental::suspend_if{
                !this->base_type::requires_delete()};
        }

        std::experimental::suspend_never yield_value(T t)
        {
            // We suspend inside future::get, if required. Otherwise all sub-
            // sequent 'invocations' of the co-routine will happen on the
            // thread executing the receiving end of the channel.
            base_type::set(std::size_t(-1), std::move(t)).get();
            return std::experimental::suspend_never{};
        }

        void destroy()
        {
            std::experimental::coroutine_handle<Derived>::
                from_promise(*static_cast<Derived*>(this)).destroy();
        }

        void set_exception(std::exception_ptr e)
        {
            try {
                std::rethrow_exception(std::move(e));
            }
            catch (...) {
                this->base_type::set_exception(boost::current_exception());
            }
        }

        void set_exception(boost::exception_ptr e)
        {
            this->base_type::set_exception(std::move(e));
        }


//         hpx::future<T> get(std::size_t generation, bool blocking = false)
//         {
//             std::experimental::coroutine_handle<Derived>::
//                 from_promise(*static_cast<Derived*>(this)).resume();
//
//             return base_type::get(generation, blocking);
//         }

//         bool try_get(std::size_t generation, hpx::future<T>* f = nullptr)
//         {
//             std::experimental::coroutine_handle<Derived>::
//                 from_promise(*static_cast<Derived*>(this)).resume();
//
//             return base_type::try_get(generation, f);
//         }
    };
}}}}

namespace std { namespace experimental
{
    // Allow for functions which use co_yield to return an hpx::generator<T>
    template <typename T, typename ...Ts>
    struct coroutine_traits<hpx::generator<T>, Ts...>
    {
        struct promise_type
          : hpx::lcos::local::detail::coroutine_generator_base<T, promise_type>
        {};
    };
}}

#endif // HPX_HAVE_AWAIT
#endif

