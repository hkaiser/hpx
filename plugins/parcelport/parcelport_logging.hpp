//  Copyright (c) 2014-2017 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_POLICIES_LOGGING
#define HPX_PARCELSET_POLICIES_LOGGING

#include <bitset>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
//
#include <hpx/config.hpp>
#include <hpx/config/parcelport_defines.hpp>
//
#include <hpx/debugging/print.hpp>

// ------------------------------------------------------------------
// useful macros for formatting log messages
// ------------------------------------------------------------------
#define binary8(p)  "0b" << std::bitset<8>(uint8_t(p)) << " "
#define binary32(p) "0b" << std::bitset<32>(uint32_t(p)) << " "
#define nhex(n)                                                                \
    "0x" << std::setfill('0')  << std::setw(n) << std::noshowbase << std::hex
#define hexpointer(p) nhex(16) << uintptr_t(p) << " "
#define hexuint64(p) nhex(16)  << uintptr_t(p) << " "
#define hexuint32(p) nhex(8)   << uint32_t(p)  << " "
#define hexlength(p) nhex(6)   << uintptr_t(p) << " "
#define hexnumber(p) nhex(4)   << uintptr_t(p) << " "
#define hexbyte(p) nhex(2)     << int32_t(p)   << " "
#define decimal(n)                                                             \
    std::setfill('0') << std::setw(n) << std::noshowbase << std::dec
#define decnumber(p) std::dec << p << " "
#define dec4(p) decimal(4) << p << " "
#define ipaddress(p)                                                           \
    std::dec << int( (reinterpret_cast<const uint8_t*>(&p))[0] ) << "."        \
             << int( (reinterpret_cast<const uint8_t*>(&p))[1] ) << "."        \
             << int( (reinterpret_cast<const uint8_t*>(&p))[2] ) << "."        \
             << int( (reinterpret_cast<const uint8_t*>(&p))[3] )
#define sockaddress(p) ipaddress(((struct sockaddr_in*) (p))->sin_addr.s_addr)
#define iplocality(p)  ipaddress(p.ip_address()) << ":" << decnumber(p.port()) \
    << "(" << std::dec << p.fi_address() << ") "

// ------------------------------------------------------------------
// helper classes/functions used in logging
// ------------------------------------------------------------------
namespace hpx {
namespace parcelset {
namespace policies {
namespace libfabric {
namespace detail {

#ifdef HPX_PARCELPORT_LOGGING_HAVE_TRACE_LOG
    // ------------------------------------------------------------------
    // helper fuction for printing CRC32
    // ------------------------------------------------------------------
    inline uint32_t crc32(const void *address, size_t length)
    {
        boost::crc_32_type result;
        result.process_bytes(address, length);
        return result.checksum();
    }

        // ------------------------------------------------------------------
        // helper fuction for printing CRC32 and short memory dump
        // ------------------------------------------------------------------
        inline std::string mem_crc32(
            const void* address, size_t length, const char* txt)
        {
            const uint64_t* uintBuf = static_cast<const uint64_t*>(address);
            std::stringstream temp;
            temp << "Memory: ";
            temp << "address " << hexpointer(address) << "length "
                 << hexuint32(length)
                 << "CRC32: " << hexuint32(crc32(address, length));
            for (size_t i = 0; i < (std::min)(length / 8, size_t(128)); i++)
            {
                temp << hexuint64(*uintBuf++);
            }
            temp << ": " << txt;
            return temp.str();
        }
    }
#endif

}}}}}    // namespace hpx::parcelset::policies::libfabric::detail


#define LOG_TRACE_MSG(x)
#define LOG_EXCLUSIVE(x)
//
#define FUNC_START_DEBUG_MSG LOG_TRACE_MSG("*** Enter " << __func__);
#define FUNC_END_DEBUG_MSG LOG_TRACE_MSG("### Exit  " << __func__);
//
#define LOG_FORMAT_MSG(x)
#define LOG_DEBUG_MSG(x)
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

#endif
