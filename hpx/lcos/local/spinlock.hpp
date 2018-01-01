////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011-2018 Hartmut Kaiser
//  Copyright (c) 2014 Thomas Heller
//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_B3A83B49_92E0_4150_A551_488F9F5E1113)
#define HPX_B3A83B49_92E0_4150_A551_488F9F5E1113

#include <hpx/config.hpp>

#include <hpx/runtime/threads/thread_helpers.hpp>
#include <hpx/util/detail/yield_k.hpp>
#include <hpx/util/itt_notify.hpp>
#include <hpx/util/register_locks.hpp>

#include <cstddef>
#include <cstdint>

#if defined(HPX_HAVE_SPINLOCK_MCS)
#include <atomic>
#else
#if defined(HPX_WINDOWS)
#  include <boost/smart_ptr/detail/spinlock.hpp>
#  if !defined(BOOST_SP_HAS_SYNC)
#    include <boost/detail/interlocked.hpp>
#  endif
#else
#  if !defined(__ANDROID__) && !defined(ANDROID) && !defined(__arm__)
#    include <boost/smart_ptr/detail/spinlock.hpp>
#    if defined(__ia64__) && defined(__INTEL_COMPILER)
#      include <ia64intrin.h>
#    endif
#  endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace lcos { namespace local
{
    namespace detail
    {
#if defined(HPX_HAVE_SPINLOCK_MCS)
        struct spinlock_mcs_node
        {
            HPX_STATIC_CONSTEXPR int cache_line_size = 64;

            spinlock_mcs_node()
              : locked_(true), next_(nullptr)
            {}

            bool locked_;
            std::uint8_t pad1_[cache_line_size - sizeof(bool)];

            spinlock_mcs_node* next_;
            std::uint8_t pad2_[cache_line_size - sizeof(spinlock_mcs_node*)];
        };

        // work around limitation that thread-local storage can't be exported
        template <typename Dummy = void>
        struct spinlock_mcs_node_tls
        {
            static HPX_NATIVE_TLS spinlock_mcs_node data_;
        };

        template <typename Dummy>
        HPX_NATIVE_TLS spinlock_mcs_node spinlock_mcs_node_tls<Dummy>::data_ =
            spinlock_mcs_node{};
    }
#endif

    // std::mutex-compatible spinlock class
    struct spinlock
    {
    public:
        HPX_NON_COPYABLE(spinlock);

    private:
#if defined(HPX_HAVE_SPINLOCK_MCS)
        //
        // The Mellor-Crummey & Scott (MCS) Lock, due to John Mellor-Crummey
        // and Michael Scott improves upon their ticket lock by expanding a
        // spinlock into a per-thread structure, an MCS lock is able to eliminate
        // much of the cache-line bouncing experienced by simpler locks,
        // especially in the contended case.
        //
        // The MCS Lock use an explicit linked list of synchronization variables,
        // into which threads' synchronization variables are enqueued by order
        // of arrival and avoids thread starvation by guaranteeing that an
        // enqueued thread will, eventually, get access.
        //
        // The MCS Lock is, therefore, fair and scalable and is the primary
        // example of linked list queue lock family of locking strategies.
        //
        // See https://www.cs.rice.edu/~johnmc/papers/tocs91.pdf
        //
        std::atomic<detail::spinlock_mcs_node*> tail_;
        detail::spinlock_mcs_node_tls<> local_node_;
#else
#if defined(__ANDROID__) && defined(ANDROID)
        int v_;
#else
        std::uint64_t v_;
#endif
#endif

    public:
        spinlock(char const* const desc = "hpx::lcos::local::spinlock")
#if defined(HPX_HAVE_SPINLOCK_MCS)
          : tail_(nullptr)
#else
          : v_(0)
#endif
        {
            HPX_ITT_SYNC_CREATE(this, desc, "");
        }

        ~spinlock()
        {
            HPX_ITT_SYNC_DESTROY(this);
        }

#if !defined(HPX_HAVE_SPINLOCK_MCS)
        void lock()
        {
            HPX_ITT_SYNC_PREPARE(this);

            for (std::size_t k = 0; !acquire_lock(); ++k)
            {
                util::detail::yield_k(k, "hpx::lcos::local::spinlock::lock",
                    hpx::threads::pending_boost);
            }

            HPX_ITT_SYNC_ACQUIRED(this);
            util::register_lock(this);
        }

        bool try_lock()
        {
            HPX_ITT_SYNC_PREPARE(this);

            bool r = acquire_lock(); //-V707

            if (r) {
                HPX_ITT_SYNC_ACQUIRED(this);
                util::register_lock(this);
                return true;
            }

            HPX_ITT_SYNC_CANCEL(this);
            return false;
        }

        void unlock()
        {
            HPX_ITT_SYNC_RELEASING(this);

            relinquish_lock();

            HPX_ITT_SYNC_RELEASED(this);
            util::unregister_lock(this);
        }

    private:
        // returns whether the mutex has been acquired
        bool acquire_lock()
        {
#if !defined(BOOST_SP_HAS_SYNC)
            std::uint64_t r = BOOST_INTERLOCKED_EXCHANGE(&v_, 1);
            HPX_COMPILER_FENCE;
#else
            std::uint64_t r = __sync_lock_test_and_set(&v_, 1);
#endif
            return r == 0;
        }

        void relinquish_lock()
        {
#if !defined(BOOST_SP_HAS_SYNC)
            HPX_COMPILER_FENCE;
            *const_cast<std::uint64_t volatile*>(&v_) = 0;
#else
            __sync_lock_release(&v_);
#endif
        }

#else // HPX_HAVE_SPINLOCK_MCS

        void lock()
        {
            HPX_ITT_SYNC_PREPARE(this);

            // to acquire the lock a thread atomically appends its own local
            // node at the tail of the list returning tail's previous contents
            detail::spinlock_mcs_node* p =
                tail_.exchange(&local_node_.data_, std::memory_order_acquire);

            if (p != nullptr)
            {
                local_node_.data_.locked_ = true;

                // if the list was not previously empty, it sets the
                // predecessor's next field to refer to its own local node
                p->next_ = &local_node_.data_;

                // the thread then spins on its local locked field, waiting
                // until its predecessor sets this field to false
                for (std::size_t k = 0; local_node_.data_.locked_; ++k)
                {
                    util::detail::yield_k(k, "hpx::lcos::local::spinlock::lock",
                        hpx::threads::pending_boost);
                }
            }

            // now first in the queue, own the lock and enter the critical
            // section...

            HPX_ITT_SYNC_ACQUIRED(this);
            util::register_lock(this);
        }

        bool try_lock()
        {
            HPX_ITT_SYNC_PREPARE(this);

            // attempts to append itself to the tail of the list, if succeeds
            // it has acquired the lock
            detail::spinlock_mcs_node* expected = nullptr;
            bool r = tail_.compare_exchange_strong(
                expected, &local_node_.data_, std::memory_order_acquire);

            if (r) {
                HPX_ITT_SYNC_ACQUIRED(this);
                util::register_lock(this);
                return true;
            }

            HPX_ITT_SYNC_CANCEL(this);
            return false;
        }

        void unlock()
        {
            // ...leave the critical section
            HPX_ITT_SYNC_RELEASING(this);

            // check whether this thread's local node's next field is null
            if (local_node_.data_.next_ == nullptr)
            {
                // If so, then either:
                //
                //  1. no other thread is contending for the lock
                //  2. there is a another thread about to acquire the lock
                //
                // To distinguish between these cases atomic compare exchange
                // the tail field. If the call succeeds, then no other thread
                // is trying to acquire the lock, tail is set to nullptr, and
                // unlock() returns.
                //
                detail::spinlock_mcs_node* p = &local_node_.data_;
                if (tail_.compare_exchange_strong(p, nullptr,
                        std::memory_order_release, std::memory_order_relaxed))
                {
                    HPX_ITT_SYNC_RELEASED(this);
                    util::unregister_lock(this);
                    return;
                }

                // otherwise, another thread is in the process of trying to
                // acquire the lock, so spin waiting for it to finish
                util::ignore_while_checking<spinlock> ignore(this);
                for (std::size_t k = 0; local_node_.data_.next_ == nullptr; ++k)
                {
                    util::detail::yield_k(k,
                        "hpx::lcos::local::spinlock::unlock",
                        hpx::threads::pending);
                }
            }

            // Once the successor has appeared, the unlock() method sets its
            // successor's locked field to false, indicating that the lock is
            // now available.
            local_node_.data_.next_->locked_ = false;

            // At this point no other thread can access this node and it can
            // be reused.
            local_node_.data_.next_ = nullptr;

            HPX_ITT_SYNC_RELEASED(this);
            util::unregister_lock(this);
        }

        // needed for util::ignore_while_checking<> above
        spinlock const* mutex() const { return this; }

#endif // HPX_HAVE_SPINLOCK_MCS
    };

}}}

#endif // HPX_B3A83B49_92E0_4150_A551_488F9F5E1113

