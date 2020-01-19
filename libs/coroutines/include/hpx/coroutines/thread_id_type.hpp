//  Copyright (c) 2018 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/coroutines/thread_id_type.hpp

#ifndef HPX_THREADS_THREAD_ID_TYPE_HPP
#define HPX_THREADS_THREAD_ID_TYPE_HPP

#include <hpx/config/constexpr.hpp>
#include <hpx/config/export_definitions.hpp>
#include <hpx/memory/intrusive_ptr.hpp>
#include <hpx/thread_support/atomic_count.hpp>

#include <cstddef>
#include <functional>
#include <iosfwd>

namespace hpx { namespace threads {

    namespace detail {

        struct thread_data_reference_counting;

        void intrusive_ptr_add_ref(thread_data_reference_counting* p);
        void intrusive_ptr_release(thread_data_reference_counting* p);

        struct thread_data_reference_counting
        {
            thread_data_reference_counting()
              : count_(0)
            {
            }

            virtual ~thread_data_reference_counting() = default;
            virtual void destroy_thread() = 0;

            // reference counting
            friend void intrusive_ptr_add_ref(thread_data_reference_counting* p)
            {
                ++p->count_;
            }

            friend void intrusive_ptr_release(thread_data_reference_counting* p)
            {
                if (--p->count_ == 0)
                {
                    // give this object back to the system
                    p->destroy_thread();
                }
            }

            util::atomic_count count_;
        };
    }    // namespace detail

    struct thread_id
    {
    private:
        using thread_id_repr =
            memory::intrusive_ptr<detail::thread_data_reference_counting>;

    public:
        thread_id() noexcept = default;

        thread_id(thread_id const&) = default;
        thread_id& operator=(thread_id const&) = default;

        thread_id(thread_id&& rhs) noexcept = default;
        thread_id& operator=(thread_id&& rhs) noexcept = default;

        ////////////////////////////////////////////////////////////////////////
        explicit thread_id(thread_id_repr const& thrd) noexcept
          : thrd_(thrd)
        {
        }
        explicit thread_id(thread_id_repr&& thrd) noexcept
          : thrd_(std::move(thrd))
        {
        }

        thread_id& operator=(thread_id_repr const& rhs) noexcept
        {
            thrd_ = rhs;
        }
        thread_id& operator=(thread_id_repr&& rhs) noexcept
        {
            thrd_ = std::move(rhs);
        }

        using thread_repr = detail::thread_data_reference_counting;

        ////////////////////////////////////////////////////////////////////////
        explicit thread_id(thread_repr* thrd) noexcept
          : thrd_(thrd)
        {
        }

        thread_id& operator=(thread_repr* rhs) noexcept
        {
            thrd_.reset(rhs);
            return *this;
        }

        ////////////////////////////////////////////////////////////////////////
        explicit operator bool() const noexcept
        {
            return !!thrd_;
        }

        thread_id_repr& get() & noexcept
        {
            return thrd_;
        }
        thread_id_repr get() && noexcept
        {
            return std::move(thrd_);
        }

        thread_id_repr const& get() const& noexcept
        {
            return thrd_;
        }
        thread_id_repr get() const&& noexcept
        {
            return std::move(thrd_);
        }

        void reset() noexcept
        {
            thrd_ = nullptr;
        }

        friend bool operator==(std::nullptr_t, thread_id const& rhs) noexcept
        {
            return nullptr == rhs.thrd_;
        }

        friend bool operator!=(std::nullptr_t, thread_id const& rhs) noexcept
        {
            return nullptr != rhs.thrd_;
        }

        friend bool operator==(thread_id const& lhs, std::nullptr_t) noexcept
        {
            return nullptr == lhs.thrd_;
        }

        friend bool operator!=(thread_id const& lhs, std::nullptr_t) noexcept
        {
            return nullptr != lhs.thrd_;
        }

        friend bool operator==(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return lhs.thrd_ == rhs.thrd_;
        }

        friend bool operator!=(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return lhs.thrd_ != rhs.thrd_;
        }

        friend bool operator<(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return std::less<thread_repr const*>{}(
                lhs.thrd_.get(), rhs.thrd_.get());
        }

        friend bool operator>(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return std::less<thread_repr const*>{}(
                rhs.thrd_.get(), lhs.thrd_.get());
        }

        friend bool operator<=(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return !(rhs > lhs);
        }

        friend bool operator>=(
            thread_id const& lhs, thread_id const& rhs) noexcept
        {
            return !(rhs < lhs);
        }

        template <typename Char, typename Traits>
        friend std::basic_ostream<Char, Traits>& operator<<(
            std::basic_ostream<Char, Traits>& os, thread_id const& id)
        {
            os << id.get();
            return os;
        }

    private:
        thread_id_repr thrd_;
    };

    static thread_id const invalid_thread_id;

}}    // namespace hpx::threads

#endif
