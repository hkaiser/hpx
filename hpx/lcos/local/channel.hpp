//  Copyright (c) 2016 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_LCOS_LOCAL_CHANNEL_JUL_23_2016_0707PM)
#define HPX_LCOS_LOCAL_CHANNEL_JUL_23_2016_0707PM

#include <hpx/config.hpp>
#include <hpx/exception.hpp>
#include <hpx/lcos/future.hpp>
#include <hpx/lcos/local/no_mutex.hpp>
#include <hpx/lcos/local/receive_buffer.hpp>
#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/atomic_count.hpp>
#include <hpx/util/iterator_facade.hpp>
#include <hpx/util/scoped_unlock.hpp>
#include <hpx/util/unused.hpp>

#include <boost/exception_ptr.hpp>
#include <boost/intrusive_ptr.hpp>

#include <cstdlib>
#include <iterator>
#include <mutex>
#include <type_traits>
#include <utility>

namespace hpx { namespace lcos { namespace local
{
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct channel_base
        {
            channel_base()
              : count_(0)
            {}

            virtual ~channel_base() {}

            virtual hpx::future<T> get(std::size_t generation,
                bool blocking = false) = 0;
            virtual bool try_get(std::size_t generation,
                hpx::future<T>* f = nullptr) = 0;
            virtual void close() = 0;

            long use_count() const { return count_; }
            long addref() { return ++count_; }
            long release() { return --count_; }

        private:
            hpx::util::atomic_count count_;
        };

        template <typename T>
        void intrusive_ptr_add_ref(channel_base<T>* p)
        {
            p->addref();
        }

        template <typename T>
        void intrusive_ptr_release(channel_base<T>* p)
        {
            if (0 == p->release())
                delete p;
        }

        template <typename T>
        struct channel_set_base : channel_base<T>
        {
            virtual void set(std::size_t generation, T && t) = 0;
        };

        template <>
        struct channel_set_base<void> : channel_base<void>
        {
            virtual void set(std::size_t generation) = 0;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        class unlimited_channel_base : public channel_set_base<T>
        {
        protected:
            typedef hpx::lcos::local::spinlock mutex_type;

            HPX_NON_COPYABLE(unlimited_channel_base);

        public:
            unlimited_channel_base()
              : closed_(false)
            {}

        protected:
            hpx::future<T> get(std::size_t generation, bool blocking)
            {
                std::unique_lock<mutex_type> l(mtx_);

                if (buffer_.empty())
                {
                    if (closed_)
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::invalid_status,
                                "hpx::lcos::local::channel::get",
                                "this channel is empty and was closed"));
                    }

                    if (blocking && this->use_count() == 1)
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::invalid_status,
                                "hpx::lcos::local::channel::get",
                                "this channel is empty and is not accessible "
                                "by any other thread causing a deadlock"));
                    }
                }

                ++get_generation_;
                if (generation == std::size_t(-1))
                    generation = get_generation_;

                if (closed_)
                {
                    // the requested item must be available, otherwise this
                    // would create a deadlock
                    hpx::future<T> f;
                    if (!buffer_.try_receive(generation, &f))
                    {
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::invalid_status,
                                "hpx::lcos::local::channel::get",
                                "this channel is closed and the requested value"
                                "has not been received yet"));
                    }
                    return f;
                }

                return buffer_.receive(generation);
            }

            bool try_get(std::size_t generation, hpx::future<T>* f = nullptr)
            {
                std::lock_guard<mutex_type> l(mtx_);

                if (buffer_.empty() && closed_)
                    return false;

                ++get_generation_;
                if (generation == std::size_t(-1))
                    generation = get_generation_;

                if (f != nullptr)
                    *f = buffer_.receive(generation);

                return true;
            }

            void close()
            {
                std::unique_lock<mutex_type> l(mtx_);
                if(closed_)
                {
                    HPX_THROW_EXCEPTION(hpx::invalid_status,
                        "hpx::lcos::local::channel::close",
                        "attempting to close an already closed channel");
                }

                closed_ = true;

                if (buffer_.empty())
                    return;

                boost::exception_ptr e;

                {
                    util::scoped_unlock<std::unique_lock<mutex_type> > ul(l);
                    e = HPX_GET_EXCEPTION(hpx::future_cancelled,
                            "hpx::lcos::local::close",
                            "canceled waiting on this entry");
                }

                // all pending requests which can't be satisfied have to be
                // canceled at this point
                buffer_.cancel_waiting(e);
            }

        protected:
            mutable mutex_type mtx_;
            receive_buffer<T, no_mutex> buffer_;
            std::size_t get_generation_;
            std::size_t set_generation_;
            bool closed_;
        };

        template <typename T>
        class unlimited_channel : public unlimited_channel_base<T>
        {
            typedef typename unlimited_channel_base<T>::mutex_type mutex_type;

            HPX_NON_COPYABLE(unlimited_channel);

        public:
            unlimited_channel() {}

            void set(std::size_t generation, T && t)
            {
                std::lock_guard<mutex_type> l(this->mtx_);
                if (this->closed_)
                {
                    HPX_THROW_EXCEPTION(hpx::invalid_status,
                        "hpx::lcos::local::channel::set",
                        "attempting to write to a closed channel");
                }

                ++this->set_generation_;
                if (generation == std::size_t(-1))
                    generation = this->set_generation_;

                buffer_.store_received(generation, std::move(t));
            }
        };

        template <>
        class unlimited_channel<void> : public unlimited_channel_base<void>
        {
            typedef typename unlimited_channel_base<void>::mutex_type mutex_type;

            HPX_NON_COPYABLE(unlimited_channel);
        public:
            unlimited_channel() {}

            void set(std::size_t generation)
            {
                std::lock_guard<mutex_type> l(this->mtx_);
                if (this->closed_)
                {
                    HPX_THROW_EXCEPTION(hpx::invalid_status,
                        "hpx::lcos::local::channel::set",
                        "attempting to write to a closed channel");
                }

                ++this->set_generation_;
                if (generation == std::size_t(-1))
                    generation = this->set_generation_;

                buffer_.store_received(generation);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T> class channel;
        template <typename T> class receive_channel;
        template <typename T> class send_channel;

        template <typename T>
        std::pair<future<T>, bool> make_default_data()
        {
            return std::make_pair(make_ready_future(T()), false);
        }

        template <>
        std::pair<future<void>, bool> make_default_data<void>()
        {
            return std::make_pair(make_ready_future(), false);
        }
    }

    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    class channel_iterator
        : public hpx::util::iterator_facade<
            channel_iterator<T>, future<T>,
            std::input_iterator_tag>
    {
        typedef hpx::util::iterator_facade<
                channel_iterator<T>, future<T>,
                std::input_iterator_tag
            > base_type;

    public:
        channel_iterator()
          : channel_(nullptr), data_(detail::make_default_data<T>())
        {}

        inline explicit channel_iterator(detail::channel<T> const* c);
        inline explicit channel_iterator(detail::receive_channel<T> const* c);

    private:
        std::pair<future<T>, bool> get_checked() const
        {
            hpx::future<T> f;
            if (!channel_->try_get(std::size_t(-1), &f))
                return detail::make_default_data<T>();
            return std::make_pair(std::move(f), true);
        }

        friend class hpx::util::iterator_core_access;

        bool equal(channel_iterator const& rhs) const
        {
            return (channel_ == rhs.channel_ && data_.second == rhs.data_.second) ||
                (!data_.second && rhs.channel_ == nullptr) ||
                (channel_ == nullptr && !rhs.data_.second);
        }

        void increment()
        {
            if (channel_)
                data_ = get_checked();
        }

        typename base_type::reference dereference() const
        {
            HPX_ASSERT(data_.second);
            return data_.first;
        }

    private:
        boost::intrusive_ptr<detail::channel_base<T> > channel_;
        mutable std::pair<future<T>, bool> data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename T>
        class channel
        {
        public:
            channel()
              : channel_(new unlimited_channel<T>())
            {}

            hpx::future<T>
            get(std::size_t generation = std::size_t(-1)) const
            {
                return channel_->get(generation);
            }
            T get_sync(std::size_t generation = std::size_t(-1)) const
            {
                return channel_->get(generation, true).get();
            }

            std::pair<future<T>, bool>
            get_checked(std::size_t generation = std::size_t(-1)) const
            {
                hpx::future<T> f;
                if (!channel_->try_get(generation, &f))
                    return make_default_data<T>();
                return std::make_pair(std::move(f), true);
            }

            template <typename U, typename U2 = T, typename Enable =
                typename std::enable_if<!std::is_void<U2>::value>::type>
            void set(U val, std::size_t generation = std::size_t(-1))
            {
                channel_->set(generation, std::move(val));
            }

            template <typename U = T, typename Enable =
                typename std::enable_if<std::is_void<U>::value>::type>
            void set(std::size_t generation = std::size_t(-1))
            {
                channel_->set(generation);
            }

            void close()
            {
                channel_->close();
            }

            channel_iterator<T> begin() const
            {
                return channel_iterator<T>(this);
            }
            channel_iterator<T> end() const
            {
                return channel_iterator<T>();
            }

            channel_iterator<T> rbegin() const
            {
                return channel_iterator<T>(this);
            }
            channel_iterator<T> rend() const
            {
                return channel_iterator<T>();
            }

        private:
            friend class channel_iterator<T>;
            friend class receive_channel<T>;
            friend class send_channel<T>;

        private:
            boost::intrusive_ptr<channel_set_base<T> > channel_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        class receive_channel
        {
        public:
            receive_channel(channel<T> const& c)
              : channel_(c.channel_)
            {}

            hpx::future<T> get(std::size_t generation = std::size_t(-1)) const
            {
                return channel_->get(generation);
            }
            T get_sync(std::size_t generation = std::size_t(-1)) const
            {
                return channel_->get(generation, true).get();
            }

            std::pair<future<T>, bool>
            get_checked(std::size_t generation = std::size_t(-1)) const
            {
                hpx::future<T> f;
                if (!channel_->try_get(generation, &f))
                    return make_default_data<T>();
                return std::make_pair(std::move(f), true);
            }

            channel_iterator<T> begin() const
            {
                return channel_iterator<T>(this);
            }
            channel_iterator<T> end() const
            {
                return channel_iterator<T>();
            }

            channel_iterator<T> rbegin() const
            {
                return channel_iterator<T>(this);
            }
            channel_iterator<T> rend() const
            {
                return channel_iterator<T>();
            }

        private:
            friend class channel_iterator<T>;

        private:
            boost::intrusive_ptr<channel_base<T> > channel_;
        };

        ///////////////////////////////////////////////////////////////////////////
        template <typename T>
        class send_channel
        {
        public:
            send_channel(channel<T> const& c)
              : channel_(c.channel_)
            {}

            template <typename U, typename U2 = T, typename Enable =
                typename std::enable_if<!std::is_void<U2>::value>::type>
            void set(U val, std::size_t generation = std::size_t(-1))
            {
                channel_->set(generation, std::move(val));
            }

            template <typename U = T, typename Enable =
                typename std::enable_if<std::is_void<U>::value>::type>
            void set(std::size_t generation = std::size_t(-1))
            {
                channel_->set(generation);
            }

            void close()
            {
                channel_->close();
            }

        private:
            friend class channel_iterator<T>;

        private:
            boost::intrusive_ptr<channel_set_base<T> > channel_;
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline channel_iterator<T>::channel_iterator(detail::channel<T> const* c)
        : channel_(c ? c->channel_ : nullptr),
        data_(c ? get_checked() : detail::make_default_data<T>())
    {}

    template <typename T>
    inline channel_iterator<T>::channel_iterator(
            detail::receive_channel<T> const* c)
        : channel_(c ? c->channel_ : nullptr),
        data_(c ? get_checked() : detail::make_default_data<T>())
    {}

    ///////////////////////////////////////////////////////////////////////////
    template <typename T = void>
    struct channel : public detail::channel<T>
    {
        channel() {}
    };

    template <typename T = void>
    struct receive_channel : public detail::receive_channel<T>
    {
        receive_channel(channel<T> const& c)
          : detail::receive_channel<T>(c)
        {}
    };

    template <typename T = void>
    struct send_channel : public detail::send_channel<T>
    {
        send_channel(channel<T> const& c)
          : detail::send_channel<T>(c)
        {}
    };
}}}

#endif
