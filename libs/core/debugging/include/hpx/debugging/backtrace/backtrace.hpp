//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011-2023 Hartmut Kaiser
//  Copyright (c) 2010 Artyom Beilis (Tonkikh)
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    namespace stack_trace {

        [[nodiscard]] HPX_CORE_EXPORT std::size_t trace(
            void** addresses, std::size_t size);
        HPX_CORE_EXPORT void write_symbols(
            void* const* addresses, std::size_t size, std::ostream&);
        [[nodiscard]] HPX_CORE_EXPORT std::string get_symbol(void* address);
        [[nodiscard]] HPX_CORE_EXPORT std::string get_symbols(
            void* const* address, std::size_t size);
    }    // namespace stack_trace

    class HPX_CORE_EXPORT backtrace
    {
    public:
        explicit backtrace(
            std::size_t frames_no = HPX_HAVE_THREAD_BACKTRACE_DEPTH);

        backtrace(backtrace const&);
        backtrace(backtrace&&) noexcept;
        backtrace& operator=(backtrace const&);
        backtrace& operator=(backtrace&&) noexcept;

        virtual ~backtrace();

        [[nodiscard]] std::size_t stack_size() const noexcept;
        [[nodiscard]] void* return_address(std::size_t frame_no) const noexcept;
        void trace_line(std::size_t frame_no, std::ostream& out) const;
        [[nodiscard]] std::string trace_line(std::size_t frame_no) const;
        [[nodiscard]] std::string trace() const;
        void trace(std::ostream& out) const;

    private:
        std::vector<void*> frames_;
    };

    namespace details {

        class HPX_CORE_EXPORT trace_manip
        {
        public:
            explicit constexpr trace_manip(backtrace const* tr) noexcept
              : tr_(tr)
            {
            }

            std::ostream& write(std::ostream& out) const;

        private:
            backtrace const* tr_;
        };

        std::ostream& operator<<(
            std::ostream& out, details::trace_manip const& t);
    }    // namespace details

    HPX_CPP_EXPORT template <typename E>
    [[nodiscard]] details::trace_manip trace(E const& e)
    {
        auto const* tr = dynamic_cast<backtrace const*>(&e);
        return details::trace_manip(tr);
    }

    [[nodiscard]] HPX_CORE_EXPORT std::string trace(
        std::size_t frames_no = HPX_HAVE_THREAD_BACKTRACE_DEPTH);
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
