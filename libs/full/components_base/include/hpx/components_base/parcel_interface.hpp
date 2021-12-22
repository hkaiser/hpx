//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (c) 2015-2016 Thomas Heller
//  Copyright (c) 2007 Richard D Guidry Jr
//  Copyright (c) 2007 Alexandre (aka Alex) TABBAL
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/coroutines/thread_enums.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/naming_base/address.hpp>
#include <hpx/naming_base/gid_type.hpp>
#include <hpx/naming_base/id_type.hpp>
#include <hpx/serialization/binary_filter.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset::detail {

    // abstract base class for parcels
    struct parcel_base
    {
        using split_gids_type =
            std::map<naming::gid_type const*, naming::gid_type>;

        virtual ~parcel_base() = default;

        virtual bool is_valid() const = 0;
        virtual void reset() = 0;

        virtual char const* get_action_name() const = 0;
        virtual int get_component_type() const = 0;
        virtual int get_action_type() const = 0;

        virtual naming::id_type source_id() const = 0;
        virtual void set_source_id(naming::id_type const& source_id) = 0;

        virtual void set_destination_id(naming::gid_type&& dest) = 0;
        virtual naming::gid_type const& destination() const = 0;

        virtual naming::address const& addr() const = 0;
        virtual naming::address& addr() = 0;

        virtual std::uint32_t destination_locality_id() const = 0;
        virtual naming::gid_type const& destination_locality() const = 0;

        virtual double start_time() const = 0;
        virtual void set_start_time(double time) = 0;
        virtual double creation_time() const = 0;

        virtual threads::thread_priority get_thread_priority() const = 0;

#if defined(HPX_HAVE_PARCEL_PROFILING)
        virtual naming::gid_type const& parcel_id() const = 0;
        virtual naming::gid_type& parcel_id() = 0;
#endif

        //virtual serialization::binary_filter* get_serialization_filter()
        //    const = 0;

        //policies::message_handler* get_message_handler(
        //    parcelset::parcelhandler* ph, locality const& loc) const;

        virtual bool does_termination_detection() const = 0;

        virtual split_gids_type move_split_gids() const = 0;
        virtual void set_split_gids(split_gids_type&& split_gids) = 0;

        virtual std::size_t num_chunks() const = 0;
        virtual std::size_t& num_chunks() = 0;

        virtual std::size_t size() const = 0;
        virtual std::size_t& size() = 0;

        virtual bool schedule_action(std::size_t num_thread) = 0;

        virtual bool load_schedule(serialization::input_archive& ar,
            std::size_t num_thread, bool& deferred_schedule) = 0;

        virtual naming::gid_type generate_unique_id(
            std::uint32_t locality_id) = 0;

        virtual std::string hpx_serialization_get_name() const = 0;

        virtual void load(serialization::input_archive& ar, unsigned n) = 0;
        virtual void save(
            serialization::output_archive& ar, unsigned n) const = 0;

        HPX_SERIALIZATION_SPLIT_MEMBER();
    };
}    // namespace hpx::parcelset::detail

namespace hpx::parcelset {

    class HPX_EXPORT parcel
    {
    private:
        using split_gids_type = typename detail::parcel_base::split_gids_type;

        bool is_valid() const;

    public:
        parcel();
        parcel(detail::parcel_base* p);

        ~parcel();

    public:
        parcel(parcel&& other) noexcept;
        parcel& operator=(parcel&& other) noexcept;

        void reset();

        char const* get_action_name() const;
        int get_component_type() const;
        int get_action_type() const;

        naming::id_type source_id() const;
        void set_source_id(naming::id_type const& source_id);
        void set_destination_id(naming::gid_type&& dest);

        naming::address const& addr() const;
        naming::address& addr();

        std::uint32_t destination_locality_id() const;

        naming::gid_type const& destination() const;
        naming::gid_type const& destination_locality() const;

        double start_time() const;
        void set_start_time(double time);
        double creation_time() const;

        threads::thread_priority get_thread_priority() const;

#if defined(HPX_HAVE_PARCEL_PROFILING)
        naming::gid_type const& parcel_id() const;
        naming::gid_type& parcel_id();
#endif

        serialization::binary_filter* get_serialization_filter() const;
        //policies::message_handler* get_message_handler(
        //    parcelset::parcelhandler* ph, locality const& loc) const;

        bool does_termination_detection() const;

        split_gids_type move_split_gids() const;
        void set_split_gids(split_gids_type&& split_gids);

        std::size_t num_chunks() const;
        std::size_t& num_chunks();

        std::size_t size() const;
        std::size_t& size();

        bool schedule_action(std::size_t num_thread = std::size_t(-1));

        // returns true if parcel was migrated, false if scheduled locally
        bool load_schedule(serialization::input_archive& ar,
            std::size_t num_thread, bool& deferred_schedule);

        // generate unique parcel id
        naming::gid_type generate_unique_id(
            std::uint32_t locality_id = naming::invalid_locality_id);

    private:
        // serialization support
        friend class hpx::serialization::access;

        void serialize(serialization::input_archive& ar, unsigned);
        void serialize(serialization::output_archive& ar, unsigned);

        std::unique_ptr<detail::parcel_base> data_;
    };

    HPX_EXPORT std::ostream& operator<<(std::ostream& os, parcel const& p);
    HPX_EXPORT std::string dump_parcel(parcel const& p);
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

#endif
