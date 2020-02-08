//  Copyright (c) 2019 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_DEBUGGING_PRINT_HPP)
#define HPX_DEBUGGING_PRINT_HPP

#include <hpx/config.hpp>
//#include <hpx/runtime/threads/thread.hpp>
//#include <hpx/runtime/threads/thread_data_.hpp>

#include <array>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__linux) || defined(linux) || defined(__linux__)
#include <linux/unistd.h>
#include <sys/mman.h>
#define DEBUGGING_PRINT_LINUX
#endif

#undef HPX_HAVE_CXX17_FOLD_EXPRESSIONS

#define LOG_EXCLUSIVE(x)
//
#define FUNC_START_DEBUG_MSG LOG_TRACE_MSG("*** Enter " << __func__);
#define FUNC_END_DEBUG_MSG LOG_TRACE_MSG("### Exit  " << __func__);
//
#define LOG_FORMAT_MSG(x)
#define LOG_DEBUG_MSG(x)
#define LOG_TRACE_MSG(...)
#define LOG_INFO_MSG(x)
#define LOG_WARN_MSG(x)
#define LOG_ERROR_MSG(x)
#define LOG_FATAL_MSG(x) LOG_ERROR_MSG(x)
//
#define LOG_DEVEL_MSG(x)
//
#define LOG_TIMED_INIT(name)
#define LOG_TIMED_MSG(name, level, delay, x)
#define LOG_TIMED_BLOCK(name, level, delay, x)

// ------------------------------------------------------------
// This file provides a simple to use printf style debugging
// tool that can be used on a per file basis to enable ouput.
// It is not intended to be exposed to users, but rather as
// an aid for hpx development.
// ------------------------------------------------------------
// Usage: Instantiate a debug print object at the top of a file
// using a template param of true/false to enable/disable output
// when the template parameter is false, the optimizer will
// not produce code and so the impact is nil.
//
// static hpx::debug::enable_print<true> spq_deb("SUBJECT");
//
// Later in code you may print information using
//
//             spq_deb.debug(str<16>("cleanup_terminated"), "v1"
//                  , "D" , dec<2>(domain_num)
//                  , "Q" , dec<3>(q_index)
//                  , "thread_num", dec<3>(local_num));
//
// various print formatters (dec/hex/str) are supplied to make
// the output regular and aligned for easy parsing/scanning.
//
// In tight loops, huge amounts of debug information might be
// produced, so a simple timer based output is provided
// To instantiate a timed output
//      static auto getnext = spq_deb.make_timer(1
//              , str<16>("get_next_thread"));
// then inside a tight loop
//      spq_deb.timed(getnext, dec<>(thread_num));
// The output will only be produced every N seconds
// ------------------------------------------------------------

// ------------------------------------------------------------
/// \cond NODETAIL
namespace hpx { namespace debug {

    // ------------------------------------------------------------------
    // format as zero padded int
    // ------------------------------------------------------------------
    namespace detail {

        template <int N, typename T>
        struct dec
        {
            dec(T const& v)
              : data_(v)
            {
            }

            T const& data_;

            friend std::ostream& operator<<(
                std::ostream& os, dec<N, T> const& d)
            {
                os << std::right << std::setfill('0') << std::setw(N)
                   << std::noshowbase << std::dec << d.data_;
                return os;
            }
        };
    }    // namespace detail

    template <int N = 2, typename T>
    detail::dec<N, T> dec(T const& v)
    {
        return detail::dec<N, T>(v);
    }

    // ------------------------------------------------------------------
    // format as pointer
    // ------------------------------------------------------------------
    struct ptr
    {
        ptr(void const* v)
          : data_(v)
        {
        }
        ptr(std::uintptr_t const v)
          : data_(reinterpret_cast<void const*>(v))
        {
        }
        void const* data_;
        friend std::ostream& operator<<(std::ostream& os, const ptr& d)
        {
            os << d.data_;
            return os;
        }
    };

    // ------------------------------------------------------------------
    // format as zero padded hex
    // ------------------------------------------------------------------
    template <int N = 4, typename T = int, typename Enable = void>
    struct hex;

    template <int N, typename T>
    struct hex<N, T, typename std::enable_if<!std::is_pointer<T>::value>::type>
    {
        hex(const T& v)
          : data_(v)
        {
        }
        const T& data_;
        friend std::ostream& operator<<(std::ostream& os, const hex<N, T>& d)
        {
            os << std::right << "0x" << std::setfill('0') << std::setw(N)
               << std::noshowbase << std::hex << d.data_;
            return os;
        }
    };

    template <int N, typename T>
    struct hex<N, T, typename std::enable_if<std::is_pointer<T>::value>::type>
    {
        hex(const void* v)
          : data_(v)
        {
        }
        const void* data_;
        friend std::ostream& operator<<(std::ostream& os, const hex<N, T>& d)
        {
            os << std::right << std::setw(N) << std::noshowbase << d.data_;
            return os;
        }
    };

    // ------------------------------------------------------------------
    // format as binary bits
    // ------------------------------------------------------------------
    template <int N = 8, typename T = int>
    struct bin
    {
        bin(const T& v)
          : data_(v)
        {
        }
        const T& data_;
        friend std::ostream& operator<<(std::ostream& os, const bin<N, T>& d)
        {
            os << std::bitset<N>(d.data_);
            return os;
        }
    };

    // ------------------------------------------------------------------
    // format as padded string
    // ------------------------------------------------------------------
    template <int N = 20>
    struct str
    {
        str(const char* v)
          : data_(v)
        {
        }
        const char* data_;
        friend std::ostream& operator<<(std::ostream& os, const str<N>& d)
        {
            os << std::left << std::setfill(' ') << std::setw(N) << d.data_;
            return os;
        }
    };

    // ------------------------------------------------------------------
    // format as ip address
    // ------------------------------------------------------------------
    struct ipaddr
    {
        ipaddr(const void* a)
          : data_(reinterpret_cast<const uint8_t*>(a))
        {
        }
        const uint8_t* data_;
        friend std::ostream& operator<<(std::ostream& os, const ipaddr& p)
        {
            os << std::dec << int((reinterpret_cast<const uint8_t*>(&p))[0])
               << "." << int((reinterpret_cast<const uint8_t*>(&p))[1]) << "."
               << int((reinterpret_cast<const uint8_t*>(&p))[2]) << "."
               << int((reinterpret_cast<const uint8_t*>(&p))[3]);
            return os;
        }
    };

    // ------------------------------------------------------------------
    // helper fuction for printing CRC32
    // ------------------------------------------------------------------
    inline uint32_t crc32(const void* address, size_t length)
    {
        //        boost::crc_32_type result;
        //        result.process_bytes(address, length);
        //        return result.checksum();
        return 0;
    }

    // ------------------------------------------------------------------
    // helper fuction for printing short memory dump and crc32
    // useful for debugging corruptions in buffers during parcelport
    // rma or other transfers
    // ------------------------------------------------------------------
    struct mem_crc32
    {
        mem_crc32(const void* a, std::size_t len, const char* txt)
          : addr_(reinterpret_cast<const uint64_t*>(a))
          , len_(len)
          , txt_(txt)
        {
        }
        const uint64_t* addr_;
        const std::size_t len_;
        const char* txt_;
        friend std::ostream& operator<<(std::ostream& os, const mem_crc32& p)
        {
            const uint64_t* uintBuf = static_cast<const uint64_t*>(p.addr_);
            os << "Memory:";
            os << " address " << hpx::debug::ptr(p.addr_) << " length "
               << hpx::debug::hex<8>(p.len_) << " CRC32:\n"
               << hpx::debug::hex<8>(crc32(p.addr_, p.len_)) << " ";
            for (size_t i = 0; i < (std::min)(p.len_ / 8, size_t(128)); i++)
            {
                os << hpx::debug::hex<16>(*uintBuf++) << " ";
            }
            os << " : " << p.txt_;
            return os;
        }
    };

    // ------------------------------------------------------------------
    // safely dump thread pointer/description
    // ------------------------------------------------------------------
    template <typename T>
    struct threadinfo
    {
        threadinfo(const T& v) {}

        friend std::ostream& operator<<(std::ostream& os, const threadinfo& d)
        {
            os << "\"<Unknown>\"";
            return os;
        }
    };

#ifdef DEBUG_PRINT_HAS_THREADS
    template <>
    struct threadinfo<threads::thread_data_*>
    {
        threadinfo(const threads::thread_data_* v)
          : data_(v)
        {
        }
        const threads::thread_data_* data_;
        friend std::ostream& operator<<(std::ostream& os, const threadinfo& d)
        {
            os << ptr(d.data_) << " \""
               << ((d.data_ != nullptr) ? d.data_->get_description() :
                                          "nullptr")
               << "\"";
            return os;
        }
    };

    template <>
    struct threadinfo<threads::thread_id_type*>
    {
        threadinfo(const threads::thread_id_type* v)
          : data_(v)
        {
        }
        const threads::thread_id_type* data_;
        friend std::ostream& operator<<(std::ostream& os, const threadinfo& d)
        {
            if (d.data_ == nullptr)
                os << "nullptr";
            else
                os << threadinfo<threads::thread_data_*>(
                    get_thread_id_data_(*d.data_));
            return os;
        }
    };

    template <>
    struct threadinfo<hpx::threads::thread_init_data_>
    {
        threadinfo(const hpx::threads::thread_init_data_& v)
          : data_(v)
        {
        }
        const hpx::threads::thread_init_data_& data_;
        friend std::ostream& operator<<(std::ostream& os, const threadinfo& d)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            os << std::left << " \"" << d.data_.description.get_description()
               << "\"";
#else
            os << "??? " << /*hex<8,uintptr_t>*/ (uintptr_t(&d.data_));
#endif
            return os;
        }
    };
#endif

    namespace detail {
        // ------------------------------------------------------------------
        // helper class for printing thread ID, either std:: or hpx::
        // ------------------------------------------------------------------
        struct current_thread_print_helper
        {
        };

        inline std::ostream& operator<<(
            std::ostream& os, const current_thread_print_helper&)
        {
#ifdef DEBUG_PRINT_HAS_THREADS
            if (hpx::threads::get_self_id() == hpx::threads::invalid_thread_id)
            {
                os << "-------------- ";
            }
            else
            {
                hpx::threads::thread_data_* dummy =
                    hpx::threads::get_self_id_data_();
                os << dummy << " ";
            }
#endif
            os << hex<12, std::thread::id>(std::this_thread::get_id())
#ifdef DEBUGGING_PRINT_LINUX
               << " cpu " << debug::dec<3, int>(sched_getcpu()) << " ";
#else
               << " cpu "
               << "--- ";
#endif
            return os;
        }

        // ------------------------------------------------------------------
        // helper class for printing time since start
        // ------------------------------------------------------------------
        struct current_time_print_helper
        {
        };

        inline std::ostream& operator<<(
            std::ostream& os, const current_time_print_helper&)
        {
            using namespace std::chrono;
            static high_resolution_clock::time_point log_t_start =
                high_resolution_clock::now();
            //
            auto now = high_resolution_clock::now();
            auto nowt = duration_cast<microseconds>(now - log_t_start).count();
            //
            os << debug::dec<10>(nowt) << " ";
            return os;
        }

#ifdef HPX_HAVE_CXX17_FOLD_EXPRESSIONS
        template <typename TupleType, std::size_t... I>
        void tuple_print(
            std::ostream& os, const TupleType& tup, std::index_sequence<I...>)
        {
            (..., (os << (I == 0 ? "" : " ") << std::get<I>(tup)));
        }

        template <typename... Args>
        void tuple_print(std::ostream& os,
            const std::tuple<std::forward<Args>(args)...>& tup)
        {
            tuple_print(os, tup, std::make_index_sequence<sizeof...(Args)>());
        }

        template <typename... Args>
        void display(const char* prefix, std::forward<Args>(args)... args)
        {
            // using a temp stream object with a single copy to cout at the end
            // prevents multiple threads from injecting overlapping text
            std::stringstream tempstream;
            tempstream << prefix << detail::current_time_print_helper()
                       << detail::current_thread_print_helper();
            ((tempstream << args << " "), ...);
            tempstream << std::endl;
            std::cout << tempstream.str();
        }

#else
        // C++14 version
        // helper function to print a tuple of any size
        template <class TupleType, std::size_t N>
        struct tuple_printer
        {
            static void print(std::ostream& os, const TupleType& t)
            {
                tuple_printer<TupleType, N - 1>::print(t);
                os << ", " << std::get<N - 1>(t);
            }
        };

        template <class TupleType>
        struct tuple_printer<TupleType, 1>
        {
            static void print(std::ostream& os, const TupleType& t)
            {
                os << std::get<0>(t);
            }
        };

        template <typename Arg, typename... Args>
        void variadic_print(std::ostream& os, Arg arg, const Args&... args)
        {
            os << arg;
            using expander = int[];
            (void) expander{0, (void(os << ' ' << args), 0)...};
        }

        template <class... Args>
        void tuple_print(std::ostream& os, const std::tuple<Args...>& t)
        {
            tuple_printer<decltype(t), sizeof...(Args)>::print(os, t);
        }

        template <class... Args>
        void tuple_print(std::ostream& os, const Args&... args)
        {
            variadic_print(os, args...);
        }

        template <typename... Args>
        void display(const char* prefix, const Args&... args)
        {
            // using a temp stream object with a single copy to cout at the end
            // prevents multiple threads from injecting overlapping text
            std::stringstream tempstream;
            tempstream << prefix << detail::current_time_print_helper()
                       << detail::current_thread_print_helper();
            variadic_print(tempstream, args...);
            tempstream << std::endl;
            std::cout << tempstream.str();
        }
#endif

        template <typename... Args>
        void debug(const Args&... args)
        {
            display("<DEB> ", args...);
        }

        template <typename... Args>
        void warning(const Args&... args)
        {
            display("<WAR> ", args...);
        }

        template <typename... Args>
        void error(const Args&... args)
        {
            display("<ERR> ", args...);
        }

        template <typename... Args>
        void trace(const Args&... args)
        {
            display("<TRC> ", args...);
        }

        template <typename... Args>
        void timed(const Args&... args)
        {
            display("<TIM> ", args...);
        }
    }    // namespace detail

    template <typename T>
    struct init
    {
        T data_;
        init(const T& t)
          : data_(t)
        {
        }
        friend std::ostream& operator<<(std::ostream& os, const init<T>& d)
        {
            os << d.data_ << " ";
            return os;
        }
    };

    template <typename T>
    void set(init<T>& var, const T val)
    {
        var.data_ = val;
    }

    template <typename... Args>
    struct timed_init
    {
        mutable std::chrono::steady_clock::time_point time_start_;
        double delay_;
        std::tuple<Args...> message_;
        //
        timed_init(double delay, const Args&... args)
          : time_start_(std::chrono::steady_clock::now())
          , delay_(delay)
          , message_(std::forward<Args>(args)...)
        {
        }

        bool elapsed(const std::chrono::steady_clock::time_point& now) const
        {
            double elapsed_ =
                std::chrono::duration_cast<std::chrono::duration<double>>(
                    now - time_start_)
                    .count();

            if (elapsed_ > delay_)
            {
                time_start_ = now;
                return true;
            }
            return false;
        }

        friend std::ostream& operator<<(
            std::ostream& os, const timed_init<Args...>& ti)
        {
            detail::tuple_print(os, ti.message_);
            return os;
        }
    };

    template <bool enable>
    struct enable_print;

    // when false, debug statements should produce no code
    template <>
    struct enable_print<false>
    {
        HPX_CONSTEXPR enable_print(const char*) {}

        HPX_CONSTEXPR bool is_enabled() const
        {
            return false;
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void debug(const Args&...) const
        {
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void warning(const Args&...) const
        {
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void trace(const Args&...) const
        {
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void error(const Args&...) const
        {
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void timed(const Args&...) const
        {
        }

        template <typename T>
        HPX_CXX14_CONSTEXPR void array(
            const std::string& name, const std::vector<T>&) const
        {
        }

        template <typename T, std::size_t N>
        HPX_CXX14_CONSTEXPR void array(
            const std::string& name, const std::array<T, N>&) const
        {
        }

        template <typename Iter>
        HPX_CXX14_CONSTEXPR void array(
            const std::string& name, Iter begin, Iter end) const
        {
        }

        template <typename T, typename... Args>
        HPX_CXX14_CONSTEXPR bool declare_variable(const Args&...) const
        {
            return true;
        }

        // @todo, return void so that timers have zero footprint when disabled
        template <typename... Args>
        HPX_CONSTEXPR int make_timer(const double, const Args&...) const
        {
            return 0;
        }
    };

    // when true, debug statements produce valid output
    template <>
    struct enable_print<true>
    {
    private:
        const char* prefix_;

    public:
        HPX_CONSTEXPR enable_print(const char* p)
          : prefix_(p)
        {
        }

        HPX_CONSTEXPR bool is_enabled() const
        {
            return true;
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void debug(const Args&... args) const
        {
            detail::debug(prefix_, args...);
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void warning(const Args&... args) const
        {
            detail::warning(prefix_, args...);
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void trace(const Args&... args) const
        {
            detail::trace(prefix_, args...);
        }

        template <typename... Args>
        HPX_CXX14_CONSTEXPR void error(const Args&... args) const
        {
            detail::error(prefix_, args...);
        }

        template <typename... T, typename... Args>
        void timed(timed_init<T...>& init, const Args&... args) const
        {
            auto now = std::chrono::steady_clock::now();
            if (init.elapsed(now))
            {
                detail::timed(prefix_, init, args...);
            }
        }

        template <typename T>
        void array(const std::string& name, const std::vector<T>& v) const
        {
            std::cout << str<20>(name.c_str()) << ": {"
                      << debug::dec<4>(v.size()) << "} : ";
            std::copy(std::begin(v), std::end(v),
                std::ostream_iterator<T>(std::cout, ", "));
            std::cout << "\n";
        }

        template <typename T, std::size_t N>
        void array(const std::string& name, const std::array<T, N>& v) const
        {
            std::cout << str<20>(name.c_str()) << ": {"
                      << debug::dec<4>(v.size()) << "} : ";
            std::copy(std::begin(v), std::end(v),
                std::ostream_iterator<T>(std::cout, ", "));
            std::cout << "\n";
        }

        template <typename Iter>
        void array(const std::string& name, Iter begin, Iter end) const
        {
            std::cout << str<20>(name.c_str()) << ": {"
                      << debug::dec<4>(std::distance(begin, end)) << "} : ";
            std::copy(begin, end,
                std::ostream_iterator<
                    typename std::iterator_traits<Iter>::value_type>(
                    std::cout, ", "));
            std::cout << std::endl;
        }

        template <typename T>
        void set(init<T>& var, const T val) const
        {
            var.data_ = val;
        }

        template <typename T, typename... Args>
        T declare_variable(const Args&... args) const
        {
            return T(args...);
        }

        template <typename... Args>
        timed_init<Args...> make_timer(const double delay, const Args&... args)
        {
            return timed_init<Args...>(delay, args...);
        }
    };

}}    // namespace hpx::debug
/// \endcond

#endif
